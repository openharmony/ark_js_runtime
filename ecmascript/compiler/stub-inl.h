
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

#ifndef ECMASCRIPT_COMPILER_STUB_INL_H
#define ECMASCRIPT_COMPILER_STUB_INL_H

#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/stub.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/message_string.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript::kungfu {
using JSFunction = panda::ecmascript::JSFunction;
using PropertyBox = panda::ecmascript::PropertyBox;

inline GateRef Stub::Int8(int8_t value)
{
    return env_.GetBulder()->Int8(value);
}

inline GateRef Stub::Int16(int16_t value)
{
    return env_.GetBulder()->Int16(value);
}

inline GateRef Stub::Int32(int32_t value)
{
    return env_.GetBulder()->Int32(value);
};

inline GateRef Stub::Int64(int64_t value)
{
    return env_.GetBulder()->Int64(value);
}

inline GateRef Stub::IntPtr(int64_t value)
{
    return env_.Is32Bit() ? Int32(value) : Int64(value);
};

inline GateRef Stub::IntPtrSize()
{
    return env_.Is32Bit() ? Int32(sizeof(uint32_t)) : Int64(sizeof(uint64_t));
}

inline GateRef Stub::True()
{
    return TruncInt32ToInt1(Int32(1));
}

inline GateRef Stub::False()
{
    return TruncInt32ToInt1(Int32(0));
}

inline GateRef Stub::Boolean(bool value)
{
    return env_.GetBulder()->Boolean(value);
}

inline GateRef Stub::Double(double value)
{
    return env_.GetBulder()->Double(value);
}

inline GateRef Stub::Undefined(VariableType type)
{
    return env_.GetBulder()->UndefineConstant(type.GetGateType());
}

inline GateRef Stub::Hole(VariableType type)
{
    return env_.GetBulder()->HoleConstant(type.GetGateType());
}

inline GateRef Stub::Null(VariableType type)
{
    return env_.GetBulder()->NullConstant(type.GetGateType());
}

inline GateRef Stub::Exception(VariableType type)
{
    return env_.GetBulder()->ExceptionConstant(type.GetGateType());
}

inline GateRef Stub::PtrMul(GateRef x, GateRef y)
{
    if (env_.Is32Bit()) {
        return Int32Mul(x, y);
    } else {
        return Int64Mul(x, y);
    }
}

inline GateRef Stub::RelocatableData(uint64_t value)
{
    return env_.GetBulder()->RelocatableData(value);
}

// parameter
inline GateRef Stub::Argument(size_t index)
{
    return env_.GetArgument(index);
}

inline GateRef Stub::Int1Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetMachineType(argument, MachineType::I1);
    return argument;
}

inline GateRef Stub::Int32Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetMachineType(argument, MachineType::I32);
    return argument;
}

inline GateRef Stub::Int64Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetMachineType(argument, MachineType::I64);
    return argument;
}

inline GateRef Stub::TaggedArgument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetGateType(argument, GateType::TaggedValue());
    env_.GetCircuit()->SetMachineType(argument, MachineType::I64);
    return argument;
}

inline GateRef Stub::TaggedPointerArgument(size_t index, GateType type)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetGateType(argument, type);
    env_.GetCircuit()->SetMachineType(argument, MachineType::I64);
    return argument;
}

inline GateRef Stub::PtrArgument(size_t index, GateType type)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetGateType(argument, type);
    if (env_.IsArch64Bit()) {
        env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
        env_.GetCircuit()->SetMachineType(argument, MachineType::I64);
    } else if (env_.IsArch32Bit()) {
        env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
        env_.GetCircuit()->SetMachineType(argument, MachineType::I32);
    } else {
        UNREACHABLE();
    }
    return argument;
}

inline GateRef Stub::Float32Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetMachineType(argument, MachineType::F32);
    return argument;
}

inline GateRef Stub::Float64Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::ARG));
    env_.GetCircuit()->SetMachineType(argument, MachineType::F64);
    return argument;
}

inline GateRef Stub::Alloca(int size)
{
    return env_.GetBulder()->Alloca(size);
}

inline GateRef Stub::Return(GateRef value)
{
    auto control = env_.GetCurrentLabel()->GetControl();
    auto depend = env_.GetCurrentLabel()->GetDepend();
    return env_.GetBulder()->Return(control, depend, value);
}

inline GateRef Stub::Return()
{
    auto control = env_.GetCurrentLabel()->GetControl();
    auto depend = env_.GetCurrentLabel()->GetDepend();
    return env_.GetBulder()->ReturnVoid(control, depend);
}

inline void Stub::Bind(Label *label)
{
    label->Bind();
    env_.SetCurrentLabel(label);
}

inline GateRef Stub::CallRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    SavePcIfNeeded(glue);
    GateRef result = env_.GetBulder()->CallRuntime(glue, index, Gate::InvalidGateRef, args);
    return result;
}

inline GateRef Stub::CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv)
{
    SavePcIfNeeded(glue);
    GateRef result = env_.GetBulder()->CallRuntimeVarargs(glue, index, argc, argv);
    return result;
}

inline GateRef Stub::CallNGCRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    GateRef result = env_.GetBulder()->CallNGCRuntime(glue, index, Gate::InvalidGateRef, args);
    return result;
}

inline GateRef Stub::UpdateLeaveFrameAndCallNGCRuntime(GateRef glue, int index,
    const std::initializer_list<GateRef>& args)
{
    if (env_.IsAsmInterp()) {
        // CpuProfiler will get the latest leaveFrame_ in thread to up frames.
        // So it's necessary to update leaveFrame_ if the program enters the c++ environment.
        // We use the latest asm interpreter frame to update it when CallNGCRuntime.
        GateRef sp = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::SP));
        GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(env_.Is32Bit()));
        Store(VariableType::NATIVE_POINTER(), glue, glue, spOffset, sp);
    }
    GateRef result = CallNGCRuntime(glue, index, args);
    return result;
}

inline GateRef Stub::CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    SavePcIfNeeded(glue);
    GateRef result = GetBuilder()->CallStub(glue, index, args);
    return result;
}

inline void Stub::DebugPrint(GateRef glue, std::initializer_list<GateRef> args)
{
    UpdateLeaveFrameAndCallNGCRuntime(glue, RTSTUB_ID(DebugPrint), args);
}

inline void Stub::FatalPrint(GateRef glue, std::initializer_list<GateRef> args)
{
    UpdateLeaveFrameAndCallNGCRuntime(glue, RTSTUB_ID(FatalPrint), args);
}

void Stub::SavePcIfNeeded(GateRef glue)
{
    if (env_.IsAsmInterp()) {
        GateRef sp = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::SP));
        GateRef pc = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::PC));
        GateRef frame = PtrSub(sp,
            IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
        Store(VariableType::INT64(), glue, frame,
            IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())), pc);
    }
}

