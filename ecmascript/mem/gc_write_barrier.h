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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_GC_WRITE_BARRIER_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_GC_WRITE_BARRIER_H

#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript {
static inline void MarkingBarrier(void *obj, size_t offset, TaggedObject *value)
{
    ASSERT(ToUintPtr(value) != 0xa);
    Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(obj));
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (!objectRegion->InYoungGeneration() && valueRegion->InYoungGeneration()) {
        [[maybe_unused]] uintptr_t slotAddr = ToUintPtr(obj) + offset;
        objectRegion->InsertOldToNewRememberedSet(slotAddr);
    }
}
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_GC_WRITE_BARRIER_H