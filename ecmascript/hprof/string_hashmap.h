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

#ifndef RUNTIME_ECMASCRIPT_HPROF_STRING_HASHMAP_H
#define RUNTIME_ECMASCRIPT_HPROF_STRING_HASHMAP_H

#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "os/mem.h"

namespace panda::ecmascript {
using StringKey = uint64_t;
using StringId = uint64_t;

// An Implementation for Native StringTable without Auto Mem-Management
// To make sure when using String, it still stays where it was.
class StringHashMap {
public:
    explicit StringHashMap(CAddressAllocator<JSTaggedType> *allocator) : allocator_(allocator)
    {
        ASSERT(allocator_ != nullptr);
    }
    ~StringHashMap()
    {
        Clear();
    }
    NO_MOVE_SEMANTIC(StringHashMap);
    NO_COPY_SEMANTIC(StringHashMap);
    /*
     * The ID is the seat number in JSON file Range from 0~string_table_.size()
     */
    StringId GetStringId(CString *string);
    /*
     * Get all keys sorted by insert order
     */
    CVector<StringKey> *GetOrderedKeyStorage()
    {
        return &orderedKey_;
    }
    /*
     * Get string by its hash key
     */
    CString *GetStringByKey(StringKey key);
    size_t GetCapcity()
    {
        ASSERT(orderedKey_.size() == hashmap_.size());
        return orderedKey_.size();
    }
    /*
     * For external call to use this StringTable
     */
    CString *GetString(CString as);

private:
    StringKey GenerateStringKey(CString *string);
    CString *FindOrInsertString(CString *string);
    CString *FormatString(CString *string);
    /*
     * Free all memory
     */
    void Clear();
    CAddressAllocator<JSTaggedType> *allocator_{nullptr};
    CVector<StringKey> orderedKey_;  // Used for Serialize Order
    size_t index_{2};                       // 2: Offset the String-Table Header
    CUnorderedMap<StringKey, StringId> indexMap_;
    CUnorderedMap<StringKey, CString *> hashmap_;
};
}  // namespace panda::ecmascript
#endif  // RUNTIME_ECMASCRIPT_HPROF_STRING_HASHMAP_H
