/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/tooling/backend/debugger_executor.h"

#include "ecmascript/tooling/backend/debugger_api.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
void DebuggerExecutor::Initialize(const EcmaVM *vm)
{
    Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
    globalObj->Set(vm, StringRef::NewFromUtf8(vm, "debuggerSetValue"), FunctionRef::New(
        const_cast<panda::EcmaVM*>(vm), DebuggerExecutor::DebuggerSetValue, nullptr));
    globalObj->Set(vm, StringRef::NewFromUtf8(vm, "debuggerGetValue"), FunctionRef::New(
        const_cast<panda::EcmaVM*>(vm), DebuggerExecutor::DebuggerGetValue, nullptr));
}

Local<JSValueRef> DebuggerExecutor::DebuggerGetValue(EcmaVM *vm, [[maybe_unused]] Local<JSValueRef> thisArg,
                                                     const Local<JSValueRef> *argv,
                                                     int32_t length, [[maybe_unused]] void *data)
{
    if (length != NUM_ARGS) {
        return JSValueRef::Undefined(vm);
    }
    Local<JSValueRef> name = argv[0];
    if (!name->IsString()) {
        return JSValueRef::Undefined(vm);
    }
    Local<JSValueRef> isThrow = argv[1];

    auto &frameHandler = vm->GetJsDebuggerManager()->GetEvalFrameHandler();
    ASSERT(frameHandler);

    Local<JSValueRef> value = GetValue(vm, frameHandler.get(), Local<StringRef>(name));
    if (!value.IsEmpty() && !value->IsException()) {
        return value;
    }

    if (!isThrow->ToBoolean(vm)->Value()) {
        DebuggerApi::ClearException(vm);
        return JSValueRef::Undefined(vm);
    }

    std::string varName = Local<StringRef>(name)->ToString();
    ThrowException(vm, varName + " is not defined");
    return JSValueRef::Exception(vm);
}

Local<JSValueRef> DebuggerExecutor::DebuggerSetValue(EcmaVM *vm, [[maybe_unused]] Local<JSValueRef> thisArg,
                                                     const Local<JSValueRef> *argv,
                                                     int32_t length, [[maybe_unused]] void *data)
{
    if (length != NUM_ARGS) {
        return JSValueRef::Undefined(vm);
    }
    Local<JSValueRef> name = argv[0];
    if (!name->IsString()) {
        return JSValueRef::Undefined(vm);
    }
    Local<JSValueRef> value = argv[1];

    auto &frameHandler = vm->GetJsDebuggerManager()->GetEvalFrameHandler();
    ASSERT(frameHandler);

    if (SetValue(vm, frameHandler.get(), Local<StringRef>(name), value)) {
        return value;
    }

    std::string varName = StringRef::Cast(*name)->ToString();
    ThrowException(vm, varName + " is not defined");
    return JSValueRef::Exception(vm);
}

Local<JSValueRef> DebuggerExecutor::GetValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                             Local<StringRef> name)
{
    Local<JSValueRef> value;
    value = GetLocalValue(vm, frameHandler, name);
    if (!value.IsEmpty()) {
        return value;
    }
    value = GetLexicalValue(vm, frameHandler, name);
    if (!value.IsEmpty()) {
        return value;
    }
    value = GetGlobalValue(vm, name);
    if (!value.IsEmpty() && !value->IsException()) {
        return value;
    }

    return Local<JSValueRef>();
}

bool DebuggerExecutor::SetValue(const EcmaVM *vm, InterpretedFrameHandler *frameHandler,
                                Local<StringRef> name, Local<JSValueRef> value)
{
    if (SetLocalValue(vm, frameHandler, name, value)) {
        return true;
    }
    if (SetLexicalValue(vm, frameHandler, name, value)) {
        return true;
    }
    if (SetGlobalValue(vm, name, value)) {
        return true;
    }

    return false;
}

void DebuggerExecutor::ThrowException(const EcmaVM *vm, const std::string &error)
{
    Local<StringRef> msg = StringRef::NewFromUtf8(vm, error.c_str());
    Local<JSValueRef> exception = Exception::ReferenceError(vm, msg);
    JSNApi::ThrowException(vm, exception);
}

Local<JSValueRef> DebuggerExecutor::GetLocalValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                                  Local<StringRef> name)
{
    Local<JSValueRef> result;

    int32_t index = DebuggerApi::GetVregIndex(frameHandler, name->ToString());
    if (index == -1) {
        return result;
    }

    result = DebuggerApi::GetVRegValue(vm, frameHandler, index);
    return result;
}

bool DebuggerExecutor::SetLocalValue(const EcmaVM *vm, InterpretedFrameHandler *frameHandler,
                                     Local<StringRef> name, Local<JSValueRef> value)
{
    std::string varName = name->ToString();
    int32_t index = DebuggerApi::GetVregIndex(frameHandler, varName);
    if (index == -1) {
        return false;
    }

    DebuggerApi::SetVRegValue(frameHandler, index, value);
    vm->GetJsDebuggerManager()->NotifyLocalScopeUpdated(varName, value);
    return true;
}

Local<JSValueRef> DebuggerExecutor::GetLexicalValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                                    Local<StringRef> name)
{
    Local<JSValueRef> result;

    auto [level, slot] = DebuggerApi::GetLevelSlot(frameHandler, name->ToString());
    if (level == -1) {
        return result;
    }

    result = DebuggerApi::GetProperties(vm, frameHandler, level, slot);
    return result;
}

bool DebuggerExecutor::SetLexicalValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                       Local<StringRef> name, Local<JSValueRef> value)
{
    std::string varName = name->ToString();
    auto [level, slot] = DebuggerApi::GetLevelSlot(frameHandler, varName);
    if (level == -1) {
        return false;
    }

    DebuggerApi::SetProperties(vm, frameHandler, level, slot, value);
    vm->GetJsDebuggerManager()->NotifyLocalScopeUpdated(varName, value);
    return true;
}

Local<JSValueRef> DebuggerExecutor::GetGlobalValue(const EcmaVM *vm, Local<StringRef> name)
{
    return DebuggerApi::GetGlobalValue(vm, name);
}

bool DebuggerExecutor::SetGlobalValue(const EcmaVM *vm, Local<StringRef> name, Local<JSValueRef> value)
{
    return DebuggerApi::SetGlobalValue(vm, name, value);
}
}  // namespace panda::ecmascript::tooling
