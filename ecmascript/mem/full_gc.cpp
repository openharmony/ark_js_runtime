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
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
FullGC::FullGC(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

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
    Initialize();
    Mark();
    Sweep();
    Finish();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticFullGC(clockScope.GetPauseTime(), youngAndOldAliveSize_,
                                                          youngSpaceCommitSize_, oldSpaceCommitSize_,
                                                          nonMoveSpaceFreeSize_, nonMoveSpaceCommitSize_);
    ECMA_GC_LOG() << "FullGC::RunPhases " << clockScope.TotalSpentTime();
}

void FullGC::Initialize()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::Initialize");
    heap_->Prepare();
    auto callback = [](Region *current) {
        current->ClearOldToNewRSet();
    };
    heap_->EnumerateNonMovableRegions(callback);
    heap_->EnumerateNonNewSpaceRegions([](Region *current) {
        current->ClearMarkGCBitset();
        current->ClearCrossRegionRSet();
    });
    youngSpaceCommitSize_ = heap_->GetNewSpace()->GetCommittedSize();
    heap_->SwapNewSpace();
    workManager_->Initialize(TriggerGCType::FULL_GC, ParallelGCTaskPhase::COMPRESS_HANDLE_GLOBAL_POOL_TASK);
    heap_->GetCompressGCMarker()->Initialize();

    youngAndOldAliveSize_ = 0;
    nonMoveSpaceFreeSize_ = 0;
    oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
    nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
}

void FullGC::Mark()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::Mark");
    heap_->GetCompressGCMarker()->MarkRoots(MAIN_THREAD_INDEX);
    heap_->GetCompressGCMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    heap_->WaitRunningTaskFinished();
}

void FullGC::Sweep()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::Sweep");
    // process weak reference
    auto totalThreadCount = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1; // gc thread and main thread
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        ProcessQueue *queue = workManager_->GetWeakReferenceQueue(i);

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
                if (!objectRegion->Test(header)) {
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
            if (objectRegion->Test(header)) {
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
    heap_->GetSweeper()->Sweep(true);
}

void FullGC::Finish()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "FullGC::Finish");
    heap_->GetSweeper()->PostConcurrentSweepTasks(true);
    heap_->Resume(FULL_GC);
    workManager_->Finish(youngAndOldAliveSize_);
}
}  // namespace panda::ecmascript
