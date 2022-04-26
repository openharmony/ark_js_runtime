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

#ifndef ECMASCRIPT_TAGGED_HASH_TABLE_H
#define ECMASCRIPT_TAGGED_HASH_TABLE_H

#include <vector>

#include "js_handle.h"
#include "tagged_array.h"

namespace panda::ecmascript {
template<typename Derived>
class TaggedHashTable : public TaggedArray {
public:
    inline int EntriesCount() const
    {
        return Get(NUMBER_OF_ENTRIES_INDEX).GetInt();
    }

    inline int HoleEntriesCount() const
    {
        return Get(NUMBER_OF_HOLE_ENTRIES_INDEX).GetInt();
    }

    inline int Size() const
    {
        return Get(SIZE_INDEX).GetInt();
    }

    inline void IncreaseEntries(const JSThread *thread)
    {
        SetEntriesCount(thread, EntriesCount() + 1);
    }

    inline void IncreaseHoleEntriesCount(const JSThread *thread, int number = 1)
    {
        SetEntriesCount(thread, EntriesCount() - number);
        SetHoleEntriesCount(thread, HoleEntriesCount() + number);
    }

    inline static int ComputeHashTableSize(uint32_t atLeastSize)
    {
        //  increase size for hash-collision
        uint32_t rawSize = atLeastSize + (atLeastSize >> 1UL);
        int newSize = static_cast<int>(helpers::math::GetPowerOfTwoValue32(rawSize));
        return (newSize > MIN_SIZE) ? newSize : MIN_SIZE;
    }

    static JSHandle<Derived> GrowHashTable(const JSThread *thread, const JSHandle<Derived> &table,
                                           int numOfAddedElements = 1)
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

    static JSHandle<Derived> Create(const JSThread *thread, int entriesCount)
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

    static JSHandle<Derived> Insert(const JSThread *thread, JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
    {
        // Make sure the key object has an identity hash code.
        int32_t hash = static_cast<int32_t>(Derived::Hash(key.GetTaggedValue()));
        int entry = table->FindEntry(key.GetTaggedValue());
        if (entry != -1) {
            table->SetValue(thread, entry, value.GetTaggedValue());
            return table;
        }

        JSHandle<Derived> newTable = GrowHashTable(thread, table);
        newTable->AddElement(thread, newTable->FindInsertIndex(hash), key, value);
        return newTable;
    }

    static JSHandle<Derived> Remove(const JSThread *thread, JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key)
    {
        int entry = table->FindEntry(key.GetTaggedValue());
        if (entry == -1) {
            return table;
        }

        table->RemoveElement(thread, entry);
        return Derived::Shrink(thread, *table);
    }

    inline static int RecalculateTableSize(int currentSize, int atLeastSize)
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

    inline static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &table, int additionalSize)
    {
        int newSize = RecalculateTableSize(table->Size(), table->EntriesCount() + additionalSize);
        if (newSize == table->Size()) {
            return table;
        }

        JSHandle<Derived> newTable = TaggedHashTable::Create(thread, newSize);

        table->Rehash(thread, *newTable);
        return newTable;
    }

    bool IsNeedGrowHashTable(int numOfAddEntries)
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

    JSTaggedValue GetKey(int entry) const
    {
        int index = Derived::GetKeyIndex(entry);
        if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
            return JSTaggedValue::Undefined();
        }
        return Get(index);
    }

    JSTaggedValue GetValue(int entry) const
    {
        int index = Derived::GetValueIndex(entry);
        if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
            return JSTaggedValue::Undefined();
        }
        return Get(index);
    }

    inline void GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const
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

    inline void GetAllKeysIntoVector(std::vector<JSTaggedValue> &vector) const
    {
        int capacity = Size();
        for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
            JSTaggedValue key = GetKey(hashIndex);
            if (!key.IsUndefined() && !key.IsHole()) {
                vector.push_back(key);
            }
        }
    }

    inline void Swap(const JSThread *thread, int src, int dst);

    // Find entry for key otherwise return -1.
    inline int FindEntry(const JSTaggedValue &key)
    {
        int size = Size();
        int count = 1;
        JSTaggedValue keyValue;
        int32_t hash = static_cast<int32_t>(Derived::Hash(key));

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

    inline int FindInsertIndex(int hash)
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

    inline void SetKey(const JSThread *thread, int entry, const JSTaggedValue &key)
    {
        int index = Derived::GetKeyIndex(entry);
        if (UNLIKELY(index < 0 || index > static_cast<int>(GetLength()))) {
            return;
        }
        Set(thread, index, key);
    }

    inline void SetValue(const JSThread *thread, int entry, const JSTaggedValue &value)
    {
        int index = Derived::GetValueIndex(entry);
        if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
            return;
        }
        Set(thread, index, value);
    }

    static constexpr int MIN_SHRINK_SIZE = 16;
    static constexpr int MIN_SIZE = 4;
    static constexpr int NUMBER_OF_ENTRIES_INDEX = 0;
    static constexpr int NUMBER_OF_HOLE_ENTRIES_INDEX = 1;
    static constexpr int SIZE_INDEX = 2;
    static constexpr int TABLE_HEADER_SIZE = 3;

