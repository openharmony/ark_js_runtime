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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_BARRIERS_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_BARRIERS_INL_H

#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript {
static inline void MarkingBarrier(void *obj, size_t offset, JSTaggedType value)
{
    ASSERT(value != JSTaggedValue::VALUE_UNDEFINED);
    Region *object_region = Region::ObjectAddressToRange(static_cast<TaggedObject *>(obj));
    Region *value_region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value));
    if (!object_region->InYoungGeneration() && value_region->InYoungGeneration()) {
        [[maybe_unused]] uintptr_t slot_addr = ToUintPtr(obj) + offset;
        // Should align with '8' in 64 and 32 bit platform
        ASSERT((slot_addr % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
        object_region->InsertOldToNewRememberedSet(slot_addr);
    }
}

/* static */
// CODECHECK-NOLINTNEXTLINE(C_RULE_ID_COMMENT_LOCATION)
template <bool need_write_barrier /* = true */>
inline void Barriers::SetDynObject([[maybe_unused]] const JSThread *thread, void *obj, size_t offset,
                                   JSTaggedType value)
{
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    *reinterpret_cast<JSTaggedType *>(reinterpret_cast<uintptr_t>(obj) + offset) = value;
    MarkingBarrier(obj, offset, value);
}
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_BARRIERS_INL_H
