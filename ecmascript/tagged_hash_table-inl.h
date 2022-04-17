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

#ifndef ECMASCRIPT_TAGGED_HASH_TABLE_INL_H
#define ECMASCRIPT_TAGGED_HASH_TABLE_INL_H

#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda::ecmascript {
template<typename Derived>
int TaggedHashTable<Derived>::EntriesCount() const
{
    return Get(NUMBER_OF_ENTRIES_INDEX).GetInt();
}

template<typename Derived>
int TaggedHashTable<Derived>::HoleEntriesCount() const
{
    return Get(NUMBER_OF_HOLE_ENTRIES_INDEX).GetInt();
}

template<typename Derived>
int TaggedHashTable<Derived>::Size() const
{
    return Get(SIZE_INDEX).GetInt();
}

template<typename Derived>
void TaggedHashTable<Derived>::IncreaseEntries(const JSThread *thread)
{
    SetEntriesCount(thread, EntriesCount() + 1);
}

template<typename Derived>
void TaggedHashTable<Derived>::IncreaseHoleEntriesCount(const JSThread *thread, int number)
{
    SetEntriesCount(thread, EntriesCount() - number);
    SetHoleEntriesCount(thread, HoleEntriesCount() + number);
}

template<typename Derived>
void TaggedHashTable<Derived>::SetEntriesCount(const JSThread *thread, int nof)
{
    Set(thread, NUMBER_OF_ENTRIES_INDEX, JSTaggedValue(nof));
}

template<typename Derived>
void TaggedHashTable<Derived>::SetHoleEntriesCount(const JSThread *thread, int nod)
{
    Set(thread, NUMBER_OF_HOLE_ENTRIES_INDEX, JSTaggedValue(nod));
}

template<typename Derived>
void TaggedHashTable<Derived>::SetHashTableSize(const JSThread *thread, int size)
{
    Set(thread, SIZE_INDEX, JSTaggedValue(size));
}

template<typename Derived>
void TaggedHashTable<Derived>::GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const
{
    ASSERT_PRINT(offset + EntriesCount() <= static_cast<int>(keyArray->GetLength()),
                 "keyArray size is not enough for dictionary");
    int arrayIndex = 0;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            keyArray->Set(thread, arrayIndex + offset, key);
            arrayIndex++;
        }
    }
}

// Find entry for key otherwise return -1.
template<typename Derived>
int TaggedHashTable<Derived>::FindEntry(const JSTaggedValue &key)
{
    int size = Size();
    int count = 1;
    JSTaggedValue keyValue;
    uint32_t hash = Derived::Hash(key);

    for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
        keyValue = GetKey(entry);
        if (keyValue.IsHole()) {
            continue;
        }
        if (keyValue.IsUndefined()) {
            return -1;
        }
        if (Derived::IsMatch(key, keyValue)) {
            return entry;
        }
    }
    return -1;
}

// static
template<typename Derived>
int TaggedHashTable<Derived>::RecalculateTableSize(int currentSize, int atLeastSize)
{
    // When the filled entries is greater than a quart of currentSize
    // it need not to shrink
    if (atLeastSize > (currentSize / 4)) {  // 4 : quarter
        return currentSize;
    }
    // Recalculate table size
    int newSize = ComputeHashTableSize(atLeastSize);
    ASSERT_PRINT(newSize > atLeastSize, "new size must greater than atLeastSize");
    // Don't go lower than room for MIN_SHRINK_SIZE elements.
    if (newSize < MIN_SHRINK_SIZE) {
        return currentSize;
    }
    return newSize;
}

// static
template<typename Derived>
JSHandle<Derived> TaggedHashTable<Derived>::Shrink(const JSThread *thread, const JSHandle<Derived> &table,
                                                   int additionalSize)
{
    int newSize = RecalculateTableSize(table->Size(), table->EntriesCount() + additionalSize);
    if (newSize == table->Size()) {
        return table;
    }

    JSHandle<Derived> newTable = TaggedHashTable::Create(thread, newSize);

    table->Rehash(thread, *newTable);
    return newTable;
}

