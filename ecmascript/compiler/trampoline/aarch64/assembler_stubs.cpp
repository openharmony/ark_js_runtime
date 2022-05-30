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
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
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

    Register frameType(X2);
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

    // construct Leave Frame and callee save
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
    __ Stp(tmp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));

    // load runtime trampoline address
    Register rtfunc(X19);
    __ Ldr(tmp, MemoryOperand(fp, GetStackArgOffSetToFp(0)));
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(tmp, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Ldr(argC, MemoryOperand(fp, GetStackArgOffSetToFp(1)));
    __ Add(argV, fp, Immediate(GetStackArgOffSetToFp(2)));
    __ Blr(rtfunc);

    // callee restore
    __ Ldr(tmp, MemoryOperand(sp, 0));

    // descontruct frame
    // 2 ：2 means stack frame slot size
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
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
    __ Str(Register(X30), MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ CalleeSave();
    __ Str(fp, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(fp, sp);


    Register frameType(X19);
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_ENTRY_FRAME)));
    __ Stp(prevFp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));

    Label copyUndefined;
    Label copyArguments;
    Register tmp(X19, W);
    __ Mov(glue, Register(X0));
    __ Mov(tmp, expectedNumArgs.W());
    __ Cmp(tmp, actualNumArgs.W());
    __ B(Condition::LS, &copyArguments);
    Register count(X9, W);
    Register undefValue(X8);
    __ Mov(count, tmp.W());
    __ Mov(undefValue, Immediate(JSTaggedValue::VALUE_UNDEFINED));

    __ Bind(&copyUndefined);
    __ Sub(count, count, Immediate(1));
    __ Cmp(count, actualNumArgs.W());
    __ Str(undefValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ B(Condition::HI, &copyUndefined);

    Label invokeCompiledJSFunction;
    __ Bind(&copyArguments);
    {
        Register argVEnd(X9);
        Register argC(X8, W);
        Register argValue(X10);
        Label copyArgLoop;

        // expectedNumArgs <= actualNumArgs
        __ Cmp(tmp.W(),  actualNumArgs.W());
        __ CMov(argC, tmp.W(), actualNumArgs.W(), Condition::LO);
        __ Cbz(argC, &invokeCompiledJSFunction);
        __ Sub(argVEnd.W(), argC, Immediate(1));
        __ Add(argVEnd, argV, Operand(argVEnd.W(), UXTW, 3));

        __ Bind(&copyArgLoop);
        __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
        __ Subs(argC, argC, Immediate(1));
        __ Str(argValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ B(Condition::NE, &copyArgLoop);
    }
    __ Bind(&invokeCompiledJSFunction);
    {
        __ Str(actualNumArgs, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ Blr(codeAddr);
    }

    // pop argV argC
    // 3 : 3 means argC * 8
    __ Add(sp, sp, Operand(tmp, UXTW, 3));
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));

    // pop prevLeaveFrameFp to restore thread->currentFrame_
    __ Ldr(prevFp, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Str(prevFp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

    // pop entry frame type and c-fp
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Ldr(fp, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));

    __ CalleeRestore();
    // restore return address
    __ Ldr(Register(X30), MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Ret();
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
void AssemblerStubs::OptimizedCallOptimized(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedCallOptimized));
    Register expectedNumArgs(X1);
    Register sp(SP);
    __ SaveFpAndLr();
    // Construct frame
    Register frameType(X5);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_FRAME)));
    __ Str(frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    // callee save
    Register tmp(X19);
    __ Str(tmp, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    Register count(X5, W);
    Register actualNumArgs(X2, W);
    Label copyArguments;
    __ Mov(count, expectedNumArgs);
    __ Cmp(count, actualNumArgs);
    __ B(Condition::LS, &copyArguments);

    Register undefValue(X8);
    __ Mov(undefValue, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    Label copyUndefined;
    __ Sub(count, count, Immediate(1));
    __ Cmp(count, actualNumArgs);
    __ B(Condition::HI, &copyUndefined);

    Label invokeCompiledJSFunction;
    Register saveNumArgs(X19);
    __ Bind(&copyArguments);
    {
        __ Cmp(expectedNumArgs.W(), actualNumArgs.W());
        __ CMov(count, expectedNumArgs.W(), actualNumArgs.W(), Condition::LO);
        __ Cbz(count, &invokeCompiledJSFunction);

        Register argVEnd(X4);
        Register argValue(X10);
        Label copyArgLoop;
        __ Mov(saveNumArgs, expectedNumArgs);
        __ Sub(count, count, Immediate(1));
        // 3 : 3 means count * 8
        __ Add(argVEnd, argVEnd, Operand(count, UXTW, 3));
        __ Bind(&copyArgLoop);
        __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
        __ Subs(count, count, Immediate(1));
        __ Str(argValue,  MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ B(Condition::NE, &copyArgLoop);
    }

    Register codeAddr(X3);
    __ Bind(&invokeCompiledJSFunction);
    __ Str(actualNumArgs, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Blr(codeAddr);
    // pop argv
    // 3 : 3 means count * 8
    __ Add(sp, sp, Operand(saveNumArgs, UXTW, 3));
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    // callee restore
    __ Ldr(saveNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    // desconstruct frame
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
    __ Ret();
}

// uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %x0 - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argv1
// sp + 16: argv0
// sp + 8:  actualArgc
// sp:      codeAddress
// construct Native Leave Frame:
//   +--------------------------+
//   |       argv0              | calltarget , newtARGET, this, ....
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

void AssemblerStubs::CallBuiltinTrampoline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallBuiltinTrampoline));
    __ SaveFpAndLr();
    // save to thread currentLeaveFrame_;
    Register fp(X29);
    Register glue(X0);
    Register sp(SP);
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

    Register nativeFuncAddr(X19);

    // construct leave frame and callee save
    Register frameType(X1);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
    __ Stp(nativeFuncAddr, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));

    // load runtime trampoline address
    __ Ldr(nativeFuncAddr, MemoryOperand(fp, GetStackArgOffSetToFp(0)));

    // construct ecma_runtime_call_info
    __ Sub(sp, sp, Immediate(sizeof(EcmaRuntimeCallInfo)));
    Register glueToThread(X1);
    Register thread(X0);
    __ Mov(glueToThread, JSThread::GetGlueDataOffset());
    __ Sub(thread, glue, glueToThread);   // thread
    __ Str(thread, MemoryOperand(sp, 0));
    Register argC(X0);
    __ Ldr(argC, MemoryOperand(fp, GetStackArgOffSetToFp(1)));  // argc
    __ Sub(argC, argC, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Str(argC, MemoryOperand(sp, EcmaRuntimeCallInfo::GetNumArgsOffset()));
    Register argV(X0);
    __ Add(argV, fp, Immediate(GetStackArgOffSetToFp(2)));    // argV
    __ Str(argV, MemoryOperand(sp, EcmaRuntimeCallInfo::GetStackArgsOffset()));

    Register callInfo(X0);
    __ Mov(callInfo, sp);
    __ Blr(nativeFuncAddr);

    __ Add(sp, sp, Immediate(sizeof(EcmaRuntimeCallInfo)));

    // descontruct leave frame and callee save register
    __ Ldp(nativeFuncAddr, frameType, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ RestoreFpAndLr();
    __ Add(sp, sp, Immediate(8));
    __ Ret();
}

// uint64_t JSCall(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new, JSTaggedType this, ...);
// webkit_jscc calling convention call js function()
// Input:
// %x0 - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argc
// sp + 16: this
// sp + 8:  new
// sp:      jsfunc
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
//   |       RuntimeId          | OptimizedFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---
void AssemblerStubs::JSCall(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCall));
    Register jsfunc(X1);
    Register sp(SP);
    __ Ldr(jsfunc, MemoryOperand(sp, FRAME_SLOT_SIZE));
    JSCallBody(assembler, jsfunc);
}

void AssemblerStubs::JSCallBody(ExtendedAssembler *assembler, Register jsfunc)
{
    Register sp(SP);
    Register taggedValue(X2);
    Label nonCallable;
    Label notJSFunction;
    __ Mov(taggedValue, JSTaggedValue::TAG_MASK);
    __ Cmp(jsfunc, taggedValue);
    __ B(Condition::HS, &nonCallable);
    __ Cbz(jsfunc, &nonCallable);
    __ Mov(taggedValue, JSTaggedValue::TAG_SPECIAL_VALUE);
    __ And(taggedValue, jsfunc, taggedValue);
    __ Cbnz(taggedValue, &nonCallable);

    Register jshclass(X2);
    __ Ldr(jshclass, MemoryOperand(jsfunc, 0));
    Register bitfield(X2);
    __ Ldr(bitfield, MemoryOperand(jshclass, JSHClass::BIT_FIELD_OFFSET));
    __ Tbz(bitfield, JSHClass::CallableBit::START_BIT, &nonCallable);

    Register jstype(X3, W);
    __ And(jstype, bitfield, LogicalImmediate::Create(0xFF, RegWSize));
    // 4 : 4 means JSType::JS_FUNCTION_BEGIN
    __ Sub(jstype, jstype, Immediate(4));
    // 9 : 9 means JSType::JS_FUNCTION_END - JSType::JS_FUNCTION_BEGIN + 1
    __ Cmp(jstype, Immediate(9));
    __ B(Condition::HS, &notJSFunction);

    Register method(X2);
    Register actualArgC(X3);
    Register callField(X4);
    Label callNativeMethod;
    Label callOptimizedMethod;
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, 0));
    __ Ldr(callField, MemoryOperand(method, JSMethod::GetCallFieldOffset(false)));
    __ Tbnz(callField, JSMethod::IsNativeBit::START_BIT, &callNativeMethod);
    __ Tbnz(callField, JSMethod::IsAotCodeBit::START_BIT, &callOptimizedMethod);
    __ Brk(0);

    __ Bind(&callNativeMethod);
    {
        Register nativeFuncAddr(X4);
        __ Ldr(nativeFuncAddr, MemoryOperand(method, JSMethod::GetNativePointerOffset()));
        // -8 : -8 means sp increase step
        __ Str(nativeFuncAddr, MemoryOperand(sp, -8, AddrMode::PREINDEX));
        __ CallAssemblerStub(RTSTUB_ID(CallBuiltinTrampoline), true);
    }

    __ Bind(&callOptimizedMethod);
    {
        Register expectedNumArgs(X1, W);
        Register arg2(X2);
        Register codeAddress(X3);
        Register argV(X4);
        Label directCallCodeEntry;
        __ Mov(arg2, actualArgC);
        __ Ldr(codeAddress, MemoryOperand(jsfunc, JSFunctionBase::CODE_ENTRY_OFFSET));
        __ Lsr(callField, callField, JSMethod::NumArgsBits::START_BIT);
        __ And(callField.W(), callField.W(),
            LogicalImmediate::Create(JSMethod::NumArgsBits::Mask() >> JSMethod::NumArgsBits::START_BIT, RegWSize));
        __ Add(expectedNumArgs, callField.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        __ Cmp(arg2.W(), expectedNumArgs);
        // 8 : 8 mean argV = sp + 8
        __ Add(argV, sp, Immediate(8));
        __ B(Condition::HS, &directCallCodeEntry);
        __ CallAssemblerStub(RTSTUB_ID(OptimizedCallOptimized), true);
        __ Bind(&directCallCodeEntry);
        __ Br(codeAddress);
    }
    Label jsBoundFunction;
    Label jsProxy;
    __ Bind(&notJSFunction);
    {
        Register jstype2(X5, W);
        __ And(jstype2, bitfield.W(), LogicalImmediate::Create(0xff, RegWSize));
        __ Cmp(jstype2, Immediate(static_cast<int64_t>(JSType::JS_BOUND_FUNCTION)));
        __ B(Condition::EQ, &jsBoundFunction);
        __ Cmp(jstype2, Immediate(static_cast<int64_t>(JSType::JS_PROXY)));
        __ B(Condition::EQ, &jsProxy);
        __ Ret();
    }

    __ Bind(&jsBoundFunction);
    {
        __ SaveFpAndLr();
        // construct frame
        Register frameType(X5);
        Register fp(X29);
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_FRAME)));
        __ Str(frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        Register argVEnd(X4);
        __ Add(argVEnd, fp, Immediate(GetStackArgOffSetToFp(0)));
        __ Ldr(actualArgC, MemoryOperand(argVEnd, 0));
        // callee save
        Register tmp(X19);
        __ Str(tmp, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

        Register boundLength(X2);
        Register realArgC(X19, W);
        Label copyBoundArgument;
        Label copyArgument;
        Label pushCallTarget;
        // get bound arguments
        __ Ldr(boundLength, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
        //  get bound length
        __ Ldr(boundLength, MemoryOperand(boundLength, TaggedArray::LENGTH_OFFSET));
        __ Add(realArgC, boundLength.W(), actualArgC.W());
        // 3 : 3 mean *8
        __ Add(argVEnd, argVEnd, Operand(actualArgC.W(), UXTW, 3));
        __ Sub(actualArgC.W(), actualArgC.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        __ Cmp(actualArgC.W(), Immediate(0));
        __ B(Condition::EQ, &copyBoundArgument);
        __ Bind(&copyArgument);
        {
            Register argValue(X5);
            __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
            __ Str(argValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            __ Sub(actualArgC.W(), actualArgC.W(), Immediate(1));
            __ Cmp(actualArgC.W(), Immediate(0));
            __ B(Condition::NE, &copyArgument);
        }
        __ Bind(&copyBoundArgument);
        {
            Register boundArgs(X4);
            Label copyBoundArgumentLoop;
            __ Ldr(boundArgs, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
            __ Add(boundArgs, boundArgs, Immediate(TaggedArray::DATA_OFFSET));
            __ Cmp(boundLength.W(), Immediate(0));
            __ B(Condition::EQ, &pushCallTarget);
            __ Sub(boundLength.W(), boundLength.W(), Immediate(1));
            // 3 : 3 means 2^3 = 8
            __ Add(boundArgs, boundArgs, Operand(boundLength.W(), UXTW, 3));
            __ Bind(&copyBoundArgumentLoop);
            {
                Register boundargValue(X5);
                __ Ldr(boundargValue, MemoryOperand(boundArgs, -FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
                __ Str(boundargValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
                __ Subs(boundLength.W(), boundLength.W(), Immediate(1));
                __ B(Condition::PL, &copyBoundArgumentLoop);
            }
        }
        __ Bind(&pushCallTarget);
        {
            Register thisObj(X5);
            Register newTarget(X6);
            Register boundTarget(X7);
            __ Ldr(thisObj, MemoryOperand(jsfunc, JSBoundFunction::BOUND_THIS_OFFSET));
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ Stp(newTarget, thisObj, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
            __ Ldr(boundTarget, MemoryOperand(jsfunc, JSBoundFunction::BOUND_TARGET_OFFSET));
            __ Stp(realArgC.X(), boundTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        }
        __ CallAssemblerStub(RTSTUB_ID(JSCall), false);
        __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
        // 3 : 3 means 2^3 = 8
        __ Add(sp, sp, Operand(realArgC, UXTW, 3));
        __ Ldr(tmp, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
        __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
        __ RestoreFpAndLr();
        __ Ret();
    }
    __ Bind(&jsProxy);
    {
        // input: x1(calltarget)
        // output: glue:x0 argc:x1 calltarget:x2 argv:x3
        __ Mov(Register(X2), jsfunc);
        __ Ldr(Register(X1), MemoryOperand(sp, 0));
        __ Add(X3, sp, Immediate(FRAME_SLOT_SIZE));

        Register proxyCallInternalId(Register(X9).W());
        Register codeAddress(X10);
        __ Mov(proxyCallInternalId, Immediate(CommonStubCSigns::JsProxyCallInternal));
        __ Add(codeAddress, X0, Immediate(JSThread::GlueData::GetCOStubEntriesOffset(false)));
        // 3 : 3 means 2 << 3 = 8
        __ Ldr(codeAddress, MemoryOperand(codeAddress, proxyCallInternalId, UXTW, 3));
        __ Br(codeAddress);
    }
    __ Bind(&nonCallable);
    {
        Register frameType(X6);
        Register taggedMessageId(X5);
        __ SaveFpAndLr();
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_FRAME)));
        __ Mov(taggedMessageId,
            Immediate(JSTaggedValue(GET_MESSAGE_STRING_ID(NonCallable)).GetRawData()));
        // 2 : 2 means pair
        __ Stp(taggedMessageId, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        Register argC(X5);
        Register runtimeId(X6);
        __ Mov(argC, Immediate(1));
        __ Mov(runtimeId, RTSTUB_ID(ThrowTypeError));
        // 2 : 2 means pair
        __ Stp(argC, runtimeId, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
        __ CallAssemblerStub(RTSTUB_ID(CallRuntime), false);
        __ Mov(Register(X0), Immediate(JSTaggedValue::VALUE_EXCEPTION));
        // 4 : 4 means stack slot
        __ Add(sp, sp, Immediate(4 * FRAME_SLOT_SIZE));
        __ RestoreFpAndLr();
        __ Ret();
    }
}

void AssemblerStubs::JSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgV));
    Register jsfunc(X1);
    Register argv(X3);
    __ Mov(jsfunc, Register(X2));
    __ Str(jsfunc, MemoryOperand(argv, 0));
    JSCallBody(assembler, jsfunc);
}

void AssemblerStubs::CallRuntimeWithArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallRuntimeWithArgv));
    Register glue(X0);
    Register runtimeId(X1);
    Register argc(X2);
    Register argv(X3);
    Register sp(SP);
    __ Stp(argc, argv, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Str(runtimeId, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ SaveFpAndLr();
    Register fp(X29);
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    // construct leave frame
    Register frameType(X9);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME_WITH_ARGV)));
    __ Str(frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

     // load runtime trampoline address
    Register tmp(X9);
    Register rtfunc(X9);
    // 3 : 3 means 2 << 3 = 8
    __ Add(tmp, glue, Operand(runtimeId, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Mov(X1, argc);
    __ Mov(X2, argv);
    __ Blr(rtfunc);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
    __ Add(sp, sp, Immediate(3 * FRAME_SLOT_SIZE));
    __ Ret();
}

void AssemblerStubs::AsmInterpreterEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AsmInterpreterEntry));
    __ Ret();
}

void AssemblerStubs::JSCallDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallDispatch));
    __ Ret();
}