// memory
inline GateRef Stub::Load(VariableType type, GateRef base, GateRef offset)
{
    auto depend = env_.GetCurrentLabel()->GetDepend();
    if (env_.IsArch64Bit()) {
        GateRef val = Int64Add(base, offset);
        if (type == VariableType::NATIVE_POINTER()) {
            type = VariableType::INT64();
        }
        GateRef result = env_.GetCircuit()->NewGate(OpCode(OpCode::LOAD), type.GetMachineType(),
            0, { depend, val }, type.GetGateType());
        env_.GetCurrentLabel()->SetDepend(result);
        return result;
    }
    if (env_.IsArch32Bit()) {
        GateRef val = Int32Add(base, offset);
        if (type == VariableType::NATIVE_POINTER()) {
            type = VariableType::INT32();
        }
        GateRef result = env_.GetCircuit()->NewGate(OpCode(OpCode::LOAD), type.GetMachineType(),
            0, { depend, val }, type.GetGateType());
        env_.GetCurrentLabel()->SetDepend(result);
        return result;
    }
    UNREACHABLE();
}

inline GateRef Stub::Load(VariableType type, GateRef base)
{
    if (type == VariableType::NATIVE_POINTER()) {
        if (env_.IsArch64Bit()) {
            type = VariableType::INT64();
        } else {
            type = VariableType::INT32();
        }
    }
    auto depend = env_.GetCurrentLabel()->GetDepend();
    GateRef result = env_.GetCircuit()->NewGate(OpCode(OpCode::LOAD), type.GetMachineType(),
        0, { depend, base }, type.GetGateType());
    env_.GetCurrentLabel()->SetDepend(result);
    return result;
}

// arithmetic
inline GateRef Stub::Int16Add(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::ADD), MachineType::I16, x, y);
}

inline GateRef Stub::Int32Add(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::ADD), MachineType::I32, x, y);
}

inline GateRef Stub::Int64Add(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::ADD), MachineType::I64, x, y);
}

inline GateRef Stub::DoubleAdd(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::ADD), MachineType::F64, x, y);
}

inline GateRef Stub::PtrAdd(GateRef x, GateRef y)
{
    if (env_.Is32Bit()) {
        return Int32Add(x, y);
    }
    return Int64Add(x, y);
}

inline GateRef Stub::IntPtrAnd(GateRef x, GateRef y)
{
    return env_.Is32Bit() ? Int32And(x, y) : Int64And(x, y);
}

inline GateRef Stub::IntPtrEqual(GateRef x, GateRef y)
{
    if (env_.Is32Bit()) {
        return Int32Equal(x, y);
    }
    return Int64Equal(x, y);
}

inline GateRef Stub::PtrSub(GateRef x, GateRef y)
{
    if (env_.Is32Bit()) {
        return Int32Sub(x, y);
    }
    return Int64Sub(x, y);
}

inline GateRef Stub::PointerSub(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SUB), MachineType::ARCH, x, y);
}

inline GateRef Stub::Int16Sub(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SUB), MachineType::I16, x, y);
}

inline GateRef Stub::Int32Sub(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SUB), MachineType::I32, x, y);
}

inline GateRef Stub::Int64Sub(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SUB), MachineType::I64, x, y);
}

inline GateRef Stub::DoubleSub(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SUB), MachineType::F64, x, y);
}

inline GateRef Stub::Int32Mul(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::MUL), MachineType::I32, x, y);
}

inline GateRef Stub::Int64Mul(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::MUL), MachineType::I64, x, y);
}

inline GateRef Stub::DoubleMul(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::MUL), MachineType::F64, x, y);
}

inline GateRef Stub::DoubleDiv(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::FDIV), MachineType::F64, x, y);
}

inline GateRef Stub::Int32Div(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SDIV), MachineType::I32, x, y);
}

inline GateRef Stub::Int64Div(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SDIV), MachineType::I64, x, y);
}

inline GateRef Stub::IntPtrDiv(GateRef x, GateRef y)
{
    return env_.Is32Bit() ? Int32Div(x, y) : Int64Div(x, y);
}

inline GateRef Stub::Int32Mod(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SMOD), MachineType::I32, x, y);
}

inline GateRef Stub::DoubleMod(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::SMOD), MachineType::F64, x, y);
}

// bit operation
inline GateRef Stub::Int32Or(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::OR), MachineType::I32, x, y);
}

inline GateRef Stub::Int8And(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::AND), MachineType::I8, x, y);
}

inline GateRef Stub::Int32And(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::AND), MachineType::I32, x, y);
}

inline GateRef Stub::BoolAnd(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::AND), MachineType::I1, x, y);
}

inline GateRef Stub::BoolOr(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::OR), MachineType::I1, x, y);
}

inline GateRef Stub::Int32Not(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::REV), MachineType::I32, x);
}

inline GateRef Stub::BoolNot(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::REV), MachineType::I1, x);
}

inline GateRef Stub::Int64Or(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::OR), MachineType::I64, x, y);
}

inline GateRef Stub::IntPtrOr(GateRef x, GateRef y)
{
    auto ptrsize = env_.Is32Bit() ? MachineType::I32 : MachineType::I64;
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::OR), ptrsize, x, y);
}

inline GateRef Stub::Int64And(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::AND), MachineType::I64, x, y);
}

inline GateRef Stub::Int16LSL(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSL), MachineType::I16, x, y);
}

inline GateRef Stub::Int64Xor(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::XOR), MachineType::I64, x, y);
}

inline GateRef Stub::Int32Xor(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::XOR), MachineType::I32, x, y);
}

inline GateRef Stub::Int8LSR(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSR), MachineType::I8, x, y);
}

inline GateRef Stub::Int64Not(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::REV), MachineType::I64, x);
}

inline GateRef Stub::Int32LSL(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSL), MachineType::I32, x, y);
}

inline GateRef Stub::Int64LSL(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSL), MachineType::I64, x, y);
}

inline GateRef Stub::IntPtrLSL(GateRef x, GateRef y)
{
    auto ptrSize = env_.Is32Bit() ? MachineType::I32 : MachineType::I64;
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSL), ptrSize, x, y);
}

inline GateRef Stub::Int32ASR(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::ASR), MachineType::I32, x, y);
}

inline GateRef Stub::Int32LSR(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSR), MachineType::I32, x, y);
}

inline GateRef Stub::Int64LSR(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSR), MachineType::I64, x, y);
}

inline GateRef Stub::IntPtrLSR(GateRef x, GateRef y)
{
    auto ptrSize = env_.Is32Bit() ? MachineType::I32 : MachineType::I64;
    return env_.GetBulder()->BinaryArithmetic(OpCode(OpCode::LSR), ptrSize, x, y);
}

template<OpCode::Op Op, MachineType Type>
inline GateRef Stub::BinaryOp(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryArithmetic(OpCode(Op), Type, x, y);
}

