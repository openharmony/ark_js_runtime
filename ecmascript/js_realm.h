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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JSREALM_H
#define PANDA_RUNTIME_ECMASCRIPT_JSREALM_H

#include "js_global_object.h"

namespace panda::ecmascript {
class JSRealm : public JSObject {
public:
    static JSRealm *Cast(ObjectHeader *object)
    {
        return static_cast<JSRealm *>(object);
    }

    static constexpr size_t VALUE_OFFSET = ObjectHeaderSize();
    ACCESSORS(Value, VALUE_OFFSET, GLOBAL_ENV_OFFSET)
    ACCESSORS(GlobalEnv, GLOBAL_ENV_OFFSET, SIZE)
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JSREALM_H