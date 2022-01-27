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

#ifndef ECMASCRIPT_WEAK_VECTOR_H
#define ECMASCRIPT_WEAK_VECTOR_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class WeakVector : public TaggedArray {
public:
    static WeakVector *Cast(ObjectHeader *object)
    {
        return static_cast<WeakVector *>(object);
    }

    static constexpr uint32_t DEFALUT_CAPACITY = 4;
    static JSHandle<WeakVector> Create(const JSThread *thread, uint32_t capacity = DEFALUT_CAPACITY);
    static JSHandle<WeakVector> Grow(const JSThread *thread, const JSHandle<WeakVector> &old, uint32_t newCapacity);
    uint32_t PushBack(const JSThread *thread, JSTaggedValue value);
    // just set index value to Hole
    bool Delete(const JSThread *thread, uint32_t index);

    inline uint32_t GetEnd() const;

    inline bool Full() const;

    inline bool Empty() const;

    inline uint32_t GetCapacity() const;

    inline JSTaggedValue Get(uint32_t index) const;

    inline void Set(const JSThread *thread, uint32_t index, JSTaggedValue value);

private:
    static const uint32_t MIN_CAPACITY = 2;
    static const uint32_t END_INDEX = 0;
    static const uint32_t ELEMENTS_START_INDEX = 1;
    static const uint32_t MAX_VECTOR_INDEX = TaggedArray::MAX_ARRAY_INDEX - ELEMENTS_START_INDEX;

    inline static constexpr uint32_t VectorToArrayIndex(uint32_t index)
    {
        return index + ELEMENTS_START_INDEX;
    }

    inline void SetEnd(const JSThread *thread, uint32_t end);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_WEAK_VECTOR_H
