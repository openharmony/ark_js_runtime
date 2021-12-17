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

#ifndef ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H
#define ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H

#include "ecmascript/mem/semi_space_collector.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/parallel_work_helper.h"

namespace panda::ecmascript {
void SemiSpaceCollector::UpdatePromotedSlot(TaggedObject *object, ObjectSlot slot)
{
#ifndef NDEBUG
    JSTaggedValue value(slot.GetTaggedType());
    ASSERT(value.IsHeapObject());
#endif
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(!objectRegion->InYoungGeneration());
    objectRegion->InsertOldToNewRememberedSet(slot.SlotAddress());
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H
