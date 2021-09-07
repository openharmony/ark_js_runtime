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

#include "ecmascript/mem/heap.h"

#include <sys/sysinfo.h>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/assert_scope-inl.h"
#include "ecmascript/mem/compress_collector.h"
#include "ecmascript/mem/ecma_heap_manager.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/old_space_collector.h"
#include "ecmascript/mem/semi_space_collector.h"
#include "ecmascript/mem/semi_space_worker.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {
Heap::Heap(EcmaVM *ecmaVm) : ecmaVm_(ecmaVm), regionFactory_(ecmaVm->GetRegionFactory()) {}

void Heap::Initialize()
{
    memController_ = CreateMemController("no-gc-for-start-up");

    if (memController_->IsInAppStartup()) {
        toSpace_ = new SemiSpace(this, DEFAULT_SEMI_SPACE_SIZE, MAX_SEMI_SPACE_SIZE_STARTUP);
        fromSpace_ = new SemiSpace(this, DEFAULT_SEMI_SPACE_SIZE, MAX_SEMI_SPACE_SIZE_STARTUP);
    } else {
        toSpace_ = new SemiSpace(this);
        fromSpace_ = new SemiSpace(this);
    }

    toSpace_->Initialize();
    // not set up from space
    oldSpace_ = new OldSpace(this);
    compressSpace_ = new OldSpace(this);
    oldSpace_->Initialize();
    nonMovableSpace_ = new NonMovableSpace(this);
    nonMovableSpace_->Initialize();
    snapshotSpace_ = new SnapShotSpace(this);
    machineCodeSpace_ = new MachineCodeSpace(this);
    machineCodeSpace_->Initialize();
    hugeObjectSpace_ = new HugeObjectSpace(this);
    markStack_ = new MarkStack(this);
    weakProcessQueue_ = new ProcessQueue(this);
    bool paralledGc = ecmaVm_->GetOptions().IsEnableParalledYoungGc();
    if (paralledGc) {
        int numOfCpuCore = get_nprocs();
        int numThread = std::min<int>(numOfCpuCore, THREAD_NUM_FOR_YOUNG_GC);
        pool_ = new ThreadPool(numThread);
        semiSpaceCollector_ = new SemiSpaceCollector(this, true);
        compressCollector_ = new CompressCollector(this, true);
    } else {
        pool_ = new ThreadPool(1);
        semiSpaceCollector_ = new SemiSpaceCollector(this, false);
        compressCollector_ = new CompressCollector(this, false);
    }
    oldSpaceCollector_ = new OldSpaceCollector(this);
}

void Heap::FlipNewSpace()
{
    SemiSpace *newSpace = fromSpace_;
    fromSpace_ = toSpace_;
    toSpace_ = newSpace;
}

void Heap::FlipCompressSpace()
{
    OldSpace *oldSpace = compressSpace_;
    compressSpace_ = oldSpace_;
    oldSpace_ = oldSpace;
}
void Heap::Destroy()
{
    pool_->WaitTaskFinish();
    toSpace_->Destroy();
    delete toSpace_;
    toSpace_ = nullptr;
    fromSpace_->Destroy();
    delete fromSpace_;
    fromSpace_ = nullptr;

    oldSpace_->Destroy();
    delete oldSpace_;
    oldSpace_ = nullptr;
    compressSpace_->Destroy();
    delete compressSpace_;
    compressSpace_ = nullptr;
    nonMovableSpace_->Destroy();
    delete nonMovableSpace_;
    nonMovableSpace_ = nullptr;
    snapshotSpace_->Destroy();
    delete snapshotSpace_;
    snapshotSpace_ = nullptr;
    machineCodeSpace_->Destroy();
    delete machineCodeSpace_;
    machineCodeSpace_ = nullptr;
    markStack_->Destroy();
    delete markStack_;
    markStack_ = nullptr;
    hugeObjectSpace_->Destroy();
    delete hugeObjectSpace_;
    hugeObjectSpace_ = nullptr;

    weakProcessQueue_->Destroy();
    delete weakProcessQueue_;
    weakProcessQueue_ = nullptr;
    delete semiSpaceCollector_;
    semiSpaceCollector_ = nullptr;
    delete oldSpaceCollector_;
    oldSpaceCollector_ = nullptr;
    delete compressCollector_;
    compressCollector_ = nullptr;
    regionFactory_ = nullptr;
    delete memController_;
    memController_ = nullptr;
    delete pool_;
    pool_ = nullptr;
}

