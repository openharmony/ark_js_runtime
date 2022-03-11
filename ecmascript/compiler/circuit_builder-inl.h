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
#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_INL_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_INL_H

#include "ecmascript/compiler/circuit_builder.h"

namespace panda::ecmascript::kungfu {
// constant
GateRef CircuitBuilder::GetInt8Constant(int8_t value)
{
    return NewInt8Constant(value);
}

GateRef CircuitBuilder::GetInt16Constant(int16_t value)
{
    return NewInt16Constant(value);
}

GateRef CircuitBuilder::GetInt32Constant(int32_t value)
{
    return NewIntegerConstant(value);
}

GateRef CircuitBuilder::GetInt64Constant(uint64_t value)
{
    return NewInteger64Constant(value);
}

GateRef CircuitBuilder::GetIntPtrConstant(int64_t value)
{
    return NewPtrConstant(value);
};

GateRef CircuitBuilder::GetRelocatableData(uint64_t value)
{
    return NewRelocatableData(value);
}

GateRef CircuitBuilder::TrueConstant()
{
    return TruncInt32ToInt1(GetInt32Constant(1));
}

GateRef CircuitBuilder::FalseConstant()
{
    return TruncInt32ToInt1(GetInt32Constant(0));
}

GateRef CircuitBuilder::GetBooleanConstant(bool value)
{
    return NewBooleanConstant(value);
}

GateRef CircuitBuilder::GetArchRelateConstant(uint64_t value)
{
    return NewPtrConstant(value);
}

GateRef CircuitBuilder::GetDoubleConstant(double value)
{
    return NewDoubleConstant(value);
}

GateRef CircuitBuilder::GetUndefinedConstant(VariableType type)
{
    return UndefineConstant(CircuitBuilder::VariableType2GateType(type));
}

GateRef CircuitBuilder::GetHoleConstant(VariableType type)
{
    return HoleConstant(CircuitBuilder::VariableType2GateType(type));
}

GateRef CircuitBuilder::GetNullConstant(VariableType type)
{
    return NullConstant(CircuitBuilder::VariableType2GateType(type));
}

GateRef CircuitBuilder::GetExceptionConstant(VariableType type)
{
    return ExceptionConstant(CircuitBuilder::VariableType2GateType(type));
}

// call operation
GateRef CircuitBuilder::CallRuntime(const CallSignature *descriptor, GateRef glue, GateRef target,
    std::initializer_list<GateRef> args)
{
    auto label = lm_->GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = NewCallGate(descriptor, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntime(const CallSignature *descriptor, GateRef glue, GateRef target, GateRef depend,
    std::initializer_list<GateRef> args)
{
    auto label = lm_->GetCurrentLabel();
    GateRef result = NewCallGate(descriptor, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntimeTrampoline(GateRef glue, GateRef target,
    std::initializer_list<GateRef> args)
{
    auto label = lm_->GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = NewRuntimeCallGate(glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntimeTrampoline(GateRef glue, GateRef target, GateRef depend,
    std::initializer_list<GateRef> args)
{
    auto label = lm_->GetCurrentLabel();
    GateRef result = NewRuntimeCallGate(glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

// memory
GateRef CircuitBuilder::Load(VariableType type, GateRef base, GateRef offset)
{
    auto label = lm_->GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef val = IntPtrAdd(base, offset);
    GateRef result = NewLoadGate(type, val, depend);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::Load(VariableType type, GateRef base)
{
    auto label = lm_->GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = NewLoadGate(type, base, depend);
    label->SetDepend(result);
    return result;
}
// arithmetic
GateRef CircuitBuilder::Int16Add(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::ADD), MachineType::I16, x, y);
}

GateRef CircuitBuilder::Int32Add(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::ADD), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64Add(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::ADD), MachineType::I64, x, y);
}

GateRef CircuitBuilder::DoubleAdd(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::ADD), MachineType::F64, x, y);
}

GateRef CircuitBuilder::IntPtrAdd(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::ADD), MachineType::ARCH, x, y);
}

GateRef CircuitBuilder::Int16Sub(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SUB), MachineType::I16, x, y);
}

GateRef CircuitBuilder::Int32Sub(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SUB), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64Sub(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SUB), MachineType::I64, x, y);
}

