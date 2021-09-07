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

#ifndef ECMASCRIPT_MEM_FREE_OBJECT_LIST_INL_H
#define ECMASCRIPT_MEM_FREE_OBJECT_LIST_INL_H

#include "ecmascript/mem/free_object_list.h"

#include <cmath>

namespace panda::ecmascript {
KindType FreeObjectList::SelectKindType(size_t size) const
{
    if (size < SMALL_KIND_MAX_SIZE) {
        if (UNLIKELY(size < MIN_SIZE)) {
            return FreeObjectKind::INVALID_KIND_TYPE;
        }
        return (size >> INTERVAL_OFFSET) - smallKindOffsetIndex;
    }
    if (size < LARGE_KIND_MAX_SIZE) {
        return MAX_BIT_OF_SIZET - __builtin_clzl(size) + LOG2_OFFSET;
    }
    if (size >= HUGE_KIND_MAX_SIZE) {
        return NUMBER_OF_LAST_HUGE;
    }

    return NUMBER_OF_LAST_LARGE;
}

void FreeObjectList::SetNoneEmptyBit(KindType type)
{
    noneEmptyKindBitMap_ |= 1ULL << static_cast<uint32_t>(type);
}

void FreeObjectList::ClearNoneEmptyBit(KindType type)
{
    noneEmptyKindBitMap_ &= ~(1ULL << static_cast<uint32_t>(type));
}

inline size_t FreeObjectList::CalcNextNoneEmptyIndex(KindType start)
{
    return __builtin_ffsll(noneEmptyKindBitMap_ >> static_cast<uint32_t>(start)) + start - 1;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_FREE_OBJECT_LIST_INL_H
