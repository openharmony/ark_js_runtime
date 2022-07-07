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

#include "js_api_lightweightmap.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "libpandabase/utils/bit_utils.h"
#include "object_factory.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript {
JSTaggedValue JSAPILightWeightMap::IncreaseCapacityTo(JSThread *thread,
                                                      const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                                      int32_t index)
{
    int32_t num = lightWeightMap->GetSize();
    if (index < DEFAULT_CAPACITY_LENGTH || num >= index) {
        return JSTaggedValue::False();
    }
    JSHandle<TaggedArray> hashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    JSHandle<TaggedArray> newHashArray = GrowCapacity(thread, hashArray, index);
    JSHandle<TaggedArray> newKeyArray = GrowCapacity(thread, keyArray, index);
    JSHandle<TaggedArray> newValueArray = GrowCapacity(thread, valueArray, index);
    lightWeightMap->SetHashes(thread, newHashArray);
    lightWeightMap->SetKeys(thread, newKeyArray);
    lightWeightMap->SetValues(thread, newValueArray);
    return JSTaggedValue::True();
}

void JSAPILightWeightMap::SetValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                   int32_t index, const JSHandle<JSTaggedValue> &value, AccossorsKind kind)
{
    JSHandle<TaggedArray> array = GetArrayByKind(thread, lightWeightMap, kind);
    int32_t len = lightWeightMap->GetSize();
    JSHandle<TaggedArray> newArray = GrowCapacity(thread, array, len + 1);
    while (len != index && len > 0) {
        JSTaggedValue oldValue = newArray->Get(len - 1);
        newArray->Set(thread, len, oldValue);
        len--;
    }

    newArray->Set(thread, index, value.GetTaggedValue());
    switch (kind) {
        case AccossorsKind::HASH:
            lightWeightMap->SetHashes(thread, newArray);
            break;
        case AccossorsKind::KEY:
            lightWeightMap->SetKeys(thread, newArray);
            break;
        case AccossorsKind::VALUE:
            lightWeightMap->SetValues(thread, newArray);
            break;
        default:
            UNREACHABLE();
    }
}

void JSAPILightWeightMap::RemoveValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                      uint32_t index, AccossorsKind kind)
{
    JSHandle<TaggedArray> array = GetArrayByKind(thread, lightWeightMap, kind);
    uint32_t len = lightWeightMap->GetLength();
    uint32_t num = index;
    ASSERT(num < len);
    while (num < len - 1) {
        array->Set(thread, num, array->Get(num + 1));
        num++;
    }
    switch (kind) {
        case AccossorsKind::HASH:
            lightWeightMap->SetHashes(thread, array);
            break;
        case AccossorsKind::KEY:
            lightWeightMap->SetKeys(thread, array);
            break;
        case AccossorsKind::VALUE:
            lightWeightMap->SetValues(thread, array);
            break;
        default:
            break;
    }
}

void JSAPILightWeightMap::Set(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                              const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    int32_t hash = Hash(key.GetTaggedValue());
    JSHandle<JSTaggedValue> hashHandle(thread, JSTaggedValue(hash));
    int32_t length = lightWeightMap->GetSize();
    int32_t index = GetHashIndex(thread, lightWeightMap, hash, key.GetTaggedValue(), length);
    if (GetIndexOfKey(thread, lightWeightMap, key) < 0) {
        SetValue(thread, lightWeightMap, index, hashHandle, AccossorsKind::HASH);
        SetValue(thread, lightWeightMap, index, key, AccossorsKind::KEY);
        SetValue(thread, lightWeightMap, index, value, AccossorsKind::VALUE);
        lightWeightMap->SetLength(length + 1);
        return;
    }
    SetValue(thread, lightWeightMap, index, hashHandle, AccossorsKind::HASH);
    SetValue(thread, lightWeightMap, index, key, AccossorsKind::KEY);
    SetValue(thread, lightWeightMap, index, value, AccossorsKind::VALUE);
}

