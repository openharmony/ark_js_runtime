/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H

#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"

namespace panda::ecmascript::kungfu {
void InterpreterStub::SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val)
{
    Store(VariableType::INT64(), glue, sp, IntPtrMul(IntPtr(sizeof(JSTaggedType)), idx), val);
}

GateRef InterpreterStub::GetVregValue(GateRef sp, GateRef idx)
{
    return Load(VariableType::JS_ANY(), sp, IntPtrMul(IntPtr(sizeof(JSTaggedType)), idx));
}

GateRef InterpreterStub::ReadInst8_0(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(1));
}

GateRef InterpreterStub::ReadInst8_1(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(2));  // 2 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_2(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(3));  // 3 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_3(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(4));  // 4 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_4(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(5));  // 5 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_5(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(6));  // 6 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_6(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(7));  // 7 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_7(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(8));  // 8 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_8(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(9));  // 9 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst4_0(GateRef pc)
{
    return Int8And(Load(VariableType::INT8(), pc, IntPtr(1)), Int8(0xf));
}

GateRef InterpreterStub::ReadInst4_1(GateRef pc)
{
    return Int8And(
        Int8LSR(Load(VariableType::INT8(), pc, IntPtr(1)), Int8(4)), Int8(0xf));
}

GateRef InterpreterStub::ReadInst4_2(GateRef pc)
{
    return Int8And(Load(VariableType::INT8(), pc, IntPtr(2)), Int8(0xf));
}

GateRef InterpreterStub::ReadInst4_3(GateRef pc)
{
    return Int8And(
        Int8LSR(Load(VariableType::INT8(), pc, IntPtr(2)), Int8(4)), Int8(0xf));
}

GateRef InterpreterStub::ReadInstSigned8_0(GateRef pc)
{
    GateRef x = Load(VariableType::INT8(), pc, IntPtr(1));
    return GetEnvironment()->GetBuilder().UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT32), x);
}

GateRef InterpreterStub::ReadInstSigned16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst = Load(VariableType::INT8(), pc, IntPtr(2));
    GateRef currentInst1 = GetEnvironment()->GetBuilder().UnaryArithmetic(
        OpCode(OpCode::SEXT_TO_INT32), currentInst);
    GateRef currentInst2 = Int32LSL(currentInst1, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst2, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInstSigned32_0(GateRef pc)
{
    /* 4 : skip 8 bits of opcode and 24 bits of low bits */
    GateRef x = Load(VariableType::INT8(), pc, IntPtr(4));
    GateRef currentInst = GetEnvironment()->GetBuilder().UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT32), x);
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst16_1(GateRef pc)
{
    /* 3 : skip 8 bits of opcode, 8 bits of prefix and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 2: skip 8 bits of opcode and 8 bits of prefix */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_1(pc)));
}

