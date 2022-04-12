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

#include "ecmascript/free_object.h"
#include "ecmascript/mem/heap-inl.h"

#ifndef PANDA_TARGET_WINDOWS
#include <sys/sysinfo.h>
#endif

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/mix_gc.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/parallel_evacuation.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/stw_young_gc_for_testing.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
Heap::Heap(EcmaVM *ecmaVm) : ecmaVm_(ecmaVm), thread_(ecmaVm->GetJSThread()),
                             nativeAreaAllocator_(ecmaVm->GetNativeAreaAllocator()),
                             heapRegionAllocator_(ecmaVm->GetHeapRegionAllocator()) {}

void Heap::Initialize()
{
    memController_ = new MemController(this);

    size_t defaultSemiSpaceCapacity = ecmaVm_->GetJSOptions().DefaultSemiSpaceCapacity();
    toSpace_ = new SemiSpace(this, defaultSemiSpaceCapacity, defaultSemiSpaceCapacity);
    toSpace_->Restart();
    toSpace_->SetWaterLine();
    fromSpace_ = new SemiSpace(this, defaultSemiSpaceCapacity, defaultSemiSpaceCapacity);

    // not set up from space
    size_t maxOldSpaceCapacity = ecmaVm_->GetJSOptions().MaxOldSpaceCapacity();
    oldSpace_ = new OldSpace(this, OLD_SPACE_LIMIT_BEGIN, maxOldSpaceCapacity);
    compressSpace_ = new OldSpace(this, OLD_SPACE_LIMIT_BEGIN, maxOldSpaceCapacity);
    oldSpace_->Initialize();
    size_t maxNonmovableSpaceCapacity = ecmaVm_->GetJSOptions().MaxNonmovableSpaceCapacity();
    nonMovableSpace_ = new NonMovableSpace(this, maxNonmovableSpaceCapacity, maxNonmovableSpaceCapacity);
    nonMovableSpace_->Initialize();
    size_t defaultSnapshotSpaceCapacity = ecmaVm_->GetJSOptions().DefaultSnapshotSpaceCapacity();
    size_t maxSnapshotSpaceCapacity = ecmaVm_->GetJSOptions().MaxSnapshotSpaceCapacity();
    snapshotSpace_ = new SnapShotSpace(this, defaultSnapshotSpaceCapacity, maxSnapshotSpaceCapacity);
    size_t maxMachineCodeSpaceCapacity = ecmaVm_->GetJSOptions().MaxMachineCodeSpaceCapacity();
    machineCodeSpace_ = new MachineCodeSpace(this, maxMachineCodeSpaceCapacity, maxMachineCodeSpaceCapacity);
    machineCodeSpace_->Initialize();
    hugeObjectSpace_ = new HugeObjectSpace(this);
    paralledGc_ = ecmaVm_->GetJSOptions().IsEnableParallelGC();
    concurrentMarkingEnabled_ = ecmaVm_->GetJSOptions().IsEnableConcurrentMark();
    markType_ = MarkType::SEMI_MARK;
#if ECMASCRIPT_DISABLE_PARALLEL_GC
    paralledGc_ = false;
#endif
#if defined(IS_STANDARD_SYSTEM)
    concurrentMarkingEnabled_ = false;
#endif
    workList_ = new WorkerHelper(this, Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1);
    stwYoungGC_ = new STWYoungGC(this, paralledGc_);
    fullGC_ = new FullGC(this);

    derivedPointers_ = new ChunkMap<DerivedDataKey, uintptr_t>(ecmaVm_->GetChunk());
    mixGC_ = new MixGC(this);
    sweeper_ = new ConcurrentSweeper(this, ecmaVm_->GetJSOptions().IsEnableConcurrentSweep());
    concurrentMarker_ = new ConcurrentMarker(this);
    nonMovableMarker_ = new NonMovableMarker(this);
    semiGcMarker_ = new SemiGcMarker(this);
    compressGcMarker_ = new CompressGcMarker(this);
    evacuation_ = new ParallelEvacuation(this);
}

