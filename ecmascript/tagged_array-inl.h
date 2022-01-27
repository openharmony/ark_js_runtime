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

#ifndef ECMASCRIPT_TAGGED_ARRAY_INL_H
#define ECMASCRIPT_TAGGED_ARRAY_INL_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
inline JSTaggedValue TaggedArray::Get(uint32_t idx) const
{
    ASSERT(idx < GetLength());
    // Note: Here we can't statically decide the element type is a primitive or heap object, especially for
    //       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    return JSTaggedValue(Barriers::GetDynValue<JSTaggedType>(GetData(), offset));
}

inline JSTaggedValue TaggedArray::Get([[maybe_unused]] const JSThread *thread, uint32_t idx) const
{
    return Get(idx);
}

inline uint32_t TaggedArray::GetIdx(const JSTaggedValue &value) const
{
    uint32_t length = GetLength();

    for (uint32_t i = 0; i < length; i++) {
        if (JSTaggedValue::SameValue(Get(i), value)) {
            return i;
        }
    }
    return TaggedArray::MAX_ARRAY_INDEX;
}

template<typename T>
inline void TaggedArray::Set(const JSThread *thread, uint32_t idx, const JSHandle<T> &value)
{
    ASSERT(idx < GetLength());
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;

    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (value.GetTaggedValue().IsHeapObject()) {
        Barriers::SetDynObject<true>(thread, GetData(), offset, value.GetTaggedValue().GetRawData());
    } else {  // NOLINTNEXTLINE(readability-misleading-indentation)
        Barriers::SetDynPrimitive<JSTaggedType>(GetData(), offset, value.GetTaggedValue().GetRawData());
    }
}

inline void TaggedArray::Set(const JSThread *thread, uint32_t idx, const JSTaggedValue &value)
{
    ASSERT(idx < GetLength());
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;

    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (value.IsHeapObject()) {
        Barriers::SetDynObject<true>(thread, GetData(), offset, value.GetRawData());
    } else {  // NOLINTNEXTLINE(readability-misleading-indentation)
        Barriers::SetDynPrimitive<JSTaggedType>(GetData(), offset, value.GetRawData());
    }
}

JSHandle<TaggedArray> TaggedArray::Append(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                          const JSHandle<TaggedArray> &second)
{
    uint32_t firstLength = first->GetLength();
    uint32_t secondLength = second->GetLength();
    uint32_t length = firstLength + secondLength;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argument = factory->NewTaggedArray(length);
    uint32_t index = 0;
    for (; index < firstLength; ++index) {
        argument->Set(thread, index, first->Get(index));
    }
    for (; index < length; ++index) {
        argument->Set(thread, index, second->Get(index - firstLength));
    }
    return argument;
}

JSHandle<TaggedArray> TaggedArray::AppendSkipHole(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                                  const JSHandle<TaggedArray> &second, uint32_t copyLength)
{
    uint32_t firstLength = first->GetLength();
    uint32_t secondLength = second->GetLength();
    ASSERT(firstLength + secondLength >= copyLength);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argument = factory->NewTaggedArray(copyLength);
    uint32_t index = 0;
    for (; index < firstLength; ++index) {
        JSTaggedValue val = first->Get(index);
        if (val.IsHole()) {
            break;
        }
        argument->Set(thread, index, val);
        ASSERT(copyLength >= index);
    }
    for (uint32_t i = 0; i < secondLength; ++i) {
        JSTaggedValue val = second->Get(i);
        if (val.IsHole()) {
            break;
        }
        argument->Set(thread, index++, val);
        ASSERT(copyLength >= index);
    }
    return argument;
}

inline bool TaggedArray::HasDuplicateEntry() const
{
    uint32_t length = GetLength();
    for (uint32_t i = 0; i < length; i++) {
        for (uint32_t j = i + 1; j < length; j++) {
            if (JSTaggedValue::SameValue(Get(i), Get(j))) {
                return true;
            }
        }
    }
    return false;
}

void TaggedArray::InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t length)
{
    ASSERT(initValue.IsSpecial());
    SetLength(length);
    for (uint32_t i = 0; i < length; i++) {
        size_t offset = JSTaggedValue::TaggedTypeSize() * i;
        Barriers::SetDynPrimitive<JSTaggedType>(GetData(), offset, initValue.GetRawData());
    }
}

inline JSHandle<TaggedArray> TaggedArray::SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                                      uint32_t capa)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = array->GetLength();
    JSHandle<TaggedArray> newArray = factory->CopyArray(array, oldLength, capa);
    return newArray;
}

inline bool TaggedArray::IsDictionaryMode() const
{
    return GetClass()->IsDictionary();
}

void TaggedArray::Trim(JSThread *thread, uint32_t newLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = GetLength();
    ASSERT(oldLength > newLength);
    size_t trimBytes = (oldLength - newLength) * JSTaggedValue::TaggedTypeSize();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    factory->FillFreeObject(ToUintPtr(this) + size, trimBytes, RemoveSlots::YES);
    SetLength(newLength);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_ARRAY_INL_H