inline GateRef Stub::TaggedIsInt(GateRef x)
{
    return Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                      Int64(JSTaggedValue::TAG_INT));
}

inline GateRef Stub::TaggedIsDouble(GateRef x)
{
    return BoolAnd(TaggedIsNumber(x), BoolNot(TaggedIsInt(x)));
}

inline GateRef Stub::TaggedIsObject(GateRef x)
{
    return Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                      Int64(JSTaggedValue::TAG_OBJECT));
}

inline GateRef Stub::TaggedIsNumber(GateRef x)
{
    return BoolNot(TaggedIsObject(x));
}

inline GateRef Stub::TaggedIsHole(GateRef x)
{
    return Int64Equal(x, Int64(JSTaggedValue::VALUE_HOLE));
}

inline GateRef Stub::TaggedIsNotHole(GateRef x)
{
    return Int64NotEqual(x, Int64(JSTaggedValue::VALUE_HOLE));
}

inline GateRef Stub::TaggedIsUndefined(GateRef x)
{
    return Int64Equal(x, Int64(JSTaggedValue::VALUE_UNDEFINED));
}

inline GateRef Stub::TaggedIsException(GateRef x)
{
    return Int64Equal(x, Int64(JSTaggedValue::VALUE_EXCEPTION));
}

inline GateRef Stub::TaggedIsSpecial(GateRef x)
{
    return BoolOr(Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_SPECIAL_MARK)),
        Int64(JSTaggedValue::TAG_SPECIAL)), TaggedIsHole(x));
}

inline GateRef Stub::TaggedIsHeapObject(GateRef x)
{
    return Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_HEAPOBJECT_MARK)), Int64(0));
}

inline GateRef Stub::TaggedIsGeneratorObject(GateRef x)
{
    GateRef isHeapObj = SExtInt1ToInt32(TaggedIsHeapObject(x));
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isGeneratorObj = Int32Or(
        SExtInt1ToInt32(Int32Equal(objType, Int32(static_cast<int32_t>(JSType::JS_GENERATOR_OBJECT)))),
        SExtInt1ToInt32(Int32Equal(objType, Int32(static_cast<int32_t>(JSType::JS_ASYNC_FUNC_OBJECT)))));
    return TruncInt32ToInt1(Int32And(isHeapObj, isGeneratorObj));
}

inline GateRef Stub::TaggedIsPropertyBox(GateRef x)
{
    return TruncInt32ToInt1(
        Int32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
                 SExtInt1ToInt32(HclassIsPropertyBox(LoadHClass(x)))));
}

inline GateRef Stub::TaggedIsWeak(GateRef x)
{
    return Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_WEAK_MARK)), Int64(JSTaggedValue::TAG_WEAK));
}

inline GateRef Stub::TaggedIsPrototypeHandler(GateRef x)
{
    return HclassIsPrototypeHandler(LoadHClass(x));
}

inline GateRef Stub::TaggedIsTransitionHandler(GateRef x)
{
    return TruncInt32ToInt1(
        Int32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
                 SExtInt1ToInt32(HclassIsTransitionHandler(LoadHClass(x)))));
}

inline GateRef Stub::GetNextPositionForHash(GateRef last, GateRef count, GateRef size)
{
    auto nextOffset = Int32LSR(Int32Mul(count, Int32Add(count, Int32(1))),
                               Int32(1));
    return Int32And(Int32Add(last, nextOffset), Int32Sub(size, Int32(1)));
}

inline GateRef Stub::DoubleIsNAN(GateRef x)
{
    GateRef diff = DoubleEqual(x, x);
    return Int32Equal(SExtInt1ToInt32(diff), Int32(0));
}

inline GateRef Stub::DoubleIsINF(GateRef x)
{
    GateRef infinity = Double(base::POSITIVE_INFINITY);
    GateRef negativeInfinity = Double(-base::POSITIVE_INFINITY);
    GateRef diff1 = DoubleEqual(x, infinity);
    GateRef diff2 = DoubleEqual(x, negativeInfinity);
    return TruncInt32ToInt1(Int32Or(Int32Equal(SExtInt1ToInt32(diff1), Int32(1)),
        Int32Equal(SExtInt1ToInt32(diff2), Int32(1))));
}

inline GateRef Stub::TaggedIsNull(GateRef x)
{
    return Int64Equal(x, Int64(JSTaggedValue::VALUE_NULL));
}

inline GateRef Stub::TaggedIsUndefinedOrNull(GateRef x)
{
    return Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_HEAPOBJECT_MARK)),
        Int64(JSTaggedValue::TAG_SPECIAL));
}

inline GateRef Stub::TaggedIsTrue(GateRef x)
{
    return Int64Equal(x, Int64(JSTaggedValue::VALUE_TRUE));
}

inline GateRef Stub::TaggedIsFalse(GateRef x)
{
    return Int64Equal(x, Int64(JSTaggedValue::VALUE_FALSE));
}

inline GateRef Stub::TaggedIsBoolean(GateRef x)
{
    return Int64Equal(Int64And(x, Int64(JSTaggedValue::TAG_HEAPOBJECT_MARK)),
        Int64(JSTaggedValue::TAG_BOOLEAN_MARK));
}

inline GateRef Stub::TaggedGetInt(GateRef x)
{
    return TruncInt64ToInt32(Int64And(x, Int64(~JSTaggedValue::TAG_MARK)));
}

inline GateRef Stub::Int8ToTaggedTypeNGC(GateRef x)
{
    GateRef val = SExtInt8ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

inline GateRef Stub::Int16ToTaggedNGC(GateRef x)
{
    GateRef val = SExtInt16ToInt64(x);
    return ChangeInt64ToTagged(Int64Or(val, Int64(JSTaggedValue::TAG_INT)));
}

inline GateRef Stub::Int16ToTaggedTypeNGC(GateRef x)
{
    GateRef val = SExtInt16ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

inline GateRef Stub::IntToTaggedNGC(GateRef x)
{
    GateRef val = SExtInt32ToInt64(x);
    return ChangeInt64ToTagged(Int64Or(val, Int64(JSTaggedValue::TAG_INT)));
}

inline GateRef Stub::IntToTaggedTypeNGC(GateRef x)
{
    GateRef val = SExtInt32ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

inline GateRef Stub::DoubleBuildTaggedWithNoGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return ChangeInt64ToTagged(Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET)));
}

inline GateRef Stub::DoubleBuildTaggedTypeWithNoGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

inline GateRef Stub::CastDoubleToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::BITCAST), MachineType::I64, x);
}

inline GateRef Stub::TaggedTrue()
{
    return Int64(JSTaggedValue::VALUE_TRUE);
}

inline GateRef Stub::TaggedFalse()
{
    return Int64(JSTaggedValue::VALUE_FALSE);
}

// compare operation
inline GateRef Stub::Int8Equal(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::EQ), x, y);
}

