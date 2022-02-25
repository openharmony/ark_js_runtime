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

namespace panda::ecmascript::kungfu {
void InterpreterStub::SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val)
{
    Store(VariableType::INT64(), glue, sp, IntPtrMul(GetIntPtrConstant(sizeof(JSTaggedType)), idx), val);
}

GateRef InterpreterStub::GetVregValue(GateRef sp, GateRef idx)
{
    return Load(VariableType::JS_ANY(), sp, IntPtrMul(GetIntPtrConstant(sizeof(JSTaggedType)), idx));
}

GateRef InterpreterStub::ReadInst8_0(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(1));
}

GateRef InterpreterStub::ReadInst8_1(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(2));  // 2 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_2(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(3));  // 3 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_3(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(4));  // 4 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_4(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(5));  // 5 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_5(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(6));  // 6 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_6(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(7));  // 7 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_7(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(8));  // 8 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst8_8(GateRef pc)
{
    return Load(VariableType::INT8(), pc, GetIntPtrConstant(9));  // 9 : skip 1 byte of bytecode
}

GateRef InterpreterStub::ReadInst4_0(GateRef pc)
{
    return Int8And(Load(VariableType::INT8(), pc, GetIntPtrConstant(1)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInst4_1(GateRef pc)
{
    return Int8And(
        Int8LSR(Load(VariableType::INT8(), pc, GetIntPtrConstant(1)), GetInt8Constant(4)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInst4_2(GateRef pc)
{
    return Int8And(Load(VariableType::INT8(), pc, GetIntPtrConstant(2)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInst4_3(GateRef pc)
{
    return Int8And(
        Int8LSR(Load(VariableType::INT8(), pc, GetIntPtrConstant(2)), GetInt8Constant(4)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInstSigned8_0(GateRef pc)
{
    GateRef x = Load(VariableType::INT8(), pc, GetIntPtrConstant(1));
    return GetEnvironment()->GetCircuitBuilder().NewArithmeticGate(OpCode(OpCode::SEXT_TO_INT32), x);
}

GateRef InterpreterStub::ReadInstSigned16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst = Load(VariableType::INT8(), pc, GetIntPtrConstant(2));
    GateRef currentInst1 = GetEnvironment()->GetCircuitBuilder().NewArithmeticGate(
        OpCode(OpCode::SEXT_TO_INT32), currentInst);
    GateRef currentInst2 = Int32LSL(currentInst1, GetInt32Constant(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst2, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInstSigned32_0(GateRef pc)
{
    /* 4 : skip 8 bits of opcode and 24 bits of low bits */
    GateRef x = Load(VariableType::INT8(), pc, GetIntPtrConstant(4));
    GateRef currentInst = GetEnvironment()->GetCircuitBuilder().NewArithmeticGate(OpCode(OpCode::SEXT_TO_INT32), x);
    GateRef currentInst1 = Int32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst16_1(GateRef pc)
{
    /* 3 : skip 8 bits of opcode, 8 bits of prefix and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 2: skip 8 bits of opcode and 8 bits of prefix */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_1(pc)));
}

GateRef InterpreterStub::ReadInst16_2(GateRef pc)
{
    /* 4 : skip 8 bits of opcode, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 3: skip 8 bits of opcode and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_2(pc)));
}

GateRef InterpreterStub::ReadInst16_3(GateRef pc)
{
    /* 5 : skip 8 bits of opcode, 8 bits of prefix, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_4(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 4: skip 8 bits of opcode, 8 bits of prefix and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_3(pc)));
}

GateRef InterpreterStub::ReadInst16_5(GateRef pc)
{
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_6(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 6: skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_5(pc)));
}

GateRef InterpreterStub::GetFrame(GateRef CurrentSp)
{
    return IntPtrSub(CurrentSp, GetIntPtrConstant(InterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetPcFromFrame(GateRef frame)
{
    return Load(VariableType::POINTER(), frame, GetIntPtrConstant(0));
}

GateRef InterpreterStub::GetFunctionFromFrame(GateRef frame)
{
    return Load(VariableType::JS_POINTER(), frame,
        GetIntPtrConstant(InterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetAccFromFrame(GateRef frame)
{
    return Load(VariableType::JS_ANY(), frame,
        GetIntPtrConstant(InterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetEnvFromFrame(GateRef frame)
{
    return Load(VariableType::JS_POINTER(), frame,
        GetIntPtrConstant(InterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStub::GetProfileTypeInfoFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, GetIntPtrConstant(JSFunction::PROFILE_TYPE_INFO_OFFSET));
}

GateRef InterpreterStub::GetConstpoolFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, GetIntPtrConstant(JSFunction::CONSTANT_POOL_OFFSET));
}

void InterpreterStub::SetEnvToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
        GetIntPtrConstant(InterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetPcToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame, GetIntPtrConstant(0), value);
}

void InterpreterStub::SetAccToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          GetIntPtrConstant(InterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          GetIntPtrConstant(InterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStub::SetConstantPoolToFunction(GateRef glue, GateRef function, GateRef value)
{
    Store(VariableType::INT64(), glue, function,
          GetIntPtrConstant(JSFunction::CONSTANT_POOL_OFFSET), value);
}

void InterpreterStub::SetResolvedToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
    GateRef mask = GetInt32Constant(
        ~(((1<<JSFunction::ResolvedBits::SIZE) - 1) << JSFunction::ResolvedBits::START_BIT));
    GateRef result = Int32Or(Int32And(bitfield, mask),
        Int32LSL(value, GetInt32Constant(JSFunction::ResolvedBits::START_BIT)));
    Store(VariableType::INT32(), glue, function, GetIntPtrConstant(JSFunction::BIT_FIELD_OFFSET), result);
}

void InterpreterStub::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = GetIntPtrConstant(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::INT64(), glue, function, offset, value);
}

GateRef InterpreterStub::GetCurrentSpFrame(GateRef glue)
{
    GateRef spOffset = GetIntPtrConstant(
        GetEnvironment()->GetGlueOffset(JSThread::GlueID::CURRENT_FRAME));
    return Load(VariableType::POINTER(), glue, spOffset);
}

void InterpreterStub::SetCurrentSpFrame(GateRef glue, GateRef value)
{
    GateRef spOffset = GetIntPtrConstant(
        GetEnvironment()->GetGlueOffset(JSThread::GlueID::CURRENT_FRAME));
    Store(VariableType::POINTER(), glue, glue, spOffset, value);
}

GateRef InterpreterStub::ReadInst32_0(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_3(pc));
    GateRef currentInst1 = Int32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst32_1(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_4(pc));
    GateRef currentInst1 = Int32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_1(pc)));
}

GateRef InterpreterStub::ReadInst32_2(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_5(pc));
    GateRef currentInst1 = Int32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_4(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_2(pc)));
}

GateRef InterpreterStub::ReadInst64_0(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt64(ReadInst8_7(pc));
    GateRef currentInst1 = Int64LSL(currentInst, GetInt64Constant(8));
    GateRef currentInst2 = Int64Add(currentInst1, ZExtInt8ToInt64(ReadInst8_6(pc)));
    GateRef currentInst3 = Int64LSL(currentInst2, GetInt64Constant(8));
    GateRef currentInst4 = Int64Add(currentInst3, ZExtInt8ToInt64(ReadInst8_5(pc)));
    GateRef currentInst5 = Int64LSL(currentInst4, GetInt64Constant(8));
    GateRef currentInst6 = Int64Add(currentInst5, ZExtInt8ToInt64(ReadInst8_4(pc)));
    GateRef currentInst7 = Int64LSL(currentInst6, GetInt64Constant(8));
    GateRef currentInst8 = Int64Add(currentInst7, ZExtInt8ToInt64(ReadInst8_3(pc)));
    GateRef currentInst9 = Int64LSL(currentInst8, GetInt64Constant(8));
    GateRef currentInst10 = Int64Add(currentInst9, ZExtInt8ToInt64(ReadInst8_2(pc)));
    GateRef currentInst11 = Int64LSL(currentInst10, GetInt64Constant(8));
    GateRef currentInst12 = Int64Add(currentInst11, ZExtInt8ToInt64(ReadInst8_1(pc)));
    GateRef currentInst13 = Int64LSL(currentInst12, GetInt64Constant(8));
    return Int64Add(currentInst13, ZExtInt8ToInt64(ReadInst8_0(pc)));
}

void InterpreterStub::Dispatch(GateRef glue, GateRef pc, GateRef sp, GateRef constpool, GateRef profileTypeInfo,
                               GateRef acc, GateRef hotnessCounter, GateRef format)
{
    GateRef newPc = IntPtrAdd(pc, format);
    GateRef opcode = Load(VariableType::INT8(), newPc);
    GateRef opcodeOffset = IntPtrMul(
        ChangeInt32ToIntPtr(ZExtInt8ToInt32(opcode)), GetIntPtrSize());
    StubDescriptor *bytecodeHandler = GET_STUBDESCRIPTOR(BytecodeHandler);
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result = GetEnvironment()->GetCircuitBuilder().NewBytecodeCallGate(bytecodeHandler, glue, opcodeOffset,
        depend, {glue, newPc, sp, constpool, profileTypeInfo, acc, hotnessCounter});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
    Return();
}

void InterpreterStub::DispatchLast(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                                   GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
{
    GateRef opcodeOffset = IntPtrMul(
        GetIntPtrConstant(EcmaOpcode::LAST_OPCODE), GetIntPtrSize());
    StubDescriptor *bytecodeHandler = GET_STUBDESCRIPTOR(BytecodeHandler);
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result = GetEnvironment()->GetCircuitBuilder().NewBytecodeCallGate(bytecodeHandler, glue, opcodeOffset,
        depend, {glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
    Return();
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
            UInt32LSR(bitfield, GetInt32Constant(JSFunction::ResolvedBits::START_BIT)),
            GetInt32Constant((1LU << JSFunction::ResolvedBits::SIZE) - 1)),
        GetInt32Constant(0));
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
