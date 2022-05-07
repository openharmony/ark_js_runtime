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

#ifndef ECMASCRIPT_GLOBAL_DICTIONARY_INL_H
#define ECMASCRIPT_GLOBAL_DICTIONARY_INL_H

#include "ecmascript/js_symbol.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda {
namespace ecmascript {
int GlobalDictionary::Hash(const JSTaggedValue &key)
{
    if (key.IsHeapObject()) {
        if (key.IsSymbol()) {
            auto symbolString = JSSymbol::Cast(key.GetTaggedObject());
            return symbolString->GetHashField();
        }
        if (key.IsString()) {
            auto keyString = EcmaString::Cast(key.GetTaggedObject());
            return keyString->GetHashcode();
        }
    }
    // key must be object
    UNREACHABLE();
}

bool GlobalDictionary::IsMatch(const JSTaggedValue &key, const JSTaggedValue &other)
{
    return key == other;
}

PropertyBox *GlobalDictionary::GetBox(int entry) const
{
    int index = GetEntryIndex(entry) + ENTRY_VALUE_INDEX;
    return PropertyBox::Cast(Get(index).GetTaggedObject());
}

JSTaggedValue GlobalDictionary::GetValue(int entry) const
{
    return GetBox(entry)->GetValue();
}

PropertyAttributes GlobalDictionary::GetAttributes(int entry) const
{
    int index = GetEntryIndex(entry) + ENTRY_DETAILS_INDEX;
    return PropertyAttributes(Get(index).GetInt());
}

void GlobalDictionary::SetEntry(const JSThread *thread, int entry, const JSTaggedValue &key, const JSTaggedValue &value,
                                const PropertyAttributes &attributes)
{
    SetKey(thread, entry, key);
    SetAttributes(thread, entry, attributes);
    UpdateValueAndAttributes(thread, entry, value, attributes);
}

void GlobalDictionary::ClearEntry(const JSThread *thread, int entry)
{
    JSTaggedValue hole = JSTaggedValue::Hole();
    PropertyAttributes metaData;
    SetEntry(thread, entry, hole, hole, metaData);
}

void GlobalDictionary::UpdateValueAndAttributes(const JSThread *thread, int entry, const JSTaggedValue &value,
                                                const PropertyAttributes &metaData)
{
    UpdateValue(thread, entry, value);
    SetAttributes(thread, entry, metaData);
}

void GlobalDictionary::SetAttributes(const JSThread *thread, int entry, const PropertyAttributes &metaData)
{
    int index = static_cast<int>(GetEntryIndex(entry) + ENTRY_DETAILS_INDEX);
    Set(thread, index, metaData.GetTaggedValue());
}

void GlobalDictionary::UpdateValue(const JSThread *thread, int entry, const JSTaggedValue &value)
{
    SetValue(thread, entry, value);
}

void GlobalDictionary::GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const
{
    ASSERT_PRINT(offset + EntriesCount() <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");
    int arrayIndex = 0;
    int size = Size();

    CVector<std::pair<JSTaggedValue, uint32_t>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            std::pair<JSTaggedValue, uint32_t> pair(key, attr.GetOffset());
            sortArr.push_back(pair);
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    for (auto entry : sortArr) {
        JSTaggedValue nameKey = entry.first;
        keyArray->Set(thread, arrayIndex + offset, nameKey);
        arrayIndex++;
    }
}

void GlobalDictionary::GetEnumAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray,
                                      uint32_t *keys) const
{
    ASSERT_PRINT(offset + EntriesCount() <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");
    int arrayIndex = 0;
    int size = Size();

    CVector<std::pair<JSTaggedValue, uint32_t>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (key.IsString()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            if (attr.IsEnumerable()) {
                std::pair<JSTaggedValue, uint32_t> pair(key, attr.GetOffset());
                sortArr.push_back(pair);
            }
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    for (const auto &entry : sortArr) {
        JSTaggedValue nameKey = entry.first;
        keyArray->Set(thread, arrayIndex + offset, nameKey);
        arrayIndex++;
    }
    *keys += arrayIndex;
}

bool GlobalDictionary::CompKey(const std::pair<JSTaggedValue, uint32_t> &a, const std::pair<JSTaggedValue, uint32_t> &b)
{
    return a.second < b.second;
}

void GlobalDictionary::InvalidatePropertyBox(JSThread *thread, const JSHandle<GlobalDictionary> &dictHandle, int entry,
                                             const PropertyAttributes &metaData)
{
    PropertyBox *box = dictHandle->GetBox(entry);
    PropertyAttributes originAttr = dictHandle->GetAttributes(entry);
    PropertyAttributes newAttr(metaData);

    ASSERT(!box->GetValue().IsHole());
    int index = static_cast<int>(originAttr.GetDictionaryOrder());
    JSHandle<JSTaggedValue> oldValue(thread, box->GetValue());
    GlobalDictionary::InvalidateAndReplaceEntry(thread, dictHandle, index, oldValue);
    dictHandle->SetAttributes(thread, entry, newAttr);
}

void GlobalDictionary::InvalidateAndReplaceEntry(JSThread *thread, const JSHandle<GlobalDictionary> &dictHandle,
                                                 int entry, const JSHandle<JSTaggedValue> &oldValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Swap with a copy.
    JSHandle<PropertyBox> newBox = factory->NewPropertyBox(oldValue);
    PropertyBox *box = dictHandle->GetBox(entry);
    PropertyAttributes attr = dictHandle->GetAttributes(entry);
    ASSERT_PRINT(attr.IsConfigurable(), "property must be configurable");
    ASSERT_PRINT(!box->GetValue().IsHole(), "value must not be hole");

    // Cell is officially mutable henceforth.
    attr.SetBoxType(PropertyBoxType::MUTABLE);
    dictHandle->SetAttributes(thread, entry, attr);
    dictHandle->UpdateValue(thread, entry, newBox.GetTaggedValue());
    box->Clear(thread);
}
}  // namespace ecmascript
}  // namespace panda

#endif
