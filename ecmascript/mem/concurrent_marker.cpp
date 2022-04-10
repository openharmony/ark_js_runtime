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
#include "ecmascript/mem/object_xray-inl.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/runtime_call_id.h"
#include "os/mutex.h"

namespace panda::ecmascript {
ConcurrentMarker::ConcurrentMarker(Heap *heap)
    : heap_(heap),
      vm_(heap->GetEcmaVM()),
      thread_(vm_->GetJSThread()),
      workList_(heap->GetWorkList())
{
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
}

void ConcurrentMarker::ConcurrentMarking()
{
    ECMA_GC_LOG() << "ConcurrentMarker: Concurrent Mark Begin";
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "ConcurrentMarker::ConcurrentMarking");
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarking);
    ClockScope scope;
    InitializeMarking();
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<MarkerTask>(heap_));
    if (!heap_->IsFullMark() && heap_->IsParallelGCEnabled()) {
        heap_->PostParallelGCTask(ParallelGCTaskPhase::CONCURRENT_HANDLE_OLD_TO_NEW_TASK);
    }
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticConcurrentMark(scope.GetPauseTime());
}

void ConcurrentMarker::FinishPhase()
{
    size_t aliveSize = 0;
    workList_->Finish(aliveSize);
}

void ConcurrentMarker::ReMarking()
{
    ECMA_GC_LOG() << "ConcurrentMarker: Remarking Begin";
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ReMarking);
    ClockScope scope;
    Marker *nonMoveMarker =  heap_->GetNonMovableMarker();
    nonMoveMarker->MarkRoots(MAIN_THREAD_INDEX);
    if (!heap_->IsFullMark() && !heap_->IsParallelGCEnabled()) {
        heap_->GetNonMovableMarker()->ProcessOldToNew(MAIN_THREAD_INDEX);
        heap_->GetNonMovableMarker()->ProcessSnapshotRSet(MAIN_THREAD_INDEX);
    } else {
        nonMoveMarker->ProcessMarkStack(MAIN_THREAD_INDEX);
    }
    heap_->WaitRunningTaskFinished();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticConcurrentRemark(scope.GetPauseTime());
}

void ConcurrentMarker::HandleMarkFinished()  // js-thread wait for sweep
{
    os::memory::LockHolder lock(waitMarkingFinishedMutex_);
    if (notifyMarkingFinished_) {
        heap_->CollectGarbage(TriggerGCType::SEMI_GC);
    }
}

void ConcurrentMarker::WaitConcurrentMarkingFinished()  // call in EcmaVm thread, wait for mark finished
{
    os::memory::LockHolder lock(waitMarkingFinishedMutex_);
    if (!notifyMarkingFinished_) {
        vmThreadWaitMarkingFinished_ = true;
        waitMarkingFinishedCV_.Wait(&waitMarkingFinishedMutex_);
    }
}

void ConcurrentMarker::Reset(bool isRevertCSet)
{
    FinishPhase();
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
    notifyMarkingFinished_ = false;
    if (isRevertCSet) {
        // Mix space gc clear cset when evacuation allocator finalize
        heap_->GetOldSpace()->RevertCSet();
        auto callback = [](Region *region) {
            region->ClearMarkBitmap();
            region->ClearCrossRegionRememberedSet();
        };
        if (heap_->IsFullMark()) {
            heap_->EnumerateRegions(callback);
        } else {
            heap_->EnumerateNewSpaceRegions(callback);
        }
    }
}

// -------------------- privete method ------------------------------------------
void ConcurrentMarker::InitializeMarking()
{
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarkingInitialize);
    heap_->Prepare();
    thread_->SetMarkStatus(MarkStatus::MARKING);

    if (heap_->IsFullMark()) {
        heapObjectSize_ = heap_->GetHeapObjectSize();
        heap_->GetOldSpace()->SelectCSet();
        // The alive object size of Region in OldSpace will be recompute
        heap_->EnumerateNonNewSpaceRegions([](Region *current) {
            current->ResetAliveObject();
        });
    } else {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    }
    workList_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK);
    heap_->GetNonMovableMarker()->MarkRoots(MAIN_THREAD_INDEX);
}

bool ConcurrentMarker::MarkerTask::Run(uint32_t threadId)
{
    ClockScope clockScope;
    heap_->GetNonMovableMarker()->ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
    heap_->GetConcurrentMarker()->MarkingFinished(clockScope.TotalSpentTime());
    return true;
}

void ConcurrentMarker::MarkingFinished(float spendTime)
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
