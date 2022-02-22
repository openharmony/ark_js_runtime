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

#ifndef ECMASCRIPT_MEM_FREE_OBJECT_LIST_H
#define ECMASCRIPT_MEM_FREE_OBJECT_LIST_H

#include <cstddef>

#include "ecmascript/mem/free_object_set.h"
#include "utils/span.h"

namespace panda::ecmascript {
class FreeObjectList {
public:
    FreeObjectList();
    ~FreeObjectList();

    FreeObject *Allocate(size_t size);

    FreeObject *LookupSuitableFreeObject(size_t size);

    void Free(uintptr_t start, size_t size, bool isAdd = true);

    void Rebuild();

    bool AddSet(FreeObjectSet *set);

    void RemoveSet(FreeObjectSet *set);

    void Merge(FreeObjectList *list);

    template<class Callback>
    void EnumerateSets(const Callback &cb) const;

    template<class Callback>
    void EnumerateSets(SetType type, const Callback &cb) const;

    template<class Callback>
    void EnumerateTopAndLastSets(const Callback &cb) const;

    NO_COPY_SEMANTIC(FreeObjectList);
    NO_MOVE_SEMANTIC(FreeObjectList);

    size_t GetFreeObjectSize() const
    {
        return available_;
    }
    size_t GetWastedSize() const
    {
        return wasted_;
    }
    void DecrementWastedSize(size_t size)
    {
        wasted_ -= size;
    }
    void IncrementWastedSize(size_t size)
    {
        wasted_ += size;
    }

    static int NumberOfSets()
    {
        return NUMBER_OF_SETS;
    }

private:
    static constexpr int NUMBER_OF_SETS = 39;
    static constexpr size_t MIN_SIZE = 16;
    static constexpr size_t SMALL_SET_MAX_SIZE = 256;
    static constexpr size_t LARGE_SET_MAX_SIZE = 65536;
    static constexpr size_t HUGE_SET_MAX_SIZE = 255 * 1024;
    static constexpr int SMALL_SET_MAX_INDEX = 29;
    static constexpr int NUMBER_OF_LAST_LARGE = NUMBER_OF_SETS - 2;
    static constexpr int NUMBER_OF_LAST_HUGE = NUMBER_OF_SETS - 1;
    static constexpr size_t INTERVAL_OFFSET = 3;
    static constexpr size_t LOG2_OFFSET = 21;
    static constexpr size_t MAX_BIT_OF_SIZET = sizeof(size_t) << INTERVAL_OFFSET;
    const int smallSetOffsetIndex = 2;

    inline SetType SelectSetType(size_t size) const;

    inline void SetNoneEmptyBit(SetType type);
    inline void ClearNoneEmptyBit(SetType type);
    inline size_t CalcNextNoneEmptyIndex(SetType start);

    size_t available_ = 0;
    size_t wasted_ = 0;
    uint64_t noneEmptySetBitMap_;
    Span<FreeObjectSet *> sets_ {};
    Span<FreeObjectSet *> lastSets_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_FREE_OBJECT_LIST_H
