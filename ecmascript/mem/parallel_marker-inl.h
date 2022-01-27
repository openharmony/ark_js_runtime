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

#ifndef ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H
#define ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H

#include "ecmascript/mem/parallel_marker.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

#include "mem/gc/bitmap.h"

namespace panda::ecmascript {
constexpr int HEAD_SIZE = TaggedObject::TaggedObjectSize();

inline void Marker::HandleObjectVisitor(uint32_t threadId, Region *objectRegion, bool needBarrier, TaggedObject *root,
                                        ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            TaggedObject *obj = nullptr;
            if (!value.IsWeak()) {
                obj = value.GetTaggedObject();
                MarkObject(threadId, obj);
            } else {
                obj = value.GetWeakReferentUnChecked();
            }
            if (needBarrier) {
                Region *valueRegion = Region::ObjectAddressToRange(obj);
                if (valueRegion->InCollectSet()) {
                    objectRegion->AtomicInsertCrossRegionRememberedSet(slot.SlotAddress());
                }
            }
        }
    }
}

inline void Marker::HandleMoveObjectVisitor(uint32_t threadId, bool promoted, TaggedObject *root, ObjectSlot start,
                                            ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            if (value.IsWeak()) {
                RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                continue;
            }
            auto slotStatus = MarkObject(threadId, value.GetTaggedObject(), slot);
            if (promoted && slotStatus == SlotStatus::KEEP_SLOT) {
                SlotNeedUpdate waitUpdate(reinterpret_cast<TaggedObject *>(root), slot);
                heap_->GetWorkList()->PushWaitUpdateSlot(threadId, waitUpdate);
            }
        }
    }
}

inline void NonMovableMarker::MarkObject(uint32_t threadId, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (heap_->IsSemiMarkNeeded() && !objectRegion->InYoungGeneration()) {
        return;
    }

    auto markBitmap = objectRegion->GetOrCreateMarkBitmap();
    if (!markBitmap->AtomicTestAndSet(object)) {
        heap_->GetWorkList()->Push(threadId, object, objectRegion);
    }
}

inline void NonMovableMarker::HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        MarkObject(threadId, value.GetTaggedObject());
    }
}

inline void NonMovableMarker::HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            MarkObject(threadId, value.GetTaggedObject());
        }
    }
}

inline void NonMovableMarker::HandleOldToNewRSet(uint32_t threadId, Region *region)
{
    auto oldRSet = region->GetOldToNewRememberedSet();
    if (LIKELY(oldRSet != nullptr)) {
        oldRSet->IterateOverMarkedChunks([this, threadId](void *mem) -> bool {
            ObjectSlot slot(ToUintPtr(mem));
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject() && !value.IsWeak()) {
                MarkObject(threadId, value.GetTaggedObject());
            }
            return true;
        });
    }
}

inline void NonMovableMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *ref)
{
    Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(ref));
    if (!objectRegion->InYoungGeneration()) {
        heap_->GetWorkList()->PushWeakReference(threadId, ref);
    }
}

inline void MovableMarker::HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        MarkObject(threadId, value.GetTaggedObject(), slot);
    }
}

inline void MovableMarker::HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            MarkObject(threadId, value.GetTaggedObject(), slot);
        }
    }
}

inline void MovableMarker::HandleOldToNewRSet(uint32_t threadId, Region *region)
{
    auto oldRSet = region->GetOldToNewRememberedSet();
    if (LIKELY(oldRSet != nullptr)) {
        oldRSet->IterateOverMarkedChunks([this, &oldRSet, threadId](void *mem) -> bool {
            ObjectSlot slot(ToUintPtr(mem));
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                if (value.IsWeak()) {
                    RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem));
                    return true;
                }
                auto slotStatus = MarkObject(threadId, value.GetTaggedObject(), slot);
                if (slotStatus == SlotStatus::CLEAR_SLOT) {
                    oldRSet->Clear(ToUintPtr(mem));
                }
            }
            return true;
        });
    }
}

inline uintptr_t MovableMarker::AllocateDstSpace(uint32_t threadId, size_t size, bool &shouldPromote)
{
    uintptr_t forwardAddress = 0;
    if (shouldPromote) {
        forwardAddress = heap_->GetWorkList()->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::OLD_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: "
                                << " size: " << size;
            UNREACHABLE();
        }
    } else {
        forwardAddress = heap_->GetWorkList()->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::YOUNG_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            forwardAddress = heap_->GetWorkList()->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::OLD_SPACE);
            if (UNLIKELY(forwardAddress == 0)) {
                LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: "
                                    << " size: " << size;
                UNREACHABLE();
            }
            shouldPromote = true;
        }
    }
    return forwardAddress;
}