GateRef CircuitBuilder::DoubleSub(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SUB), MachineType::F64, x, y);
}

GateRef CircuitBuilder::IntPtrSub(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SUB), MachineType::ARCH, x, y);
}

GateRef CircuitBuilder::Int32Mul(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::MUL), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64Mul(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::MUL), MachineType::I64, x, y);
}

GateRef CircuitBuilder::DoubleMul(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::MUL), MachineType::F64, x, y);
}

GateRef CircuitBuilder::IntPtrMul(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::MUL), MachineType::ARCH, x, y);
}

GateRef CircuitBuilder::Int32Div(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SDIV), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64Div(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SDIV), MachineType::I64, x, y);
}

GateRef CircuitBuilder::UInt32Div(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::UDIV), MachineType::I32, x, y);
}

GateRef CircuitBuilder::UInt64Div(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::UDIV), MachineType::I64, x, y);
}

GateRef CircuitBuilder::DoubleDiv(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::FDIV), MachineType::F64, x, y);
}

GateRef CircuitBuilder::IntPtrDiv(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SDIV), MachineType::ARCH, x, y);
}

GateRef CircuitBuilder::Int32Mod(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SMOD), MachineType::I32, x, y);
}

GateRef CircuitBuilder::DoubleMod(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::SMOD), MachineType::F64, x, y);
}

// bit operation
GateRef CircuitBuilder::BoolAnd(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::AND), MachineType::I1, x, y);
}

GateRef CircuitBuilder::Int8And(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::AND), MachineType::I8, x, y);
}

GateRef CircuitBuilder::Int32And(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::AND), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64And(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::AND), MachineType::I64, x, y);
}

GateRef CircuitBuilder::IntPtrAnd(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::AND), MachineType::ARCH, x, y);
}

GateRef CircuitBuilder::BoolOr(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::OR), MachineType::I1, x, y);
}

GateRef CircuitBuilder::Int32Or(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::OR), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64Or(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::OR), MachineType::I64, x, y);
}

GateRef CircuitBuilder::IntPtrOr(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::OR), ARCH, x, y);
}

GateRef CircuitBuilder::Int32Xor(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::XOR), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64Xor(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::XOR), MachineType::I64, x, y);
}

GateRef CircuitBuilder::BoolNot(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::REV), MachineType::I1, x);
}

GateRef CircuitBuilder::Int32Not(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::REV), MachineType::I32, x);
}

GateRef CircuitBuilder::Int64Not(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::REV), MachineType::I64, x);
}

GateRef CircuitBuilder::Int16LSL(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSL), MachineType::I16, x, y);
}

GateRef CircuitBuilder::Int32LSL(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSL), MachineType::I32, x, y);
}

GateRef CircuitBuilder::Int64LSL(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSL), MachineType::I64, x, y);
}

GateRef CircuitBuilder::UInt64LSL(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSL), MachineType::I64, x, y);
}

GateRef CircuitBuilder::IntPtrLSL(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSL), MachineType::ARCH, x, y);
}

GateRef CircuitBuilder::Int8LSR(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSR), MachineType::I8, x, y);
}

GateRef CircuitBuilder::UInt32LSR(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSR), MachineType::I32, x, y);
}

GateRef CircuitBuilder::UInt64LSR(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSR), MachineType::I64, x, y);
}

GateRef CircuitBuilder::IntPtrLSR(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::LSR), MachineType::ARCH, x, y);
}

// cast operation
GateRef CircuitBuilder::SExtInt32ToInt64(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::SEXT_TO_INT64), x);
}

GateRef CircuitBuilder::SExtInt1ToInt64(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::SEXT_TO_INT64), x);
}

GateRef CircuitBuilder::SExtInt1ToInt32(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::SEXT_TO_INT32), x);
}

GateRef CircuitBuilder::ZExtInt8ToInt16(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT16), x);
}

GateRef CircuitBuilder::ZExtInt32ToInt64(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT64), x);
}

GateRef CircuitBuilder::ZExtInt1ToInt64(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT64), x);
}

GateRef CircuitBuilder::ZExtInt1ToInt32(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT32), x);
}

GateRef CircuitBuilder::ZExtInt8ToInt32(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT32), x);
}

GateRef CircuitBuilder::ZExtInt8ToInt64(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT64), x);
}

