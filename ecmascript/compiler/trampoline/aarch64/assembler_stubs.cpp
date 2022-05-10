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

#include "assembler_stubs.h"

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

namespace panda::ecmascript::aarch64 {
#define __ assembler->

// uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %x0 - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argv1
// sp + 16: argv0
// sp + 8:  argc
// sp:      runtime_id
// construct Leave Frame:
//   +--------------------------+
//   |       argv[argc-1]       |
//   +--------------------------+
//   |       ..........         |
//   +--------------------------+
//   |       argv[1]            |
//   +--------------------------+
//   |       argv[0]            |
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
void AssemblerStubs::CallRuntime(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register fp(X29);
    Register tmp(X19);
    Register sp(SP);
    Register argC(X1);
    Register argV(X2);
    
    __ BindAssemblerStub(RTSTUB_ID(CallRuntime));
    __ SaveFpAndLr();

    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    // callee save
    __ Str(tmp, MemoryOperand(sp, -16, MemoryOperand::AddrMode::PREINDEX));
    
    // construct Leave Frame
    __ Mov(tmp, FrameType::LEAVE_FRAME);  
    __ Str(tmp, MemoryOperand(sp, -8));
    __ Add(sp, sp, Immediate(-16));

    // load runtime trampoline address
    Register rtfunc(X19);
    __ Ldr(tmp, MemeoryOperand(fp, 16));
    __ Add(tmp, glue, Operand(tmp, LSL, 3));
    __ Ldr(rtfunc, MemeoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Ldr(argC, MemeoryOperand(fp, 24));
    __ Add(x2, fp, Immediate(32));
    __ Blr(rtfunc);

    // callee restore
    __ Ldr(tmp, MemeoryOperand(sp, 0));
    // descontruct frame
    __ Add(sp, sp, 16);
    __ RestoreFpAndLr();
    __ Ret();
}

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
// Input:
// %x0 - glue
// %x1 - prevFp
// %x2 - expectedNumArgs
// %x3 - actualNumArgs
// %x4 - argV
// %x5 - codeAddr
// construct Entry Frame
//   +--------------------------+
//   |   returnaddress      |   ^
//   |----------------------|   |
//   |calleesave registers  | Fixed
//   |----------------------| OptimizedEntryFrame
//   |      prevFp          |   |
//   |----------------------|   |
//   |      frameType       |   |
//   |----------------------|   |
//   |  prevLeaveFrameFp    |   v
//   +--------------------------+
void AssemblerStubs::JSFunctionEntry(ExtendedAssembler *assembler)
{
    Register glue(X20);
    Register prevFp(X1);
    Register expectedNumArgs(X2);
    Register actualNumArgs(X3);
    Register argV(X4);
    Register codeAddr(X5);
    Register sp(SP);
    Register fp(X29);

    __ BindAssemblerStub(RTSTUB_ID(JSFunctionEntry));
    __ Str(Register(X30), MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));
    __ CalleeSave();
    __ Str(fp, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));
    __ Mov(fp, sp);

    
    Register frameType(X19);
    // construct frame
    __ Mov(frameType, FrameType::OPTIMIZED_ENTRY_FRAME);
    __ Stp(prevFp, frameType, MemoryOperand(sp, -16, MemoryOperand::AddrMode::PREINDEX));

    Label copyUndefined;
    Label copyArguments;
    Register tmp(X19, true);
    __ Mov(glue, Register(X0));
    __ Mov(tmp, expectedNumArgs.W());
    __ Cmp(tmp, actualNumArgs.W());
    __ B(Condition::LS, &copyUndefined);
    Register count(X9, true);
    Register undefValue(x8)
    __ Mov(count, tmp.W());
    __ Mov(x9, JSTaggedValue::Undefined());
    
    __ Bind(&copyUndefined);
    __ Sub(count, count, Immediate(-1));
    __ Cmp(count, actualNumArgs.W());
    __ Str(undefValue, MemoryOperand(sp, -8));
    __ B(Condition::HI, &copyUndefined);
    
