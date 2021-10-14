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

#include "ecmascript/mem/compress_collector.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/compress_gc_marker-inl.h"
#include "ecmascript/mem/ecma_heap_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/heap_roots-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/vmstat/runtime_stat.h"

namespace panda::ecmascript {
CompressCollector::CompressCollector(Heap *heap, bool parallelGc)
    : heap_(heap), paralledGC_(parallelGc), marker_(this), rootManager_(heap->GetEcmaVM())
{
    workList_ = new CompressGCWorker(heap_, heap_->GetThreadPool()->GetThreadNum());
}

CompressCollector::~CompressCollector()
{
    if (workList_ != nullptr) {
        delete workList_;
        workList_ = nullptr;
    }
}

void CompressCollector::RunPhases()
{
    [[maybe_unused]] ecmascript::JSThread *thread = heap_->GetEcmaVM()->GetJSThread();
    INTERPRETER_TRACE(thread, CompressCollector_RunPhases);
    trace::ScopedTrace scoped_trace("CompressCollector::RunPhases");
    [[maybe_unused]] ClockScope clock("CompressCollector::RunPhases");
    InitializePhase();
    MarkingPhase();
    SweepPhases();
    FinishPhase();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticCompressCollector(clock.GetPauseTime(), youngAndOldAliveSize_,
                                                                     youngSpaceCommitSize_, oldSpaceCommitSize_,
                                                                     nonMoveSpaceFreeSize_, nonMoveSpaceCommitSize_);
}

void CompressCollector::InitializePhase()
{
    heap_->GetThreadPool()->WaitTaskFinish();
    heap_->GetSweeper()->EnsureAllTaskFinish();
    auto compressSpace = const_cast<OldSpace *>(heap_->GetCompressSpace());
    if (compressSpace->GetCommittedSize() == 0) {
        compressSpace->Initialize();
    }
    auto fromSpace = const_cast<SemiSpace *>(heap_->GetFromSpace());
    if (fromSpace->GetCommittedSize() == 0) {
        heap_->InitializeFromSpace();
    }

    FreeListAllocator compressAllocator(compressSpace);
    oldSpaceAllocator_.Swap(compressAllocator);
    fromSpaceAllocator_.Reset(fromSpace);

    auto callback = [](Region *current) {
        // ensure mark bitmap
        auto bitmap = current->GetMarkBitmap();
        if (bitmap == nullptr) {
            current->GetOrCreateMarkBitmap();
        } else {
            bitmap->ClearAllBits();
        }
        auto rememberset = current->GetOldToNewRememberedSet();
        if (rememberset != nullptr) {
            rememberset->ClearAllBits();
        }
    };
    heap_->GetNonMovableSpace()->EnumerateRegions(callback);
    heap_->GetMachineCodeSpace()->EnumerateRegions(callback);
    heap_->GetSnapShotSpace()->EnumerateRegions(callback);
    heap_->GetHugeObjectSpace()->EnumerateRegions(callback);

    heap_->FlipCompressSpace();
    heap_->FlipNewSpace();
    workList_->Initialize();
    youngAndOldAliveSize_ = 0;
    nonMoveSpaceFreeSize_ = 0;
    youngSpaceCommitSize_ = heap_->GetFromSpace()->GetCommittedSize();
    oldSpaceCommitSize_ = heap_->GetCompressSpace()->GetCommittedSize();
    nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
}

void CompressCollector::FinishPhase()
{
    // swap
    if (paralledGC_) {
        heap_->GetThreadPool()->Submit([this]([[maybe_unused]] uint32_t threadId) -> bool {
            const_cast<OldSpace *>(heap_->GetCompressSpace())->ReclaimRegions();
            const_cast<SemiSpace *>(heap_->GetFromSpace())->ReclaimRegions();
            return true;
        });
    } else {
        const_cast<OldSpace *>(heap_->GetCompressSpace())->ReclaimRegions();
        const_cast<SemiSpace *>(heap_->GetFromSpace())->ReclaimRegions();
    }
    workList_->Finish(youngAndOldAliveSize_);
    auto heapManager = heap_->GetHeapManager();
    heapManager->GetOldSpaceAllocator().Swap(oldSpaceAllocator_);
    heapManager->GetNewSpaceAllocator().Swap(fromSpaceAllocator_);
}

void CompressCollector::MarkingPhase()
{
    trace::ScopedTrace scoped_trace("MarkingPhase::SweepPhases");
    RootVisitor gcMarkYoung = [this]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            marker_.MarkObject(0, value.GetTaggedObject(), slot);
        }
    };
    RootRangeVisitor gcMarkRangeYoung = [this]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                marker_.MarkObject(0, value.GetTaggedObject(), slot);
            }
        }
    };
    rootManager_.VisitVMRoots(gcMarkYoung, gcMarkRangeYoung);

    ProcessMarkStack(0);
    if (paralledGC_) {
        heap_->GetThreadPool()->WaitTaskFinish();
    }
}

void CompressCollector::ProcessMarkStack(uint32_t threadId)
{
    while (true) {
        TaggedObject *obj = nullptr;
        if (!workList_->Pop(threadId, &obj)) {
            break;
        }
        auto jsHclass = obj->GetClass();
        ObjectSlot objectSlot(ToUintPtr(obj));
        // mark dynClass
        marker_.MarkObject(threadId, jsHclass, objectSlot);

        rootManager_.MarkObjectBody<GCType::OLD_GC>(
            obj, jsHclass, [this, threadId]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end) {
                for (ObjectSlot slot = start; slot < end; slot++) {
                    JSTaggedValue value(slot.GetTaggedType());
                    if (value.IsWeak()) {
                        RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                        continue;
                    }
                    if (value.IsHeapObject()) {
                        marker_.MarkObject(threadId, value.GetTaggedObject(), slot);
                    }
                }
            });
    }
}

void CompressCollector::SweepPhases()
{
    trace::ScopedTrace scoped_trace("CompressCollector::SweepPhases");
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
            TaggedObject *dst = markWord.ToForwardingAddress();
            return dst;
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    stringTable->SweepWeakReference(gcUpdateWeak);
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);

    heap_->GetSweeper()->SweepPhases(true);
}

uintptr_t CompressCollector::AllocateOld(size_t size)
{
    os::memory::LockHolder lock(mtx_);
    uintptr_t result = oldSpaceAllocator_.Allocate(size);
    if (UNLIKELY(result == 0)) {
        if (!heap_->FillOldSpaceAndTryGC(&oldSpaceAllocator_, false)) {
            return 0;
        }
        result = oldSpaceAllocator_.Allocate(size);
        if (UNLIKELY(result == 0)) {
            return 0;
        }
    }
    return result;
}

void CompressCollector::RecordWeakReference(uint32_t threadId, JSTaggedType *ref)
{
    workList_->PushWeakReference(threadId, ref);
}
}  // namespace panda::ecmascript
