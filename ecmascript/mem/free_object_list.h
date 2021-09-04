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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_FREE_OBJECT_LIST_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_FREE_OBJECT_LIST_H

#include <cstddef>

#include "ecmascript/mem/free_object_kind.h"
#include "utils/span.h"

namespace panda::ecmascript {
class FreeObjectList {
public:
    FreeObjectList();
    ~FreeObjectList();

    FreeObject *Allocator(size_t size);

    void Free(uintptr_t start, size_t size);

    void Rebuild();

    NO_COPY_SEMANTIC(FreeObjectList);
    NO_MOVE_SEMANTIC(FreeObjectList);

    size_t GetFreeObjectSize() const;

private:
    static constexpr int NUMBER_OF_KINDS = 39;
    static constexpr size_t MIN_SIZE = 16;
    static constexpr size_t SMALL_KIND_MAX_SIZE = 256;
    static constexpr size_t LARGE_KIND_MAX_SIZE = 65536;
    static constexpr size_t HUGE_KIND_MAX_SIZE = 255 * 1024;
    static constexpr int SMALL_KIND_MAX_INDEX = 29;
    static constexpr int NUMBER_OF_LAST_LARGE = NUMBER_OF_KINDS - 2;
    static constexpr int NUMBER_OF_LAST_HUGE = NUMBER_OF_KINDS - 1;
    static constexpr size_t INTERVAL_OFFSET = 3;
    static constexpr size_t LOG2_OFFSET = 21;
    static constexpr size_t MAX_BIT_OF_SIZET = sizeof(size_t) << INTERVAL_OFFSET;
    const int smallKindOffsetIndex = 2;

    inline KindType SelectKindType(size_t size) const;

    inline void SetNoneEmptyBit(KindType type);
    inline void ClearNoneEmptyBit(KindType type);
    inline size_t CalcNextNoneEmptyIndex(KindType start);

    bool AddKind(FreeObjectKind *kind);
    void RemoveKind(FreeObjectKind *kind);

    size_t available_ = 0;
    uint64_t noneEmptyKindBitMap_;
    Span<FreeObjectKind *> kinds_ {};
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_FREE_OBJECT_LIST_H
