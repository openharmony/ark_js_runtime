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

#ifndef PANDA_RUNTIME_ECMASCRIPT_PROPERTIES_CACHE_H
#define PANDA_RUNTIME_ECMASCRIPT_PROPERTIES_CACHE_H

#include <array>

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_macros.h"

namespace panda::ecmascript {
class EcmaVM;
class PropertiesCache {
public:
    inline int Get(JSHClass *jsHclass, JSTaggedValue key);
    inline void Set(JSHClass *jsHclass, JSTaggedValue key, int index);
    inline void Clear();

    static const int NOT_FOUND = -1;

private:
    PropertiesCache()
    {
        for (uint32_t i = 0; i < CACHE_LENGTH; ++i) {
            keys_[i].hclass_ = nullptr;
            keys_[i].key_ = JSTaggedValue::Hole();
            keys_[i].results_ = NOT_FOUND;
        }
    }
    ~PropertiesCache() = default;

    struct PropertyKey {
        JSHClass *hclass_{nullptr};
        JSTaggedValue key_{JSTaggedValue::Hole()};
        int results_{NOT_FOUND};
    };

    static inline int Hash(JSHClass *cls, JSTaggedValue key);

    static const uint32_t CACHE_LENGTH_BIT = 10;
    static const uint32_t CACHE_LENGTH = (1U << CACHE_LENGTH_BIT);
    static const uint32_t CACHE_LENGTH_MASK = CACHE_LENGTH - 1;

    std::array<PropertyKey, CACHE_LENGTH> keys_{};

    friend class EcmaVM;
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_PROPERTIES_CACHE_H
