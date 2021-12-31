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

#include "ecmascript/mem/heap-inl.h"

#include <sys/sysinfo.h>

#include "ecmascript/cpu_profiler/cpu_profiler.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/assert_scope-inl.h"
#include "ecmascript/mem/compress_collector.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/mem_manager.h"
#include "ecmascript/mem/evacuation_allocator.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/mix_space_collector.h"
#include "ecmascript/mem/parallel_evacuation.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/semi_space_collector.h"
#include "ecmascript/mem/verification.h"

static constexpr int MAX_PARALLEL_THREAD_NUM = 3;

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
    paralledGc_ = ecmaVm_->GetJSOptions().IsEnableParalledYoungGc();
#if ECMASCRIPT_DISABLE_PARALLEL_GC
    paralledGc_ = false;
#endif
#if defined(IS_STANDARD_SYSTEM)
    paralledGc_ = false;
    concurrentMarkingEnable_ = false;
#endif
    workList_ = new WorkerHelper(this, Platform::GetCurrentPlatform()->GetTotalThreadNum() + 1);
    semiSpaceCollector_ = new SemiSpaceCollector(this, paralledGc_);
    compressCollector_ = new CompressCollector(this);

    derivedPointers_ = new ChunkVector<DerivedData>(ecmaVm_->GetChunk());
    derivedPointers_->resize(STACK_MAP_DEFALUT_DERIVED_SIZE);
    derivedPointers_->clear();
    mixSpaceCollector_ = new MixSpaceCollector(this);
    sweeper_ = new ConcurrentSweeper(this, ecmaVm_->GetJSOptions().IsEnableConcurrentSweep());
    concurrentMarker_ = new ConcurrentMarker(this);
    nonMovableMarker_ = new NonMovableMarker(this);
    semiGcMarker_ = new SemiGcMarker(this);
    compressGcMarker_ = new CompressGcMarker(this);
    evacuationAllocator_ = new EvacuationAllocator(this);
    evacuation_ = new ParallelEvacuation(this);
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

    delete workList_;
    workList_ = nullptr;
    delete semiSpaceCollector_;
    semiSpaceCollector_ = nullptr;
    delete mixSpaceCollector_;
    mixSpaceCollector_ = nullptr;
    delete compressCollector_;
    compressCollector_ = nullptr;
    regionFactory_ = nullptr;
    delete memController_;
    memController_ = nullptr;
    delete sweeper_;
    sweeper_ = nullptr;
    delete derivedPointers_;
    derivedPointers_ = nullptr;
    delete concurrentMarker_;
    concurrentMarker_ = nullptr;
    delete nonMovableMarker_;
    nonMovableMarker_ = nullptr;
    delete semiGcMarker_;
    semiGcMarker_ = nullptr;
    delete compressGcMarker_;
    compressGcMarker_ = nullptr;
}

void Heap::Prepare()
{
    WaitRunningTaskFinished();
    sweeper_->EnsureAllTaskFinished();
    evacuationAllocator_->WaitFreeTaskFinish();
}