// void PushCallArgsxAndDispatch(uintptr_t glue, uintptr_t sp, uint64_t callTarget, uintptr_t method,
//     uint64_t callField, ...)
// GHC calling convention
// Input1: for callarg0/1/2/3         Input2: for callrange
// X19 - glue                        // X19 - glue
// X20 - sp                          // X20 - sp
// X21 - callTarget                  // X21 - callTarget
// X22 - method                      // X22 - method
// X23 - callField                   // X23 - callField
// X24 - arg0                        // X24 - actualArgc
// X25 - arg1                        // X25 - argv
// X26 - arg2
void AssemblerStubs::PushCallIThisRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIThisRangeAndDispatch));

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8)));
    CallIThisRangeEntry(assembler);
    __ Ret();
}

void AssemblerStubs::PushCallIRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatch));

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8)));
    CallIRangeEntry(assembler);
    __ Ret();
}

void AssemblerStubs::PushCallArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatch));

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8_V8)));
    Callargs3Entry(assembler);
    __ Ret();
}

void AssemblerStubs::PushCallArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatch));

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8)));
    Callargs2Entry(assembler);
    __ Ret();
}

void AssemblerStubs::PushCallArgs1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs1AndDispatch));

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
    Callarg1Entry(assembler);
    __ Ret();
}

