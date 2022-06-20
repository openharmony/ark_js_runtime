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

    static void JSProxyCallInternalWithArgV(ExtendedAssembler *assembler);

    static void JSCall(ExtendedAssembler *assembler);

    static void CallRuntimeWithArgv(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterp(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterpDispatch(ExtendedAssembler *assembler);

    static void JSCallDispatch(ExtendedAssembler *assembler);

    static void AsmInterpreterEntry(ExtendedAssembler *assembler);

    static void PushCallIThisRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs3AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs2AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs1AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs0AndDispatch(ExtendedAssembler *assembler);

    static void PushCallNewAndDispatch(ExtendedAssembler *assembler);

    static void PushCallNewAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallArgsAndDispatchNative(ExtendedAssembler *assembler);

    static void ResumeRspAndDispatch(ExtendedAssembler *assembler);

    static void ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler);

    static void CallGetter(ExtendedAssembler *assembler);

    static void CallSetter(ExtendedAssembler *assembler);

    static void ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler);

    static void ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler);

    static void CallOptimizedJSFunction(ExtendedAssembler *assembler);

    static void JSCallWithArgV(ExtendedAssembler *assembler);

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
    static void PushAsmInterpBridgeFrame(ExtendedAssembler *assembler);
    static void PopAsmInterpBridgeFrame(ExtendedAssembler *assembler);
    static void CallBCStub(ExtendedAssembler *assembler, Register newSpRegister, Register glueRegister,
        Register callTargetRegister, Register methodRegister, Register pcRegister);
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
    static void PushCallThis(ExtendedAssembler *assembler, JSCallMode mode);
    static Register GetThisRegsiter(ExtendedAssembler *assembler, JSCallMode mode);
    static void PushVregs(ExtendedAssembler *assembler);
    static void DispatchCall(ExtendedAssembler *assembler, Register pcRegister, Register newSpRegister);
    static void CallNativeEntry(ExtendedAssembler *assemblSer);
    static void CallNativeWithArgv(ExtendedAssembler *assembler, bool callNew);
    static void CallNativeInternal(ExtendedAssembler *assembler,
        Register glue, Register numArgs, Register stackArgs, Register nativeCode);
    static void PushBuiltinFrame(ExtendedAssembler *assembler, Register glue, FrameType type);
    static void JSCallCommonEntry(ExtendedAssembler *assembler, JSCallMode mode);
    static void JSCallCommonFastPath(ExtendedAssembler *assembler, JSCallMode mode);
    static void JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
        Label *fastPathEntry, Label *pushCallThis);
    static void OptimizedCallAsmInterpreter(ExtendedAssembler *assembler);
    static void PushArgsWithArgV(ExtendedAssembler *assembler, Register jsfunc,
                                 Register actualNumArgs, Register argV, Label *pushCallThis);
    static void CopyArgumentWithArgV(ExtendedAssembler *assembler, Register argc, Register argV);
    static void PushMandatoryJSArgs(ExtendedAssembler *assembler, Register jsfunc,
                                    Register thisObj, Register newTarget);
    static void PopAotArgs(ExtendedAssembler *assembler, Register expectedNumArgs);
    static void PushAotEntryFrame(ExtendedAssembler *assembler, Register prevFp);
    static void PopAotEntryFrame(ExtendedAssembler *assembler, Register glue);
    static void PushOptimizedFrame(ExtendedAssembler *assembler, Register callSiteSp);
    static void PopOptimizedFrame(ExtendedAssembler *assembler);
};
}  // namespace panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H