void Heap::CollectGarbage(TriggerGCType gcType)
{
    CHECK_NO_GC
    // pre gc heap verify
    {
        if (ecmaVm_->GetOptions().IsPreGcHeapVerifyEnabled()) {
            auto failCount = Verification(this).VerifyAll();
            if (failCount > 0) {
                LOG(FATAL, GC) << "Before gc heap corrupted and " << failCount << " corruptions";
            }
        }
    }
    switch (gcType) {
        case TriggerGCType::SEMI_GC:
            if (GetMemController()->IsInAppStartup()) {
                semiSpaceCollector_->RunPhases();
                SetFromSpaceMaximumCapacity(SEMI_SPACE_SIZE_CAPACITY);
                SetNewSpaceMaximumCapacity(SEMI_SPACE_SIZE_CAPACITY);
                ResetAppStartup();
            } else {
                semiSpaceCollector_->RunPhases();
            }
            break;
        case TriggerGCType::OLD_GC:
            if (oldSpace_->GetHeapObjectSize() < OLD_SPACE_LIMIT_BEGIN) {
                oldSpaceCollector_->RunPhases();
            } else {
                compressCollector_->RunPhases();
            }
            RecomputeLimits();
            break;
        case TriggerGCType::NON_MOVE_GC:
        case TriggerGCType::HUGE_GC:
        case TriggerGCType::MACHINE_CODE_GC:
            oldSpaceCollector_->RunPhases();
            RecomputeLimits();
            break;
        case TriggerGCType::COMPRESS_FULL_GC:
            compressCollector_->RunPhases();
            RecomputeLimits();
            break;
        default:
            UNREACHABLE();
            break;
    }

    // post gc heap verify
    {
        if (ecmaVm_->GetOptions().IsPreGcHeapVerifyEnabled()) {
            auto failCount = Verification(this).VerifyAll();
            if (failCount > 0) {
                LOG(FATAL, GC) << "After gc heap corrupted and " << failCount << " corruptions";
            }
        }
    }
}

void Heap::ThrowOutOfMemoryError(size_t size)
{
    LOG_ECMA_MEM(FATAL) << "OOM when trying to allocate " << size << " bytes";
}

size_t Heap::VerifyHeapObjects() const
{
    size_t failCount = 0;
    {
        VerifyObjectVisitor verifier(this, &failCount);
        oldSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount);
        nonMovableSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount);
        hugeObjectSpace_->IterateOverObjects(verifier);
    }

    return failCount;
}

void Heap::RecomputeLimits()
{
    size_t oldSpaceSize = oldSpace_->GetHeapObjectSize();
    size_t newSpaceCapacity = toSpace_->GetMaximumCapacity();

    constexpr double GrowingFactor = 1.1;
    auto newOldSpaceLimit = memController_->CalculateAllocLimit(oldSpaceSize, DEFAULT_OLD_SPACE_SIZE,
                                                                MAX_OLD_SPACE_SIZE, newSpaceCapacity, GrowingFactor);
    oldSpaceAllocLimit_ = newOldSpaceLimit;
}

bool Heap::CheckAndTriggerOldGC()
{
    if (oldSpace_->GetHeapObjectSize() <= oldSpaceAllocLimit_) {
        return false;
    }
    CollectGarbage(TriggerGCType::OLD_GC);
    return true;
}

bool Heap::CheckAndTriggerCompressGC()
{
    if (oldSpace_->GetHeapObjectSize() <= oldSpaceAllocLimit_) {
        return false;
    }
    CollectGarbage(TriggerGCType::COMPRESS_FULL_GC);
    return true;
}

bool Heap::CheckAndTriggerNonMovableGC()
{
    if (nonMovableSpace_->GetHeapObjectSize() <= DEFAULT_NON_MOVABLE_SPACE_LIMIT) {
        return false;
    }
    CollectGarbage(TriggerGCType::NON_MOVE_GC);
    return true;
}
}  // namespace panda::ecmascript
