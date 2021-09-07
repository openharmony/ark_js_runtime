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

namespace panda::ecmascript {
OldSpaceCollector::OldSpaceCollector(Heap *heap) : heap_(heap), rootManager_(heap->GetEcmaVM()) {}

void OldSpaceCollector::RunPhases()
{
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
    markStack_.BeginMarking(heap_, heap_->GetMarkStack());
    weakProcessQueue_.BeginMarking(heap_, heap_->GetProcessQueue());
    auto heapManager = heap_->GetHeapManager();
    oldSpaceAllocator_.Swap(heapManager->GetOldSpaceAllocator());
    nonMovableAllocator_.Swap(heapManager->GetNonMovableSpaceAllocator());
    machineCodeSpaceAllocator_.Swap(heapManager->GetMachineCodeSpaceAllocator());
    heap_->EnumerateRegions([](Region *current) {
        // ensure mark bitmap
        auto bitmap = current->GetMarkBitmap();
        if (bitmap == nullptr) {
            current->GetOrCreateMarkBitmap();
        } else {
            bitmap->ClearAllBits();
        }
    });
    freeSize_ = 0;
    hugeSpaceFreeSize_ = 0;
    oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
    nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
}

void OldSpaceCollector::FinishPhase()
{
    // swap
    markStack_.FinishMarking(heap_->GetMarkStack());
    weakProcessQueue_.FinishMarking(heap_->GetProcessQueue());
    auto heapManager = heap_->GetHeapManager();
    heapManager->GetOldSpaceAllocator().Swap(oldSpaceAllocator_);
    heapManager->GetNonMovableSpaceAllocator().Swap(nonMovableAllocator_);
    heapManager->GetMachineCodeSpaceAllocator().Swap(machineCodeSpaceAllocator_);
}

void OldSpaceCollector::MarkingPhase()
{
    trace::ScopedTrace scoped_trace("OldSpaceCollector::MarkingPhase");
    RootVisitor gcMarkYoung = [this]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            MarkObject(value.GetTaggedObject());
        }
    };
    RootRangeVisitor gcMarkRangeYoung = [this]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                MarkObject(value.GetTaggedObject());
            }
        }
    };
    rootManager_.VisitVMRoots(gcMarkYoung, gcMarkRangeYoung);

    ProcessMarkStack();
}

void OldSpaceCollector::ProcessMarkStack()
{
    while (true) {
        auto obj = markStack_.PopBack();
        if (UNLIKELY(obj == nullptr)) {
            break;
        }
        auto jsHclass = obj->GetClass();
        // mark dynClass
        MarkObject(jsHclass);

        rootManager_.MarkObjectBody<GCType::OLD_GC>(
            obj, jsHclass, [this]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end) {
                for (ObjectSlot slot = start; slot < end; slot++) {
                    JSTaggedValue value(slot.GetTaggedType());
                    if (value.IsWeak()) {
                        RecordWeakReference(reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                        continue;
                    }
                    if (value.IsHeapObject()) {
                        MarkObject(value.GetTaggedObject());
                    }
                }
            });
    }
}

void OldSpaceCollector::SweepSpace(Space *space, FreeListAllocator &allocator)
{
    allocator.RebuildFreeList();
    space->EnumerateRegions([this, &allocator](Region *current) {
        auto markBitmap = current->GetMarkBitmap();
        ASSERT(markBitmap != nullptr);
        uintptr_t freeStart = current->GetBegin();
        markBitmap->IterateOverMarkedChunks([this, &current, &freeStart, &allocator](void *mem) {
            ASSERT(current->InRange(ToUintPtr(mem)));
            auto header = reinterpret_cast<TaggedObject *>(mem);
            auto klass = header->GetClass();
            JSType jsType = klass->GetObjectType();
            auto size = klass->SizeFromJSHClass(jsType, header);
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));

            uintptr_t freeEnd = ToUintPtr(mem);
            if (freeStart != freeEnd) {
                FreeLiveRange(allocator, current, freeStart, freeEnd);
            }
            freeStart = freeEnd + size;
        });
        uintptr_t freeEnd = current->GetEnd();
        if (freeStart != freeEnd) {
            FreeLiveRange(allocator, current, freeStart, freeEnd);
        }
    });
}

void OldSpaceCollector::SweepSpace(HugeObjectSpace *space)
{
    Region *currentRegion = space->GetRegionList().GetFirst();

    while (currentRegion != nullptr) {
        Region *next = currentRegion->GetNext();
        auto markBitmap = currentRegion->GetMarkBitmap();
        bool isMarked = false;
        markBitmap->IterateOverMarkedChunks([&isMarked]([[maybe_unused]] void *mem) { isMarked = true; });
        if (!isMarked) {
            space->GetRegionList().RemoveNode(currentRegion);
            hugeSpaceFreeSize_ += currentRegion->GetCapacity();
            space->ClearAndFreeRegion(currentRegion);
        }
        currentRegion = next;
    }
}

void OldSpaceCollector::SweepPhases()
{
    trace::ScopedTrace scoped_trace("OldSpaceCollector::SweepPhases");
    // process weak reference
    while (true) {
        auto obj = weakProcessQueue_.PopBack();
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

    SweepSpace(const_cast<OldSpace *>(heap_->GetOldSpace()), oldSpaceAllocator_);
    SweepSpace(const_cast<NonMovableSpace *>(heap_->GetNonMovableSpace()), nonMovableAllocator_);
    SweepSpace(const_cast<HugeObjectSpace *>(heap_->GetHugeObjectSpace()));
    SweepSpace(const_cast<MachineCodeSpace *>(heap_->GetMachineCodeSpace()), machineCodeSpaceAllocator_);
}
}  // namespace panda::ecmascript
