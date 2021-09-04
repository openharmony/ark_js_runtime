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

#ifndef PANDA_RUNTIME_ECMASCRIPT_LINKED_HASH_TABLE_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_LINKED_HASH_TABLE_INL_H

#include "linked_hash_table.h"
#include "tagged_array-inl.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript {
template <typename Derived, typename HashObject>
JSTaggedValue LinkedHashTable<Derived, HashObject>::GetElement(int index) const
{
    if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
        return JSTaggedValue::Undefined();
    }
    return Get(index);
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetElement(const JSThread *thread, int index, JSTaggedValue element)
{
    if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
        return;
    }
    Set(thread, index, element);
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::NumberOfElements() const
{
    return Get(NUMBER_OF_ELEMENTS_INDEX).GetInt();
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::NumberOfDeletedElements() const
{
    return Get(NUMBER_OF_DELETED_ELEMENTS_INDEX).GetInt();
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::Capacity() const
{
    return JSTaggedValue(Get(CAPACITY_INDEX)).GetInt();
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetNumberOfElements(const JSThread *thread, int nof)
{
    Set(thread, NUMBER_OF_ELEMENTS_INDEX, JSTaggedValue(nof));
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetNumberOfDeletedElements(const JSThread *thread, int nod)
{
    Set(thread, NUMBER_OF_DELETED_ELEMENTS_INDEX, JSTaggedValue(nod));
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetCapacity(const JSThread *thread, int capacity)
{
    Set(thread, CAPACITY_INDEX, JSTaggedValue(capacity));
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetNextTable(const JSThread *thread, JSTaggedValue nextTable)
{
    Set(thread, NEXT_TABLE_INDEX, nextTable);
}

template <typename Derived, typename HashObject>
JSTaggedValue LinkedHashTable<Derived, HashObject>::GetNextTable() const
{
    return JSTaggedValue(Get(NEXT_TABLE_INDEX));
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::GetDeletedNum(int entry) const
{
    ASSERT_PRINT(!GetNextTable().IsUndefined(), "function only execute after rehash");
    return GetNextEntry(entry).GetInt();
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetDeletedNum(const JSThread *thread, int entry, JSTaggedValue num)
{
    ASSERT_PRINT(!GetNextTable().IsUndefined(), "function only execute after rehash");
    SetNextEntry(thread, entry, num);
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::GetDeletedElementsAt(int entry) const
{
    ASSERT_PRINT(!GetNextTable().IsUndefined(), "function only execute after rehash");
    int currentEntry = entry - 1;
    while (currentEntry >= 0) {
        if (GetKey(currentEntry).IsHole()) {
            return GetDeletedNum(currentEntry);
        }
        currentEntry--;
    }
    return 0;
}

template <typename Derived, typename HashObject>
uint32_t LinkedHashTable<Derived, HashObject>::HashToBucket(uint32_t hash) const
{
    return hash & static_cast<uint32_t>(Capacity() - 1);
}

template <typename Derived, typename HashObject>
uint32_t LinkedHashTable<Derived, HashObject>::BucketToIndex(uint32_t bucket)
{
    return bucket + ELEMENTS_START_INDEX;
}

template <typename Derived, typename HashObject>
uint32_t LinkedHashTable<Derived, HashObject>::EntryToIndex(uint32_t entry) const
{
    return ELEMENTS_START_INDEX + Capacity() + entry * (HashObject::ENTRY_SIZE + 1);
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetKey(const JSThread *thread, int entry, JSTaggedValue key)
{
    int index = EntryToIndex(entry);
    SetElement(thread, index, key);
}

template <typename Derived, typename HashObject>
JSTaggedValue LinkedHashTable<Derived, HashObject>::GetKey(int entry) const
{
    int index = EntryToIndex(entry);
    return GetElement(index);
}

template <typename Derived, typename HashObject>
JSTaggedValue LinkedHashTable<Derived, HashObject>::GetValue(int entry) const
{
    int index = EntryToIndex(entry) + HashObject::ENTRY_VALUE_INDEX;
    return GetElement(index);
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetValue(const JSThread *thread, int entry, JSTaggedValue value)
{
    int index = EntryToIndex(entry) + HashObject::ENTRY_VALUE_INDEX;
    SetElement(thread, index, value);
}

template <typename Derived, typename HashObject>
JSTaggedValue LinkedHashTable<Derived, HashObject>::GetNextEntry(int entry) const
{
    int index = EntryToIndex(entry) + HashObject::ENTRY_SIZE;
    return GetElement(index);
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::SetNextEntry(const JSThread *thread, int entry, JSTaggedValue nextEntry)
{
    int index = EntryToIndex(entry) + HashObject::ENTRY_SIZE;
    SetElement(thread, index, nextEntry);
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::InsertNewEntry(const JSThread *thread, int bucket, int entry)
{
    int bucketIndex = BucketToIndex(bucket);
    JSTaggedValue previousEntry = GetElement(bucketIndex);
    SetNextEntry(thread, entry, previousEntry);
    SetElement(thread, bucketIndex, JSTaggedValue(entry));
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::FindElement(JSTaggedValue key) const
{
    if (!IsKey(key)) {
        return -1;
    }
    int hash = HashObject::Hash(key);
    int bucket = HashToBucket(hash);
    for (JSTaggedValue entry = GetElement(BucketToIndex(bucket)); !entry.IsHole();
         entry = GetNextEntry(entry.GetInt())) {
        JSTaggedValue element = GetKey(entry.GetInt());
        if (element.IsHole()) {
            continue;
        }
        if (element.IsWeak()) {
            element.RemoveWeakTag();
        }
        if (HashObject::IsMatch(key, element)) {
            return entry.GetInt();
        }
    }
    return -1;
}  // namespace panda::ecmascript

template <typename Derived, typename HashObject>
bool LinkedHashTable<Derived, HashObject>::HasSufficientCapacity(int numOfAddElements) const
{
    int numberOfElements = NumberOfElements();
    int numOfDelElements = NumberOfDeletedElements();
    int capacity = Capacity();
    int nof = numberOfElements + numOfAddElements;
    // Return true if:
    //   50% is still free after adding numOfAddElements elements and
    //   at most 50% of the free elements are deleted elements.
    if ((nof < capacity) && ((numOfDelElements <= (capacity - nof) / 2))) {  // 2: half
        int neededFree = nof / 2;                                            // 2: half
        if (nof + neededFree <= capacity) {
            return true;
        }
    }
    return false;
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::ComputeCapacity(uint32_t atLeastSpaceFor)
{
    // Add 50% slack to make slot collisions sufficiently unlikely.
    // See matching computation in HashTable::HasSufficientCapacity().
    uint32_t rawCap = atLeastSpaceFor + (atLeastSpaceFor >> 1UL);
    int capacity = static_cast<int>(helpers::math::GetPowerOfTwoValue32(rawCap));
    return (capacity > MIN_CAPACITY) ? capacity : MIN_CAPACITY;
}

template <typename Derived, typename HashObject>
void LinkedHashTable<Derived, HashObject>::RemoveEntry(const JSThread *thread, int entry)
{
    ASSERT_PRINT(entry >= 0 && entry < Capacity(), "entry must be a non-negative integer less than capacity");
    int index = EntryToIndex(entry);
    for (int i = 0; i < HashObject::ENTRY_SIZE; i++) {
        SetElement(thread, index + i, JSTaggedValue::Hole());
    }
    SetNumberOfElements(thread, NumberOfElements() - 1);
    SetNumberOfDeletedElements(thread, NumberOfDeletedElements() + 1);
}

template <typename Derived, typename HashObject>
int LinkedHashTable<Derived, HashObject>::ComputeCapacityWithShrink(int currentCapacity, int atLeastSpaceFor)
{
    // Shrink to fit the number of elements if only a quarter of the
    // capacity is filled with elements.
    if (atLeastSpaceFor > (currentCapacity / 4)) {  // 4: quarter
        return currentCapacity;
    }
    // Recalculate the smaller capacity actually needed.
    int newCapacity = ComputeCapacity(atLeastSpaceFor);
    ASSERT_PRINT(newCapacity > atLeastSpaceFor, "new capacity must greater than atLeastSpaceFor");
    // Don't go lower than room for MIN_SHRINK_CAPACITY elements.
    if (newCapacity < Derived::MIN_SHRINK_CAPACITY) {
        return currentCapacity;
    }
    return newCapacity;
}

bool LinkedHashMapObject::IsMatch(JSTaggedValue key, JSTaggedValue other)
{
    return JSTaggedValue::SameValueZero(key, other);
}
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_LINKED_HASH_TABLE_INL_H