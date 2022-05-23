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
    __ Stp(tmp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));

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
    __ Str(Register(X30), MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
    __ CalleeSave();
    __ Str(fp, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));
    __ Mov(fp, sp);

    
    Register frameType(X19);
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_ENTRY_FRAME)));
    __ Stp(prevFp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));

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
    __ Str(undefValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
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
        __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
        __ Subs(argC, argC, Immediate(1));
        __ Str(argValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
        __ B(Condition::NE, &copyArgLoop);
    }
    __ Bind(&invokeCompiledJSFunction);
    {
        __ Str(actualNumArgs, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
        __ Blr(codeAddr);
    }

    // pop argV argC
    // 3 : 3 means argC * 8
    __ Add(sp, sp, Operand(tmp, UXTW, 3));
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));

    // pop prevLeaveFrameFp to restore thread->currentFrame_
    __ Ldr(prevFp, MemoryOperand(sp, FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
    __ Str(prevFp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    
    // pop entry frame type and c-fp
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Ldr(fp, MemoryOperand(sp, FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));

    __ CalleeRestore();
    // restore return address
    __ Ldr(Register(X30), MemoryOperand(sp, FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
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
    __ Str(frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));

    // callee save
    Register tmp(X19);
    __ Str(tmp, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));

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
        __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
        __ Subs(count, count, Immediate(1));
        __ Str(argValue,  MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
        __ B(Condition::NE, &copyArgLoop);
    }

    Register codeAddr(X3);
    __ Bind(&invokeCompiledJSFunction);
    __ Str(actualNumArgs, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
    __ Blr(codeAddr);
    // pop argv
    // 3 : 3 means count * 8
    __ Add(sp, sp, Operand(saveNumArgs, UXTW, 3));
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    // callee restore
    __ Ldr(saveNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
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
    __ Stp(nativeFuncAddr, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));
    
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
    __ Ldp(nativeFuncAddr, frameType, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
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
    JSCallStart(assembler, jsfunc);
}

void AssemblerStubs::JSCallStart(ExtendedAssembler *assembler, Register jsfunc)
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
        __ Str(nativeFuncAddr, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));
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
        __ Str(frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
        Register argVEnd(X4);
        __ Add(argVEnd, fp, Immediate(GetStackArgOffSetToFp(0)));
        __ Ldr(actualArgC, MemoryOperand(argVEnd, 0));
        // callee save
        Register tmp(X19);
        __ Str(tmp, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));

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
            __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
            __ Str(argValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
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
                __ Ldr(boundargValue, MemoryOperand(boundArgs, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
                __ Str(boundargValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, MemoryOperand::AddrMode::PREINDEX));
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
            __ Stp(newTarget, thisObj, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));
            __ Ldr(boundTarget, MemoryOperand(jsfunc, JSBoundFunction::BOUND_TARGET_OFFSET));
            __ Stp(realArgC.X(), boundTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));
        }
        __ CallAssemblerStub(RTSTUB_ID(JSCall), false);
        __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
        // 3 : 3 means 2^3 = 8 
        __ Add(sp, sp, Operand(realArgC, UXTW, 3));
        __ Ldr(tmp, MemoryOperand(sp, FRAME_SLOT_SIZE, MemoryOperand::AddrMode::POSTINDEX));
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
        __ Add(X3, sp, Immediate(8));

        Register proxyCallInternalId(Register(X9).W());
        Register codeAddress(X10);
        __ Mov(proxyCallInternalId, Immediate(CommonStubCSigns::JsProxyCallInternal));
        __ Add(codeAddress, X0, Immediate(JSThread::GlueData::GetCOStubEntriesOffset(false)));
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
        __ Stp(taggedMessageId, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));
        Register argC(X5);
        Register runtimeId(X6);
        __ Mov(argC, Immediate(1));
        __ Mov(runtimeId, RTSTUB_ID(ThrowTypeError));
        // 2 : 2 means pair
        __ Stp(argC, runtimeId, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, MemoryOperand::AddrMode::PREINDEX));
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
    __ Ldr(jsfunc, MemoryOperand(argv, 0));
    JSCallStart(assembler, jsfunc);
}

void AssemblerStubs::CallRuntimeWithArgv(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallRuntimeWithArgv));
    Register returnAddr(X9);
    Register glue(X0);
    Register runtimeId(X1);
    Register argc(X2);
    Register argv(X3);
    Register sp(SP);
    __ Ldr(returnAddr, MemoryOperand(sp, 8));
    __ Stp(argc, argv, MemoryOperand(sp, -16, MemoryOperand::AddrMode::PREINDEX));
    __ Stp(returnAddr, runtimeId, MemoryOperand(sp, -16, MemoryOperand::AddrMode::PREINDEX));
    // construct leave frame
    Register frameType(X9);
    Register fp(X29);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME_WITH_ARGV)));
    __ Str(frameType, MemoryOperand(sp, -8, MemoryOperand::AddrMode::PREINDEX));

     // load runtime trampoline address
    Register tmp(X9);
    Register rtfunc(X9);
    __ Add(tmp, glue, Operand(runtimeId, LSL, 3));
    __ Ldr(rtfunc, MemoryOperand(tmp, JSThread::GlueData::GetRTStubEntriesOffset(false)));
    __ Mov(X1, argc);
    __ Mov(X2, argv);
    __ Blr(rtfunc);
    __ Add(sp, sp, Immediate(8 * 7));

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

void AssemblerStubs::PushCallIThisRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIThisRangeAndDispatch));
    __ Ret();
}

void AssemblerStubs::PushCallIRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatch));
    __ Ret();
}

void AssemblerStubs::PushCallArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatch));
    __ Ret();
}

void AssemblerStubs::PushCallArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatch));
    __ Ret();
}

void AssemblerStubs::PushCallArgs1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs1AndDispatch));
    __ Ret();
}

void AssemblerStubs::PushCallArgs0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs0AndDispatch));
    __ Ret();
}

void AssemblerStubs::PushCallIThisRangeAndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIThisRangeAndDispatchSlowPath));
    __ Ret();
}

void AssemblerStubs::PushCallIRangeAndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatchSlowPath));
    __ Ret();
}

void AssemblerStubs::PushCallArgs3AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatchSlowPath));
    __ Ret();
}

void AssemblerStubs::PushCallArgs2AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatchSlowPath));
    __ Ret();
}

void AssemblerStubs::PushCallArgs1AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs1AndDispatchSlowPath));
    __ Ret();
}

void AssemblerStubs::PushCallArgs0AndDispatchSlowPath(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs0AndDispatchSlowPath));
    __ Ret();
}


void AssemblerStubs::PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatchNative));
    __ Ret();
}

void AssemblerStubs::PushCallArgsAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgsAndDispatchNative));
    __ Ret();
}

void AssemblerStubs::ResumeRspAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndDispatch));
    __ Ret();
}

void AssemblerStubs::ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturn));
    __ Ret();
}

void AssemblerStubs::ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeCaughtFrameAndDispatch));
    __ Ret();
}

void AssemblerStubs::GeneratorReEnterAsmInterp(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(GeneratorReEnterAsmInterp));
    __ Ret();
}
}  // panda::ecmascript::aarch64