GateRef InterpreterStub::ReadInst16_2(GateRef pc)
{
    /* 4 : skip 8 bits of opcode, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 3: skip 8 bits of opcode and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_2(pc)));
}

GateRef InterpreterStub::ReadInst16_3(GateRef pc)
{
    /* 5 : skip 8 bits of opcode, 8 bits of prefix, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_4(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 4: skip 8 bits of opcode, 8 bits of prefix and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_3(pc)));
}

GateRef InterpreterStub::ReadInst16_5(GateRef pc)
{
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_6(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 6: skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_5(pc)));
}

GateRef InterpreterStub::GetFrame(GateRef CurrentSp)
{
    return IntPtrSub(CurrentSp, IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetPcFromFrame(GateRef frame)
{
    return Load(VariableType::POINTER(), frame, IntPtr(0));
}

GateRef InterpreterStub::GetFunctionFromFrame(GateRef frame)
{
    return Load(VariableType::JS_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetCallSizeFromFrame(GateRef frame)
{
    return Load(VariableType::POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetAccFromFrame(GateRef frame)
{
    return Load(VariableType::JS_ANY(), frame,
        IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetEnvFromFrame(GateRef frame)
{
    return Load(VariableType::JS_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetEnvFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

GateRef InterpreterStub::GetProfileTypeInfoFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::PROFILE_TYPE_INFO_OFFSET));
}

GateRef InterpreterStub::GetModuleFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::ECMA_MODULE_OFFSET));
}

GateRef InterpreterStub::GetConstpoolFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::CONSTANT_POOL_OFFSET));
}

GateRef InterpreterStub::GetResumeModeFromGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        UInt32LSR(bitfield, Int32(JSGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
}

void InterpreterStub::SetPcToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame, IntPtr(0), value);
}

void InterpreterStub::SetCallSizeToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::POINTER(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetAccToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetEnvToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetConstantPoolToFunction(GateRef glue, GateRef function, GateRef value)
{
    Store(VariableType::INT64(), glue, function,
          IntPtr(JSFunction::CONSTANT_POOL_OFFSET), value);
}

void InterpreterStub::SetResolvedToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
    GateRef mask = Int32(
        ~(((1<<JSFunction::ResolvedBits::SIZE) - 1) << JSFunction::ResolvedBits::START_BIT));
    GateRef result = Int32Or(Int32And(bitfield, mask),
        Int32LSL(ZExtInt1ToInt32(value), Int32(JSFunction::ResolvedBits::START_BIT)));
    Store(VariableType::INT32(), glue, function, IntPtr(JSFunction::BIT_FIELD_OFFSET), result);
}

void InterpreterStub::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::INT64(), glue, function, offset, value);
}

void InterpreterStub::SetModuleToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value);
}

GateRef InterpreterStub::GetCurrentSpFrame(GateRef glue)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef spOffset = IntPtr(JSThread::GlueData::GetCurrentFrameOffset(isArch32));
    return Load(VariableType::POINTER(), glue, spOffset);
}

void InterpreterStub::SetCurrentSpFrame(GateRef glue, GateRef value)
{
    GateRef spOffset = IntPtr(JSThread::GlueData::GetCurrentFrameOffset(GetEnvironment()->Is32Bit()));
    Store(VariableType::POINTER(), glue, glue, spOffset, value);
}

GateRef InterpreterStub::ReadInst32_0(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_3(pc));
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst32_1(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_4(pc));
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_1(pc)));
}

GateRef InterpreterStub::ReadInst32_2(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_5(pc));
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_4(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_2(pc)));
}

GateRef InterpreterStub::ReadInst64_0(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt64(ReadInst8_7(pc));
    GateRef currentInst1 = Int64LSL(currentInst, Int64(8));
    GateRef currentInst2 = Int64Add(currentInst1, ZExtInt8ToInt64(ReadInst8_6(pc)));
    GateRef currentInst3 = Int64LSL(currentInst2, Int64(8));
    GateRef currentInst4 = Int64Add(currentInst3, ZExtInt8ToInt64(ReadInst8_5(pc)));
    GateRef currentInst5 = Int64LSL(currentInst4, Int64(8));
    GateRef currentInst6 = Int64Add(currentInst5, ZExtInt8ToInt64(ReadInst8_4(pc)));
    GateRef currentInst7 = Int64LSL(currentInst6, Int64(8));
    GateRef currentInst8 = Int64Add(currentInst7, ZExtInt8ToInt64(ReadInst8_3(pc)));
    GateRef currentInst9 = Int64LSL(currentInst8, Int64(8));
    GateRef currentInst10 = Int64Add(currentInst9, ZExtInt8ToInt64(ReadInst8_2(pc)));
    GateRef currentInst11 = Int64LSL(currentInst10, Int64(8));
    GateRef currentInst12 = Int64Add(currentInst11, ZExtInt8ToInt64(ReadInst8_1(pc)));
    GateRef currentInst13 = Int64LSL(currentInst12, Int64(8));
    return Int64Add(currentInst13, ZExtInt8ToInt64(ReadInst8_0(pc)));
}

template<typename... Args>
void InterpreterStub::DispatchBase(GateRef bcOffset, const CallSignature *signature, GateRef glue, Args... args)
{
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result =
        GetEnvironment()->GetBuilder().BytecodeCall(signature, glue, bcOffset, depend, {glue, args...});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
}

void InterpreterStub::Dispatch(GateRef glue, GateRef pc, GateRef sp, GateRef constpool, GateRef profileTypeInfo,
                               GateRef acc, GateRef hotnessCounter, GateRef format)
{
    GateRef newPc = IntPtrAdd(pc, format);
    GateRef opcode = Load(VariableType::INT8(), newPc);
    GateRef opcodeOffset = IntPtrMul(ChangeInt32ToIntPtr(ZExtInt8ToInt32(opcode)), IntPtrSize());
    const CallSignature *bytecodeHandler = BytecodeStubCSigns::Get(BYTECODE_STUB_BEGIN_ID);
    DispatchBase(opcodeOffset, bytecodeHandler, glue, newPc, sp, constpool, profileTypeInfo, acc, hotnessCounter);
    Return();
}

void InterpreterStub::DispatchLast(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                                   GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
{
    GateRef opcodeOffset = IntPtrMul(
        IntPtr(BytecodeStubCSigns::ID_ExceptionHandler), IntPtrSize());
    const CallSignature *bytecodeHandler = BytecodeStubCSigns::Get(BYTECODE_STUB_BEGIN_ID);
    DispatchBase(opcodeOffset, bytecodeHandler, glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);
    Return();
}

template<RuntimeStubCSigns::ID id, typename... Args>
void InterpreterStub::DispatchCommonCall(GateRef glue, GateRef function, Args... args)
{
    GateRef index = IntPtr(id);
    const CallSignature *signature = RuntimeStubCSigns::Get(id);
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result = GetEnvironment()->GetBuilder().NoGcRuntimeCall(
        signature, glue, index, depend, {glue, function, args...});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
    Return();
}

template<RuntimeStubCSigns::ID id, typename... Args>
GateRef InterpreterStub::CommonCallNative(GateRef glue, GateRef function, Args... args)
{
    GateRef index = IntPtr(id);
    const CallSignature *signature = RuntimeStubCSigns::Get(id);
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result = GetEnvironment()->GetBuilder().NoGcRuntimeCall(
        signature, glue, index, depend, {glue, function, args...});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
    return result;
}

GateRef InterpreterStub::GetObjectFromConstPool(GateRef constpool, GateRef index)
{
    return GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, index);
}

GateRef InterpreterStub::FunctionIsResolved(GateRef object)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(object);
    // decode
    return Int32NotEqual(
        Int32And(
            UInt32LSR(bitfield, Int32(JSFunction::ResolvedBits::START_BIT)),
            Int32((1LU << JSFunction::ResolvedBits::SIZE) - 1)),
        Int32(0));
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