inline GateRef Stub::Int32Equal(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::EQ), x, y);
}

inline GateRef Stub::Int32NotEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::NE), x, y);
}

inline GateRef Stub::Int64Equal(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::EQ), x, y);
}

inline GateRef Stub::DoubleEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::EQ), x, y);
}

inline GateRef Stub::DoubleLessThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SLT), x, y);
}

inline GateRef Stub::DoubleLessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SLE), x, y);
}

inline GateRef Stub::DoubleGreaterThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SGT), x, y);
}

inline GateRef Stub::DoubleGreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SGE), x, y);
}

inline GateRef Stub::Int64NotEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::NE), x, y);
}

inline GateRef Stub::Int32GreaterThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SGT), x, y);
}

inline GateRef Stub::Int32LessThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SLT), x, y);
}

inline GateRef Stub::Int32GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SGE), x, y);
}

inline GateRef Stub::Int32LessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SLE), x, y);
}

inline GateRef Stub::Int32UnsignedGreaterThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::UGT), x, y);
}

inline GateRef Stub::Int32UnsignedLessThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::ULT), x, y);
}

inline GateRef Stub::Int32UnsignedGreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::UGE), x, y);
}

inline GateRef Stub::Int64GreaterThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SGT), x, y);
}

inline GateRef Stub::Int64LessThan(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SLT), x, y);
}

inline GateRef Stub::Int64LessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SLE), x, y);
}

inline GateRef Stub::Int64GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::SGE), x, y);
}

inline GateRef Stub::Int64UnsignedLessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetBulder()->BinaryLogic(OpCode(OpCode::ULE), x, y);
}

inline GateRef Stub::IntPtrGreaterThan(GateRef x, GateRef y)
{
    return env_.Is32Bit() ? Int32GreaterThan(x, y) : Int64GreaterThan(x, y);
}

// cast operation
inline GateRef Stub::ChangeInt64ToInt32(GateRef val)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::TRUNC_TO_INT32), val);
}

inline GateRef Stub::ChangeInt64ToIntPtr(GateRef val)
{
    if (env_.IsArch32Bit()) {
        return ChangeInt64ToInt32(val);
    }
    return val;
}

inline GateRef Stub::ChangeInt32ToIntPtr(GateRef val)
{
    if (env_.IsArch32Bit()) {
        return val;
    }
    return ZExtInt32ToInt64(val);
}

inline GateRef Stub::ChangeIntPtrToInt32(GateRef val)
{
    if (env_.IsArch32Bit()) {
        return val;
    }
    return ChangeInt64ToInt32(val);
}

inline GateRef Stub::GetSetterFromAccessor(GateRef accessor)
{
    GateRef setterOffset = IntPtr(AccessorData::SETTER_OFFSET);
    return Load(VariableType::JS_ANY(), accessor, setterOffset);
}

inline GateRef Stub::GetElementsArray(GateRef object)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    return Load(VariableType::JS_POINTER(), object, elementsOffset);
}

inline void Stub::SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    Store(type, glue, object, elementsOffset, elementsArray);
}

inline GateRef Stub::GetPropertiesArray(GateRef object)
{
    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    return Load(VariableType::JS_POINTER(), object, propertiesOffset);
}

// SetProperties in js_object.h
inline void Stub::SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray)
{
    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    Store(type, glue, object, propertiesOffset, propsArray);
}

inline void Stub::SetHash(GateRef glue, GateRef object, GateRef hash)
{
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    Store(VariableType::INT64(), glue, object, hashOffset, hash);
}

inline GateRef Stub::GetLengthOfTaggedArray(GateRef array)
{
    return Load(VariableType::INT32(), array, IntPtr(TaggedArray::LENGTH_OFFSET));
}

inline GateRef Stub::IsJSHClass(GateRef obj)
{
    return Int32Equal(GetObjectType(LoadHClass(obj)),  Int32(static_cast<int32_t>(JSType::HCLASS)));
}
// object operation
inline GateRef Stub::LoadHClass(GateRef object)
{
    return Load(VariableType::JS_POINTER(), object);
}

inline void Stub::StoreHClass(GateRef glue, GateRef object, GateRef hclass)
{
    Store(VariableType::JS_POINTER(), glue, object, IntPtr(0), hclass);
}

inline GateRef Stub::GetObjectType(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return Int32And(bitfield, Int32((1LU << JSHClass::ObjectTypeBits::SIZE) - 1));
}

inline GateRef Stub::IsDictionaryMode(GateRef object)
{
    GateRef objectType = GetObjectType(LoadHClass(object));
    return Int32Equal(objectType,
        Int32(static_cast<int32_t>(JSType::TAGGED_DICTIONARY)));
}

inline GateRef Stub::IsDictionaryModeByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return Int32NotEqual(
        Int32And(
            Int32LSR(bitfield, Int32(JSHClass::IsDictionaryBit::START_BIT)),
            Int32((1LU << JSHClass::IsDictionaryBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsDictionaryElement(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return Int32NotEqual(
        Int32And(
            Int32LSR(bitfield, Int32(JSHClass::DictionaryElementBits::START_BIT)),
            Int32((1LU << JSHClass::DictionaryElementBits::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsClassConstructorFromBitField(GateRef bitfield)
{
    // decode
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ClassConstructorBit::START_BIT)),
                 Int32((1LU << JSHClass::ClassConstructorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsClassConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);

    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return IsClassConstructorFromBitField(bitfield);
}

inline GateRef Stub::IsClassPrototype(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);

    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ClassPrototypeBit::START_BIT)),
            Int32((1LU << JSHClass::ClassPrototypeBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsExtensible(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);

    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ExtensibleBit::START_BIT)),
                 Int32((1LU << JSHClass::ExtensibleBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::TaggedObjectIsEcmaObject(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    auto ret = Int32And(
        ZExtInt1ToInt32(
            Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_END)))),
        ZExtInt1ToInt32(
            Int32GreaterThanOrEqual(objectType,
                Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_BEGIN)))));
    return TruncInt32ToInt1(ret);
}

inline GateRef Stub::IsJSObject(GateRef obj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objectType = GetObjectType(LoadHClass(obj));
        auto ret1 = Int32And(
            ZExtInt1ToInt32(
                Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT_END)))),
            ZExtInt1ToInt32(
                Int32GreaterThanOrEqual(objectType,
                    Int32(static_cast<int32_t>(JSType::JS_OBJECT_BEGIN)))));
        result = TruncInt32ToInt1(ret1);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

inline GateRef Stub::IsJSFunctionBase(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = ZExtInt1ToInt32(Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_BASE))));
    GateRef less = ZExtInt1ToInt32(Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION))));
    return TruncInt32ToInt1(Int32And(greater, less));
}

