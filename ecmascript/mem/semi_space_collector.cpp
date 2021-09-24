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

#include "ecmascript/mem/semi_space_collector-inl.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/ecma_heap_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/heap_roots-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/vmstat/runtime_stat.h"

namespace panda::ecmascript {
SemiSpaceCollector::SemiSpaceCollector(Heap *heap, bool parallelGc)
    : heap_(heap), rootManager_(heap->GetEcmaVM()), paralledGC_(parallelGc), markObject_(this)
{
    workList_ = new SemiSpaceWorker(heap_, heap_->GetThreadPool()->GetThreadNum());
}

SemiSpaceCollector::~SemiSpaceCollector()
{
    if (workList_ != nullptr) {
        delete workList_;
        workList_ = nullptr;
    }
}

void SemiSpaceCollector::RunPhases()
{
    [[maybe_unused]] ecmascript::JSThread *thread = heap_->GetEcmaVM()->GetJSThread();
    INTERPRETER_TRACE(thread, SemiSpaceCollector_RunPhases);
    trace::ScopedTrace scoped_trace("SemiSpaceCollector::RunPhases");
    [[maybe_unused]] ClockScope clock("SemiSpaceCollector::RunPhases");
    InitializePhase();
    ParallelMarkingPhase();
    SweepPhases();
    FinishPhase();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticSemiCollector(clock.GetPauseTime(), semiCopiedSize_, promotedSize_,
                                                                 commitSize_);
}

void SemiSpaceCollector::InitializePhase()
{
    heap_->GetThreadPool()->WaitTaskFinish();
    heap_->GetSweeper()->EnsureAllTaskFinish();
    auto fromSpace = heap_->GetFromSpace();
    if (fromSpace->GetCommittedSize() == 0) {
        heap_->InitializeFromSpace();
    }
    fromSpaceAllocator_.Reset(fromSpace);
    auto heapManager = heap_->GetHeapManager();
    oldSpaceAllocator_.Swap(heapManager->GetOldSpaceAllocator());
    ageMark_ = heap_->GetNewSpace()->GetAgeMark();
    heap_->FlipNewSpace();
    workList_->Initialize();
    promotedSize_ = 0;
    semiCopiedSize_ = 0;
    commitSize_ = heap_->GetFromSpace()->GetCommittedSize();
}

void SemiSpaceCollector::FinishPhase()
{
    // swap
    const_cast<SemiSpace *>(heap_->GetNewSpace())->Swap(const_cast<SemiSpace *>(heap_->GetFromSpace()));
    if (paralledGC_) {
        heap_->GetThreadPool()->Submit([this]([[maybe_unused]] uint32_t threadId) -> bool {
            const_cast<SemiSpace *>(heap_->GetFromSpace())->ReclaimRegions();
            return true;
        });
    } else {
        const_cast<SemiSpace *>(heap_->GetFromSpace())->ReclaimRegions();
    }
    workList_->Finish(semiCopiedSize_, promotedSize_);
    auto heapManager = heap_->GetHeapManager();
    heapManager->GetOldSpaceAllocator().Swap(oldSpaceAllocator_);
    heapManager->GetNewSpaceAllocator().Swap(fromSpaceAllocator_);

    auto newSpace = heap_->GetNewSpace();
    heap_->SetNewSpaceAgeMark(fromSpaceAllocator_.GetTop());
    Region *last = newSpace->GetCurrentRegion();
    last->SetFlag(RegionFlags::HAS_AGE_MARK);

    newSpace->EnumerateRegions([&last](Region *current) {
        if (current != last) {
            current->SetFlag(RegionFlags::BELOW_AGE_MARK);
        }
    });
}

bool SemiSpaceCollector::ParallelHandleOldToNew(uint32_t threadId, Region *region)
{
    auto cb = [this, threadId](Region *current) {
        auto rememberedSet = current->GetOldToNewRememberedSet();
        if (LIKELY(rememberedSet != nullptr)) {
            rememberedSet->IterateOverMarkedChunks([&rememberedSet, this, threadId](void *mem) -> bool {
                ObjectSlot slot(ToUintPtr(mem));
                JSTaggedValue value(slot.GetTaggedType());

                if (value.IsWeak()) {
                    RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem));
                } else if (value.IsHeapObject()) {
                    auto slotStatus = markObject_.MarkObject(threadId, value.GetTaggedObject(), slot);
                    if (slotStatus == SlotStatus::CLEAR_SLOT) {
                        // clear bit
                        rememberedSet->Clear(ToUintPtr(mem));
                    }
                }
                return true;
            });
        }
    };
    heap_->EnumerateOldSpaceRegions(cb, region);
    ProcessMarkStack(threadId);
    return true;
}

