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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_BIGINT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_BIGINT_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::builtins {
class BuiltinsBigInt : public base::BuiltinsBase {
public:
    // 21.2.1.1
    static JSTaggedValue BigIntConstructor(EcmaRuntimeCallInfo *argv);
    // 21.2.2.1
    static JSTaggedValue AsUintN(EcmaRuntimeCallInfo *argv);
    // 21.2.2.2
    static JSTaggedValue AsIntN(EcmaRuntimeCallInfo *argv);
    // 21.2.3.2
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 21.2.3.3
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 21.2.3.4
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);
private:
    static JSTaggedValue ThisBigIntValue(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_BIGINT_H