inline GateRef Stub::IsConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);

    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ConstructorBit::START_BIT)),
                 Int32((1LU << JSHClass::ConstructorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsBase(GateRef func)
{
    GateRef bitfieldOffset = IntPtr(JSFunction::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), func, bitfieldOffset);
    // decode
    return Int32LessThanOrEqual(
        Int32And(Int32LSR(bitfield, Int32(JSFunction::FunctionKindBits::START_BIT)),
                 Int32((1LU << JSFunction::FunctionKindBits::SIZE) - 1)),
        Int32(static_cast<int32_t>(FunctionKind::CLASS_CONSTRUCTOR)));
}

inline GateRef Stub::IsSymbol(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
}

inline GateRef Stub::IsString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::STRING)));
}

inline GateRef Stub::IsBigInt(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::BIGINT)));
}

inline GateRef Stub::IsJsProxy(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_PROXY)));
}

inline GateRef Stub::IsJsArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

inline GateRef Stub::IsWritable(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            Int32LSR(attr, Int32(PropertyAttributes::WritableField::START_BIT)),
            Int32((1LLU << PropertyAttributes::WritableField::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(PropertyAttributes::IsAccessorField::START_BIT)),
            Int32((1LLU << PropertyAttributes::IsAccessorField::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsInlinedProperty(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(PropertyAttributes::IsInlinedPropsField::START_BIT)),
            Int32((1LLU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::GetProtoCell(GateRef object)
{
    GateRef protoCellOffset = IntPtr(PrototypeHandler::PROTO_CELL_OFFSET);
    return Load(VariableType::INT64(), object, protoCellOffset);
}

inline GateRef Stub::GetPrototypeHandlerHolder(GateRef object)
{
    GateRef holderOffset = IntPtr(PrototypeHandler::HOLDER_OFFSET);
    return Load(VariableType::JS_ANY(), object, holderOffset);
}

inline GateRef Stub::GetPrototypeHandlerHandlerInfo(GateRef object)
{
    GateRef handlerInfoOffset = IntPtr(PrototypeHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), object, handlerInfoOffset);
}

inline GateRef Stub::GetHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), object, bitfieldOffset);
    GateRef mask = Int32(1LLU << (ProtoChangeMarker::HAS_CHANGED_BITS - 1));
    return Int32NotEqual(Int32And(bitfield, mask), Int32(0));
}

inline GateRef Stub::HclassIsPrototypeHandler(GateRef hclass)
{
    return Int32Equal(GetObjectType(hclass),
        Int32(static_cast<int32_t>(JSType::PROTOTYPE_HANDLER)));
}

inline GateRef Stub::HclassIsTransitionHandler(GateRef hclass)
{
    return Int32Equal(GetObjectType(hclass),
        Int32(static_cast<int32_t>(JSType::TRANSITION_HANDLER)));
}

inline GateRef Stub::HclassIsPropertyBox(GateRef hclass)
{
    return Int32Equal(GetObjectType(hclass),
        Int32(static_cast<int32_t>(JSType::PROPERTY_BOX)));
}

inline GateRef Stub::IsField(GateRef attr)
{
    return Int32Equal(
        Int32And(
            Int32LSR(attr, Int32(HandlerBase::KindBit::START_BIT)),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::FIELD));
}

inline GateRef Stub::IsNonExist(GateRef attr)
{
    return Int32Equal(
        Int32And(
            Int32LSR(attr, Int32(HandlerBase::KindBit::START_BIT)),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::NON_EXIST));
}

inline GateRef Stub::HandlerBaseIsAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::AccessorBit::START_BIT)),
            Int32((1LLU << HandlerBase::AccessorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::HandlerBaseIsJSArray(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::IsJSArrayBit::START_BIT)),
            Int32((1LLU << HandlerBase::IsJSArrayBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::HandlerBaseIsInlinedProperty(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::InlinedPropsBit::START_BIT)),
            Int32((1LLU << HandlerBase::InlinedPropsBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::HandlerBaseGetOffset(GateRef attr)
{
    return Int32And(Int32LSR(attr,
        Int32(HandlerBase::OffsetBit::START_BIT)),
        Int32((1LLU << HandlerBase::OffsetBit::SIZE) - 1));
}

inline GateRef Stub::IsInternalAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::InternalAccessorBit::START_BIT)),
            Int32((1LLU << HandlerBase::InternalAccessorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsInvalidPropertyBox(GateRef obj)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    GateRef value = Load(VariableType::INT64(), obj, valueOffset);
    return TaggedIsHole(value);
}

inline GateRef Stub::GetValueFromPropertyBox(GateRef obj)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    return Load(VariableType::JS_ANY(), obj, valueOffset);
}

inline void Stub::SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    Store(VariableType::JS_ANY(), glue, obj, valueOffset, value);
}

inline GateRef Stub::GetTransitionFromHClass(GateRef obj)
{
    GateRef transitionHClassOffset = IntPtr(TransitionHandler::TRANSITION_HCLASS_OFFSET);
    return Load(VariableType::JS_POINTER(), obj, transitionHClassOffset);
}

inline GateRef Stub::GetTransitionHandlerInfo(GateRef obj)
{
    GateRef handlerInfoOffset = IntPtr(TransitionHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), obj, handlerInfoOffset);
}

inline GateRef Stub::PropAttrGetOffset(GateRef attr)
{
    return Int32And(
        Int32LSR(attr, Int32(PropertyAttributes::OffsetField::START_BIT)),
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetDictionaryOrder func in property_attribute.h
inline GateRef Stub::SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int32LSL(
        Int32((1LLU << PropertyAttributes::DictionaryOrderField::SIZE) - 1),
        Int32(PropertyAttributes::DictionaryOrderField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(value, Int32(PropertyAttributes::DictionaryOrderField::START_BIT)));
    return newVal;
}

inline GateRef Stub::GetPrototypeFromHClass(GateRef hClass)
{
    GateRef protoOffset = IntPtr(JSHClass::PROTOTYPE_OFFSET);
    return Load(VariableType::JS_ANY(), hClass, protoOffset);
}

inline GateRef Stub::GetLayoutFromHClass(GateRef hClass)
{
    GateRef attrOffset = IntPtr(JSHClass::LAYOUT_OFFSET);
    return Load(VariableType::JS_POINTER(), hClass, attrOffset);
}

inline GateRef Stub::GetBitFieldFromHClass(GateRef hClass)
{
    GateRef offset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    return Load(VariableType::INT32(), hClass, offset);
}

inline GateRef Stub::GetLengthFromString(GateRef value)
{
    GateRef len = Load(VariableType::INT32(), value, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32LSR(len, Int32(2));  // 2 : 2 means len must be right shift 2 bits
}

inline void Stub::SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield)
{
    GateRef offset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    Store(VariableType::INT32(), glue, hClass, offset, bitfield);
}

inline void Stub::SetPrototypeToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef proto)
{
    GateRef offset = IntPtr(JSHClass::PROTOTYPE_OFFSET);
    Store(type, glue, hClass, offset, proto);
}

inline void Stub::SetProtoChangeDetailsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef protoChange)
{
    GateRef offset = IntPtr(JSHClass::PROTO_CHANGE_DETAILS_OFFSET);
    Store(type, glue, hClass, offset, protoChange);
}

