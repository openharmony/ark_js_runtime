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

#include "ecmascript/layout_info-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/assert_scope-inl.h"

namespace panda::ecmascript {
void LayoutInfo::AddKey(const JSThread *thread, [[maybe_unused]] int index, const JSTaggedValue &key,
                        const PropertyAttributes &attr)
{
    DISALLOW_GARBAGE_COLLECTION;
    int number = NumberOfElements();
    ASSERT(attr.GetOffset() == static_cast<uint32_t>(number));
    ASSERT(number + 1 <= GetPropertiesCapacity());
    ASSERT(number == index);
    SetNumberOfElements(thread, number + 1);
    SetPropertyInit(thread, number, key, attr);

    uint32_t keyHash = key.GetKeyHashCode();
    int insertIndex = number;
    for (; insertIndex > 0; --insertIndex) {
        JSTaggedValue prevKey = GetSortedKey(insertIndex - 1);
        if (prevKey.GetKeyHashCode() <= keyHash) {
            break;
        }
        SetSortedIndex(thread, insertIndex, GetSortedIndex(insertIndex - 1));
    }
    SetSortedIndex(thread, insertIndex, number);
}

void LayoutInfo::GetAllKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset + end <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString()) {
            keyArray->Set(thread, enumKeys + offset, key);
            enumKeys++;
        }
    }

    if (enumKeys < end) {
        for (int i = 0; i < end; i++) {
            JSTaggedValue key = GetKey(i);
            if (key.IsSymbol()) {
                keyArray->Set(thread, enumKeys + offset, key);
                enumKeys++;
            }
        }
    }
}

void LayoutInfo::GetAllKeys(const JSThread *thread, int end, std::vector<JSTaggedValue> &keyVector)
{
    ASSERT(end <= NumberOfElements());
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString()) {
            keyVector.emplace_back(key);
            enumKeys++;
        }
    }
}

void LayoutInfo::GetAllEnumKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray,
                                uint32_t *keys)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset + end <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString() && GetAttr(i).IsEnumerable()) {
            keyArray->Set(thread, enumKeys + offset, key);
            enumKeys++;
        }
    }
    *keys += enumKeys;
}

void LayoutInfo::GetAllNames(const JSThread *thread, int end, const JSHandle<TaggedArray> &keyArray,
                             uint32_t *length)
{
    DISALLOW_GARBAGE_COLLECTION;
    int arrayIndex = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString()) {
            PropertyAttributes attr = GetAttr(i);
            if (attr.IsEnumerable()) {
                keyArray->Set(thread, arrayIndex++, key);
            }
        }
    }
    *length += arrayIndex;
}
}  // namespace panda::ecmascript
