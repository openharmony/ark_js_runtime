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
#include "ecmascript/js_generator_object.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

// uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %x0 - glue
// stack layout: sp + N*8 argvN
//               ........
//               sp + 24: argv1
//               sp + 16: argv0
// sp + 8:       argc
// sp:           runtime_id
// construct Leave Frame
//               +--------------------------+
//               |       argv[argc-1]       |
//               +--------------------------+
//               |       ..........         |
//               +--------------------------+
//               |       argv[1]            |
//               +--------------------------+
//               |       argv[0]            |
//               +--------------------------+ ---
//               |       argc               |   ^
//               |--------------------------|  Fixed
//               |       RuntimeId          | OptimizedLeaveFrame
//               |--------------------------|   |
//               |       returnAddr         |   |
//               |--------------------------|   |
//               |       callsiteFp         |   |
//               |--------------------------|   |
//               |       frameType          |   v
//               +--------------------------+ ---
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
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr)
// Input: %x0 - glue
//        %x1 - prevFp
//        %x2 - expectedNumArgs
//        %x3 - actualNumArgs
//        %x4 - argV
//        %x5 - codeAddr
// construct Entry Frame
//        +--------------------------+
//        |   returnaddress      |   ^
//        |----------------------|   |
//        |calleesave registers  | Fixed
//        |----------------------| OptimizedEntryFrame
//        |      prevFp          |   |
//        |----------------------|   |
//        |      frameType       |   |
//        |----------------------|   |
//        |  prevLeaveFrameFp    |   v
//        +--------------------------+
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
// Input:  %x0 - glue
//         %w1 - expectedNumArgs
//         %w2 - actualNumArgs
//         %x3 - codeAddr
//         %x4 - argv

//         sp[0 * 8]  -  argc
//         sp[1 * 8]  -  argv[0]
//         sp[2 * 8]  -  argv[1]
//         .....
//         sp[(N -3) * 8] - argv[N - 1]
// Output: stack as followsn from high address to lowAdress
//         sp       -      argv[N - 1]
//         sp[-8]    -      argv[N -2]
//         ...........................
//         sp[- 8(N - 1)] - arg[0]
//         sp[- 8(N)]     - argc
void AssemblerStubs::OptimizedCallOptimized(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(OptimizedCallOptimized));
    Register expectedNumArgs(X1);
    Register sp(SP);
    __ SaveFpAndLr();
    // Construct frame
    Register frameType(X5);
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
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
    __ Str(undefValue, MemoryOperand(sp, -FRAME_SLOT_SIZE, PREINDEX));
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
        __ Subs(count, count, Immediate(1));
        // 3 : 3 means count * 8
        __ Add(argVEnd, argVEnd, Operand(count, UXTW, 3));
        __ Bind(&copyArgLoop);
        __ Ldr(argValue, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
        __ Subs(count, count, Immediate(1));
        __ Str(argValue,  MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ B(Condition::PL, &copyArgLoop);
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

// uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %x0 - glue
// stack layout: sp + N*8 argvN
//               ........
//               sp + 24: argv1
//               sp + 16: argv0
//               sp + 8:  actualArgc
//               sp:      codeAddress
// construct Native Leave Frame
//               +--------------------------+
//               |       argv0              | calltarget , newtARGET, this, ....
//               +--------------------------+ ---
//               |       argc               |   ^
//               |--------------------------|  Fixed
//               |       codeAddress        | OptimizedLeaveFrame
//               |--------------------------|   |
//               |       returnAddr         |   |
//               |--------------------------|   |
//               |       callsiteFp         |   |
//               |--------------------------|   |
//               |       frameType          |   v
//               +--------------------------+ ---

// Output:       sp - 8 : pc
//               sp - 16: rbp <---------current rbp & current sp
//               current sp - 8:        type

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
    // 2 : 2 means pair
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
    // 1 : 1 means argC id
    __ Ldr(argC, MemoryOperand(fp, GetStackArgOffSetToFp(1)));  // argc
    __ Sub(argC, argC, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Str(argC, MemoryOperand(sp, EcmaRuntimeCallInfo::GetNumArgsOffset()));
    Register argV(X0);
    // 2 : 2 means argV id
    __ Add(argV, fp, Immediate(GetStackArgOffSetToFp(2)));    // argV
    __ Str(argV, MemoryOperand(sp, EcmaRuntimeCallInfo::GetStackArgsOffset()));

    Register callInfo(X0);
    __ Mov(callInfo, sp);
    __ Blr(nativeFuncAddr);

    __ Add(sp, sp, Immediate(sizeof(EcmaRuntimeCallInfo)));

    // descontruct leave frame and callee save register
    __ Ldp(nativeFuncAddr, frameType, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ RestoreFpAndLr();
    __ Add(sp, sp, Immediate(8)); // 8 : 8 skip native code address
    __ Ret();
}

// uint64_t JSCall(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new, JSTaggedType this, ...)
// webkit_jscc calling convention call js function()
// %x0 - glue
// stack layout
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
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
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
            // 2 : 2 means pair
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
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
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
    // 2 : 2 means pair
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
    __ Add(sp, sp, Immediate(3 * FRAME_SLOT_SIZE)); // 3 : 3 means pair
    __ Ret();
}

// Generate code for entering asm interpreter
// c++ calling convention
// Input: %X0 - glue
//        %X1 - argc
//        %X2 - argv(<callTarget, newTarget, this> are at the beginning of argv)
void AssemblerStubs::AsmInterpreterEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AsmInterpreterEntry));

    PushAsmInterpEntryFrame(assembler, true);

    __ CallAssemblerStub(RTSTUB_ID(JSCallDispatch), false);

    PopAsmInterpEntryFrame(assembler, true);
    __ Ret();
}