protected:
    inline bool IsKey(const JSTaggedValue &key) const
    {
        return !key.IsHole() && !key.IsUndefined();
    };

    inline static uint32_t GetFirstPosition(uint32_t hash, uint32_t size)
    {
        return hash & (size - 1);
    }

    inline static uint32_t GetNextPosition(uint32_t last, uint32_t number, uint32_t size)
    {
        return (last + (number * (number + 1)) / 2) & (size - 1);  // 2 : half
    }

    inline void SetEntriesCount(const JSThread *thread, int nof)
    {
        Set(thread, NUMBER_OF_ENTRIES_INDEX, JSTaggedValue(nof));
    }

    inline void SetHoleEntriesCount(const JSThread *thread, int nod)
    {
        Set(thread, NUMBER_OF_HOLE_ENTRIES_INDEX, JSTaggedValue(nod));
    }

    // Sets the size of the hash table.
    inline void SetHashTableSize(const JSThread *thread, int size)
    {
        Set(thread, SIZE_INDEX, JSTaggedValue(size));
    }

    inline static int GetHeadSizeOfTable();
    inline static int GetEntrySize();
    inline static int GetKeyOffset();
    inline static int GetValueOffset();

    inline void AddElement(const JSThread *thread, int entry, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value)
    {
        this->SetKey(thread, entry, key.GetTaggedValue());
        this->SetValue(thread, entry, value.GetTaggedValue());
        this->IncreaseEntries(thread);
    }

    inline void RemoveElement(const JSThread *thread, int entry)
    {
        JSTaggedValue defaultValue(JSTaggedValue::Hole());
        this->SetKey(thread, entry, defaultValue);
        this->SetValue(thread, entry, defaultValue);
        this->IncreaseHoleEntriesCount(thread);
    }

    // Rehash element to new_table
    void Rehash(const JSThread *thread, Derived *newTable)
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
            int32_t hash = static_cast<int32_t>(Derived::Hash(k));
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
};

template<typename Derived>
class OrderTaggedHashTable : public TaggedHashTable<Derived> {
public:
    using HashTableT = TaggedHashTable<Derived>;
    static Derived *Cast(TaggedObject *object)
    {
        return reinterpret_cast<Derived *>(object);
    }

    // Attempt to shrink the table after deletion of key.
    static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &table)
    {
        int index = table->GetNextEnumerationIndex();
        JSHandle<Derived> newTable = HashTableT::Shrink(thread, table, 0);
        newTable->SetNextEnumerationIndex(thread, index);
        return newTable;
    }

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER)
    {
        JSHandle<Derived> dict = HashTableT::Create(thread, numberOfElements);
        dict->SetNextEnumerationIndex(thread, PropertyAttributes::INITIAL_PROPERTY_INDEX);
        return dict;
    }

    static JSHandle<Derived> PutIfAbsent(const JSThread *thread, const JSHandle<Derived> &table,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                         const PropertyAttributes &metaData)
    {
        int32_t hash = static_cast<int32_t>(Derived::Hash(key.GetTaggedValue()));

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

    static JSHandle<Derived> Put(const JSThread *thread, const JSHandle<Derived> &table,
                                 const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
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
    static JSHandle<Derived> Remove(const JSThread *thread, const JSHandle<Derived> &table, int entry)
    {
        if (!(table->IsKey(table->GetKey(entry)))) {
            return table;
        }
        table->ClearEntry(thread, entry);
        table->IncreaseHoleEntriesCount(thread);
        return Shrink(thread, table);
    }

    inline void SetNextEnumerationIndex(const JSThread *thread, int index)
    {
        HashTableT::Set(thread, NEXT_ENUMERATION_INDEX, JSTaggedValue(index));
    }
    inline int GetNextEnumerationIndex() const
    {
        return HashTableT::Get(NEXT_ENUMERATION_INDEX).GetInt();
    }

    inline int NextEnumerationIndex(const JSThread *thread)
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

    inline std::vector<int> GetEnumerationOrder()
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

    static const int NEXT_ENUMERATION_INDEX = HashTableT::SIZE_INDEX + 1;
    static const int DEFAULT_ELEMENTS_NUMBER = 128;
    static constexpr int TABLE_HEADER_SIZE = 4;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_NEW_HASH_TABLE_H