bool SemiSpaceCollector::ParallelHandleGlobalPool(uint32_t threadId)
{
    ProcessMarkStack(threadId);
    return true;
}

bool SemiSpaceCollector::ParallelHandleThreadRoots(uint32_t threadId)
{
    RootVisitor gcMarkYoung = [this, threadId]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            markObject_.MarkObject(threadId, value.GetTaggedObject(), slot);
        }
    };
    RootRangeVisitor gcMarkRangeYoung = [this, threadId]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                markObject_.MarkObject(threadId, value.GetTaggedObject(), slot);
            }
        }
    };

    rootManager_.VisitVMRoots(gcMarkYoung, gcMarkRangeYoung);
    ProcessMarkStack(threadId);
    return true;
}

bool SemiSpaceCollector::ParallelHandleSnapShot(uint32_t threadId)
{
    auto cb = [this, threadId](Region *current) {
        auto rememberedSet = current->GetOldToNewRememberedSet();
        if (LIKELY(rememberedSet != nullptr)) {
            rememberedSet->IterateOverMarkedChunks([&rememberedSet, this, threadId](void *mem) -> bool {
                ObjectSlot slot(ToUintPtr(mem));
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsWeak()) {
                    RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem));
                } else if (value.IsHeapObject()) {
                    auto slotStatus = markObject_.MarkObject(threadId, value.GetTaggedObject(), slot);
                    if (slotStatus == SlotStatus::CLEAR_SLOT) {
                        // clear bit
                        rememberedSet->Clear(ToUintPtr(mem));
                    }
                }
                return true;
            });
        }
    };

    heap_->EnumerateSnapShotSpaceRegions(cb);
    ProcessMarkStack(threadId);
    return true;
}

void SemiSpaceCollector::ParallelMarkingPhase()
{
    trace::ScopedTrace scoped_trace("SemiSpaceCollector::ParallelMarkingPhase");
    auto oldSpace = heap_->GetOldSpace();
    auto region = oldSpace->GetCurrentRegion();

    if (paralledGC_) {
        heap_->GetThreadPool()->Submit(
            std::bind(&SemiSpaceCollector::ParallelHandleThreadRoots, this, std::placeholders::_1));
        heap_->GetThreadPool()->Submit(
            std::bind(&SemiSpaceCollector::ParallelHandleSnapShot, this, std::placeholders::_1));
        ParallelHandleOldToNew(0, region);
        heap_->GetThreadPool()->WaitTaskFinish();
    } else {
        ParallelHandleOldToNew(0, region);
        ParallelHandleSnapShot(0);
        ParallelHandleThreadRoots(0);
    }

    for (uint32_t i = 0; i < heap_->GetThreadPool()->GetThreadNum(); i++) {
        SlotNeedUpdate needUpdate(nullptr, ObjectSlot(0));
        while (workList_->GetSlotNeedUpdate(i, &needUpdate)) {
            UpdatePromotedSlot(needUpdate.first, needUpdate.second);
        }
    }
}

void SemiSpaceCollector::ProcessMarkStack(uint64_t threadId)
{
    while (true) {
        TaggedObject *obj = nullptr;
        if (!workList_->Pop(threadId, &obj)) {
            break;
        }

        auto jsHclass = obj->GetClass();
        Region *objectRegion = Region::ObjectAddressToRange(obj);
        bool promoted = !objectRegion->InYoungGeneration();
        rootManager_.MarkObjectBody<GCType::SEMI_GC>(
            obj, jsHclass,
            [this, &promoted, threadId]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end) {
                for (ObjectSlot slot = start; slot < end; slot++) {
                    JSTaggedValue value(slot.GetTaggedType());
                    if (value.IsWeak()) {
                        RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                        continue;
                    }
                    if (value.IsHeapObject()) {
                        auto slotStatus = markObject_.MarkObject(threadId, value.GetTaggedObject(), slot);
                        if (promoted && slotStatus == SlotStatus::KEEP_SLOT) {
                            SlotNeedUpdate waitUpdate(reinterpret_cast<TaggedObject *>(root), slot);
                            workList_->PushWaitUpdateSlot(threadId, waitUpdate);
                        }
                    }
                }
            });
    }
}

void SemiSpaceCollector::SweepPhases()
{
    trace::ScopedTrace scoped_trace("SemiSpaceCollector::SweepPhases");
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

    auto stringTable = heap_->GetEcmaVM()->GetEcmaStringTable();
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (!objectRegion->InYoungGeneration()) {
            return header;
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
}
}  // namespace panda::ecmascript