// Input: glueRegister   - %X0
//        argcRegister   - %X1
//        argvRegister   - %X2(<callTarget, newTarget, this> are at the beginning of argv)
//        prevSpRegister - %X29
void AssemblerStubs::JSCallDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallDispatch));
    Label notJSFunction;
    Label callNativeEntry;
    Label callJSFunctionEntry;
    Label pushArgsSlowPath;
    Label callJSProxyEntry;
    Label notCallable;
    Register glueRegister(X0);
    Register argcRegister(X1, W);
    Register argvRegister(X2);
    Register prevSpRegister(X29);

    Register callTargetRegister(X3);
    Register methodRegister(X4);
    Register bitFieldRegister(X5);
    Register tempRegister(X6); // can not be used to store any variable
    Register functionTypeRegister(X7, W);
    __ Ldr(callTargetRegister, MemoryOperand(argvRegister, 0));
    __ Ldr(tempRegister, MemoryOperand(callTargetRegister, 0)); // hclass
    __ Ldr(bitFieldRegister, MemoryOperand(tempRegister, JSHClass::BIT_FIELD_OFFSET));
    __ And(functionTypeRegister, bitFieldRegister.W(), LogicalImmediate::Create(0xFF, RegWSize));
    __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_FUNCTION_BEGIN)));
    __ Cmp(functionTypeRegister, tempRegister.W());
    __ B(Condition::LO, &notJSFunction);
    __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_FUNCTION_END)));
    __ Cmp(functionTypeRegister, tempRegister.W());
    __ B(Condition::LS, &callJSFunctionEntry);
    __ Bind(&notJSFunction);
    {
        __ Tst(bitFieldRegister,
            LogicalImmediate::Create(static_cast<int64_t>(1ULL << JSHClass::CallableBit::START_BIT), RegXSize));
        __ B(Condition::EQ, &notCallable);
        __ Mov(tempRegister.W(), Immediate(static_cast<int64_t>(JSType::JS_PROXY)));
        __ Cmp(functionTypeRegister, tempRegister.W());
        __ B(Condition::EQ, &callJSProxyEntry);
        // bound function branch, default native
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSFunctionBase::METHOD_OFFSET));
        // fall through
    }
    __ Bind(&callNativeEntry);
    CallNativeEntry(assembler);
    __ Bind(&callJSProxyEntry);
    {
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSProxy::METHOD_OFFSET));
        __ B(&callNativeEntry);
    }
    __ Bind(&callJSFunctionEntry);
    {
        __ Ldr(methodRegister, MemoryOperand(callTargetRegister, JSFunctionBase::METHOD_OFFSET));
        Register callFieldRegister(X7);
        __ Ldr(callFieldRegister, MemoryOperand(methodRegister, JSMethod::GetCallFieldOffset(false)));
        __ Tbnz(callFieldRegister, JSMethod::IsNativeBit::START_BIT, &callNativeEntry);
        Register declaredNumArgsRegister(X9);
        GetDeclaredNumArgsFromCallField(assembler, callFieldRegister, declaredNumArgsRegister);
        __ Cmp(declaredNumArgsRegister.W(), argcRegister);
        __ B(Condition::NE, &pushArgsSlowPath);
        // fast path
        __ PushFpAndLr();
        Register fpRegister(X10);
        __ Mov(fpRegister, Register(SP));
        PushArgsFastPath(assembler, glueRegister, argcRegister, argvRegister, callTargetRegister, methodRegister,
            prevSpRegister, fpRegister, callFieldRegister);
        __ Bind(&pushArgsSlowPath);
        PushArgsSlowPath(assembler, glueRegister, declaredNumArgsRegister, argcRegister, argvRegister,
            callTargetRegister, methodRegister, prevSpRegister, callFieldRegister);
    }
    __ Bind(&notCallable);
    {
        Register runtimeId(X11);
        Register trampoline(X12);
        __ Mov(runtimeId, Immediate(kungfu::RuntimeStubCSigns::ID_ThrowNotCallableException));
        // 3 : 3 means *8
        __ Add(trampoline, glueRegister, Operand(runtimeId, LSL, 3));
        __ Ldr(trampoline, MemoryOperand(trampoline, JSThread::GlueData::GetRTStubEntriesOffset(false)));
        __ Blr(trampoline);
        __ Ret();
    }
}

