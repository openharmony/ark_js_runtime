/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_REQUIRE_JS_CJS_MODULE_CACHE_H
#define ECMASCRIPT_REQUIRE_JS_CJS_MODULE_CACHE_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class CjsModuleCache : public TaggedHashTable<CjsModuleCache> {
public:
    using HashTable = TaggedHashTable<CjsModuleCache>;

    static CjsModuleCache *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return reinterpret_cast<CjsModuleCache *>(object);
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

    static inline bool IsMatch(const JSTaggedValue &fileName, const JSTaggedValue &other)
    {
        if (fileName.IsHole() || fileName.IsUndefined()) {
            return false;
        }

        auto *nameString = static_cast<EcmaString *>(fileName.GetTaggedObject());
        auto *otherString = static_cast<EcmaString *>(other.GetTaggedObject());
        return EcmaString::StringsAreEqual(nameString, otherString);
    }

    static inline uint32_t Hash(const JSTaggedValue &key)
    {
        ASSERT(key.IsString());
        EcmaString *nameStr = static_cast<EcmaString *>(key.GetTaggedObject());
        return nameStr->GetHashcode();
    }

    static const int DEFAULT_ELEMENTS_NUMBER = 64;

    static JSHandle<CjsModuleCache> Create(JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER)
    {
        return HashTable::Create(thread, numberOfElements);
    }

    inline int FindEntry(const JSTaggedValue &key)
    {
        int size = Size();
        int count = 1;
        JSTaggedValue keyValue;
        uint32_t hash = Hash(key);

        for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
            keyValue = GetKey(entry);
            if (keyValue.IsHole()) {
                continue;
            }
            if (keyValue.IsUndefined()) {
                return -1;
            }
            if (IsMatch(key, keyValue)) {
                return entry;
            }
        }
        return -1;
    }

    inline bool ContainsModule(const JSTaggedValue &key)
    {
        int entry = FindEntry(key);
        return entry != -1;
    }

    inline JSTaggedValue GetModule(const JSTaggedValue &key)
    {
        int entry = FindEntry(key);
        ASSERT(entry != -1);
        return GetValue(entry);
    }

    inline void SetEntry(const JSThread *thread, int entry,
                         const JSHandle<JSTaggedValue> &key,
                         const JSHandle<JSTaggedValue> &value)
    {
        JSTaggedValue keyValue = key.GetTaggedValue();
        JSTaggedValue valueValue = value.GetTaggedValue();
        SetKey(thread, entry, keyValue);
        SetValue(thread, entry, valueValue);
    }

    static JSHandle<CjsModuleCache> PutIfAbsent(const JSThread *thread,
                                                const JSHandle<CjsModuleCache> &dictionary,
                                                const JSHandle<JSTaggedValue> &key,
                                                const JSHandle<JSTaggedValue> &value);

    static JSHandle<CjsModuleCache> ResetModule(const JSThread *thread,
                                                const JSHandle<CjsModuleCache> &dictionary,
                                                const JSHandle<JSTaggedValue> &key,
                                                const JSHandle<JSTaggedValue> &value);
    static constexpr int ENTRY_KEY_INDEX = 0;
    static constexpr int ENTRY_VALUE_INDEX = 1;
    static constexpr int ENTRY_SIZE = 2;
    static constexpr int DEAULT_DICTIONART_CAPACITY = 4;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_REQUIRE_CJS_MODULE_CACHE_H