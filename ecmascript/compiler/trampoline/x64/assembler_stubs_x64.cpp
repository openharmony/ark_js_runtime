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

#include "assembler_stubs_x64.h"

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/message_string.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::x64 {
#define __ assembler->
void AssemblerStubsX64::CallRuntime(ExtendedAssemblerX64 *assembler)
{
    __ Pushq(rbp);
    __ Movq(rsp, rbp);
    __ Movq(rsp, Operand(rax, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Pushq(static_cast<int32_t>(FrameType::LEAVE_FRAME));

    __ Pushq(r10);
    __ Pushq(rdx);
    __ Pushq(rax);

    __ Movq(rbp, rdx);
    // 16: rbp & return address
    __ Addq(16, rdx);

    __ Movq(Operand(rdx, 0), r10);
    __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
    __ Movq(rax, rdi);
    // 8: argc
    __ Movq(Operand(rdx, 8), rsi);
    // 16: argv
    __ Addq(16, rdx);
    __ Callq(r10);

    // 8: skip rax
    __ Addq(8, rsp);
    __ Popq(rdx);
    __ Popq(r10);

    // 8: skip frame type
    __ Addq(8, rsp);
    __ Popq(rbp);
    __ Ret();
}

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//     uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
// Input:
// %rdi - glue
// %rsi - prevFp
// %rdx - expectedNumArgs
// %ecx - actualNumArgs
// %r8  - argV
// %r9  - codeAddr
void AssemblerStubsX64::JSFunctionEntry(ExtendedAssemblerX64 *assembler)
{
    Register glueReg = rdi;
    Register prevFpReg = rsi;
    Register expectedNumArgsReg = rdx;
    Register actualNumArgsReg = rcx;
    Register argvReg = r8;
    Register codeAddrReg = r9;

    Label lAlign16Bytes;
    Label lCopyExtraAument;
    Label lCopyArguments;
    Label lCopyLoop;
    Label lPopFrame;

    __ PushCppCalleeSaveRegisters();
    __ Pushq(glueReg); // caller save
    // construct the frame
    __ Pushq(rbp);
    __ Movq(rsp, rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_FRAME));
    __ Pushq(prevFpReg);

    // 16 bytes align check
    __ Movl(expectedNumArgsReg, r14);
    __ Testb(1, r14);
    __ Jne(&lAlign16Bytes);
    __ Pushq(0); // push zero to align 16 bytes stack

    __ Bind(&lAlign16Bytes);
    // expectedNumArgs > actualNumArgs
    __ Movl(expectedNumArgsReg, rbx); // save expectedNumArgs
    __ Cmpl(actualNumArgsReg, expectedNumArgsReg);
    __ Jbe(&lCopyArguments);
    __ Movl(actualNumArgsReg, rax);
    __ Movl(rbx, expectedNumArgsReg);

    __ Bind(&lCopyExtraAument); // copy undefined value to stack
    __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
    __ Addq(-1, expectedNumArgsReg);
    __ Cmpq(rax, expectedNumArgsReg);
    __ Ja(&lCopyExtraAument);

    __ Bind(&lCopyArguments);
    __ Cmpl(actualNumArgsReg, rbx);
    __ CMovbe(rbx, actualNumArgsReg);
    __ Movl(actualNumArgsReg, rax); // rax -> actualNumArgsReg

    __ Bind(&lCopyLoop);
    __ Movq(Operand(argvReg, rax, Scale::Times8, -8), actualNumArgsReg); // -8 : disp
    __ Pushq(actualNumArgsReg);
    __ Addq(-1, rax);
    __ Jne(&lCopyLoop);

    __ Pushq(r14);
    __ Movq(glueReg, rax); // mov glue to rax
    __ Callq(codeAddrReg); // then call jsFunction
    __ Leaq(Operand(r14, Scale::Times8, 0), actualNumArgsReg); // Note: fixed for 3 extra arguments
    __ Addq(actualNumArgsReg, rsp);
    __ Addq(8, rsp); // 8: skip r14
    __ Testb(1, r14); // stack 16bytes align check
    __ Jne(&lPopFrame);
    __ Addq(8, rsp); // 8: align byte

    __ Bind(&lPopFrame);
    __ Popq(prevFpReg);
    __ Addq(8, rsp); // 8: frame type
    __ Popq(rbp);
    __ Popq(glueReg); // caller restore
    __ PopCppCalleeSaveRegisters(); // callee restore
    __ Movq(prevFpReg, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Ret();
}

// uint64_t OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, uintptr_t codeAddr, uintptr_t argv);
// Input:
// %rdi - glue
// %rsi - expectedNumArgs
// %rdx - actualNumArgs
// %rcx - codeAddr
// %r8  - argv

// sp[0 * 8]  -  argc
// sp[1 * 8]  -  argv[0]
// sp[2 * 8]  -  argv[1]
// .....
// sp[(N -3) * 8] - argv[N - 1]
// Output:
// stack as followsn from high address to lowAdress
//  sp       -      argv[N - 1]
// sp[-8]    -      argv[N -2]
// ...........................
// sp[- 8(N - 1)] - arg[0]
void AssemblerStubsX64::OptimizedCallOptimized(ExtendedAssemblerX64 *assembler)
{
    Register glueReg = rdi;
    Register expectedNumArgsReg = rsi;
    Register actualNumArgsReg = rdx;
    Register codeAddrReg = rcx;
    Register argvReg = r8;

    Label lAlign16Bytes1;
    Label lCopyExtraAument1;
    Label lCopyArguments1;
    Label lCopyLoop1;
    Label lPopFrame1;
    __ Pushq(rbp);
    __ Movq(rsp, rbp);
    __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_FRAME));
    // callee save
    __ Pushq(r14);
    __ Pushq(rbx);
    __ Pushq(rax);

    // 16 bytes align check
    __ Movl(expectedNumArgsReg, r14);
    __ Testb(1, r14);
    __ Jne(&lAlign16Bytes1);
    __ Pushq(0);

    __ Bind(&lAlign16Bytes1);
    // expectedNumArgs > actualNumArgs
    __ Movl(expectedNumArgsReg, rbx);
    __ Cmpl(actualNumArgsReg, expectedNumArgsReg); // save expectedNumArgs
    __ Jbe(&lCopyArguments1);
    __ Movl(actualNumArgsReg, rax);
    __ Movl(rbx, expectedNumArgsReg);

    __ Bind(&lCopyExtraAument1); // copy undefined value to stack
    __ Pushq(JSTaggedValue::VALUE_UNDEFINED);
    __ Addq(-1, expectedNumArgsReg);
    __ Cmpq(rax, expectedNumArgsReg);
    __ Ja(&lCopyExtraAument1);

    __ Bind(&lCopyArguments1);
    __ Cmpl(actualNumArgsReg, rbx);
    __ CMovbe(rbx, actualNumArgsReg);
    __ Movl(actualNumArgsReg, rax); // rax = actualNumArgsReg

    __ Bind(&lCopyLoop1);
    __ Movq(Operand(argvReg, rax, Scale::Times8, -8), rbx); // -8: stack index
    __ Pushq(rbx);
    __ Addq(-1, rax);
    __ Jne(&lCopyLoop1);
    __ Pushq(actualNumArgsReg); // actual argc
    __ Movq(glueReg, rax); // mov glue to rax
    __ Callq(codeAddrReg); // then call jsFunction
    __ Leaq(Operand(r14, Scale::Times8, 0), codeAddrReg);
    __ Addq(codeAddrReg, rsp);
    __ Addq(8, rsp); // 8: skip actualNumArgsReg
    __ Testb(1, r14); // stack 16bytes align check
    __ Jne(&lPopFrame1);
    __ Addq(8, rsp); // 8: align byte

    __ Bind(&lPopFrame1);
    __ Addq(8, rsp); // 8: skip rax
    __ Popq(rbx);
    __ Popq(r14);
    __ Addq(8, rsp); // 8: skip frame type
    __ Pop(rbp);
    __ Ret();
}

// uint64_t CallNativeTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %rax - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argv1
// sp + 16: argv0
// sp + 8:  actualArgc
// sp:      codeAddress
// construct Native Leave Frame:
//   +--------------------------+
//   |       argv0              | calltarget , newTarget, this, ....
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|  Fixed
//   |       codeAddress        | OptimizedLeaveFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---

// Output:
//  sp - 8 : pc
//  sp - 16: rbp <---------current rbp & current sp
//  current sp - 8:  type
void AssemblerStubsX64::CallNativeTrampoline(ExtendedAssemblerX64 *assembler)
{
    Register glueReg = rax;
    __ Pushq(rbp);
    __ Movq(rsp, rbp);
    __ Movq(rbp, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false))); // save to thread->leaveFrame_
    __ Pushq(static_cast<int32_t>(FrameType::LEAVE_FRAME));

    // callee save
    __ Pushq(r10);
    __ Pushq(rbx);

    __ Movq(rbp, rdx);
    __ Addq(16, rdx); // 16 : for rbp & return address
    // load native pointer address
    __ Movq(Operand(rdx, 0), r10);  // r10 -> argv[0]
    __ Subq(8, rsp); // 8: align 16 bytes
    __ Subq(sizeof(EcmaRuntimeCallInfo), rsp);

    // construct ecma_runtime_call_info
    // get thread
    Register threadReg = rax;
    __ Subq(JSThread::GetGlueDataOffset(), threadReg);
    __ Movq(threadReg, Operand(rsp, 0));   // thread_
    // get numArgs
    __ Movq(0, rax);
    __ Movl(Operand(rdx, 8), rax); // 8: sp + 8 actualArgc
    __ Subl(3, rax); // 3: size
    __ Movq(rax, Operand(rsp, EcmaRuntimeCallInfo::GetNumArgsOffset())); // actualArgc
    // get gpr data
    __ Movq(rdx, rbx);
    __ Addq(16, rbx); // 16: argv[0]
    __ Movq(rbx, Operand(rsp, EcmaRuntimeCallInfo::GetStackArgsOffset())); // argv0
    __ Movq(0, Operand(rsp, EcmaRuntimeCallInfo::GetDataOffset())); // argv1

    __ Movq(rsp, rdi);
    __ Callq(r10);
    __ Addq(ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE, rsp);
    __ Addq(8, rsp); //  8: sp + 8 align 16bytes
    __ Popq(rbx);
    __ Pop(r10);
    __ Addq(8, rsp); // 8: sp + 8
    __ Popq(rbp);
    Register pcReg = rdx;
    __ Popq(pcReg); // load pc
    __ Addq(8, rsp); // 8: skip code address
    __ Pushq(pcReg); // save pc
    __ Ret();
}

