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

#ifndef ECMASCRIPT_COMPILER_AARCH64_EXTENDED_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_AARCH64_EXTENDED_ASSEMBLER_H

#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/assembler/aarch64/extend_assembler.h"

namespace panda::ecmascript::aarch64 {
class AssemblerStubs {
public:
    static void CallRuntime(ExtendedAssembler *assembler);

    static void JSFunctionEntry(ExtendedAssembler *assembler);

    static void OptimizedCallOptimized(ExtendedAssembler *assembler);

    static void CallNativeTrampoline(ExtendedAssembler *assembler);

    static void JSCallWithArgV(ExtendedAssembler *assembler);

    static void JSCall(ExtendedAssembler *assembler);

    static void CallRuntimeWithArgv(ExtendedAssembler *assembler);

    static void AsmInterpreterEntry(ExtendedAssembler *assembler);

    static void JSCallDispatch(ExtendedAssembler *assembler);

    static void PushCallIThisRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs3AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs2AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs1AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs0AndDispatch(ExtendedAssembler *assembler);

    static void PushCallIThisRangeAndDispatchSlowPath(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatchSlowPath(ExtendedAssembler *assembler);

    static void PushCallArgs3AndDispatchSlowPath(ExtendedAssembler *assembler);

    static void PushCallArgs2AndDispatchSlowPath(ExtendedAssembler *assembler);

    static void PushCallArgs1AndDispatchSlowPath(ExtendedAssembler *assembler);

    static void PushCallArgs0AndDispatchSlowPath(ExtendedAssembler *assembler);

    static void PushCallIThisRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallArgs3AndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallArgs2AndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallArgs1AndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallArgs0AndDispatchNative(ExtendedAssembler *assembler);

    static void ResumeRspAndDispatch(ExtendedAssembler *assembler);

    static void ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler);

    static void ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler);
};
}  // namespace panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H
