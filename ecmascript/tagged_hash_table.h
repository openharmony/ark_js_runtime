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
    inline int EntriesCount() const;

    inline int HoleEntriesCount() const;

    inline int Size() const;

    inline void IncreaseEntries(const JSThread *thread);

    inline void IncreaseHoleEntriesCount(const JSThread *thread, int number = 1);

    inline static int ComputeHashTableSize(uint32_t atLeastSize);

    static JSHandle<Derived> GrowHashTable(const JSThread *thread, const JSHandle<Derived> &table,
                                           int numOfAddedElements = 1);

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfElements);

    static JSHandle<Derived> Insert(const JSThread *thread, JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static JSHandle<Derived> Remove(const JSThread *thread, JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key);

    inline static int RecalculateTableSize(int currentSize, int atLeastSize);

    inline static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &table, int additionalSize);

    bool IsNeedGrowHashTable(int numOfAddEntries);

    JSTaggedValue GetKey(int entry) const;

    JSTaggedValue GetValue(int entry) const;

    inline void GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const;

    inline void GetAllKeysIntoVector(std::vector<JSTaggedValue> &vector) const;

    inline void Swap(const JSThread *thread, int src, int dst);

    // Find entry for key otherwise return -1.
    inline int FindEntry(const JSTaggedValue &key);

    inline int FindInsertIndex(int hash);

    inline void SetKey(const JSThread *thread, int entry, const JSTaggedValue &key);

    inline void SetValue(const JSThread *thread, int entry, const JSTaggedValue &value);

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

    inline void SetEntriesCount(const JSThread *thread, int noe);

    inline void SetHoleEntriesCount(const JSThread *thread, int nod);

    // Sets the size of the hash table.
    inline void SetHashTableSize(const JSThread *thread, int size);

    inline static int GetHeadSizeOfTable();
    inline static int GetEntrySize();
    inline static int GetKeyOffset();
    inline static int GetValueOffset();

    inline void AddElement(const JSThread *thread, int entry, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value);

    inline void RemoveElement(const JSThread *thread, int entry);

    // Rehash element to new_table
    void Rehash(const JSThread *thread, Derived *newTable);
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

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER);
    static JSHandle<Derived> PutIfAbsent(const JSThread *thread, const JSHandle<Derived> &table,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                         const PropertyAttributes &metaData);
    static JSHandle<Derived> Put(const JSThread *thread, const JSHandle<Derived> &table,
                                 const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                 const PropertyAttributes &metaData);
    static JSHandle<Derived> Remove(const JSThread *thread, const JSHandle<Derived> &table, int entry);

    inline void SetNextEnumerationIndex(const JSThread *thread, int index)
    {
        HashTableT::Set(thread, NEXT_ENUMERATION_INDEX, JSTaggedValue(index));
    }
    inline int GetNextEnumerationIndex() const
    {
        return HashTableT::Get(NEXT_ENUMERATION_INDEX).GetInt();
    }

    inline int NextEnumerationIndex(const JSThread *thread);

    inline std::vector<int> GetEnumerationOrder();

    static const int NEXT_ENUMERATION_INDEX = HashTableT::SIZE_INDEX + 1;
    static const int DEFAULT_ELEMENTS_NUMBER = 128;
    static constexpr int TABLE_HEADER_SIZE = 4;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_NEW_HASH_TABLE_H