GateRef CircuitBuilder::ZExtInt8ToPtr(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_ARCH), x);
}

GateRef CircuitBuilder::ZExtInt16ToPtr(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_ARCH), x);
}

GateRef CircuitBuilder::ZExtInt32ToPtr(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_ARCH), x);
}

GateRef CircuitBuilder::SExtInt32ToPtr(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::SEXT_TO_ARCH), x);
}

GateRef CircuitBuilder::ZExtInt16ToInt32(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT32), x);
}

GateRef CircuitBuilder::ZExtInt16ToInt64(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::ZEXT_TO_INT64), x);
}

GateRef CircuitBuilder::ChangeInt64ToInt32(GateRef val)
{
    return NewArithmeticGate(OpCode(OpCode::TRUNC_TO_INT32), val);
}

GateRef CircuitBuilder::ChangeInt32ToIntPtr(GateRef val)
{
    return ZExtInt32ToArch(val);
}

GateRef CircuitBuilder::TruncInt64ToInt32(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::TRUNC_TO_INT32), x);
}

GateRef CircuitBuilder::TruncPtrToInt32(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::TRUNC_TO_INT32), x);
}

GateRef CircuitBuilder::TruncInt64ToInt1(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::TRUNC_TO_INT1), x);
}

GateRef CircuitBuilder::TruncInt32ToInt1(GateRef x)
{
    return NewArithmeticGate(OpCode(OpCode::TRUNC_TO_INT1), x);
}

// compare operation
GateRef CircuitBuilder::Int8Equal(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::EQ), x, y);
}

GateRef CircuitBuilder::Int32Equal(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::EQ), x, y);
}

GateRef CircuitBuilder::Int64Equal(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::EQ), x, y);
}

GateRef CircuitBuilder::IntPtrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::EQ), x, y);
}

GateRef CircuitBuilder::DoubleEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::EQ), x, y);
}

GateRef CircuitBuilder::Int32NotEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::NE), x, y);
}

GateRef CircuitBuilder::Int64NotEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::NE), x, y);
}

GateRef CircuitBuilder::DoubleLessThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SLT), x, y);
}

GateRef CircuitBuilder::DoubleLessThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SLE), x, y);
}

GateRef CircuitBuilder::DoubleGreaterThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGT), x, y);
}

GateRef CircuitBuilder::DoubleGreaterThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGE), x, y);
}

GateRef CircuitBuilder::Int32LessThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SLT), x, y);
}

GateRef CircuitBuilder::Int32LessThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SLE), x, y);
}

GateRef CircuitBuilder::Int32GreaterThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGT), x, y);
}

GateRef CircuitBuilder::Int32GreaterThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGE), x, y);
}

GateRef CircuitBuilder::UInt32LessThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::ULT), x, y);
}

GateRef CircuitBuilder::UInt32LessThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::ULE), x, y);
}

GateRef CircuitBuilder::Int64GreaterThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGT), x, y);
}

GateRef CircuitBuilder::UInt32GreaterThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::UGE), x, y);
}

GateRef CircuitBuilder::Int64LessThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SLT), x, y);
}

GateRef CircuitBuilder::Int64LessThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SLE), x, y);
}

GateRef CircuitBuilder::Int64GreaterThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGT), x, y);
}

GateRef CircuitBuilder::Int64GreaterThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::SGE), x, y);
}

GateRef CircuitBuilder::UInt64LessThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::ULT), x, y);
}

GateRef CircuitBuilder::UInt64LessThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::ULE), x, y);
}

GateRef CircuitBuilder::UInt6464GreaterThan(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::UGT), x, y);
}

GateRef CircuitBuilder::UInt6464GreaterThanOrEqual(GateRef x, GateRef y)
{
    return NewLogicGate(OpCode(OpCode::UGE), x, y);
}

//JsCircuitBuilder
GateRef JsCircuitBuilder::TaggedIsInt(GateRef x)
{
    return builder_->Int64Equal(builder_->Int64And(x, builder_->GetInt64Constant(JSTaggedValue::TAG_MASK)),
                                builder_->GetInt64Constant(JSTaggedValue::TAG_INT));
}

