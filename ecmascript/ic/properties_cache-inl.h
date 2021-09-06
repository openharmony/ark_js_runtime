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

#ifndef PANDA_RUNTIME_ECMASCRIPT_PROPERTIES_CACHE_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_PROPERTIES_CACHE_INL_H

#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
int PropertiesCache::Get(JSHClass *jsHclass, JSTaggedValue key)
{
    int hash = Hash(jsHclass, key);
    PropertyKey &prop = keys_[hash];
    if ((prop.hclass_ == jsHclass) && (prop.key_ == key)) {
        return keys_[hash].results_;
    }
    return NOT_FOUND;
}

void PropertiesCache::Set(JSHClass *jsHclass, JSTaggedValue key, int index)
{
    int hash = Hash(jsHclass, key);
    PropertyKey &prop = keys_[hash];
    prop.hclass_ = jsHclass;
    prop.key_ = key;
    keys_[hash].results_ = index;
}

void PropertiesCache::Clear()
{
    for (auto &key : keys_) {
        key.hclass_ = nullptr;
    }
}

int PropertiesCache::Hash(JSHClass *cls, JSTaggedValue key)
{
    uint32_t clsHash = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cls)) >> 3U;  // skip 8bytes
    uint32_t keyHash = key.GetKeyHashCode();
    return static_cast<int>((clsHash ^ keyHash) & CACHE_LENGTH_MASK);
}
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_PROPERTIES_CACHE_INL_H