void AssemblerStubs::PushCallArgs0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs0AndDispatch));

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
    PushCallThisUndefined(assembler);  // Callarg0Entry
    __ Ret();
}

// void PushCallArgsxAndDispatchSlowPath(uintptr_t glue, uintptr_t sp, uint64_t callTarget, uintptr_t method,
//       uint64_t callField, ...)
// GHC calling convention
// Input1: for callarg0/1/2/3         Input2: for callrange
// X19 - glue                        // X19 - glue
// X20 - sp                          // X20 - sp
// X21 - callTarget                  // X21 - callTarget
// X22 - method                      // X22 - method
// X23 - callField                   // X23 - callField
// X24 - arg0                        // X24 - actualArgc
// X25 - arg1                        // X25 - argv
// X26 - arg2
void AssemblerStubs::PushCallIThisRangeAndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIThisRangeAndDispatchSlowPath));
    Register callField(X23);
    Register argc(X25);

    Label haveExtraEntry;
    Label pushArgsNoExtraEntry;
    Label pushArgsEntry;

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8)));
    Register declaredNumArgs(X3);
    Register diff(X4);
    Register temp(X5);
    GetDeclaredNumArgsFromCallField(assembler, callField, declaredNumArgs);
    __ Sub(diff.W(), declaredNumArgs.W(), argc.W());
    __ Tbnz(callField, JSMethod::HaveExtraBit::START_BIT, &haveExtraEntry);
    // fall through: no extra
    PushUndefinedWithArgc(assembler, diff, temp, &pushArgsNoExtraEntry);
    __ Bl(&pushArgsNoExtraEntry);

    __ Bind(&haveExtraEntry);
    {
        __ PushArgc(argc, temp);
        PushUndefinedWithArgc(assembler, diff, temp, &pushArgsEntry);
        __ Bl(&pushArgsEntry);
    }

    __ Bind(&pushArgsNoExtraEntry);
    {
        CallIThisRangeNoExtraEntry(assembler, declaredNumArgs);
    }
    __ Bind(&pushArgsEntry);
    {
        CallIThisRangeEntry(assembler);
    }
    __ Ret();
}

