/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_CJS_EXPORTS_H
#define ECMASCRIPT_BUILTINS_BUILTINS_CJS_EXPORTS_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/require/js_cjs_exports.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::builtins {
class BuiltinsCjsExports : public base::BuiltinsBase {
public:
    static JSTaggedValue CjsExportsConstructor(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_CJS_EXPORTS_H
