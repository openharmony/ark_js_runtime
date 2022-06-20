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
#include "ecmascript/frames.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
class AssemblerStubs {
public:
    static const int FRAME_SLOT_SIZE = 8;
    static inline int64_t GetStackArgOffSetToFp(unsigned argId)
    {
        //   +--------------------------+
        //   |       argv0              | calltarget , newtARGET, this, ....
        //   +--------------------------+ ---
        //   |       argc               |   ^
        //   |--------------------------|   arguments
        //   |       codeAddress        |   |
        //   |--------------------------|   |
        //   |       returnAddr         |   |
        //   |--------------------------| Fixed OptimizedLeaveFrame
        //   |       callsiteFp         |   |
        //   |--------------------------|   |
        //   |       frameType          |   v
        //   +--------------------------+ ---
        // 16 : 16 means arguments offset to fp
        return 16 + argId * FRAME_SLOT_SIZE;
    }

    static void CallRuntime(ExtendedAssembler *assembler);

    static void JSFunctionEntry(ExtendedAssembler *assembler);

    static void OptimizedCallOptimized(ExtendedAssembler *assembler);

    static void CallBuiltinTrampoline(ExtendedAssembler *assembler);

    static void JSProxyCallInternalWithArgV(ExtendedAssembler *assembler);

    static void JSCall(ExtendedAssembler *assembler);

    static void CallRuntimeWithArgv(ExtendedAssembler *assembler);

    static void AsmInterpreterEntry(ExtendedAssembler *assembler);

    static void JSCallDispatch(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterp(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterpDispatch(ExtendedAssembler *assembler);

    static void PushCallIThisRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs3AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs2AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs1AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs0AndDispatch(ExtendedAssembler *assembler);

    static void PushCallIThisRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallNewAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallNewAndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgsAndDispatchNative(ExtendedAssembler *assembler);

    static void ResumeRspAndDispatch(ExtendedAssembler *assembler);

    static void ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler);

    static void ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler);

    static void ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler);

    static void CallGetter(ExtendedAssembler *assembler);

    static void CallSetter(ExtendedAssembler *assembler);

    static void CallOptimizedJSFunction(ExtendedAssembler *assembler);

    static void JSCallWithArgV(ExtendedAssembler *assembler);

private:
    static void JSCallBody(ExtendedAssembler *assembler, Register jsfunc);

    static void PushCallThis(ExtendedAssembler *assembler, JSCallMode mode);

    static Register GetThisRegsiter(ExtendedAssembler *assembler, JSCallMode mode);

    static void PushVregs(ExtendedAssembler *assembler);

    static void DispatchCall(ExtendedAssembler *assembler, Register pc, Register newSp);

    static void CallNativeInternal(ExtendedAssembler *assembler, Register glue, Register numArgs, Register stackArgs,
        Register nativeCode);

    static void PushBuiltinFrame(ExtendedAssembler *assembler, Register glue, FrameType type, Register op);

    static void PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fp, Register callTarget,
        Register method, Register pc, Register op);

    static void JSCallCommonEntry(ExtendedAssembler *assembler, JSCallMode mode);
    static void JSCallCommonFastPath(ExtendedAssembler *assembler, JSCallMode mode, Label *pushCallThis);
    static void JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
                                     Label *fastPathEntry, Label *pushCallThis);

    static void GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callField, Register numVregs);

    static void GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callField,
        Register declaredNumArgs);

    static void PushUndefinedWithArgc(ExtendedAssembler *assembler, Register argc, Register temp,
        panda::ecmascript::Label *next);

    static void SaveFpAndJumpSize(ExtendedAssembler *assembler, Immediate jumpSize);

    static void GlueToThread(ExtendedAssembler *assembler, Register glue, Register thread);

    static void ConstructEcmaRuntimeCallInfo(ExtendedAssembler *assembler, Register thread, Register numArgs,
        Register stackArgs);

    static void StackOverflowCheck([[maybe_unused]] ExtendedAssembler *assembler);

    static void PushAsmInterpEntryFrame(ExtendedAssembler *assembler, bool saveLeave);

    static void PopAsmInterpEntryFrame(ExtendedAssembler *assembler, bool saveLeave);

    static void PushGeneratorFrameState(ExtendedAssembler *assembler, Register &prevSpRegister, Register &fpRegister,
        Register &callTargetRegister, Register &methodRegister, Register &contextRegister, Register &pcRegister,
        Register &operatorRegister);

    static void CallBCStub(ExtendedAssembler *assembler, Register &newSp, Register &glue,
        Register &callTarget, Register &method, Register &pc, Register &temp);

    static void CallNativeEntry(ExtendedAssembler *assembler);

    static void CallNativeWithArgv(ExtendedAssembler *assembler, bool callNew);

    static void PushArgsFastPath(ExtendedAssembler *assembler,
        Register &glue, Register &argc, Register &argv, Register &callTarget,
        Register &method, Register &prevSp, Register &fp, Register &callField);

    static void PushArgsSlowPath(ExtendedAssembler *assembler, Register &glueRegister,
        Register &declaredNumArgsRegister, Register &argcRegister, Register &argvRegister, Register &callTargetRegister,
        Register &methodRegister, Register &prevSpRegister, Register &callFieldRegister);
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
