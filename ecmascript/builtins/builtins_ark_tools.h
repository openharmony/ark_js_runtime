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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ARK_TOOLS_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ARK_TOOLS_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::builtins {
class BuiltinsArkTools : public base::BuiltinsBase {
public:
    // Make sure the ECMASCRIPT_OBJECT_DUMP in config.h has been opened before use it
    // Use through arkTools.print(msg, [obj1, obj2, ... objn]) in js
    static JSTaggedValue ObjectDump(EcmaRuntimeCallInfo *msg);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ARK_TOOLS_H
