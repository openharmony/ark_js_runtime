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

#include "ecmascript/compiler/assembler/x64/assembler_x64.h"
#include "ecmascript/compiler/assembler/x64/extended_assembler_x64.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript::x64 {
class AssemblerStubsX64 {
public:
    static void CallRuntime(ExtendedAssembler *assembler);

    static void JSFunctionEntry(ExtendedAssembler *assembler);

    static void OptimizedCallOptimized(ExtendedAssembler *assembler);

    static void CallBuiltinTrampoline(ExtendedAssembler *assembler);

    static void JSCallWithArgV(ExtendedAssembler *assembler);

    static void JSCall(ExtendedAssembler *assembler);

    static void CallRuntimeWithArgv(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterp(ExtendedAssembler *assembler);

    static void JSCallDispatch(ExtendedAssembler *assembler);

    static void AsmInterpreterEntry(ExtendedAssembler *assembler);

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

    static void PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallArgsAndDispatchNative(ExtendedAssembler *assembler);

    static void ResumeRspAndDispatch(ExtendedAssembler *assembler);

    static void ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler);

    static void ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler);

    static void ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler);

private:
    static void PushArgsFastPath(ExtendedAssembler *assembler, Register glueRegister, Register argcRegister,
        Register argvRegister, Register callTargetRegister, Register methodRegister, Register prevSpRegister,
        Register fpRegister, Register callFieldRegister);
    static void PushArgsSlowPath(ExtendedAssembler *assembler, Register glueRegister,
        Register declaredNumArgsRegister, Register argcRegister, Register argvRegister, Register callTargetRegister,
        Register methodRegister, Register prevSpRegister, Register callFieldRegister);
    static void PushFrameState(ExtendedAssembler *assembler, Register prevSpRegister, Register fpRegister,
        Register callTargetRegister, Register methodRegister, Register pcRegister, Register operatorRegister);
    static void PushGeneratorFrameState(ExtendedAssembler *assembler, Register prevSpRegister,
        Register fpRegister, Register callTargetRegister, Register methodRegister, Register contextRegister,
        Register pcRegister, Register operatorRegister);
    static void PushAsmInterpEntryFrame(ExtendedAssembler *assembler);
    static void PopAsmInterpEntryFrame(ExtendedAssembler *assembler);
    static void CallBCStub(ExtendedAssembler *assembler, Register newSpRegister, Register glueRegister,
        Register callTargetRegister, Register methodRegister, Register pcRegister, bool isReturn);
    static void GlueToThread(ExtendedAssembler *assembler, Register glueRegister, Register threadRegister);
    static void ConstructEcmaRuntimeCallInfo(ExtendedAssembler *assembler, Register threadRegister,
        Register numArgsRegister, Register stackArgsRegister);
    static void GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callFieldRegister,
        Register declaredNumArgsRegister);
    static void GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callFieldRegister,
        Register numVregsRegister);
    static void PushUndefinedWithArgc(ExtendedAssembler *assembler, Register argc);
    static void HasPendingException(ExtendedAssembler *assembler, Register threadRegister);
    static void StackOverflowCheck(ExtendedAssembler *assembler);
    static void CallIThisRangeNoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgsRegister);
    static void CallIRangeNoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgsRegister);
    static void Callargs3NoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgsRegister);
    static void Callargs2NoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgsRegister);
    static void Callargs1NoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgsRegister);
    static void Callargs0NoExtraEntry(ExtendedAssembler *assembler);
    static void CallIThisRangeEntry(ExtendedAssembler *assembler);
    static void PushCallThis(ExtendedAssembler *assembler);
    static void CallIRangeEntry(ExtendedAssembler *assembler);
    static void Callargs3Entry(ExtendedAssembler *assembler);
    static void Callargs2Entry(ExtendedAssembler *assembler);
    static void Callarg1Entry(ExtendedAssembler *assembler);
    static void PushCallThisUndefined(ExtendedAssembler *assembler);
    static void PushNewTarget(ExtendedAssembler *assembler);
    static void PushCallTarget(ExtendedAssembler *assembler);
    static void PushVregs(ExtendedAssembler *assembler);
    static void DispatchCall(ExtendedAssembler *assembler, Register pcRegister, Register newSpRegister);
    static void CallNativeEntry(ExtendedAssembler *assembler);
    static void CallNativeInternal(ExtendedAssembler *assembler,
        Register glue, Register numArgs, Register stackArgs, Register nativeCode);
    static void PushBuiltinFrame(ExtendedAssembler *assembler, Register glue, FrameType type);
};
}  // namespace panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H
