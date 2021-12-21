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

#ifndef ECMASCRIPT_LINKED_HASH_TABLE_H
#define ECMASCRIPT_LINKED_HASH_TABLE_H

#include "ecmascript/js_tagged_value.h"
#include "js_handle.h"
#include "js_symbol.h"
#include "js_tagged_number.h"
#include "tagged_array.h"

namespace panda::ecmascript {
/**
 * memory in LinkedHashTable is divided into 3 parts
 * 1.array[0-2] is used to store common information of hashtale such as numberOfElements and capacity
 * 2.array[3,3+capacity] is buckets which store the position of entry
 * 3.array[3+capacity+1,3+capacity + capacity*(entry_size+1)] is the entry stored in order, the last element of an entry
 * is a number which point to next entry.
 * */
template<typename Derived, typename HashObject>
class LinkedHashTable : public TaggedArray {
public:
    static const int MIN_CAPACITY = 4;
    static const int NUMBER_OF_ELEMENTS_INDEX = 0;
    static const int NUMBER_OF_DELETED_ELEMENTS_INDEX = 1;
    static const int CAPACITY_INDEX = 2;
    static const int NEXT_TABLE_INDEX = 3;
    static const int ELEMENTS_START_INDEX = 4;
    // Don't shrink a HashTable below this capacity.
    static const int MIN_SHRINK_CAPACITY = 16;

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfElements);

    static JSHandle<Derived> Insert(const JSThread *thread, const JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static JSHandle<Derived> InsertWeakRef(const JSThread *thread, const JSHandle<Derived> &table,
                                           const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static JSHandle<Derived> GrowCapacity(const JSThread *thread, const JSHandle<Derived> &table,
                                          int numberOfAddedElements = 1);

    static JSHandle<Derived> Remove(const JSThread *thread, const JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key);

    static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &table, int additionalCapacity = 0);

    void Rehash(const JSThread *thread, Derived *newTable);

    inline bool HasSufficientCapacity(int numOfAddElements) const;

    inline int FindElement(JSTaggedValue key) const;

    inline void RemoveEntry(const JSThread *thread, int entry);

    inline static int ComputeCapacity(uint32_t atLeastSpaceFor);

    inline static int ComputeCapacityWithShrink(int currentCapacity, int atLeastSpaceFor);

    inline int NumberOfElements() const;

    inline int NumberOfDeletedElements() const;

    inline int Capacity() const;

    inline JSTaggedValue GetKey(int entry) const;

    inline JSTaggedValue GetValue(int entry) const;

    inline static bool IsKey(JSTaggedValue key)
    {
        return !key.IsHole();
    }

    inline void SetNumberOfElements(const JSThread *thread, int nof);

    inline void SetNumberOfDeletedElements(const JSThread *thread, int nod);

    inline void SetCapacity(const JSThread *thread, int capacity);

    inline JSTaggedValue GetNextTable() const;

    inline void SetNextTable(const JSThread *thread, JSTaggedValue nextTable);

    inline int GetDeletedElementsAt(int entry) const;

protected:
    inline JSTaggedValue GetElement(int index) const;

    inline void SetElement(const JSThread *thread, int index, JSTaggedValue element);

    inline void SetKey(const JSThread *thread, int entry, JSTaggedValue key);

    inline void SetValue(const JSThread *thread, int entry, JSTaggedValue value);

    inline JSTaggedValue GetNextEntry(int entry) const;

    inline void SetNextEntry(const JSThread *thread, int entry, JSTaggedValue nextEntry);

    inline uint32_t HashToBucket(uint32_t hash) const;

    inline static uint32_t BucketToIndex(uint32_t bucket);

    // min entry = 0
    inline uint32_t EntryToIndex(uint32_t entry) const;

    inline void InsertNewEntry(const JSThread *thread, int bucket, int entry);

    inline int GetDeletedNum(int entry) const;

    inline void SetDeletedNum(const JSThread *thread, int entry, JSTaggedValue num);
};

class LinkedHash {
public:
    static int Hash(JSTaggedValue key);
};

class LinkedHashMapObject {
public:
    // key must be string now for other object has no 'equals' method
    static inline bool IsMatch(JSTaggedValue key, JSTaggedValue other);

    static const int ENTRY_SIZE = 2;
    static const int ENTRY_VALUE_INDEX = 1;
};

class LinkedHashMap : public LinkedHashTable<LinkedHashMap, LinkedHashMapObject> {
public:
    static LinkedHashMap *Cast(ObjectHeader *obj)
    {
        return static_cast<LinkedHashMap *>(obj);
    }
    static JSHandle<LinkedHashMap> Create(const JSThread *thread, int numberOfElements = MIN_CAPACITY);

    static JSHandle<LinkedHashMap> Delete(const JSThread *thread, const JSHandle<LinkedHashMap> &obj,
        const JSHandle<JSTaggedValue> &key);

    static JSHandle<LinkedHashMap> Set(const JSThread *thread, const JSHandle<LinkedHashMap> &obj,
        const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static JSHandle<LinkedHashMap> SetWeakRef(const JSThread *thread, const JSHandle<LinkedHashMap> &obj,
        const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    JSTaggedValue Get(JSTaggedValue key) const;

    static JSHandle<LinkedHashMap> Shrink(const JSThread *thread, const JSHandle<LinkedHashMap> &table,
        int additionalCapacity = 0);

    bool Has(JSTaggedValue key) const;

    void Clear(const JSThread *thread);
    DECL_DUMP()
};

class LinkedHashSetObject {
public:
    // key must be string now for other object has no 'equals' method
    static inline bool IsMatch(JSTaggedValue key, JSTaggedValue other)
    {
        return JSTaggedValue::SameValueZero(key, other);
    }

    static const int ENTRY_SIZE = 1;
    static const int ENTRY_VALUE_INDEX = 0;
};

class LinkedHashSet : public LinkedHashTable<LinkedHashSet, LinkedHashSetObject> {
public:
    static LinkedHashSet *Cast(ObjectHeader *obj)
    {
        return static_cast<LinkedHashSet *>(obj);
    }
    static JSHandle<LinkedHashSet> Create(const JSThread *thread, int numberOfElements = MIN_CAPACITY);

    static JSHandle<LinkedHashSet> Delete(const JSThread *thread, const JSHandle<LinkedHashSet> &obj,
        const JSHandle<JSTaggedValue> &key);

    static JSHandle<LinkedHashSet> Add(const JSThread *thread, const JSHandle<LinkedHashSet> &obj,
        const JSHandle<JSTaggedValue> &key);

    static JSHandle<LinkedHashSet> AddWeakRef(const JSThread *thread, const JSHandle<LinkedHashSet> &obj,
        const JSHandle<JSTaggedValue> &key);

    static JSHandle<LinkedHashSet> Shrink(const JSThread *thread, const JSHandle<LinkedHashSet> &table,
        int additionalCapacity = 0);

    bool Has(JSTaggedValue key) const;

    void Clear(const JSThread *thread);
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LINKED_HASH_TABLE_H
