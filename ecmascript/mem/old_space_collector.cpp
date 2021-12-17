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

#include "ecmascript/mem/old_space_collector.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/ecma_heap_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/heap_roots-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/vmstat/runtime_stat.h"

namespace panda::ecmascript {
OldSpaceCollector::OldSpaceCollector(Heap *heap)
    : heap_(heap), workList_(heap->GetWorkList()) {}

void OldSpaceCollector::RunPhases()
{
    JSThread *thread = heap_->GetEcmaVM()->GetJSThread();
    INTERPRETER_TRACE(thread, OldSpaceCollector_RunPhases);
    ClockScope clockScope;

    concurrentMark_ = heap_->CheckConcurrentMark(thread);
    if (concurrentMark_) {
        heap_->GetConcurrentMarker()->ReMarking();
        SweepPhases();
        auto marker = heap_->GetConcurrentMarker();
        marker->Reset();
    } else {
        InitializePhase();
        MarkingPhase();
        SweepPhases();
        FinishPhase();
    }
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticOldCollector(
        clockScope.GetPauseTime(), freeSize_, oldSpaceCommitSize_, nonMoveSpaceCommitSize_);
    ECMA_GC_LOG() << "OldSpaceCollector::RunPhases " << clockScope.TotalSpentTime();
}

void OldSpaceCollector::InitializePhase()
{
    heap_->WaitRunningTaskFinished();
    heap_->GetSweeper()->EnsureAllTaskFinished();
    heap_->EnumerateRegions([](Region *current) {
        // ensure mark bitmap
        auto bitmap = current->GetMarkBitmap();
        if (bitmap == nullptr) {
            current->GetOrCreateMarkBitmap();
        } else {
            bitmap->ClearAllBits();
        }
    });
    workList_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);

    freeSize_ = 0;
    hugeSpaceFreeSize_ = 0;
    oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
    nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
}

void OldSpaceCollector::FinishPhase()
{
    size_t aliveSize = 0;
    workList_->Finish(aliveSize);
}

void OldSpaceCollector::MarkingPhase()
{
    trace::ScopedTrace scoped_trace("OldSpaceCollector::MarkingPhase");
    heap_->GetNonMovableMarker()->MarkRoots(0);
    if (heap_->IsOnlyMarkSemi()) {
        heap_->GetNonMovableMarker()->ProcessOldToNew(0);
    } else {
        heap_->GetNonMovableMarker()->ProcessMarkStack(0);
    }
    heap_->WaitRunningTaskFinished();
}

void OldSpaceCollector::SweepPhases()
{
    trace::ScopedTrace scoped_trace("OldSpaceCollector::SweepPhases");
    // process weak reference
    auto totalThreadCount = Platform::GetCurrentPlatform()->GetTotalThreadNum() + 1;
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        ProcessQueue *queue = workList_->GetWeakReferenceQueue(i);
        while (true) {
            auto obj = queue->PopBack();
            if (UNLIKELY(obj == nullptr)) {
                break;
            }
            ObjectSlot slot(ToUintPtr(obj));
            JSTaggedValue value(slot.GetTaggedType());
            auto header = value.GetTaggedWeakRef();

            Region *objectRegion = Region::ObjectAddressToRange(header);
            auto markBitmap = objectRegion->GetMarkBitmap();
            if (!markBitmap->Test(header)) {
                slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
            }
        }
    }

    auto stringTable = heap_->GetEcmaVM()->GetEcmaStringTable();
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (objectRegion->InYoungGeneration()) {
            return header;
        }

        auto markBitmap = objectRegion->GetMarkBitmap();
        if (markBitmap->Test(header)) {
            return header;
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    stringTable->SweepWeakReference(gcUpdateWeak);
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);
    heap_->UpdateDerivedObjectInStack();

    heap_->GetSweeper()->SweepPhases();
}
}  // namespace panda::ecmascript
