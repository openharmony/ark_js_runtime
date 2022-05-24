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

#include "ecmascript/tooling/interface/debugger_api.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/class_linker/program_object-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_method.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/jsnapi_helper-inl.h"
#include "ecmascript/tooling/interface/js_debugger.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::ALLOW_BINARY;
using panda::ecmascript::base::ALLOW_HEX;
using panda::ecmascript::base::ALLOW_OCTAL;
using panda::ecmascript::base::NumberHelper;

// CString
uint64_t DebuggerApi::CStringToULL(const CString &str)
{
    return panda::ecmascript::CStringToULL(str);
}

CString DebuggerApi::ToCString(int32_t number)
{
    return panda::ecmascript::ToCString(number);
}

CString DebuggerApi::ConvertToString(const std::string &str)
{
    return panda::ecmascript::ConvertToString(str);
}

// InterpretedFrameHandler
uint32_t DebuggerApi::GetStackDepth(const EcmaVM *ecmaVm)
{
    uint32_t count = 0;
    InterpretedFrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsBreakFrame()) {
            continue;
        }
        ++count;
    }
    return count;
}

std::shared_ptr<InterpretedFrameHandler> DebuggerApi::NewFrameHandler(const EcmaVM *ecmaVm)
{
    return std::make_shared<InterpretedFrameHandler>(ecmaVm->GetJSThread());
}

bool DebuggerApi::StackWalker(const EcmaVM *ecmaVm, std::function<StackState(const InterpretedFrameHandler *)> func)
{
    InterpretedFrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsBreakFrame()) {
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
    return InterpretedFrameHandler(ecmaVm->GetJSThread()).GetBytecodeOffset();
}

JSMethod *DebuggerApi::GetMethod(const EcmaVM *ecmaVm)
{
    return InterpretedFrameHandler(ecmaVm->GetJSThread()).GetMethod();
}

void DebuggerApi::SetClosureVariables(const EcmaVM *ecmaVm, const InterpretedFrameHandler *frameHandler,
    Local<ObjectRef> &localObj)
{
    JSTaggedValue env = DebuggerApi::GetEnv(frameHandler);
    if (env.IsTaggedArray() && DebuggerApi::GetBytecodeOffset(frameHandler) != 0) {
        LexicalEnv *lexEnv = LexicalEnv::Cast(env.GetTaggedObject());
        if (lexEnv->GetScopeInfo().IsHole()) {
            return;
        }
        auto ptr = JSNativePointer::Cast(lexEnv->GetScopeInfo().GetTaggedObject())->GetExternalPointer();
        auto *scopeDebugInfo = reinterpret_cast<ScopeDebugInfo *>(ptr);
        JSThread *thread = ecmaVm->GetJSThread();
        for (const auto &scopeInfo : scopeDebugInfo->scopeInfo) {
            if (scopeInfo.name == "this" || scopeInfo.name == "4newTarget") {
                continue;
            }
            Local<JSValueRef> name = StringRef::NewFromUtf8(ecmaVm, scopeInfo.name.c_str());
            Local<JSValueRef> value = JSNApiHelper::ToLocal<JSValueRef>(
                JSHandle<JSTaggedValue>(thread, lexEnv->GetProperties(scopeInfo.slot)));
            PropertyAttribute descriptor(value, true, true, true);
            localObj->DefineProperty(ecmaVm, name, descriptor);
        }
    }
}

