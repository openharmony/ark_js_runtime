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
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_method.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tooling/interface/js_debugger.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CStringToL;
using panda::ecmascript::EcmaString;
using panda::ecmascript::JSHandle;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSNativePointer;
using panda::ecmascript::LexicalEnv;
using panda::ecmascript::ScopeDebugInfo;
using panda::ecmascript::Program;
using panda::ecmascript::base::ALLOW_BINARY;
using panda::ecmascript::base::ALLOW_HEX;
using panda::ecmascript::base::ALLOW_OCTAL;
using panda::ecmascript::base::NumberHelper;

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

Local<JSValueRef> DebuggerApi::GetVRegValue(const EcmaVM *ecmaVm,
    const InterpretedFrameHandler *frameHandler, size_t index)
{
    auto value = frameHandler->GetVRegValue(index);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

CString DebuggerApi::ToCString(Local<JSValueRef> str)
{
    ecmascript::JSHandle<ecmascript::JSTaggedValue> ret = JSNApiHelper::ToJSHandle(str);
    ASSERT(ret->IsString());
    EcmaString *ecmaStr = EcmaString::Cast(ret.GetTaggedValue().GetTaggedObject());
    return ConvertToString(ecmaStr);
}

int32_t DebuggerApi::CStringToInt(const CString &str)
{
    return CStringToL(str);
}

int32_t DebuggerApi::StringToInt(Local<JSValueRef> str)
{
    return CStringToInt(ToCString(str));
}

// JSThread
Local<JSValueRef> DebuggerApi::GetException(const EcmaVM *ecmaVm)
{
    auto exception = ecmaVm->GetJSThread()->GetException();
    JSHandle<JSTaggedValue> handledException(ecmaVm->GetJSThread(), exception);
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

std::optional<Error> DebuggerApi::RegisterHooks(JSDebugger *debugger, PtHooks *hooks)
{
    return debugger->RegisterHooks(hooks);
}

std::optional<Error> DebuggerApi::SetBreakpoint(JSDebugger *debugger, const PtLocation &location)
{
    return debugger->SetBreakpoint(location);
}

std::optional<Error> DebuggerApi::RemoveBreakpoint(JSDebugger *debugger, const PtLocation &location)
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
    JSTaggedValue env = ecmaVm->GetJSThread()->GetCurrentLexenv();
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env= taggedParentEnv;
    }
    JSTaggedValue value = LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

void DebuggerApi::SetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot, Local<JSValueRef> value)
{
    JSTaggedValue target = JSNApiHelper::ToJSHandle(value).GetTaggedValue();
    JSTaggedValue env = ecmaVm->GetJSThread()->GetCurrentLexenv();
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
        ASSERT(!taggedParentEnv.IsUndefined());
        env= taggedParentEnv;
    }
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(ecmaVm->GetJSThread(), slot, target);
}

bool DebuggerApi::EvaluateLexicalValue(const EcmaVM *ecmaVm, const CString &name, int32_t &level, uint32_t &slot)
{
    JSTaggedValue curEnv = ecmaVm->GetJSThread()->GetCurrentLexenv();
    for (; curEnv.IsTaggedArray(); curEnv = LexicalEnv::Cast(curEnv.GetTaggedObject())->GetParentEnv()) {
        level++;
        LexicalEnv *lexicalEnv = LexicalEnv::Cast(curEnv.GetTaggedObject());
        if (lexicalEnv->GetScopeInfo().IsHole()) {
            continue;
        }
        auto result = JSNativePointer::Cast(lexicalEnv->GetScopeInfo().GetTaggedObject())->GetExternalPointer();
        ScopeDebugInfo *scopeDebugInfo = reinterpret_cast<ScopeDebugInfo *>(result);
        auto iter = scopeDebugInfo->scopeInfo.find(name);
        if (iter == scopeDebugInfo->scopeInfo.end()) {
            continue;
        }
        slot = iter->second;
        return true;
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
        auto iter = scopeDebugInfo->scopeInfo.find(name);
        if (iter == scopeDebugInfo->scopeInfo.end()) {
            continue;
        }
        uint32_t slot = iter->second;
        JSTaggedValue value = lexicalEnv->GetProperties(slot);
        JSHandle<JSTaggedValue> handledValue(thread, value);
        return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
    }
    JSHandle<JSTaggedValue> handledValue(thread, JSTaggedValue::Hole());
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}
}  // namespace panda::tooling::ecmascript
