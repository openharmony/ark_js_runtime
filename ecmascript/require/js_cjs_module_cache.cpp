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

#include "js_cjs_module_cache.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda::ecmascript {
JSHandle<CjsModuleCache> CjsModuleCache::PutIfAbsent(const JSThread *thread,
                                                     const JSHandle<CjsModuleCache> &dictionary,
                                                     const JSHandle<JSTaggedValue> &key,
                                                     const JSHandle<JSTaggedValue> &value)
{
    int hash = Hash(key.GetTaggedValue());

    int entry = dictionary->FindEntry(key.GetTaggedValue());
    if (entry != -1) {
        return dictionary;
    }

    // Check whether the dictionary should be extended.
    JSHandle<CjsModuleCache> newDictionary(HashTable::GrowHashTable(thread, dictionary));

    // Compute the key object.
    entry = newDictionary->FindInsertIndex(hash);
    newDictionary->SetEntry(thread, entry, key, value);
    newDictionary->IncreaseEntries(thread);

    return newDictionary;    
}
JSHandle<CjsModuleCache> CjsModuleCache::ResetModule(const JSThread *thread,
                                                     const JSHandle<CjsModuleCache> &dictionary,
                                                     const JSHandle<JSTaggedValue> &key,
                                                     const JSHandle<JSTaggedValue> &value)
{
    int entry = dictionary->FindEntry(key.GetTaggedValue());
    ASSERT(entry != -1);

    dictionary->SetEntry(thread, entry, key, value);
    return dictionary;
}
}  // namespace panda::ecmascript