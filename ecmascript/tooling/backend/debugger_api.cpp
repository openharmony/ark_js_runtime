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

#include "ecmascript/tooling/backend/debugger_api.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_method.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tooling/backend/js_debugger.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::ALLOW_BINARY;
using panda::ecmascript::base::ALLOW_HEX;
using panda::ecmascript::base::ALLOW_OCTAL;
using panda::ecmascript::base::NumberHelper;

// FrameHandler
uint32_t DebuggerApi::GetStackDepth(const EcmaVM *ecmaVm)
{
    uint32_t count = 0;
    FrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        ++count;
    }
    return count;
}

std::shared_ptr<FrameHandler> DebuggerApi::NewFrameHandler(const EcmaVM *ecmaVm)
{
    return std::make_shared<FrameHandler>(ecmaVm->GetJSThread());
}

bool DebuggerApi::StackWalker(const EcmaVM *ecmaVm, std::function<StackState(const FrameHandler *)> func)
{
    FrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        StackState state = func(&frameHandler);
        if (state == StackState::CONTINUE) {
            continue;
        }
        if (state == StackState::FAILED) {
            return false;
        }
        return true;
    }
    return true;
}

uint32_t DebuggerApi::GetBytecodeOffset(const EcmaVM *ecmaVm)
{
    return FrameHandler(ecmaVm->GetJSThread()).GetBytecodeOffset();
}

JSMethod *DebuggerApi::GetMethod(const EcmaVM *ecmaVm)
{
    return FrameHandler(ecmaVm->GetJSThread()).GetMethod();
}

void DebuggerApi::SetVRegValue(FrameHandler *frameHandler, size_t index, Local<JSValueRef> value)
{
    return frameHandler->SetVRegValue(index, JSNApiHelper::ToJSTaggedValue(*value));
}

uint32_t DebuggerApi::GetBytecodeOffset(const FrameHandler *frameHandler)
{
    return frameHandler->GetBytecodeOffset();
}

JSMethod *DebuggerApi::GetMethod(const FrameHandler *frameHandler)
{
    return frameHandler->GetMethod();
}

JSTaggedValue DebuggerApi::GetEnv(const FrameHandler *frameHandler)
{
    return frameHandler->GetEnv();
}

JSTaggedType *DebuggerApi::GetSp(const FrameHandler *frameHandler)
{
    return frameHandler->GetSp();
}

int32_t DebuggerApi::GetVregIndex(const FrameHandler *frameHandler, std::string_view name)
{
    JSMethod *method = frameHandler->GetMethod();
    if (method->IsNativeWithCallField()) {
        LOG(ERROR, DEBUGGER) << "GetVregIndex: native frame not support";
        return -1;
    }
    JSPtExtractor *extractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GetVregIndex: extractor is null";
        return -1;
    }
    auto table = extractor->GetLocalVariableTable(method->GetMethodId());
    auto iter = table.find(name.data());
    if (iter == table.end()) {
        return -1;
    }
    return iter->second;
}

Local<JSValueRef> DebuggerApi::GetVRegValue(const EcmaVM *ecmaVm,
    const FrameHandler *frameHandler, size_t index)
{
    auto value = frameHandler->GetVRegValue(index);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

std::string DebuggerApi::ToStdString(Local<JSValueRef> str)
{
    ecmascript::JSHandle<ecmascript::JSTaggedValue> ret = JSNApiHelper::ToJSHandle(str);
    ASSERT(ret->IsString());
    EcmaString *ecmaStr = EcmaString::Cast(ret.GetTaggedValue().GetTaggedObject());
    return CstringConvertToStdString(ConvertToString(ecmaStr));
}

int32_t DebuggerApi::StringToInt(Local<JSValueRef> str)
{
    return std::stoi(ToStdString(str));
}

// JSThread
Local<JSValueRef> DebuggerApi::GetAndClearException(const EcmaVM *ecmaVm)
{
    auto exception = ecmaVm->GetJSThread()->GetException();
    JSHandle<JSTaggedValue> handledException(ecmaVm->GetJSThread(), exception);
    ecmaVm->GetJSThread()->ClearException();
    return JSNApiHelper::ToLocal<JSValueRef>(handledException);
}

void DebuggerApi::SetException(const EcmaVM *ecmaVm, Local<JSValueRef> exception)
{
    ecmaVm->GetJSThread()->SetException(JSNApiHelper::ToJSTaggedValue(*exception));
}

void DebuggerApi::ClearException(const EcmaVM *ecmaVm)
{
    return ecmaVm->GetJSThread()->ClearException();
}

// NumberHelper
double DebuggerApi::StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix)
{
    return NumberHelper::StringToDouble(start, end, radix, ALLOW_BINARY | ALLOW_HEX | ALLOW_OCTAL);
}

