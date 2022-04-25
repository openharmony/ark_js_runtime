/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 
#include "js_api_plain_array.h"
#include "js_api_plain_array_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
void JSAPIPlainArray::Add(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj, JSHandle<JSTaggedValue> key,
                          JSHandle<JSTaggedValue> value)
{
    JSHandle<TaggedArray> keyArray(thread, obj->GetKeys());
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues());
    int32_t size = obj->GetLength();
    int32_t index = obj->BinarySearch(*keyArray, 0, size, key.GetTaggedValue().GetNumber());
    if (index >= 0) {
        keyArray->Set(thread, index, key);
        valueArray->Set(thread, index, value);
        return;
    }
    index ^= 0xFFFFFFFF;
    if (index < size) {
        obj->AdjustArray(thread, *keyArray, index, size, true);
        obj->AdjustArray(thread, *valueArray, index, size, true);
    }
    uint32_t capacity = valueArray->GetLength();
    if (size + 1 >= static_cast<int32_t>(capacity)) {
        uint32_t newCapacity = static_cast<uint32_t>(capacity) << 1U;
        keyArray =
            thread->GetEcmaVM()->GetFactory()->CopyArray(keyArray, capacity, newCapacity);
        valueArray =
            thread->GetEcmaVM()->GetFactory()->CopyArray(valueArray, capacity, newCapacity);
        obj->SetKeys(thread, keyArray);
        obj->SetValues(thread, valueArray);
    }
    keyArray->Set(thread, index, key);
    valueArray->Set(thread, index, value);
    size++;
    obj->SetLength(size);
}

JSHandle<TaggedArray> JSAPIPlainArray::CreateSlot(const JSThread *thread, const uint32_t capacity)
{
    ASSERT_PRINT(capacity > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(capacity, JSTaggedValue::Hole());
    return taggedArray;
}

bool JSAPIPlainArray::AdjustForward(JSThread *thread, int32_t index, int32_t forwardSize)
{
    int32_t size = GetLength();
    TaggedArray *keys = TaggedArray::Cast(GetKeys().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    AdjustArray(thread, keys, index + forwardSize, index, false);
    AdjustArray(thread, values, index + forwardSize, index, false);
    size = size - forwardSize;
    SetLength(size);
    return true;
}

void JSAPIPlainArray::AdjustArray(JSThread *thread, TaggedArray *srcArray, int32_t fromIndex,
                                  int32_t toIndex, bool direction)
{
    int32_t size = GetLength();
    int32_t idx = size - 1;
    if (direction) {
        while (fromIndex < toIndex) {
            JSTaggedValue value = srcArray->Get(idx);
            srcArray->Set(thread, idx + 1, value);
            idx--;
            fromIndex++;
        }
    } else {
        int32_t moveSize = size - fromIndex;
        for (int32_t i = 0; i < moveSize; i++) {
            if ((fromIndex + i) < size) {
                JSTaggedValue value = srcArray->Get(fromIndex + i);
                srcArray->Set(thread, toIndex + i, value);
            } else {
                srcArray->Set(thread, toIndex + i, JSTaggedValue::Hole());
            }
        }
    }
}

int32_t JSAPIPlainArray::BinarySearch(TaggedArray *array, int32_t fromIndex, int32_t toIndex, int32_t key)
{
    int32_t low = fromIndex;
    int32_t high = toIndex - 1;
    while (low <= high) {
        int32_t mid = static_cast<uint32_t>(low + high) >> 1U;
        int32_t midVal = static_cast<int32_t>(array->Get(mid).GetNumber());
        if (midVal < key) {
            low = mid + 1;
        } else {
            if (midVal <= key) {
                return mid;
            }
            high = mid - 1;
        }
    }
    return -(low + 1);
}

void JSAPIPlainArray::Clear(JSThread *thread)
{
    TaggedArray *keys = TaggedArray::Cast(GetKeys().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    int32_t size = GetLength();
    for (int32_t index = 0; index < size; index++) {
        keys->Set(thread, index, JSTaggedValue::Hole());
        values->Set(thread, index, JSTaggedValue::Hole());
    }
    SetLength(0);
}

JSTaggedValue JSAPIPlainArray::RemoveRangeFrom(JSThread *thread, int32_t index, int32_t batchSize)
{
    int32_t size = GetLength();
    if (index < 0 || index >= size) {
        return JSTaggedValue(-1);
    }
    if (batchSize < 1) {
        return JSTaggedValue(-1);
    }
    int32_t safeSize = (size - (index + batchSize)) < 0 ? size - index : batchSize;
    AdjustForward(thread, index, safeSize);
    return JSTaggedValue(safeSize);
}

JSTaggedValue JSAPIPlainArray::Set(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                                   const uint32_t index, JSTaggedValue value)
{
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSAPIPlainArray::Add(thread, obj, key, valueHandle);
    return JSTaggedValue::Undefined();
}

bool JSAPIPlainArray::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                                     const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    TaggedArray *keyArray = TaggedArray::Cast(obj->GetKeys().GetTaggedObject());
    int32_t size = obj->GetLength();
    int32_t index = obj->BinarySearch(keyArray, 0, size, key.GetTaggedValue().GetNumber());
    if (index < 0 || index > size) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(obj), key, desc);
}

OperationResult JSAPIPlainArray::GetProperty(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                                             const JSHandle<JSTaggedValue> &key)
{
    TaggedArray *keyArray = TaggedArray::Cast(obj->GetKeys().GetTaggedObject());
    int32_t size = obj->GetLength();
    int32_t index = obj->BinarySearch(keyArray, 0, size, key.GetTaggedValue().GetNumber());
    if (index < 0 || index > size) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetProperty index out-of-bounds",
                                     OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    
    return OperationResult(thread, obj->Get(JSTaggedValue(index)), PropertyMetaData(false));
}

JSHandle<JSAPIPlainArray> JSAPIPlainArray::Clone(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj)
{
    JSHandle<TaggedArray> keys(thread, obj->GetKeys());
    JSHandle<TaggedArray> values(thread, obj->GetValues());
    uint32_t capacity = keys->GetLength();
    int32_t size = obj->GetLength();
    JSHandle<JSAPIPlainArray> newPlainArray = thread->GetEcmaVM()->GetFactory()->NewJSAPIPlainArray(capacity);
    newPlainArray->SetLength(size);
    TaggedArray *newKeys = TaggedArray::Cast(newPlainArray->GetKeys().GetTaggedObject());
    TaggedArray *newValues = TaggedArray::Cast(newPlainArray->GetValues().GetTaggedObject());
    for (int32_t i = 0; i < size; i++) {
        newKeys->Set(thread, i, keys->Get(i));
        newValues->Set(thread, i, values->Get(i));
    }
    return newPlainArray;
}

bool JSAPIPlainArray::Has(const int32_t key)
{
    int32_t size = GetLength();
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys().GetTaggedObject());
    int32_t index = BinarySearch(keyArray, 0, size, key);
    if (index < 0) {
        return false;
    }
    return true;
}

