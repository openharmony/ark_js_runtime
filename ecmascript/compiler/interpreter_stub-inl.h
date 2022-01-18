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
    Store(MachineType::UINT64, glue, sp, ArchRelatePtrMul(GetArchRelateConstant(sizeof(JSTaggedType)), idx), val);
}

GateRef InterpreterStub::GetVregValue(GateRef sp, GateRef idx)
{
    return Load(MachineType::TAGGED, sp, ArchRelatePtrMul(GetArchRelateConstant(sizeof(JSTaggedType)), idx));
}

GateRef InterpreterStub::ReadInst8_0(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(1));
}

GateRef InterpreterStub::ReadInst8_1(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(2));
}

GateRef InterpreterStub::ReadInst8_2(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(3));
}

GateRef InterpreterStub::ReadInst8_3(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(4));
}

GateRef InterpreterStub::ReadInst8_4(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(5));
}

GateRef InterpreterStub::ReadInst8_5(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(6));
}

GateRef InterpreterStub::ReadInst8_6(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(7));
}

GateRef InterpreterStub::ReadInst8_7(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(8));
}

GateRef InterpreterStub::ReadInst8_8(GateRef pc)
{
    return Load(MachineType::UINT8, pc, GetArchRelateConstant(9));
}

GateRef InterpreterStub::ReadInst4_0(GateRef pc)
{
    return Word8And(Load(MachineType::UINT8, pc, GetArchRelateConstant(1)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInst4_1(GateRef pc)
{
    return Word8And(
        Word8LSR(Load(MachineType::UINT8, pc, GetArchRelateConstant(1)), GetInt8Constant(4)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInst4_2(GateRef pc)
{
    return Word8And(Load(MachineType::UINT8, pc, GetArchRelateConstant(2)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInst4_3(GateRef pc)
{
    return Word8And(
        Word8LSR(Load(MachineType::UINT8, pc, GetArchRelateConstant(2)), GetInt8Constant(4)), GetInt8Constant(0xf));
}

GateRef InterpreterStub::ReadInstSigned8_0(GateRef pc)
{
    GateRef x = Load(MachineType::INT8, pc, GetArchRelateConstant(1));
    return GetEnvironment()->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT8_TO_INT32), x);
}

GateRef InterpreterStub::ReadInstSigned16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst = Load(MachineType::INT8, pc, GetArchRelateConstant(2));
    GateRef currentInst1 = GetEnvironment()->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT8_TO_INT32), currentInst);
    GateRef currentInst2 = Word32LSL(currentInst1, GetInt32Constant(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst2, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInstSigned32_0(GateRef pc)
{
    /* 4 : skip 8 bits of opcode and 24 bits of low bits */
    GateRef x = Load(MachineType::INT8, pc, GetArchRelateConstant(4));
    GateRef currentInst = GetEnvironment()->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT8_TO_INT32), x);
    GateRef currentInst1 = Word32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Word32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Word32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef currentInst2 = Word16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst16_1(GateRef pc)
{
    /* 3 : skip 8 bits of opcode, 8 bits of prefix and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef currentInst2 = Word16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 2: skip 8 bits of opcode and 8 bits of prefix */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_1(pc)));
}

GateRef InterpreterStub::ReadInst16_2(GateRef pc)
{
    /* 4 : skip 8 bits of opcode, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef currentInst2 = Word16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 3: skip 8 bits of opcode and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_2(pc)));
}

GateRef InterpreterStub::ReadInst16_3(GateRef pc)
{
    /* 5 : skip 8 bits of opcode, 8 bits of prefix, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_4(pc));
    GateRef currentInst2 = Word16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 4: skip 8 bits of opcode, 8 bits of prefix and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_3(pc)));
}

GateRef InterpreterStub::ReadInst16_5(GateRef pc)
{
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_6(pc));
    GateRef currentInst2 = Word16LSL(currentInst1, GetInt16Constant(8));  // 8 : set as high 8 bits
    /* 6: skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_5(pc)));
}

GateRef InterpreterStub::GetFrame(GateRef CurrentSp)
{
    return ArchRelateSub(CurrentSp, GetArchRelateConstant(InterpretedFrame::GetSize(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::GetPcFromFrame(GateRef frame)
{
    return Load(MachineType::NATIVE_POINTER, frame, GetArchRelateConstant(0));
}

GateRef InterpreterStub::GetSpFromFrame(GateRef frame)
{
    return Load(MachineType::NATIVE_POINTER, frame, GetArchRelateConstant(InterpretedFrame::GetSpOffset(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::GetConstpoolFromFrame(GateRef frame)
{
    return Load(MachineType::TAGGED, frame, GetArchRelateConstant(InterpretedFrame::GetConstpoolOffset(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::GetFunctionFromFrame(GateRef frame)
{
    return Load(MachineType::TAGGED, frame, GetArchRelateConstant(InterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::GetProfileTypeInfoFromFrame(GateRef frame)
{
    return Load(MachineType::TAGGED, frame, GetArchRelateConstant(InterpretedFrame::GetProfileTypeInfoOffset(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::GetAccFromFrame(GateRef frame)
{
    return Load(MachineType::TAGGED, frame, GetArchRelateConstant(InterpretedFrame::GetAccOffset(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::GetEnvFromFrame(GateRef frame)
{
    return Load(MachineType::TAGGED, frame, GetArchRelateConstant(InterpretedFrame::GetEnvOffset(GetEnvironment()->IsArm32())));
}

void InterpreterStub::SetEnvToFrame(GateRef glue, GateRef frame, GateRef env)
{
    Store(MachineType::UINT64, glue, frame, GetArchRelateConstant(InterpretedFrame::GetEnvOffset(GetEnvironment()->IsArm32())), env);
}

GateRef InterpreterStub::LoadAccFromSp(GateRef glue, GateRef CurrentSp)
{
    return Load(MachineType::TAGGED, CurrentSp, GetArchRelateConstant(InterpretedFrame::GetAccOffset(GetEnvironment()->IsArm32())));
}

void InterpreterStub::SavePc(GateRef glue, GateRef CurrentSp, GateRef pc)
{
    Store(MachineType::NATIVE_POINTER, glue, GetFrame(CurrentSp), GetArchRelateConstant(0), pc);
}

void InterpreterStub::SaveAcc(GateRef glue, GateRef CurrentSp, GateRef acc)
{
    Store(MachineType::UINT64, glue, GetFrame(CurrentSp), GetArchRelateConstant(InterpretedFrame::GetAccOffset(GetEnvironment()->IsArm32())), acc);
}

GateRef InterpreterStub::RestoreAcc(GateRef CurrentSp)
{
    return Load(MachineType::TAGGED, GetFrame(CurrentSp), GetArchRelateConstant(InterpretedFrame::GetAccOffset(GetEnvironment()->IsArm32())));
}

GateRef InterpreterStub::ReadInst32_0(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_3(pc));
    GateRef currentInst1 = Word32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Word32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Word32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStub::ReadInst32_1(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_4(pc));
    GateRef currentInst1 = Word32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst3 = Word32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst5 = Word32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_1(pc)));
}

GateRef InterpreterStub::ReadInst32_2(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_5(pc));
    GateRef currentInst1 = Word32LSL(currentInst, GetInt32Constant(8));
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_4(pc)));
    GateRef currentInst3 = Word32LSL(currentInst2, GetInt32Constant(8));
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst5 = Word32LSL(currentInst4, GetInt32Constant(8));
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_2(pc)));
}

GateRef InterpreterStub::ReadInst64_0(GateRef pc)
{
    GateRef currentInst = ZExtInt8ToInt64(ReadInst8_7(pc));
    GateRef currentInst1 = Word64LSL(currentInst, GetWord64Constant(8));
    GateRef currentInst2 = Int64Add(currentInst1, ZExtInt8ToInt64(ReadInst8_6(pc)));
    GateRef currentInst3 = Word64LSL(currentInst2, GetWord64Constant(8));
    GateRef currentInst4 = Int64Add(currentInst3, ZExtInt8ToInt64(ReadInst8_5(pc)));
    GateRef currentInst5 = Word64LSL(currentInst4, GetWord64Constant(8));
    GateRef currentInst6 = Int64Add(currentInst5, ZExtInt8ToInt64(ReadInst8_4(pc)));
    GateRef currentInst7 = Word64LSL(currentInst6, GetWord64Constant(8));
    GateRef currentInst8 = Int64Add(currentInst7, ZExtInt8ToInt64(ReadInst8_3(pc)));
    GateRef currentInst9 = Word64LSL(currentInst8, GetWord64Constant(8));
    GateRef currentInst10 = Int64Add(currentInst9, ZExtInt8ToInt64(ReadInst8_2(pc)));
    GateRef currentInst11 = Word64LSL(currentInst10, GetWord64Constant(8));
    GateRef currentInst12 = Int64Add(currentInst11, ZExtInt8ToInt64(ReadInst8_1(pc)));
    GateRef currentInst13 = Word64LSL(currentInst12, GetWord64Constant(8));
    return Int64Add(currentInst13, ZExtInt8ToInt64(ReadInst8_0(pc)));
}

void InterpreterStub::Dispatch(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                        GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter, GateRef format)
{
    GateRef newPc = PtrAdd(pc, format);
    GateRef opcode = Load(MachineType::UINT8, newPc);
    GateRef opcodeOffset = ArchRelatePtrMul(
        ChangeInt32ToUintPtr(ZExtInt8ToInt32(opcode)), GetArchRelatePointerSize());
    StubDescriptor *bytecodeHandler = GET_STUBDESCRIPTOR(BytecodeHandler);
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result = GetEnvironment()->GetCircuitBuilder().NewBytecodeCallGate(bytecodeHandler, glue, opcodeOffset, depend,
        {glue, newPc, sp, constpool, profileTypeInfo, acc, hotnessCounter});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
    Return();
}

void InterpreterStub::DispatchLast(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                                   GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
{
    GateRef opcodeOffset = ArchRelatePtrMul(
        GetArchRelateConstant(EcmaOpcode::LAST_OPCODE), GetArchRelatePointerSize());
    StubDescriptor *bytecodeHandler = GET_STUBDESCRIPTOR(BytecodeHandler);
    auto depend = GetEnvironment()->GetCurrentLabel()->GetDepend();
    GateRef result = GetEnvironment()->GetCircuitBuilder().NewBytecodeCallGate(bytecodeHandler, glue, opcodeOffset, depend,
        {glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter});
    GetEnvironment()->GetCurrentLabel()->SetDepend(result);
    Return();
}

GateRef InterpreterStub::GetObjectFromConstPool(GateRef constpool, GateRef index)
{
    return GetValueFromTaggedArray(MachineType::TAGGED, constpool, index);
}

GateRef InterpreterStub::FunctionIsResolved(GateRef object)
{
    GateRef bitfield = TaggedGetInt(GetFunctionInfoFlagFromJSFunction(object));
    // decode
    return Word32NotEqual(
        Word32And(
            Word32LSR(bitfield, GetInt32Constant(JSFunction::ResolvedBit::START_BIT)),
            GetInt32Constant((1LU << JSFunction::ResolvedBit::SIZE) - 1)),
        GetInt32Constant(0));
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
