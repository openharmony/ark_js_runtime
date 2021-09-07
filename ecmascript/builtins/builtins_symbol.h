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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SYMBOL_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SYMBOL_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::builtins {
class BuiltinsSymbol : public base::BuiltinsBase {
public:
    // 19.4.1
    static JSTaggedValue SymbolConstructor(EcmaRuntimeCallInfo *argv);

    // prototype
    // 19.4.3.2
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 19.4.3.3
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);
    // 19.4.2.1 Symbol.for (key)
    static JSTaggedValue For(EcmaRuntimeCallInfo *argv);
    // 19.4.2.5 Symbol.keyFor (sym)
    static JSTaggedValue KeyFor(EcmaRuntimeCallInfo *argv);

    // 19.4.3.2 get Symbol.prototype.description
    static JSTaggedValue DescriptionGetter(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ThisSymbolValue(JSThread *thread, const JSHandle<JSTaggedValue> &value);

    // 19.4.3.4
    static JSTaggedValue ToPrimitive(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue SymbolDescriptiveString(JSThread *thread, JSTaggedValue sym);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SYMBOL_H
