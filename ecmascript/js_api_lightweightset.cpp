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
 
#include "js_api_lightweightset.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "js_api_lightweightset_iterator.h"

namespace panda::ecmascript {
bool JSAPILightWeightSet::Add(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                              const JSHandle<JSTaggedValue> &value)
{
    uint32_t hashCode = obj->Hash(value.GetTaggedValue());
    JSHandle<TaggedArray> hashArray(thread, obj->GetHashes());
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues());
    int32_t size = static_cast<int32_t>(obj->GetLength());
    int32_t index = obj->GetHashIndex(value, size);
    if (index < 0) {
        index ^= JSAPILightWeightSet::HASH_REBELLION;
    }
    if (index < size) {
        obj->AdjustArray(thread, hashArray, index, size, true);
        obj->AdjustArray(thread, valueArray, index, size, true);
    }
    uint32_t capacity = hashArray->GetLength();
    if (size + 1 >= static_cast<int32_t>(capacity)) {
        // need expanding
        uint32_t newCapacity = capacity << 1U;
        hashArray = thread->GetEcmaVM()->GetFactory()->CopyArray(hashArray, capacity, newCapacity);
        valueArray = thread->GetEcmaVM()->GetFactory()->CopyArray(valueArray, capacity, newCapacity);
        obj->SetHashes(thread, hashArray);
        obj->SetValues(thread, valueArray);
    }
    hashArray->Set(thread, index, JSTaggedValue(hashCode));
    valueArray->Set(thread, index, value.GetTaggedValue());
    size++;
    obj->SetLength(size);
    return true;
}

JSHandle<TaggedArray> JSAPILightWeightSet::GrowCapacity(const JSThread *thread,
                                                        const JSHandle<JSAPILightWeightSet> &obj, uint32_t capacity)
{
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    uint32_t oldLength = oldElements->GetLength();
    if (capacity < oldLength) {
        return oldElements;
    }
    uint32_t newCapacity = ComputeCapacity(capacity);
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldLength, newCapacity);
    obj->SetElements(thread, newElements);
    return newElements;
}

JSTaggedValue JSAPILightWeightSet::Get(const uint32_t index)
{
    TaggedArray *valueArray = TaggedArray::Cast(GetValues().GetTaggedObject());
    return valueArray->Get(index);
}

JSHandle<TaggedArray> JSAPILightWeightSet::CreateSlot(const JSThread *thread, const uint32_t capacity)
{
    ASSERT_PRINT(capacity > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(capacity);
    for (uint32_t i = 0; i < capacity; i++) {
        taggedArray->Set(thread, i, JSTaggedValue::Hole());
    }
    return taggedArray;
}

int32_t JSAPILightWeightSet::GetHashIndex(const JSHandle<JSTaggedValue> &value, int32_t size)
{
    uint32_t hashCode = Hash(value.GetTaggedValue());
    int32_t index = BinarySearchHashes(hashCode, size);
    if (index < 0) {
        return index;
    }
    TaggedArray *valueArray = TaggedArray::Cast(GetValues().GetTaggedObject());
    if (index < size && (JSTaggedValue::SameValue(valueArray->Get(index), value.GetTaggedValue()))) {
        return index;
    }
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes().GetTaggedObject());
    int32_t right = index;
    while (right < size && (hashArray->Get(right).GetNumber() == hashCode)) {
        if (JSTaggedValue::SameValue(valueArray->Get(right), value.GetTaggedValue())) {
            return right;
        }
        right++;
    }
    int32_t left = index - 1;
    while (left >= 0 && ((hashArray->Get(left).GetNumber() == hashCode))) {
        if (JSTaggedValue::SameValue(valueArray->Get(left), value.GetTaggedValue())) {
            return left;
        }
        left--;
    }
    return -right;
}

