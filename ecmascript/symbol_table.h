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

#ifndef ECMASCRIPT_SYMBOL_TABLE_H
#define ECMASCRIPT_SYMBOL_TABLE_H

#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class SymbolTable : public TaggedHashTable<SymbolTable> {
public:
    using HashTable = TaggedHashTable<SymbolTable>;
    static SymbolTable *Cast(ObjectHeader *object)
    {
        return reinterpret_cast<SymbolTable *>(object);
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
    static inline bool IsMatch(const JSTaggedValue &name, const JSTaggedValue &other);
    static inline uint32_t Hash(const JSTaggedValue &obj);

    static const int DEFAULT_ELEMENTS_NUMBER = 64;
    static JSHandle<SymbolTable> Create(JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER)
    {
        return HashTable::Create(thread, numberOfElements);
    }

    inline bool ContainsKey(const JSTaggedValue &key);

    inline JSTaggedValue GetSymbol(const JSTaggedValue &key);

    inline JSTaggedValue FindSymbol(const JSTaggedValue &value);
    static constexpr int ENTRY_KEY_INDEX = 0;
    static constexpr int ENTRY_VALUE_INDEX = 1;
    static constexpr int ENTRY_SIZE = 2;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SYMBOL_TABLE_H