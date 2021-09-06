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

#ifndef PANDA_RUNTIME_ECMASCRIPT_BUILTINS_PROXY_H
#define PANDA_RUNTIME_ECMASCRIPT_BUILTINS_PROXY_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript::builtins {
class BuiltinsProxy : public base::BuiltinsBase {
public:
    // 26.2.1.1 Proxy( [ value ] )
    static JSTaggedValue ProxyConstructor(EcmaRuntimeCallInfo *argv);

    // 26.2.2.1 Proxy.revocable ( target, handler )
    static JSTaggedValue Revocable(EcmaRuntimeCallInfo *argv);

    // A Proxy revocation function to invalidate a specific Proxy object
    static JSTaggedValue InvalidateProxyFunction(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // PANDA_RUNTIME_ECMASCRIPT_H
