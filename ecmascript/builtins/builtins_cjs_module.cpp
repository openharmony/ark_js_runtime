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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_cjs_module.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsCjsModule::CjsModuleConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    LOG_ECMA(ERROR) << "BuiltinsCjsModule::CjsModuleConstructor : can not be call";
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsCjsModule::Compiler(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::Load(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::ResolveFilename(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t length = argv->GetArgsNumber();
    JSMutableHandle<JSTaggedValue> parent(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> dirname(thread, JSTaggedValue::Undefined());
    const JSPandaFile *jsPandaFile = EcmaInterpreter::GetNativeCallPandafile(thread);
    RequireManager::ResolveCurrentPath(thread, parent, dirname, jsPandaFile);

    if (length != 1) {  // strange arg's number
        LOG_ECMA(ERROR) << "BuiltinsCjsModule::Load : can only accept one argument";
        UNREACHABLE();
    }
    JSHandle<EcmaString> requestName = JSHandle<EcmaString>::Cast(GetCallArg(argv, 0));
    JSHandle<EcmaString> filename = CjsModule::ResolveFilename(thread, dirname.GetTaggedValue(),
                                                               requestName.GetTaggedValue());

    return filename.GetTaggedValue();
}

JSTaggedValue BuiltinsCjsModule::Require(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}
    
JSTaggedValue BuiltinsCjsModule::GetExportsForCircularRequire(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::UpdateChildren(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}
}  // namespace panda::ecmascript::builtins

