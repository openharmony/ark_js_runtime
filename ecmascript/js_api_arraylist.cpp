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

#include "js_api_arraylist.h"
#include "js_api_arraylist_iterator.h"
#include "js_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
bool JSAPIArrayList::Add(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                         const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = arrayList->GetLength().GetArrayLength();
    JSHandle<TaggedArray> elements = GrowCapacity(thread, arrayList, length + 1);

    ASSERT(!elements->IsDictionaryMode());
    elements->Set(thread, length, value);
    arrayList->SetLength(thread, JSTaggedValue(++length));
    return true;
}

void JSAPIArrayList::Insert(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                            const JSHandle<JSTaggedValue> &value, const int &index)
{
    int length = static_cast<int>(arrayList->GetLength().GetArrayLength());
    if (index < 0 || index >= length) {
        THROW_RANGE_ERROR(thread, "ArrayList: set out-of-bounds");
    }
    JSHandle<TaggedArray> elements = GrowCapacity(thread, arrayList, length + 1);

    ASSERT(!elements->IsDictionaryMode());
    for (int i = length; i >= index; --i) {
        elements->Set(thread, i, elements->Get(i - 1));
    }
    elements->Set(thread, index, value);
    arrayList->SetLength(thread, JSTaggedValue(++length));
}

void JSAPIArrayList::Clear(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList)
{
    if (!arrayList.IsEmpty()) {
        arrayList->SetLength(thread, JSTaggedValue(0));
    }
}

JSHandle<JSAPIArrayList> JSAPIArrayList::Clone(JSThread *thread, const JSHandle<JSAPIArrayList> &obj)
{
    uint32_t length = obj->GetSize();
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    JSHandle<JSAPIArrayList> newArrayList = thread->GetEcmaVM()->GetFactory()->NewJSAPIArrayList(capacity);
    
    newArrayList->SetLength(thread, JSTaggedValue(length));
    for (uint32_t i = 0; i < length; i ++) {
        newArrayList->Set(thread, i, elements->Get(i));
    }
    
    return newArrayList;
}

uint32_t JSAPIArrayList::GetCapacity(JSThread *thread, const JSHandle<JSAPIArrayList> &obj)
{
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    return capacity;
}

void JSAPIArrayList::IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                        int capacity)
{
    JSHandle<TaggedArray> elementData(thread, arrayList->GetElements());
    ASSERT(!elementData->IsDictionaryMode());
    int length = static_cast<int>(arrayList->GetLength().GetArrayLength());
    if (length < capacity) {
        JSHandle<TaggedArray> newElements =
            thread->GetEcmaVM()->GetFactory()->CopyArray(elementData, length, capacity);

        arrayList->SetElements(thread, newElements);
    }
}

void JSAPIArrayList::TrimToCurrentLength(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList)
{
    uint32_t length = arrayList->GetLength().GetArrayLength();
    JSHandle<TaggedArray> oldElements(thread, arrayList->GetElements());
    ASSERT(!oldElements->IsDictionaryMode());
    JSHandle<TaggedArray> newElements = thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, length, length);
    arrayList->SetElements(thread, newElements);
}