void AssemblerStubs::JSCallCommonEntry(ExtendedAssembler *assembler, JSCallMode mode)
{
    Register fpRegister = __ AvailableRegister1();
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    // save fp
    __ Mov(fpRegister, Register(SP));

    auto jumpSize = kungfu::AssemblerModule::GetJumpSizeFromJSCallMode(mode);
    if (jumpSize > 0) {
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register temp = __ TempRegister1();
        __ Mov(temp, Immediate(static_cast<int>(jumpSize)));
        int64_t offset = static_cast<int64_t>(AsmInterpretedFrame::GetCallSizeOffset(false))
            - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
        ASSERT(offset < 0);
        __ Stur(temp, MemoryOperand(Register(FP), offset));
    }
    Register declaredNumArgsRegister = __ AvailableRegister2();
    GetDeclaredNumArgsFromCallField(assembler, callFieldRegister, declaredNumArgsRegister);

    if (mode == JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV) {
        Register thisRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register tempArgcRegister = __ TempRegister1();
        __ PushArgc(argcRegister, tempArgcRegister);
        __ Str(thisRegister, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }

    Label slowPathEntry;
    Label fastPathEntry;
    Label pushCallThis;
    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    if (argc >= 0) {
        __ Cmp(declaredNumArgsRegister, Immediate(argc));
    } else {
        __ Cmp(declaredNumArgsRegister, argcRegister);
    }
    __ B(Condition::NE, &slowPathEntry);
    __ Bind(&fastPathEntry);
    JSCallCommonFastPath(assembler, mode, &pushCallThis);
    __ Bind(&pushCallThis);
    PushCallThis(assembler, mode);
    __ Bind(&slowPathEntry);
    JSCallCommonSlowPath(assembler, mode, &fastPathEntry, &pushCallThis);
}

void AssemblerStubs::JSCallCommonFastPath(ExtendedAssembler *assembler, JSCallMode mode, Label *pushCallThis)
{
    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    // call range
    if (argc < 0) {
        Register numRegister = __ AvailableRegister2();
        Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
        Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGV);
        __ Mov(numRegister, argcRegister);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register opRegister = __ TempRegister1();
        __ PushArgsWithArgv(numRegister, argvRegister, opRegister, pushCallThis);
    } else if (argc > 0) {
        Register arg0 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
        Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
        Register arg2 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        if (argc > 2) { // 2: call arg2
            __ Str(arg2, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        if (argc > 1) {
            __ Str(arg1, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        if (argc > 0) {
            __ Str(arg0, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
    }
}

void AssemblerStubs::JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
                                          Label *fastPathEntry, Label *pushCallThis)
{
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGV);

    Register arg0 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
    Label noExtraEntry;
    Label pushArgsEntry;

    auto argc = kungfu::AssemblerModule::GetArgcFromJSCallMode(mode);
    Register declaredNumArgsRegister = __ AvailableRegister2();
    __ Tbz(callFieldRegister, JSMethod::HaveExtraBit::START_BIT, &noExtraEntry);
    // extra entry
    {
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register tempArgcRegister = __ TempRegister1();
        if (argc >= 0) {
            __ PushArgc(argc, tempArgcRegister);
        } else {
            __ PushArgc(argcRegister, tempArgcRegister);
        }
        // fall through
    }
    __ Bind(&noExtraEntry);
    {
        if (argc == 0) {
            {
                [[maybe_unused]] TempRegister1Scope scope(assembler);
                Register tempRegister = __ TempRegister1();
                PushUndefinedWithArgc(assembler, declaredNumArgsRegister, tempRegister, nullptr);
            }
            __ Bl(fastPathEntry);
            return;
        }
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register diffRegister = __ TempRegister1();
        if (argc >= 0) {
            __ Sub(diffRegister.W(), declaredNumArgsRegister.W(), Immediate(argc));
        } else {
            __ Sub(diffRegister.W(), declaredNumArgsRegister.W(), argcRegister.W());
        }
        [[maybe_unused]] TempRegister2Scope scope2(assembler);
        Register tempRegister = __ TempRegister2();
        PushUndefinedWithArgc(assembler, diffRegister, tempRegister, &pushArgsEntry);
        __ Bl(fastPathEntry);
    }
    // declare < actual
    __ Bind(&pushArgsEntry);
    {
        __ Tbnz(callFieldRegister, JSMethod::HaveExtraBit::START_BIT, fastPathEntry);
        // no extra branch
        // arg1, declare must be 0
        if (argc == 1) {
            __ Bl(pushCallThis);
            return;
        }
        __ Cmp(declaredNumArgsRegister, Immediate(0));
        __ B(Condition::EQ, pushCallThis);
        // call range
        if (argc < 0) {
            [[maybe_unused]] TempRegister1Scope scope(assembler);
            Register opRegister = __ TempRegister1();
            __ PushArgsWithArgv(declaredNumArgsRegister, argvRegister, opRegister, nullptr);
        } else if (argc > 0) {
            Label pushArgs0;
            if (argc > 2) {  // 2: call arg2
                // decalare is 2 or 1 now
                __ Cmp(declaredNumArgsRegister, Immediate(1));
                __ B(Condition::EQ, &pushArgs0);
                __ Str(arg1, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            }
            if (argc > 1) {
                __ Bind(&pushArgs0);
                // decalare is is 1 now
                __ Str(arg0, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            }
        }
        __ Bl(pushCallThis);
    }
}

Register AssemblerStubs::GetThisRegsiter(ExtendedAssembler *assembler, JSCallMode mode)
{
    switch (mode) {
        case JSCallMode::CALL_GETTER:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
        case JSCallMode::CALL_SETTER:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        default:
            UNREACHABLE();
    }
    return INVALID_REG;
}

// void PushCallArgsxAndDispatch(uintptr_t glue, uintptr_t sp, uint64_t callTarget, uintptr_t method,
//     uint64_t callField, ...)
// GHC calling convention
// Input1: for callarg0/1/2/3        Input2: for callrange
// X19 - glue                        // X19 - glue
// FP  - sp                          // FP  - sp
// X20 - callTarget                  // X20 - callTarget
// X21 - method                      // X21 - method
// X22 - callField                   // X22 - callField
// X23 - arg0                        // X23 - actualArgc
// X24 - arg1                        // X24 - argv
// X25 - arg2
void AssemblerStubs::PushCallIThisRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIThisRangeAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_WITH_ARGV);
}

void AssemblerStubs::PushCallIRangeAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_WITH_ARGV);
}

void AssemblerStubs::PushCallNewAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallNewAndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
}

void AssemblerStubs::PushCallArgs3AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs3AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3);
}