int32_t JSAPILightWeightSet::BinarySearchHashes(uint32_t hash, int32_t size)
{
    int32_t low = 0;
    int32_t high = size - 1;
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes().GetTaggedObject());
    while (low <= high) {
        uint32_t mid = static_cast<uint32_t>(low + high) >> 1U;
        uint32_t midVal = (uint32_t)(hashArray->Get(mid).GetNumber());
        if (midVal < hash) {
            low = mid + 1;
        } else {
            if (midVal <= hash) {
                return mid;
            }
            high = mid - 1;
        }
    }
    return -(low + 1);
}

bool JSAPILightWeightSet::AddAll(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                 const JSHandle<JSTaggedValue> &value)
{
    bool changed = false;
    JSHandle<JSAPILightWeightSet> srcLightWeightSet = JSHandle<JSAPILightWeightSet>::Cast(value);
    uint32_t srcSize = srcLightWeightSet->GetSize();
    uint32_t size = obj->GetSize();
    obj->EnsureCapacity(thread, obj, size + srcSize);
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < srcSize; i++) {
        element.Update(srcLightWeightSet->GetValueAt(i));
        changed = JSAPILightWeightSet::Add(thread, obj, element);
    }
    return changed;
}

void JSAPILightWeightSet::EnsureCapacity(const JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                         uint32_t minimumCapacity)
{
    TaggedArray *hashes = TaggedArray::Cast(obj->GetValues().GetTaggedObject());
    uint32_t capacity = hashes->GetLength();
    uint32_t newCapacity = capacity;
    if (capacity > minimumCapacity) {
        return;
    }
    // adjust
    while (newCapacity <= minimumCapacity) {
        newCapacity = newCapacity << 1U;
    }
    obj->SizeCopy(thread, obj, capacity, newCapacity);
}

void JSAPILightWeightSet::SizeCopy(const JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                   uint32_t capacity, uint32_t newCapacity)
{
    JSHandle<TaggedArray> hashArray(thread, obj->GetHashes());
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues());
    hashArray = thread->GetEcmaVM()->GetFactory()->CopyArray(hashArray, capacity, newCapacity);
    valueArray = thread->GetEcmaVM()->GetFactory()->CopyArray(valueArray, capacity, newCapacity);
    
    obj->SetValues(thread, hashArray);
    obj->SetHashes(thread, valueArray);
}

bool JSAPILightWeightSet::IsEmpty()
{
    return GetLength() == 0;
}

JSTaggedValue JSAPILightWeightSet::GetValueAt(int32_t index)
{
    int32_t size = static_cast<int32_t>(GetLength());
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    return values->Get(index);
}

JSTaggedValue JSAPILightWeightSet::GetHashAt(int32_t index)
{
    int32_t size = static_cast<int32_t>(GetLength());
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetHashes().GetTaggedObject());
    return values->Get(index);
}

bool JSAPILightWeightSet::HasAll(const JSHandle<JSTaggedValue> &value)
{
    bool result = false;
    uint32_t relocate = 0;
    JSAPILightWeightSet *lightweightSet = JSAPILightWeightSet::Cast(value.GetTaggedValue().GetTaggedObject());
    uint32_t size = GetLength();
    uint32_t destSize = lightweightSet->GetLength();
    TaggedArray *hashes = TaggedArray::Cast(GetHashes().GetTaggedObject());
    TaggedArray *destHashes = TaggedArray::Cast(lightweightSet->GetHashes().GetTaggedObject());
    if (destSize > size) {
        return result;
    }
    for (uint32_t i = 0; i < destSize; i++) {
        uint32_t destHashCode = destHashes->Get(i).GetNumber();
        result = false;
        for (uint32_t j = relocate; j < size; j++) {
            uint32_t hashCode = hashes->Get(j).GetNumber();
            if (destHashCode == hashCode) {
                result = true;
                relocate = j + 1;
                break;
            }
        }
        if (!result) {
            break;
        }
    }
    return result;
}

bool JSAPILightWeightSet::Has(const JSHandle<JSTaggedValue> &value)
{
    uint32_t size = GetLength();
    int32_t index = GetHashIndex(value, size);
    if (index < 0) {
        return false;
    }
    return true;
}

