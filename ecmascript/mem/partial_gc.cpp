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

#include "ecmascript/mem/partial_gc.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/object_xray-inl.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
PartialGC::PartialGC(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void PartialGC::RunPhases()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "PartialGC::RunPhases");
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), PartialGC_RunPhases);
    ClockScope clockScope;

    concurrentMark_ = heap_->CheckConcurrentMark();

    ECMA_GC_LOG() << "concurrentMark_" << concurrentMark_;
    Initialize();
    Mark();
    Sweep();
    Evacuate();
    Finish();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticPartialGC(concurrentMark_, clockScope.GetPauseTime(), freeSize_);
    ECMA_GC_LOG() << "PartialGC::RunPhases " << clockScope.TotalSpentTime();
}

void PartialGC::Initialize()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "PartialGC::Initialize");
    if (!concurrentMark_) {
        LOG(INFO, RUNTIME) << "Concurrent mark failure";
        heap_->Prepare();
        if (heap_->IsFullMark()) {
            heap_->GetOldSpace()->SelectCSet();
            heap_->EnumerateNonNewSpaceRegions([](Region *current) {
                current->ClearMarkGCBitset();
                current->ClearCrossRegionRSet();
                current->ResetAliveObject();
            });
        }
        workManager_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);

        freeSize_ = 0;
        hugeSpaceFreeSize_ = 0;
        oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
        nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
    }
}

void PartialGC::Finish()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "PartialGC::Finish");
    if (concurrentMark_) {
        auto marker = heap_->GetConcurrentMarker();
        marker->Reset(false);
    } else {
        size_t aliveSize = 0;
        workManager_->Finish(aliveSize);
    }
}

void PartialGC::Mark()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "PartialGC::Mark");
    if (concurrentMark_) {
        heap_->GetConcurrentMarker()->ReMark();
        return;
    }
    heap_->GetNonMovableMarker()->MarkRoots(MAIN_THREAD_INDEX);
    if (heap_->IsFullMark()) {
        heap_->GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    } else {
        heap_->GetNonMovableMarker()->ProcessOldToNew(MAIN_THREAD_INDEX);
        heap_->GetNonMovableMarker()->ProcessSnapshotRSet(MAIN_THREAD_INDEX);
    }
    heap_->WaitRunningTaskFinished();
}

void PartialGC::Sweep()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "PartialGC::Sweep");
    if (heap_->IsFullMark()) {
        ProcessNativeDelete();
        heap_->GetSweeper()->Sweep();
    }
}

void PartialGC::ProcessNativeDelete()
{
    WeakRootVisitor gcUpdateWeak = [this](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (!objectRegion->InYoungOrCSetGeneration() && !heap_->IsFullMark()) {
            return header;
        }
        if (!objectRegion->Test(header)) {
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        return header;
    };
    heap_->GetEcmaVM()->ProcessNativeDelete(gcUpdateWeak);
}

void PartialGC::Evacuate()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "PartialGC::Evacuate");
    heap_->GetEvacuator()->Evacuate();
}
}  // namespace panda::ecmascript