// uint64_t JSCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[]);
// c++ calling convention call js function
// Input:
// %rdi - glue
// %rsi - argc
// %rdx - calltarget
// %rcx - argV (calltarget, newtarget, thisObj, ...)
void AssemblerStubsX64::JSCallWithArgv(ExtendedAssemblerX64 *assembler)
{
    Label jsCall;
    Label lJSCallStart;
    Label lNotJSFunction;
    Label lNonCallable;
    Label lJSFunctionCall;
    Label lJSBoundFunction;
    Label lJSProxy;
    Label lCallOptimziedMethod;
    Label lDirectCallCodeEntry;
    Label lCallNativeMethod;
    Label optimizedCallOptimized;
    Label callNativeTrampoline;
    Label lAlign16Bytes2;
    Label lCopyBoundArgument;
    Label lCopyArgument2;
    Label lPushCallTarget;
    Label lCopyBoundArgumentLoop;
    Label lPopFrame2;
    Register glueReg = rdi;
    Register callTarget = rdx;
    Register argvReg = rcx;
    __ Movq(callTarget, Operand(argvReg, 0));
    __ Movq(callTarget, rax);
    __ Jmp(&lJSCallStart);
    __ Bind(&jsCall);
    {
        __ Movq(glueReg, rdi);
        glueReg = rdi;
        __ Movq(Operand(rsp, 16), rax); // 16: get jsFunc
    }
    __ Bind(&lJSCallStart);
    Register jsFuncReg = rax;
    {
        __ Movabs(JSTaggedValue::TAG_INT, rdx); // IsTaggedInt
        __ And(jsFuncReg, rdx);
        __ Cmp(0x0, rdx);
        __ Jne(&lNonCallable);
        __ Cmp(0x0, jsFuncReg); // IsHole
        __ Je(&lNonCallable);
        __ Movabs(JSTaggedValue::TAG_SPECIAL_VALUE, rdx);
        __ And(jsFuncReg, rdx);  // IsSpecial
        __ Cmp(0x0, rdx);
        __ Jne(&lNonCallable);

        __ Movq(jsFuncReg, r8); // save jsFunc
        __ Movq(Operand(jsFuncReg, 0), rax); // get jsHclass
        Register jsHclassReg = rax;
        __ Movl(Operand(jsHclassReg, JSHClass::BIT_FIELD_OFFSET), rax);
        __ Btl(JSHClass::BIT_FIELD1_OFFSET, rax); // IsCallable
        __ Jnb(&lNonCallable);

        __ Shll(24, rax); // objectType << 24
        __ Leal(Operand(rax, -50331649), rdx); // -50331649: disp
        __ Cmpl(0x9FFFFFF, rdx); // 0x9FFFFFF: is Jsfunction
        __ Jae(&lNotJSFunction); // objecttype in (0x04 ~ 0x0c)
        __ Jmp(&lJSFunctionCall);
    }

    __ Bind(&lNotJSFunction);
    {
        __ Cmpl(0xd000000, rax); // 0xd000000: IsJsBoundFunction
        __ Je(&lJSBoundFunction);
        __ Cmpl(0x4f000000, rax); // 0x4f000000: IsJsProxy
        __ Je(&lJSProxy);
    }

    __ Bind(&lNonCallable);
    {
        __ Pushq(rbp);
        __ Movq(rsp, rbp); // set frame pointer
        __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_FRAME)); // set frame type
        __ Movq(MessageString::Message_NonCallable, rax);
        __ Pushq(rax); // message id
        __ Pushq(1); // argc
        __ Pushq(EcmaRuntimeCallerId::RUNTIME_ID_ThrowTypeError); // runtime id
        __ Movq(rcx, rax); // glue
        __ Movq(kungfu::RuntimeStubCSigns::ID_CallRuntime, r10);
        __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
        __ Callq(r10); // call CallRuntime
        __ Movabs(JSTaggedValue::VALUE_EXCEPTION, rax); // return exception
        __ Addq(32, rsp); // 32: sp + 32 argv
        __ Pop(rbp);
        __ Ret();
    }

    __ Bind(&lJSFunctionCall);
    jsFuncReg = r8;
    Register argc = rdx;
    Register methodCallField = rax;
    Register jsMethod = rsi;
    {
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), jsMethod); // get method
        __ Movl(Operand(rsp, 8), argc); // 8: sp + 8 actual argc
        __ Mov(Operand(jsMethod, JSMethod::GetCallFieldOffset(false)), methodCallField); // get call field
        __ Btq(JSMethod::IsNativeBit::SIZE, methodCallField); // is native
        __ Jb(&lCallNativeMethod);
        __ Btq(JSMethod::IsAotCodeBit::SIZE, methodCallField); // is aot
        __ Jb(&lCallOptimziedMethod);
        __ Int3();
        __ Ret();
    }

    __ Bind(&lCallOptimziedMethod);
    Register codeAddrReg = rcx;
    Register expectedNumArgsReg = rsi;
    {
        __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), codeAddrReg); // get codeAddress
        __ Shr(JSMethod::NumArgsBits::START_BIT, methodCallField);
        __ Andl(((1LU <<  JSMethod::NumArgsBits::SIZE) - 1), methodCallField);
        __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argument
        __ Mov(methodCallField, expectedNumArgsReg); // expected numbers
        __ Movq(rsp, r8);
        argvReg = r8;
        __ Addq(16, argvReg); // 16: sp + 8 argv
        __ Cmpl(expectedNumArgsReg, argc); // expectedNumArgs <= actualNumArgs
        __ Jg(&lDirectCallCodeEntry);
        __ Jmp(&optimizedCallOptimized);
    }
    __ Bind(&optimizedCallOptimized);
    OptimizedCallOptimized(assembler);

    __ Bind(&lDirectCallCodeEntry);
    {
        __ Movq(glueReg, rax); // rax = glue
        __ Jmp(codeAddrReg);
    }

    __ Bind(&lCallNativeMethod);
    {
        __ Pop(rax); // pc
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), jsMethod); // Get Method
        Register nativePointer = rsi;
        __ Mov(Operand(jsMethod, JSMethod::GetBytecodeArrayOffset(false)), nativePointer); // get native pointer
        __ Push(nativePointer); // native code address
        __ Push(rax); // pc
        __ Movq(glueReg, rax);
        __ Jmp(&callNativeTrampoline);
    }

    __ Bind(&callNativeTrampoline);
    CallNativeTrampoline(assembler);

    __ Bind(&lJSBoundFunction);
    {
        __ Pushq(rbp);
        __ Movq(rsp, rbp);
        __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_FRAME));
        __ Pushq(r10); // callee save
        __ Movq(rsp, rdx);
        __ Addq(32, rdx); // 32: sp + 32 argv
        __ Mov(Operand(rdx, 0), rax); // get origin argc
        __ Movq(rax, r10);
        // get bound target
        __ Mov(Operand(r8, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rcx);
        // get bound length
        __ Mov(Operand(rcx, TaggedArray::LENGTH_OFFSET), rcx);
        __ Addq(rcx, r10);

        // 16 bytes align check
        __ Testb(1, r10);
        __ Jne(&lAlign16Bytes2);
        __ PushAlignBytes(); // push zero to align 16 bytes stack
    }

    __ Bind(&lAlign16Bytes2);
    {
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, rax);
        __ Cmp(0, rax);
        __ Je(&lCopyBoundArgument);
    }

    __ Bind(&lCopyArgument2);
    {
        __ Movq(Operand(rdx, rax, Scale::Times8, 24), rcx); // 24: slot size
        __ Pushq(rcx);
        __ Addq(-1, rax);
        __ Jne(&lCopyArgument2);

        // get bound target
        __ Mov(Operand(r8, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rdx);
        // get bound length
        __ Mov(Operand(rdx, TaggedArray::LENGTH_OFFSET), rax);
        __ Addq(TaggedArray::DATA_OFFSET, rdx);
    }
    __ Bind(&lCopyBoundArgument);
    {
        __ Cmp(0, rax);
        __ Je(&lPushCallTarget);
    }
    __ Bind(&lCopyBoundArgumentLoop);
    {
        __ Addq(-1, rax);
        __ Movq(Operand(rdx, rax, Scale::Times8, 0), rcx);
        __ Pushq(rcx);
        __ Jne(&lCopyBoundArgumentLoop);
    }
    __ Bind(&lPushCallTarget);
    {
        __ Mov(Operand(r8, JSBoundFunction::BOUND_THIS_OFFSET), rax); // thisObj
        __ Pushq(rax);
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED); // newTarget
        __ Mov(Operand(r8, JSBoundFunction::BOUND_TARGET_OFFSET), rax); // callTarget
        __ Pushq(rax);
        __ Pushq(r10); // push actual arguments
        __ Movq(rdi, rax);
        __ Callq(&jsCall); // call JSCall
        __ Leaq(Operand(r10, Scale::Times8, 8), rcx); // 8: offset
        __ Addq(rcx, rsp);
        __ Testb(1, r10);  // stack 16bytes align check
        __ Jne(&lPopFrame2);
        __ Addq(8, rsp); // 8: sp + 8
    }

    __ Bind(&lPopFrame2);
    {
        __ Pop(r10);
        __ Addq(8, rsp); // 8: sp + 8
        __ Pop(rbp);
        __ Ret();
    }
    __ Bind(&lJSProxy);
    __ Movq(rsp, rcx);
    __ Addq(8, rcx); // 8: sp + 8
    __ Mov(Operand(rcx, 0), rsi); // get origin argc
    __ Movq(r8, rdx);
    __ Addq(8, rcx); // 8: sp + 8 argv
    __ Movq(kungfu::CommonStubCSigns::JsProxyCallInternal, r9);
    __ Movq(Operand(rdi, r9, Scale::Times8, JSThread::GlueData::GetCOStubEntriesOffset(false)), r8);
    __ Jmp(r8);
    __ Ret();
}

