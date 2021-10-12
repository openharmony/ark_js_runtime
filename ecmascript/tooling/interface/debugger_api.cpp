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
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/jsnapi_helper-inl.h"
#include "ecmascript/tooling/interface/js_debugger.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::JSHandle;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::Program;
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
    for (; frameHandler.HasFrame(); frameHandler.PrevFrame()) {
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
    for (; frameHandler.HasFrame(); frameHandler.PrevFrame()) {
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

// EcmaVM
const panda_file::File *DebuggerApi::FindPandaFile(const EcmaVM *ecmaVm, const CString &fileName)
{
    const panda_file::File *pfs = nullptr;
    ecmaVm->EnumeratePandaFiles([&pfs, fileName]([[maybe_unused]] const Program *pg, const panda_file::File *pf) {
        if (ConvertToString(pf->GetFilename()) == fileName) {
            pfs = pf;
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
JSDebugger *DebuggerApi::CreateJSDebugger(Runtime *runtime, const EcmaVM *ecmaVm)
{
    return new JSDebugger(runtime, ecmaVm);
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
}  // namespace panda::tooling::ecmascript
