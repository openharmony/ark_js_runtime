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

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/runtime_call_id.h"
#include "os/mutex.h"

namespace panda::ecmascript {
ConcurrentMarker::ConcurrentMarker(Heap *heap, EnableConcurrentMarkType type)
    : heap_(heap),
      vm_(heap->GetEcmaVM()),
      thread_(vm_->GetJSThread()),
      workManager_(heap->GetWorkManager()),
      enableMarkType_(type)
{
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
}

void ConcurrentMarker::EnableConcurrentMarking(EnableConcurrentMarkType type)
{
    if (IsConfigDisabled()) {
        return;
    }
    if (IsEnabled() && thread_->IsMarking() && type == EnableConcurrentMarkType::DISABLE) {
        enableMarkType_ = EnableConcurrentMarkType::REQUEST_DISABLE;
    } else {
        enableMarkType_ = type;
    }
}

void ConcurrentMarker::Mark()
{
    LOG_GC(DEBUG) << "ConcurrentMarker: Concurrent Marking Begin";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ConcurrentMarker::Mark");
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarking);
    ClockScope scope;
    InitializeMarking();
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<MarkerTask>(heap_));
    if (!heap_->IsFullMark() && heap_->IsParallelGCEnabled()) {
        heap_->PostParallelGCTask(ParallelGCTaskPhase::CONCURRENT_HANDLE_OLD_TO_NEW_TASK);
    }
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticConcurrentMark(scope.GetPauseTime());
}

void ConcurrentMarker::Finish()
{
    workManager_->Finish();
}

void ConcurrentMarker::ReMark()
{
    LOG_GC(DEBUG) << "ConcurrentMarker: Remarking Begin";
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ReMarking);
    ClockScope scope;
    Marker *nonMovableMarker = heap_->GetNonMovableMarker();
    nonMovableMarker->MarkRoots(MAIN_THREAD_INDEX);
    if (!heap_->IsFullMark() && !heap_->IsParallelGCEnabled()) {
        nonMovableMarker->ProcessOldToNew(MAIN_THREAD_INDEX);
        nonMovableMarker->ProcessSnapshotRSet(MAIN_THREAD_INDEX);
    } else {
        nonMovableMarker->ProcessMarkStack(MAIN_THREAD_INDEX);
    }
    heap_->WaitRunningTaskFinished();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticConcurrentRemark(scope.GetPauseTime());
}

void ConcurrentMarker::HandleMarkingFinished()  // js-thread wait for sweep
{
    os::memory::LockHolder lock(waitMarkingFinishedMutex_);
    if (notifyMarkingFinished_) {
        heap_->CollectGarbage(TriggerGCType::YOUNG_GC);
    }
}

void ConcurrentMarker::WaitMarkingFinished()  // call in EcmaVm thread, wait for mark finished
{
    os::memory::LockHolder lock(waitMarkingFinishedMutex_);
    if (!notifyMarkingFinished_) {
        vmThreadWaitMarkingFinished_ = true;
        waitMarkingFinishedCV_.Wait(&waitMarkingFinishedMutex_);
    }
}

void ConcurrentMarker::Reset(bool revertCSet)
{
    Finish();
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
    notifyMarkingFinished_ = false;
    if (revertCSet) {
        // Partial gc clear cset when evacuation allocator finalize
        heap_->GetOldSpace()->RevertCSet();
        auto callback = [](Region *region) {
            region->ClearMarkGCBitset();
            region->ClearCrossRegionRSet();
            region->ResetAliveObject();
        };
        if (heap_->IsFullMark()) {
            heap_->EnumerateRegions(callback);
        } else {
            heap_->EnumerateNewSpaceRegions(callback);
        }
    }
}

void ConcurrentMarker::InitializeMarking()
{
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarkingInitialize);
    heap_->Prepare();
    thread_->SetMarkStatus(MarkStatus::MARKING);

    if (heap_->IsFullMark()) {
        heapObjectSize_ = heap_->GetHeapObjectSize();
        heap_->GetOldSpace()->SelectCSet();
        // The alive object size of Region in OldSpace will be recalculated.
        heap_->EnumerateNonNewSpaceRegions([](Region *current) {
            current->ResetAliveObject();
        });
    } else {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    }
    workManager_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK);
    heap_->GetNonMovableMarker()->MarkRoots(MAIN_THREAD_INDEX);
}

bool ConcurrentMarker::MarkerTask::Run(uint32_t threadId)
{
    ClockScope clockScope;
    heap_->GetNonMovableMarker()->ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
    heap_->GetConcurrentMarker()->FinishMarking(clockScope.TotalSpentTime());
    return true;
}

void ConcurrentMarker::FinishMarking(float spendTime)
{
    os::memory::LockHolder lock(waitMarkingFinishedMutex_);
    thread_->SetMarkStatus(MarkStatus::MARK_FINISHED);
    if (vmThreadWaitMarkingFinished_) {
        vmThreadWaitMarkingFinished_ = false;
        waitMarkingFinishedCV_.Signal();
    }
    notifyMarkingFinished_ = true;
    if (!heap_->IsFullMark()) {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    } else {
        heapObjectSize_ = heap_->GetHeapObjectSize();
    }
    SetDuration(spendTime);
}
}  // namespace panda::ecmascript