bool JSAPILightWeightSet::HasHash(const JSHandle<JSTaggedValue> &hashCode)
{
    uint32_t size = GetLength();
    int32_t index = BinarySearchHashes(hashCode.GetTaggedValue().GetNumber(), size);
    if (index < 0) {
        return false;
    }
    return true;
}

bool JSAPILightWeightSet::Equal(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                const JSHandle<JSTaggedValue> &value)
{
    bool result = false;
    JSHandle<TaggedArray> destHashes(thread, obj->GetHashes());
    uint32_t destSize = obj->GetLength();
    uint32_t srcSize;
    JSMutableHandle<TaggedArray> srcHashes(thread, obj->GetHashes());
    if (value.GetTaggedValue().IsJSAPILightWeightSet()) {
        JSAPILightWeightSet *srcLightWeightSet = JSAPILightWeightSet::Cast(value.GetTaggedValue().GetTaggedObject());
        srcSize = srcLightWeightSet->GetLength();
        if (srcSize == 0 || destSize == 0) {
            return false;
        }
        srcHashes.Update(srcLightWeightSet->GetHashes());
    }
    if (value.GetTaggedValue().IsJSArray()) {
        srcHashes.Update(JSArray::ToTaggedArray(thread, value));
        srcSize = srcHashes->GetLength();
        if (srcSize == 0 || destSize == 0) {
            return false;
        }
    }
    if (srcSize != destSize) {
        return false;
    }
    for (uint32_t i = 0; i < srcSize; i++) {
        uint32_t destHashCode = destHashes->Get(i).GetNumber();
        uint32_t srcHashCode = srcHashes->Get(i).GetNumber();
        if (srcHashCode != destHashCode) {
            result = false;
            break;
        }
        result = true;
    }
    return result;
}

void JSAPILightWeightSet::IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                             int32_t minCapacity)
{
    uint32_t capacity = TaggedArray::Cast(obj->GetValues().GetTaggedObject())->GetLength();
    int32_t intCapacity = static_cast<int32_t>(capacity);
    if (minCapacity <= 0 || intCapacity >= minCapacity) {
        THROW_TYPE_ERROR(thread, "the index is not integer");
    }
    obj->SizeCopy(thread, obj, intCapacity, minCapacity);
}

JSHandle<JSTaggedValue> JSAPILightWeightSet::GetIteratorObj(JSThread *thread, const
                                                            JSHandle<JSAPILightWeightSet> &obj, IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> iter =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSAPILightWeightSetIterator(obj, kind));
    return iter;
}

JSTaggedValue JSAPILightWeightSet::ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                           const JSHandle<JSTaggedValue> &callbackFn,
                                           const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPILightWeightSet> lightweightset = JSHandle<JSAPILightWeightSet>::Cast(thisHandle);
    uint32_t length = lightweightset->GetSize();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    for (uint32_t k = 0; k < length; k++) {
        JSTaggedValue kValue = lightweightset->GetValueAt(k);
        JSTaggedValue kHash = lightweightset->GetHashAt(k);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFn, thisArg, undefined, 3); // 3:three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        info->SetCallArg(kValue, kHash, thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
    }
    return JSTaggedValue::Undefined();
}

int32_t JSAPILightWeightSet::GetIndexOf(JSHandle<JSTaggedValue> &value)
{
    uint32_t size = GetLength();
    int32_t index = GetHashIndex(value, size);
    return index;
}

JSTaggedValue JSAPILightWeightSet::Remove(JSThread *thread, JSHandle<JSTaggedValue> &value)
{
    uint32_t size = GetLength();
    TaggedArray *valueArray = TaggedArray::Cast(GetValues().GetTaggedObject());
    int32_t index = GetHashIndex(value, size);
    if (index < 0) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue result = valueArray->Get(index);
    bool success = RemoveAt(thread, index);
    if (!success) {
        result = JSTaggedValue::Undefined();
    }
    return result;
}

