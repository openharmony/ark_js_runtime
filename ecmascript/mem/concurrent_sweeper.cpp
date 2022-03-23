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
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
ConcurrentSweeper::ConcurrentSweeper(Heap *heap, bool concurrentSweep)
    : heap_(heap), concurrentSweep_(concurrentSweep)
{
}

void ConcurrentSweeper::PostConcurrentSweepTasks(bool fullGC)
{
    if (concurrentSweep_) {
        if (!fullGC) {
            Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(this, OLD_SPACE));
        }
        Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(this, NON_MOVABLE));
        Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(this, MACHINE_CODE_SPACE));
    }
}

void ConcurrentSweeper::SweepPhases(bool fullGC)
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ConcurrentSweepingInitialize);
    if (concurrentSweep_) {
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
            remainderTaskNum_[type] = FREE_LIST_NUM - startSpaceType_;
        }
    } else {
        if (!fullGC) {
            heap_->GetOldSpace()->Sweeping();
        }
        heap_->GetNonMovableSpace()->Sweeping();
        heap_->GetMachineCodeSpace()->Sweeping();
    }
    heap_->GetHugeObjectSpace()->Sweeping();
}

void ConcurrentSweeper::AsyncSweepSpace(MemSpaceType type, bool isMain)
{
    auto space = heap_->GetSpaceWithType(type);
    space->AsyncSweeping(isMain);

    os::memory::LockHolder holder(mutexs_[type]);
    if (--remainderTaskNum_[type] == 0) {
        cvs_[type].SignalAll();
    }
}

void ConcurrentSweeper::WaitAllTaskFinished()
{
    if (!isSweeping_) {
        return;
    }
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
        if (remainderTaskNum_[i] > 0) {
            os::memory::LockHolder holder(mutexs_[i]);
            while (remainderTaskNum_[i] > 0) {
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
    if (remainderTaskNum_[type] > 0) {
        {
            os::memory::LockHolder holder(mutexs_[type]);
            remainderTaskNum_[type]++;
        }
        AsyncSweepSpace(type, true);
        os::memory::LockHolder holder(mutexs_[type]);
        while (remainderTaskNum_[type] > 0) {
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

bool ConcurrentSweeper::SweeperTask::Run(uint32_t threadIndex)
{
    int sweepTypeNum = FREE_LIST_NUM - sweeper_->startSpaceType_;
    for (size_t i = sweeper_->startSpaceType_; i < FREE_LIST_NUM; i++) {
        auto type = static_cast<MemSpaceType>(((i + type_) % sweepTypeNum) + sweeper_->startSpaceType_);
        sweeper_->AsyncSweepSpace(type, false);
    }
    return true;
}
}  // namespace panda::ecmascript