void AssemblerStubs::PushCallIRangeAndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatchSlowPath));

    Register callField(X23);
    Register argc(X25);

    Label haveExtraEntry;
    Label pushArgsNoExtraEntry;
    Label pushArgsEntry;

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8)));
    Register declaredNumArgs(X3);
    Register diff(X4);
    Register temp(X5);
    GetDeclaredNumArgsFromCallField(assembler, callField, declaredNumArgs);
    __ Sub(diff.W(), declaredNumArgs.W(), argc.W());
    __ Tbnz(callField, JSMethod::HaveExtraBit::START_BIT, &haveExtraEntry);
    // fall through: no extra
    PushUndefinedWithArgc(assembler, diff, temp, &pushArgsNoExtraEntry);
    __ Bl(&pushArgsNoExtraEntry);

    __ Bind(&haveExtraEntry);
    {
        __ PushArgc(argc, temp);
        PushUndefinedWithArgc(assembler, diff, temp, &pushArgsEntry);
        __ Bl(&pushArgsEntry);
    }

    __ Bind(&pushArgsNoExtraEntry);
    {
        CallIRangeNoExtraEntry(assembler, declaredNumArgs);
    }
    __ Bind(&pushArgsEntry);
    {
        CallIRangeEntry(assembler);
    }
    __ Ret();
}

