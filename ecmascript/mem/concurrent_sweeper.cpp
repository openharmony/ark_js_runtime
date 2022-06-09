/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/mem/concurrent_sweeper.h"

#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
ConcurrentSweeper::ConcurrentSweeper(Heap *heap, EnableConcurrentSweepType type)
    : heap_(heap),
      enableType_(type)
{
}

void ConcurrentSweeper::Sweep(bool fullGC)
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ConcurrentSweepingInitialize);
    if (ConcurrentSweepEnabled()) {
        // Add all region to region list. Ensure all task finish
        if (!fullGC) {
            heap_->GetOldSpace()->PrepareSweeping();
        }
        heap_->GetNonMovableSpace()->PrepareSweeping();
        heap_->GetMachineCodeSpace()->PrepareSweeping();
        // Prepare
        isSweeping_ = true;
        startSpaceType_ = fullGC ? NON_MOVABLE : OLD_SPACE;
        for (int type = startSpaceType_; type < FREE_LIST_NUM; type++) {
            remainingTaskNum_[type] = FREE_LIST_NUM - startSpaceType_;
        }
        if (!fullGC) {
            Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(this, OLD_SPACE));
        }
        Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(this, NON_MOVABLE));
        Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(this, MACHINE_CODE_SPACE));
    } else {
        if (!fullGC) {
            heap_->GetOldSpace()->Sweep();
        }
        heap_->GetNonMovableSpace()->Sweep();
        heap_->GetMachineCodeSpace()->Sweep();
    }
    heap_->GetHugeObjectSpace()->Sweep(ConcurrentSweepEnabled());
}

void ConcurrentSweeper::AsyncSweepSpace(MemSpaceType type, bool isMain)
{
    auto space = heap_->GetSpaceWithType(type);
    space->AsyncSweep(isMain);

    os::memory::LockHolder holder(mutexs_[type]);
    if (--remainingTaskNum_[type] == 0) {
        cvs_[type].SignalAll();
    }
}

void ConcurrentSweeper::WaitAllTaskFinished()
{
    if (!isSweeping_) {
        return;
    }
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
        if (remainingTaskNum_[i] > 0) {
            os::memory::LockHolder holder(mutexs_[i]);
            while (remainingTaskNum_[i] > 0) {
                cvs_[i].Wait(&mutexs_[i]);
            }
        }
    }
}

void ConcurrentSweeper::EnsureAllTaskFinished()
{
    CHECK_JS_THREAD(heap_->GetEcmaVM());
    if (!isSweeping_) {
        return;
    }
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
        WaitingTaskFinish(static_cast<MemSpaceType>(i));
    }
    isSweeping_ = false;
    if (IsRequestDisabled()) {
        enableType_ = EnableConcurrentSweepType::DISABLE;
    }
}

void ConcurrentSweeper::EnsureTaskFinished(MemSpaceType type)
{
    CHECK_JS_THREAD(heap_->GetEcmaVM());
    if (!isSweeping_) {
        return;
    }
    WaitingTaskFinish(type);
}

void ConcurrentSweeper::WaitingTaskFinish(MemSpaceType type)
{
    if (remainingTaskNum_[type] > 0) {
        {
            os::memory::LockHolder holder(mutexs_[type]);
            remainingTaskNum_[type]++;
        }
        AsyncSweepSpace(type, true);
        os::memory::LockHolder holder(mutexs_[type]);
        while (remainingTaskNum_[type] > 0) {
            cvs_[type].Wait(&mutexs_[type]);
        }
    }
    FinishSweeping(type);
}

void ConcurrentSweeper::FinishSweeping(MemSpaceType type)
{
    SparseSpace *space = heap_->GetSpaceWithType(type);
    space->FillSweptRegion();
}

void ConcurrentSweeper::TryFillSweptRegion()
{
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
        FinishSweeping(static_cast<MemSpaceType>(i));
    }
    if (ConcurrentSweepEnabled()) {
        heap_->GetHugeObjectSpace()->FinishConcurrentSweep();
    }
}

void ConcurrentSweeper::ClearRSetInRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd)
{
    if (ConcurrentSweepEnabled()) {
        current->AtomicClearSweepingRSetInRange(freeStart, freeEnd);
        current->AtomicClearCrossRegionRSetInRange(freeStart, freeEnd);
    } else {
        current->ClearOldToNewRSetInRange(freeStart, freeEnd);
        current->ClearCrossRegionRSetInRange(freeStart, freeEnd);
    }
}

bool ConcurrentSweeper::SweeperTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    int sweepTypeNum = FREE_LIST_NUM - sweeper_->startSpaceType_;
    for (size_t i = sweeper_->startSpaceType_; i < FREE_LIST_NUM; i++) {
        auto type = static_cast<MemSpaceType>(((i + type_) % sweepTypeNum) + sweeper_->startSpaceType_);
        sweeper_->AsyncSweepSpace(type, false);
    }
    return true;
}

void ConcurrentSweeper::EnableConcurrentSweep(EnableConcurrentSweepType type)
{
    if (IsConfigDisabled()) {
        return;
    }
    if (ConcurrentSweepEnabled() && isSweeping_ && type == EnableConcurrentSweepType::DISABLE) {
        enableType_ = EnableConcurrentSweepType::REQUEST_DISABLE;
    } else {
        enableType_ = type;
    }
}
}  // namespace panda::ecmascript