    Label invokeCompiledJSFunction;
    __ Bind(&copyArguments);
    {
        Register argVEnd(X9);
        Register argC(X8, true);
        Register argValue(X10);
        Label copyArgLoop;

        // expectedNumArgs <= actualNumArgs
        __ Cmp(tmp.W(),  actualNumArgs.W());
        __ CMov(argC, tmp.W(), actualNumArgs.W(), Condition::LO);
        __ Cbz(argC, invokeCompiledJSFunction);
        __ Sub(argVEnd.W(), argC, Immediate(-1));
        __ Add(argVEnd, argV, Operand(argVEnd.W(), UXTW, 3));
        
        __ Bind(&copyArgLoop);
        __ Ldr(argValue, MemoryOperand(argVEnd, -8, MemoryOperand::AddrMode::POSTINDEX));
        __ Subs(argC, argC, 1);
        __ Str(argValue, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));
        __ B(Condition::NE, &copyArgLoop);
    }
    __ Bind(&invokeCompiledJSFunction);
    {
        __ Str(actualNumArgs, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));
        __ Blr(codeAddr);
    }

    // pop argV argC
    __ Add(sp, sp, Operand(tmp, UXTW, 3));
    __ Add(sp, sp, Immediate(8));

    // pop prevLeaveFrameFp to restore thread->currentFrame_
    __ Ldr(prevFp, MemoryOperand(sp, 8, MemoryOperand::AddrMode::POSTINDEX));
    __ Str(prevFp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    
    // pop entry frame type and c-fp
    __ Add(sp, sp, 8);
    __ Ldr(fp, MemoryOperand(sp, 8, MemoryOperand::AddrMode::POSTINDEX));

    CalleRestore();
    // restore return address
    __ Ldr(Register(X30), MemoryOperand(sp, 8, MemoryOperand::AddrMode::POSTINDEX));
    Ret();
}

// extern "C" JSTaggedType OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
//                                  uint32_t actualNumArgs, uintptr_t codeAddr, uintptr_t argv)
// Input:
// %x0 - glue
// %w1 - expectedNumArgs
// %w2 - actualNumArgs
// %x3 - codeAddr
// %x4 - argv

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
// sp[- 8(N)]     - argc
static void AssemblerStubs::OptimizedCallOptimized(ExtendedAssembler *assembler)
{
    __ SaveFpAndLr();
    // Construct frame
    Register frameType(X5);
    
}

OptimizedCallOptimized:
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp
    mov     x5, #OPTIMIZE_FRAME_TYPE
    str     x5, [sp, #-8]!
    // callee save
    str     x19, [sp, #-8]!

    mov     w5, w1
    // expectedNumArgs <= actualNumArgs
    cmp     w5, w2
    b.ls    .LCopyArguments1
    // undefined
    mov     x8, #JSUNDEFINED

.LCopyUndefined1:
    sub     x5, x5, #1
    cmp     w5, w2
    str     x8, [sp, #-8]!
    b.hi    .LCopyUndefined1
.LCopyArguments1:
    // w8 = min(expectedNumArgs, actualNumArgs)
    cmp     w1, w2
    csel    w5, w1, w2, lo
    cbz     w5, .InvokeCompiledJSFunction1
    mov     x19, x1                 // save expected numArgs
    sub     w5, w5, #1
    add     x5, x4, w5, uxtw #3
.LCopyArgLoop1:
    ldr     x10, [x4], #-8
    subs    w5, w5, #1
    str     x10, [sp, #-8]!

    b.ne    .LCopyArgLoop1

// Input:
// %x0 - glue
// argv push stack
.InvokeCompiledJSFunction1:
    str     x2, [sp, #-8]!
    blr     x3

    // pop argv
    add     sp, sp, w19, uxtw #3
    add     sp, sp, #8
    // callee restore
    ldr     x19, [sp], #8
    // deconstruct frame
    add     sp, sp, #8
    ldp     x29, x30, [sp], #16
    ret


}  // panda::ecmascript::aarch64