Local<JSValueRef> DebuggerApi::GetVRegValue(const EcmaVM *ecmaVm, size_t index)
{
    auto value = InterpretedFrameHandler(ecmaVm->GetJSThread()).GetVRegValue(index);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

void DebuggerApi::SetVRegValue(const EcmaVM *ecmaVm, size_t index, Local<JSValueRef> value)
{
    return InterpretedFrameHandler(ecmaVm->GetJSThread()).SetVRegValue(index, JSNApiHelper::ToJSTaggedValue(*value));
}

uint32_t DebuggerApi::GetBytecodeOffset(const InterpretedFrameHandler *frameHandler)
{
    return frameHandler->GetBytecodeOffset();
}

JSMethod *DebuggerApi::GetMethod(const InterpretedFrameHandler *frameHandler)
{
    return frameHandler->GetMethod();
}

JSTaggedValue DebuggerApi::GetEnv(const InterpretedFrameHandler *frameHandler)
{
    return frameHandler->GetEnv();
}

JSTaggedType *DebuggerApi::GetSp(const InterpretedFrameHandler *frameHandler)
{
    return frameHandler->GetSp();
}

Local<JSValueRef> DebuggerApi::GetVRegValue(const EcmaVM *ecmaVm,
    const InterpretedFrameHandler *frameHandler, size_t index)
{
    auto value = frameHandler->GetVRegValue(index);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
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

// EcmaVM
const panda_file::File *DebuggerApi::FindPandaFile(const EcmaVM *ecmaVm, const CString &fileName)
{
    const panda_file::File *pfs = nullptr;
    EcmaVM::GetJSPandaFileManager()->EnumerateJSPandaFiles([&pfs, fileName](
        const panda::ecmascript::JSPandaFile *jsPandaFile) {
        if (jsPandaFile->GetJSPandaFileDesc() == fileName) {
            pfs = jsPandaFile->GetPandaFile();
            return false;
        }
        return true;
    });

    return pfs;
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

bool DebuggerApi::SetBreakpoint(JSDebugger *debugger, const JSPtLocation &location)
{
    return debugger->SetBreakpoint(location);
}

bool DebuggerApi::RemoveBreakpoint(JSDebugger *debugger, const JSPtLocation &location)
{
    return debugger->RemoveBreakpoint(location);
}

// JSMethod
CString DebuggerApi::ParseFunctionName(const JSMethod *method)
{
    return method->ParseFunctionName();
}

// ScopeInfo
Local<JSValueRef> DebuggerApi::GetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot)
{
    JSTaggedValue env = GetCurrentEvaluateEnv(ecmaVm);
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    JSTaggedValue value = LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

void DebuggerApi::SetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot, Local<JSValueRef> value)
{
    JSTaggedValue env = GetCurrentEvaluateEnv(ecmaVm);
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env = taggedParentEnv;
    }
    JSTaggedValue target = JSNApiHelper::ToJSHandle(value).GetTaggedValue();
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(ecmaVm->GetJSThread(), slot, target);
}

bool DebuggerApi::EvaluateLexicalValue(const EcmaVM *ecmaVm, const CString &name, int32_t &level, uint32_t &slot)
{
    JSTaggedValue curEnv = GetCurrentEvaluateEnv(ecmaVm);
    for (; curEnv.IsTaggedArray(); curEnv = LexicalEnv::Cast(curEnv.GetTaggedObject())->GetParentEnv(), level++) {
        LexicalEnv *lexicalEnv = LexicalEnv::Cast(curEnv.GetTaggedObject());
        if (lexicalEnv->GetScopeInfo().IsHole()) {
            continue;
        }
        auto result = JSNativePointer::Cast(lexicalEnv->GetScopeInfo().GetTaggedObject())->GetExternalPointer();
        ScopeDebugInfo *scopeDebugInfo = reinterpret_cast<ScopeDebugInfo *>(result);
        for (const auto &info : scopeDebugInfo->scopeInfo) {
            if (info.name == name) {
                slot = info.slot;
                return true;
            }
        }
    }
    return false;
}

Local<JSValueRef> DebuggerApi::GetLexicalValueInfo(const EcmaVM *ecmaVm, const CString &name)
{
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue curEnv = thread->GetCurrentLexenv();
    for (; curEnv.IsTaggedArray(); curEnv = LexicalEnv::Cast(curEnv.GetTaggedObject())->GetParentEnv()) {
        LexicalEnv *lexicalEnv = LexicalEnv::Cast(curEnv.GetTaggedObject());
        if (lexicalEnv->GetScopeInfo().IsHole()) {
            continue;
        }
        void *pointer = JSNativePointer::Cast(lexicalEnv->GetScopeInfo().GetTaggedObject())->GetExternalPointer();
        ScopeDebugInfo *scopeDebugInfo = static_cast<ScopeDebugInfo *>(pointer);
        for (const auto &info : scopeDebugInfo->scopeInfo) {
            if (info.name == name) {
                uint16_t slot = info.slot;
                JSTaggedValue value = lexicalEnv->GetProperties(slot);
                JSHandle<JSTaggedValue> handledValue(thread, value);
                return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
            }
        }
    }
    JSHandle<JSTaggedValue> handledValue(thread, JSTaggedValue::Hole());
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

void DebuggerApi::InitJSDebugger(JSDebugger *debugger)
{
    debugger->Init();
}

bool DebuggerApi::HandleUncaughtException(const EcmaVM *ecmaVm, CString &message)
{
    JSThread *thread = ecmaVm->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    if (!ecmaVm->GetJSThread()->HasPendingException()) {
        return false;
    }

    JSHandle<JSTaggedValue> exHandle(thread, thread->GetException());
    if (exHandle->IsJSError()) {
        JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
        JSHandle<EcmaString> name(JSObject::GetProperty(thread, exHandle, nameKey).GetValue());
        JSHandle<JSTaggedValue> msgKey = thread->GlobalConstants()->GetHandledMessageString();
        JSHandle<EcmaString> msg(JSObject::GetProperty(thread, exHandle, msgKey).GetValue());
        message = ecmascript::ConvertToString(*name) + ": " + ecmascript::ConvertToString(*msg);
    } else {
        [[maybe_unused]] JSHandle<EcmaString> ecmaStr = JSTaggedValue::ToString(thread, exHandle);
        message = ecmascript::ConvertToString(*ecmaStr);
    }
    thread->ClearException();
    return true;
}

JSTaggedValue DebuggerApi::GetCurrentEvaluateEnv(const EcmaVM *ecmaVm)
{
    auto &frameHandler = ecmaVm->GetJsDebuggerManager()->GetEvalFrameHandler();
    if (frameHandler != nullptr) {
        return frameHandler->GetEnv();
    }
    return ecmaVm->GetJSThread()->GetCurrentLexenv();
}

Local<FunctionRef> DebuggerApi::GenerateFuncFromBuffer(const EcmaVM *ecmaVm, const void *buffer,
                                                       size_t size)
{
    JSPandaFileManager *mgr = EcmaVM::GetJSPandaFileManager();
    const auto *jsPandaFile = mgr->LoadBufferAbc("", buffer, size);
    if (jsPandaFile == nullptr) {
        LOG(ERROR, DEBUGGER) << "GenerateFuncFromBuffer: null jsPandaFile";
        return JSValueRef::Undefined(ecmaVm);
    }

    JSHandle<Program> program = mgr->GenerateProgram(const_cast<EcmaVM *>(ecmaVm), jsPandaFile);
    JSTaggedValue func = program->GetMainFunction();
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(ecmaVm->GetJSThread(), func));
}

Local<JSValueRef> DebuggerApi::EvaluateViaFuncCall(EcmaVM *ecmaVm, const Local<FunctionRef> &funcRef,
    std::shared_ptr<InterpretedFrameHandler> &frameHandler)
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
