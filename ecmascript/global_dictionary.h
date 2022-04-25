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

#ifndef ECMASCRIPT_GLOBAL_DICTIONARY_H
#define ECMASCRIPT_GLOBAL_DICTIONARY_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda {
namespace ecmascript {
class GlobalDictionary : public OrderTaggedHashTable<GlobalDictionary> {
public:
    using OrderHashTableT = OrderTaggedHashTable<GlobalDictionary>;
    inline static int GetKeyIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_KEY_INDEX;
    }
    inline static int GetValueIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_VALUE_INDEX;
    }
    inline static int GetEntryIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize();
    }
    inline static int GetEntrySize()
    {
        return ENTRY_SIZE;
    }
    static inline bool IsMatch(const JSTaggedValue &key, const JSTaggedValue &other);

    static inline int Hash(const JSTaggedValue &key);
    inline static void InvalidatePropertyBox(JSThread *thread, const JSHandle<GlobalDictionary> &dictHandle, int entry,
                                             const PropertyAttributes &metaData);

    inline static void InvalidateAndReplaceEntry(JSThread *thread, const JSHandle<GlobalDictionary> &dictHandle,
                                                 int entry, const JSHandle<JSTaggedValue> &oldValue);

    inline PropertyBox *GetBox(int entry) const;

    inline JSTaggedValue GetValue(int entry) const;

    inline PropertyAttributes GetAttributes(int entry) const;

    inline void SetEntry(const JSThread *thread, int entry, const JSTaggedValue &key, const JSTaggedValue &value,
                         const PropertyAttributes &detail);

    inline void ClearEntry([[maybe_unused]] const JSThread *thread, int entry);

    inline void UpdateValueAndAttributes(const JSThread *thread, int entry, const JSTaggedValue &value,
                                         const PropertyAttributes &metaData);

    inline void UpdateValue(const JSThread *thread, int entry, const JSTaggedValue &value);

    inline void SetAttributes(const JSThread *thread, int entry, const PropertyAttributes &metaData);

    inline void GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const;

    inline void GetEnumAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray, uint32_t *keys) const;

    static bool inline CompKey(const std::pair<JSTaggedValue, uint32_t> &a,
                               const std::pair<JSTaggedValue, uint32_t> &b);

    DECL_DUMP()

public:
    static constexpr int ENTRY_KEY_INDEX = 0;
    static constexpr int ENTRY_VALUE_INDEX = 1;
    static constexpr int ENTRY_DETAILS_INDEX = 2;
    static constexpr int ENTRY_SIZE = 3;
};
}  // namespace ecmascript
}  // namespace panda
#endif