// uint64_t JSCall(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new, JSTaggedType this, ...);
// webkit_jscc calling convention call js function()
// Input:
// %rax - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argc
// sp + 16: this
// sp + 8:  new
// sp:      jsfunc
//   +--------------------------+
//   |       ...                |
//   +--------------------------+
//   |       arg0               |
//   +--------------------------+
//   |       this               |
//   +--------------------------+
//   |       new                |
//   +--------------------------+ ---
//   |       jsfunction         |   ^
//   |--------------------------|  Fixed
//   |       argc               | OptimizedFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---
void AssemblerStubsX64::JSCall(ExtendedAssemblerX64 *assembler)
{
    Label jsCall;
    Label lJSCallStart;
    Label lNotJSFunction;
    Label lNonCallable;
    Label lJSFunctionCall;
    Label lJSBoundFunction;
    Label lJSProxy;
    Label lCallOptimziedMethod;
    Label lDirectCallCodeEntry;
    Label lCallNativeMethod;
    Label optimizedCallOptimized;
    Label callNativeTrampoline;
    Label lAlign16Bytes2;
    Label lCopyBoundArgument;
    Label lCopyArgument2;
    Label lPushCallTarget;
    Label lCopyBoundArgumentLoop;
    Label lPopFrame2;
    Register glueReg = rax;
    __ Bind(&jsCall);
    {
        __ Movq(glueReg, rdi);
        glueReg = rdi;
        __ Movq(Operand(rsp, 16), rax); // 16: sp + 16 get jsFunc
    }
    __ Bind(&lJSCallStart);
    Register jsFuncReg = rax;
    {
        __ Movabs(JSTaggedValue::TAG_INT, rdx); // IsTaggedInt
        __ And(jsFuncReg, rdx);
        __ Cmp(0x0, rdx);
        __ Jne(&lNonCallable);
        __ Cmp(0x0, jsFuncReg); // IsHole
        __ Je(&lNonCallable);
        __ Movabs(JSTaggedValue::TAG_SPECIAL_VALUE, rdx);
        __ And(jsFuncReg, rdx);  // IsSpecial
        __ Cmp(0x0, rdx);
        __ Jne(&lNonCallable);

        __ Movq(jsFuncReg, r8); // save jsFunc
        __ Movq(Operand(jsFuncReg, 0), rax); // get jsHclass
        Register jsHclassReg = rax;
        __ Movl(Operand(jsHclassReg, JSHClass::BIT_FIELD_OFFSET), rax);
        __ Btl(JSHClass::BIT_FIELD1_OFFSET, rax); // IsCallable
        __ Jnb(&lNonCallable);

        __ Shll(24, rax); // objectType << 24
        __ Leal(Operand(rax, -50331649), rdx); // -50331649: disp
        __ Cmpl(0x9FFFFFF, rdx); // 0x9FFFFFF: is jsfunction
        __ Jae(&lNotJSFunction); // objecttype in (0x04 ~ 0x0c)
        __ Jmp(&lJSFunctionCall);
    }

    __ Bind(&lNotJSFunction);
    {
        __ Cmpl(0xd000000, rax); // 0xd000000: IsJsBoundFunction
        __ Je(&lJSBoundFunction);
        __ Cmpl(0x4f000000, rax); // 0x4f000000: IsJsProxy
        __ Je(&lJSProxy);
    }

    __ Bind(&lNonCallable);
    {
        __ Pushq(rbp);
        __ Movq(rsp, rbp); // set frame pointer
        __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_FRAME)); // set frame type
        __ Movq(MessageString::Message_NonCallable, rax);
        __ Pushq(rax); // message id
        __ Pushq(1); // argc
        __ Pushq(EcmaRuntimeCallerId::RUNTIME_ID_ThrowTypeError); // runtime id
        __ Movq(rcx, rax); // glue
        __ Movq(kungfu::RuntimeStubCSigns::ID_CallRuntime, r10);
        __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
        __ Callq(r10); // call CallRuntime
        __ Movabs(JSTaggedValue::VALUE_EXCEPTION, rax); // return exception
        __ Addq(32, rsp); // 32: sp + 32 argv
        __ Pop(rbp);
        __ Ret();
    }

    __ Bind(&lJSFunctionCall);
    jsFuncReg = r8;
    Register argc = rdx;
    Register methodCallField = rax;
    Register jsMethod = rsi;
    {
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), jsMethod); // get method
        __ Movl(Operand(rsp, 8), argc); // 8: sp + 8 actual argc
        __ Mov(Operand(jsMethod, JSMethod::GetCallFieldOffset(false)), methodCallField); // get call field
        __ Btq(JSMethod::IsNativeBit::SIZE, methodCallField); // is native
        __ Jb(&lCallNativeMethod);
        __ Btq(JSMethod::IsAotCodeBit::SIZE, methodCallField); // is aot
        __ Jb(&lCallOptimziedMethod);
        __ Int3();
        __ Ret();
    }

    __ Bind(&lCallOptimziedMethod);
    Register codeAddrReg = rcx;
    Register expectedNumArgsReg = rsi;
    {
        __ Mov(Operand(jsFuncReg, JSFunctionBase::CODE_ENTRY_OFFSET), codeAddrReg); // get codeAddress
        __ Shr(JSMethod::NumArgsBits::START_BIT, methodCallField);
        __ Andl(((1LU <<  JSMethod::NumArgsBits::SIZE) - 1), methodCallField);
        __ Addl(NUM_MANDATORY_JSFUNC_ARGS, methodCallField); // add mandatory argument
        __ Mov(methodCallField, expectedNumArgsReg); // expected numbers
        __ Movq(rsp, r8);
        Register argvReg = r8;
        __ Addq(16, argvReg); // 16: sp + 16 argv
        __ Cmpl(expectedNumArgsReg, argc); // expectedNumArgs <= actualNumArgs
        __ Jg(&lDirectCallCodeEntry);
        __ Jmp(&optimizedCallOptimized);
    }
    __ Bind(&optimizedCallOptimized);
    OptimizedCallOptimized(assembler);

    __ Bind(&lDirectCallCodeEntry);
    {
        __ Movq(glueReg, rax); // rax = glue
        __ Jmp(codeAddrReg);
    }

    __ Bind(&lCallNativeMethod);
    {
        __ Pop(rax); // pc
        __ Mov(Operand(jsFuncReg, JSFunctionBase::METHOD_OFFSET), jsMethod); // Get Method
        Register nativePointer = rsi;
        __ Mov(Operand(jsMethod, JSMethod::GetBytecodeArrayOffset(false)), nativePointer); // get native pointer
        __ Push(nativePointer); // native code address
        __ Push(rax); // pc
        __ Movq(glueReg, rax);
        __ Jmp(&callNativeTrampoline);
    }

    __ Bind(&callNativeTrampoline);
    CallNativeTrampoline(assembler);

    __ Bind(&lJSBoundFunction);
    {
        __ Pushq(rbp);
        __ Movq(rsp, rbp);
        __ Pushq(static_cast<int32_t>(FrameType::OPTIMIZED_FRAME));
        __ Pushq(r10); // callee save
        __ Movq(rsp, rdx);
        __ Addq(32, rdx); // 32: sp + 32 argv
        __ Mov(Operand(rdx, 0), rax); // get origin argc
        __ Movq(rax, r10);
        // get bound target
        __ Mov(Operand(r8, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rcx);
        // get bound length
        __ Mov(Operand(rcx, TaggedArray::LENGTH_OFFSET), rcx);
        __ Addq(rcx, r10);

        // 16 bytes align check
        __ Testb(1, r10);
        __ Jne(&lAlign16Bytes2);
        __ PushAlignBytes(); // push zero to align 16 bytes stack
    }

    __ Bind(&lAlign16Bytes2);
    {
        __ Subq(NUM_MANDATORY_JSFUNC_ARGS, rax);
        __ Cmp(0, rax);
        __ Je(&lCopyBoundArgument);
    }

    __ Bind(&lCopyArgument2);
    {
        __ Movq(Operand(rdx, rax, Scale::Times8, 24), rcx); // 24 : disp
        __ Pushq(rcx);
        __ Addq(-1, rax);
        __ Jne(&lCopyArgument2);

        // get bound target
        __ Mov(Operand(r8, JSBoundFunction::BOUND_ARGUMENTS_OFFSET), rdx);
        // get bound length
        __ Mov(Operand(rdx, TaggedArray::LENGTH_OFFSET), rax);
        __ Addq(TaggedArray::DATA_OFFSET, rdx);
    }
    __ Bind(&lCopyBoundArgument);
    {
        __ Cmp(0, rax);
        __ Je(&lPushCallTarget);
    }
    __ Bind(&lCopyBoundArgumentLoop);
    {
        __ Addq(-1, rax);
        __ Movq(Operand(rdx, rax, Scale::Times8, 0), rcx);
        __ Pushq(rcx);
        __ Jne(&lCopyBoundArgumentLoop);
    }
    __ Bind(&lPushCallTarget);
    {
        __ Mov(Operand(r8, JSBoundFunction::BOUND_THIS_OFFSET), rax); // thisObj
        __ Pushq(rax);
        __ Pushq(JSTaggedValue::VALUE_UNDEFINED); // newTarget
        __ Mov(Operand(r8, JSBoundFunction::BOUND_TARGET_OFFSET), rax); // callTarget
        __ Pushq(rax);
        __ Pushq(r10); // push actual arguments
        __ Movq(rdi, rax);
        __ Callq(&jsCall); // call JSCall
        __ Leaq(Operand(r10, Scale::Times8, 8), rcx); // 8: disp
        __ Addq(rcx, rsp);
        __ Testb(1, r10);  // stack 16bytes align check
        __ Jne(&lPopFrame2);
        __ Addq(8, rsp); // 8: align byte
    }

    __ Bind(&lPopFrame2);
    {
        __ Pop(r10);
        __ Addq(8, rsp); // 8: sp + 8
        __ Pop(rbp);
        __ Ret();
    }
    __ Bind(&lJSProxy);
    __ Movq(rsp, rcx);
    __ Addq(8, rcx); // 8: sp + 8
    __ Mov(Operand(rcx, 0), rsi); // get origin argc
    __ Movq(r8, rdx); // 8: slot size
    __ Addq(8, rcx); // argv
    __ Movq(kungfu::CommonStubCSigns::JsProxyCallInternal, r9);
    __ Movq(Operand(rdi, r9, Scale::Times8, JSThread::GlueData::GetCOStubEntriesOffset(false)), r8);
    __ Jmp(r8);
    __ Ret();
}