GateRef JsCircuitBuilder::TaggedIsDouble(GateRef x)
{
    return builder_->BoolAnd(TaggedIsNumber(x), builder_->BoolNot(TaggedIsInt(x)));
}

GateRef JsCircuitBuilder::TaggedIsObject(GateRef x)
{
    return builder_->Int64Equal(builder_->Int64And(x, builder_->GetInt64Constant(JSTaggedValue::TAG_MASK)),
                                builder_->GetInt64Constant(JSTaggedValue::TAG_OBJECT));
}

GateRef JsCircuitBuilder::TaggedIsNumber(GateRef x)
{
    return builder_->BoolNot(TaggedIsObject(x));
}

GateRef JsCircuitBuilder::TaggedIsHole(GateRef x)
{
    return builder_->Int64Equal(x, builder_->GetInt64Constant(JSTaggedValue::VALUE_HOLE));
}

GateRef JsCircuitBuilder::TaggedIsNotHole(GateRef x)
{
    return builder_->Int64NotEqual(x, builder_->GetInt64Constant(JSTaggedValue::VALUE_HOLE));
}

GateRef JsCircuitBuilder::TaggedIsUndefined(GateRef x)
{
    return Int64Equal(x, GetInt64Constant(JSTaggedValue::VALUE_UNDEFINED));
}

GateRef JsCircuitBuilder::TaggedIsException(GateRef x)
{
    return builder_->Int64Equal(x, builder_->GetInt64Constant(JSTaggedValue::VALUE_EXCEPTION));
}

GateRef JsCircuitBuilder::TaggedIsSpecial(GateRef x)
{
    return builder_->TruncInt32ToInt1(builder_->Int32And(builder_->SExtInt1ToInt32(
        builder_->Int64Equal(builder_->Int64And(x, builder_->GetInt64Constant(~JSTaggedValue::TAG_SPECIAL_MASK)),
        builder_->GetInt64Constant(0))), builder_->Int32Or(builder_->SExtInt1ToInt32(builder_->Int64NotEqual(
        builder_->Int64And(x, builder_->GetInt64Constant(JSTaggedValue::TAG_SPECIAL_VALUE)),
        builder_->GetInt64Constant(0))),builder_->SExtInt1ToInt32(TaggedIsHole(x)))));
}

GateRef JsCircuitBuilder::TaggedIsHeapObject(GateRef x)
{
    return builder_->TruncInt32ToInt1(builder_->Int32And(builder_->SExtInt1ToInt32(TaggedIsObject(x)),
        builder_->SExtInt1ToInt32(builder_->Int32Equal(builder_->SExtInt1ToInt32(TaggedIsSpecial(x)),
        builder_->GetInt32Constant(0)))));
}

GateRef JsCircuitBuilder::TaggedIsGeneratorObject(GateRef x)
{
    GateRef isHeapObj = builder_->SExtInt1ToInt32(TaggedIsHeapObject(x));
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isGeneratorObj = builder_->Int32Or(
        builder_->SExtInt1ToInt32(builder_->Int32Equal(objType,
        builder_->GetInt32Constant(static_cast<int32_t>(JSType::JS_GENERATOR_OBJECT)))),
        builder_->SExtInt1ToInt32(builder_->Int32Equal(objType,
        builder_->GetInt32Constant(static_cast<int32_t>(JSType::JS_ASYNC_FUNC_OBJECT)))));
    return builder_->TruncInt32ToInt1(builder_->Int32And(isHeapObj, isGeneratorObj));
}

GateRef JsCircuitBuilder::TaggedIsPropertyBox(GateRef x)
{
    return builder_->TruncInt32ToInt1(
        builder_->Int32And(builder_->SExtInt1ToInt32(TaggedIsHeapObject(x)),
        builder_->SExtInt1ToInt32(HclassIsPropertyBox(LoadHClass(x)))));
}

GateRef JsCircuitBuilder::TaggedIsWeak(GateRef x)
{
    return builder_->TruncInt32ToInt1(
        builder_->Int32And(builder_->SExtInt1ToInt32(TaggedIsHeapObject(x)),
        builder_->SExtInt1ToInt32(builder_->Int64Equal(builder_->Int64And(x,
        builder_->GetInt64Constant(JSTaggedValue::TAG_WEAK_MASK)),
        builder_->GetInt64Constant(1)))));
}