inline void Stub::SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr)
{
    GateRef offset = IntPtr(JSHClass::LAYOUT_OFFSET);
    Store(type, glue, hClass, offset, attr);
}

inline void Stub::SetEnumCacheToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef key)
{
    GateRef offset = IntPtr(JSHClass::ENUM_CACHE_OFFSET);
    Store(type, glue, hClass, offset, key);
}

inline void Stub::SetTransitionsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef transition)
{
    GateRef offset = IntPtr(JSHClass::TRANSTIONS_OFFSET);
    Store(type, glue, hClass, offset, transition);
}

inline void Stub::SetIsProtoTypeToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef oldValue = ZExtInt1ToInt32(value);
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    GateRef mask = Int32LSL(
        Int32((1LU << JSHClass::IsPrototypeBit::SIZE) - 1),
        Int32(JSHClass::IsPrototypeBit::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(oldValue, Int32(JSHClass::IsPrototypeBit::START_BIT)));
    SetBitFieldToHClass(glue, hClass, newVal);
}

inline GateRef Stub::IsProtoTypeHClass(GateRef hClass)
{
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    return TruncInt32ToInt1(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::IsPrototypeBit::SIZE) - 1)));
}

inline void Stub::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset, VariableType type)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass,
                            IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(
        Int32Add(inlinedPropsStart, attrOffset), Int32(JSTaggedValue::TaggedTypeSize()));

    // NOTE: need to translate MarkingBarrier
    Store(type, glue, obj, ChangeInt32ToIntPtr(propOffset), value);
}

inline void Stub::IncNumberOfProps(GateRef glue, GateRef hClass)
{
    GateRef propNums = GetNumberOfPropsFromHClass(hClass);
    SetNumberOfPropsToHClass(glue, hClass, Int32Add(propNums, Int32(1)));
}

inline GateRef Stub::GetNumberOfPropsFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::NumberOfPropsBits::START_BIT)),
        Int32((1LLU << JSHClass::NumberOfPropsBits::SIZE) - 1));
}

inline void Stub::SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield1 = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef oldWithMask = Int32And(bitfield1,
        Int32(~static_cast<uint32_t>(JSHClass::NumberOfPropsBits::Mask())));
    GateRef newValue = Int32LSR(value, Int32(JSHClass::NumberOfPropsBits::START_BIT));
    Store(VariableType::INT32(), glue, hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET),
        Int32Or(oldWithMask, newValue));
}

inline GateRef Stub::GetInlinedPropertiesFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef objectSizeInWords = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ObjectSizeInWordsBits::START_BIT)),
        Int32((1LU << JSHClass::ObjectSizeInWordsBits::SIZE) - 1));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    return Int32Sub(objectSizeInWords, inlinedPropsStart);
}

inline GateRef Stub::GetObjectSizeFromHClass(GateRef hClass) // NOTE: check for special case of string and TAGGED_ARRAY
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef objectSizeInWords = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ObjectSizeInWordsBits::START_BIT)),
        Int32((1LU << JSHClass::ObjectSizeInWordsBits::SIZE) - 1));
    return PtrMul(ChangeInt32ToIntPtr(objectSizeInWords),
        IntPtr(JSTaggedValue::TaggedTypeSize()));
}

inline GateRef Stub::GetInlinedPropsStartFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
}

inline void Stub::SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val)
{
    // NOTE: need to translate MarkingBarrier
    GateRef offset =
        PtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(valType, glue, array, dataOffset, val);
}

inline GateRef Stub::GetValueFromTaggedArray(VariableType returnType, GateRef array, GateRef index)
{
    GateRef offset =
        PtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(returnType, array, dataOffset);
}

inline GateRef Stub::IsSpecialIndexedObj(GateRef jsType)
{
    return Int32GreaterThan(jsType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

inline GateRef Stub::IsSpecialContainer(GateRef jsType)
{
    // arraylist and vector has fast pass now
    return TruncInt32ToInt1(Int32And(
        ZExtInt1ToInt32(
            Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_API_ARRAY_LIST)))),
        ZExtInt1ToInt32(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_API_VECTOR))))));
}

inline GateRef Stub::IsFastTypeArray(GateRef jsType)
{
    return TruncInt32ToInt1(Int32And(
        ZExtInt1ToInt32(
            Int32GreaterThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_BEGIN)))),
        ZExtInt1ToInt32(Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_FLOAT64_ARRAY))))));
}

inline GateRef Stub::IsAccessorInternal(GateRef value)
{
    return Int32Equal(GetObjectType(LoadHClass(value)),
                      Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR)));
}

inline GateRef Stub::GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry)
{
    GateRef index = Int32Add(Int32Add(Int32(LayoutInfo::ELEMENTS_START_INDEX),
        Int32LSL(entry, Int32(1))), Int32(1));
    return GetValueFromTaggedArray(VariableType::INT64(), layout, index);
}

inline GateRef Stub::GetPropertyMetaDataFromAttr(GateRef attr)
{
    return Int32And(Int32LSR(attr, Int32(PropertyAttributes::PropertyMetaDataField::START_BIT)),
        Int32((1LLU << PropertyAttributes::PropertyMetaDataField::SIZE) - 1));
}

inline GateRef Stub::GetKeyFromLayoutInfo(GateRef layout, GateRef entry)
{
    GateRef index = Int32Add(
        Int32(LayoutInfo::ELEMENTS_START_INDEX),
        Int32LSL(entry, Int32(1)));
    return GetValueFromTaggedArray(VariableType::JS_ANY(), layout, index);
}

inline GateRef Stub::GetPropertiesAddrFromLayoutInfo(GateRef layout)
{
    GateRef eleStartIdx = PtrMul(IntPtr(LayoutInfo::ELEMENTS_START_INDEX),
        IntPtr(JSTaggedValue::TaggedTypeSize()));
    return PtrAdd(layout, PtrAdd(IntPtr(TaggedArray::DATA_OFFSET), eleStartIdx));
}

inline GateRef Stub::TaggedCastToInt64(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    return Int64And(tagged, Int64(~JSTaggedValue::TAG_MARK));
}

inline GateRef Stub::TaggedCastToInt32(GateRef x)
{
    return ChangeInt64ToInt32(TaggedCastToInt64(x));
}

inline GateRef Stub::TaggedCastToIntPtr(GateRef x)
{
    return env_.Is32Bit() ? ChangeInt64ToInt32(TaggedCastToInt64(x)) : TaggedCastToInt64(x);
}