inline void MovableMarker::UpdateForwardAddressIfSuccess(uint32_t threadId, TaggedObject *object, JSHClass *klass,
    uintptr_t toAddress, size_t size, const MarkWord &markWord, ObjectSlot slot)
{
    CopyObjectWithoutHeader(object, toAddress, size);
    heap_->GetWorkList()->AddAliveSize(threadId, size);
    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
    heap_->OnMoveEvent(reinterpret_cast<intptr_t>(object), toAddress);
    if (klass->HasReferenceField()) {
        heap_->GetWorkList()->Push(threadId, reinterpret_cast<TaggedObject *>(toAddress));
    }
    slot.Update(reinterpret_cast<TaggedObject *>(toAddress));
}

inline bool MovableMarker::UpdateForwardAddressIfFailed(TaggedObject *object, uintptr_t toAddress, size_t size,
    ObjectSlot slot)
{
    FreeObject::FillFreeObject(heap_->GetEcmaVM(), toAddress, size);
    TaggedObject *dst = MarkWord(object).ToForwardingAddress();
    slot.Update(dst);
    return Region::ObjectAddressToRange(dst)->InYoungGeneration();
}

inline void MovableMarker::CopyObjectWithoutHeader(TaggedObject *object, uintptr_t toAddress, size_t size)
{
    if (memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(ToUintPtr(object) + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_ECMA_MEM(FATAL) << "CopyObjectWithoutHeader memcpy_s failed: "
                            << " dst: " << toAddress << " src: " << ToUintPtr(object) << " size: " << size;
        UNREACHABLE();
    }
}

inline SlotStatus SemiGcMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InYoungGeneration()) {
        return SlotStatus::CLEAR_SLOT;
    }

    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        Region *valueRegion = Region::ObjectAddressToRange(dst);
        return valueRegion->InYoungGeneration() ? SlotStatus::KEEP_SLOT : SlotStatus::CLEAR_SLOT;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

inline SlotStatus SemiGcMarker::EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
    ObjectSlot slot)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    bool isPromoted = ShouldBePromoted(object);

    uintptr_t forwardAddress = AllocateDstSpace(threadId, size, isPromoted);
    bool result = Barriers::AtomicSetDynPrimitive(object, 0, markWord.GetValue(),
                                                  MarkWord::FromForwardingAddress(forwardAddress));
    if (result) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, markWord, slot);
        return isPromoted ? SlotStatus::CLEAR_SLOT : SlotStatus::KEEP_SLOT;
    }
    bool keepSlot = UpdateForwardAddressIfFailed(object, forwardAddress, size, slot);
    return keepSlot ? SlotStatus::KEEP_SLOT : SlotStatus::CLEAR_SLOT;
}

inline bool SemiGcMarker::ShouldBePromoted(TaggedObject *object)
{
    Region *region = Region::ObjectAddressToRange(object);
    return (region->BelowAgeMark() || (region->HasAgeMark() && ToUintPtr(object) < ageMark_));
}

inline void SemiGcMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *ref)
{
    auto value = JSTaggedValue(*ref);
    Region *objectRegion = Region::ObjectAddressToRange(value.GetTaggedWeakRef());
    if (objectRegion->InYoungGeneration()) {
        heap_->GetWorkList()->PushWeakReference(threadId, ref);
    }
}

inline SlotStatus CompressGcMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InYoungAndOldGeneration()) {
        auto markBitmap = objectRegion->GetMarkBitmap();
        if (!markBitmap->AtomicTestAndSet(object)) {
            heap_->GetWorkList()->Push(threadId, object);
        }
        return SlotStatus::CLEAR_SLOT;
    }

    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        return SlotStatus::CLEAR_SLOT;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

inline SlotStatus CompressGcMarker::EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
    ObjectSlot slot)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    bool isPromoted = true;

    uintptr_t forwardAddress = AllocateDstSpace(threadId, size, isPromoted);
    bool result = Barriers::AtomicSetDynPrimitive(object, 0, markWord.GetValue(),
                                                  MarkWord::FromForwardingAddress(forwardAddress));
    if (result) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, markWord, slot);
        return SlotStatus::CLEAR_SLOT;
    }
    UpdateForwardAddressIfFailed(object, forwardAddress, size, slot);
    return SlotStatus::CLEAR_SLOT;
}

inline void CompressGcMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *ref)
{
    heap_->GetWorkList()->PushWeakReference(threadId, ref);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H