void Heap::Destroy()
{
    Prepare();
    if (toSpace_ != nullptr) {
        toSpace_->Destroy();
        delete toSpace_;
        toSpace_ = nullptr;
    }
    if (fromSpace_ != nullptr) {
        fromSpace_->Destroy();
        delete fromSpace_;
        fromSpace_ = nullptr;
    }
    if (oldSpace_ != nullptr) {
        oldSpace_->Destroy();
        delete oldSpace_;
        oldSpace_ = nullptr;
    }
    if (compressSpace_ != nullptr) {
        compressSpace_->Destroy();
        delete compressSpace_;
        compressSpace_ = nullptr;
    }
    if (nonMovableSpace_ != nullptr) {
        nonMovableSpace_->Destroy();
        delete nonMovableSpace_;
        nonMovableSpace_ = nullptr;
    }
    if (snapshotSpace_ != nullptr) {
        snapshotSpace_->Destroy();
        delete snapshotSpace_;
        snapshotSpace_ = nullptr;
    }
    if (machineCodeSpace_ != nullptr) {
        machineCodeSpace_->Destroy();
        delete machineCodeSpace_;
        machineCodeSpace_ = nullptr;
    }
    if (hugeObjectSpace_ != nullptr) {
        hugeObjectSpace_->Destroy();
        delete hugeObjectSpace_;
        hugeObjectSpace_ = nullptr;
    }
    if (workList_ != nullptr) {
        delete workList_;
        workList_ = nullptr;
    }
    if (stwYoungGC_ != nullptr) {
        delete stwYoungGC_;
        stwYoungGC_ = nullptr;
    }
    if (mixGC_ != nullptr) {
        delete mixGC_;
        mixGC_ = nullptr;
    }
    if (fullGC_ != nullptr) {
        delete fullGC_;
        fullGC_ = nullptr;
    }

    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;

    if (memController_ != nullptr) {
        delete memController_;
        memController_ = nullptr;
    }
    if (sweeper_ != nullptr) {
        delete sweeper_;
        sweeper_ = nullptr;
    }
    if (derivedPointers_ != nullptr) {
        delete derivedPointers_;
        derivedPointers_ = nullptr;
    }
    if (concurrentMarker_ != nullptr) {
        delete concurrentMarker_;
        concurrentMarker_ = nullptr;
    }
    if (nonMovableMarker_ != nullptr) {
        delete nonMovableMarker_;
        nonMovableMarker_ = nullptr;
    }
    if (semiGcMarker_ != nullptr) {
        delete semiGcMarker_;
        semiGcMarker_ = nullptr;
    }
    if (compressGcMarker_ != nullptr) {
        delete compressGcMarker_;
        compressGcMarker_ = nullptr;
    }
}

void Heap::Prepare()
{
    MEM_ALLOCATE_AND_GC_TRACE(GetEcmaVM(), HeapPrepare);
    WaitRunningTaskFinished();
    sweeper_->EnsureAllTaskFinished();
    WaitClearTaskFinished();
}

void Heap::Resume(TriggerGCType gcType)
{
    if (gcType == TriggerGCType::FULL_GC) {
        compressSpace_->SetInitialCapacity(oldSpace_->GetInitialCapacity());
        auto *oldSpace = compressSpace_;
        compressSpace_ = oldSpace_;
        oldSpace_ = oldSpace;
    }
    if (toSpace_->AdjustCapacity(fromSpace_->GetAllocatedSizeSinceGC())) {
        fromSpace_->SetMaximumCapacity(toSpace_->GetMaximumCapacity());
    }

    toSpace_->SetWaterLine();
    if (paralledGc_) {
        isClearTaskFinished_ = false;
        Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<AsyncClearTask>(this, gcType));
    } else {
        ReclaimRegions(gcType);
    }
}

TriggerGCType Heap::SelectGCType() const
{
    // If concurrent mark is enable, The TryTriggerConcurrentMarking decide which GC to choose.
    if (concurrentMarkingEnabled_) {
        return SEMI_GC;
    }
    if (oldSpace_->CanExpand(toSpace_->GetSurvivalObjectSize()) && GetHeapObjectSize() <= globalSpaceAllocLimit_) {
        return SEMI_GC;
    } else {
        return OLD_GC;
    }
}