GateRef JsCircuitBuilder::TaggedIsPrototypeHandler(GateRef x)
{
    return HclassIsPrototypeHandler(LoadHClass(x));
}

GateRef JsCircuitBuilder::TaggedIsTransitionHandler(GateRef x)
{
    return builder_->TruncInt32ToInt1(
        builder_->Int32And(builder_->SExtInt1ToInt32(TaggedIsHeapObject(x)),
        builder_->SExtInt1ToInt32(HclassIsTransitionHandler(LoadHClass(x)))));
}

GateRef JsCircuitBuilder::LoadHClass(GateRef object)
{
    return builder_->Load(VariableType::JS_POINTER(), object);
}

GateRef JsCircuitBuilder::HclassIsPrototypeHandler(GateRef hclass)
{
    return builder_->Int32Equal(GetObjectType(hclass),
        builder_->GetInt32Constant(static_cast<int32_t>(JSType::PROTOTYPE_HANDLER)));
}

GateRef JsCircuitBuilder::HclassIsTransitionHandler(GateRef hclass)
{
    return builder_->Int32Equal(GetObjectType(hclass),
        builder_->GetInt32Constant(static_cast<int32_t>(JSType::TRANSITION_HANDLER)));
}

GateRef JsCircuitBuilder::HclassIsPropertyBox(GateRef hclass)
{
    return builder_->Int32Equal(GetObjectType(hclass),
        builder_->GetInt32Constant(static_cast<int32_t>(JSType::PROPERTY_BOX)));
}

GateRef JsCircuitBuilder::GetObjectType(GateRef hClass)
{
    GateRef bitfieldOffset = builder_->GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = builder_->Load(VariableType::INT32(), hClass, bitfieldOffset);
    return builder_->Int32And(bitfield, builder_->GetInt32Constant((1LU << JSHClass::ObjectTypeBits::SIZE) - 1));
}

GateRef JsCircuitBuilder::IsString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return builder_->Int32Equal(objectType, builder_->GetInt32Constant(static_cast<int32_t>(JSType::STRING)));
}

GateRef JsCircuitBuilder::BothIsString(GateRef x, GateRef y)
{
    return builder_->TruncInt32ToInt1(builder_->Int32And(builder_->SExtInt1ToInt32(IsString(x)),
                                                         builder_->SExtInt1ToInt32(IsString(y))));
}

void Label::Seal()
{
    return impl_->Seal();
}

void Label::WriteVariable(Variable *var, GateRef value)
{
    impl_->WriteVariable(var, value);
}

GateRef Label::ReadVariable(Variable *var)
{
    return impl_->ReadVariable(var);
}

void Label::Bind()
{
    impl_->Bind();
}

void Label::MergeAllControl()
{
    impl_->MergeAllControl();
}

void Label::MergeAllDepend()
{
    impl_->MergeAllDepend();
}

void Label::AppendPredecessor(const Label *predecessor)
{
    impl_->AppendPredecessor(predecessor->GetRawLabel());
}

std::vector<Label> Label::GetPredecessors() const
{
    std::vector<Label> labels;
    for (auto rawlabel : impl_->GetPredecessors()) {
        labels.emplace_back(Label(rawlabel));
    }
    return labels;
}

void Label::SetControl(GateRef control)
{
    impl_->SetControl(control);
}

void Label::SetPreControl(GateRef control)
{
    impl_->SetPreControl(control);
}

void Label::MergeControl(GateRef control)
{
    impl_->MergeControl(control);
}

GateRef Label::GetControl() const
{
    return impl_->GetControl();
}

GateRef Label::GetDepend() const
{
    return impl_->GetDepend();
}

void Label::SetDepend(GateRef depend)
{
    return impl_->SetDepend(depend);
}

GateType LabelManager::GetGateType(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetGateType();
}

Label LabelManager::GetLabelFromSelector(GateRef sel)
{
    Label::LabelImpl *rawlabel = phiToLabels_[sel];
    return Label(rawlabel);
}

void LabelManager::AddSelectorToLabel(GateRef sel, Label label)
{
    phiToLabels_[sel] = label.GetRawLabel();
}

Label::LabelImpl *LabelManager::NewLabel(LabelManager *lm, GateRef control)
{
    auto impl = new Label::LabelImpl(lm, control);
    rawLabels_.emplace_back(impl);
    return impl;
}