JSTaggedValue JSAPIArrayList::Get(JSThread *thread, const uint32_t index)
{
    if (index >= GetLength().GetArrayLength()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Get property index out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    return elements->Get(index);
}

bool JSAPIArrayList::IsEmpty(const JSHandle<JSAPIArrayList> &arrayList)
{
    return arrayList->GetLength().GetArrayLength() == 0;
}

int JSAPIArrayList::GetIndexOf(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                               const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> elements(thread, arrayList->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t length = arrayList->GetLength().GetArrayLength();
    
    for (uint32_t i = 0; i < length; ++i) {
        JSHandle<JSTaggedValue> element(thread, elements->Get(i));
        if (JSTaggedValue::StrictEqual(thread, value, element)) {
            return i;
        }
    }
    return -1;
}

int JSAPIArrayList::GetLastIndexOf(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                   const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> elements(thread, arrayList->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t length = arrayList->GetLength().GetArrayLength();
    for (int i = length - 1; i >= 0; --i) {
        JSHandle<JSTaggedValue> element(thread, elements->Get(i));
        if (JSTaggedValue::StrictEqual(thread, value, element)) {
            return i;
        }
    }
    return -1;
}

bool JSAPIArrayList::RemoveByIndex(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList, int index)
{
    int length = arrayList->GetLength().GetArrayLength();
    int curLength = static_cast<int>(length);
    if (index < 0 || index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "removeByIndex is out-of-bounds", false);
    }

    JSHandle<TaggedArray> elements(thread, arrayList->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    for (int i = index; i <= length - 2; i++) { // 2 : 2 get index of (lastElementIndex - 1)
        elements->Set(thread, i, elements->Get(i + 1));
    }
    
    arrayList->SetLength(thread, JSTaggedValue(--curLength));
    return true;
}

bool JSAPIArrayList::Remove(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                            const JSHandle<JSTaggedValue> &value)
{
    int index = GetIndexOf(thread, arrayList, value);
    uint32_t length = arrayList->GetSize();
    int curLength = static_cast<int>(length);
    if (index >= 0) {
        if (index >= curLength) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "index-out-of-bounds", false);
        }

        JSHandle<TaggedArray> elements(thread, arrayList->GetElements());
        ASSERT(!elements->IsDictionaryMode());
        for (uint32_t i = index; i < length - 1; i++) {
            elements->Set(thread, i, elements->Get(i + 1));
        }
        length--;
        arrayList->SetLength(thread, JSTaggedValue(length));
        return true;
    }
    return false;
}

JSTaggedValue JSAPIArrayList::RemoveByRange(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                            const JSHandle<JSTaggedValue> &value1,
                                            const JSHandle<JSTaggedValue> &value2)
{
    int startIndex = JSTaggedValue::ToInt32(thread, value1);
    int endIndex = JSTaggedValue::ToInt32(thread, value2);
    int length = static_cast<int>(arrayList->GetLength().GetArrayLength());
    if (endIndex <= startIndex) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "fromIndex cannot be less than or equal to toIndex",
                                     JSTaggedValue::Exception());
    }

    if (startIndex < 0 || startIndex >= length || endIndex < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "ArrayList: set out-of-bounds", JSTaggedValue::Exception());
    }

    int32_t toIndex;
    if (endIndex > length) {
        toIndex = length;
    } else if (endIndex == length) {
        toIndex = length - 1;
    } else {
        toIndex = endIndex;
    }

    JSHandle<TaggedArray> elements(thread, arrayList->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t numMoved = length - toIndex;
    for (uint32_t i = 0; i <= numMoved; i++) {
        elements->Set(thread, startIndex + i, elements->Get(toIndex + i));
    }
    
    int newLength = length - (toIndex - startIndex);
    arrayList->SetLength(thread, JSTaggedValue(newLength));
    return JSTaggedValue::True();
}

