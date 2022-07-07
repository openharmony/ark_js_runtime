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
#include "ecmascript/compiler/argument_accessor.h"
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
    Register fp(FP);
    Register tmp(X19);
    Register sp(SP);
    Register argC(X1);
    Register argV(X2);

    __ BindAssemblerStub(RTSTUB_ID(CallRuntime));
    __ PushFpAndLr();

    Register frameType(X2);
    // construct Leave Frame and callee save
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
    __ Stp(tmp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(16));  // 16: skip frame type and tmp
    __ Str(fp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

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

void AssemblerStubs::IncreaseStackForArguments(ExtendedAssembler *assembler, Register argc, Register currentSp)
{
    Register sp(SP);
    __ Mov(currentSp, sp);
    // add extra aguments, env and numArgs
    __ Add(argc, argc, Immediate(static_cast<int64_t>(CommonArgIdx::ACTUAL_ARGC)));
    __ Sub(currentSp, currentSp, Operand(argc, UXTW, SHIFT_OF_FRAMESLOT));
    Label aligned;
    __ Tst(currentSp, LogicalImmediate::Create(0xf, RegXSize));  // 0xf: 0x1111
    __ B(Condition::EQ, &aligned);
    __ Sub(currentSp, currentSp, Immediate(FRAME_SLOT_SIZE));
    __ Bind(&aligned);
    __ Mov(sp, currentSp);
    __ Add(currentSp, currentSp, Operand(argc, UXTW, SHIFT_OF_FRAMESLOT));
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
    Register currentSp(X6);
    Label copyArguments;

    __ BindAssemblerStub(RTSTUB_ID(JSFunctionEntry));
    PushJSFunctionEntryFrame (assembler, prevFp);
    Register argC(X7);
    __ Cmp(expectedNumArgs, actualNumArgs);
    __ CMov(argC, expectedNumArgs, actualNumArgs, Condition::HI);
    IncreaseStackForArguments(assembler, argC, currentSp);

    Label invokeCompiledJSFunction;
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register argc = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();
        __ Subs(argc, expectedNumArgs, Operand(actualNumArgs));
        __ B(Condition::LS, &copyArguments);
        PushUndefinedWithArgc(assembler, argc, undefinedValue, currentSp, nullptr);
    }
    __ Bind(&copyArguments);
    __ Cbz(actualNumArgs, &invokeCompiledJSFunction);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register argc = __ TempRegister1();
        Register argValue = __ TempRegister2();
        __ Mov(argc, actualNumArgs);
        __ PushArgsWithArgv(argc, argV, argValue, currentSp, &invokeCompiledJSFunction);
    }
    __ Bind(&invokeCompiledJSFunction);
    {
        TempRegister1Scope scope1(assembler);
        Register env = __ TempRegister1();
        __ Mov(Register(X19), expectedNumArgs);
        __ Ldr(env, MemoryOperand(argV, actualNumArgs, UXTW, SHIFT_OF_FRAMESLOT));
        __ Str(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
        __ Str(env, MemoryOperand(sp, 0));
        __ Blr(codeAddr);
    }

    // pop argV
    // 3 : 3 means argC * 8
    __ Ldr(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
    PopJSFunctionArgs(assembler, Register(X19), actualNumArgs);

    // pop prevLeaveFrameFp to restore thread->currentFrame_
    PopJSFunctionEntryFrame(assembler, glue);
    __ Ret();
}

// extern "C" JSTaggedType OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
//                                  uint32_t actualNumArgs, uintptr_t codeAddr, uintptr_t argv)
// Input:  %x0 - glue
//         %w1 - expectedNumArgs
//         %w2 - actualNumArgs
//         %x3 - codeAddr
//         %x4 - argv
//         %x5 - env

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
    Register actualNumArgs(X2, W);
    Register codeAddr(X3);
    Register argV(X4);
    Register env(X5);
    Register currentSp(X6);
    Register sp(SP);
    Label copyArguments;
    Label invokeCompiledJSFunction;

    // construct frame
    PushOptimizedJSFunctionFrame(assembler);
    Register argC(X7);
    __ Cmp(expectedNumArgs, actualNumArgs);
    __ CMov(argC, expectedNumArgs, actualNumArgs, Condition::HI);
    IncreaseStackForArguments(assembler, argC, currentSp);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register tmp = __ TempRegister1();
        Register undefinedValue = __ TempRegister2();
        __ Subs(tmp, expectedNumArgs, actualNumArgs);
        __ B(Condition::LS, &copyArguments);
        PushUndefinedWithArgc(assembler, tmp, undefinedValue, currentSp, nullptr);
    }
    __ Bind(&copyArguments);
    __ Cbz(actualNumArgs, &invokeCompiledJSFunction);
    {
        TempRegister1Scope scope1(assembler);
        TempRegister2Scope scope2(assembler);
        Register argc = __ TempRegister1();
        Register argValue = __ TempRegister2();
        __ Mov(argc, actualNumArgs);
        __ PushArgsWithArgv(argc, argV, argValue, currentSp, &invokeCompiledJSFunction);
    }
    __ Bind(&invokeCompiledJSFunction);
    {
        __ Mov(Register(X19), expectedNumArgs);
        __ Str(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
        __ Str(env, MemoryOperand(sp, 0));
        __ Blr(codeAddr);
    }

    // pop argV argC
    // 3 : 3 means argC * 8
    __ Ldr(actualNumArgs, MemoryOperand(sp, FRAME_SLOT_SIZE));
    PopJSFunctionArgs(assembler, Register(X19), actualNumArgs);
    // pop prevLeaveFrameFp to restore thread->currentFrame_
    PopOptimizedJSFunctionFrame(assembler);
    __ Ret();
}

void AssemblerStubs::OptimizedCallAsmInterpreter(ExtendedAssembler *assembler)
{
    Label target;
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        __ PushFpAndLr();
        JSCallCommonEntry(assembler, JSCallMode::CALL_FROM_AOT);
    }
}