JSTaggedValue JSAPILightWeightMap::Get(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                       const JSHandle<JSTaggedValue> &key)
{
    int32_t index = GetIndexOfKey(thread, lightWeightMap, key);
    if (index < 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    return valueArray->Get(index);
}

JSTaggedValue JSAPILightWeightMap::HasAll(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          const JSHandle<JSAPILightWeightMap> &newLightWeightMap)
{
    int32_t length = newLightWeightMap->GetSize();
    int32_t len = lightWeightMap->GetSize();
    if (length > len) {
        return JSTaggedValue::False();
    }
    JSHandle<TaggedArray> oldHashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    JSHandle<TaggedArray> oldKeyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> oldValueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    JSHandle<TaggedArray> newKeyArray = GetArrayByKind(thread, newLightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> newValueArray = GetArrayByKind(thread, newLightWeightMap, AccossorsKind::VALUE);
    JSTaggedValue dealKey = JSTaggedValue::Undefined();
    int32_t index = -1;
    int32_t hash;

    for (int32_t num = 0; num < length; num++) {
        dealKey = newKeyArray->Get(num);
        hash = Hash(dealKey);
        index = BinarySearchHashes(oldHashArray, hash, len);
        if (index < 0 || index >= len) {
            return JSTaggedValue::False();
        }
        HashParams params { oldHashArray, oldKeyArray, &dealKey };
        index = AvoidHashCollision(params, index, len, hash);
        if (!JSTaggedValue::SameValue(oldKeyArray->Get(index), dealKey) ||
            !JSTaggedValue::SameValue(oldValueArray->Get(index), newValueArray->Get(num))) {
            // avoid Hash collision
            return JSTaggedValue::False();
        }
    }
    return JSTaggedValue::True();
}

JSTaggedValue JSAPILightWeightMap::HasKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          const JSHandle<JSTaggedValue> &key)
{
    int32_t index = GetIndexOfKey(thread, lightWeightMap, key);
    return index >= 0 ? JSTaggedValue::True() : JSTaggedValue::False();
}

JSTaggedValue JSAPILightWeightMap::HasValue(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                            const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    int32_t length = lightWeightMap->GetSize();
    for (int32_t num = 0; num < length; num++) {
        if (JSTaggedValue::SameValue(valueArray->Get(num), value.GetTaggedValue())) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

int32_t JSAPILightWeightMap::GetIndexOfKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                           const JSHandle<JSTaggedValue> &key)
{
    int32_t hash = Hash(key.GetTaggedValue());
    int32_t length = lightWeightMap->GetSize();
    JSHandle<TaggedArray> hashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    int32_t index = BinarySearchHashes(hashArray, hash, length);
    if (index >= 0) {
        // avoid Hash Collision
        JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
        int32_t right = index;
        while ((right < length) && (hashArray->Get(right).GetInt() == hash)) {
            if (JSTaggedValue::SameValue(keyArray->Get(right), key.GetTaggedValue())) {
                return right;
            }
            right++;
        }
        int32_t left = index - 1;
        while ((left >= 0) && ((hashArray->Get(left).GetInt() == hash))) {
            if (JSTaggedValue::SameValue(keyArray->Get(left), key.GetTaggedValue())) {
                return left;
            }
            left--;
        }
    }
    return -1;
}

int32_t JSAPILightWeightMap::GetIndexOfValue(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                             const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    int32_t length = lightWeightMap->GetSize();
    JSTaggedValue compValue = value.GetTaggedValue();
    for (int32_t i = 0; i < length; i++) {
        if (valueArray->Get(i) == compValue) {
            return i;
        }
    }
    return -1; // not find, default return -1
}

JSTaggedValue JSAPILightWeightMap::GetKeyAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                            int32_t index)
{
    int32_t length = lightWeightMap->GetSize();
    if (index < 0 || length <= index) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "index is not exits", JSTaggedValue::Exception());
    }
    JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    return keyArray->Get(index);
}

JSTaggedValue JSAPILightWeightMap::GetValueAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                              int32_t index)
{
    int32_t length = lightWeightMap->GetSize();
    if (index < 0 || length <= index) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "index is not exits", JSTaggedValue::Exception());
    }
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    return valueArray->Get(index);
}