JSTaggedValue JSAPIArrayList::ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                                 const JSHandle<JSTaggedValue> &callbackFn,
                                                 const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPIArrayList> arraylist = JSHandle<JSAPIArrayList>::Cast(thisHandle);
    uint32_t length = arraylist->GetSize();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    InternalCallParams *arguments = thread->GetInternalCallParams();

    for (uint32_t k = 0; k < length; k++) {
        JSHandle<JSTaggedValue> kValue = JSHandle<JSTaggedValue>(thread, arraylist->Get(thread, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        key.Update(JSTaggedValue(k));
        arguments->MakeArgv(kValue, key, thisHandle);
        JSTaggedValue funcResult =
            JSFunction::Call(thread, callbackFn, thisArg, 3, arguments->GetArgv()); // 3: three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);

        arraylist->Set(thread, k, funcResult);
    }
    
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIArrayList::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    if (index >= GetLength().GetArrayLength()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Set property index out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

JSHandle<JSAPIArrayList> JSAPIArrayList::SubArrayList(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                                      const JSHandle<JSTaggedValue> &value1,
                                                      const JSHandle<JSTaggedValue> &value2)
{
    int length = static_cast<int>(arrayList->GetLength().GetArrayLength());
    int fromIndex = JSTaggedValue::ToInt32(thread, value1);
    int toIndex = JSTaggedValue::ToInt32(thread, value2);
    if (toIndex <= fromIndex) {
        JSHandle<JSAPIArrayList> newArrayList = thread->GetEcmaVM()->GetFactory()->NewJSAPIArrayList(0);
        THROW_RANGE_ERROR_AND_RETURN(thread, "fromIndex cannot be less than or equal to toIndex", newArrayList);
    }
    if (fromIndex < 0 || fromIndex >= length || toIndex < 0) {
        JSHandle<JSAPIArrayList> newArrayList = thread->GetEcmaVM()->GetFactory()->NewJSAPIArrayList(0);
        THROW_RANGE_ERROR_AND_RETURN(thread, "fromIndex or toIndex is out-of-bounds", newArrayList);
    }

    int endIndex = toIndex >= length - 1 ? length - 1 : toIndex;
    if (fromIndex > endIndex) {
        int tmp = fromIndex;
        fromIndex = endIndex;
        endIndex = tmp;
    }

    int newLength = endIndex - fromIndex;
    JSHandle<JSAPIArrayList> subArrayList =
        thread->GetEcmaVM()->GetFactory()->NewJSAPIArrayList(newLength);
    JSHandle<TaggedArray> elements(thread, arrayList->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    subArrayList->SetLength(thread, JSTaggedValue(newLength));
    
    for (int i = 0; i < newLength; i++) {
        subArrayList->Set(thread, i, elements->Get(fromIndex + i));
    }

    return subArrayList;
}

JSTaggedValue JSAPIArrayList::ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                      const JSHandle<JSTaggedValue> &callbackFn,
                                      const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPIArrayList> arrayList = JSHandle<JSAPIArrayList>::Cast(thisHandle);
    uint32_t length = arrayList->GetSize();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    InternalCallParams *arguments = thread->GetInternalCallParams();

    for (uint32_t k = 0; k < length; k++) {
        JSHandle<JSTaggedValue> kValue = JSHandle<JSTaggedValue>(thread, arrayList->Get(thread, k));

        key.Update(JSTaggedValue(k));
        arguments->MakeArgv(kValue, key, thisHandle);
        JSTaggedValue funcResult =
            JSFunction::Call(thread, callbackFn, thisArg, 3, arguments->GetArgv()); // 3: three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        if (static_cast<int>(length) != arrayList->GetSize()) {
            length = arrayList->GetSize();
        }
    }

    return JSTaggedValue::Undefined();
}

JSHandle<TaggedArray> JSAPIArrayList::GrowCapacity(const JSThread *thread, const JSHandle<JSAPIArrayList> &obj,
                                                   uint32_t capacity)
{
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t oldCapacity = oldElements->GetLength();
    if (capacity < oldCapacity) {
        return oldElements;
    }
    uint32_t newCapacity = ComputeCapacity(capacity);
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldCapacity, newCapacity);

    obj->SetElements(thread, newElements);
    return newElements;
}

bool JSAPIArrayList::Has(JSTaggedValue value) const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t length = GetSize();
    if (length == 0) {
        return false;
    }
    
    for (uint32_t i = 0; i < length; i++) {
        if (JSTaggedValue::SameValue(elements->Get(i), value)) {
            return true;
        }
    }
    return false;
}

JSHandle<TaggedArray> JSAPIArrayList::OwnKeys(JSThread *thread, const JSHandle<JSAPIArrayList> &obj)
{
    uint32_t length = obj->GetLength().GetArrayLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys = factory->NewTaggedArray(length);

    for (uint32_t i = 0; i < length; i++) {
        keys->Set(thread, i, JSTaggedValue(i));
    }

    return keys;
}

bool JSAPIArrayList::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIArrayList> &obj,
                                    const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }

    uint32_t length = obj->GetLength().GetArrayLength();
    if (index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(obj), key, desc);
}

JSTaggedValue JSAPIArrayList::GetIteratorObj(JSThread *thread, const JSHandle<JSAPIArrayList> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIArrayListIterator> iter(factory->NewJSAPIArrayListIterator(obj));

    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript
