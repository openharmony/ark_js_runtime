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
#include "ecmascript/jspandafile/scope_info_extractor.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/tooling/interface/js_debug_interface.h"

namespace panda {
namespace ecmascript {
class InterpretedFrameHandler;
class EcmaVM;
class JSMethod;
class JSThread;
namespace tooling {
class JSDebugger;
}
}  // ecmascript
}  // panda

namespace panda::ecmascript::tooling {
enum StackState {
    CONTINUE,
    FAILED,
    SUCCESS,
};

class PUBLIC_API DebuggerApi {
public:
    // JSPandaFileExecutor
    Local<JSValueRef> Execute(const EcmaVM *ecmaVm, const void *buffer, size_t size,
                              std::string_view entryPoint);

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

    // String
    static int32_t CStringToInt(const CString &str);
    static CString ToCString(Local<JSValueRef> str);
    static int32_t StringToInt(Local<JSValueRef> str);

    // JSThread
    static Local<JSValueRef> GetAndClearException(const EcmaVM *ecmaVm);
    static void SetException(const EcmaVM *ecmaVm, Local<JSValueRef> exception);
    static void ClearException(const EcmaVM *ecmaVm);

    // NumberHelper
    static double StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix);

    // JSDebugger
    static JSDebugger *CreateJSDebugger(const EcmaVM *ecmaVm);
    static void DestroyJSDebugger(JSDebugger *debugger);
    static void RegisterHooks(JSDebugger *debugger, PtHooks *hooks);
    static bool SetBreakpoint(JSDebugger *debugger, const JSPtLocation &location);
    static bool RemoveBreakpoint(JSDebugger *debugger, const JSPtLocation &location);

    // JSMehthod
    static CString ParseFunctionName(const JSMethod *method);

    // ScopeInfo
    static Local<JSValueRef> GetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot);
    static void SetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot, Local<JSValueRef> value);
    static bool EvaluateLexicalValue(const EcmaVM *ecmaVm, const CString &name, int32_t &level, uint32_t &slot);
    static Local<JSValueRef> GetLexicalValueInfo(const EcmaVM *ecmaVm, const CString &name);
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_DEBUGGER_API_H