void JSAPILightWeightMap::SetAll(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                 const JSHandle<JSAPILightWeightMap> &needLightWeightMap)
{
    JSHandle<TaggedArray> needKeyArray = GetArrayByKind(thread, needLightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> needValueArray = GetArrayByKind(thread, needLightWeightMap, AccossorsKind::VALUE);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    int32_t length = needLightWeightMap->GetSize();
    for (int32_t num = 0; num < length; num++) {
        key.Update(needKeyArray->Get(num));
        value.Update(needValueArray->Get(num));
        JSAPILightWeightMap::Set(thread, lightWeightMap, key, value);
    }
}

JSTaggedValue JSAPILightWeightMap::Remove(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          const JSHandle<JSTaggedValue> &key)
{
    int32_t hash = Hash(key.GetTaggedValue());
    int32_t length = lightWeightMap->GetSize();
    int32_t index = GetHashIndex(thread, lightWeightMap, hash, key.GetTaggedValue(), length);
    if (index < 0 || index >= length) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue value = JSTaggedValue::Undefined();
    JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    for (int32_t i = 0; length > i; i++) {
        if (keyArray->Get(i) == key.GetTaggedValue()) {
            value = valueArray->Get(index);
            RemoveValue(thread, lightWeightMap, index, AccossorsKind::HASH);
            RemoveValue(thread, lightWeightMap, index, AccossorsKind::VALUE);
            RemoveValue(thread, lightWeightMap, index, AccossorsKind::KEY);
            lightWeightMap->SetLength(length - 1);
        }
    }

    return value;
}

JSTaggedValue JSAPILightWeightMap::RemoveAt(JSThread *thread,
                                            const JSHandle<JSAPILightWeightMap> &lightWeightMap, int32_t index)
{
    int32_t length = lightWeightMap->GetSize();
    if (index < 0 || length <= index) {
        return JSTaggedValue::False();
    }
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::HASH);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::VALUE);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::KEY);
    lightWeightMap->SetLength(length - 1);
    return JSTaggedValue::True();
}

JSTaggedValue JSAPILightWeightMap::IsEmpty()
{
    if (GetLength() == 0) {
        return JSTaggedValue::True();
    } else {
        return JSTaggedValue::False();
    }
}

void JSAPILightWeightMap::Clear(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(DEFAULT_CAPACITY_LENGTH));
    JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(DEFAULT_CAPACITY_LENGTH));
    JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(DEFAULT_CAPACITY_LENGTH));
    lightWeightMap->SetHashes(thread, hashArray);
    lightWeightMap->SetKeys(thread, keyArray);
    lightWeightMap->SetValues(thread, valueArray);
    lightWeightMap->SetLength(0);
}

JSTaggedValue JSAPILightWeightMap::SetValueAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                              int32_t index, const JSHandle<JSTaggedValue> &value)
{
    int32_t length = lightWeightMap->GetSize();
    if (index < 0 || length <= index) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "index is not exits", JSTaggedValue::False());
    }
    SetValue(thread, lightWeightMap, index, value, AccossorsKind::VALUE);
    return JSTaggedValue::True();
}

int32_t JSAPILightWeightMap::GetHashIndex(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          int32_t hash, const JSTaggedValue &key, int32_t size)
{
    JSHandle<TaggedArray> hashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    int32_t index = BinarySearchHashes(hashArray, hash, size);
    if (index < 0) {
        index ^= HASH_REBELLION;
        return index;
    }
    if ((index < size) && (JSTaggedValue::SameValue(keyArray->Get(index), key))) {
        return index;
    }
    JSTaggedValue dealKey = key;
    HashParams params { hashArray, keyArray, &dealKey };
    return AvoidHashCollision(params, index, size, hash);
}

int32_t JSAPILightWeightMap::AvoidHashCollision(HashParams &params, int32_t index, int32_t size, int32_t hash)
{
    int32_t right = index;
    while ((right < size) && ((params.hashArray)->Get(right).GetInt() == hash)) {
        if (JSTaggedValue::SameValue((params.keyArray)->Get(right), *(params.key))) {
            return right;
        }
        right++;
    }
    int32_t left = index - 1;
    while ((left >= 0) && ((params.hashArray)->Get(left).GetInt() == hash)) {
        if (JSTaggedValue::SameValue((params.keyArray)->Get(left), *(params.key))) {
            return left;
        }
        left--;
    }

    int32_t res = (-right) ^ HASH_REBELLION;
    return res;
}