inline GateRef Stub::TaggedCastToDouble(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    GateRef val = Int64Sub(tagged, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    return CastInt64ToFloat64(val);
}

inline GateRef Stub::TaggedCastToWeakReferentUnChecked(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return Int64And(x, Int64(~JSTaggedValue::TAG_WEAK));
}

inline GateRef Stub::ChangeInt32ToFloat64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SIGNED_INT_TO_FLOAT), MachineType::F64, x);
}

inline GateRef Stub::ChangeUInt32ToFloat64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::UNSIGNED_INT_TO_FLOAT), MachineType::F64, x);
}

inline GateRef Stub::ChangeFloat64ToInt32(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::FLOAT_TO_SIGNED_INT), MachineType::I32, x);
}

inline GateRef Stub::ChangeTaggedPointerToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::TAGGED_TO_INT64), x);
}

inline GateRef Stub::ChangeInt64ToTagged(GateRef x)
{
    return env_.GetBulder()->TaggedNumber(OpCode(OpCode::INT64_TO_TAGGED), x);
}

inline GateRef Stub::CastInt64ToFloat64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::BITCAST), MachineType::F64, x);
}

inline GateRef Stub::SExtInt32ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT64), x);
}

inline GateRef Stub::SExtInt16ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT64), x);
}

inline GateRef Stub::SExtInt8ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT64), x);
}

inline GateRef Stub::SExtInt1ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT64), x);
}

inline GateRef Stub::SExtInt1ToInt32(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT32), x);
}

inline GateRef Stub::ZExtInt8ToInt16(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT16), x);
}

inline GateRef Stub::ZExtInt32ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT64), x);
}

inline GateRef Stub::ZExtInt1ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT64), x);
}

inline GateRef Stub::ZExtInt1ToInt32(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT32), x);
}

inline GateRef Stub::ZExtInt8ToInt32(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT32), x);
}

inline GateRef Stub::ZExtInt8ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT64), x);
}

inline GateRef Stub::ZExtInt8ToPtr(GateRef x)
{
    if (env_.IsArch32Bit()) {
        return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT32), x);
    }
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT64), x);
}

inline GateRef Stub::ZExtInt16ToPtr(GateRef x)
{
    if (env_.IsArch32Bit()) {
        return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT32), x);
    }
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT64), x);
}

inline GateRef Stub::SExtInt32ToPtr(GateRef x)
{
    if (env_.IsArch32Bit()) {
        return x;
    }
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT64), x);
}

inline GateRef Stub::ZExtInt16ToInt32(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT32), x);
}

inline GateRef Stub::ZExtInt16ToInt64(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::ZEXT_TO_INT64), x);
}

inline GateRef Stub::TruncInt64ToInt32(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::TRUNC_TO_INT32), x);
}

inline GateRef Stub::TruncPtrToInt32(GateRef x)
{
    if (env_.Is32Bit()) {
        return x;
    }
    return TruncInt64ToInt32(x);
}

inline GateRef Stub::TruncInt64ToInt1(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::TRUNC_TO_INT1), x);
}

inline GateRef Stub::TruncInt32ToInt1(GateRef x)
{
    return env_.GetBulder()->UnaryArithmetic(OpCode(OpCode::TRUNC_TO_INT1), x);
}

inline GateRef Stub::GetGlobalConstantAddr(GateRef index)
{
    return Int64Mul(Int64(sizeof(JSTaggedValue)), index);
}

inline GateRef Stub::GetGlobalConstantString(ConstantIndex index)
{
    if (env_.Is32Bit()) {
        return Int32Mul(Int32(sizeof(JSTaggedValue)), Int32(static_cast<int>(index)));
    } else {
        return Int64Mul(Int64(sizeof(JSTaggedValue)), Int64(static_cast<int>(index)));
    }
}

inline GateRef Stub::IsCallableFromBitField(GateRef bitfield)
{
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::CallableBit::START_BIT)),
            Int32((1LU << JSHClass::CallableBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef Stub::IsCallable(GateRef obj)
{
    GateRef hclass = LoadHClass(obj);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hclass, bitfieldOffset);
    return IsCallableFromBitField(bitfield);
}

// GetOffset func in property_attribute.h
inline GateRef Stub::GetOffsetFieldInPropAttr(GateRef attr)
{
    return Int32And(
        Int32LSR(attr, Int32(PropertyAttributes::OffsetField::START_BIT)),
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetOffset func in property_attribute.h
inline GateRef Stub::SetOffsetFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int32LSL(
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1),
        Int32(PropertyAttributes::OffsetField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(value, Int32(PropertyAttributes::OffsetField::START_BIT)));
    return newVal;
}

// SetIsInlinedProps func in property_attribute.h
inline GateRef Stub::SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int32LSL(
        Int32((1LU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1),
        Int32(PropertyAttributes::IsInlinedPropsField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(value, Int32(PropertyAttributes::IsInlinedPropsField::START_BIT)));
    return newVal;
}

inline void Stub::SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef mask = Int32LSL(
        Int32((1LU << JSHClass::HasConstructorBits::SIZE) - 1),
        Int32(JSHClass::HasConstructorBits::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(value, Int32(JSHClass::HasConstructorBits::START_BIT)));
    Store(VariableType::INT32(), glue, hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET), newVal);
}

inline GateRef Stub::IntPtrEuqal(GateRef x, GateRef y)
{
    return env_.Is32Bit() ? Int32Equal(x, y) : Int64Equal(x, y);
}

inline GateRef Stub::GetBitMask(GateRef bitoffset)
{
    // BIT_PER_WORD_MASK
    GateRef bitPerWordMask = Int32(GCBitset::BIT_PER_WORD_MASK);
    // IndexInWord(bitOffset) = bitOffset & BIT_PER_WORD_MASK
    GateRef indexInWord = Int32And(bitoffset, bitPerWordMask);
    // Mask(indeInWord) = 1 << index
    return Int32LSL(Int32(1), indexInWord);
}

inline GateRef Stub::ObjectAddressToRange(GateRef x)
{
    return IntPtrAnd(TaggedCastToIntPtr(x), IntPtr(~panda::ecmascript::DEFAULT_REGION_MASK));
}

inline GateRef Stub::InYoungGeneration(GateRef region)
{
    auto offset = env_.Is32Bit() ? Region::REGION_FLAG_OFFSET_32 : Region::REGION_FLAG_OFFSET_64;
    GateRef x = Load(VariableType::NATIVE_POINTER(), PtrAdd(IntPtr(offset), region),
        IntPtr(0));
    if (env_.Is32Bit()) {
        return Int32Equal(Int32And(x,
            Int32(RegionSpaceFlag::VALID_SPACE_MASK)), Int32(RegionSpaceFlag::IN_YOUNG_SPACE));
    } else {
        return Int64Equal(Int64And(x,
            Int64(RegionSpaceFlag::VALID_SPACE_MASK)), Int64(RegionSpaceFlag::IN_YOUNG_SPACE));
    }
}

inline GateRef Stub::GetParentEnv(GateRef object)
{
    GateRef index = Int32(LexicalEnv::PARENT_ENV_INDEX);
    return GetValueFromTaggedArray(VariableType::JS_ANY(), object, index);
}

inline GateRef Stub::GetPropertiesFromLexicalEnv(GateRef object, GateRef index)
{
    GateRef valueIndex = Int32Add(index, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    return GetValueFromTaggedArray(VariableType::JS_ANY(), object, valueIndex);
}

inline void Stub::SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value)
{
    GateRef valueIndex = Int32Add(index, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, object, valueIndex, value);
}

inline GateRef Stub::GetFunctionBitFieldFromJSFunction(GateRef object)
{
    GateRef offset = IntPtr(JSFunction::BIT_FIELD_OFFSET);
    return Load(VariableType::INT32(), object, offset);
}

inline GateRef Stub::GetHomeObjectFromJSFunction(GateRef object)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return Load(VariableType::JS_ANY(), object, offset);
}

inline GateRef Stub::GetMethodFromJSFunction(GateRef object)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(methodOffset, VariableType::INT32(), Int32(0));
    Label funcIsJSFunctionBase(env);
    Label funcIsJSProxy(env);
    Label getMethod(env);
    Branch(IsJSFunctionBase(object), &funcIsJSFunctionBase, &funcIsJSProxy);
    Bind(&funcIsJSFunctionBase);
    {
        methodOffset = Int32(JSFunctionBase::METHOD_OFFSET);
        Jump(&getMethod);
    }
    Bind(&funcIsJSProxy);
    {
        methodOffset = Int32(JSProxy::METHOD_OFFSET);
        Jump(&getMethod);
    }
    Bind(&getMethod);
    GateRef method = Load(VariableType::NATIVE_POINTER(), object, ChangeInt32ToIntPtr(*methodOffset));
    env->SubCfgExit();
    return method;
}

