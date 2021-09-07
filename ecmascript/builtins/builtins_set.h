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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SET_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SET_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::builtins {
class BuiltinsSet : public base::BuiltinsBase {
public:
    // 23.2.1.1
    static JSTaggedValue SetConstructor(EcmaRuntimeCallInfo *argv);
    // 23.2.2.2
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    // 23.2.3.1
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    // 23.2.3.2
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    // 23.2.3.4
    static JSTaggedValue Delete(EcmaRuntimeCallInfo *argv);
    // 23.2.3.5
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    // 23.2.3.6
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    // 23.2.3.7
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    // 23.2.3.9
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    // 23.2.3.10
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SET_H
