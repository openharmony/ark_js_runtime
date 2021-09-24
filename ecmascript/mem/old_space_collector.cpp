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

#include "ecmascript/mem/old_space_collector-inl.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/ecma_heap_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/heap_roots-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/vmstat/runtime_stat.h"

namespace panda::ecmascript {
OldSpaceCollector::OldSpaceCollector(Heap *heap, bool parallelGc)
    : heap_(heap), rootManager_(heap->GetEcmaVM()), paralledGC_(parallelGc)
{
    workList_ = new OldGCWorker(heap_, heap_->GetThreadPool()->GetThreadNum());
}

void OldSpaceCollector::RunPhases()
{
    [[maybe_unused]] ecmascript::JSThread *thread = heap_->GetEcmaVM()->GetJSThread();
    INTERPRETER_TRACE(thread, OldSpaceCollector_RunPhases);
    trace::ScopedTrace scoped_trace("OldSpaceCollector::RunPhases");
    [[maybe_unused]] ClockScope clock("OldSpaceCollector::RunPhases");
    InitializePhase();
    MarkingPhase();
    SweepPhases();
    FinishPhase();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticOldCollector(clock.GetPauseTime(), freeSize_, oldSpaceCommitSize_,
                                                                nonMoveSpaceCommitSize_);
}

void OldSpaceCollector::InitializePhase()
{
    heap_->GetThreadPool()->WaitTaskFinish();
    heap_->GetSweeper()->EnsureAllTaskFinish();
    heap_->EnumerateRegions([](Region *current) {
        // ensure mark bitmap
        auto bitmap = current->GetMarkBitmap();
        if (bitmap == nullptr) {
            current->GetOrCreateMarkBitmap();
        } else {
            bitmap->ClearAllBits();
        }
    });
    workList_->Initialize();
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
    RootVisitor gcMarkYoung = [this]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            MarkObject(0, value.GetTaggedObject());
        }
    };
    RootRangeVisitor gcMarkRangeYoung = [this]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                MarkObject(0, value.GetTaggedObject());
            }
        }
    };
    rootManager_.VisitVMRoots(gcMarkYoung, gcMarkRangeYoung);

    ProcessMarkStack(0);
    if (paralledGC_) {
        heap_->GetThreadPool()->WaitTaskFinish();
    }
}

void OldSpaceCollector::ProcessMarkStack(uint64_t threadId)
{
    while (true) {
        TaggedObject *obj = nullptr;
        if (!workList_->Pop(threadId, &obj)) {
            break;
        }
        auto jsHclass = obj->GetClass();
        // mark dynClass
        MarkObject(threadId, jsHclass);

        rootManager_.MarkObjectBody<GCType::OLD_GC>(
            obj, jsHclass, [this, &threadId]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end) {
                for (ObjectSlot slot = start; slot < end; slot++) {
                    JSTaggedValue value(slot.GetTaggedType());
                    if (value.IsWeak()) {
                        RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                        continue;
                    }
                    if (value.IsHeapObject()) {
                        MarkObject(threadId, value.GetTaggedObject());
                    }
                }
            });
    }
}

void OldSpaceCollector::SweepPhases()
{
    trace::ScopedTrace scoped_trace("OldSpaceCollector::SweepPhases");
    // process weak reference
    for (uint32_t i = 0; i < heap_->GetThreadPool()->GetThreadNum(); i++) {
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

    heap_->GetSweeper()->SweepPhases();
}
}  // namespace panda::ecmascript