// JSDebugger
JSDebugger *DebuggerApi::CreateJSDebugger(const EcmaVM *ecmaVm)
{
    return new JSDebugger(ecmaVm);
}

void DebuggerApi::DestroyJSDebugger(JSDebugger *debugger)
{
    delete debugger;
}

void DebuggerApi::RegisterHooks(JSDebugger *debugger, PtHooks *hooks)
{
    debugger->RegisterHooks(hooks);
}

bool DebuggerApi::SetBreakpoint(JSDebugger *debugger, const JSPtLocation &location,
    const Local<FunctionRef> &condFuncRef)
{
    return debugger->SetBreakpoint(location, condFuncRef);
}

bool DebuggerApi::RemoveBreakpoint(JSDebugger *debugger, const JSPtLocation &location)
{
    return debugger->RemoveBreakpoint(location);
}

// JSMethod
std::string DebuggerApi::ParseFunctionName(const JSMethod *method)
{
    return method->ParseFunctionName();
}

// ScopeInfo
Local<JSValueRef> DebuggerApi::GetProperties(const EcmaVM *vm, const FrameHandler *frameHandler,
                                             int32_t level, uint32_t slot)
{
    JSTaggedValue env = frameHandler->GetEnv();
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    JSTaggedValue value = LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot);
    JSHandle<JSTaggedValue> handledValue(vm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

void DebuggerApi::SetProperties(const EcmaVM *vm, const FrameHandler *frameHandler,
                                int32_t level, uint32_t slot, Local<JSValueRef> value)
{
    JSTaggedValue env = frameHandler->GetEnv();
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    JSTaggedValue target = JSNApiHelper::ToJSHandle(value).GetTaggedValue();
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(vm->GetJSThread(), slot, target);
}

std::pair<int32_t, uint32_t> DebuggerApi::GetLevelSlot(const FrameHandler *frameHandler, std::string_view name)
{
    int32_t level = 0;
    uint32_t slot = 0;
    JSTaggedValue curEnv = frameHandler->GetEnv();
    for (; curEnv.IsTaggedArray(); curEnv = LexicalEnv::Cast(curEnv.GetTaggedObject())->GetParentEnv(), level++) {
        LexicalEnv *lexicalEnv = LexicalEnv::Cast(curEnv.GetTaggedObject());
        if (lexicalEnv->GetScopeInfo().IsHole()) {
            continue;
        }
        auto result = JSNativePointer::Cast(lexicalEnv->GetScopeInfo().GetTaggedObject())->GetExternalPointer();
        ScopeDebugInfo *scopeDebugInfo = reinterpret_cast<ScopeDebugInfo *>(result);
        auto iter = scopeDebugInfo->scopeInfo.find(name.data());
        if (iter == scopeDebugInfo->scopeInfo.end()) {
            continue;
        }
        slot = iter->second;
        return std::make_pair(level, slot);
    }
    return std::make_pair(-1, 0);
}

Local<JSValueRef> DebuggerApi::GetGlobalValue(const EcmaVM *vm, Local<StringRef> name)
{
    JSTaggedValue result;
    JSTaggedValue globalObj = vm->GetGlobalEnv()->GetGlobalObject();
    JSThread *thread = vm->GetJSThread();

    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSTaggedValue globalRec = SlowRuntimeStub::LdGlobalRecord(thread, key);
    if (!globalRec.IsUndefined()) {
        ASSERT(globalRec.IsPropertyBox());
        result = PropertyBox::Cast(globalRec.GetTaggedObject())->GetValue();
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, result));
    }

    JSTaggedValue globalVar = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    if (!globalVar.IsHole()) {
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, globalVar));
    } else {
        result = SlowRuntimeStub::TryLdGlobalByName(thread, globalObj, key);
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, result));
    }

    return JSValueRef::Exception(vm);
}