void AssemblerStubs::PushCallArgs2AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs2AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2);
}

void AssemblerStubs::PushCallArgs1AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs1AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1);
}

void AssemblerStubs::PushCallArgs0AndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgs0AndDispatch));
    JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0);
}

// uint64_t PushCallIRangeAndDispatchNative(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[])
// c++ calling convention call js function
// Input: X0 - glue
//        X1 - nativeCode
//        X2 - callTarget
//        X3 - thisValue
//        X4  - argc
//        X5  - argV (...)
void AssemblerStubs::PushCallIRangeAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallIRangeAndDispatchNative));
    CallNativeWithArgv(assembler, false);
}

void AssemblerStubs::PushCallNewAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallNewAndDispatchNative));
    CallNativeWithArgv(assembler, true);
}

void AssemblerStubs::CallNativeWithArgv(ExtendedAssembler *assembler, bool callNew)
{
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
    // newTarget
    if (callNew) {
        __ Str(callTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    } else {
        __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Str(temp, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Str(callTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // callTarget
    __ Mov(stackArgs, sp);

    CallNativeInternal(assembler, glue, argc, stackArgs, nativeCode);
    __ Ret();
}

// uint64_t PushCallArgsAndDispatchNative(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input: X0 - glue
// stack layout: sp + N*8 argvN
//               ........
//               sp + 24: argv1
//               sp + 16: argv0
//               sp + 8:  actualArgc
// sp:           codeAddress
// construct Native Leave Frame
//               +--------------------------+
//               |       argv0              | calltarget , newTarget, this, ....
//               +--------------------------+ ---
//               |       argc               |   ^
//               |--------------------------|  Fixed
//               |       codeAddress        | BuiltinFrame
//               |--------------------------|   |
//               |       returnAddr         |   |
//               |--------------------------|   |
//               |       callsiteFp         |   |
//               |--------------------------|   |
//               |       frameType          |   v
//               +--------------------------+ ---
void AssemblerStubs::PushCallArgsAndDispatchNative(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(PushCallArgsAndDispatchNative));

    Register glue(X0);
    Register nativeCode = __ AvailableRegister1();
    Register argc(X4);
    Register argv(X5);
    Register temp(X6);
    Register fp(FP);

    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME, temp);

    __ Ldr(nativeCode, MemoryOperand(fp, BuiltinFrame::GetNativeCodeToFpDelta(false)));
    __ Ldr(argc, MemoryOperand(fp, BuiltinFrame::GetNumArgsToFpDelta(false)));
    __ Add(argv, fp, Immediate(BuiltinFrame::GetStackArgsToFpDelta(false)));

    CallNativeInternal(assembler, glue, argc, argv, nativeCode);
    __ Ret();
}

void AssemblerStubs::PushBuiltinFrame(ExtendedAssembler *assembler, Register glue, FrameType type, Register op)
{
    __ SaveFpAndLr();
    __ Str(Register(FP), MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
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
    __ Mov(Register(SP), Register(FP));
    __ RestoreFpAndLr();
}

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// X19 - glue
// FP  - sp
// X20 - pc
// X21 - constantPool
// X22 - profileTypeInfo
// X23 - acc
// X24 - hotnessCounter
// X25 - jumpSizeAfterCall
void AssemblerStubs::ResumeRspAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndDispatch));

    Register glueRegister = __ GlueRegister();
    Register sp(FP);
    Register pc(X20);
    Register jumpSize(X25);

    Register opcode(X6, W);
    Register bcStub(X7);

    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t spOffset = static_cast<int64_t>(AsmInterpretedFrame::GetBaseOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    ASSERT(spOffset < 0);
    __ Ldur(Register(SP), MemoryOperand(sp, fpOffset));  // resume rsp

    Label newObjectDynRangeReturn;
    Label dispatch;
    __ Cmp(jumpSize, Immediate(0));
    __ B(Condition::EQ, &newObjectDynRangeReturn);
    __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp

    __ Add(pc, pc, Operand(jumpSize, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));
    __ Bind(&dispatch);
    {
        __ Add(bcStub, glueRegister, Operand(opcode, UXTW, 3));  // 3： bc * 8
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }
    __ Bind(&newObjectDynRangeReturn);
    {
        __ Mov(opcode, kungfu::BytecodeStubCSigns::ID_NewObjectDynRangeReturn);
        __ Bl(&dispatch);
    }
}

void AssemblerStubs::ResumeRspAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturn));
    Register sp(SP);
    Register lr(X30);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register fpRegister = __ TempRegister1();
    int64_t offset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(offset < 0);
    __ Ldur(fpRegister, MemoryOperand(Register(FP), offset));
    __ Mov(sp, fpRegister);

    // return
    {
        __ RestoreFpAndLr();
        __ Mov(Register(X0), Register(X19));
        __ Ret();
    }
}

// ResumeCaughtFrameAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter)
// GHC calling convention
// X19 - glue
// FP  - sp
// X20 - pc
// X21 - constantPool
// X22 - profileTypeInfo
// X23 - acc
// X24 - hotnessCounter
void AssemblerStubs::ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeCaughtFrameAndDispatch));

    Register glue(X19);
    Register pc(X20);
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
        __ Add(bcStub, glue, Operand(opcode, UXTW, 3));  // 3： bc * 8
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

// c++ calling convention
// X0 - glue
// X1 - callTarget
// X2 - method
// X3 - callField
// X4 - receiver
// X5 - value
void AssemblerStubs::CallGetter(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallGetter));
    Label target;

    PushAsmInterpEntryFrame(assembler, false);
    __ Bl(&target);
    PopAsmInterpEntryFrame(assembler, false);
    __ Ret();
    __ Bind(&target);
    {
        __ PushFpAndLr();
        JSCallCommonEntry(assembler, JSCallMode::CALL_GETTER);
    }
}

void AssemblerStubs::CallSetter(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallSetter));
    Label target;
    PushAsmInterpEntryFrame(assembler, false);
    __ Bl(&target);
    PopAsmInterpEntryFrame(assembler, false);
    __ Ret();
    __ Bind(&target);
    {
        __ PushFpAndLr();
        JSCallCommonEntry(assembler, JSCallMode::CALL_SETTER);
    }
}

// Generate code for generator re-entering asm interpreter
// c++ calling convention
// Input: %X0 - glue
//        %X1 - context(GeneratorContext)
void AssemblerStubs::GeneratorReEnterAsmInterp(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(GeneratorReEnterAsmInterp));
    Label pushFrameState;
    Register glue = __ GlueRegister();
    Register contextRegister(X1);
    Register spRegister(SP);
    Register pc(X21);

    PushAsmInterpEntryFrame(assembler, true);

    Register prevSpRegister(FP);
    Register callTarget(X4);
    Register method(X5);
    Register temp(X6); // can not be used to store any variable
    Register fpRegister(X7);
    Register nRegsRegister(X26, W);
    Register regsArrayRegister(X27);
    Register newSp(X28);
    __ Ldr(callTarget, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_METHOD_OFFSET));
    __ Ldr(method, MemoryOperand(callTarget, JSFunctionBase::METHOD_OFFSET));
    __ Mov(fpRegister, spRegister);
    // push context regs
    __ Ldr(nRegsRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_NREGS_OFFSET));
    __ Ldr(regsArrayRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET));
    __ Add(regsArrayRegister, regsArrayRegister, Immediate(TaggedArray::DATA_OFFSET));
    __ PushArgsWithArgv(nRegsRegister, regsArrayRegister, temp, &pushFrameState);

    __ Bind(&pushFrameState);
    __ Mov(newSp, spRegister);
    // push frame state
    PushGeneratorFrameState(assembler, prevSpRegister, fpRegister, callTarget, method, contextRegister, pc, temp);

    // call bc stub
    CallBCStub(assembler, newSp, glue, callTarget, method, pc, temp);

    PopAsmInterpEntryFrame(assembler, true);
    __ Ret();
}