void AssemblerStubs::PushCallArgs3AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatchSlowPath));

    Register callField(X23);
    constexpr int32_t argc = 3;

    Label haveExtraEntry;
    Label pushArgsNoExtraEntry;
    Label pushArgsEntry;

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8_V8)));
    Register declaredNumArgs(X3);
    Register diff(X4);
    Register temp(X5);
    GetDeclaredNumArgsFromCallField(assembler, callField, declaredNumArgs);
    __ Subs(diff.W(), declaredNumArgs.W(), Immediate(argc));
    __ Tbnz(callField, JSMethod::HaveExtraBit::START_BIT, &haveExtraEntry);
    // fall through: no extra
    PushUndefinedWithArgc(assembler, diff, temp, &pushArgsNoExtraEntry);
    __ Bl(&pushArgsNoExtraEntry);

    __ Bind(&haveExtraEntry);
    {
        __ PushArgc(argc, temp);
        PushUndefinedWithArgc(assembler, diff, temp, &pushArgsEntry);
        __ Bl(&pushArgsEntry);
    }

    __ Bind(&pushArgsNoExtraEntry);
    {
        Callargs3NoExtraEntry(assembler, declaredNumArgs);
    }
    __ Bind(&pushArgsEntry);
    {
        Callargs3Entry(assembler);
    }
    __ Ret();
}

void AssemblerStubs::PushCallArgs2AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatchSlowPath));

    Register callField(X23);
    constexpr int32_t argc = 2;

    Label haveExtraEntry;
    Label pushArgsNoExtraEntry;
    Label pushArgsEntry;

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8)));
    Register declaredNumArgs(X3);
    Register diff(X4);
    Register temp(X5);
    GetDeclaredNumArgsFromCallField(assembler, callField, declaredNumArgs);
    __ Subs(diff.W(), declaredNumArgs.W(), Immediate(argc));
    __ Tbnz(callField, JSMethod::HaveExtraBit::START_BIT, &haveExtraEntry);
    // fall through: no extra
    PushUndefinedWithArgc(assembler, diff, temp, &pushArgsNoExtraEntry);
    __ Bl(&pushArgsNoExtraEntry);

    __ Bind(&haveExtraEntry);
    {
        __ PushArgc(argc, temp);
        PushUndefinedWithArgc(assembler, diff, temp, &pushArgsEntry);
        __ Bl(&pushArgsEntry);
    }

    __ Bind(&pushArgsNoExtraEntry);
    {
        Callargs2NoExtraEntry(assembler, declaredNumArgs);
    }
    __ Bind(&pushArgsEntry);
    {
        Callargs2Entry(assembler);
    }
    __ Ret();
}

void AssemblerStubs::PushCallArgs1AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs1AndDispatchSlowPath));

    Register callField(X23);
    constexpr int32_t argc = 1;

    Label haveExtraEntry;
    Label pushArgsNoExtraEntry;
    Label pushArgsEntry;

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
    Register declaredNumArgs(X3);
    Register diff(X4);
    Register temp(X5);
    GetDeclaredNumArgsFromCallField(assembler, callField, declaredNumArgs);
    __ Subs(diff.W(), declaredNumArgs.W(), Immediate(argc));
    __ Tbnz(callField, JSMethod::HaveExtraBit::START_BIT, &haveExtraEntry);
    // fall through: no extra
    PushUndefinedWithArgc(assembler, diff, temp, &pushArgsNoExtraEntry);
    __ Bl(&pushArgsNoExtraEntry);

    __ Bind(&haveExtraEntry);
    {
        __ PushArgc(argc, temp);
        PushUndefinedWithArgc(assembler, diff, temp, &pushArgsEntry);
        __ Bl(&pushArgsEntry);
    }

    __ Bind(&pushArgsNoExtraEntry);
    {
        Callargs1NoExtraEntry(assembler, declaredNumArgs);
    }
    __ Bind(&pushArgsEntry);
    {
        Callarg1Entry(assembler);
    }
    __ Ret();
}

void AssemblerStubs::PushCallArgs0AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs0AndDispatchSlowPath));

    Register callField(X23);
    constexpr int32_t argc = 0;

    Label haveExtraEntry;
    Label pushArgsNoExtraEntry;
    Label pushArgsEntry;

    SaveFpAndJumpSize(assembler, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
    Register declaredNumArgs(X3);
    Register diff(X4);
    Register temp(X5);
    GetDeclaredNumArgsFromCallField(assembler, callField, declaredNumArgs);
    __ Subs(diff.W(), declaredNumArgs.W(), Immediate(argc));
    __ Tbnz(callField, JSMethod::HaveExtraBit::START_BIT, &haveExtraEntry);
    // fall through: no extra
    PushUndefinedWithArgc(assembler, diff, temp, &pushArgsNoExtraEntry);
    __ Bl(&pushArgsNoExtraEntry);

    __ Bind(&haveExtraEntry);
    {
        __ PushArgc(argc, temp);
        PushUndefinedWithArgc(assembler, diff, temp, &pushArgsEntry);
        __ Bl(&pushArgsEntry);
    }

    __ Bind(&pushArgsNoExtraEntry);
    {
        Callargs0NoExtraEntry(assembler);
    }
    __ Bind(&pushArgsEntry);
    {
        PushCallThisUndefined(assembler);
    }
    __ Ret();
}

// Input:
// X24 - actualArgc
// X25 - argv
void AssemblerStubs::CallIThisRangeNoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgs)
{
    Register argc(X24);
    Register argv(X25);
    Register op(X5);
    Register opArgv(X6);

    Label pushCallThis;
    Register numRegister = declaredNumArgs;
    __ Cmp(declaredNumArgs.W(), argc.W());
    __ CMov(numRegister.W(), declaredNumArgs.W(), argc.W(), Condition::LO);
    __ Mov(opArgv, argv);
    __ PushArgsWithArgv(numRegister, opArgv, op, &pushCallThis);
    __ Bind(&pushCallThis);
    {
        PushCallThis(assembler);
    }
}

// Input:
// X24 - actualArgc
// X25 - argv
void AssemblerStubs::CallIRangeNoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgs)
{
    Register argc(X24);
    Register argv(X25);
    Register op(X5);
    Register opArgv(X6);

    Label pushCallThisUndefined;
    Register numRegister = declaredNumArgs;
    __ Cmp(declaredNumArgs.W(), argc.W());
    __ CMov(numRegister.W(), declaredNumArgs.W(), argc.W(), Condition::LO);
    __ Mov(opArgv, argv);
    __ PushArgsWithArgv(numRegister, opArgv, op, &pushCallThisUndefined);
    __ Bind(&pushCallThisUndefined);
    {
        PushCallThisUndefined(assembler);
    }
}

