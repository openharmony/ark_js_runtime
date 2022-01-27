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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATION_INL_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATION_INL_H

#include "ecmascript/mem/parallel_evacuation.h"

#include "ecmascript/mem/evacuation_allocator-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/platform/platform.h"
#include "mark_word.h"

namespace panda::ecmascript {
bool ParallelEvacuation::IsPromoteComplete(Region *region)
{
    return (static_cast<double>(region->AliveObject()) / DEFAULT_REGION_SIZE) > MIN_OBJECT_SURVIVAL_RATE &&
        !region->HasAgeMark();
}

bool ParallelEvacuation::UpdateObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        if (value.IsWeak()) {
            return UpdateWeakObjectSlot(value.GetTaggedWeakRef(), slot);
        }
        TaggedObject *object = value.GetTaggedObject();
        MarkWord markWord(object);
        if (markWord.IsForwardingAddress()) {
            TaggedObject *dst = markWord.ToForwardingAddress();
            slot.Update(dst);
        }
        return true;
    }
    return false;
}

bool ParallelEvacuation::UpdateWeakObjectSlot(TaggedObject *value, ObjectSlot &slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(value);
    if (objectRegion->InYoungAndCSetGeneration() && !objectRegion->InPromoteSet()) {
        MarkWord markWord(value);
        if (markWord.IsForwardingAddress()) {
            TaggedObject *dst = markWord.ToForwardingAddress();
            auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
            slot.Update(weakRef);
            return true;
        }
        slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
        return false;
    } else {
        if (!heap_->IsSemiMarkNeeded() || objectRegion->InPromoteSet()) {
            auto markBitmap = objectRegion->GetOrCreateMarkBitmap();
            if (!markBitmap->Test(value)) {
                slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
                return false;
            }
        }
        return true;
    }
}

std::unique_ptr<ParallelEvacuation::Fragment> ParallelEvacuation::GetFragmentSafe()
{
    os::memory::LockHolder holder(mutex_);
    std::unique_ptr<Fragment> unit;
    if (!fragments_.empty()) {
        unit = std::move(fragments_.back());
        fragments_.pop_back();
    }
    return unit;
}

void ParallelEvacuation::AddFragment(std::unique_ptr<Fragment> region)
{
    fragments_.emplace_back(std::move(region));
}

void ParallelEvacuation::AddSweptRegionSafe(Region *region)
{
    os::memory::LockHolder holder(mutex_);
    sweptList_.emplace_back(region);
}

void ParallelEvacuation::FillSweptRegion()
{
    while (!sweptList_.empty()) {
        Region *region = sweptList_.back();
        sweptList_.pop_back();
        region->EnumerateKinds([this](FreeObjectKind *kind) {
            if (kind == nullptr || kind->Empty()) {
                return;
            }
            evacuationAllocator_->FillFreeList(kind);
        });
    }
}

int ParallelEvacuation::CalculateEvacuationThreadNum()
{
    int length = fragments_.size();
    int regionPerThread = 8;
    return std::min(
        std::max(1, length / regionPerThread), static_cast<int>(Platform::GetCurrentPlatform()->GetTotalThreadNum()));
}

int ParallelEvacuation::CalculateUpdateThreadNum()
{
    int length = fragments_.size();
    double regionPerThread = 1.0 / 4;
    length = static_cast<int>(std::pow(length, regionPerThread));
    return std::min(std::max(1, length), static_cast<int>(Platform::GetCurrentPlatform()->GetTotalThreadNum()));
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATION_INL_H
