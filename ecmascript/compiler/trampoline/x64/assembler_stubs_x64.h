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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H

#include "ecmascript/compiler/assembler/assembler_x64.h"
#include "ecmascript/compiler/assembler/extended_assembler_x64.h"

namespace panda::ecmascript::x64 {
class AssemblerStubsX64 {
public:
    static void CallRuntime(ExtendedAssemblerX64 *assembler);

    static void JSFunctionEntry(ExtendedAssemblerX64 *assembler);

    static void OptimizedCallOptimized(ExtendedAssemblerX64 *assembler);

    static void CallNativeTrampoline(ExtendedAssemblerX64 *assembler);

    static void JSCallWithArgv(ExtendedAssemblerX64 *assembler);

    static void JSCall(ExtendedAssemblerX64 *assembler);

    static void CallRuntimeWithArgv(ExtendedAssemblerX64 *assembler);

    static void AsmInterpreterEntry(ExtendedAssemblerX64 *assembler);

    static void JSCallDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallIThisRangeAndDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallIRangeAndDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs3AndDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs2AndDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs1AndDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs0AndDispatch(ExtendedAssemblerX64 *assembler);

    static void PushCallIThisRangeAndDispatchSlowPath(ExtendedAssemblerX64 *assembler);

    static void PushCallIRangeAndDispatchSlowPath(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs3AndDispatchSlowPath(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs2AndDispatchSlowPath(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs1AndDispatchSlowPath(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs0AndDispatchSlowPath(ExtendedAssemblerX64 *assembler);

    static void PushCallIThisRangeAndDispatchNative(ExtendedAssemblerX64 *assembler);

    static void PushCallIRangeAndDispatchNative(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs3AndDispatchNative(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs2AndDispatchNative(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs1AndDispatchNative(ExtendedAssemblerX64 *assembler);

    static void PushCallArgs0AndDispatchNative(ExtendedAssemblerX64 *assembler);

    static void ResumeRspAndDispatch(ExtendedAssemblerX64 *assembler);

    static void ResumeRspAndReturn([[maybe_unused]] ExtendedAssemblerX64 *assembler);

private:
    static void PushArgsFastPath(ExtendedAssemblerX64 *assembler, Register glueRegister, Register argcRegister,
        Register argvRegister, Register callTargetRegister, Register methodRegister, Register prevSpRegister,
        Register fpRegister, Register callFieldRegister);
    static void PushArgsSlowPath(ExtendedAssemblerX64 *assembler, Register glueRegister,
        Register declaredNumArgsRegister, Register argcRegister, Register argvRegister, Register callTargetRegister,
        Register methodRegister, Register prevSpRegister, Register callFieldRegister);
    static void CallNativeEntry(ExtendedAssemblerX64 *assembler, Register glueRegister, Register argcRegister,
        Register argvRegister, Register callTargetRegister, Register methodRegister, Register prevSpRegister);
    static void PushFrameState(ExtendedAssemblerX64 *assembler, Register prevSpRegister, Register fpRegister,
        Register callTargetRegister, Register methodRegister, Register pcRegister, Register operatorRegister,
        bool isNative);
    static void GlueToThread(ExtendedAssemblerX64 *assembler, Register glueRegister, Register threadRegister);
    static void ConstructEcmaRuntimeCallInfo(ExtendedAssemblerX64 *assembler, Register threadRegister,
        Register numArgsRegister, Register stackArgsRegister);
    static void GetDeclaredNumArgsFromCallField(ExtendedAssemblerX64 *assembler, Register callFieldRegister,
        Register declaredNumArgsRegister);
    static void GetNumVregsFromCallField(ExtendedAssemblerX64 *assembler, Register callFieldRegister,
        Register numVregsRegister);
    static void PushUndefinedWithArgc(ExtendedAssemblerX64 *assembler, Register argc);
    static void HasPendingException(ExtendedAssemblerX64 *assembler, Register threadRegister);
    static void StackOverflowCheck(ExtendedAssemblerX64 *assembler);
    static void CallIThisRangeNoExtraEntry(ExtendedAssemblerX64 *assembler, Register declaredNumArgsRegister);
    static void CallIRangeNoExtraEntry(ExtendedAssemblerX64 *assembler, Register declaredNumArgsRegister);
    static void Callargs3NoExtraEntry(ExtendedAssemblerX64 *assembler, Register declaredNumArgsRegister);
    static void Callargs2NoExtraEntry(ExtendedAssemblerX64 *assembler, Register declaredNumArgsRegister);
    static void Callargs1NoExtraEntry(ExtendedAssemblerX64 *assembler, Register declaredNumArgsRegister);
    static void Callargs0NoExtraEntry(ExtendedAssemblerX64 *assembler);
    static void CallIThisRangeEntry(ExtendedAssemblerX64 *assembler);
    static void PushCallThis(ExtendedAssemblerX64 *assembler);
    static void CallIRangeEntry(ExtendedAssemblerX64 *assembler);
    static void Callargs3Entry(ExtendedAssemblerX64 *assembler);
    static void Callargs2Entry(ExtendedAssemblerX64 *assembler);
    static void Callarg1Entry(ExtendedAssemblerX64 *assembler);
    static void PushCallThisUndefined(ExtendedAssemblerX64 *assembler);
    static void PushNewTarget(ExtendedAssemblerX64 *assembler);
    static void PushCallTarget(ExtendedAssemblerX64 *assembler);
    static void PushVregs(ExtendedAssemblerX64 *assembler);
    static void DispatchCall(ExtendedAssemblerX64 *assembler, Register pcRegister, Register newSpRegister);
    static void CallIThisRangeNativeEntry(ExtendedAssemblerX64 *assembler);
    static void CallIRangeNativeEntry(ExtendedAssemblerX64 *assembler);
    static void Callargs3NativeEntry(ExtendedAssemblerX64 *assembler, Register opRegister);
    static void Callargs2NativeEntry(ExtendedAssemblerX64 *assembler, Register opRegister);
    static void Callarg1NativeEntry(ExtendedAssemblerX64 *assembler);
    static void PushCallThisUndefinedNative(ExtendedAssemblerX64 *assembler);
    static void PushLeftFrameNative(ExtendedAssemblerX64 *assembler);
    static void PushFrameStateNativeAndCall(ExtendedAssemblerX64 *assembler);
};
}  // namespace panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H
