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

#include "ecmascript/mem/object_xray-inl.h"

namespace panda::ecmascript {
Marker::Marker(Heap *heap) : heap_(heap), objXRay_(heap_->GetEcmaVM()) {}

void Marker::MarkRoots(uint32_t threadId)
{
    objXRay_.VisitVMRoots(
        std::bind(&Marker::HandleRoots, this, threadId, std::placeholders::_1, std::placeholders::_2),
        std::bind(&Marker::HandleRangeRoots, this, threadId, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    heap_->GetWorkList()->PushWorkNodeToGlobal(threadId, false);
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
    heap_->EnumerateSnapShotSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
    ProcessMarkStack(threadId);
}

void NonMovableMarker::ProcessMarkStack(uint32_t threadId)
{
    WorkerHelper *worklist = heap_->GetWorkList();
    TaggedObject *obj = nullptr;
    bool isOnlySemi = heap_->IsOnlyMarkSemi();
    while (true) {
        obj = nullptr;
        if (!worklist->Pop(threadId, &obj)) {
            break;
        }

        JSHClass *jsHclass = obj->GetClass();
        MarkObject(threadId, jsHclass);

        Region *objectRegion = Region::ObjectAddressToRange(obj);
        bool needBarrier = !isOnlySemi && !objectRegion->InYoungAndCSetGeneration();
        objXRay_.VisitObjectBody<GCType::OLD_GC>(obj, jsHclass,
                                                    std::bind(&Marker::HandleObjectVisitor, this, threadId,
                                                              objectRegion, needBarrier, std::placeholders::_1,
                                                              std::placeholders::_2, std::placeholders::_3));
    }
}

void SemiGcMarker::Initialized()
{
    ageMark_ = heap_->GetNewSpace()->GetAgeMark();
}

void SemiGcMarker::ProcessMarkStack(uint32_t threadId)
{
    WorkerHelper *worklist = heap_->GetWorkList();
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!worklist->Pop(threadId, &obj)) {
            break;
        }

        auto jsHclass = obj->GetClass();
        Region *objectRegion = Region::ObjectAddressToRange(obj);
        bool promoted = !objectRegion->InYoungGeneration();
        objXRay_.VisitObjectBody<GCType::SEMI_GC>(obj, jsHclass,
                                                     std::bind(&Marker::HandleMoveObjectVisitor, this, threadId,
                                                               promoted, std::placeholders::_1, std::placeholders::_2,
                                                               std::placeholders::_3));
    }
}

void CompressGcMarker::ProcessMarkStack(uint32_t threadId)
{
    WorkerHelper *worklist = heap_->GetWorkList();
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!worklist->Pop(threadId, &obj)) {
            break;
        }

        auto jsHclass = obj->GetClass();
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, jsHclass, objectSlot);

        objXRay_.VisitObjectBody<GCType::OLD_GC>(obj, jsHclass,
                                                    std::bind(&Marker::HandleMoveObjectVisitor, this, threadId, false,
                                                              std::placeholders::_1, std::placeholders::_2,
                                                              std::placeholders::_3));
    }
}
}  // namespace panda::ecmascript