// Input:
// X24 - arg0
// X25 - arg1
// X26 - arg2
void AssemblerStubs::Callargs3NoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgs)
{
    constexpr int32_t argc = 3;
    Label callargs2NoExtraEntry;
    __ Cmp(declaredNumArgs, Immediate(argc));
    __ B(Condition::LO, &callargs2NoExtraEntry);
    __ Str(Register(X26), MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // arg2
    // fall through
    __ Bind(&callargs2NoExtraEntry);
    Callargs2NoExtraEntry(assembler, declaredNumArgs);
}

// Input:
// X24 - arg0
// X25 - arg1
void AssemblerStubs::Callargs2NoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgs)
{
    constexpr int32_t argc = 2;
    Label callargs1NoExtraEntry;
    __ Cmp(declaredNumArgs, Immediate(argc));
    __ B(Condition::LO, &callargs1NoExtraEntry);
    __ Str(Register(X25), MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // arg1
    // fall through
    __ Bind(&callargs1NoExtraEntry);
    Callargs1NoExtraEntry(assembler, declaredNumArgs);
}

// Input:
// X24 - arg0
void AssemblerStubs::Callargs1NoExtraEntry(ExtendedAssembler *assembler, Register declaredNumArgs)
{
    constexpr int32_t argc = 1;
    Label callargs0NoExtraEntry;
    __ Cmp(declaredNumArgs, Immediate(argc));
    __ B(Condition::LO, &callargs0NoExtraEntry);
    __ Str(Register(X24), MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // arg0
    // fall through
    __ Bind(&callargs0NoExtraEntry);
    Callargs0NoExtraEntry(assembler);
}

void AssemblerStubs::Callargs0NoExtraEntry(ExtendedAssembler *assembler)
{
    PushCallThisUndefined(assembler);
}

// uint64_t PushCallIRangeAndDispatchNative(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[]);
// c++ calling convention call js function
// Input:
// X0 - glue
// X1 - nativeCode
// X2 - callTarget
// X3 - thisValue
// X4  - argc
// X5  - argV (...)
void AssemblerStubs::PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatchNative));

    Register glue(X0);
    Register nativeCode(X1);
    Register callTarget(X2);
    Register thisObj(X3);
    Register argc(X4);
    Register argv(X5);
    Register opArgc(X8);
    Register opArgv(X9);
    Register temp(X10);
    Register stackArgs(X11);
    Register sp(SP);

    Label pushThis;

    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME_WITH_ARGV, temp);

    StackOverflowCheck(assembler);

    __ Str(argc, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(opArgc, argc);
    __ Mov(opArgv, argv);
    __ PushArgsWithArgv(opArgc, opArgv, temp, &pushThis);

    __ Bind(&pushThis);
    __ Str(thisObj, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));    // this
    __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Str(temp, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));       // newTarget
    __ Str(callTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // callTarget
    __ Mov(stackArgs, sp);

    CallNativeInternal(assembler, glue, argc, stackArgs, nativeCode);
    __ Ret();
}

// uint64_t PushCallArgsAndDispatchNative(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// X0 - glue
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
//   |       codeAddress        | BuiltinFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---
void AssemblerStubs::PushCallArgsAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgsAndDispatchNative));

    Register glue(X0);
    Register nativeCode(X1);
    Register argc(X4);
    Register argv(X5);
    Register temp(X6);
    Register fp(X20);

    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME, temp);

    __ Ldr(nativeCode, MemoryOperand(fp, BuiltinFrame::GetNativeCodeToFpDelta(false)));
    __ Ldr(argc, MemoryOperand(fp, BuiltinFrame::GetNumArgsToFpDelta(false)));
    __ Add(argv, fp, Immediate(BuiltinFrame::GetStackArgsToFpDelta(false)));

    CallNativeInternal(assembler, glue, argc, argv, nativeCode);
    __ Ret();
}

void AssemblerStubs::PushBuiltinFrame(ExtendedAssembler *assembler, Register glue, FrameType type, Register op)
{
    Register sp(X20);
    __ Str(sp, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Mov(sp, Register(SP));
    __ Str(sp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Mov(op, Immediate(static_cast<int32_t>(type)));
    __ Str(op, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
}

void AssemblerStubs::CallNativeInternal(ExtendedAssembler *assembler, Register glue, Register numArgs,
    Register stackArgs, Register nativeCode)
{
    GlueToThread(assembler, glue, glue);
    ConstructEcmaRuntimeCallInfo(assembler, glue, numArgs, stackArgs);

    // rsp is ecma callinfo base
    __ Mov(Register(X0), Register(SP));
    __ Blr(nativeCode);
    // resume rsp
    __ Mov(Register(SP), Register(X20));
    __ Ldr(Register(X20), MemoryOperand(Register(SP), FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
}

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// X19 - glue
// X20 - sp
// X21 - pc
// X22 - constantPool
// X23 - profileTypeInfo
// X24 - acc
// X25 - hotnessCounter
// X26 - jumpSizeAfterCall
void AssemblerStubs::ResumeRspAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndDispatch));

    Register glue(X19);
    Register sp(X20);
    Register pc(X21);
    Register jumpSize(X26);
    Register frameState(X5);
    Register opcode(X6, W);
    Register bcStub(X7);

    __ Sub(frameState, sp, Immediate(sizeof(AsmInterpretedFrame)));
    __ Ldr(Register(SP), MemoryOperand(frameState, AsmInterpretedFrame::GetFpOffset(false)));  // resume rsp
    __ Ldr(sp, MemoryOperand(frameState, AsmInterpretedFrame::GetBaseOffset(false)));  // update sp

    __ Add(pc, pc, Operand(jumpSize, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));
    __ Add(bcStub, glue, Operand(opcode, LSL, 3));  // 3： bc * 8
    __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(bcStub);
}

void AssemblerStubs::ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturn));
    Register sp(SP);
    Register lr(X30);
    // 2 ：2 means stack frame slot size
    __ Ldr(lr, MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
    __ Ret();
}

// ResumeCaughtFrameAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter)
// GHC calling convention
// X19 - glue
// X20 - sp
// X21 - pc
// X22 - constantPool
// X23 - profileTypeInfo
// X24 - acc
// X25 - hotnessCounter
void AssemblerStubs::ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeCaughtFrameAndDispatch));

    Register glue(X19);
    Register pc(X21);
    Register fp(X5);
    Register opcode(X6, W);
    Register bcStub(X7);

    Label dispatch;
    __ Ldr(fp, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Cmp(fp, Immediate(0));
    __ CMov(Register(SP), Register(SP), fp, Condition::EQ);
    __ Bind(&dispatch);
    {
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ Add(bcStub, glue, Operand(opcode, LSL, 3));  // 3： bc * 8
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }
}

// ResumeUncaughtFrameAndReturn(uintptr_t glue)
// GHC calling convention
// X19 - glue
void AssemblerStubs::ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeUncaughtFrameAndReturn));

    Register glue(X19);
    Register fp(X5);

    Label ret;

    __ Ldr(fp, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Cmp(fp, Immediate(0));
    __ CMov(Register(SP), Register(SP), fp, Condition::EQ);
    __ Bind(&ret);
    __ Ret();
}

void AssemblerStubs::GeneratorReEnterAsmInterp(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(GeneratorReEnterAsmInterp));
    __ Ret();
}

// Input:
// X24 - actualArgc
// X25 - argv
void AssemblerStubs::CallIThisRangeEntry(ExtendedAssembler *assembler)
{
    Register argc(X24);
    Register argv(X25);
    Register opArgc(X3);
    Register opArgv(X4);
    Register op(X5);
    Label pushCallThis;

    __ Mov(opArgc, argc);
    __ Mov(opArgv, argv);
    __ PushArgsWithArgv(opArgc, opArgv, op, &pushCallThis);
    __ Bind(&pushCallThis);
    PushCallThis(assembler);
}

