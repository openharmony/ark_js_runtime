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

#ifndef ECMASCRIPT_MEM_SPACE_INL_H
#define ECMASCRIPT_MEM_SPACE_INL_H

#include "ecmascript/mem/space.h"

namespace panda::ecmascript {
void Space::AddRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Add region:" << region << " to " << ToSpaceTypeName(spaceType_);
    regionList_.AddNode(region);
    IncrementCommitted(region->GetCapacity());
    IncrementLiveObjectSize(region->AliveObject());
}

void Space::RemoveRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Remove region:" << region << " to " << ToSpaceTypeName(spaceType_);
    regionList_.RemoveNode(region);
    DecrementCommitted(region->GetCapacity());
    DecrementLiveObjectSize(region->AliveObject());
}

template<class Callback>
void Space::EnumerateRegions(const Callback &cb, Region *end) const
{
    Region *current = regionList_.GetFirst();
    if (end == nullptr) {
        end = regionList_.GetLast();
    }
    while (current != end) {
        auto next = current->GetNext();
        cb(current);
        current = next;
    }

    if (current != nullptr) {
        cb(current);
    }
}

template<class Callback>
void OldSpace::EnumerateCollectRegionSet(const Callback &cb) const
{
    for (Region *current : collectRegionSet_) {
        if (current != nullptr) {
            cb(current);
        }
    }
}

template<class Callback>
void OldSpace::EnumerateNonCollectRegionSet(const Callback &cb) const
{
    EnumerateRegions([this, &cb](Region *region) {
        if (!region->InCollectSet()) {
            cb(region);
        }
    });
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SPACE_INL_H
