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

#ifndef PANDA_RUNTIME_ECMASCRIPT_BUILTINS_ERRORS_H
#define PANDA_RUNTIME_ECMASCRIPT_BUILTINS_ERRORS_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::builtins {
class BuiltinsError : public base::BuiltinsBase {
public:
    // 19.5.1.1
    static JSTaggedValue ErrorConstructor(EcmaRuntimeCallInfo *argv);
    // 19.5.2.4
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
};

// 19.5.5.2
class BuiltinsRangeError : public base::BuiltinsBase {
public:
    static JSTaggedValue RangeErrorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
};

// 19.5.5.3
class BuiltinsReferenceError : public base::BuiltinsBase {
public:
    static JSTaggedValue ReferenceErrorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
};

// 19.5.5.5
class BuiltinsTypeError : public base::BuiltinsBase {
public:
    static JSTaggedValue TypeErrorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ThrowTypeError(EcmaRuntimeCallInfo *argv);
};

// 19.5.5.6
class BuiltinsURIError : public base::BuiltinsBase {
public:
    static JSTaggedValue URIErrorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
};

// 19.5.5.4
class BuiltinsSyntaxError : public base::BuiltinsBase {
public:
    static JSTaggedValue SyntaxErrorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
};

// 19.5.5.1
class BuiltinsEvalError : public base::BuiltinsBase {
public:
    static JSTaggedValue EvalErrorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins

#endif  // PANDA_RUNTIME_ECMASCRIPT_BUILTINS_ERRORS_H
