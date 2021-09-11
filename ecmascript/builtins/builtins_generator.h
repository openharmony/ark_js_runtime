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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_GENERATOR_H
#define ECMASCRIPT_BUILTINS_BUILTINS_GENERATOR_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript::builtins {
class BuiltinsGenerator : public base::BuiltinsBase {
public:
    // 26.2.1.1 GeneratorFunction(p1, p2, … , pn, body)
    static JSTaggedValue GeneratorFunctionConstructor(EcmaRuntimeCallInfo *argv);

    // 26.4.1.2 Generator.prototype.next(value)
    static JSTaggedValue GeneratorPrototypeNext(EcmaRuntimeCallInfo *argv);

    // 26.4.1.3 Generator.prototype.return(value)
    static JSTaggedValue GeneratorPrototypeReturn(EcmaRuntimeCallInfo *argv);

    // 26.4.1.4 Generator.prototype.throw(exception)
    static JSTaggedValue GeneratorPrototypeThrow(EcmaRuntimeCallInfo *argv);

    // 26.4.1.5 Generator.prototype[@@toStringTag]
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_GENERATOR_H
