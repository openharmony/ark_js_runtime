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

#ifndef ECMASCRIPT_TOOLING_BACKEND_DEBUGGER_EXECUTOR_H
#define ECMASCRIPT_TOOLING_BACKEND_DEBUGGER_EXECUTOR_H

#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
class InterpretedFrameHandler;
namespace tooling {
class DebuggerExecutor {
public:
    DebuggerExecutor() = default;
    ~DebuggerExecutor() = default;

    static void Initialize(const EcmaVM *vm);

    static Local<JSValueRef> GetValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                      Local<StringRef> name);
    static bool SetValue(const EcmaVM *vm, InterpretedFrameHandler *frameHandler,
                         Local<StringRef> name, Local<JSValueRef> value);

private:
    NO_COPY_SEMANTIC(DebuggerExecutor);
    NO_MOVE_SEMANTIC(DebuggerExecutor);

    static Local<JSValueRef> DebuggerGetValue(EcmaVM *vm, Local<JSValueRef> thisArg, const Local<JSValueRef> *argv,
                                              int32_t length, void *data);
    static Local<JSValueRef> DebuggerSetValue(EcmaVM *vm, Local<JSValueRef> thisArg, const Local<JSValueRef> *argv,
                                              int32_t length, void *data);

    static void ThrowException(const EcmaVM *vm, const std::string &error);

    static Local<JSValueRef> GetLocalValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                           Local<StringRef> name);
    static Local<JSValueRef> GetLexicalValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                             Local<StringRef> name);
    static Local<JSValueRef> GetGlobalValue(const EcmaVM *vm, Local<StringRef> name);

    static bool SetLocalValue(const EcmaVM *vm, InterpretedFrameHandler *frameHandler,
                              Local<StringRef> name, Local<JSValueRef> value);
    static bool SetLexicalValue(const EcmaVM *vm, const InterpretedFrameHandler *frameHandler,
                                Local<StringRef> name, Local<JSValueRef> value);
    static bool SetGlobalValue(const EcmaVM *vm, Local<StringRef> name, Local<JSValueRef> value);

    constexpr static int32_t NUM_ARGS = 2;
};
}  // namespace tooling
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TOOLING_BACKEND_DEBUGGER_EXECUTOR_H
