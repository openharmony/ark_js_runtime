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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_OBJECT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_OBJECT_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript::builtins {
enum class KeyType : uint8_t {
    STRING_TYPE = 0,
    SYMBOL_TYPE,
};

class BuiltinsObject : public base::BuiltinsBase {
public:
    // 19.1.1.1Object ( [ value ] )
    static JSTaggedValue ObjectConstructor(EcmaRuntimeCallInfo *argv);

    // 19.1.2.1Object.assign ( target, ...sources )
    static JSTaggedValue Assign(EcmaRuntimeCallInfo *argv);
    // 19.1.2.2Object.create ( O [ , Properties ] )
    static JSTaggedValue Create(EcmaRuntimeCallInfo *argv);
    // 19.1.2.3Object.defineProperties ( O, Properties )
    static JSTaggedValue DefineProperties(EcmaRuntimeCallInfo *argv);
    // 19.1.2.4Object.defineProperty ( O, P, Attributes )
    static JSTaggedValue DefineProperty(EcmaRuntimeCallInfo *argv);
    // 19.1.2.5Object.freeze ( O )
    static JSTaggedValue Freeze(EcmaRuntimeCallInfo *argv);
    // 19.1.2.6Object.getOwnPropertyDescriptor ( O, P )
    static JSTaggedValue GetOwnPropertyDescriptor(EcmaRuntimeCallInfo *argv);
    // 19.1.2.7Object.getOwnPropertyNames ( O )
    static JSTaggedValue GetOwnPropertyNames(EcmaRuntimeCallInfo *argv);
    // 19.1.2.8Object.getOwnPropertySymbols ( O )
    static JSTaggedValue GetOwnPropertySymbols(EcmaRuntimeCallInfo *argv);
    // 19.1.2.9Object.getPrototypeOf ( O )
    static JSTaggedValue GetPrototypeOf(EcmaRuntimeCallInfo *argv);
    // 19.1.2.10Object.is ( value1, value2 )
    static JSTaggedValue Is(EcmaRuntimeCallInfo *argv);
    // 19.1.2.11Object.isExtensible ( O )
    static JSTaggedValue IsExtensible(EcmaRuntimeCallInfo *argv);
    // 19.1.2.12Object.isFrozen ( O )
    static JSTaggedValue IsFrozen(EcmaRuntimeCallInfo *argv);
    // 19.1.2.13Object.isSealed ( O )
    static JSTaggedValue IsSealed(EcmaRuntimeCallInfo *argv);
    // 19.1.2.14 Object.keys(O)
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    // 19.1.2.15 Object.preventExtensions(O)
    static JSTaggedValue PreventExtensions(EcmaRuntimeCallInfo *argv);
    // 19.1.2.17 Object.seal(O)
    static JSTaggedValue Seal(EcmaRuntimeCallInfo *argv);
    // 19.1.2.18 Object.setPrototypeOf(O, proto)
    static JSTaggedValue SetPrototypeOf(EcmaRuntimeCallInfo *argv);

    // 19.1.3.2 Object.prototype.hasOwnProperty(V)
    static JSTaggedValue HasOwnProperty(EcmaRuntimeCallInfo *argv);
    // 19.1.3.3 Object.prototype.isPrototypeOf(V)
    static JSTaggedValue IsPrototypeOf(EcmaRuntimeCallInfo *argv);
    // 19.1.3.4 Object.prototype.propertyIsEnumerable(V)
    static JSTaggedValue PropertyIsEnumerable(EcmaRuntimeCallInfo *argv);
    // 19.1.3.5 Object.prototype.toLocaleString([reserved1[, reserved2]])
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 19.1.3.6 Object.prototype.toString()
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 19.1.3.7 Object.prototype.valueOf()
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue CreateRealm(EcmaRuntimeCallInfo *argv);
    // 20.1.2.5 Object.entries ( O )
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);

    // B.2.2.1 Object.prototype.__proto__
    static JSTaggedValue ProtoGetter(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ProtoSetter(EcmaRuntimeCallInfo *argv);

private:
    static JSTaggedValue ObjectDefineProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &prop);
    static JSTaggedValue GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const KeyType &type);
    static JSTaggedValue GetBuiltinTag(JSThread *thread, const JSHandle<JSObject> &object);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_OBJECT_H