template<typename Derived>
bool TaggedHashTable<Derived>::IsNeedGrowHashTable(int numOfAddEntries)
{
    int entriesCount = EntriesCount();
    int numOfDelEntries = HoleEntriesCount();
    int currentSize = Size();
    int numberFilled = entriesCount + numOfAddEntries;
    // needn't to grow table:
    //   1. after adding number entries, table have half free entries.
    //   2. deleted entries are less than half of the free entries.
    const int halfFree = 2;
    if ((numberFilled < currentSize) && ((numOfDelEntries <= (currentSize - numberFilled) / halfFree))) {
        int neededFree = numberFilled / halfFree;
        if (numberFilled + neededFree <= currentSize) {
            return false;
        }
    }
    return true;
}

template<typename Derived>
void TaggedHashTable<Derived>::AddElement(const JSThread *thread, int entry, const JSHandle<JSTaggedValue> &key,
                                          const JSHandle<JSTaggedValue> &value)
{
    this->SetKey(thread, entry, key.GetTaggedValue());
    this->SetValue(thread, entry, value.GetTaggedValue());
    this->IncreaseEntries(thread);
}

template<typename Derived>
void TaggedHashTable<Derived>::RemoveElement(const JSThread *thread, int entry)
{
    JSTaggedValue defaultValue(JSTaggedValue::Hole());
    this->SetKey(thread, entry, defaultValue);
    this->SetValue(thread, entry, defaultValue);
    this->IncreaseHoleEntriesCount(thread);
}

template<typename Derived>
JSHandle<Derived> TaggedHashTable<Derived>::Insert(const JSThread *thread, JSHandle<Derived> &table,
                                                   const JSHandle<JSTaggedValue> &key,
                                                   const JSHandle<JSTaggedValue> &value)
{
    // Make sure the key object has an identity hash code.
    uint32_t hash = Derived::Hash(key.GetTaggedValue());
    int entry = table->FindEntry(key.GetTaggedValue());
    if (entry != -1) {
        table->SetValue(thread, entry, value.GetTaggedValue());
        return table;
    }

    JSHandle<Derived> newTable = GrowHashTable(thread, table);
    newTable->AddElement(thread, newTable->FindInsertIndex(hash), key, value);
    return newTable;
}

template<typename Derived>
JSHandle<Derived> TaggedHashTable<Derived>::Remove(const JSThread *thread, JSHandle<Derived> &table,
                                                   const JSHandle<JSTaggedValue> &key)
{
    int entry = table->FindEntry(key.GetTaggedValue());
    if (entry == -1) {
        return table;
    }

    table->RemoveElement(thread, entry);
    return Derived::Shrink(thread, *table);
}

template<typename Derived>
void TaggedHashTable<Derived>::Rehash(const JSThread *thread, Derived *newTable)
{
    if ((newTable == nullptr) || (newTable->Size() < EntriesCount())) {
        return;
    }
    int currentSize = this->Size();
    // Rehash elements to new table
    for (int i = 0; i < currentSize; i++) {
        int fromIndex = Derived::GetKeyIndex(i);
        JSTaggedValue k = this->GetKey(i);
        if (!IsKey(k)) {
            continue;
        }
        uint32_t hash = Derived::Hash(k);
        int insertionIndex = Derived::GetKeyIndex(newTable->FindInsertIndex(hash));
        JSTaggedValue tv = Get(fromIndex);
        newTable->Set(thread, insertionIndex, tv);
        for (int j = 1; j < Derived::GetEntrySize(); j++) {
            tv = Get(fromIndex + j);
            newTable->Set(thread, insertionIndex + j, tv);
        }
    }
    newTable->SetEntriesCount(thread, EntriesCount());
    newTable->SetHoleEntriesCount(thread, 0);
}

