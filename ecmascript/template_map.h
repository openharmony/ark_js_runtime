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

#ifndef ECMASCRIPT_TEMPLATE_MAP_H
#define ECMASCRIPT_TEMPLATE_MAP_H

#include "ecmascript/tagged_hash_table-inl.h"
#include "ecmascript/js_array.h"

namespace panda::ecmascript {
class TemplateMap : public TaggedHashTable<TemplateMap> {
public:
    using HashTable = TaggedHashTable<TemplateMap>;
    static inline bool IsMatch(const JSTaggedValue &key, const JSTaggedValue &other)
    {
        return key == other;
    }
    static inline int Hash(const JSTaggedValue &obj)
    {
        ASSERT(obj.IsJSArray());
        JSArray *array = JSArray::Cast(obj.GetHeapObject());
        uint32_t len = array->GetArrayLength();
        return len;
    }
    inline static int GetKeyIndex(int entry)
    {
        return HashTable::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_KEY_INDEX;
    }
    inline static int GetValueIndex(int entry)
    {
        return HashTable::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_VALUE_INDEX;
    }
    inline static int GetEntryIndex(int entry)
    {
        return HashTable::TABLE_HEADER_SIZE + entry * GetEntrySize();
    }
    inline static int GetEntrySize()
    {
        return ENTRY_SIZE;
    }
    static TemplateMap *Cast(ObjectHeader *object)
    {
        return reinterpret_cast<TemplateMap *>(object);
    }
    static const int DEFAULT_ELEMENTS_NUMBER = 16;
    static JSHandle<TemplateMap> Create(JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER)
    {
        return HashTable::Create(thread, numberOfElements);
    }
    static const int ENTRY_SIZE = 2;
    static const int ENTRY_KEY_INDEX = 0;
    static const int ENTRY_VALUE_INDEX = 1;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TEMPLATE_MAP_H