void Heap::CollectGarbage(TriggerGCType gcType)
{
    JSThread *thread = GetEcmaVM()->GetAssociatedJSThread();
    [[maybe_unused]] GcStateScope scope(thread);
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
    // verify need semiGC or fullGC.
    if (gcType != TriggerGCType::SEMI_GC) {
        gcType = TriggerGCType::COMPRESS_FULL_GC;
    }
#endif

#if ECMASCRIPT_SWITCH_GC_MODE_TO_COMPRESS_GC
    gcType = TriggerGCType::COMPRESS_FULL_GC;
#endif
    switch (gcType) {
        case TriggerGCType::SEMI_GC:
            if (GetMemController()->IsInAppStartup()) {
                SetFromSpaceMaximumCapacity(SEMI_SPACE_SIZE_CAPACITY);
                SetNewSpaceMaximumCapacity(SEMI_SPACE_SIZE_CAPACITY);
                compressCollector_->RunPhases();
                ResetAppStartup();
            } else {
                if (!CheckAndTriggerCompressGC()) {
                    mixSpaceCollector_->RunPhases();
                    if (!sweeper_->IsConcurrentSweepEnabled()) {
                        RecomputeLimits();
                    }
                }
            }
            break;
        case TriggerGCType::OLD_GC:
            mixSpaceCollector_->RunPhases();
            if (!sweeper_->IsConcurrentSweepEnabled()) {
                RecomputeLimits();
            }
            break;
        case TriggerGCType::NON_MOVE_GC:
        case TriggerGCType::HUGE_GC:
        case TriggerGCType::MACHINE_CODE_GC:
            mixSpaceCollector_->RunPhases();
            if (!sweeper_->IsConcurrentSweepEnabled()) {
                RecomputeLimits();
            }
            break;
        case TriggerGCType::COMPRESS_FULL_GC:
            compressCollector_->RunPhases();
            RecomputeLimits();
            break;
        default:
            UNREACHABLE();
            break;
    }

#if ECMASCRIPT_ENABLE_GC_LOG
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

void Heap::RecomputeLimits()
{
    size_t oldSpaceSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetCommittedSize();
    size_t newSpaceCapacity = toSpace_->GetMaximumCapacity();

    constexpr double GrowingFactor = 1.1;
    auto newOldSpaceLimit = memController_->CalculateAllocLimit(oldSpaceSize, DEFAULT_OLD_SPACE_SIZE,
                                                                MAX_OLD_SPACE_SIZE, newSpaceCapacity, GrowingFactor);
    oldSpaceAllocLimit_ = newOldSpaceLimit;
}

bool Heap::CheckConcurrentMark(JSThread *thread)
{
    if (ConcurrentMarkingEnable() && !thread->IsNotBeginMark()) {
        if (thread->IsMarking()) {
            [[maybe_unused]] ClockScope clockScope;
            WaitConcurrentMarkingFinished();
            ECMA_GC_LOG() << "wait concurrent marking finish pause time " << clockScope.TotalSpentTime();
        }
        return true;
    }
    return false;
}

void Heap::TryTriggerConcurrentMarking(bool allowGc)
{
    if (allowGc &&
        toSpace_->GetCommittedSize() > SEMI_SPACE_TRIGGER_CONCURRENT_MARK &&
        GetEcmaVM()->GetJSThread()->IsNotBeginMark() &&
        !GetMemController()->IsInAppStartup()) {
        TriggerConcurrentMarking();
    }
}

void Heap::TriggerConcurrentMarking()
{
    if (concurrentMarkingEnable_) {
        concurrentMarker_->ConcurrentMarking();
    }
}

void Heap::CheckNeedFullMark()
{
    if ((oldSpace_->GetCommittedSize() + hugeObjectSpace_->GetCommittedSize()) > oldSpaceAllocLimit_) {
        SetOnlyMarkSemi(false);
    }
}

bool Heap::CheckAndTriggerOldGC()
{
    if ((oldSpace_->GetCommittedSize() + hugeObjectSpace_->GetCommittedSize()) <= oldSpaceAllocLimit_) {
        return false;
    }
    CollectGarbage(TriggerGCType::OLD_GC);
    return true;
}

bool Heap::CheckAndTriggerCompressGC()
{
    if ((oldSpace_->GetCommittedSize() + hugeObjectSpace_->GetCommittedSize()) <= oldSpaceAllocLimit_) {
        return false;
    }
    CollectGarbage(TriggerGCType::COMPRESS_FULL_GC);
    return true;
}

bool Heap::CheckAndTriggerNonMovableGC()
{
    if (nonMovableSpace_->GetCommittedSize() <= DEFAULT_NON_MOVABLE_SPACE_LIMIT) {
        return false;
    }
    CollectGarbage(TriggerGCType::NON_MOVE_GC);
    return true;
}

bool Heap::CheckAndTriggerMachineCodeGC()
{
    if (machineCodeSpace_->GetCommittedSize() <= DEFAULT_MACHINE_CODE_SPACE_LIMIT) {
        return false;
    }
    CollectGarbage(TriggerGCType::MACHINE_CODE_GC);
    return true;
}

void Heap::UpdateDerivedObjectInStack()
{
    if (derivedPointers_->empty()) {
        return;
    }
    for (auto derived : *derivedPointers_) {
        auto baseAddr = reinterpret_cast<JSTaggedValue *>(std::get<0>(derived));
        JSTaggedValue base = *baseAddr;
        if (base.IsHeapObject()) {
            uintptr_t baseOldObject = std::get<1>(derived);
            uintptr_t *derivedAddr = reinterpret_cast<uintptr_t *>(std::get<2>(derived));
            // derived is always bigger than base
            *derivedAddr = reinterpret_cast<uintptr_t>(base.GetHeapObject()) + (*derivedAddr - baseOldObject);
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << std::hex << "fix base:" << baseAddr << " base Value: " << base.GetHeapObject() <<
                " derived:" << derivedAddr << " New Value: " << *derivedAddr;
#endif
        }
    }
    derivedPointers_->resize(STACK_MAP_DEFALUT_DERIVED_SIZE);
    derivedPointers_->clear();
}

void Heap::WaitRunningTaskFinished()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    while (runningTastCount_ > 0) {
        waitTaskFinishedCV_.Wait(&waitTaskFinishedMutex_);
    }
}

void Heap::WaitConcurrentMarkingFinished()
{
    concurrentMarker_->WaitConcurrentMarkingFinished();
}

void Heap::SetConcurrentMarkingEnable(bool flag)
{
    concurrentMarkingEnable_ = flag;
}

bool Heap::ConcurrentMarkingEnable() const
{
    return concurrentMarkingEnable_;
}

void Heap::PostParallelGCTask(ParallelGCTaskPhase gcTask)
{
    IncreaseTaskCount();
    Platform::GetCurrentPlatform()->PostTask(std::make_unique<ParallelGCTask>(this, gcTask));
}

void Heap::IncreaseTaskCount()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    runningTastCount_++;
}

bool Heap::CheckCanDistributeTask()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    return (runningTastCount_ < Platform::GetCurrentPlatform()->GetTotalThreadNum() - 1) &&
        (runningTastCount_ <= MAX_PARALLEL_THREAD_NUM);
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
}  // namespace panda::ecmascript