void Heap::CollectGarbage(TriggerGCType gcType)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    [[maybe_unused]] GcStateScope scope(thread_);
#endif
    CHECK_NO_GC
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    isVerifying_ = true;
    // pre gc heap verify
    sweeper_->EnsureAllTaskFinished();
    auto failCount = Verification(this).VerifyAll();
    if (failCount > 0) {
        LOG(FATAL, GC) << "Before gc heap corrupted and " << failCount << " corruptions";
    }
    isVerifying_ = false;
#endif

#if ECMASCRIPT_SWITCH_GC_MODE_TO_FULL_GC
    gcType = TriggerGCType::FULL_GC;
#endif
    if (isFullGCRequested_ && thread_->IsReadyToMark() && gcType != TriggerGCType::FULL_GC) {
        gcType = TriggerGCType::FULL_GC;
    }
    startNewSpaceSize_ = toSpace_->GetHeapObjectSize();
    memController_->StartCalculationBeforeGC();
    OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "Heap::CollectGarbage, gcType = " << gcType
                                             << " global CommittedSize" << GetCommittedSize()
                                             << " global limit" << globalSpaceAllocLimit_;
    switch (gcType) {
        case TriggerGCType::SEMI_GC:
            if (!concurrentMarkingEnabled_) {
                SetMarkType(MarkType::SEMI_MARK);
            }
            mixGC_->RunPhases();
            break;
        case TriggerGCType::OLD_GC:
            if (concurrentMarkingEnabled_ && markType_ == MarkType::SEMI_MARK) {
                bool concurrentMark = CheckConcurrentMark();
                if (concurrentMark) {
                    GetConcurrentMarker()->Reset();
                }
            }
            SetMarkType(MarkType::FULL_MARK);
            mixGC_->RunPhases();
            break;
        case TriggerGCType::FULL_GC:
            fullGC_->RunPhases();
            if (isFullGCRequested_) {
                isFullGCRequested_ = false;
            }
            break;
        default:
            UNREACHABLE();
            break;
    }

    if (!oldSpaceLimitAdjusted_ && startNewSpaceSize_ > 0) {
        semiSpaceCopiedSize_ = toSpace_->GetHeapObjectSize();
        double copiedRate = semiSpaceCopiedSize_ * 1.0 / startNewSpaceSize_;
        promotedSize_ = GetEvacuation()->GetPromotedSize();
        double promotedRate = promotedSize_ * 1.0 / startNewSpaceSize_;
        memController_->AddSurvivalRate(std::min(copiedRate + promotedRate, 1.0));
        AdjustOldSpaceLimit();
    }

    memController_->StopCalculationAfterGC(gcType);

    if (gcType == TriggerGCType::FULL_GC || IsFullMark()) {
        // Only when the gc type is not semiGC and after the old space sweeping has been finished,
        // the limits of old space and global space can be recomputed.
        RecomputeLimits();
        OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << " GC after: is full mark" << IsFullMark()
                                                 << " global CommittedSize" << GetCommittedSize()
                                                 << " global limit" << globalSpaceAllocLimit_;
        markType_ = MarkType::SEMI_MARK;
    }

# if ECMASCRIPT_ENABLE_GC_LOG
    ecmaVm_->GetEcmaGCStats()->PrintStatisticResult();
#endif

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    // post gc heap verify
    isVerifying_ = true;
    sweeper_->EnsureAllTaskFinished();
    failCount = Verification(this).VerifyAll();
    if (failCount > 0) {
        LOG(FATAL, GC) << "After gc heap corrupted and " << failCount << " corruptions";
    }
    isVerifying_ = false;
#endif
}

void Heap::ThrowOutOfMemoryError(size_t size, std::string functionName)
{
    GetEcmaVM()->GetEcmaGCStats()->PrintHeapStatisticResult(true);
    LOG_ECMA_MEM(FATAL) << "OOM when trying to allocate " << size << " bytes"
        << " function name: " << functionName.c_str();
}

