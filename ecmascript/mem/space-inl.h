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
#include "ecmascript/mem/remembered_set.h"

namespace panda::ecmascript {
template<class Callback>
void Space::EnumerateRegions(const Callback &cb, Region *region) const
{
    Region *current = regionList_.GetFirst();
    if (region == nullptr) {
        region = regionList_.GetLast();
    }
    while (current != region) {
        auto next = current->GetNext();
        cb(current);
        current = next;
    }

    if (region != nullptr) {
        cb(current);
    }
}

void Region::InsertCrossRegionRememberedSet(uintptr_t addr)
{
    auto set = GetOrCreateCrossRegionRememberedSet();
    set->Insert(addr);
}

void Region::InsertOldToNewRememberedSet(uintptr_t addr)
{
    auto set = GetOrCreateOldToNewRememberedSet();
    set->Insert(addr);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SPACE_INL_H