void LabelManager::PushCurrentLabel(Label *entry)
{
    if (currentLabel_ != nullptr) {
        GateRef control = currentLabel_->GetControl();
        GateRef depend = currentLabel_->GetDepend();
        stack_.push(currentLabel_);
        currentLabel_ = entry;
        currentLabel_->SetControl(control);
        currentLabel_->SetDepend(depend);
    }
}

void LabelManager::PopCurrentLabel()
{
    GateRef control = currentLabel_->GetControl();
    GateRef depend = currentLabel_->GetDepend();
    if (!stack_.empty()) {
        currentLabel_ = stack_.top();
        currentLabel_->SetControl(control);
        currentLabel_->SetDepend(depend);
        stack_.pop();
    }
}

GateRef LabelManager::GetInput(size_t index) const
{
    return inputList_.at(index);
}

template<bool noThrow>
void LabelManager::MergeMirCircuit(GateRef hir, GateRef outir,
                                   const std::vector<GateRef> &successControl,
                                   const std::vector<GateRef> &exceptionControl)
{
    GateAccessor acc(circuit_);
    acc.SetGateType(outir, acc.GetGateType(hir));

    auto uses = acc.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        // replace HIR:IF_SUCCESS/IF_EXCEPTION with control flow in Label successExit/failExit of MIR Circuit
        if (acc.GetOpCode(*useIt) == OpCode::IF_SUCCESS) {
            acc.ReplaceHirControlGate(useIt, successControl[0]);
        } else if (acc.GetOpCode(*useIt) == OpCode::IF_EXCEPTION) {
            acc.ReplaceHirControlGate<noThrow>(useIt, exceptionControl[0]);
        // change depend flow in catch block from HIR:JS_BYTECODE to depend flow in MIR Circuit
        } else if ((acc.GetOpCode(*useIt) == OpCode::DEPEND_SELECTOR) ||
                   (acc.GetOpCode(*useIt) == OpCode::DEPEND_RELAY)) {
            if (acc.GetOpCode(acc.GetIn(acc.GetIn(*useIt, 0), useIt.GetIndex() - 1)) == OpCode::IF_EXCEPTION) {
                noThrow ? acc.DeleteIn(useIt) : acc.ReplaceIn(useIt, exceptionControl[1]);
            } else {
                acc.ReplaceIn(useIt, successControl[1]);
            }
        // replace normal depend
        } else if ((acc.GetOpCode(*useIt) == OpCode::JS_BYTECODE) && useIt.GetIndex() == 1) {
            acc.ReplaceIn(useIt, successControl[1]);
        // if no catch block, just throw exception(RETURN)
        } else if ((acc.GetOpCode(*useIt) == OpCode::RETURN) &&
                    acc.GetOpCode(acc.GetIn(*useIt, 0)) == OpCode::IF_EXCEPTION) {
            noThrow ? acc.DeleteIn(useIt) : acc.ReplaceIn(useIt, exceptionControl[1]);
        } else if (useIt.GetIndex() == 1) {
            acc.ReplaceIn(useIt, successControl[1]);
        // replace data flow with data output in label successExit(valueSelector...)
        } else {
            acc.ReplaceIn(useIt, outir);
        }
    }

    circuit_->DeleteGate(hir);
}

GateRef LabelManager::Return(GateRef value)
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return builder_.Return(control, depend, value);
}

GateRef LabelManager::Return()
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return builder_.ReturnVoid(control, depend);
}

void LabelManager::Bind(Label *label)
{
    label->Bind();
    SetCurrentLabel(label);
}

void LabelManager::Bind(Label *label, bool justSlowPath)
{
    if (!justSlowPath) {
        label->Bind();
        SetCurrentLabel(label);
    }
}

void LabelManager::HandleException(GateRef result, Label *success, Label *fail, Label *exit, VariableType type)
{
    Branch(builder_.Int64Equal(result, builder_.GetExceptionConstant(type)), success, fail);
    Bind(fail);
    {
        Jump(exit);
    }
}

void LabelManager::HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal)
{
    Branch(builder_.Int64Equal(result, exceptionVal), success, fail);
    Bind(fail);
    {
        Jump(exit);
    }
}

} // namespace panda::ecmascript::kungfu

#endif