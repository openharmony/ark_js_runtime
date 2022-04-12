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

#include "ecmascript/js_object-inl.h"

#include "libpandabase/utils/bit_utils.h"
#include "linked_hash_table-inl.h"
#include "object_factory.h"

namespace panda::ecmascript {
template<typename Derived, typename HashObject>
JSHandle<Derived> LinkedHashTable<Derived, HashObject>::Create(const JSThread *thread, int numberOfElements)
{
    ASSERT_PRINT(numberOfElements > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto capacity = static_cast<uint32_t>(numberOfElements);
    ASSERT_PRINT(helpers::math::IsPowerOfTwo(capacity), "capacity must be pow of '2'");
    int length = ELEMENTS_START_INDEX + numberOfElements + numberOfElements * (HashObject::ENTRY_SIZE + 1);

    auto table = JSHandle<Derived>(factory->NewTaggedArray(length));
    table->SetNumberOfElements(thread, 0);
    table->SetNumberOfDeletedElements(thread, 0);
    table->SetCapacity(thread, capacity);
    return table;
}

template<typename Derived, typename HashObject>
JSHandle<Derived> LinkedHashTable<Derived, HashObject>::Insert(const JSThread *thread, const JSHandle<Derived> &table,
                                                               const JSHandle<JSTaggedValue> &key,
                                                               const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsKey(key.GetTaggedValue()));
    int hash = LinkedHash::Hash(key.GetTaggedValue());
    int entry = table->FindElement(key.GetTaggedValue());
    if (entry != -1) {
        table->SetValue(thread, entry, value.GetTaggedValue());
        return table;
    }

    JSHandle<Derived> newTable = GrowCapacity(thread, table);

    uint32_t bucket = newTable->HashToBucket(hash);
    entry = newTable->NumberOfElements() + newTable->NumberOfDeletedElements();
    newTable->InsertNewEntry(thread, bucket, entry);
    newTable->SetKey(thread, entry, key.GetTaggedValue());
    newTable->SetValue(thread, entry, value.GetTaggedValue());
    newTable->SetNumberOfElements(thread, newTable->NumberOfElements() + 1);

    return newTable;
}

template<typename Derived, typename HashObject>
JSHandle<Derived> LinkedHashTable<Derived, HashObject>::InsertWeakRef(const JSThread *thread,
    const JSHandle<Derived> &table, const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsKey(key.GetTaggedValue()));
    int hash = LinkedHash::Hash(key.GetTaggedValue());
    int entry = table->FindElement(key.GetTaggedValue());
    if (entry != -1) {
        table->SetValue(thread, entry, value.GetTaggedValue());
        return table;
    }

    JSHandle<Derived> newTable = GrowCapacity(thread, table);

    uint32_t bucket = newTable->HashToBucket(hash);
    entry = newTable->NumberOfElements() + newTable->NumberOfDeletedElements();
    newTable->InsertNewEntry(thread, bucket, entry);
    JSTaggedValue weakKey(key->CreateAndGetWeakRef());
    newTable->SetKey(thread, entry, weakKey);
    // The ENTRY_VALUE_INDEX of LinkedHashSet is 0. SetValue will cause the overwitten key.
    if (std::is_same_v<LinkedHashMap, Derived>) {
        newTable->SetValue(thread, entry, value.GetTaggedValue());
    }
    newTable->SetNumberOfElements(thread, newTable->NumberOfElements() + 1);

    return newTable;
}

