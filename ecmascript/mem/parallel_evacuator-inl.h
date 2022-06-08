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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATOR_INL_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATOR_INL_H

#include "ecmascript/mem/parallel_evacuator.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
// Move regions with a survival rate of more than 75% to new space
bool ParallelEvacuator::IsWholeRegionEvacuate(Region *region)
{
    return (static_cast<double>(region->AliveObject()) / region->GetSize()) > MIN_OBJECT_SURVIVAL_RATE &&
        !region->HasAgeMark();
}

bool ParallelEvacuator::UpdateObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        if (value.IsWeakForHeapObject()) {
            return UpdateWeakObjectSlot(value.GetTaggedWeakRef(), slot);
        }
        TaggedObject *object = value.GetTaggedObject();
        MarkWord markWord(object);
        if (markWord.IsForwardingAddress()) {
            TaggedObject *dst = markWord.ToForwardingAddress();
            slot.Update(dst);
            return true;
        }
    }
    return false;
}

bool ParallelEvacuator::UpdateWeakObjectSlot(TaggedObject *value, ObjectSlot &slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(value);
    if (objectRegion->InYoungSpaceOrCSet()) {
        if (objectRegion->InNewToNewSet()) {
            if (!objectRegion->Test(value)) {
                slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
            }
        } else {
            MarkWord markWord(value);
            if (markWord.IsForwardingAddress()) {
                TaggedObject *dst = markWord.ToForwardingAddress();
                auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
                slot.Update(weakRef);
                return true;
            }
            slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
        }
        return false;
    }

    if (heap_->IsFullMark()) {
        if (!objectRegion->Test(value)) {
            slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
        }
    }
    return false;
}

void ParallelEvacuator::SetObjectFieldRSet(TaggedObject *object, JSHClass *cls)
{
    Region *region = Region::ObjectAddressToRange(object);
    auto callbackWithCSet = [region]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                     [[maybe_unused]] bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedType value = slot.GetTaggedType();
            if (!JSTaggedValue(value).IsHeapObject()) {
                continue;
            }
            Region *valueRegion = Region::ObjectAddressToRange(value);
            if (valueRegion->InYoungSpace()) {
                region->InsertOldToNewRSet(slot.SlotAddress());
            } else if (valueRegion->InCollectSet() || JSTaggedValue(value).IsWeakForHeapObject()) {
                region->InsertCrossRegionRSet(slot.SlotAddress());
            }
        }
    };
    objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls, callbackWithCSet);
}


std::unique_ptr<ParallelEvacuator::Workload> ParallelEvacuator::GetWorkloadSafe()
{
    os::memory::LockHolder holder(mutex_);
    std::unique_ptr<Workload> unit;
    if (!workloads_.empty()) {
        unit = std::move(workloads_.back());
        workloads_.pop_back();
    }
    return unit;
}

void ParallelEvacuator::AddWorkload(std::unique_ptr<Workload> region)
{
    workloads_.emplace_back(std::move(region));
}

int ParallelEvacuator::CalculateEvacuationThreadNum()
{
    uint32_t length = workloads_.size();
    uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = std::min(heap_->GetMaxEvacuateTaskCount(),
        Taskpool::GetCurrentTaskpool()->GetTotalThreadNum());
    return static_cast<int>(std::min(std::max(1U, length / regionPerThread), maxThreadNum));
}

int ParallelEvacuator::CalculateUpdateThreadNum()
{
    uint32_t length = workloads_.size();
    double regionPerThread = 1.0 / 4;
    length = std::pow(length, regionPerThread);
    uint32_t maxThreadNum = std::min(heap_->GetMaxEvacuateTaskCount(),
        Taskpool::GetCurrentTaskpool()->GetTotalThreadNum());
    return static_cast<int>(std::min(std::max(1U, length), maxThreadNum));
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATOR_INL_H