// static
template<typename Derived>
int TaggedHashTable<Derived>::ComputeHashTableSize(uint32_t atLeastSize)
{
    //  increase size for hash-collision
    uint32_t rawSize = atLeastSize + (atLeastSize >> 1UL);
    int newSize = static_cast<int>(helpers::math::GetPowerOfTwoValue32(rawSize));
    return (newSize > MIN_SIZE) ? newSize : MIN_SIZE;
}

template<typename Derived>
JSHandle<Derived> TaggedHashTable<Derived>::GrowHashTable(const JSThread *thread, const JSHandle<Derived> &table,
                                                          int numOfAddedElements)
{
    if (!table->IsNeedGrowHashTable(numOfAddedElements)) {
        return table;
    }
    int newSize = ComputeHashTableSize(table->Size() + numOfAddedElements);
    int length = Derived::GetEntryIndex(newSize);
    JSHandle<Derived> newTable(thread->GetEcmaVM()->GetFactory()->NewDictionaryArray(length));
    newTable->SetHashTableSize(thread, newSize);
    table->Rehash(thread, *newTable);
    return newTable;
}

template<typename Derived>
JSHandle<Derived> TaggedHashTable<Derived>::Create(const JSThread *thread, int entriesCount)
{
    ASSERT_PRINT((entriesCount > 0), "the size must be greater than zero");
    auto size = static_cast<uint32_t>(entriesCount);
    ASSERT_PRINT(helpers::math::IsPowerOfTwo(static_cast<uint32_t>(entriesCount)), "the size must be power of two");

    int length = Derived::GetEntryIndex(entriesCount);

    JSHandle<Derived> table(thread->GetEcmaVM()->GetFactory()->NewDictionaryArray(length));
    table->SetEntriesCount(thread, 0);
    table->SetHoleEntriesCount(thread, 0);
    table->SetHashTableSize(thread, size);
    return table;
}

template<typename Derived>
void TaggedHashTable<Derived>::SetKey(const JSThread *thread, int entry, const JSTaggedValue &key)
{
    int index = Derived::GetKeyIndex(entry);
    if (UNLIKELY(index < 0 || index > static_cast<int>(GetLength()))) {
        return;
    }
    Set(thread, index, key);
}

template<typename Derived>
JSTaggedValue TaggedHashTable<Derived>::GetKey(int entry) const
{
    int index = Derived::GetKeyIndex(entry);
    if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
        return JSTaggedValue::Undefined();
    }
    return Get(index);
}

template<typename Derived>
void TaggedHashTable<Derived>::SetValue(const JSThread *thread, int entry, const JSTaggedValue &value)
{
    int index = Derived::GetValueIndex(entry);
    if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
        return;
    }
    Set(thread, index, value);
}

template<typename Derived>
JSTaggedValue TaggedHashTable<Derived>::GetValue(int entry) const
{
    int index = Derived::GetValueIndex(entry);
    if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
        return JSTaggedValue::Undefined();
    }
    return Get(index);
}

template<typename Derived>
int TaggedHashTable<Derived>::FindInsertIndex(int hash)
{
    int size = Size();
    int count = 1;
    // GrowHashTable will guarantee the hash table is never full.
    for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
        if (!IsKey(GetKey(entry))) {
            return entry;
        }
    }
}

template<typename Derived>
JSHandle<Derived> OrderTaggedHashTable<Derived>::Create(const JSThread *thread, int numberOfElements)
{
    JSHandle<Derived> dict = HashTableT::Create(thread, numberOfElements);
    dict->SetNextEnumerationIndex(thread, PropertyAttributes::INITIAL_PROPERTY_INDEX);
    return dict;
}