JSTaggedValue JSAPILightWeightMap::GetIteratorObj(JSThread *thread, const JSHandle<JSAPILightWeightMap> &obj,
                                                  IterationKind type)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightMapIterator> iter(factory->NewJSAPILightWeightMapIterator(obj, type));

    return iter.GetTaggedValue();
}

JSTaggedValue JSAPILightWeightMap::ToString(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string sepStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(",");
    std::u16string colonStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(":");
    uint32_t length = lightWeightMap->GetLength();
    std::u16string concatStr;
    std::u16string concatStrNew;
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());

    for (uint32_t k = 0; k < length; k++) {
        std::u16string valueStr;
        valueHandle.Update(lightWeightMap->GetValueAt(thread, lightWeightMap, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!valueHandle->IsUndefined() && !valueHandle->IsNull()) {
            JSHandle<EcmaString> valueStringHandle = JSTaggedValue::ToString(thread, valueHandle);
            uint32_t valueLen = valueStringHandle->GetLength();
            if (valueStringHandle->IsUtf16()) {
                valueStr = base::StringHelper::Utf16ToU16String(valueStringHandle->GetDataUtf16(), valueLen);
            } else {
                valueStr = base::StringHelper::Utf8ToU16String(valueStringHandle->GetDataUtf8(), valueLen);
            }
        }

        std::u16string keyStr;
        keyHandle.Update(lightWeightMap->GetKeyAt(thread, lightWeightMap, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!keyHandle->IsUndefined() && !keyHandle->IsNull()) {
            JSHandle<EcmaString> keyStringHandle = JSTaggedValue::ToString(thread, keyHandle);
            uint32_t keyLen = keyStringHandle->GetLength();
            if (keyStringHandle->IsUtf16()) {
                keyStr = base::StringHelper::Utf16ToU16String(keyStringHandle->GetDataUtf16(), keyLen);
            } else {
                keyStr = base::StringHelper::Utf8ToU16String(keyStringHandle->GetDataUtf8(), keyLen);
            }
        }

        std::u16string nextStr = base::StringHelper::Append(keyStr, colonStr);
        nextStr = base::StringHelper::Append(nextStr, valueStr);

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

JSHandle<TaggedArray> JSAPILightWeightMap::GrowCapacity(const JSThread *thread, const JSHandle<TaggedArray> &oldArray,
                                                        uint32_t needCapacity)
{
    uint32_t oldLength = oldArray->GetLength();
    if (needCapacity <= oldLength) {
        return oldArray;
    }
    uint32_t newCapacity = ComputeCapacity(needCapacity);
    JSHandle<TaggedArray> newArray = thread->GetEcmaVM()->GetFactory()->CopyArray(oldArray, oldLength, newCapacity);
    return newArray;
}

JSHandle<TaggedArray> JSAPILightWeightMap::GetArrayByKind(const JSThread *thread,
                                                          const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                                          AccossorsKind kind)
{
    JSHandle<TaggedArray> array;
    switch (kind) {
        case AccossorsKind::HASH:
            array = JSHandle<TaggedArray>(thread, lightWeightMap->GetHashes());
            break;
        case AccossorsKind::KEY:
            array = JSHandle<TaggedArray>(thread, lightWeightMap->GetKeys());
            break;
        case AccossorsKind::VALUE:
            array = JSHandle<TaggedArray>(thread, lightWeightMap->GetValues());
            break;
        default:
            UNREACHABLE();
    }
    return array;
}

int32_t JSAPILightWeightMap::Hash(JSTaggedValue key)
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
        int32_t hash = key.GetInt();
        return hash;
    }
    uint64_t keyValue = key.GetRawData();
    return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
}

int32_t JSAPILightWeightMap::BinarySearchHashes(JSHandle<TaggedArray> &array, int32_t hash, int32_t size)
{
    int32_t low = 0;
    int32_t high = size - 1;
    while (low <= high) {
        uint32_t mid = static_cast<uint32_t>(low + high) >> 1U;
        int32_t midHash = array->Get(mid).GetInt();
        if (midHash < hash) {
            low = static_cast<int32_t>(mid) + 1;
        } else {
            if (midHash == hash) {
                return mid;
            }
            high = static_cast<int32_t>(mid) - 1;
        }
    }
    return -(low + 1);
}
}  // namespace panda::ecmascript