inline GateRef Stub::GetCallFieldFromMethod(GateRef method)
{
    GateRef callFieldOffset = IntPtr(JSMethod::GetCallFieldOffset(env_.IsArch32Bit()));
    return Load(VariableType::INT64(), method, callFieldOffset);
}

inline void Stub::SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, object, offset, lexicalEnv);
}

inline GateRef Stub::GetGlobalObject(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetGlobalObjOffset(env_.Is32Bit()));
    return Load(VariableType::JS_ANY(), glue, offset);
}

inline GateRef Stub::GetEntryIndexOfGlobalDictionary(GateRef entry)
{
    return Int32Add(Int32(OrderTaggedHashTable<GlobalDictionary>::TABLE_HEADER_SIZE),
        Int32Mul(entry, Int32(GlobalDictionary::ENTRY_SIZE)));
}

inline GateRef Stub::GetBoxFromGlobalDictionary(GateRef object, GateRef entry)
{
    GateRef index = GetEntryIndexOfGlobalDictionary(entry);
    GateRef offset = PtrAdd(ChangeInt32ToIntPtr(index),
        IntPtr(GlobalDictionary::ENTRY_VALUE_INDEX));
    return Load(VariableType::JS_POINTER(), object, offset);
}

inline GateRef Stub::GetValueFromGlobalDictionary(GateRef object, GateRef entry)
{
    GateRef box = GetBoxFromGlobalDictionary(object, entry);
    return Load(VariableType::JS_ANY(), box, IntPtr(PropertyBox::VALUE_OFFSET));
}

inline GateRef Stub::GetPropertiesFromJSObject(GateRef object)
{
    GateRef offset = IntPtr(JSObject::PROPERTIES_OFFSET);
    return Load(VariableType::JS_ANY(), object, offset);
}

inline GateRef Stub::IsJSFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = ZExtInt1ToInt32(Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_BEGIN))));
    GateRef less = ZExtInt1ToInt32(Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_END))));
    return TruncInt32ToInt1(Int32And(greater, less));
}

inline GateRef Stub::IsBoundFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
}

inline GateRef Stub::IsNativeMethod(GateRef method)
{
    GateRef callFieldOffset = IntPtr(JSMethod::GetCallFieldOffset(env_.Is32Bit()));
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return Int64NotEqual(
        Int64And(
            Int64LSR(callfield, Int32(JSMethod::IsNativeBit::START_BIT)),
            Int64((1LU << JSMethod::IsNativeBit::SIZE) - 1)),
        Int64(0));
}

inline GateRef Stub::HasAotCode(GateRef method)
{
    GateRef callFieldOffset = IntPtr(JSMethod::GetCallFieldOffset(env_.Is32Bit()));
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return Int64NotEqual(
        Int64And(
            Int64LSR(callfield, Int32(JSMethod::IsAotCodeBit::START_BIT)),
            Int64((1LU << JSMethod::IsAotCodeBit::SIZE) - 1)),
        Int64(0));
}

inline GateRef Stub::GetExpectedNumOfArgs(GateRef method)
{
    GateRef callFieldOffset = IntPtr(JSMethod::GetCallFieldOffset(env_.Is32Bit()));
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return TruncInt64ToInt32(Int64And(
        Int64LSR(callfield, Int32(JSMethod::NumArgsBits::START_BIT)),
        Int64((1LU << JSMethod::NumArgsBits::SIZE) - 1)));
}

inline GateRef Stub::GetMethodFromJSProxy(GateRef proxy)
{
    GateRef offset = IntPtr(JSProxy::METHOD_OFFSET);
    return Load(VariableType::JS_ANY(), proxy, offset);
}

inline GateRef Stub::GetHandlerFromJSProxy(GateRef proxy)
{
    GateRef offset = IntPtr(JSProxy::HANDLER_OFFSET);
    return Load(VariableType::JS_ANY(), proxy, offset);
}

inline GateRef Stub::GetTargetFromJSProxy(GateRef proxy)
{
    GateRef offset = IntPtr(JSProxy::TARGET_OFFSET);
    return Load(VariableType::JS_ANY(), proxy, offset);
}

inline GateRef Stub::ComputeTaggedArraySize(GateRef length)
{
    return PtrAdd(IntPtr(TaggedArray::DATA_OFFSET),
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), length));
}
inline GateRef Stub::GetGlobalConstantValue(VariableType type, GateRef glue, ConstantIndex index)
{
    GateRef gConstAddr = PtrAdd(glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_.Is32Bit())));
    auto constantIndex = IntPtr(JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(index));
    return Load(type, gConstAddr, constantIndex);
}

inline GateRef Stub::HasPendingException(GateRef glue)
{
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env_.IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    return Int64NotEqual(exception, Int64(JSTaggedValue::VALUE_HOLE));
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_STUB_INL_H