template<typename Derived>
JSHandle<Derived> OrderTaggedHashTable<Derived>::PutIfAbsent(const JSThread *thread, const JSHandle<Derived> &table,
                                                             const JSHandle<JSTaggedValue> &key,
                                                             const JSHandle<JSTaggedValue> &value,
                                                             const PropertyAttributes &metaData)
{
    uint32_t hash = Derived::Hash(key.GetTaggedValue());

    /* no need to add key if exist */
    int entry = table->FindEntry(key.GetTaggedValue());
    if (entry != -1) {
        return table;
    }
    int enumIndex = table->NextEnumerationIndex(thread);
    PropertyAttributes attr(metaData);
    attr.SetDictionaryOrder(enumIndex);
    // Check whether the table should be growed.
    JSHandle<Derived> newTable = HashTableT::GrowHashTable(thread, table);

    // Compute the key object.
    entry = newTable->FindInsertIndex(hash);
    newTable->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), attr);

    newTable->IncreaseEntries(thread);
    newTable->SetNextEnumerationIndex(thread, enumIndex + 1);
    return newTable;
}

template<typename Derived>
JSHandle<Derived> OrderTaggedHashTable<Derived>::Put(const JSThread *thread, const JSHandle<Derived> &table,
                                                     const JSHandle<JSTaggedValue> &key,
                                                     const JSHandle<JSTaggedValue> &value,
                                                     const PropertyAttributes &metaData)
{
    int hash = Derived::Hash(key.GetTaggedValue());
    int enumIndex = table->NextEnumerationIndex(thread);
    PropertyAttributes attr(metaData);
    attr.SetDictionaryOrder(enumIndex);
    int entry = table->FindEntry(key.GetTaggedValue());
    if (entry != -1) {
        table->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), attr);
        return table;
    }
    // Check whether the table should be extended.
    JSHandle<Derived> newTable = HashTableT::GrowHashTable(thread, table);

    // Compute the key object.
    entry = newTable->FindInsertIndex(hash);
    newTable->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), attr);

    newTable->IncreaseEntries(thread);
    newTable->SetNextEnumerationIndex(thread, enumIndex + 1);
    return newTable;
}

template<typename Derived>
void TaggedHashTable<Derived>::GetAllKeysIntoVector(std::vector<JSTaggedValue> &vector) const
{
    int capacity = Size();
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            vector.push_back(key);
        }
    }
}

template<typename Derived>
JSHandle<Derived> OrderTaggedHashTable<Derived>::Remove(const JSThread *thread, const JSHandle<Derived> &table,
                                                        int entry)
{
    if (!(table->IsKey(table->GetKey(entry)))) {
        return table;
    }
    table->ClearEntry(thread, entry);
    table->IncreaseHoleEntriesCount(thread);
    return Shrink(thread, table);
}

template<typename Derived>
int OrderTaggedHashTable<Derived>::NextEnumerationIndex(const JSThread *thread)
{
    int index = GetNextEnumerationIndex();
    auto table = Derived::Cast(this);

    if (!PropertyAttributes::IsValidIndex(index)) {
        std::vector<int> indexOrder = GetEnumerationOrder();
        int length = static_cast<int>(indexOrder.size());
        for (int i = 0; i < length; i++) {
            int oldIndex = indexOrder[i];
            int enumIndex = PropertyAttributes::INITIAL_PROPERTY_INDEX + i;
            PropertyAttributes attr = table->GetAttributes(oldIndex);
            attr.SetDictionaryOrder(enumIndex);
            table->SetAttributes(thread, oldIndex, attr);
        }
        index = PropertyAttributes::INITIAL_PROPERTY_INDEX + length;
    }
    return index;
}

template<typename Derived>
std::vector<int> OrderTaggedHashTable<Derived>::GetEnumerationOrder()
{
    std::vector<int> result;
    auto table = Derived::Cast(this);
    int size = table->Size();
    for (int i = 0; i < size; i++) {
        if (table->IsKey(table->GetKey(i))) {
            result.push_back(i);
        }
    }
    std::sort(result.begin(), result.end(), [table](int a, int b) {
        PropertyAttributes attrA = table->GetAttributes(a);
        PropertyAttributes attrB = table->GetAttributes(b);
        return attrA.GetDictionaryOrder() < attrB.GetDictionaryOrder();
    });
    return result;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_HASH_TABLE_INL_H