void AssemblerStubs::PushLeaveFrame(ExtendedAssembler *assembler, Register glue, bool isBuiltin)
{
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    Register currentSp(X6);
    Register sp(SP);
    Register prevfp(X29);
    
    // construct leave frame
    if (isBuiltin) {
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME)));
        // current sp is not 16bytes aligned because native code address has pushed.
        __ Mov(currentSp, sp);
        __ Sub(sp, sp, Immediate(3 * FRAME_SLOT_SIZE));   // 3 : 3 for 16bytes align
        // 2 : 2 means pairs
        __ Stp(prevfp, Register(X30), MemoryOperand(currentSp, -2 * FRAME_SLOT_SIZE, PREINDEX));
        __ Str(frameType, MemoryOperand(currentSp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        __ Add(prevfp, sp, Immediate(FRAME_SLOT_SIZE));
    } else {
        __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::LEAVE_FRAME)));
        __ SaveFpAndLr();
        // 2 : 2 means pairs
        __ Stp(Register(X19), frameType, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    // save to thread currentLeaveFrame_;
    __ Str(prevfp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
}


void AssemblerStubs::PopLeaveFrame(ExtendedAssembler *assembler, bool isBuiltin)
{
    Register sp(SP);
    Register currentSp(X6);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    if (isBuiltin) {
        __ Add(currentSp, sp, Immediate(FRAME_SLOT_SIZE));  // skip frame type
        __ Add(sp, sp, Immediate(3 * FRAME_SLOT_SIZE));  // 3 : 3 means for 16bytes align
        // 2 : 2 means pairs
        __ Ldp(Register(X29), Register(X30), MemoryOperand(currentSp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    } else {
        // 2 : 2 means pairs
        __ Ldp(Register(X19), frameType, MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
        __ RestoreFpAndLr();
    }
}

// uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %x0 - glue
// stack layout: sp + N*8 argvN
//               ........
//               sp + 24: argv0
//               sp + 16: actualArgc
//               sp + 8:  env
//               sp:      codeAddress
// construct Native Leave Frame
//               +--------------------------+
//               |       argv0              | calltarget , newtARGET, this, ....
//               +--------------------------+ ---
//               |       argc               |   ^
//               |--------------------------|   |
//               |       env | thread       |   |
//               |--------------------------|  Fixed
//               |       codeAddress        | OptimizedBuiltinLeaveFrame
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
    Register fp(X29);
    Register glue(X0);
    Register sp(SP);
    Register nativeFuncAddr(X4);

    PushLeaveFrame(assembler, glue, true);

    __ Str(glue, MemoryOperand(fp, GetStackArgOffSetToFp(BuiltinsLeaveFrameArgId::ENV))); // thread (instead of env)
    __ Add(Register(X0), fp, Immediate(GetStackArgOffSetToFp(BuiltinsLeaveFrameArgId::ENV)));
    __ Blr(nativeFuncAddr);

    // descontruct leave frame and callee save register
    PopLeaveFrame(assembler, true);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE)); // skip native code address
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
    __ Ldr(jsfunc, MemoryOperand(sp, FRAME_SLOT_SIZE * 2)); // 2: skip env and argc
    JSCallBody(assembler, jsfunc);
}

void AssemblerStubs::JSCallBody(ExtendedAssembler *assembler, Register jsfunc)
{
    Register sp(SP);
    Register taggedValue(X2);
    Label nonCallable;
    Label notJSFunction;
    __ Mov(taggedValue, JSTaggedValue::TAG_MARK);
    __ Cmp(jsfunc, taggedValue);
    __ B(Condition::HS, &nonCallable);
    __ Cbz(jsfunc, &nonCallable);
    __ Mov(taggedValue, JSTaggedValue::TAG_SPECIAL);
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
    __ Sub(jstype, jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_BEGIN)));
    // 9 : 9 means JSType::JS_FUNCTION_END - JSType::JS_FUNCTION_BEGIN + 1
    __ Cmp(jstype, Immediate(static_cast<int>(JSType::JS_FUNCTION_END)
         - static_cast<int>(JSType::JS_FUNCTION_BEGIN) + 1));
    __ B(Condition::HS, &notJSFunction);

    Register method(X2);
    Register callField(X3);
    Register actualArgC(X4);
    Label callNativeMethod;
    Label callOptimizedMethod;
    __ Ldr(method, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(actualArgC, MemoryOperand(sp, FRAME_SLOT_SIZE));
    __ Ldr(callField, MemoryOperand(method, JSMethod::GetCallFieldOffset(false)));
    __ Tbnz(callField, JSMethod::IsNativeBit::START_BIT, &callNativeMethod);
    __ Tbnz(callField, JSMethod::IsAotCodeBit::START_BIT, &callOptimizedMethod);
    {
        Register argV(X5);
        // argV = sp + 16
        __ Add(argV, sp, Immediate(DOUBLE_SLOT_SIZE));
        OptimizedCallAsmInterpreter(assembler);
    }

    __ Bind(&callNativeMethod);
    {
        Register nativeFuncAddr(X4);
        __ Ldr(nativeFuncAddr, MemoryOperand(method, JSMethod::GetNativePointerOffset()));
        // -8 : -8 means sp increase step
        __ Str(nativeFuncAddr, MemoryOperand(sp, -8, AddrMode::PREINDEX));
        CallBuiltinTrampoline(assembler);
    }

    __ Bind(&callOptimizedMethod);
    {
        Register expectedNumArgs(X1, W);
        Register arg2(X2);
        Register codeAddress(X3);
        Register argV(X4);
        Register env(X5);
        Label directCallCodeEntry;
        const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
        __ Mov(Register(X5), jsfunc);
        __ Mov(arg2, actualArgC);
        __ Lsr(callField, callField, JSMethod::NumArgsBits::START_BIT);
        __ And(callField.W(), callField.W(),
            LogicalImmediate::Create(JSMethod::NumArgsBits::Mask() >> JSMethod::NumArgsBits::START_BIT, RegWSize));
        __ Add(expectedNumArgs, callField.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        __ Cmp(arg2.W(), expectedNumArgs);
        __ Add(argV, sp, Immediate(argoffsetSlot * FRAME_SLOT_SIZE));  // skip env and numArgs
        __ Ldr(codeAddress, MemoryOperand(Register(X5), JSFunctionBase::CODE_ENTRY_OFFSET));
        __ Ldr(env, MemoryOperand(sp, 0));
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
        // construct frame
        PushOptimizedJSFunctionFrame(assembler);
        Register basefp(X29);
        Register fp = __ AvailableRegister1();
        Register env(X5);

        Register argV(X6);
        __ Add(argV, basefp, Immediate(GetStackArgOffSetToFp(0)));
        __ Ldr(actualArgC, MemoryOperand(argV, FRAME_SLOT_SIZE));
        __ Ldr(env, MemoryOperand(argV, 0));

        Register boundLength(X2);
        Register realArgC(X7, W);
        Label copyBoundArgument;
        Label pushCallTarget;
        // get bound arguments
        __ Ldr(boundLength, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
        //  get bound length
        __ Ldr(boundLength, MemoryOperand(boundLength, TaggedArray::LENGTH_OFFSET));
        __ Add(realArgC, boundLength.W(), actualArgC.W());
        __ Mov(Register(X19), realArgC);
        IncreaseStackForArguments(assembler, realArgC, fp);
        __ Sub(actualArgC.W(), actualArgC.W(), Immediate(NUM_MANDATORY_JSFUNC_ARGS));
        __ Cmp(actualArgC.W(), Immediate(0));
        __ B(Condition::EQ, &copyBoundArgument);
        {
            TempRegister1Scope scope1(assembler);
            Register tmp = __ TempRegister1();
            const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
            __ Add(argV, argV, Immediate((NUM_MANDATORY_JSFUNC_ARGS + argoffsetSlot) *FRAME_SLOT_SIZE));
            __ PushArgsWithArgv(actualArgC, argV, tmp, fp, nullptr);
        }
        __ Bind(&copyBoundArgument);
        {
            Register boundArgs(X4);
            __ Ldr(boundArgs, MemoryOperand(jsfunc, JSBoundFunction::BOUND_ARGUMENTS_OFFSET));
            __ Add(boundArgs, boundArgs, Immediate(TaggedArray::DATA_OFFSET));
            __ Cmp(boundLength.W(), Immediate(0));
            __ B(Condition::EQ, &pushCallTarget);
            {
                TempRegister1Scope scope1(assembler);
                Register tmp = __ TempRegister1();
                __ PushArgsWithArgv(boundLength, boundArgs, tmp, fp, nullptr);
            }
        }
        __ Bind(&pushCallTarget);
        {
            Register thisObj(X4);
            Register newTarget(X6);
            Register boundTarget(X7);
            __ Ldr(thisObj, MemoryOperand(jsfunc, JSBoundFunction::BOUND_THIS_OFFSET));
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            // 2 : 2 means pair
            __ Stp(newTarget, thisObj, MemoryOperand(fp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
            __ Ldr(boundTarget, MemoryOperand(jsfunc, JSBoundFunction::BOUND_TARGET_OFFSET));
            // 2 : 2 means pair
            __ Stp(Register(X19), boundTarget, MemoryOperand(fp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
            __ Str(env, MemoryOperand(fp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        __ CallAssemblerStub(RTSTUB_ID(JSCall), false);

        PopJSFunctionArgs(assembler, Register(X19), Register(X19));
        PopOptimizedJSFunctionFrame(assembler);
        __ Ret();
    }
    __ Bind(&jsProxy);
    {
        // input: x1(calltarget)
        // output: glue:x0 argc:x1 calltarget:x2 argv:x3
        __ Mov(Register(X2), jsfunc);
        __ Ldr(Register(X1), MemoryOperand(sp, FRAME_SLOT_SIZE)); // 8: skip env
        __ Add(X3, sp, Immediate(FRAME_SLOT_SIZE * 2)); // 2: get argv

        Register proxyCallInternalId(Register(X9).W());
        Register codeAddress(X10);
        __ Mov(proxyCallInternalId, Immediate(CommonStubCSigns::JsProxyCallInternal));
        __ Add(codeAddress, X0, Immediate(JSThread::GlueData::GetCOStubEntriesOffset(false)));
        __ Ldr(codeAddress, MemoryOperand(codeAddress, proxyCallInternalId, UXTW, SHIFT_OF_FRAMESLOT));
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

void AssemblerStubs::JSProxyCallInternalWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSProxyCallInternalWithArgV));
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
// Input: glue           - %X0
//        callTarget     - %X1
//        method         - %X2
//        callField      - %X3
//        argc           - %X4
//        argv           - %X5(<callTarget, newTarget, this> are at the beginning of argv)
void AssemblerStubs::AsmInterpreterEntry(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(AsmInterpreterEntry));
    Label target;
    PushAsmInterpEntryFrame(assembler);
    __ Bl(&target);
    PopAsmInterpEntryFrame(assembler);
    __ Ret();

    __ Bind(&target);
    {
        JSCallDispatch(assembler);
    }
}

// Input: glue           - %X0
//        callTarget     - %X1
//        method         - %X2
//        callField      - %X3
//        argc           - %X4
//        argv           - %X5(<callTarget, newTarget, this> are at the beginning of argv)
void AssemblerStubs::JSCallDispatch(ExtendedAssembler *assembler)
{
    Label notJSFunction;
    Label callNativeEntry;
    Label callJSFunctionEntry;
    Label notCallable;
    Register glueRegister(X0);
    Register argcRegister(X4, W);
    Register argvRegister(X5);
    Register callTargetRegister(X1);
    Register callFieldRegister(X3);
    Register bitFieldRegister(X16);
    Register tempRegister(X17); // can not be used to store any variable
    Register functionTypeRegister(X18, W);
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
        // fall through
    }
    __ Bind(&callNativeEntry);
    CallNativeEntry(assembler);
    __ Bind(&callJSFunctionEntry);
    {
        __ Tbnz(callFieldRegister, JSMethod::IsNativeBit::START_BIT, &callNativeEntry);
        // fast path
        __ PushFpAndLr();
        __ Add(argvRegister, argvRegister, Immediate(NUM_MANDATORY_JSFUNC_ARGS * JSTaggedValue::TaggedTypeSize()));
        JSCallCommonEntry(assembler, JSCallMode::CALL_ENTRY);
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
    if (mode == JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV) {
        Register thisRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register tempArgcRegister = __ TempRegister1();
        __ PushArgc(argcRegister, tempArgcRegister, fpRegister);
        __ Str(thisRegister, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        jumpSize = 0;
    }

    if (jumpSize >= 0) {
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
    Register fpRegister = __ AvailableRegister1();
    // call range
    if (argc < 0) {
        Register numRegister = __ AvailableRegister2();
        Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
        Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGV);
        __ Mov(numRegister, argcRegister);
        [[maybe_unused]] TempRegister1Scope scope(assembler);
        Register opRegister = __ TempRegister1();
        __ PushArgsWithArgv(numRegister, argvRegister, opRegister, fpRegister, pushCallThis);
    } else if (argc > 0) {
        Register arg0 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG0);
        Register arg1 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
        Register arg2 = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG2);
        if (argc > 2) { // 2: call arg2
            __ Str(arg2, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        if (argc > 1) {
            __ Str(arg1, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
        if (argc > 0) {
            __ Str(arg0, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
    }
}

void AssemblerStubs::JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
                                          Label *fastPathEntry, Label *pushCallThis)
{
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register argcRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGC);
    Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARGV);
    Register fpRegister = __ AvailableRegister1();
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
            __ PushArgc(argc, tempArgcRegister, fpRegister);
        } else {
            __ PushArgc(argcRegister, tempArgcRegister, fpRegister);
        }
        // fall through
    }
    __ Bind(&noExtraEntry);
    {
        if (argc == 0) {
            {
                [[maybe_unused]] TempRegister1Scope scope(assembler);
                Register tempRegister = __ TempRegister1();
                PushUndefinedWithArgc(assembler,
                    declaredNumArgsRegister, tempRegister, fpRegister, nullptr);
            }
            __ B(fastPathEntry);
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
        PushUndefinedWithArgc(assembler, diffRegister, tempRegister, fpRegister, &pushArgsEntry);
        __ B(fastPathEntry);
    }
    // declare < actual
    __ Bind(&pushArgsEntry);
    {
        __ Tbnz(callFieldRegister, JSMethod::HaveExtraBit::START_BIT, fastPathEntry);
        // no extra branch
        // arg1, declare must be 0
        if (argc == 1) {
            __ B(pushCallThis);
            return;
        }
        __ Cmp(declaredNumArgsRegister, Immediate(0));
        __ B(Condition::EQ, pushCallThis);
        // call range
        if (argc < 0) {
            [[maybe_unused]] TempRegister1Scope scope(assembler);
            Register opRegister = __ TempRegister1();
            __ PushArgsWithArgv(declaredNumArgsRegister, argvRegister, opRegister, fpRegister, nullptr);
        } else if (argc > 0) {
            Label pushArgs0;
            if (argc > 2) {  // 2: call arg2
                // decalare is 2 or 1 now
                __ Cmp(declaredNumArgsRegister, Immediate(1));
                __ B(Condition::EQ, &pushArgs0);
                __ Str(arg1, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            }
            if (argc > 1) {
                __ Bind(&pushArgs0);
                // decalare is is 1 now
                __ Str(arg0, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
            }
        }
        __ B(pushCallThis);
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
        case JSCallMode::CALL_FROM_AOT:
        case JSCallMode::CALL_ENTRY: {
            Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            Register thisRegister = __ AvailableRegister2();
            __ Ldur(thisRegister, MemoryOperand(argvRegister, -FRAME_SLOT_SIZE));
            return thisRegister;
        }
        default:
            UNREACHABLE();
    }
    return INVALID_REG;
}

Register AssemblerStubs::GetNewTargetRegsiter(ExtendedAssembler *assembler, JSCallMode mode)
{
    switch (mode) {
        case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
        case JSCallMode::CALL_THIS_WITH_ARGV:
            return __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
        case JSCallMode::CALL_FROM_AOT:
        case JSCallMode::CALL_ENTRY: {
            Register argvRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::ARG1);
            Register newTargetRegister = __ AvailableRegister2();
            // 2: new Target index
            __ Ldur(newTargetRegister, MemoryOperand(argvRegister, -2 * FRAME_SLOT_SIZE));
            return newTargetRegister;
        }
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
    Register fpRegister(X11);
    Register spRegister(SP);

    Label pushThis;
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME_WITH_ARGV, temp, argc);
    StackOverflowCheck(assembler);

    __ Mov(fpRegister, spRegister);
    __ Mov(opArgc, argc);
    __ Mov(opArgv, argv);
    __ PushArgsWithArgv(opArgc, opArgv, temp, fpRegister, &pushThis);

    __ Bind(&pushThis);
    // newTarget
    if (callNew) {
        // 16: this & newTarget
        __ Stp(callTarget, thisObj, MemoryOperand(fpRegister, -16, AddrMode::PREINDEX));
    } else {
        __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        // 16: this & newTarget
        __ Stp(temp, thisObj, MemoryOperand(fpRegister, -16, AddrMode::PREINDEX));
    }
    // callTarget
    __ Str(callTarget, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(temp, fpRegister, Immediate(40));  // 40: skip frame type, numArgs, func, newTarget and this
    __ Add(Register(FP), temp, Operand(argc, LSL, 3));  // 3: argc * 8

    __ Add(temp, argc, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    // 2: thread & argc
    __ Stp(glue, temp, MemoryOperand(fpRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(Register(X0), fpRegister, Immediate(0));

    __ Align16(fpRegister);
    __ Mov(spRegister, fpRegister);

    CallNativeInternal(assembler, nativeCode);
    __ Ret();
}

// uint64_t PushCallArgsAndDispatchNative(uintptr_t codeAddress, uintptr_t glue, uint32_t argc, ...)
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input: X0 - codeAddress
// stack layout: sp + N*8 argvN
//               ........
//               sp + 24: argv1
//               sp + 16: argv0
//               sp + 8:  actualArgc
//               sp:      thread
// construct Native Leave Frame
//               +--------------------------+
//               |       argv0              | calltarget , newTarget, this, ....
//               +--------------------------+ ---
//               |       argc               |   ^
//               |--------------------------|  Fixed
//               |       thread             | BuiltinFrame
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

    Register nativeCode(X0);
    Register glue(X1);
    Register argv(X5);
    Register temp(X6);
    Register sp(SP);
    Register nativeCodeTemp(X2);

    __ Mov(nativeCodeTemp, nativeCode);

    __ Ldr(glue, MemoryOperand(sp, 0));
    __ Add(Register(X0), sp, Immediate(0));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_FRAME, temp, argv);

    CallNativeInternal(assembler, nativeCodeTemp);
    __ Ret();
}

void AssemblerStubs::PushBuiltinFrame(ExtendedAssembler *assembler, Register glue,
    FrameType type, Register op, Register next)
{
    Register sp(SP);
    __ PushFpAndLr();
    __ Mov(op, sp);
    __ Str(op, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Mov(op, Immediate(static_cast<int32_t>(type)));
    if (type == FrameType::BUILTIN_FRAME) {
        // push stack args
        __ Add(next, sp, Immediate(BuiltinFrame::GetStackArgsToFpDelta(false)));
        // 16: type & next
        __ Stp(next, op, MemoryOperand(sp, -16, AddrMode::PREINDEX));
        __ Add(Register(FP), sp, Immediate(16));  // 16: skip next and frame type
    } else if (type == FrameType::BUILTIN_ENTRY_FRAME) {
        // 16: type & next
        __ Stp(next, op, MemoryOperand(sp, -16, AddrMode::PREINDEX));
        __ Add(Register(FP), sp, Immediate(16));  // 16: skip next and frame type
    } else {
        // 16: type & next
        __ Stp(next, op, MemoryOperand(sp, -16, AddrMode::PREINDEX));
    }
}

void AssemblerStubs::CallNativeInternal(ExtendedAssembler *assembler, Register nativeCode)
{
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
    Register rsp(SP);
    Register pc(X20);
    Register jumpSizeRegister(X25);

    Register ret(X23);
    Register opcode(X6, W);
    Register temp(X7);
    Register bcStub(X7);
    Register fp(X8);

    int64_t fpOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    int64_t spOffset = static_cast<int64_t>(AsmInterpretedFrame::GetBaseOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(fpOffset < 0);
    ASSERT(spOffset < 0);

    Label newObjectDynRangeReturn;
    Label dispatch;
    __ Ldur(fp, MemoryOperand(sp, fpOffset));  // store fp for temporary
    __ Cmp(jumpSizeRegister, Immediate(0));
    __ B(Condition::EQ, &newObjectDynRangeReturn);
    __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp

    __ Add(pc, pc, Operand(jumpSizeRegister, LSL, 0));
    __ Ldrb(opcode, MemoryOperand(pc, 0));
    __ Bind(&dispatch);
    {
        __ Mov(rsp, fp);  // resume rsp
        __ Add(bcStub, glueRegister, Operand(opcode, UXTW, SHIFT_OF_FRAMESLOT));
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }

    auto jumpSize = kungfu::AssemblerModule::GetJumpSizeFromJSCallMode(JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV);
    Label getHiddenThis;
    Label notUndefined;
    __ Bind(&newObjectDynRangeReturn);
    {
        __ Cmp(ret, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ B(Condition::NE, &notUndefined);
        auto index = AsmInterpretedFrame::ReverseIndex::THIS_OBJECT_REVERSE_INDEX;
        auto thisOffset = index * 8;  // 8: byte size
        ASSERT(thisOffset < 0);
        __ Bind(&getHiddenThis);
        __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp
        __ Mov(rsp, fp);  // resume rsp
        __ Ldur(ret, MemoryOperand(rsp, thisOffset));  // update acc
        __ Add(pc, pc, Immediate(jumpSize));
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ Add(bcStub, glueRegister, Operand(opcode, UXTW, SHIFT_OF_FRAMESLOT));
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }
    __ Bind(&notUndefined);
    {
        Label notEcmaObject;
        __ Mov(temp, Immediate(JSTaggedValue::TAG_HEAPOBJECT_MARK));
        __ And(temp, temp, ret);
        __ Cmp(temp, Immediate(0));
        __ B(Condition::NE, &notEcmaObject);
        // acc is heap object
        __ Ldr(temp, MemoryOperand(ret, 0)); // hclass
        __ Ldr(temp, MemoryOperand(temp, JSHClass::BIT_FIELD_OFFSET));
        __ And(temp.W(), temp.W(), LogicalImmediate::Create(0xFF, RegWSize));
        __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_END)));
        __ B(Condition::HI, &notEcmaObject);
        __ Cmp(temp.W(), Immediate(static_cast<int64_t>(JSType::ECMA_OBJECT_BEGIN)));
        __ B(Condition::LO, &notEcmaObject);
        // acc is ecma object
        __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp
        __ Add(pc, pc, Immediate(jumpSize));
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ B(&dispatch);

        __ Bind(&notEcmaObject);
        {
            int64_t constructorOffset = static_cast<int64_t>(AsmInterpretedFrame::GetFunctionOffset(false))
                - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
            ASSERT(constructorOffset < 0);
            __ Ldur(temp, MemoryOperand(sp, constructorOffset));  // load constructor
            __ Ldr(temp, MemoryOperand(temp, JSFunction::BIT_FIELD_OFFSET));
            __ Lsr(temp.W(), temp.W(), JSFunction::FunctionKindBits::START_BIT);
            __ And(temp.W(), temp.W(),
                LogicalImmediate::Create((1LU << JSFunction::FunctionKindBits::SIZE) - 1, RegWSize));
            __ Cmp(temp.W(), Immediate(static_cast<int64_t>(FunctionKind::CLASS_CONSTRUCTOR)));
            __ B(Condition::LS, &getHiddenThis);  // constructor is base
            // exception branch
            {
                __ Mov(opcode, kungfu::BytecodeStubCSigns::ID_NewObjectDynRangeThrowException);
                __ Ldur(sp, MemoryOperand(sp, spOffset));  // update sp
                __ B(&dispatch);
            }
        }
    }
}

// ResumeRspAndReturn(uintptr_t acc)
// GHC calling convention
// X19 - acc
// FP - prevSp
// X20 - sp
void AssemblerStubs::ResumeRspAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeRspAndReturn));
    Register rsp(SP);
    Register currentSp(X20);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register fpRegister = __ TempRegister1();
    int64_t offset = static_cast<int64_t>(AsmInterpretedFrame::GetFpOffset(false))
        - static_cast<int64_t>(AsmInterpretedFrame::GetSize(false));
    ASSERT(offset < 0);
    __ Ldur(fpRegister, MemoryOperand(currentSp, offset));
    __ Mov(rsp, fpRegister);

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
    __ B(Condition::EQ, &dispatch);
    // up frame
    __ Mov(Register(SP), fp);
    // fall through
    __ Bind(&dispatch);
    {
        __ Ldrb(opcode, MemoryOperand(pc, 0));
        __ Add(bcStub, glue, Operand(opcode, UXTW, SHIFT_OF_FRAMESLOT));
        __ Ldr(bcStub, MemoryOperand(bcStub, JSThread::GlueData::GetBCStubEntriesOffset(false)));
        __ Br(bcStub);
    }
}

// ResumeUncaughtFrameAndReturn(uintptr_t glue)
// GHC calling convention
// X19 - glue
// FP - sp
// X20 - acc
void AssemblerStubs::ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(ResumeUncaughtFrameAndReturn));

    Register glue(X19);
    Register fp(X5);
    Register acc(X20);
    Register cppRet(X0);

    __ Ldr(fp, MemoryOperand(glue, JSThread::GlueData::GetLastFpOffset(false)));
    __ Mov(Register(SP), fp);
    // this method will return to Execute(cpp calling convention), and the return value should be put into X0.
    __ Mov(cppRet, acc);
    __ RestoreFpAndLr();
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

    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
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
    PushAsmInterpBridgeFrame(assembler);
    __ Bl(&target);
    PopAsmInterpBridgeFrame(assembler);
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
    Label target;
    PushAsmInterpEntryFrame(assembler);
    __ Bl(&target);
    PopAsmInterpEntryFrame(assembler);
    __ Ret();
    __ Bind(&target);
    {
        GeneratorReEnterAsmInterpDispatch(assembler);
    }
}

void AssemblerStubs::GeneratorReEnterAsmInterpDispatch(ExtendedAssembler *assembler)
{
    Label pushFrameState;
    Register glue = __ GlueRegister();
    Register contextRegister(X1);
    Register spRegister(SP);
    Register pc(X8);
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
    __ PushFpAndLr();
    __ Mov(fpRegister, spRegister);
    // push context regs
    __ Ldr(nRegsRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_NREGS_OFFSET));
    __ Ldr(regsArrayRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET));
    __ Add(regsArrayRegister, regsArrayRegister, Immediate(TaggedArray::DATA_OFFSET));
    __ PushArgsWithArgv(nRegsRegister, regsArrayRegister, temp, fpRegister, &pushFrameState);

    __ Bind(&pushFrameState);
    __ Mov(newSp, fpRegister);
    // push frame state
    PushGeneratorFrameState(assembler, prevSpRegister, fpRegister, callTarget, method, contextRegister, pc, temp);
    __ Align16(fpRegister);
    __ Mov(Register(SP), fpRegister);
    // call bc stub
    CallBCStub(assembler, newSp, glue, callTarget, method, pc, temp);
}

void AssemblerStubs::PushCallThis(ExtendedAssembler *assembler, JSCallMode mode)
{
    Register callFieldRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_FIELD);
    Register callTargetRegister = __ CallDispatcherArgument(kungfu::CallDispatchInputs::CALL_TARGET);
    Register fpRegister = __ AvailableRegister1();

    Label pushVregs;
    Label pushNewTarget;
    Label pushCallTarget;
    bool haveThis = kungfu::AssemblerModule::JSModeHaveThisArg(mode);
    bool haveNewTarget = kungfu::AssemblerModule::JSModeHaveNewTargetArg(mode);
    if (!haveThis) {
        __ Tst(callFieldRegister, LogicalImmediate::Create(CALL_TYPE_MASK, RegXSize));
        __ B(Condition::EQ, &pushVregs);
    }
    __ Tbz(callFieldRegister, JSMethod::HaveThisBit::START_BIT, &pushNewTarget);
    if (!haveThis) {
        [[maybe_unused]] TempRegister1Scope scope1(assembler);
        Register tempRegister = __ TempRegister1();
        __ Mov(tempRegister, Immediate(JSTaggedValue::VALUE_UNDEFINED));
        __ Str(tempRegister, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    } else {
        Register thisRegister = GetThisRegsiter(assembler, mode);
        __ Str(thisRegister, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    }
    __ Bind(&pushNewTarget);
    {
        __ Tbz(callFieldRegister, JSMethod::HaveNewTargetBit::START_BIT, &pushCallTarget);
        if (!haveNewTarget) {
            [[maybe_unused]] TempRegister1Scope scope1(assembler);
            Register newTarget = __ TempRegister1();
            __ Mov(newTarget, Immediate(JSTaggedValue::VALUE_UNDEFINED));
            __ Str(newTarget, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        } else {
            Register newTargetRegister = GetNewTargetRegsiter(assembler, mode);
            __ Str(newTargetRegister, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
        }
    }
    __ Bind(&pushCallTarget);
    {
        __ Tbz(callFieldRegister, JSMethod::HaveFuncBit::START_BIT, &pushVregs);
        __ Str(callTargetRegister, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
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
    PushUndefinedWithArgc(assembler, numVregsRegister,
        tempRegister, fpRegister, &pushFrameStateAndCall);
    // fall through
    __ Bind(&pushFrameStateAndCall);
    {
        Register newSpRegister = __ AvailableRegister2();
        __ Mov(newSpRegister, fpRegister);

        StackOverflowCheck(assembler);

        [[maybe_unused]] TempRegister2Scope scope2(assembler);
        Register pcRegister = __ TempRegister2();
        PushFrameState(assembler, prevSpRegister, fpRegister, callTargetRegister, methodRegister, pcRegister,
            tempRegister);

        __ Align16(fpRegister);
        __ Mov(Register(SP), fpRegister);
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
    __ Add(tempRegister, glueRegister, Operand(bcIndexRegister.W(), UXTW, SHIFT_OF_FRAMESLOT));
    __ Ldr(tempRegister, MemoryOperand(tempRegister, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(tempRegister);
}

void AssemblerStubs::PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fp,
    Register callTarget, Register method, Register pc, Register op)
{
    __ Mov(op, Immediate(static_cast<int32_t>(FrameType::ASM_INTERPRETER_FRAME)));
    __ Stp(prevSp, op, MemoryOperand(fp, -16, AddrMode::PREINDEX));                 // -16: frame type & prevSp
    __ Ldr(pc, MemoryOperand(method, JSMethod::GetBytecodeArrayOffset(false)));
    __ Mov(op, Register(SP));
    __ Stp(op, pc, MemoryOperand(fp, -16, AddrMode::PREINDEX));                     // -16: pc & fp
    __ Ldr(op, MemoryOperand(callTarget, JSFunction::LEXICAL_ENV_OFFSET));
    __ Stp(op, Register(Zero), MemoryOperand(fp, -16, AddrMode::PREINDEX));         // -16: jumpSizeAfterCall & env
    __ Mov(op, Immediate(JSTaggedValue::VALUE_HOLE));
    __ Stp(callTarget, op, MemoryOperand(fp, -16, AddrMode::PREINDEX));             // -16: acc & callTarget
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
    Register fp, panda::ecmascript::Label *next)
{
    if (next != nullptr) {
        __ Cmp(argc.W(), Immediate(0));
        __ B(Condition::LE, next);
    }
    Label loopBeginning;
    __ Mov(temp, Immediate(JSTaggedValue::VALUE_UNDEFINED));
    __ Bind(&loopBeginning);
    __ Str(temp, MemoryOperand(fp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Sub(argc.W(), argc.W(), Immediate(1));
    __ Cbnz(argc.W(), &loopBeginning);
}

void AssemblerStubs::StackOverflowCheck([[maybe_unused]] ExtendedAssembler *assembler)
{
}

void AssemblerStubs::PushAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    Register glue = __ GlueRegister();
    Register fp(X29);
    Register sp(SP);

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeSave();
    }

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register frameTypeRegister = __ TempRegister2();

    __ PushFpAndLr();

    // prev managed fp is leave frame or nullptr(the first frame)
    __ Ldr(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Mov(frameTypeRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_ENTRY_FRAME)));
    // 2 : prevSp & frame type
    __ Stp(prevFrameRegister, frameTypeRegister, MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2 : pc & glue
    __ Stp(glue, Register(Zero), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));  // pc
    __ Add(fp, sp, Immediate(32));  // 32: skip frame type, prevSp, pc and glue
}

void AssemblerStubs::PopAsmInterpEntryFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register prevFrameRegister = __ TempRegister1();
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register glue = __ TempRegister2();
    // 2: glue & pc
    __ Ldp(glue, Register(Zero), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    // 2: skip frame type & prev
    __ Ldp(prevFrameRegister, Register(Zero), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));

    __ RestoreFpAndLr();
    __ Str(prevFrameRegister, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));
    if (!assembler->FromInterpreterHandler()) {
        __ CalleeRestore();
    }
}

void AssemblerStubs::PushAsmInterpBridgeFrame(ExtendedAssembler *assembler)
{
    Register fp(X29);
    Register sp(SP);

    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register frameTypeRegister = __ TempRegister1();

    __ Mov(frameTypeRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME)));
    // 2 : return addr & frame type
    __ Stp(frameTypeRegister, Register(X30), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2 : prevSp & pc
    __ Stp(Register(Zero), Register(FP), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Add(fp, sp, Immediate(24));  // 24: skip frame type, prevSp, pc

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeSave();
    }
}

void AssemblerStubs::PopAsmInterpBridgeFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);

    if (!assembler->FromInterpreterHandler()) {
        __ CalleeRestore();
    }
    // 2: prevSp & pc
    __ Ldp(Register(Zero), Register(FP), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    // 2: return addr & frame type
    __ Ldp(Register(Zero), Register(X30), MemoryOperand(sp, 2 * FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
}

void AssemblerStubs::PushGeneratorFrameState(ExtendedAssembler *assembler, Register &prevSpRegister,
    Register &fpRegister, Register &callTargetRegister, Register &methodRegister,
    Register &contextRegister, Register &pcRegister, Register &operatorRegister)
{
    __ Mov(operatorRegister, Immediate(static_cast<int64_t>(FrameType::ASM_INTERPRETER_FRAME)));
    // 2 : frameType and prevSp
    __ Stp(prevSpRegister, operatorRegister, MemoryOperand(fpRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(pcRegister, MemoryOperand(methodRegister, JSMethod::GetBytecodeArrayOffset(false)));
    // offset need 8 align, GENERATOR_NREGS_OFFSET instead of GENERATOR_BC_OFFSET_OFFSET
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_NREGS_OFFSET));
    // 32: get high 32bit
    __ Lsr(operatorRegister, operatorRegister, 32);
    __ Add(pcRegister, operatorRegister, pcRegister);
    __ Add(pcRegister, pcRegister, Immediate(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
    // pc and fp
    __ Mov(operatorRegister, Register(SP));
    __ Stp(operatorRegister, pcRegister, MemoryOperand(fpRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX)); // 2 : 2 means pair
    // jumpSizeAfterCall
    __ Str(Register(Zero), MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_LEXICALENV_OFFSET));
    // env
    __ Str(operatorRegister, MemoryOperand(fpRegister, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Ldr(operatorRegister, MemoryOperand(contextRegister, GeneratorContext::GENERATOR_ACC_OFFSET));
    // 2 : acc and callTarget
    __ Stp(callTargetRegister, operatorRegister, MemoryOperand(fpRegister, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
}

void AssemblerStubs::CallBCStub(ExtendedAssembler *assembler, Register &newSp, Register &glue,
    Register &callTarget, Register &method, Register &pc, Register &temp)
{
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
    __ Add(temp, glue, Operand(temp.W(), UXTW, SHIFT_OF_FRAMESLOT));
    __ Ldr(temp, MemoryOperand(temp, JSThread::GlueData::GetBCStubEntriesOffset(false)));
    __ Br(temp);
}

void AssemblerStubs::CallNativeEntry(ExtendedAssembler *assembler)
{
    Register glue(X0);
    Register argv(X5);
    Register method(X2);
    Register function(X1);
    Register nativeCode(X7);
    Register temp(X9);

    Register sp(SP);
    // 2: function & align
    __ Stp(function, Register(Zero), MemoryOperand(sp, -2 * FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    // 2: skip argc & thread
    __ Sub(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    PushBuiltinFrame(assembler, glue, FrameType::BUILTIN_ENTRY_FRAME, temp, argv);
    // get native pointer
    __ Ldr(nativeCode, MemoryOperand(method, JSMethod::GetBytecodeArrayOffset(false)));
    __ Mov(temp, argv);
    __ Sub(Register(X0), temp, Immediate(2 * FRAME_SLOT_SIZE));  // 2: skip argc & thread
    CallNativeInternal(assembler, nativeCode);

    // 4: skip function
    __ Add(sp, sp, Immediate(4 * FRAME_SLOT_SIZE));
    __ Ret();
}

void AssemblerStubs::PushArgsWithArgV(ExtendedAssembler *assembler, Register jsfunc,
                                      Register actualNumArgs, Register argV, Label *pushCallThis)
{
    Register expectedNumArgs(X19); // output
    [[maybe_unused]] TempRegister1Scope scope1(assembler);
    Register tmp = __ TempRegister1();
    Label copyArguments;

    // get expected num Args
    __ Ldr(tmp, MemoryOperand(jsfunc, JSFunction::METHOD_OFFSET));
    __ Ldr(tmp, MemoryOperand(tmp, JSMethod::GetCallFieldOffset(false)));
    __ Lsr(tmp, tmp, JSMethod::NumArgsBits::START_BIT);
    __ And(tmp.W(), tmp.W(),
        LogicalImmediate::Create(JSMethod::NumArgsBits::Mask() >> JSMethod::NumArgsBits::START_BIT, RegWSize));
    __ Mov(expectedNumArgs.W(), tmp.W());
    __ Subs(tmp.W(), tmp.W(), actualNumArgs.W());
    __ B(Condition::LS, &copyArguments);
    {
        [[maybe_unused]] TempRegister2Scope scope2(assembler);
        Register undefinedValue = __ TempRegister2();
        PushUndefinedWithArgc(assembler, tmp, undefinedValue, Register(SP), nullptr);
    }

    __ Bind(&copyArguments);
    {
        __ Mov(tmp, expectedNumArgs);
        // expectedNumArgs <= actualNumArgs
        __ Cmp(tmp.W(),  actualNumArgs.W());
        __ CMov(tmp, tmp.W(), actualNumArgs.W(), Condition::LO);
        __ Cbz(tmp, pushCallThis);
        CopyArgumentWithArgV(assembler, tmp, argV);
    }
}

void AssemblerStubs::CopyArgumentWithArgV(ExtendedAssembler *assembler, Register argc, Register argV)
{
    [[maybe_unused]] TempRegister2Scope scope2(assembler);
    Register argVEnd = __ AvailableRegister1();
    Register sp(SP);
    Label copyArgLoop;
    Register arg = __ TempRegister2();
    __ Sub(argVEnd.W(), argc, Immediate(1));
    __ Add(argVEnd, argV, Operand(argVEnd.W(), UXTW, SHIFT_OF_FRAMESLOT));
    __ Bind(&copyArgLoop);
    __ Ldr(arg, MemoryOperand(argVEnd, -FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Subs(argc, argc, Immediate(1));
    __ Str(arg, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ B(Condition::NE, &copyArgLoop);
}

void AssemblerStubs::PushMandatoryJSArgs(ExtendedAssembler *assembler, Register jsfunc,
                                         Register thisObj, Register newTarget)
{
    Register sp(SP);
    __ Str(thisObj, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Str(newTarget, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
    __ Str(jsfunc, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));
}

void AssemblerStubs::PopJSFunctionArgs(ExtendedAssembler *assembler, Register expectedNumArgs, Register actualNumArgs)
{
    Register sp(SP);
    Register fp(X6);
    Label aligned;
    const int64_t argoffsetSlot = static_cast<int64_t>(CommonArgIdx::FUNC) - 1;
    if (expectedNumArgs != actualNumArgs) {
        TempRegister1Scope scop1(assembler);
        Register tmp = __ TempRegister1();
        __ Cmp(expectedNumArgs, actualNumArgs);
        __ CMov(tmp, expectedNumArgs, actualNumArgs, Condition::HI);
        __ Add(sp, sp, Operand(tmp, UXTW, SHIFT_OF_FRAMESLOT));
    } else {
        __ Add(sp, sp, Operand(expectedNumArgs, UXTW, SHIFT_OF_FRAMESLOT));
    }
    __ Add(sp, sp, Immediate(argoffsetSlot * FRAME_SLOT_SIZE));
    __ Mov(fp, sp);
    __ Tst(fp, LogicalImmediate::Create(0xf, RegXSize));  // 0xf: 0x1111
    __ B(Condition::EQ, &aligned);
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    __ Bind(&aligned);
}

void AssemblerStubs::PushJSFunctionEntryFrame (ExtendedAssembler *assembler, Register prevFp)
{
    Register fp(X29);
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    __ SaveFpAndLr();
    Register frameType = __ TempRegister2();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_ENTRY_FRAME)));
    // 2 : 2 means pairs
    __ Stp(prevFp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
    __ CalleeSave();
}

void AssemblerStubs::PopJSFunctionEntryFrame(ExtendedAssembler *assembler, Register glue)
{
    Register fp(X29);
    Register sp(SP);
    Register prevFp(X1);
    __ CalleeRestore();

    // pop prevLeaveFrameFp to restore thread->currentFrame_
    __ Ldr(prevFp, MemoryOperand(sp, FRAME_SLOT_SIZE, AddrMode::POSTINDEX));
    __ Str(prevFp, MemoryOperand(glue, JSThread::GlueData::GetLeaveFrameOffset(false)));

    // pop entry frame type
    __ Add(sp, sp, Immediate(FRAME_SLOT_SIZE));
    // restore return address
    __ RestoreFpAndLr();
}

void AssemblerStubs::PushOptimizedJSFunctionFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ SaveFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)));
    // 2 : 2 means pairs. X19 means calleesave and 16bytes align
    __ Stp(Register(X19), frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
}

void AssemblerStubs::PopOptimizedJSFunctionFrame(ExtendedAssembler *assembler)
{
    TempRegister2Scope temp2Scope(assembler);
    Register sp(SP);
    Register frameType = __ TempRegister2();
    // 2 : 2 means pop call site sp and type
    __ Ldp(Register(X19), frameType, MemoryOperand(sp, FRAME_SLOT_SIZE * 2, AddrMode::POSTINDEX));
    __ RestoreFpAndLr();
}

void AssemblerStubs::PushOptimizedFrame(ExtendedAssembler *assembler, Register callSiteSp)
{
    Register sp(SP);
    TempRegister2Scope temp2Scope(assembler);
    Register frameType = __ TempRegister2();
    __ SaveFpAndLr();
    // construct frame
    __ Mov(frameType, Immediate(static_cast<int64_t>(FrameType::OPTIMIZED_FRAME)));
    // 2 : 2 means pairs
    __ Stp(callSiteSp, frameType, MemoryOperand(sp, -FRAME_SLOT_SIZE * 2, AddrMode::PREINDEX));
}

void AssemblerStubs::PopOptimizedFrame(ExtendedAssembler *assembler)
{
    Register sp(SP);
    // 2 : 2 means pop call site sp and type
    __ Add(sp, sp, Immediate(2 * FRAME_SLOT_SIZE));
    __ RestoreFpAndLr();
}

void AssemblerStubs::JSCallWithArgV(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(JSCallWithArgV));
    Register sp(SP);
    Register glue(X0);
    Register actualNumArgs(X1);
    Register jsfunc(X2);
    Register newTarget(X3);
    Register thisObj(X4);
    Register argV(X5);
    Register callsiteSp = __ AvailableRegister2();
    Label pushCallThis;

    __ Mov(callsiteSp, sp);
    PushOptimizedFrame(assembler, callsiteSp);
    __ Cbz(actualNumArgs, &pushCallThis);
    {
        TempRegister1Scope scope1(assembler);
        Register tmp = __ TempRegister1();
        __ Mov(tmp, actualNumArgs);
        CopyArgumentWithArgV(assembler, tmp, argV);
    }
    __ Bind(&pushCallThis);
    PushMandatoryJSArgs(assembler, jsfunc, thisObj, newTarget);
    __ Add(actualNumArgs, actualNumArgs, Immediate(NUM_MANDATORY_JSFUNC_ARGS));
    __ Str(actualNumArgs, MemoryOperand(sp, -FRAME_SLOT_SIZE, AddrMode::PREINDEX));

    __ CallAssemblerStub(RTSTUB_ID(JSCall), false);
    __ Ldr(actualNumArgs, MemoryOperand(sp, 0));
    PopJSFunctionArgs(assembler, actualNumArgs, actualNumArgs);
    PopOptimizedFrame(assembler);
    __ Ret();
}
}  // panda::ecmascript::aarch64