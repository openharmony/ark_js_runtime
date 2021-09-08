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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_WEAK_MAP_H
#define ECMASCRIPT_BUILTINS_BUILTINS_WEAK_MAP_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::builtins {
class BuiltinsWeakMap : public base::BuiltinsBase {
public:
    // 23.3.3.1
    static JSTaggedValue WeakMapConstructor(EcmaRuntimeCallInfo *argv);
    // 23.3.3.2
    static JSTaggedValue Delete(EcmaRuntimeCallInfo *argv);
    // 23.3.3.3
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    // 23.3.3.4
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    // 23.1.3.5
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    // 23.1.3.6 @@toStringTag
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_MAP_H
