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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_REFLECT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_REFLECT_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_array.h"

namespace panda::ecmascript::builtins {
class BuiltinsReflect : public base::BuiltinsBase {
public:
    // ecma 26.1.1
    static JSTaggedValue ReflectApply(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.2
    static JSTaggedValue ReflectConstruct(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.3
    static JSTaggedValue ReflectDefineProperty(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.4
    static JSTaggedValue ReflectDeleteProperty(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.5
    static JSTaggedValue ReflectGet(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.6
    static JSTaggedValue ReflectGetOwnPropertyDescriptor(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.7
    static JSTaggedValue ReflectGetPrototypeOf(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.8
    static JSTaggedValue ReflectHas(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.9
    static JSTaggedValue ReflectIsExtensible(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.10
    static JSTaggedValue ReflectOwnKeys(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.11
    static JSTaggedValue ReflectPreventExtensions(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.12
    static JSTaggedValue ReflectSet(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.13
    static JSTaggedValue ReflectSetPrototypeOf(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_REFLECT_H
