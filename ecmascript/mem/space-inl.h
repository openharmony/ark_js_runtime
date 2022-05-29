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
    IncreaseCommitted(region->GetCapacity());
    IncreaseObjectSize(region->GetSize());
}

void Space::RemoveRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Remove region:" << region << " to " << ToSpaceTypeName(spaceType_);
    regionList_.RemoveNode(region);
    DecreaseCommitted(region->GetCapacity());
    DecreaseObjectSize(region->GetSize());
}

template<class Callback>
void Space::EnumerateRegions(const Callback &cb, Region *end) const
{
    Region *current = regionList_.GetFirst();
    if (current == nullptr) {
        return;
    }
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
void Space::EnumerateRegionsWithRecord(const Callback &cb) const
{
    EnumerateRegions(cb, recordRegion_);
}

RegionFlags Space::GetRegionFlag() const
{
    RegionFlags flags = RegionFlags::IS_INVALID;
    switch (spaceType_) {
        case MemSpaceType::OLD_SPACE:
        case MemSpaceType::LOCAL_SPACE:
            flags = RegionFlags::IS_IN_OLD_GENERATION;
            break;
        case MemSpaceType::NON_MOVABLE:
        case MemSpaceType::MACHINE_CODE_SPACE:
            flags = RegionFlags::IS_IN_NON_MOVABLE_GENERATION;
            break;
        case MemSpaceType::SEMI_SPACE:
            flags = RegionFlags::IS_IN_YOUNG_GENERATION;
            break;
        case MemSpaceType::SNAPSHOT_SPACE:
            flags = RegionFlags::IS_IN_SNAPSHOT_GENERATION;
            break;
        case MemSpaceType::HUGE_OBJECT_SPACE:
            flags = RegionFlags::IS_HUGE_OBJECT;
            break;
        default:
            UNREACHABLE();
            break;
    }
    return flags;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPACE_INL_H
