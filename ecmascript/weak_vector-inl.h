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

#ifndef ECMASCRIPT_WEAK_VECTOR_INL_H
#define ECMASCRIPT_WEAK_VECTOR_INL_H

#include "weak_vector.h"
#include "tagged_array-inl.h"

namespace panda::ecmascript {
array_size_t WeakVector::GetEnd() const
{
    return TaggedArray::Get(END_INDEX).GetArrayLength();
}

bool WeakVector::Full() const
{
    return GetEnd() == GetCapacity();
}

bool WeakVector::Empty() const
{
    return GetEnd() == 0;
}

array_size_t WeakVector::GetCapacity() const
{
    return TaggedArray::GetLength() - ELEMENTS_START_INDEX;
}

JSTaggedValue WeakVector::Get(array_size_t index) const
{
    ASSERT(index < GetCapacity());
    return TaggedArray::Get(VectorToArrayIndex(index));
}

void WeakVector::Set(const JSThread *thread, array_size_t index, JSTaggedValue value)
{
    ASSERT(index < GetCapacity());
    TaggedArray::Set(thread, VectorToArrayIndex(index), value);
}

void WeakVector::SetEnd(const JSThread *thread, array_size_t end)
{
    ASSERT(end <= GetCapacity());
    TaggedArray::Set(thread, END_INDEX, JSTaggedValue(end));
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_WEAK_VECTOR_INL_H