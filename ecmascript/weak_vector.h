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
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
class WeakVector : public TaggedArray {
public:
    static WeakVector *Cast(TaggedObject *object)
    {
        return static_cast<WeakVector *>(object);
    }

    static constexpr uint32_t DEFAULT_CAPACITY = 4;
    static JSHandle<WeakVector> Create(const JSThread *thread, uint32_t capacity = DEFAULT_CAPACITY);
    static JSHandle<WeakVector> Grow(const JSThread *thread, const JSHandle<WeakVector> &old, uint32_t newCapacity);
    uint32_t PushBack(const JSThread *thread, JSTaggedValue value);
    // just set index value to Hole
    bool Delete(const JSThread *thread, uint32_t index);

    inline uint32_t GetEnd() const
    {
        return TaggedArray::Get(END_INDEX).GetArrayLength();
    }

    inline bool Full() const
    {
        return GetEnd() == GetCapacity();
    }

    inline bool Empty() const
    {
        return GetEnd() == 0;
    }

    inline uint32_t GetCapacity() const
    {
        return TaggedArray::GetLength() - ELEMENTS_START_INDEX;
    }

    inline JSTaggedValue Get(uint32_t index) const
    {
        ASSERT(index < GetCapacity());
        return TaggedArray::Get(VectorToArrayIndex(index));
    }

    inline void Set(const JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        ASSERT(index < GetCapacity());
        TaggedArray::Set(thread, VectorToArrayIndex(index), value);
    }

private:
    static const uint32_t MIN_CAPACITY = 2;
    static const uint32_t END_INDEX = 0;
    static const uint32_t ELEMENTS_START_INDEX = 1;
    static const uint32_t MAX_VECTOR_INDEX = TaggedArray::MAX_ARRAY_INDEX - ELEMENTS_START_INDEX;

    inline static constexpr uint32_t VectorToArrayIndex(uint32_t index)
    {
        return index + ELEMENTS_START_INDEX;
    }

    inline void SetEnd(const JSThread *thread, uint32_t end)
    {
        ASSERT(end <= GetCapacity());
        TaggedArray::Set(thread, END_INDEX, JSTaggedValue(end));
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_WEAK_VECTOR_H
