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

#ifndef ECMASCRIPT_TOOLING_INTERFACE_DEBUGGER_API_H
#define ECMASCRIPT_TOOLING_INTERFACE_DEBUGGER_API_H

#include <functional>

#include "ecmascript/common.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/include/jsnapi.h"

#include "mem/rendezvous.h"
#include "include/runtime.h"
#include "include/tooling/debug_interface.h"

namespace panda {
namespace tooling::ecmascript {
class JSDebugger;
}  // tooling::ecmascript

namespace ecmascript {
class InterpretedFrameHandler;
class EcmaVM;
class JSMethod;
class JSThread;
}  // ecmascript
}  // panda

namespace panda::tooling::ecmascript {
using panda::ecmascript::CString;
using panda::ecmascript::InterpretedFrameHandler;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::JSMethod;
using panda::ecmascript::JSThread;

enum StackState {
    CONTINUE,
    FAILED,
    SUCCESS,
};

class PUBLIC_API DebuggerApi {
public:
    // CString
    static uint64_t CStringToULL(const CString &str);
    static CString ToCString(int32_t number);
    static CString ConvertToString(const std::string &str);

    // InterpretedFrameHandler
    static uint32_t GetStackDepth(const EcmaVM *ecmaVm);
    static bool StackWalker(const EcmaVM *ecmaVm, std::function<StackState(const InterpretedFrameHandler *)> func);
    static uint32_t GetBytecodeOffset(const EcmaVM *ecmaVm);
    static JSMethod *GetMethod(const EcmaVM *ecmaVm);
    static Local<JSValueRef> GetVRegValue(const EcmaVM *ecmaVm, size_t index);
    static void SetVRegValue(const EcmaVM *ecmaVm, size_t index, Local<JSValueRef> value);
    static uint32_t GetBytecodeOffset(const InterpretedFrameHandler *frameHandler);
    static JSMethod *GetMethod(const InterpretedFrameHandler *frameHandler);
    static Local<JSValueRef> GetVRegValue(const EcmaVM *ecmaVm,
        const InterpretedFrameHandler *frameHandler, size_t index);

    // JSThread
    static Local<JSValueRef> GetException(const EcmaVM *ecmaVm);
    static void SetException(const EcmaVM *ecmaVm, Local<JSValueRef> exception);
    static void ClearException(const EcmaVM *ecmaVm);

    // EcmaVM
    static const panda_file::File *FindPandaFile(const EcmaVM *ecmaVm, const CString &fileName);

    // NumberHelper
    static double StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix);

    // JSDebugger
    static JSDebugger *CreateJSDebugger(const EcmaVM *ecmaVm);
    static void DestroyJSDebugger(JSDebugger *debugger);
    static std::optional<Error> RegisterHooks(JSDebugger *debugger, PtHooks *hooks);
    static std::optional<Error> SetBreakpoint(JSDebugger *debugger, const PtLocation &location);
    static std::optional<Error> RemoveBreakpoint(JSDebugger *debugger, const PtLocation &location);

    // JSMehthod
    static CString ParseFunctionName(const JSMethod *method);
};
}  // namespace panda::tooling::ecmascript

#endif  // ECMASCRIPT_TOOLING_DEBUGGER_API_H
