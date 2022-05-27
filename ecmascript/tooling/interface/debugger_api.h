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
#include "ecmascript/scope_info_extractor.h"
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
}  // tooling
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
    // CString
    static uint64_t CStringToULL(const CString &str);
    static CString ToCString(int32_t number);
    static CString ConvertToString(const std::string &str);

    // InterpretedFrameHandler
    static uint32_t GetStackDepth(const EcmaVM *ecmaVm);
    static std::shared_ptr<InterpretedFrameHandler> NewFrameHandler(const EcmaVM *ecmaVm);
    static bool StackWalker(const EcmaVM *ecmaVm, std::function<StackState(const InterpretedFrameHandler *)> func);
    static uint32_t GetBytecodeOffset(const EcmaVM *ecmaVm);
    static JSMethod *GetMethod(const EcmaVM *ecmaVm);
    static void SetClosureVariables(const EcmaVM *ecmaVm, const InterpretedFrameHandler *frameHandler,
        Local<ObjectRef> &localObj);
    static Local<JSValueRef> GetVRegValue(const EcmaVM *ecmaVm, size_t index);
    static void SetVRegValue(const EcmaVM *ecmaVm, size_t index, Local<JSValueRef> value);
    static uint32_t GetBytecodeOffset(const InterpretedFrameHandler *frameHandler);
    static JSMethod *GetMethod(const InterpretedFrameHandler *frameHandler);
    static JSTaggedValue GetEnv(const InterpretedFrameHandler *frameHandler);
    static JSTaggedType *GetSp(const InterpretedFrameHandler *frameHandler);
    static Local<JSValueRef> GetVRegValue(const EcmaVM *ecmaVm,
        const InterpretedFrameHandler *frameHandler, size_t index);

    // JSThread
    static Local<JSValueRef> GetAndClearException(const EcmaVM *ecmaVm);
    static void SetException(const EcmaVM *ecmaVm, Local<JSValueRef> exception);
    static void ClearException(const EcmaVM *ecmaVm);

    // EcmaVM
    static const panda_file::File *FindPandaFile(const EcmaVM *ecmaVm, const CString &fileName);

    // NumberHelper
    static double StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix);

    // JSDebugger
    static JSDebugger *CreateJSDebugger(const EcmaVM *ecmaVm);
    static void DestroyJSDebugger(JSDebugger *debugger);
    static void RegisterHooks(JSDebugger *debugger, PtHooks *hooks);
    static bool SetBreakpoint(JSDebugger *debugger, const JSPtLocation &location,
        const Local<FunctionRef> &condFuncRef);
    static bool RemoveBreakpoint(JSDebugger *debugger, const JSPtLocation &location);

    // JSMehthod
    static CString ParseFunctionName(const JSMethod *method);

    // ScopeInfo
    static Local<JSValueRef> GetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot);
    static void SetProperties(const EcmaVM *ecmaVm, int32_t level, uint32_t slot, Local<JSValueRef> value);
    static bool EvaluateLexicalValue(const EcmaVM *ecmaVm, const CString &name, int32_t &level, uint32_t &slot);
    static Local<JSValueRef> GetLexicalValueInfo(const EcmaVM *ecmaVm, const CString &name);
    static void InitJSDebugger(JSDebugger *debugger);
    static bool HandleUncaughtException(const EcmaVM *ecmaVm, CString &message);
    static Local<FunctionRef> GenerateFuncFromBuffer(const EcmaVM *ecmaVm, const void *buffer, size_t size);
    static Local<JSValueRef> EvaluateViaFuncCall(EcmaVM *ecmaVm, const Local<FunctionRef> &funcRef,
        std::shared_ptr<InterpretedFrameHandler> &frameHandler);

private:
    static JSTaggedValue GetCurrentEvaluateEnv(const EcmaVM *ecmaVm);
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_DEBUGGER_API_H
