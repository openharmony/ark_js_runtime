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

#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
Marker::Marker(Heap *heap) : heap_(heap), objXRay_(heap->GetEcmaVM()), workManager_(heap->GetWorkManager()) {}

void Marker::MarkRoots(uint32_t threadId)
{
    objXRay_.VisitVMRoots(
        std::bind(&Marker::HandleRoots, this, threadId, std::placeholders::_1, std::placeholders::_2),
        std::bind(&Marker::HandleRangeRoots, this, threadId, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    workManager_->PushWorkNodeToGlobal(threadId, false);
}

void Marker::ProcessOldToNew(uint32_t threadId)
{
    heap_->EnumerateOldSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
    ProcessMarkStack(threadId);
}

void Marker::ProcessOldToNew(uint32_t threadId, Region *region)
{
    heap_->EnumerateOldSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1),
                                    region);
    ProcessMarkStack(threadId);
}

void Marker::ProcessSnapshotRSet(uint32_t threadId)
{
    heap_->EnumerateSnapshotSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
    ProcessMarkStack(threadId);
}

void NonMovableMarker::ProcessMarkStack(uint32_t threadId)
{
    bool isFullMark = heap_->IsFullMark();
    auto visitor = [this, threadId, isFullMark](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                [[maybe_unused]] bool isNative) {
        Region *rootRegion = Region::ObjectAddressToRange(root);
        bool needBarrier = isFullMark && !rootRegion->InYoungSpaceOrCSet();
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                TaggedObject *obj = nullptr;
                if (!value.IsWeakForHeapObject()) {
                    obj = value.GetTaggedObject();
                    MarkObject(threadId, obj);
                } else {
                    RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                    obj = value.GetWeakReferentUnChecked();
                }
                if (needBarrier) {
                    Region *valueRegion = Region::ObjectAddressToRange(obj);
                    if (valueRegion->InCollectSet()) {
                        rootRegion->AtomicInsertCrossRegionRSet(slot.SlotAddress());
                    }
                }
            }
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            break;
        }

        JSHClass *jsHclass = obj->GetClass();
        MarkObject(threadId, jsHclass);
        objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
    }
}

void SemiGCMarker::Initialize()
{
    waterLine_ = heap_->GetNewSpace()->GetWaterLine();
}

void SemiGCMarker::ProcessMarkStack(uint32_t threadId)
{
    auto visitor = [this, threadId](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                    [[maybe_unused]] bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                if (value.IsWeakForHeapObject()) {
                    RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                    continue;
                }
                Region *rootRegion = Region::ObjectAddressToRange(root);
                auto slotStatus = MarkObject(threadId, value.GetTaggedObject(), slot);
                if (!rootRegion->InYoungSpace() && slotStatus == SlotStatus::KEEP_SLOT) {
                    SlotNeedUpdate waitUpdate(reinterpret_cast<TaggedObject *>(root), slot);
                    workManager_->PushSlotNeedUpdate(threadId, waitUpdate);
                }
            }
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            break;
        }

        auto jsHclass = obj->GetClass();
        objXRay_.VisitObjectBody<VisitType::SEMI_GC_VISIT>(obj, jsHclass, visitor);
    }
}

void CompressGCMarker::ProcessMarkStack(uint32_t threadId)
{
    auto visitor = [this, threadId]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                    [[maybe_unused]] bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsHeapObject()) {
                if (value.IsWeakForHeapObject()) {
                    RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
                    continue;
                }
                MarkObject(threadId, value.GetTaggedObject(), slot);
            }
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            break;
        }

        auto jsHClass = obj->GetClass();
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, jsHClass, objectSlot);
        objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHClass, visitor);
    }
}
}  // namespace panda::ecmascript