// Input:
// X23 - callField
// X25 - argv
void AssemblerStubs::PushCallThis(ExtendedAssembler *assembler)
{
    Register callField(X23);
    Register argv(X25);
    Register sp(SP);
    Register thisObj(X5);

    Label pushVregs;
    Label pushNewTarget;
    __ Tst(callField, LogicalImmediate::Create(CALL_TYPE_MASK, RegXSize));
    __ B(Condition::EQ, &pushVregs);
    // fall through
    __ Tbz(callField, JSMethod::HaveThisBit::START_BIT, &pushNewTarget);
    // 8: this is just before the argv list
    __ Ldr(thisObj, MemoryOperand(argv, -8));
    __ Str(thisObj, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // fall through
    __ Bind(&pushNewTarget);
    {
        PushNewTarget(assembler);
    }
    __ Bind(&pushVregs);
    {
        PushVregs(assembler);
    }
}

// Input:
// X24 - actualArgc
// X25 - argv
void AssemblerStubs::CallIRangeEntry(ExtendedAssembler *assembler)
{
    Register argc(X24);
    Register argv(X25);
    Register opArgc(X3);
    Register opArgv(X4);
    Register op(X5);
    Label pushCallThisUndefined;

    __ Mov(opArgc, argc);
    __ Mov(opArgv, argv);
    __ PushArgsWithArgv(opArgc, opArgv, op, &pushCallThisUndefined);
    __ Bind(&pushCallThisUndefined);
    PushCallThisUndefined(assembler);
}

// Input:
// X24 - arg0
// X25 - arg1
// X26 - arg2
void AssemblerStubs::Callargs3Entry(ExtendedAssembler *assembler)
{
    __ Str(Register(X26), MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // arg2
    Callargs2Entry(assembler);
}

// Input:
// X24 - arg0
// X25 - arg1
void AssemblerStubs::Callargs2Entry(ExtendedAssembler *assembler)
{
    __ Str(Register(X25), MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // arg1
    Callarg1Entry(assembler);
}

// Input:
// X24 - arg0
void AssemblerStubs::Callarg1Entry(ExtendedAssembler *assembler)
{
    __ Str(Register(X24), MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // arg0
    PushCallThisUndefined(assembler);
}

// Input:
// X23 - callField
void AssemblerStubs::PushCallThisUndefined(ExtendedAssembler *assembler)
{
    Register callField(X23);
    Register thisObj(X3);

    Label pushVregs;
    Label pushNewTarget;
    __ Tst(callField, LogicalImmediate::Create(CALL_TYPE_MASK, RegXSize));
    __ B(Condition::EQ, &pushVregs);
    // fall through
    __ Tbz(callField, JSMethod::HaveThisBit::START_BIT, &pushNewTarget);
    // push undefined
    __ Mov(thisObj, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Str(thisObj, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // fall through
    __ Bind(&pushNewTarget);
    {
        PushNewTarget(assembler);
    }
    __ Bind(&pushVregs);
    {
        PushVregs(assembler);
    }
}

// Input:
// X23 - callField
void AssemblerStubs::PushNewTarget(ExtendedAssembler *assembler)
{
    Register callField(X23);
    Register newTarget(X6);

    Label pushCallTarget;
    __ Tbz(callField, JSMethod::HaveNewTargetBit::START_BIT, &pushCallTarget);
    // push undefined
    __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Str(newTarget, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Bind(&pushCallTarget);
    PushCallTarget(assembler);
}

// Input:
// X21 - callTarget
// X23 - callField
void AssemblerStubs::PushCallTarget(ExtendedAssembler *assembler)
{
    Register callTarget(X21);
    Register callField(X23);

    Label pushVregs;
    __ Tbz(callField, JSMethod::HaveFuncBit::START_BIT, &pushVregs);
    // push undefined
    __ Str(callTarget, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // fall through
    __ Bind(&pushVregs);
    PushVregs(assembler);
}

// Input:
// X20 - sp
// X21 - callTarget
// X22 - method
// X23 - callField
// X1  - jumpSizeAfterCall
// X2  - fp
void AssemblerStubs::PushVregs(ExtendedAssembler *assembler)
{
    Register prevSp(X20);
    Register callTarget(X21);
    Register method(X22);
    Register callField(X23);
    Register jumpSize(X1);
    Register fp(X2);
    Register pc(X6);
    Register newSp(X7);
    Register numVregs(X8);
    Register temp(X9);

    Label pushFrameState;
    Label dispatchCall;
    GetNumVregsFromCallField(assembler, callField, numVregs);
    PushUndefinedWithArgc(assembler, numVregs, temp, &pushFrameState);
    // fall through
    __ Bind(&pushFrameState);
    {
        __ Mov(newSp, Register(SP));

        StackOverflowCheck(assembler);

        __ Sub(temp, prevSp, Immediate(sizeof(AsmInterpretedFrame)));
        __ Str(jumpSize, MemoryOperand(temp, AsmInterpretedFrame::GetCallSizeOffset(false)));
        PushFrameState(assembler, prevSp, fp, callTarget, method, pc, temp);
        // fall through
    }
    __ Bind(&dispatchCall);
    {
        DispatchCall(assembler, pc, newSp);
    }
}

// Input:
// X19 - glue
// X20 - sp
// X21 - callTarget
// X22 - method
void AssemblerStubs::DispatchCall(ExtendedAssembler *assembler, Register pc, Register newSp)
{
    Register glue(X19);
    Register callTarget(X21);
    Register method(X22);

    Register constantPool(X22);
    Register profileTypeInfo(X23);
    Register acc(X24);
    Register hotnessCounter(X25, W);

    __ Ldrh(hotnessCounter, MemoryOperand(method, JSMethod::GetHotnessCounterOffset(false)));
    __ Mov(acc, Immediate(JSTaggedValue::VALUE_HOLE));
    __ Ldr(profileTypeInfo, MemoryOperand(callTarget, JSFunction::PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(constantPool, MemoryOperand(callTarget, JSFunction::CONSTANT_POOL_OFFSET));
    __ Mov(Register(X21), pc);
    __ Mov(Register(X20), newSp);

    Register bcIndex(X9, W);
    Register temp(X10);
    __ Ldrb(bcIndex, MemoryOperand(pc, 0));
    __ Add(temp, glue, Operand(bcIndex, LSL, 3));  // 3： bc * 8
    __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(temp);
}

void AssemblerStubs::PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fp,
    Register callTarget, Register method, Register pc, Register op)
{
    const AddrMode preIndex = AddrMode::PREINDEX;
    Register sp(SP);
    __ Mov(op, Immediate(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME)));
    __ Stp(prevSp, op, MemoryOperand(sp, -16, preIndex));                 // -16: frame type & prevSp
    __ Ldr(pc, MemoryOperand(method, JSMethod::GetBytecodeArrayOffset(false)));
    __ Stp(fp, pc, MemoryOperand(sp, -16, preIndex));                     // -16: pc & fp
    __ Ldr(op, MemoryOperand(callTarget, JSFunction::LEXICAL_ENV_OFFSET));
    __ Stp(op, Register(Zero), MemoryOperand(sp, -16, preIndex));         // -16: jumpSizeAfterCall & env
    __ Mov(op, Immediate(JSTaggedValue::VALUE_HOLE));
    __ Stp(callTarget, op, MemoryOperand(sp, -16, preIndex));             // -16: acc & callTarget
}

void AssemblerStubs::GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callField, Register numVregs)
{
    __ Mov(numVregs, callField);
    __ Lsr(numVregs, numVregs, JSMethod::NumVregsBits::START_BIT);
    __ And(numVregs.W(), numVregs.W(),
        LogicalImmediate::Create(JSMethod::NumVregsBits::Mask() >> JSMethod::NumVregsBits::START_BIT, RegWSize));
}

void AssemblerStubs::GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callField,
    Register declaredNumArgs)
{
    __ Mov(declaredNumArgs, callField);
    __ Lsr(declaredNumArgs, declaredNumArgs, JSMethod::NumArgsBits::START_BIT);
    __ And(declaredNumArgs.W(), declaredNumArgs.W(),
        LogicalImmediate::Create(JSMethod::NumArgsBits::Mask() >> JSMethod::NumArgsBits::START_BIT, RegWSize));
}

void AssemblerStubs::PushUndefinedWithArgc(ExtendedAssembler *assembler, Register argc, Register temp,
    panda::ecmascript::Label *next)
{
    __ Cmp(argc.W(), Immediate(0));
    __ B(Condition::LE, next);
    Label loopBeginning;
    __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Bind(&loopBeginning);
    __ Str(temp, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Sub(argc.W(), argc.W(), Immediate(1));
    __ Cbnz(argc.W(), &loopBeginning);
}

void AssemblerStubs::SaveFpAndJumpSize(ExtendedAssembler *assembler, Immediate jumpSize)
{
    __ Mov(Register(X1), jumpSize);
    __ Mov(Register(X2), Register(SP));
}

void AssemblerStubs::GlueToThread(ExtendedAssembler *assembler, Register glue, Register thread)
{
    __ Sub(thread, glue, Immediate(JSThread::GetGlueDataOffset()));
}

void AssemblerStubs::ConstructEcmaRuntimeCallInfo(ExtendedAssembler *assembler, Register thread, Register numArgs,
    Register stackArgs)
{
    Register sp(SP);
    __ Sub(sp, sp, Immediate(sizeof(EcmaRuntimeCallInfo)));
    __ Str(thread, MemoryOperand(sp, EcmaRuntimeCallInfo::GetThreadOffset()));
    __ Str(numArgs, MemoryOperand(sp, EcmaRuntimeCallInfo::GetNumArgsOffset()));
    __ Str(stackArgs, MemoryOperand(sp, EcmaRuntimeCallInfo::GetStackArgsOffset()));
}

void AssemblerStubs::StackOverflowCheck([[maybe_unused]] ExtendedAssembler *assembler)
{
}
}  // panda::ecmascript::aarch64