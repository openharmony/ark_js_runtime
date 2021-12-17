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

#ifndef ECMASCRIPT_LAYOUT_INFO_INL_H
#define ECMASCRIPT_LAYOUT_INFO_INL_H

#include "ecmascript/layout_info.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/ic/properties_cache-inl.h"

namespace panda::ecmascript {
inline int LayoutInfo::GetPropertiesCapacity() const
{
    return static_cast<int>((GetLength() - ELEMENTS_START_INDEX) >> 1U);
}

inline int LayoutInfo::NumberOfElements() const
{
    return TaggedArray::Get(NUMBER_OF_PROPERTIES_INDEX).GetInt();
}

inline void LayoutInfo::SetNumberOfElements(const JSThread *thread, int properties)
{
    return TaggedArray::Set(thread, NUMBER_OF_PROPERTIES_INDEX, JSTaggedValue(properties));
}

inline array_size_t LayoutInfo::GetKeyIndex(int index) const
{
    return ELEMENTS_START_INDEX + (static_cast<uint32_t>(index) << 1U);
}

inline array_size_t LayoutInfo::GetAttrIndex(int index) const
{
    return ELEMENTS_START_INDEX + (static_cast<uint32_t>(index) << 1U) + 1;
}

inline void LayoutInfo::SetPropertyInit(const JSThread *thread, int index, const JSTaggedValue &key,
                                        const PropertyAttributes &attr)
{
    array_size_t fixed_idx = GetKeyIndex(index);
    TaggedArray::Set(thread, fixed_idx, key);
    TaggedArray::Set(thread, fixed_idx + 1, attr.GetNormalTagged());
}

inline void LayoutInfo::SetNormalAttr(const JSThread *thread, int index, const PropertyAttributes &attr)
{
    array_size_t fixed_idx = GetAttrIndex(index);
    PropertyAttributes oldAttr(TaggedArray::Get(fixed_idx));
    oldAttr.SetNormalAttr(attr.GetNormalAttr());
    TaggedArray::Set(thread, fixed_idx, oldAttr.GetTaggedValue());
}

inline JSTaggedValue LayoutInfo::GetKey(int index) const
{
    array_size_t fixed_idx = GetKeyIndex(index);
    return TaggedArray::Get(fixed_idx);
}

inline PropertyAttributes LayoutInfo::GetAttr(int index) const
{
    array_size_t fixed_idx = GetAttrIndex(index);
    return PropertyAttributes(TaggedArray::Get(fixed_idx));
}

inline JSTaggedValue LayoutInfo::GetSortedKey(int index) const
{
    array_size_t fixed_idx = GetSortedIndex(index);
    return GetKey(fixed_idx);
}

inline array_size_t LayoutInfo::GetSortedIndex(int index) const
{
    return GetAttr(index).GetSortedIndex();
}

inline void LayoutInfo::SetSortedIndex(const JSThread *thread, int index, int sortedIndex)
{
    array_size_t fixed_idx = GetAttrIndex(index);
    PropertyAttributes attr(TaggedArray::Get(fixed_idx));
    attr.SetSortedIndex(sortedIndex);
    TaggedArray::Set(thread, fixed_idx, attr.GetTaggedValue());
}

inline int LayoutInfo::FindElementWithCache(JSThread *thread, JSHClass *cls, JSTaggedValue key,
                                            int propertiesNumber)
{
    ASSERT(NumberOfElements() >= propertiesNumber);
    const int MAX_ELEMENTS_LINER_SEARCH = 9; // 9: Builtins Object properties number is nine;
    if (propertiesNumber <= MAX_ELEMENTS_LINER_SEARCH) {
        Span<struct Properties> sp(GetProperties(), propertiesNumber);
        for (int i = 0; i < propertiesNumber; i++) {
            if (sp[i].key_ == key) {
                return i;
            }
        }
        return -1;
    }

    PropertiesCache *cache = thread->GetPropertiesCache();
    int index = cache->Get(cls, key);
    if (index == PropertiesCache::NOT_FOUND) {
        index = BinarySearch(key, propertiesNumber);
        cache->Set(cls, key, index);
    }
    return index;
}

inline int LayoutInfo::BinarySearch(JSTaggedValue key, int propertiesNumber)
{
    ASSERT(NumberOfElements() >= propertiesNumber);
    int low = 0;
    int elements = NumberOfElements();
    int high = elements - 1;
    uint32_t keyHash = key.GetKeyHashCode();

    ASSERT(low <= high);

    while (low <= high) {
        int mid = low + (high - low) / 2;  // 2: half
        JSTaggedValue midKey = GetSortedKey(mid);
        uint32_t midHash = midKey.GetKeyHashCode();
        if (midHash > keyHash) {
            high = mid - 1;
        } else if (midHash < keyHash) {
            low = mid + 1;
        } else {
            int sortIndex = GetSortedIndex(mid);
            JSTaggedValue currentKey = GetKey(sortIndex);
            if (currentKey == key) {
                return sortIndex < propertiesNumber ? sortIndex : -1;
            }
            int midLeft = mid;
            int midRight = mid;
            while (midLeft - 1 >= 0) {
                sortIndex = GetSortedIndex(--midLeft);
                currentKey = GetKey(sortIndex);
                if (currentKey.GetKeyHashCode() == keyHash) {
                    if (currentKey == key) {
                        return sortIndex < propertiesNumber ? sortIndex : -1;
                    }
                } else {
                    break;
                }
            }
            while (midRight + 1 < elements) {
                sortIndex = GetSortedIndex(++midRight);
                currentKey = GetKey(sortIndex);
                if (currentKey.GetKeyHashCode() == keyHash) {
                    if (currentKey == key) {
                        return sortIndex < propertiesNumber ? sortIndex : -1;
                    }
                } else {
                    break;
                }
            }
            return -1;
        }
    }
    return -1;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LAYOUT_INFO_INL_H