size_t Heap::VerifyHeapObjects() const
{
    size_t failCount = 0;
    {
        VerifyObjectVisitor verifier(this, &failCount);
        toSpace_->IterateOverObjects(verifier);
    }

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

void Heap::AdjustOldSpaceLimit()
{
    if (oldSpaceLimitAdjusted_) {
        return;
    }
    size_t oldSpaceAllocLimit = GetOldSpace()->GetInitialCapacity();
    size_t newOldSpaceAllocLimit = std::max(oldSpace_->GetHeapObjectSize() + MIN_GROWING_STEP,
        static_cast<size_t>(oldSpaceAllocLimit * memController_->GetAverageSurvivalRate()));
    if (newOldSpaceAllocLimit <= oldSpaceAllocLimit) {
        GetOldSpace()->SetInitialCapacity(newOldSpaceAllocLimit);
    } else {
        oldSpaceLimitAdjusted_ = true;
    }

    size_t newGlobalSpaceAllocLimit = std::max(GetHeapObjectSize() + MIN_GROWING_STEP,
        static_cast<size_t>(globalSpaceAllocLimit_ * memController_->GetAverageSurvivalRate()));
    if (newGlobalSpaceAllocLimit < globalSpaceAllocLimit_) {
        globalSpaceAllocLimit_ = newGlobalSpaceAllocLimit;
    }
    OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "AdjustOldSpaceLimit oldSpaceAllocLimit_" << oldSpaceAllocLimit
        << " globalSpaceAllocLimit_" << globalSpaceAllocLimit_;
}

void Heap::RecomputeLimits()
{
    double gcSpeed = memController_->CalculateMarkCompactSpeedPerMS();
    double mutatorSpeed = memController_->GetCurrentOldSpaceAllocationThroughtputPerMS();
    size_t oldSpaceSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
    size_t newSpaceCapacity = toSpace_->GetMaximumCapacity();

    double growingFactor = memController_->CalculateGrowingFactor(gcSpeed, mutatorSpeed);
    size_t maxOldSpaceCapacity = GetEcmaVM()->GetJSOptions().MaxOldSpaceCapacity();
    auto newOldSpaceLimit = memController_->CalculateAllocLimit(oldSpaceSize, MIN_OLD_SPACE_LIMIT, maxOldSpaceCapacity,
                                                                newSpaceCapacity, growingFactor);
    auto newGlobalSpaceLimit = memController_->CalculateAllocLimit(GetHeapObjectSize(), DEFAULT_HEAP_SIZE,
                                                                   MAX_HEAP_SIZE, newSpaceCapacity, growingFactor);
    globalSpaceAllocLimit_ = newGlobalSpaceLimit;
    oldSpace_->SetInitialCapacity(newOldSpaceLimit);
    OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "RecomputeLimits oldSpaceAllocLimit_" << newOldSpaceLimit
        << " globalSpaceAllocLimit_" << globalSpaceAllocLimit_;
}

bool Heap::CheckConcurrentMark()
{
    if (concurrentMarkingEnabled_ && !thread_->IsReadyToMark()) {
        if (thread_->IsMarking()) {
            [[maybe_unused]] ClockScope clockScope;
            ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "Heap::CheckConcurrentMark");
            MEM_ALLOCATE_AND_GC_TRACE(GetEcmaVM(), WaitConcurrentMarkingFinished);
            GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
            WaitConcurrentMarkingFinished();
            ecmaVm_->GetEcmaGCStats()->StatisticConcurrentMarkWait(clockScope.GetPauseTime());
            ECMA_GC_LOG() << "wait concurrent marking finish pause time " << clockScope.TotalSpentTime();
        }
        memController_->RecordAfterConcurrentMark(IsFullMark(), concurrentMarker_);
        return true;
    }
    return false;
}