template<typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::Rehash(const JSThread *thread, Derived *newTable)
{
    ASSERT_PRINT(newTable != nullptr && newTable->Capacity() > NumberOfElements(), "can not rehash to new table");
    // Rehash elements to new table
    int numberOfAllElements = NumberOfElements() + NumberOfDeletedElements();
    int desEntry = 0;
    int currentDeletedElements = 0;
    SetNextTable(thread, JSTaggedValue(newTable));
    for (int i = 0; i < numberOfAllElements; i++) {
        int fromIndex = static_cast<int>(EntryToIndex(i));
        JSTaggedValue key = GetElement(fromIndex);
        if (key.IsHole()) {
            // store num_of_deleted_element before entry i; it will be used when iterator update.
            currentDeletedElements++;
            SetDeletedNum(thread, i, JSTaggedValue(currentDeletedElements));
            continue;
        }

        if (key.IsWeak()) {
            // If the key is a weak reference, we use the weak referent to calculate the new index in the new table.
            key.RemoveWeakTag();
        }

        int bucket = static_cast<int>(newTable->HashToBucket(LinkedHash::Hash(key)));
        newTable->InsertNewEntry(thread, bucket, desEntry);
        int desIndex = static_cast<int>(newTable->EntryToIndex(desEntry));
        for (int j = 0; j < HashObject::ENTRY_SIZE; j++) {
            newTable->SetElement(thread, desIndex + j, GetElement(fromIndex + j));
        }
        desEntry++;
    }
    newTable->SetNumberOfElements(thread, NumberOfElements());
    newTable->SetNumberOfDeletedElements(thread, 0);
}

template<typename Derived, typename HashObject>
JSHandle<Derived> LinkedHashTable<Derived, HashObject>::GrowCapacity(const JSThread *thread,
    const JSHandle<Derived> &table, int numberOfAddedElements)
{
    if (table->HasSufficientCapacity(numberOfAddedElements)) {
        return table;
    }
    int newCapacity = ComputeCapacity(table->NumberOfElements() + numberOfAddedElements);
    JSHandle<Derived> newTable = Create(thread, newCapacity);
    table->Rehash(thread, *newTable);
    return newTable;
}

template<typename Derived, typename HashObject>
JSHandle<Derived> LinkedHashTable<Derived, HashObject>::Remove(const JSThread *thread, const JSHandle<Derived> &table,
                                                               const JSHandle<JSTaggedValue> &key)
{
    int entry = table->FindElement(key.GetTaggedValue());
    if (entry == -1) {
        return table;
    }

    table->RemoveEntry(thread, entry);
    return Shrink(thread, table);
}

template<typename Derived, typename HashObject>
JSHandle<Derived> LinkedHashTable<Derived, HashObject>::Shrink(const JSThread *thread, const JSHandle<Derived> &table,
    int additionalCapacity)
{
    int newCapacity = ComputeCapacityWithShrink(table->Capacity(), table->NumberOfElements() + additionalCapacity);
    if (newCapacity == table->Capacity()) {
        return table;
    }

    JSHandle<Derived> newTable = Create(thread, newCapacity);

    table->Rehash(thread, *newTable);
    return newTable;
}

// LinkedHashMap
JSHandle<LinkedHashMap> LinkedHashMap::Create(const JSThread *thread, int numberOfElements)
{
    return LinkedHashTable<LinkedHashMap, LinkedHashMapObject>::Create(thread, numberOfElements);
}

JSHandle<LinkedHashMap> LinkedHashMap::Delete(const JSThread *thread, const JSHandle<LinkedHashMap> &obj,
                                              const JSHandle<JSTaggedValue> &key)
{
    return LinkedHashTable<LinkedHashMap, LinkedHashMapObject>::Remove(thread, obj, key);
}

JSHandle<LinkedHashMap> LinkedHashMap::Set(const JSThread *thread, const JSHandle<LinkedHashMap> &obj,
    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    return LinkedHashTable<LinkedHashMap, LinkedHashMapObject>::Insert(thread, obj, key, value);
}

JSHandle<LinkedHashMap> LinkedHashMap::SetWeakRef(const JSThread *thread, const JSHandle<LinkedHashMap> &obj,
    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    return LinkedHashTable<LinkedHashMap, LinkedHashMapObject>::InsertWeakRef(thread, obj, key, value);
}

JSTaggedValue LinkedHashMap::Get(JSTaggedValue key) const
{
    int entry = FindElement(key);
    if (entry == -1) {
        return JSTaggedValue::Undefined();
    }
    return GetValue(entry);
}

