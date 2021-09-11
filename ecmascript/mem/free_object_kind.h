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

#ifndef ECMASCRIPT_MEM_FREE_OBJECT_KIND_H
#define ECMASCRIPT_MEM_FREE_OBJECT_KIND_H

#include <cstdint>

#include "libpandabase/macros.h"

namespace panda::ecmascript {
using KindType = int32_t;

class FreeObject;

class FreeObjectKind {
public:
    FreeObjectKind(KindType type, uintptr_t begin, size_t size) : kindType_(type)
    {
        Free(begin, size);
    }
    ~FreeObjectKind() = default;

    inline bool Empty() const
    {
        return available_ == 0;
    }

    inline size_t Available() const
    {
        return available_;
    }

    void Free(uintptr_t begin, size_t size);

    void Rebuild();

    FreeObject *SearchSmallFreeObject(size_t size);
    FreeObject *SearchLargeFreeObject(size_t size);

    NO_COPY_SEMANTIC(FreeObjectKind);
    NO_MOVE_SEMANTIC(FreeObjectKind);

    static constexpr KindType INVALID_KIND_TYPE = -1;

private:
    FreeObjectKind *next_ = nullptr;
    FreeObjectKind *prev_ = nullptr;
    KindType kindType_ = INVALID_KIND_TYPE;
    size_t available_ = 0;
    FreeObject *freeObject_ = nullptr;

    friend class FreeObjectList;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_FREE_OBJECT_LIST_H
