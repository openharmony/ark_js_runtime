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

#include "js_arraylist.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
void JSArrayList::Add(JSThread *thread, const JSHandle<JSArrayList> &arrayList, const JSHandle<JSTaggedValue> &value)
{
    // GrowCapacity
    array_size_t length = arrayList->GetLength().GetArrayLength();
    JSHandle<TaggedArray> elements = GrowCapacity(thread, arrayList, length + 1);

    ASSERT(!elements->IsDictionaryMode());
    elements->Set(thread, length, value);
    arrayList->SetLength(thread, JSTaggedValue(++length));
}

JSHandle<TaggedArray> JSArrayList::GrowCapacity(const JSThread *thread, const JSHandle<JSArrayList> &obj,
                                                array_size_t capacity)
{
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    array_size_t oldLength = oldElements->GetLength();
    if (capacity < oldLength) {
        return oldElements;
    }
    array_size_t newCapacity = ComputeCapacity(capacity);
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldLength, newCapacity);

    obj->SetElements(thread, newElements);
    return newElements;
}

JSTaggedValue JSArrayList::Get(JSThread *thread, const uint32_t index)
{
    if (index < 0 || index >= GetLength().GetArrayLength()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Get property index out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    return elements->Get(index);
}

JSTaggedValue JSArrayList::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    if (index < 0 || index >= GetLength().GetArrayLength()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Set property index out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSArrayList::Delete(JSThread *thread, const JSHandle<JSArrayList> &obj, const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not delete a type other than number", false);
    }
    array_size_t length = obj->GetLength().GetArrayLength();
    if (index < 0 || index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Delete property index out-of-bounds", false);
    }
    TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetTaggedObject());
    for (array_size_t i = 0; i < length - 1; i++) {
        elements->Set(thread, i, elements->Get(i + 1));
    }
    obj->SetLength(thread, JSTaggedValue(--length));
    return true;
}

bool JSArrayList::Has(JSTaggedValue value) const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    return !(elements->GetIdx(value) == TaggedArray::MAX_ARRAY_INDEX);
}

JSHandle<TaggedArray> JSArrayList::OwnKeys(JSThread *thread, const JSHandle<JSArrayList> &obj)
{
    array_size_t length = obj->GetLength().GetArrayLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys = factory->NewTaggedArray(length);

    for (array_size_t i = 0; i < length; i++) {
        keys->Set(thread, i, JSTaggedValue(i));
    }

    return keys;
}

bool JSArrayList::GetOwnProperty(JSThread *thread, const JSHandle<JSArrayList> &obj,
                                 const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not get property whose type is not number", false);
    }

    array_size_t length = obj->GetLength().GetArrayLength();
    if (index < 0 || index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Get property index out-of-bounds", false);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(obj), key, desc);
}
}  // namespace panda::ecmascript