bool LinkedHashMap::Has(JSTaggedValue key) const
{
    int entry = FindElement(key);
    return entry != -1;
}

void LinkedHashMap::Clear(const JSThread *thread)
{
    int numberOfElements = NumberOfElements() + NumberOfDeletedElements();
    for (int entry = 0; entry < numberOfElements; entry++) {
        SetKey(thread, entry, JSTaggedValue::Hole());
        SetValue(thread, entry, JSTaggedValue::Hole());
    }
    SetNumberOfElements(thread, 0);
    SetNumberOfDeletedElements(thread, numberOfElements);
}

JSHandle<LinkedHashMap> LinkedHashMap::Shrink(const JSThread *thread, const JSHandle<LinkedHashMap> &table,
                                              int additionalCapacity)
{
    return LinkedHashTable<LinkedHashMap, LinkedHashMapObject>::Shrink(thread, table, additionalCapacity);
}

// LinkedHashSet
JSHandle<LinkedHashSet> LinkedHashSet::Create(const JSThread *thread, int numberOfElements)
{
    return LinkedHashTable<LinkedHashSet, LinkedHashSetObject>::Create(thread, numberOfElements);
}

JSHandle<LinkedHashSet> LinkedHashSet::Delete(const JSThread *thread, const JSHandle<LinkedHashSet> &obj,
                                              const JSHandle<JSTaggedValue> &key)
{
    return LinkedHashTable<LinkedHashSet, LinkedHashSetObject>::Remove(thread, obj, key);
}

JSHandle<LinkedHashSet> LinkedHashSet::Add(const JSThread *thread, const JSHandle<LinkedHashSet> &obj,
                                           const JSHandle<JSTaggedValue> &key)
{
    return LinkedHashTable<LinkedHashSet, LinkedHashSetObject>::Insert(thread, obj, key, key);
}

JSHandle<LinkedHashSet> LinkedHashSet::AddWeakRef(const JSThread *thread, const JSHandle<LinkedHashSet> &obj,
    const JSHandle<JSTaggedValue> &key)
{
    return LinkedHashTable<LinkedHashSet, LinkedHashSetObject>::InsertWeakRef(thread, obj, key, key);
}

bool LinkedHashSet::Has(JSTaggedValue key) const
{
    int entry = FindElement(key);
    return entry != -1;
}

void LinkedHashSet::Clear(const JSThread *thread)
{
    int numberOfElements = NumberOfElements() + NumberOfDeletedElements();
    for (int entry = 0; entry < numberOfElements; entry++) {
        SetKey(thread, entry, JSTaggedValue::Hole());
    }
    SetNumberOfElements(thread, 0);
    SetNumberOfDeletedElements(thread, numberOfElements);
}

JSHandle<LinkedHashSet> LinkedHashSet::Shrink(const JSThread *thread, const JSHandle<LinkedHashSet> &table,
    int additionalCapacity)
{
    return LinkedHashTable<LinkedHashSet, LinkedHashSetObject>::Shrink(thread, table, additionalCapacity);
}

int LinkedHash::Hash(JSTaggedValue key)
{
    if (key.IsDouble() && key.GetDouble() == 0.0) {
        key = JSTaggedValue(0);
    }
    if (key.IsSymbol()) {
        auto symbolString = JSSymbol::Cast(key.GetHeapObject());
        return symbolString->GetHashField();
    }
    if (key.IsString()) {
        auto keyString = reinterpret_cast<EcmaString *>(key.GetHeapObject());
        return keyString->GetHashcode();
    }
    if (key.IsECMAObject()) {
        int32_t hash = ECMAObject::Cast(key.GetHeapObject())->GetHash();
        if (hash == 0) {
            uint64_t keyValue = key.GetRawData();
            hash = static_cast<int32_t>(
                GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t)));
            ECMAObject::Cast(key.GetHeapObject())->SetHash(hash);
        }
        return hash;
    }

    // Int, Double, Special and HeapObject(except symbol and string)
    uint64_t keyValue = key.GetRawData();
    return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
}
}  // namespace panda::ecmascript
