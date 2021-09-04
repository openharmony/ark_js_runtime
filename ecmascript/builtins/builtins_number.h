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

#ifndef PANDA_RUNTIME_ECMASCRIPT_BUILTINS_NUMBER_H
#define PANDA_RUNTIME_ECMASCRIPT_BUILTINS_NUMBER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::builtins {
class BuiltinsNumber : public base::BuiltinsBase {
public:
    // 20.1.1.1
    static JSTaggedValue NumberConstructor(EcmaRuntimeCallInfo *argv);

    // 20.1.2.2
    static JSTaggedValue IsFinite(EcmaRuntimeCallInfo *argv);
    // 20.1.2.3
    static JSTaggedValue IsInteger(EcmaRuntimeCallInfo *argv);
    // 20.1.2.4
    static JSTaggedValue IsNaN(EcmaRuntimeCallInfo *argv);
    // 20.1.2.5
    static JSTaggedValue IsSafeInteger(EcmaRuntimeCallInfo *argv);
    // 20.1.2.12
    static JSTaggedValue ParseFloat(EcmaRuntimeCallInfo *argv);
    // 20.1.2.13
    static JSTaggedValue ParseInt(EcmaRuntimeCallInfo *argv);

    // prototype
    // 20.1.3.2
    static JSTaggedValue ToExponential(EcmaRuntimeCallInfo *argv);
    // 20.1.3.3
    static JSTaggedValue ToFixed(EcmaRuntimeCallInfo *argv);
    // 20.1.3.4
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 20.1.3.5
    static JSTaggedValue ToPrecision(EcmaRuntimeCallInfo *argv);
    // 20.1.3.6
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 20.1.3.7
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);

private:
    static JSTaggedNumber ThisNumberValue(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // PANDA_RUNTIME_ECMASCRIPT_NUBMER_H