bool JSAPILightWeightSet::RemoveAt(JSThread *thread, int32_t index)
{
    int32_t size = static_cast<int32_t>(GetLength());
    if (index < 0 || index >= size) {
        return false;
    }
    JSHandle<TaggedArray> valueArray(thread, GetValues());
    JSHandle<TaggedArray> hashArray(thread, GetHashes());
    AdjustArray(thread, hashArray, index + 1, index, false);
    AdjustArray(thread, valueArray, index + 1, index, false);
    size--;
    SetLength(size);
    return true;
}

void JSAPILightWeightSet::AdjustArray(JSThread *thread, JSHandle<TaggedArray> srcArray, uint32_t fromIndex,
                                      uint32_t toIndex, bool direction)
{
    uint32_t size = GetLength();
    uint32_t idx = size - 1;
    if (direction) {
        while (fromIndex < toIndex) {
            JSTaggedValue value = srcArray->Get(idx);
            srcArray->Set(thread, idx + 1, value);
            idx--;
            fromIndex++;
        }
    } else {
        uint32_t moveSize = size - fromIndex;
        for (uint32_t i = 0; i < moveSize; i++) {
            if ((fromIndex + i) < size) {
                JSTaggedValue value = srcArray->Get(fromIndex + i);
                srcArray->Set(thread, toIndex + i, value);
            } else {
                srcArray->Set(thread, toIndex + i, JSTaggedValue::Hole());
            }
        }
    }
}

JSTaggedValue JSAPILightWeightSet::ToString(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string sepStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(",");
    
    uint32_t length = obj->GetSize();
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues());
    std::u16string concatStr;
    std::u16string concatStrNew;
    JSMutableHandle<JSTaggedValue> values(thread, JSTaggedValue::Undefined());
    for (uint32_t k = 0; k < length; k++) {
        std::u16string nextStr;
        values.Update(valueArray->Get(k));
        if (!values->IsUndefined() && !values->IsNull()) {
            JSHandle<EcmaString> nextStringHandle = JSTaggedValue::ToString(thread, values);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            uint32_t nextLen = nextStringHandle->GetLength();
            if (nextStringHandle->IsUtf16()) {
                nextStr = base::StringHelper::Utf16ToU16String(nextStringHandle->GetDataUtf16(), nextLen);
            } else {
                nextStr = base::StringHelper::Utf8ToU16String(nextStringHandle->GetDataUtf8(), nextLen);
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
    int32_t u16strSize = concatStr.size();
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
}

void JSAPILightWeightSet::Clear(JSThread *thread)
{
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes().GetTaggedObject());
    TaggedArray *valueArray = TaggedArray::Cast(GetValues().GetTaggedObject());
    uint32_t size = GetLength();
    for (uint32_t index = 0; index < size; index++) {
        hashArray->Set(thread, index, JSTaggedValue::Hole());
        valueArray->Set(thread, index, JSTaggedValue::Hole());
    }
    SetLength(0);
}

uint32_t JSAPILightWeightSet::Hash(JSTaggedValue key)
{
    if (key.IsDouble() && key.GetDouble() == 0.0) {
        key = JSTaggedValue(0);
    }
    if (key.IsSymbol()) {
        auto symbolString = JSSymbol::Cast(key.GetTaggedObject());
        return symbolString->GetHashField();
    }
    if (key.IsString()) {
        auto keyString = EcmaString::Cast(key.GetTaggedObject());
        return keyString->GetHashcode();
    }
    if (key.IsECMAObject()) {
        uint32_t hash = ECMAObject::Cast(key.GetTaggedObject())->GetHash();
        if (hash == 0) {
            uint64_t keyValue = key.GetRawData();
            hash = GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
            ECMAObject::Cast(key.GetTaggedObject())->SetHash(hash);
        }
        return hash;
    }
    if (key.IsInt()) {
        uint32_t hash = key.GetInt();
        return hash;
    }
    uint64_t keyValue = key.GetRawData();
    return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
}
} // namespace panda::ecmascript