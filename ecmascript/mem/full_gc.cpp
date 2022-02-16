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

#include "ecmascript/mem/full_gc.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/mem_manager.h"
#include "ecmascript/mem/object_xray-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
FullGC::FullGC(Heap *heap) : heap_(heap), workList_(heap->GetWorkList()) {}

void FullGC::RunPhases()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::RunPhases");
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), FullGC_RunPhases);
    ClockScope clockScope;

    bool concurrentMark = heap_->CheckConcurrentMark();
    if (concurrentMark) {
        ECMA_GC_LOG() << "FullGC after ConcurrentMarking";
        heap_->GetConcurrentMarker()->Reset();  // HPPGC use mark result to move TaggedObject.
    }
    InitializePhase();
    MarkingPhase();
    SweepPhases();
    FinishPhase();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticFullGC(clockScope.GetPauseTime(), youngAndOldAliveSize_,
                                                          youngSpaceCommitSize_, oldSpaceCommitSize_,
                                                          nonMoveSpaceFreeSize_, nonMoveSpaceCommitSize_);
    ECMA_GC_LOG() << "FullGC::RunPhases " << clockScope.TotalSpentTime();
}

void FullGC::InitializePhase()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::InitializePhase");
    heap_->Prepare();
    auto callback = [](Region *current) {
        // ensure mark bitmap
        auto rememberset = current->GetOldToNewRememberedSet();
        if (rememberset != nullptr) {
            rememberset->ClearAllBits();
        }
    };
    heap_->EnumerateNonMovableRegions(callback);
    heap_->ResetNewSpace();
    workList_->Initialize(TriggerGCType::FULL_GC, ParallelGCTaskPhase::COMPRESS_HANDLE_GLOBAL_POOL_TASK);
    heap_->GetCompressGcMarker()->Initialized();

    youngAndOldAliveSize_ = 0;
    nonMoveSpaceFreeSize_ = 0;
    youngSpaceCommitSize_ = heap_->GetFromSpace()->GetCommittedSize();
    oldSpaceCommitSize_ = heap_->GetCompressSpace()->GetCommittedSize();
    nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
}

void FullGC::MarkingPhase()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::MarkingPhase");
    heap_->GetCompressGcMarker()->MarkRoots(0);
    heap_->GetCompressGcMarker()->ProcessMarkStack(0);
    heap_->WaitRunningTaskFinished();
}

void FullGC::SweepPhases()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::SweepPhases");
    // process weak reference
    auto totalThreadCount = Platform::GetCurrentPlatform()->GetTotalThreadNum() + 1; // gc thread and main thread
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
            if (!objectRegion->InYoungAndOldGeneration()) {
                auto markBitmap = objectRegion->GetMarkBitmap();
                if (!markBitmap->Test(header)) {
                    slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
                }
            } else {
                MarkWord markWord(header);
                if (markWord.IsForwardingAddress()) {
                    TaggedObject *dst = markWord.ToForwardingAddress();
                    auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
                    slot.Update(weakRef);
                } else {
                    slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
                }
            }
        }
    }

    auto stringTable = heap_->GetEcmaVM()->GetEcmaStringTable();
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (!objectRegion->InYoungAndOldGeneration()) {
            auto markBitmap = objectRegion->GetMarkBitmap();
            if (markBitmap->Test(header)) {
                return header;
            }
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }

        MarkWord markWord(header);
        if (markWord.IsForwardingAddress()) {
            return markWord.ToForwardingAddress();
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    stringTable->SweepWeakReference(gcUpdateWeak);
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);

    heap_->UpdateDerivedObjectInStack();
    heap_->GetSweeper()->SweepPhases(true);
}

void FullGC::FinishPhase()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::FinishPhase");
    heap_->Resume(FULL_GC);
    workList_->Finish(youngAndOldAliveSize_);
}
}  // namespace panda::ecmascript