void Heap::TryTriggerConcurrentMarking()
{
    // When the concurrent mark is enabled, concurrent mark can be tried to triggered. When the size of old space or
    // global space reaches to the limits, isFullMarkNeeded will be true. If the predicted duration of the current full
    // mark can allow the new space and old space to allocate to their limits, full mark will be triggered. In the same
    // way, if the size of the new space reaches to the capacity, and the predicted duration of the current semi mark
    // can exactly allow the new space to allocate to the capacity, semi mark can be triggered. But when it will spend
    // a lot of time in full mark, the compress full GC will be requested after the spaces reach to limits. And If the
    // global space is larger than the half max heap size, we will turn to use full mark and trigger mix GC.
    if (!concurrentMarkingEnabled_ || !thread_->IsReadyToMark()) {
        return;
    }
    bool isFullMarkNeeded = false;
    double oldSpaceMarkDuration = 0, newSpaceMarkDuration = 0, newSpaceRemainSize = 0, newSpaceAllocToLimitDuration = 0,
           oldSpaceAllocToLimitDuration = 0;
    double oldSpaceAllocSpeed = memController_->GetOldSpaceAllocationThroughtPerMS();
    double oldSpaceConcurrentMarkSpeed = memController_->GetFullSpaceConcurrentMarkSpeedPerMS();
    size_t oldSpaceHeapObjectSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
    size_t globalHeapObjectSize = GetHeapObjectSize();
    size_t oldSpaceAllocLimit = oldSpace_->GetInitialCapacity();
    if (oldSpaceConcurrentMarkSpeed == 0 || oldSpaceAllocSpeed == 0) {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit ||  globalHeapObjectSize >= globalSpaceAllocLimit_) {
            markType_ = MarkType::FULL_MARK;
            OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "Trigger the first full mark";
            TriggerConcurrentMarking();
        }
    } else {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_) {
            isFullMarkNeeded = true;
        }
        oldSpaceAllocToLimitDuration = (oldSpaceAllocLimit - oldSpaceHeapObjectSize) / oldSpaceAllocSpeed;
        oldSpaceMarkDuration = GetHeapObjectSize() / oldSpaceConcurrentMarkSpeed;
        // oldSpaceRemainSize means the predicted size which can be allocated after the full concurrent mark.
        double oldSpaceRemainSize = (oldSpaceAllocToLimitDuration - oldSpaceMarkDuration) * oldSpaceAllocSpeed;
        if (oldSpaceRemainSize > 0 && oldSpaceRemainSize < DEFAULT_REGION_SIZE) {
            isFullMarkNeeded = true;
        }
    }

    double newSpaceAllocSpeed = memController_->GetNewSpaceAllocationThroughtPerMS();
    double newSpaceConcurrentMarkSpeed = memController_->GetNewSpaceConcurrentMarkSpeedPerMS();

    if (newSpaceConcurrentMarkSpeed == 0 || newSpaceAllocSpeed == 0) {
        if (toSpace_->GetCommittedSize() >= SEMI_SPACE_TRIGGER_CONCURRENT_MARK) {
            markType_ = MarkType::SEMI_MARK;
            TriggerConcurrentMarking();
            OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "Trigger the first semi mark" << isFullGCRequested_;
        }
        return;
    }
    newSpaceAllocToLimitDuration = (toSpace_->GetMaximumCapacity() - toSpace_->GetCommittedSize())
                                    / newSpaceAllocSpeed;
    newSpaceMarkDuration = toSpace_->GetHeapObjectSize() / newSpaceConcurrentMarkSpeed;
    // newSpaceRemainSize means the predicted size which can be allocated after the semi concurrent mark.
    newSpaceRemainSize = (newSpaceAllocToLimitDuration - newSpaceMarkDuration) * newSpaceAllocSpeed;

    if (isFullMarkNeeded) {
        if (oldSpaceMarkDuration < newSpaceAllocToLimitDuration
            && oldSpaceMarkDuration < oldSpaceAllocToLimitDuration) {
            markType_ = MarkType::FULL_MARK;
            TriggerConcurrentMarking();
            OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "Trigger full mark by speed";
        } else {
            if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_) {
                markType_ = MarkType::FULL_MARK;
                TriggerConcurrentMarking();
                OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "Trigger full mark by limit";
            }
        }
    } else if (newSpaceRemainSize < DEFAULT_REGION_SIZE) {
        markType_ = MarkType::SEMI_MARK;
        TriggerConcurrentMarking();
        OPTIONAL_LOG(ecmaVm_, ERROR, ECMASCRIPT) << "Trigger semi mark";
    }
}

void Heap::TriggerConcurrentMarking()
{
    if (concurrentMarkingEnabled_ && !isFullGCRequested_) {
        concurrentMarker_->ConcurrentMarking();
    }
}