// uint64_t CallRuntimeWithArgv(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t argv);
// cc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %rdi - glue
// %rsi - runtime_id
// %edx - argc
// %rcx - argv
// stack layout:
//   +--------------------------+
//   |       return addr        |
//   +--------------------------+

// %r8  - argV
// %r9  - codeAddr

// Output:
// construct Leave Frame:
//   +--------------------------+
//   |       returnAddr         |
//   +--------------------------+
//   |       argv[]             |
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|  Fixed
//   |       RuntimeId          | OptimizedLeaveFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---
void AssemblerStubsX64::CallRuntimeWithArgv(ExtendedAssemblerX64 *assembler)
{
    Register glueReg = rdi;
    Register runtimeIdReg = rsi;
    Register argcReg = rdx;
    Register argvReg = rcx;

    __ Movq(rsp, r8);
    Register returnAddrReg = r9;
    __ Movq(Operand(rsp, 0), returnAddrReg);
    __ Pushq(argvReg); // argv[]
    __ Pushq(argcReg); // argc
    __ Pushq(runtimeIdReg); // runtime_id
    __ Pushq(returnAddrReg); // returnAddr

    // construct leave frame
    __ Pushq(rbp);
    __ Movq(rsp, rbp); // set frame pointer
    __ Movq(rbp, Operand(glueReg, JSThread::GlueData::GetLeaveFrameOffset(false))); // save to thread->leaveFrame_
    __ Pushq(static_cast<int32_t>(FrameType::LEAVE_FRAME_WITH_ARGV));

    __ Movq(Operand(glueReg, runtimeIdReg, Scale::Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r9);
    __ Movq(argcReg, rsi); // argc
    __ Movq(argvReg, rdx); // argv
    __ Pushq(r8);
    __ Callq(r9);
    __ Popq(r8);
    __ Addq(8, rsp); // 8: skip type
    __ Popq(rbp);
    __ Movq(r8, rsp);
    __ Ret();
}
#undef __
}  // namespace panda::ecmascript::x64
