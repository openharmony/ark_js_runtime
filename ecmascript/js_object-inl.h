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

#ifndef ECMASCRIPT_JSOBJECT_INL_H
#define ECMASCRIPT_JSOBJECT_INL_H

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
inline void ECMAObject::SetBuiltinsCtorMode()
{
    GetClass()->SetBuiltinsCtor(true);
}

inline bool ECMAObject::IsBuiltinsConstructor() const
{
    return GetClass()->IsBuiltinsCtor();
}

inline void ECMAObject::SetCallable(bool flag)
{
    GetClass()->SetCallable(flag);
}

inline bool ECMAObject::IsCallable() const
{
    return GetClass()->IsCallable();
}

// JSObject
inline bool JSObject::IsExtensible() const
{
    return GetJSHClass()->IsExtensible();
}

inline void JSObject::FillElementsWithHoles(const JSThread *thread, uint32_t start, uint32_t end)
{
    if (start >= end) {
        return;
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    for (uint32_t i = start; i < end; i++) {
        elements->Set(thread, i, JSTaggedValue::Hole());
    }
}

inline JSHClass *JSObject::GetJSHClass() const
{
    return GetClass();
}

inline bool JSObject::IsJSGlobalObject() const
{
    return GetJSHClass()->IsJSGlobalObject();
}

inline bool JSObject::IsConstructor() const
{
    return GetJSHClass()->IsConstructor();
}

inline bool JSObject::IsECMAObject() const
{
    return GetJSHClass()->IsECMAObject();
}

inline bool JSObject::IsJSError() const
{
    return GetJSHClass()->IsJSError();
}

inline bool JSObject::IsArguments() const
{
    return GetJSHClass()->IsArguments();
}

inline bool JSObject::IsDate() const
{
    return GetJSHClass()->IsDate();
}

inline bool JSObject::IsJSArray() const
{
    return GetJSHClass()->IsJSArray();
}

inline bool JSObject::IsJSMap() const
{
    return GetJSHClass()->IsJSMap();
}

inline bool JSObject::IsJSSet() const
{
    return GetJSHClass()->IsJSSet();
}

inline bool JSObject::IsJSRegExp() const
{
    return GetJSHClass()->IsJSRegExp();
}

inline bool JSObject::IsJSFunction() const
{
    return GetJSHClass()->IsJSFunction();
}

inline bool JSObject::IsBoundFunction() const
{
    return GetJSHClass()->IsJsBoundFunction();
}

inline bool JSObject::IsJSIntlBoundFunction() const
{
    return GetJSHClass()->IsJSIntlBoundFunction();
}

inline bool JSObject::IsProxyRevocFunction() const
{
    return GetJSHClass()->IsJSProxyRevocFunction();
}

inline bool JSObject::IsAccessorData() const
{
    return GetJSHClass()->IsAccessorData();
}

inline bool JSObject::IsJSGlobalEnv() const
{
    return GetJSHClass()->IsJsGlobalEnv();
}

inline bool JSObject::IsJSProxy() const
{
    return GetJSHClass()->IsJSProxy();
}

inline bool JSObject::IsGeneratorObject() const
{
    return GetJSHClass()->IsGeneratorObject();
}

inline bool JSObject::IsForinIterator() const
{
    return GetJSHClass()->IsForinIterator();
}

inline bool JSObject::IsJSSetIterator() const
{
    return GetJSHClass()->IsJSSetIterator();
}

inline bool JSObject::IsJSMapIterator() const
{
    return GetJSHClass()->IsJSMapIterator();
}

inline bool JSObject::IsJSArrayIterator() const
{
    return GetJSHClass()->IsJSArrayIterator();
}

inline bool JSObject::IsJSAPIArrayListIterator() const
{
    return GetJSHClass()->IsJSAPIArrayListIterator();
}

inline bool JSObject::IsJSPrimitiveRef() const
{
    return GetJSHClass()->IsJsPrimitiveRef();
}

inline bool JSObject::IsElementDict() const
{
    return TaggedArray::Cast(GetElements().GetTaggedObject())->IsDictionaryMode();
}

inline bool JSObject::IsPropertiesDict() const
{
    return TaggedArray::Cast(GetProperties().GetTaggedObject())->IsDictionaryMode();
}

inline bool JSObject::IsTypedArray() const
{
    return GetJSHClass()->IsTypedArray();
}

void JSObject::SetPropertyInlinedProps(const JSThread *thread, uint32_t index, JSTaggedValue value)
{
    SetPropertyInlinedProps(thread, GetJSHClass(), index, value);
}

JSTaggedValue JSObject::GetPropertyInlinedProps(uint32_t index) const
{
    return GetPropertyInlinedProps(GetJSHClass(), index);
}

void JSObject::SetPropertyInlinedProps(const JSThread *thread, const JSHClass *hclass, uint32_t index,
                                       JSTaggedValue value)
{
    uint32_t offset = hclass->GetInlinedPropertiesOffset(index);
    SET_VALUE_WITH_BARRIER(thread, this, offset, value);
}

JSTaggedValue JSObject::GetPropertyInlinedProps(const JSHClass *hclass, uint32_t index) const
{
    uint32_t offset = hclass->GetInlinedPropertiesOffset(index);
    return JSTaggedValue(GET_VALUE(this, offset));
}

JSTaggedValue JSObject::GetProperty(const JSHClass *hclass, PropertyAttributes attr) const
{
    if (attr.IsInlinedProps()) {
        return GetPropertyInlinedProps(hclass, attr.GetOffset());
    }
    TaggedArray *array = TaggedArray::Cast(GetProperties().GetTaggedObject());
    return array->Get(attr.GetOffset() - hclass->GetInlinedProperties());
}

void JSObject::SetProperty(const JSThread *thread, const JSHClass *hclass, PropertyAttributes attr, JSTaggedValue value)
{
    if (attr.IsInlinedProps()) {
        SetPropertyInlinedProps(thread, hclass, attr.GetOffset(), value);
    } else {
        TaggedArray *array = TaggedArray::Cast(GetProperties().GetTaggedObject());
        array->Set(thread, attr.GetOffset() - hclass->GetInlinedProperties(), value);
    }
}

inline bool JSObject::ShouldTransToDict(uint32_t capacity, uint32_t index)
{
    if (index < capacity) {
        return false;
    }
    if (index - capacity > MAX_GAP) {
        return true;
    }

    if (capacity >= MIN_GAP) {
        return index > capacity * FAST_ELEMENTS_FACTOR;
    }

    return false;
}

inline uint32_t JSObject::ComputeElementCapacity(uint32_t oldCapacity)
{
    uint32_t newCapacity = oldCapacity + (oldCapacity >> 1U);
    return newCapacity > MIN_ELEMENTS_LENGTH ? newCapacity : MIN_ELEMENTS_LENGTH;
}

inline uint32_t JSObject::ComputePropertyCapacity(uint32_t oldCapacity)
{
    uint32_t newCapacity = oldCapacity + PROPERTIES_GROW_SIZE;
    return newCapacity > JSHClass::MAX_CAPACITY_OF_OUT_OBJECTS ? JSHClass::MAX_CAPACITY_OF_OUT_OBJECTS
                                                               : newCapacity;
}

// static
template<ElementTypes types>
JSHandle<JSTaggedValue> JSObject::CreateListFromArrayLike(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    // 3. If Type(obj) is not Object, throw a TypeError exception.
    if (!obj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "CreateListFromArrayLike must accept object",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    // 4. Let len be ToLength(Get(obj, "length")).
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();

    JSHandle<JSTaggedValue> value = GetProperty(thread, obj, lengthKeyHandle).GetValue();
    JSTaggedNumber number = JSTaggedValue::ToLength(thread, value);
    // 5. ReturnIfAbrupt(len).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (number.GetNumber() > MAX_ELEMENT_INDEX) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "len is bigger than 2^32 - 1",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }

    uint32_t len = number.ToUint32();
    // 6. Let list be an empty List.
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(len);

    if (obj->IsTypedArray()) {
        JSTypedArray::FastCopyElementToArray(thread, obj, array);
        // c. ReturnIfAbrupt(next).
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        return JSHandle<JSTaggedValue>(array);
    }
    // 8. Repeat while index < len
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue next = JSTaggedValue::GetProperty(thread, obj, i).GetValue().GetTaggedValue();
        // c. ReturnIfAbrupt(next).
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

        if constexpr (types == ElementTypes::STRING_AND_SYMBOL) {
            if (!next.IsString() && !next.IsSymbol()) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "CreateListFromArrayLike: not an element of elementTypes",
                                            JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
            }
        }

        array->Set(thread, i, next);
    }
    return JSHandle<JSTaggedValue>(array);
}
}  //  namespace panda::ecmascript
#endif  // ECMASCRIPT_JSOBJECT_INL_H