void Heap::UpdateDerivedObjectInStack()
{
    if (derivedPointers_->empty()) {
        return;
    }
    for (auto derived : *derivedPointers_) {
        auto baseAddr = reinterpret_cast<JSTaggedValue *>(derived.first.first);
        JSTaggedValue base = *baseAddr;
        if (base.IsHeapObject()) {
            uintptr_t baseOldObject = derived.second;
            uintptr_t *derivedAddr = reinterpret_cast<uintptr_t *>(derived.first.second);
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << std::hex << "fix base before:" << baseAddr << " base old Value: " << baseOldObject <<
                " derived:" << derivedAddr << " old Value: " << *derivedAddr << std::endl;
#endif
            // derived is always bigger than base
            *derivedAddr = reinterpret_cast<uintptr_t>(base.GetHeapObject()) + (*derivedAddr - baseOldObject);
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << std::hex << "fix base after:" << baseAddr <<
                " base New Value: " << base.GetHeapObject() <<
                " derived:" << derivedAddr << " New Value: " << *derivedAddr << std::endl;
#endif
        }
    }
    derivedPointers_->clear();
}

void Heap::WaitRunningTaskFinished()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    while (runningTastCount_ > 0) {
        waitTaskFinishedCV_.Wait(&waitTaskFinishedMutex_);
    }
}

void Heap::WaitClearTaskFinished()
{
    os::memory::LockHolder holder(waitClearTaskFinishedMutex_);
    while (!isClearTaskFinished_) {
        waitClearTaskFinishedCV_.Wait(&waitClearTaskFinishedMutex_);
    }
}

void Heap::WaitConcurrentMarkingFinished()
{
    concurrentMarker_->WaitConcurrentMarkingFinished();
}

void Heap::PostParallelGCTask(ParallelGCTaskPhase gcTask)
{
    IncreaseTaskCount();
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<ParallelGCTask>(this, gcTask));
}

void Heap::IncreaseTaskCount()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    runningTastCount_++;
}

bool Heap::CheckCanDistributeTask()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    return (runningTastCount_ < Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() - 1);
}

void Heap::ReduceTaskCount()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    runningTastCount_--;
    if (runningTastCount_ == 0) {
        waitTaskFinishedCV_.SignalAll();
    }
}

bool Heap::ParallelGCTask::Run(uint32_t threadIndex)
{
    switch (taskPhase_) {
        case ParallelGCTaskPhase::SEMI_HANDLE_THREAD_ROOTS_TASK:
            heap_->GetSemiGcMarker()->MarkRoots(threadIndex);
            heap_->GetSemiGcMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::SEMI_HANDLE_SNAPSHOT_TASK:
            heap_->GetSemiGcMarker()->ProcessSnapshotRSet(threadIndex);
            break;
        case ParallelGCTaskPhase::SEMI_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetSemiGcMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::COMPRESS_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetCompressGcMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::CONCURRENT_HANDLE_OLD_TO_NEW_TASK:
            heap_->GetNonMovableMarker()->ProcessOldToNew(threadIndex);
            break;
        default:
            break;
    }
    heap_->ReduceTaskCount();
    return true;
}

bool Heap::AsyncClearTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    heap_->ReclaimRegions(gcType_, lastRegionOfToSpace_);
    return true;
}

size_t Heap::GetArrayBufferSize() const
{
    size_t result = 0;
    this->IteratorOverObjects([&result](TaggedObject *obj) {
        JSHClass* jsClass = obj->GetClass();
        result += jsClass->IsArrayBuffer() ? jsClass->GetObjectSize() : 0;
    });
    return result;
}

bool Heap::IsLive(TaggedObject *object) const
{
    if (!ContainObject(object)) {
        LOG(ERROR, RUNTIME) << "The region is already free";
        return false;
    }

    Region *region = Region::ObjectAddressToRange(object);
    if (region->InHugeObjectGeneration()) {
        return true;
    }
    bool isFree = FreeObject::Cast(ToUintPtr(object))->IsFreeObject();
    if (isFree) {
        LOG(ERROR, RUNTIME) << "The object " << object << " in "
                            << ToSpaceTypeName(region->GetSpace()->GetSpaceType())
                            << " already free";
    }
    return !isFree;
}

bool Heap::ContainObject(TaggedObject *object) const
{
    // semi space
    if (toSpace_->ContainObject(object)) {
        return true;
    }
    // old space
    if (oldSpace_->ContainObject(object)) {
        return true;
    }
    // non movable space
    if (nonMovableSpace_->ContainObject(object)) {
        return true;
    }
    // huge object space
    if (hugeObjectSpace_->ContainObject(object)) {
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