JSTaggedValue JSAPIPlainArray::Get(const JSTaggedValue key)
{
    int32_t size = GetLength();
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys().GetTaggedObject());
    int32_t index = BinarySearch(keyArray, 0, size, key.GetNumber());
    if (index < 0) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    return values->Get(index);
}

JSHandle<JSTaggedValue> JSAPIPlainArray::GetIteratorObj(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                                                        IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> iter =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSAPIPlainArrayIterator(obj, kind));
    return iter;
}

JSTaggedValue JSAPIPlainArray::ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                       const JSHandle<JSTaggedValue> &callbackFn,
                                       const JSHandle<JSTaggedValue> &thisArg)
{
    JSAPIPlainArray *plainarray = JSAPIPlainArray::Cast(thisHandle->GetTaggedObject());
    int32_t length = plainarray->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<TaggedArray> keyArray(thread, plainarray->GetKeys());
    JSHandle<TaggedArray> valueArray(thread, plainarray->GetValues());
    for (int32_t k = 0; k < length; k++) {
        JSTaggedValue kValue = valueArray->Get(k);
        JSTaggedValue key = keyArray->Get(k);
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFn, thisArg, undefined, 3);  // 3: three args
        info.SetCallArg(kValue, key, thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIPlainArray::ToString(JSThread *thread, const JSHandle<JSAPIPlainArray> &plainarray)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    std::u16string sepStr = std::wstring_convert < std::codecvt_utf8_utf16<char16_t>, char16_t > {}.from_bytes(",");
    int32_t length = plainarray->GetLength();
    JSHandle<TaggedArray> keyArray(thread, plainarray->GetKeys());
    JSHandle<TaggedArray> elements(thread, plainarray->GetValues());
    std::u16string concatStr;
    std::u16string concatStrNew;

    JSHandle<EcmaString> stringSeparate = factory->NewFromASCII(":");
    JSMutableHandle<JSTaggedValue> keys(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    JSMutableHandle<EcmaString> ret(thread, nullptr);
    JSMutableHandle<EcmaString> stringKey(thread, nullptr);
    JSMutableHandle<EcmaString> stringValue(thread, nullptr);
    JSMutableHandle<EcmaString> concatValue(thread, nullptr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    for (int32_t k = 0; k < length; k++) {
        std::u16string nextStr;
        keys.Update(JSTaggedValue(keyArray->Get(k)));
        element.Update(JSTaggedValue(elements->Get(k)));
        if (!element->IsUndefined() && !element->IsNull()) {
            stringKey.Update(JSTaggedValue::ToString(thread, keys).GetTaggedValue());
            ret.Update(JSTaggedValue(EcmaString::Concat(stringKey, stringSeparate, ecmaVM)));

            stringValue.Update(JSTaggedValue::ToString(thread, element).GetTaggedValue());
            concatValue.Update(JSTaggedValue(EcmaString::Concat(ret, stringValue, ecmaVM)));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            uint32_t nextLen = concatValue->GetLength();
            if (concatValue->IsUtf16()) {
                nextStr = base::StringHelper::Utf16ToU16String(concatValue->GetDataUtf16(), nextLen);
            } else {
                nextStr = base::StringHelper::Utf8ToU16String(concatValue->GetDataUtf8(), nextLen);
            }
        }
        if (k > 0) {
            concatStrNew = base::StringHelper::Append(concatStr, sepStr);
            concatStr = base::StringHelper::Append(concatStrNew, nextStr);
            continue;
        }
        concatStr = base::StringHelper::Append(concatStr, nextStr);
    }
    char16_t *char16tData = concatStr.data();
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = concatStr.size();
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
}

JSTaggedValue JSAPIPlainArray::GetIndexOfKey(int32_t key)
{
    int32_t size = GetLength();
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys().GetTaggedObject());
    int32_t index = BinarySearch(keyArray, 0, size, key);
    if (index < 0) {
        return JSTaggedValue(-1);
    }
    return JSTaggedValue(index);
}

JSTaggedValue JSAPIPlainArray::GetIndexOfValue(JSTaggedValue value)
{
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    int32_t size = GetLength();
    int32_t index = -1;
    for (int32_t i = 0; i < size; i++) {
        if (JSTaggedValue::SameValue(values->Get(i), value)) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        return JSTaggedValue(-1);
    }
    return JSTaggedValue(index);
}

bool JSAPIPlainArray::IsEmpty()
{
    int32_t length = GetLength();
    return length == 0;
}

JSTaggedValue JSAPIPlainArray::GetKeyAt(int32_t index)
{
    int32_t size = GetLength();
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys().GetTaggedObject());
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    return keyArray->Get(index);
}

JSTaggedValue JSAPIPlainArray::GetValueAt(int32_t index)
{
    int32_t size = GetLength();
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    return values->Get(index);
}

JSTaggedValue JSAPIPlainArray::Remove(JSThread *thread, JSTaggedValue key)
{
    int32_t size = GetLength();
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys().GetTaggedObject());
    int32_t index = BinarySearch(keyArray, 0, size, key.GetNumber());
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    JSTaggedValue value = values->Get(index);
    AdjustForward(thread, index, 1); // 1 means the length of array
    return value;
}

JSTaggedValue JSAPIPlainArray::RemoveAt(JSThread *thread, JSTaggedValue index)
{
    int32_t size = GetLength();
    int32_t seat = index.GetNumber();
    if (seat < 0 || seat >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    JSTaggedValue value = values->Get(seat);
    AdjustForward(thread, seat, 1);
    return value;
}

bool JSAPIPlainArray::SetValueAt(JSThread *thread, JSTaggedValue index, JSTaggedValue value)
{
    int32_t size = GetLength();
    int32_t seat = index.GetNumber();
    if (seat < 0 || seat >= size) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", false);
    }
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    values->Set(thread, seat, value);
    return true;
}
} // namespace panda::ecmascript
