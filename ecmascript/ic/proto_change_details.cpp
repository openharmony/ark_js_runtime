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

#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/weak_vector-inl.h"

namespace panda::ecmascript {
JSHandle<ChangeListener> ChangeListener::Add(const JSThread *thread, const JSHandle<ChangeListener> &array,
                                             const JSHandle<JSHClass> &value, array_size_t *index)
{
    JSTaggedValue weakValue;
    if (!array->Full()) {
        weakValue = JSTaggedValue(value.GetTaggedValue().CreateAndGetWeakRef());
        array_size_t arrayIndex = array->PushBack(thread, weakValue);
        if (arrayIndex != TaggedArray::MAX_ARRAY_INDEX) {
            if (index != nullptr) {
                *index = arrayIndex;
            }
            return array;
        }
        UNREACHABLE();
    }
    // if exist hole, use it.
    array_size_t holeIndex = CheckHole(array);
    if (holeIndex != TaggedArray::MAX_ARRAY_INDEX) {
        weakValue = JSTaggedValue(value.GetTaggedValue().CreateAndGetWeakRef());
        array->Set(thread, holeIndex, weakValue);
        if (index != nullptr) {
            *index = holeIndex;
        }
        return array;
    }
    // the vector is full and no hole exists.
    JSHandle<WeakVector> newArray = WeakVector::Grow(thread, JSHandle<WeakVector>(array), array->GetCapacity() + 1);
    weakValue = JSTaggedValue(value.GetTaggedValue().CreateAndGetWeakRef());
    array_size_t arrayIndex = newArray->PushBack(thread, weakValue);
    ASSERT(arrayIndex != TaggedArray::MAX_ARRAY_INDEX);
    if (index != nullptr) {
        *index = arrayIndex;
    }
    return JSHandle<ChangeListener>(newArray);
}

array_size_t ChangeListener::CheckHole(const JSHandle<ChangeListener> &array)
{
    for (array_size_t i = 0; i < array->GetEnd(); i++) {
        JSTaggedValue value = array->Get(i);
        if (value == JSTaggedValue::Hole() || value == JSTaggedValue::Undefined()) {
            return i;
        }
    }
    return TaggedArray::MAX_ARRAY_INDEX;
}

JSTaggedValue ChangeListener::Get(array_size_t index)
{
    JSTaggedValue value = WeakVector::Get(index);
    if (!value.IsHeapObject()) {
        return value;
    }
    return JSTaggedValue(value.GetTaggedWeakRef());
}
}  // namespace panda::ecmascript