void AssemblerStubs::PushCallThis(ExtendedAssembler *assembler, JSCallMode mode)
{
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register sp(SP);

    Label pushVregs;
    Label pushNewTarget;
    Label pushCallTarget;
    bool haveThis = kungfu::AssemblerModule::JSModeHaveThisArg(mode);
    bool haveNewTarget = kungfu::AssemblerModule::JSModeHaveThisArg(mode);
    if (!haveThis) {
        __ Tst(callFieldRegister, LogicalImmediate::Create(CALL_TYPE_MASK, RegXSize));
        __ B(Condition::EQ, &pushVregs);
    }
    __ Tbz(callFieldRegister, JSMethod::HaveThisBit::START_BIT, &pushNewTarget);
    if (!haveThis) {
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register tempRegister = __ TempRegister1();
        __ Mov(tempRegister, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Str(tempRegister, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    } else {
        Register thisRegister = GetThisRegsiter(assembler, mode);
        __ Str(thisRegister, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Bind(&pushNewTarget);
    {
        __ Tbz(callFieldRegister, JSMethod::HaveNewTargetBit::START_BIT, &pushCallTarget);
        if (!haveNewTarget) {
            [[maybe_unused]] TempRegister1Scope scope1(assembler);
            Register newTarget = __ TempRegister1();
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ Str(newTarget, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        } else {
            __ Str(callTargetRegister, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
    }
    __ Bind(&pushCallTarget);
    {
        __ Tbz(callFieldRegister, JSMethod::HaveFuncBit::START_BIT, &pushVregs);
        __ Str(callTargetRegister, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Bind(&pushVregs);
    {
        PushVregs(assembler);
    }
}

void AssemblerStubs::PushVregs(ExtendedAssembler *assembler)
{
    Register prevSpRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::SP);
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register fpRegister = __ AvailableRegister1();

    Label pushFrameStateAndCall;
    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register tempRegister = __ TempRegister1();
    // args register can be reused now.
    Register numVregsRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
    GetNumVregsFromCallField(assembler, callFieldRegister, numVregsRegister);
    PushUndefinedWithArgc(assembler, numVregsRegister, tempRegister, &pushFrameStateAndCall);
    // fall through
    __ Bind(&pushFrameStateAndCall);
    {
        Register newSpRegister = __ AvailableRegister2();
        __ Mov(newSpRegister, Register(SP));

        StackOverflowCheck(assembler);

        [[maybe_unused]] TempRegister2Scope scope2(assembler);
        Register pcRegister = __ TempRegister2();
        PushFrameState(assembler, prevSpRegister, fpRegister, callTargetRegister, methodRegister, pcRegister,
            tempRegister);

        DispatchCall(assembler, pcRegister, newSpRegister);
    }
}

// Input: X19 - glue
//        FP - sp
//        X20 - callTarget
//        X21 - method
void AssemblerStubs::DispatchCall(ExtendedAssembler *assembler, Register pcRegister, Register newSpRegister)
{
    Register glueRegister = __ GlueRegister();
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register methodRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::METHOD);

    if (glueRegister.GetId() != X19) {
        __ Mov(Register(X19), glueRegister);
    }
    __ Ldrh(Register(X24, W), MemoryOperand(methodRegister, JSMethod::GetHotnessCounterOffset(false)));
    __ Mov(Register(X23), Immediate(JSTaggedValue::VALUE_HOLE));
    __ Ldr(Register(X22), MemoryOperand(callTargetRegister, JSFunction::PROFILE_TYPE_INFO_OFFSET));
    __ Ldr(Register(X21), MemoryOperand(callTargetRegister, JSFunction::CONSTANT_POOL_OFFSET));
    __ Mov(Register(X20), pcRegister);
    __ Mov(Register(FP), newSpRegister);

    Register bcIndexRegister = __ AvailableRegister1();
    Register tempRegister = __ AvailableRegister2();
    __ Ldrb(bcIndexRegister.W(), MemoryOperand(pcRegister, 0));
    __ Add(tempRegister, glueRegister, Operand(bcIndexRegister.W(), UXTW, 3));  // 3： bc * 8
    __ Ldr(tempRegister, MemoryOperand(tempRegister, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(tempRegister);
}

void AssemblerStubs::PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fp,
    Register callTarget, Register method, Register pc, Register op)
{
    Register sp(SP);
    __ Mov(op, Immediate(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME)));
    __ Stp(prevSp, op, MemoryOperand(sp, -16, AddrMode::PREINDEX));                 // -16: frame type & prevSp
    __ Ldr(pc, MemoryOperand(method, JSMethod::GetBytecodeArrayOffset(false)));
    __ Stp(fp, pc, MemoryOperand(sp, -16, AddrMode::PREINDEX));                     // -16: pc & fp
    __ Ldr(op, MemoryOperand(callTarget, JSFunction::LEXICAL_ENV_OFFSET));
    __ Stp(op, Register(Zero), MemoryOperand(sp, -16, AddrMode::PREINDEX));         // -16: jumpSizeAfterCall & env
    __ Mov(op, Immediate(JSTaggedValue::VALUE_HOLE));
    __ Stp(callTarget, op, MemoryOperand(sp, -16, AddrMode::PREINDEX));             // -16: acc & callTarget
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
    if (next != nullptr) {
        __ Cmp(argc.W(), Immediate(0));
        __ B(Condition::LE, next);
    }
    Label loopBeginning;
    __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Bind(&loopBeginning);
    __ Str(temp, MemoryOperand(Register(SP), -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Sub(argc.W(), argc.W(), Immediate(1));
    __ Cbnz(argc.W(), &loopBeginning);
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
    __ And(numArgs, numArgs, LogicalImmediate::Create(0x00000000FFFFFFFF, RegXSize));
    __ Str(numArgs, MemoryOperand(sp, EcmaRuntimeCallInfo::GetNumArgsOffset()));
    __ Str(stackArgs, MemoryOperand(sp, EcmaRuntimeCallInfo::GetStackArgsOffset()));
}

void AssemblerStubs::StackOverflowCheck([[maybe_unused]] ExtendedAssembler *assembler)
{
}

void AssemblerStubs::PushAsmInterpEntryFrame(ExtendedAssembler *assembler, bool saveLeave)
{
    Register glue = __ GlueRegister();
    Register sp(SP);

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeSave();
    }

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register frameTypeRegister = __ TempRegister2();

    if (saveLeave) {
        __ Str(glue, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        // prev managed fp is leave frame or nullptr(the first frame)
        __ Ldr(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    } else {
        __ Mov(prevFrameRegister, Register(FP));
    }
    __ SaveFpAndLr();
    __ Mov(frameTypeRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_ENTRY_FRAME)));

    // 2 : 2 means pair
    __ Stp(prevFrameRegister, frameTypeRegister, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Str(Register(Zero), MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // pc
}

void AssemblerStubs::PopAsmInterpEntryFrame(ExtendedAssembler *assembler, bool saveLeave)
{
    Register sp(SP);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register glue = __ TempRegister2();
    // skip pc
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Ldr(prevFrameRegister, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    // skip frame type
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));

    __ RestoreFpAndLr();
    if (saveLeave) {
        __ Ldr(glue, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
        __ Str(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    }
    if (!assembler->FromInterpreterHandler()) {
        __ CalleeRestore();
    }
}

void AssemblerStubs::PushGeneratorFrameState(ExtendedAssembler *assembler, Register &prevSpRegister,
    Register &fpRegister, Register &callTargetRegister, Register &methodRegister,
    Register &contextRegister, Register &pcRegister, Register &operatorRegister)
{
    Register sp(SP);
    __ Mov(operatorRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_FRAME)));
    // 2 : frameType and prevSp
    __ Stp(prevSpRegister, operatorRegister, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(pcRegister, MemoryOperand(methodRegister, JSMethod::GetBytecodeArrayOffset(false)));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_BC_OFFSET_OFFSET));
    __ Add(pcRegister, operatorRegister, pcRegister);
    __ Add(pcRegister, pcRegister, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
    // pc and fp
    __ Stp(fpRegister, pcRegister, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // 2 : 2 means pair
    // jumpSizeAfterCall
    __ Str(Register(Zero), MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_LEXICALENV_OFFSET));
    // env
    __ Str(operatorRegister, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_ACC_OFFSET));
    // 2 : acc and callTarget
    __ Stp(callTargetRegister, operatorRegister, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
}

void AssemblerStubs::CallBCStub(ExtendedAssembler *assembler, Register &newSp, Register &glue,
    Register &callTarget, Register &method, Register &pc, Register &temp)
{
    Register sp(SP);
    // caller save newSp register to restore rsp after call
    __ Str(newSp, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // prepare call entry
    __ Mov(Register(X19), glue);    // X19 - glue
    __ Mov(Register(FP), newSp);    // FP - sp
    __ Mov(Register(X20), pc);      // X20 - pc
    __ Ldr(Register(X21), MemoryOperand(callTarget, JSFunction::CONSTANT_POOL_OFFSET));     // X21 - constantpool
    __ Ldr(Register(X22), MemoryOperand(callTarget, JSFunction::PROFILE_TYPE_INFO_OFFSET)); // X22 - profileTypeInfo
    __ Mov(Register(X23), Immediate(JSTaggedValue::Hole().GetRawData()));                   // X23 - acc
    __ Ldr(Register(X24), MemoryOperand(method, JSMethod::GetHotnessCounterOffset(false))); // X24 - hotnessCounter

    // call the first bytecode handler
    __ Ldrb(temp.W(), MemoryOperand(pc, 0));
    // 3 : 3 means *8
    __ Add(temp, glue, Operand(temp.W(), UXTW, 3));
    __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(temp);
}

void AssemblerStubs::CallNativeEntry(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register argc(X1);
    Register argv(X2);
    Register method(X4);
    Register function(X3);
    Register nativeCode(X7);
    Register temp(X9);

    Register sp(SP);
    __ Str(function, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 16: skip nativeCode & argc
    __ Sub(sp, sp, Immediate(16));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_ENTRY_FRAME, temp);
    // get native pointer
    __ Ldr(nativeCode, MemoryOperand(method, JSMethod::GetBytecodeArrayOffset(false)));
    CallNativeInternal(assembler, glue, argc, argv, nativeCode);

    // 24: skip function
    __ Add(sp, sp, Immediate(24));
    __ Ret();
}

// Input: glue       - %X0
//        argc       - %X1
//        argv       - %X2(<callTarget, newTarget, this> are at the beginning of argv)
//        callTarget - %X3
//        method     - %X4
//        prevSp     - %X29
//        fp         - %10
//        callField  - %X7
void AssemblerStubs::PushArgsFastPath(ExtendedAssembler *assembler,
    Register &glue, Register &argc, Register &argv, Register &callTarget,
    Register &method, Register &prevSp, Register &fp, Register &callField)
{
    Label pushCallThis;
    Label pushNewTarget;
    Label pushCallTarget;
    Label pushVregs;
    Label pushFrameState;
    Register sp(SP);

    Register argvOnlyHaveArgs(X11);
    __ Add(argvOnlyHaveArgs, argv, Immediate(BuiltinFrame::RESERVED_CALL_ARGCOUNT * JSTaggedValue::TaggedTypeSize()));
    Register tempRegister(X12);
    __ PushArgsWithArgv(argc, argvOnlyHaveArgs, tempRegister, &pushCallThis);

    __ Bind(&pushCallThis);
    __ Tst(callField, LogicalImmediate::Create(CALL_TYPE_MASK, RegXSize));
    __ B(Condition::EQ, &pushVregs);
    __ Tst(callField, LogicalImmediate::Create(JSMethod::HaveThisBit::Mask(), RegXSize));
    __ B(Condition::EQ, &pushNewTarget);
    __ Ldr(tempRegister, MemoryOperand(argv, 16)); // 16: skip callTarget, newTarget
    __ Str(tempRegister, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    __ Bind(&pushNewTarget);
    __ Tst(callField, LogicalImmediate::Create(JSMethod::HaveNewTargetBit::Mask(), RegXSize));
    __ B(Condition::EQ, &pushCallTarget);
    __ Ldr(tempRegister, MemoryOperand(argv, 8)); // 8: skip callTarget
    __ Str(tempRegister, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    __ Bind(&pushCallTarget);
    __ Tst(callField, LogicalImmediate::Create(JSMethod::HaveFuncBit::Mask(), RegXSize));
    __ B(Condition::EQ, &pushVregs);
    __ Str(callTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    __ Bind(&pushVregs);
    {
        Register numVregsRegister(X13);
        GetNumVregsFromCallField(assembler, callField, numVregsRegister);
        __ Cbz(numVregsRegister.W(), &pushFrameState);
        PushUndefinedWithArgc(assembler, numVregsRegister, tempRegister, &pushFrameState);
    }

    __ Bind(&pushFrameState);
    Register newSpRegister(X14);
    __ Mov(newSpRegister, sp);

    StackOverflowCheck(assembler);

    Register pcRegister(X11);
    PushFrameState(assembler, prevSp, fp, callTarget, method, pcRegister, tempRegister);
    CallBCStub(assembler, newSpRegister, glue, callTarget, method, pcRegister, tempRegister);
}

// Input: glueRegister       - %X0
//        declaredNumArgsRegister - %X9
//        argcRegister       - %X1
//        argvRegister       - %X2(<callTarget, newTarget, this> are at the beginning of argv)
//        callTargetRegister - %X3
//        methodRegister     - %X4
//        prevSpRegister     - %X29
//        callFieldRegister  - %X7
void AssemblerStubs::PushArgsSlowPath(ExtendedAssembler *assembler, Register &glueRegister,
    Register &declaredNumArgsRegister, Register &argcRegister, Register &argvRegister, Register &callTargetRegister,
    Register &methodRegister, Register &prevSpRegister, Register &callFieldRegister)
{
    Label jumpToFastPath;
    Label haveExtra;
    Label pushUndefined;
    Register fpRegister(X11);
    Register tempRegister(X12);

    __ PushFpAndLr();
    __ Mov(fpRegister, Register(SP));
    __ Tst(callFieldRegister, LogicalImmediate::Create(JSMethod::HaveExtraBit::Mask(), RegXSize));
    __ B(Condition::NE, &haveExtra);
    __ Mov(tempRegister.W(), declaredNumArgsRegister.W());
    __ Sub(declaredNumArgsRegister.W(), declaredNumArgsRegister.W(), argcRegister.W());
    __ Cmp(declaredNumArgsRegister.W(), Immediate(0));
    __ B(Condition::GT, &pushUndefined);
    __ Mov(argcRegister.W(), tempRegister.W()); // actual = std::min(declare, actual)
    __ B(&jumpToFastPath);
    // fall through
    __ Bind(&haveExtra);
    {
        Register tempArgcRegister(X13);
        __ PushArgc(argcRegister, tempArgcRegister);
        __ Sub(declaredNumArgsRegister.W(), declaredNumArgsRegister.W(), argcRegister.W());
        __ Cmp(declaredNumArgsRegister.W(), Immediate(0));
        __ B(Condition::LE, &jumpToFastPath);
        // fall through
    }
    __ Bind(&pushUndefined);
    {
        PushUndefinedWithArgc(assembler, declaredNumArgsRegister, tempRegister, &jumpToFastPath);
        // fall through
    }
    __ Bind(&jumpToFastPath);
    PushArgsFastPath(assembler, glueRegister, argcRegister, argvRegister, callTargetRegister, methodRegister,
        prevSpRegister, fpRegister, callFieldRegister);
}
}  // panda::ecmascript::aarch64