bool DebuggerApi::SetGlobalValue(const EcmaVM *vm, Local<StringRef> name, Local<JSValueRef> value)
{
    JSTaggedValue result;
    JSTaggedValue globalObj = vm->GetGlobalEnv()->GetGlobalObject();
    JSThread *thread = vm->GetJSThread();

    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSTaggedValue newVal = JSNApiHelper::ToJSTaggedValue(*value);
    JSTaggedValue globalRec = SlowRuntimeStub::LdGlobalRecord(thread, key);
    if (!globalRec.IsUndefined()) {
        result = SlowRuntimeStub::TryUpdateGlobalRecord(thread, key, newVal);
        return !result.IsException();
    }

    JSTaggedValue globalVar = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    if (!globalVar.IsHole()) {
        result = SlowRuntimeStub::StGlobalVar(thread, key, newVal);
        return !result.IsException();
    }

    return false;
}

void DebuggerApi::HandleUncaughtException(const EcmaVM *ecmaVm, std::string &message)
{
    JSThread *thread = ecmaVm->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> exHandle(thread, thread->GetException());
    if (exHandle->IsJSError()) {
        JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
        JSHandle<EcmaString> name(JSObject::GetProperty(thread, exHandle, nameKey).GetValue());
        JSHandle<JSTaggedValue> msgKey = globalConst->GetHandledMessageString();
        JSHandle<EcmaString> msg(JSObject::GetProperty(thread, exHandle, msgKey).GetValue());
        message = ConvertToString(*name) + ": " + ConvertToString(*msg);
    } else {
        JSHandle<EcmaString> ecmaStr = JSTaggedValue::ToString(thread, exHandle);
        message = ConvertToString(*ecmaStr);
    }
    thread->ClearException();
}

Local<FunctionRef> DebuggerApi::GenerateFuncFromBuffer(const EcmaVM *ecmaVm, const void *buffer,
                                                       size_t size, std::string_view entryPoint)
{
    JSPandaFileManager *mgr = JSPandaFileManager::GetInstance();
    const auto *jsPandaFile = mgr->LoadJSPandaFile("", entryPoint, buffer, size);
    if (jsPandaFile == nullptr) {
        return JSValueRef::Undefined(ecmaVm);
    }

    JSHandle<Program> program = mgr->GenerateProgram(const_cast<EcmaVM *>(ecmaVm), jsPandaFile);
    JSTaggedValue func = program->GetMainFunction();
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(ecmaVm->GetJSThread(), func));
}

Local<JSValueRef> DebuggerApi::EvaluateViaFuncCall(EcmaVM *ecmaVm, const Local<FunctionRef> &funcRef,
    std::shared_ptr<FrameHandler> &frameHandler)
{
    JSNApi::EnableUserUncaughtErrorHandler(ecmaVm);

    JsDebuggerManager *mgr = ecmaVm->GetJsDebuggerManager();
    bool prevDebugMode = mgr->IsDebugMode();
    mgr->SetEvalFrameHandler(frameHandler);
    mgr->SetDebugMode(false); // in order to catch exception
    std::vector<Local<JSValueRef>> args;
    auto result = funcRef->Call(ecmaVm, JSValueRef::Undefined(ecmaVm), args.data(), args.size());
    mgr->SetDebugMode(prevDebugMode);
    mgr->SetEvalFrameHandler(nullptr);

    return result;
}
}  // namespace panda::ecmascript::tooling
