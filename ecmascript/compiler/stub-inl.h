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

#include "ecmascript/compiler/stub.h"

namespace panda::ecmascript::kungfu {
using LabelImpl = Stub::Label::LabelImpl;
using JSTaggedValue = JSTaggedValue;
using JSFunction = panda::ecmascript::JSFunction;
using PropertyBox = panda::ecmascript::PropertyBox;

void Stub::Label::Seal()
{
    return impl_->Seal();
}
void Stub::Label::WriteVariable(Stub::Variable *var, GateRef value)
{
    impl_->WriteVariable(var, value);
}
GateRef Stub::Label::ReadVariable(Stub::Variable *var)
{
    return impl_->ReadVariable(var);
}
void Stub::Label::Bind()
{
    impl_->Bind();
}
void Stub::Label::MergeAllControl()
{
    impl_->MergeAllControl();
}
void Stub::Label::MergeAllDepend()
{
    impl_->MergeAllDepend();
}
void Stub::Label::AppendPredecessor(const Stub::Label *predecessor)
{
    impl_->AppendPredecessor(predecessor->GetRawLabel());
}
std::vector<Stub::Label> Stub::Label::GetPredecessors() const
{
    std::vector<Label> labels;
    for (auto rawlabel : impl_->GetPredecessors()) {
        labels.emplace_back(Label(rawlabel));
    }
    return labels;
}
void Stub::Label::SetControl(GateRef control)
{
    impl_->SetControl(control);
}
void Stub::Label::SetPreControl(GateRef control)
{
    impl_->SetPreControl(control);
}
void Stub::Label::MergeControl(GateRef control)
{
    impl_->MergeControl(control);
}
GateRef Stub::Label::GetControl() const
{
    return impl_->GetControl();
}
GateRef Stub::Label::GetDepend() const
{
    return impl_->GetDepend();
}
void Stub::Label::SetDepend(GateRef depend)
{
    return impl_->SetDepend(depend);
}

TypeCode Stub::Environment::GetTypeCode(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetTypeCode();
}

Stub::Label Stub::Environment::GetLabelFromSelector(GateRef sel)
{
    LabelImpl *rawlabel = phiToLabels_[sel];
    return Stub::Label(rawlabel);
}

void Stub::Environment::AddSelectorToLabel(GateRef sel, Label label)
{
    phiToLabels_[sel] = label.GetRawLabel();
}

LabelImpl *Stub::Environment::NewLabel(Stub::Environment *env, GateRef control)
{
    auto impl = new LabelImpl(env, control);
    rawLabels_.emplace_back(impl);
    return impl;
}

void Stub::Environment::PushCurrentLabel(Stub::Label *entry)
{
    GateRef control = currentLabel_->GetControl();
    GateRef depend = currentLabel_->GetDepend();
    if (currentLabel_ != nullptr) {
        stack_.push(currentLabel_);
        currentLabel_ = entry;
        currentLabel_->SetControl(control);
        currentLabel_->SetDepend(depend);
    }
}

void Stub::Environment::PopCurrentLabel()
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

void Stub::Environment::SetFrameType(FrameType type)
{
    circuit_->SetFrameType(type);
}

GateRef Stub::Environment::GetArgument(size_t index) const
{
    return arguments_.at(index);
}

// constant
GateRef Stub::GetInt32Constant(int32_t value)
{
    return env_.GetCircuitBuilder().NewIntegerConstant(value);
};
GateRef Stub::GetWord64Constant(uint64_t value)
{
    return env_.GetCircuitBuilder().NewInteger64Constant(value);
};

GateRef Stub::GetArchRelateConstant(uint64_t value)
{
    if (env_.IsArm32()) {
        return GetInt32Constant(value);
    }
    return GetWord64Constant(value);
};

GateRef Stub::TrueConstant()
{
    return TruncInt32ToInt1(GetInt32Constant(1));
}

GateRef Stub::FalseConstant()
{
    return TruncInt32ToInt1(GetInt32Constant(0));
}

GateRef Stub::GetBooleanConstant(bool value)
{
    return env_.GetCircuitBuilder().NewBooleanConstant(value);
}

GateRef Stub::GetDoubleConstant(double value)
{
    return env_.GetCircuitBuilder().NewDoubleConstant(value);
}

GateRef Stub::GetUndefinedConstant(MachineType type)
{
    return env_.GetCircuitBuilder().UndefineConstant(CircuitBuilder::MachineType2TypeCode(type));
}

GateRef Stub::GetHoleConstant(MachineType type)
{
    return env_.GetCircuitBuilder().HoleConstant(CircuitBuilder::MachineType2TypeCode(type));
}


GateRef Stub::GetNullConstant(MachineType type)
{
    return env_.GetCircuitBuilder().NullConstant(CircuitBuilder::MachineType2TypeCode(type));
}

GateRef Stub::GetExceptionConstant(MachineType type)
{
    return env_.GetCircuitBuilder().ExceptionConstant(CircuitBuilder::MachineType2TypeCode(type));
}

GateRef Stub::ArchRelatePtrMul(GateRef x, GateRef y)
{
    if (env_.IsArm32()) {
        return Int32Mul(x, y);
    } else {
        return Int64Mul(x, y);
    }
}

// parameter
GateRef Stub::Argument(size_t index)
{
    return env_.GetArgument(index);
}

GateRef Stub::Int1Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT1_ARG));
    return argument;
}

GateRef Stub::Int32Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT32_ARG));
    return argument;
}

GateRef Stub::Int64Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT64_ARG));
    return argument;
}

GateRef Stub::TaggedArgument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT64_ARG));
    env_.GetCircuit()->SetTypeCode(argument, TypeCode::JS_ANY);
    return argument;
}

GateRef Stub::TaggedPointerArgument(size_t index, TypeCode type)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT64_ARG));
    env_.GetCircuit()->SetTypeCode(argument, type);
    return argument;
}

GateRef Stub::PtrArgument(size_t index, TypeCode type)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetTypeCode(argument, type);
    if (env_.IsArch64Bit()) {
        env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT64_ARG));
    } else if (env_.IsArch32Bit()) {
        env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::INT32_ARG));
    } else {
        UNREACHABLE();
    }
    return argument;
}

GateRef Stub::Float32Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::FLOAT32_ARG));
    return argument;
}

GateRef Stub::Float64Argument(size_t index)
{
    GateRef argument = Argument(index);
    env_.GetCircuit()->SetOpCode(argument, OpCode(OpCode::FLOAT64_ARG));
    return argument;
}

GateRef Stub::Alloca(int size, TypeCode type)
{
    return env_.GetCircuitBuilder().Alloca(size, type);
}

GateRef Stub::Return(GateRef value)
{
    auto control = env_.GetCurrentLabel()->GetControl();
    auto depend = env_.GetCurrentLabel()->GetDepend();
    return env_.GetCircuitBuilder().Return(control, depend, value);
}

GateRef Stub::Return()
{
    auto control = env_.GetCurrentLabel()->GetControl();
    auto depend = env_.GetCurrentLabel()->GetDepend();
    return env_.GetCircuitBuilder().ReturnVoid(control, depend);
}

void Stub::Bind(Label *label)
{
    label->Bind();
    env_.SetCurrentLabel(label);
}

GateRef Stub::CallStub(StubDescriptor *descriptor,  GateRef glue, GateRef target,
    std::initializer_list<GateRef> args)
{
    auto depend = env_.GetCurrentLabel()->GetDepend();
    GateRef result = env_.GetCircuitBuilder().NewCallGate(descriptor, glue, target, depend, args);
    env_.GetCurrentLabel()->SetDepend(result);
    return result;
}
GateRef Stub::CallStub(StubDescriptor *descriptor,  GateRef glue, GateRef target, GateRef depend,
    std::initializer_list<GateRef> args)
{
    GateRef result = env_.GetCircuitBuilder().NewCallGate(descriptor, glue, target, depend, args);
    env_.GetCurrentLabel()->SetDepend(result);
    return result;
}

GateRef Stub::CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target,
    std::initializer_list<GateRef> args)
{
    auto depend = env_.GetCurrentLabel()->GetDepend();
    GateRef result = env_.GetCircuitBuilder().NewCallGate(descriptor, glue, target, depend, args);
    env_.GetCurrentLabel()->SetDepend(result);
    return result;
}

GateRef Stub::CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target, GateRef depend,
    std::initializer_list<GateRef> args)
{
    GateRef result = env_.GetCircuitBuilder().NewCallGate(descriptor, glue, target, depend, args);
    env_.GetCurrentLabel()->SetDepend(result);
    return result;
}

void Stub::DebugPrint(GateRef glue, std::initializer_list<GateRef> args)
{
    StubDescriptor *debugPrint = GET_STUBDESCRIPTOR(DebugPrint);
    CallRuntime(debugPrint, glue, GetWord64Constant(FAST_STUB_ID(DebugPrint)), args);
}

// memory
GateRef Stub::Load(MachineType type, GateRef base, GateRef offset)
{
    auto depend = env_.GetCurrentLabel()->GetDepend();
    if (env_.IsArch64Bit()) {
        GateRef val = Int64Add(base, offset);
        if (type == MachineType::NATIVE_POINTER) {
            type = MachineType::INT64;
        }
        GateRef result = env_.GetCircuitBuilder().NewLoadGate(type, val, depend);
        env_.GetCurrentLabel()->SetDepend(result);
        return result;
    }
    if (env_.IsArch32Bit()) {
        GateRef val = Int32Add(base, offset);
        if (type == MachineType::NATIVE_POINTER) {
            type = MachineType::INT32;
        }
        GateRef result = env_.GetCircuitBuilder().NewLoadGate(type, val, depend);
        env_.GetCurrentLabel()->SetDepend(result);
        return result;
    }
    UNREACHABLE();
}

GateRef Stub::Load(MachineType type, GateRef base)
{
    if (type == MachineType::NATIVE_POINTER) {
        if (env_.IsArch64Bit()) {
            type = MachineType::INT64;
        } else {
            type = MachineType::INT32;
        }
    }
    auto depend = env_.GetCurrentLabel()->GetDepend();
    GateRef result = env_.GetCircuitBuilder().NewLoadGate(type, base, depend);
    env_.GetCurrentLabel()->SetDepend(result);
    return result;
}

// arithmetic
GateRef Stub::Int32Add(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_ADD), x, y);
}

GateRef Stub::Int64Add(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_ADD), x, y);
}

GateRef Stub::DoubleAdd(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_ADD), x, y);
}

GateRef Stub::ArchRelateAdd(GateRef x, GateRef y)
{
    if (env_.IsArm32()) {
        return Int32Add(x, y);
    }
    return Int64Add(x, y);
}

GateRef Stub::ArchRelateSub(GateRef x, GateRef y)
{
    if (env_.IsArm32()) {
        return Int32Sub(x, y);
    }
    return Int64Sub(x, y);
}

GateRef Stub::PtrAdd(GateRef x, GateRef y)
{
    return ArchRelateAdd(x, y);
}

GateRef Stub::PtrSub(GateRef x, GateRef y)
{
    return ArchRelateSub(x, y);
}

GateRef Stub::Int32Sub(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_SUB), x, y);
}

GateRef Stub::Int64Sub(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_SUB), x, y);
}

GateRef Stub::DoubleSub(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_SUB), x, y);
}

GateRef Stub::Int32Mul(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_MUL), x, y);
}

GateRef Stub::Int64Mul(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_MUL), x, y);
}

GateRef Stub::DoubleMul(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_MUL), x, y);
}

GateRef Stub::DoubleDiv(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_DIV), x, y);
}
GateRef Stub::Int32Div(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_SDIV), x, y);
}

GateRef Stub::Word32Div(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_UDIV), x, y);
}

GateRef Stub::Int32Mod(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_SMOD), x, y);
}

GateRef Stub::DoubleMod(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_SMOD), x, y);
}

// bit operation
GateRef Stub::Word32Or(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_OR), x, y);
}

GateRef Stub::Word32And(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_AND), x, y);
}

GateRef Stub::Word32Not(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_REV), x);
}

GateRef Stub::Word64Or(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_OR), x, y);
}

GateRef Stub::Word64And(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_AND), x, y);
}

GateRef Stub::Word64Xor(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_XOR), x, y);
}

GateRef Stub::Word64Not(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_REV), x);
}

GateRef Stub::Word32LSL(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_LSL), x, y);
}

GateRef Stub::Word64LSL(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_LSL), x, y);
}

GateRef Stub::Word32LSR(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_LSR), x, y);
}

GateRef Stub::Word64LSR(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_LSR), x, y);
}

GateRef Stub::TaggedIsInt(GateRef x)
{
    return Word64Equal(Word64And(x, GetWord64Constant(JSTaggedValue::TAG_MASK)),
                       GetWord64Constant(JSTaggedValue::TAG_INT));
}

GateRef Stub::TaggedIsDouble(GateRef x)
{
    return Word32Equal(Word32Or(SExtInt1ToInt32(TaggedIsInt(x)), SExtInt1ToInt32(TaggedIsObject(x))),
                       GetInt32Constant(0));
}

GateRef Stub::TaggedIsObject(GateRef x)
{
    return Word64Equal(Word64And(x, GetWord64Constant(JSTaggedValue::TAG_MASK)),
                       GetWord64Constant(JSTaggedValue::TAG_OBJECT));
}

GateRef Stub::TaggedIsNumber(GateRef x)
{
    return TruncInt32ToInt1(Word32Or(SExtInt1ToInt32(TaggedIsInt(x)), SExtInt1ToInt32(TaggedIsDouble(x))));
}

GateRef Stub::TaggedIsHole(GateRef x)
{
    return Word64Equal(x, GetWord64Constant(JSTaggedValue::VALUE_HOLE));
}

GateRef Stub::TaggedIsNotHole(GateRef x)
{
    return Word64NotEqual(x, GetWord64Constant(JSTaggedValue::VALUE_HOLE));
}

GateRef Stub::TaggedIsUndefined(GateRef x)
{
    return Word64Equal(x, GetWord64Constant(JSTaggedValue::VALUE_UNDEFINED));
}

GateRef Stub::TaggedIsSpecial(GateRef x)
{
    return TruncInt32ToInt1(Word32And(
        SExtInt1ToInt32(
            Word64Equal(Word64And(x, GetWord64Constant(~JSTaggedValue::TAG_SPECIAL_MASK)), GetWord64Constant(0))),
        Word32Or(SExtInt1ToInt32(
            Word64NotEqual(Word64And(x, GetWord64Constant(JSTaggedValue::TAG_SPECIAL_VALUE)), GetWord64Constant(0))),
            SExtInt1ToInt32(TaggedIsHole(x)))));
}

GateRef Stub::TaggedIsHeapObject(GateRef x)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(TaggedIsObject(x)),
                  SExtInt1ToInt32(Word32Equal(SExtInt1ToInt32(TaggedIsSpecial(x)), GetInt32Constant(0)))));
}

GateRef Stub::TaggedIsPropertyBox(GateRef x)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
                  SExtInt1ToInt32(HclassIsPropertyBox(LoadHClass(x)))));
}

GateRef Stub::TaggedIsWeak(GateRef x)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(TaggedIsHeapObject(x)), SExtInt1ToInt32(
            Word64Equal(Word64And(x, GetWord64Constant(JSTaggedValue::TAG_WEAK_MASK)),
                GetWord64Constant(1)))));
}

GateRef Stub::TaggedIsPrototypeHandler(GateRef x)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
                  SExtInt1ToInt32(HclassIsPrototypeHandler(LoadHClass(x)))));
}

GateRef Stub::TaggedIsTransitionHandler(GateRef x)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
                  SExtInt1ToInt32(HclassIsTransitionHandler(LoadHClass(x)))));
}

GateRef Stub::GetNextPositionForHash(GateRef last, GateRef count, GateRef size)
{
    auto nextOffset = Word32LSR(Int32Mul(count, Int32Add(count, GetInt32Constant(1))),
                                GetInt32Constant(1));
    return Word32And(Int32Add(last, nextOffset), Int32Sub(size, GetInt32Constant(1)));
}

GateRef Stub::DoubleIsNAN(GateRef x)
{
    GateRef diff = DoubleEqual(x, x);
    return Word32Equal(SExtInt1ToInt32(diff), GetInt32Constant(0));
}

GateRef Stub::DoubleIsINF(GateRef x)
{
    GateRef infinity = GetDoubleConstant(base::POSITIVE_INFINITY);
    GateRef negativeInfinity = GetDoubleConstant(-base::POSITIVE_INFINITY);
    GateRef diff1 = DoubleEqual(x, infinity);
    GateRef diff2 = DoubleEqual(x, negativeInfinity);
    return TruncInt32ToInt1(Word32Or(Word32Equal(SExtInt1ToInt32(diff1), GetInt32Constant(1)),
        Word32Equal(SExtInt1ToInt32(diff2), GetInt32Constant(1))));
}

GateRef Stub::TaggedIsNull(GateRef x)
{
    return Word64Equal(x, GetWord64Constant(JSTaggedValue::VALUE_NULL));
}

GateRef Stub::TaggedIsUndefinedOrNull(GateRef x)
{
    return TruncInt32ToInt1(Word32Or(SExtInt1ToInt32(TaggedIsUndefined(x)), SExtInt1ToInt32(TaggedIsNull(x))));
}

GateRef Stub::TaggedIsTrue(GateRef x)
{
    return Word64Equal(x, GetWord64Constant(JSTaggedValue::VALUE_TRUE));
}

GateRef Stub::TaggedIsFalse(GateRef x)
{
    return Word64Equal(x, GetWord64Constant(JSTaggedValue::VALUE_FALSE));
}

GateRef Stub::TaggedIsBoolean(GateRef x)
{
    return TruncInt32ToInt1(Word32Or(SExtInt1ToInt32(TaggedIsTrue(x)), SExtInt1ToInt32(TaggedIsFalse(x))));
}

GateRef Stub::IntBuildTaggedWithNoGC(GateRef x)
{
    GateRef val = ZExtInt32ToInt64(x);
    return Word64Or(val, GetWord64Constant(JSTaggedValue::TAG_INT));
}

GateRef Stub::Int64BuildTaggedWithNoGC(GateRef x)
{
    return Word64Or(x, GetWord64Constant(JSTaggedValue::TAG_INT));
}

GateRef Stub::DoubleBuildTaggedWithNoGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return Int64Add(val, GetWord64Constant(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef Stub::CastDoubleToInt64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::BITCAST_FLOAT64_TO_INT64), x);
}

GateRef Stub::TaggedTrue()
{
    return GetWord64Constant(JSTaggedValue::VALUE_TRUE);
}

GateRef Stub::TaggedFalse()
{
    return GetWord64Constant(JSTaggedValue::VALUE_FALSE);
}

// compare operation
GateRef Stub::Word32Equal(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_EQ), x, y);
}

GateRef Stub::Word32NotEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_NE), x, y);
}

GateRef Stub::Word64Equal(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_EQ), x, y);
}

GateRef Stub::DoubleEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::FLOAT64_EQ), x, y);
}

GateRef Stub::Word64NotEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_NE), x, y);
}

GateRef Stub::Int32GreaterThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SGT), x, y);
}

GateRef Stub::Int32LessThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SLT), x, y);
}

GateRef Stub::Int32GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SGE), x, y);
}

GateRef Stub::Int32LessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SLE), x, y);
}

GateRef Stub::Word32GreaterThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_UGT), x, y);
}

GateRef Stub::Word32LessThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_ULT), x, y);
}

GateRef Stub::Word32LessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_ULE), x, y);
}

GateRef Stub::Word32GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_UGE), x, y);
}

GateRef Stub::Int64GreaterThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SGT), x, y);
}

GateRef Stub::Int64LessThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SLT), x, y);
}

GateRef Stub::Int64LessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SLE), x, y);
}

GateRef Stub::Int64GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SGE), x, y);
}

GateRef Stub::Word64GreaterThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_UGT), x, y);
}

GateRef Stub::Word64LessThan(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_ULT), x, y);
}

GateRef Stub::Word64LessThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_ULE), x, y);
}

GateRef Stub::Word64GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_.GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_UGE), x, y);
}

// cast operation
GateRef Stub::ChangeInt64ToInt32(GateRef val)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT32), val);
}

GateRef Stub::ChangeInt64ToUintPtr(GateRef val)
{
    if (env_.IsArch32Bit()) {
        return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT32), val);
    }
    return val;
}

GateRef Stub::ChangeInt32ToUintPtr(GateRef val)
{
    if (env_.IsArch32Bit()) {
        return val;
    }
    return ZExtInt32ToInt64(val);
}

GateRef Stub::GetSetterFromAccessor(GateRef accessor)
{
    GateRef setterOffset = GetArchRelateConstant(AccessorData::SETTER_OFFSET);
    return Load(MachineType::TAGGED, accessor, setterOffset);
}

GateRef Stub::GetElementsArray(GateRef object)
{
    GateRef elementsOffset = GetArchRelateConstant(JSObject::ELEMENTS_OFFSET);
    return Load(MachineType::TAGGED_POINTER, object, elementsOffset);
}

void Stub::SetElementsArray(GateRef glue, GateRef object, GateRef elementsArray)
{
    GateRef elementsOffset = GetArchRelateConstant(JSObject::ELEMENTS_OFFSET);
    Store(MachineType::TAGGED_POINTER, glue, object, elementsOffset, elementsArray);
}

GateRef Stub::GetPropertiesArray(GateRef object)
{
    GateRef propertiesOffset = GetArchRelateConstant(JSObject::PROPERTIES_OFFSET);
    return Load(MachineType::TAGGED_POINTER, object, propertiesOffset);
}

// SetProperties in js_object.h
void Stub::SetPropertiesArray(GateRef glue, GateRef object, GateRef propsArray)
{
    GateRef propertiesOffset = GetArchRelateConstant(JSObject::PROPERTIES_OFFSET);
    Store(MachineType::TAGGED_POINTER, glue, object, propertiesOffset, propsArray);
}

GateRef Stub::GetLengthofTaggedArray(GateRef array)
{
    return Load(MachineType::UINT32, array, GetArchRelateConstant(TaggedArray::GetLengthOffset()));
}

GateRef Stub::IsJSHClass(GateRef obj)
{
    return Word32Equal(GetObjectType(LoadHClass(obj)),  GetInt32Constant(static_cast<int32_t>(JSType::HCLASS)));
}
// object operation
GateRef Stub::LoadHClass(GateRef object)
{
    return Load(MachineType::TAGGED_POINTER, object);
}

void Stub::StoreHClass(GateRef glue, GateRef object, GateRef hclass)
{
    Store(MachineType::TAGGED_POINTER, glue, object, GetArchRelateConstant(0), hclass);
}

GateRef Stub::GetObjectType(GateRef hClass)
{
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    if (env_.IsArm32()) {
        GateRef bitfield1 = Load(MachineType::NATIVE_POINTER, hClass, ZExtInt32ToInt64(bitfieldOffset));
        return Word32And(bitfield1,
            ChangeInt64ToInt32(GetWord64Constant((1LLU << JSHClass::ObjectTypeBits::SIZE) - 1)));
    } else {
        GateRef bitfield2 = Load(MachineType::NATIVE_POINTER, hClass, bitfieldOffset);
        return ChangeInt64ToInt32(
            Word64And(bitfield2, GetWord64Constant((1LLU << JSHClass::ObjectTypeBits::SIZE) - 1)));
    }
}

GateRef Stub::IsDictionaryMode(GateRef object)
{
    GateRef objectType = GetObjectType(LoadHClass(object));
    return Word32Equal(objectType,
        GetInt32Constant(static_cast<int32_t>(JSType::TAGGED_DICTIONARY)));
}

GateRef Stub::IsDictionaryModeByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(MachineType::UINT32, hClass, bitfieldOffset);
    return Word32NotEqual(
        Word32And(
            Word32LSR(bitfield, GetInt32Constant(JSHClass::IsDictionaryBit::START_BIT)),
            GetInt32Constant((1LLU << JSHClass::IsDictionaryBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::IsDictionaryElement(GateRef hClass)
{
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(MachineType::UINT32, hClass, bitfieldOffset);
    // decode
    return Word32NotEqual(
        Word32And(
            Word32LSR(bitfield, GetInt32Constant(JSHClass::DictionaryElementBits::START_BIT)),
            GetInt32Constant((1LU << JSHClass::DictionaryElementBits::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::NotBuiltinsConstructor(GateRef object)
{
    GateRef hclass = LoadHClass(object);
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);

    GateRef bitfield = Load(MachineType::UINT32, hclass, bitfieldOffset);
    // decode
    return Word32Equal(
        Word32And(
            Word32LSR(bitfield, GetInt32Constant(JSHClass::BuiltinsCtorBit::START_BIT)),
            GetInt32Constant((1LU << JSHClass::BuiltinsCtorBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::IsClassConstructor(GateRef object)
{
    GateRef functionInfoFlagOffset = GetArchRelateConstant(JSFunction::FUNCTION_INFO_FLAG_OFFSET);
    GateRef functionInfoTaggedValue = Load(MachineType::UINT64, object, functionInfoFlagOffset);
    GateRef functionInfoInt32 = TaggedCastToInt32(functionInfoTaggedValue);
    GateRef functionInfoFlag = ZExtInt32ToInt64(functionInfoInt32);
    // decode
    return Word64NotEqual(
        Word64And(
            Word64LSR(functionInfoFlag, GetWord64Constant(JSFunction::ClassConstructorBit::START_BIT)),
            GetWord64Constant((1LLU << JSFunction::ClassConstructorBit::SIZE) - 1)),
        GetWord64Constant(0));
}

GateRef Stub::IsExtensible(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);

    GateRef bitfield = Load(MachineType::UINT32, hClass, bitfieldOffset);
    // decode
    return Word32NotEqual(
        Word32And(Word32LSR(bitfield, GetInt32Constant(JSHClass::ExtensibleBit::START_BIT)),
                  GetInt32Constant((1LU << JSHClass::ExtensibleBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::IsEcmaObject(GateRef obj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->PushCurrentLabel(&subentry);
    Label exit(env);
    Label isHeapObject(env);
    DEFVARIABLE(result, MachineType::BOOL, FalseConstant());
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objectType = GetObjectType(LoadHClass(obj));
        auto ret1 = Word32And(
            ZExtInt1ToInt32(
                Int32LessThanOrEqual(objectType, GetInt32Constant(static_cast<int32_t>(JSType::ECMA_OBJECT_END)))),
            ZExtInt1ToInt32(
                Int32GreaterThanOrEqual(objectType,
                    GetInt32Constant(static_cast<int32_t>(JSType::ECMA_OBJECT_BEGIN)))));
        result = TruncInt32ToInt1(ret1);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::IsSymbol(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Word32Equal(objectType, GetInt32Constant(static_cast<int32_t>(JSType::SYMBOL)));
}

GateRef Stub::IsString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Word32Equal(objectType, GetInt32Constant(static_cast<int32_t>(JSType::STRING)));
}

GateRef Stub::IsJsProxy(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Word32Equal(objectType, GetInt32Constant(static_cast<int32_t>(JSType::JS_PROXY)));
}

GateRef Stub::IsJsArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Word32Equal(objectType, GetInt32Constant(static_cast<int32_t>(JSType::JS_ARRAY)));
}

GateRef Stub::IsWritable(GateRef attr)
{
    return Word32NotEqual(
        Word32And(
            Word32LSR(attr, GetInt32Constant(PropertyAttributes::WritableField::START_BIT)),
            GetInt32Constant((1LLU << PropertyAttributes::WritableField::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::IsAccessor(GateRef attr)
{
    return Word32NotEqual(
        Word32And(Word32LSR(attr,
            GetInt32Constant(PropertyAttributes::IsAccessorField::START_BIT)),
            GetInt32Constant((1LLU << PropertyAttributes::IsAccessorField::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::IsInlinedProperty(GateRef attr)
{
    return Word32NotEqual(
        Word32And(Word32LSR(attr,
            GetInt32Constant(PropertyAttributes::IsInlinedPropsField::START_BIT)),
            GetInt32Constant((1LLU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::GetProtoCell(GateRef object)
{
    GateRef protoCellOffset = GetArchRelateConstant(PrototypeHandler::PROTO_CELL_OFFSET);
    return Load(MachineType::UINT64, object, protoCellOffset);
}

GateRef Stub::GetPrototypeHandlerHolder(GateRef object)
{
    GateRef holderOffset = GetArchRelateConstant(PrototypeHandler::HOLDER_OFFSET);
    return Load(MachineType::TAGGED, object, holderOffset);
}

GateRef Stub::GetPrototypeHandlerHandlerInfo(GateRef object)
{
    GateRef handlerInfoOffset = GetArchRelateConstant(PrototypeHandler::HANDLER_INFO_OFFSET);
    return Load(MachineType::TAGGED, object, handlerInfoOffset);
}

GateRef Stub::GetHasChanged(GateRef object)
{
    GateRef hasChangedOffset = GetArchRelateConstant(ProtoChangeMarker::HAS_CHANGED_OFFSET);
    return Word64NotEqual(Load(MachineType::UINT64, object, hasChangedOffset), GetWord64Constant(0));
}

GateRef Stub::HclassIsPrototypeHandler(GateRef hclass)
{
    return Word32Equal(GetObjectType(hclass),
        GetInt32Constant(static_cast<int32_t>(JSType::PROTOTYPE_HANDLER)));
}

GateRef Stub::HclassIsTransitionHandler(GateRef hclass)
{
    return Word32Equal(GetObjectType(hclass),
        GetInt32Constant(static_cast<int32_t>(JSType::TRANSITION_HANDLER)));
}

GateRef Stub::HclassIsPropertyBox(GateRef hclass)
{
    return Word32Equal(GetObjectType(hclass),
        GetInt32Constant(static_cast<int32_t>(JSType::PROPERTY_BOX)));
}

GateRef Stub::IsField(GateRef attr)
{
    return Word32Equal(
        Word32And(
            Word32LSR(attr, GetInt32Constant(HandlerBase::KindBit::START_BIT)),
            GetInt32Constant((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        GetInt32Constant(HandlerBase::HandlerKind::FIELD));
}

GateRef Stub::IsNonExist(GateRef attr)
{
    return Word32Equal(
        Word32And(
            Word32LSR(attr, GetInt32Constant(HandlerBase::KindBit::START_BIT)),
            GetInt32Constant((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        GetInt32Constant(HandlerBase::HandlerKind::NON_EXIST));
}

GateRef Stub::HandlerBaseIsAccessor(GateRef attr)
{
    return Word32NotEqual(
        Word32And(Word32LSR(attr,
            GetInt32Constant(HandlerBase::AccessorBit::START_BIT)),
            GetInt32Constant((1LLU << HandlerBase::AccessorBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::HandlerBaseIsJSArray(GateRef attr)
{
    return Word32NotEqual(
        Word32And(Word32LSR(attr,
            GetInt32Constant(HandlerBase::IsJSArrayBit::START_BIT)),
            GetInt32Constant((1LLU << HandlerBase::IsJSArrayBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::HandlerBaseIsInlinedProperty(GateRef attr)
{
    return Word32NotEqual(
        Word32And(Word32LSR(attr,
            GetInt32Constant(HandlerBase::InlinedPropsBit::START_BIT)),
            GetInt32Constant((1LLU << HandlerBase::InlinedPropsBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::HandlerBaseGetOffset(GateRef attr)
{
    return Word32And(Word32LSR(attr,
        GetInt32Constant(HandlerBase::OffsetBit::START_BIT)),
        GetInt32Constant((1LLU << HandlerBase::OffsetBit::SIZE) - 1));
}

GateRef Stub::IsInternalAccessor(GateRef attr)
{
    return Word32NotEqual(
        Word32And(Word32LSR(attr,
            GetInt32Constant(HandlerBase::InternalAccessorBit::START_BIT)),
            GetInt32Constant((1LLU << HandlerBase::InternalAccessorBit::SIZE) - 1)),
        GetInt32Constant(0));
}

GateRef Stub::IsInvalidPropertyBox(GateRef obj)
{
    GateRef valueOffset = GetArchRelateConstant(PropertyBox::VALUE_OFFSET);
    GateRef value = Load(MachineType::UINT64, obj, valueOffset);
    return TaggedIsHole(value);
}

GateRef Stub::GetValueFromPropertyBox(GateRef obj)
{
    GateRef valueOffset = GetArchRelateConstant(PropertyBox::VALUE_OFFSET);
    return Load(MachineType::TAGGED, obj, valueOffset);
}

void Stub::SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value)
{
    GateRef valueOffset = GetArchRelateConstant(PropertyBox::VALUE_OFFSET);
    Store(MachineType::TAGGED, glue, obj, valueOffset, value);
}

GateRef Stub::GetTransitionFromHClass(GateRef obj)
{
    GateRef transitionHClassOffset = GetArchRelateConstant(TransitionHandler::TRANSITION_HCLASS_OFFSET);
    return Load(MachineType::TAGGED_POINTER, obj, transitionHClassOffset);
}

GateRef Stub::GetTransitionHandlerInfo(GateRef obj)
{
    GateRef handlerInfoOffset = GetArchRelateConstant(TransitionHandler::HANDLER_INFO_OFFSET);
    return Load(MachineType::TAGGED, obj, handlerInfoOffset);
}

GateRef Stub::PropAttrGetOffset(GateRef attr)
{
    return Word32And(
        Word32LSR(attr, GetInt32Constant(PropertyAttributes::OffsetField::START_BIT)),
        GetInt32Constant((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetDictionaryOrder func in property_attribute.h
GateRef Stub::SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Word32LSL(
        GetInt32Constant((1LLU << PropertyAttributes::DictionaryOrderField::SIZE) - 1),
        GetInt32Constant(PropertyAttributes::DictionaryOrderField::START_BIT));
    GateRef newVal = Word32Or(Word32And(attr, Word32Not(mask)),
        Word32LSL(value, GetInt32Constant(PropertyAttributes::DictionaryOrderField::START_BIT)));
    return newVal;
}

GateRef Stub::GetPrototypeFromHClass(GateRef hClass)
{
    GateRef protoOffset = GetArchRelateConstant(JSHClass::PROTOTYPE_OFFSET);
    return Load(MachineType::TAGGED, hClass, protoOffset);
}

GateRef Stub::GetLayoutFromHClass(GateRef hClass)
{
    GateRef attrOffset = GetArchRelateConstant(JSHClass::LAYOUT_OFFSET);
    return Load(MachineType::TAGGED_POINTER, hClass, attrOffset);
}

GateRef Stub::GetBitFieldFromHClass(GateRef hClass)
{
    GateRef offset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    return Load(MachineType::UINT64, hClass, offset);
}

GateRef Stub::SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield)
{
    GateRef offset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    return Store(MachineType::UINT64, glue, hClass, offset, bitfield);
}

GateRef Stub::SetPrototypeToHClass(MachineType type, GateRef glue, GateRef hClass, GateRef proto)
{
    GateRef offset = GetArchRelateConstant(JSHClass::PROTOTYPE_OFFSET);
    return Store(type, glue, hClass, offset, proto);
}

GateRef Stub::SetProtoChangeDetailsToHClass(MachineType type, GateRef glue, GateRef hClass, GateRef protoChange)
{
    GateRef offset = GetArchRelateConstant(JSHClass::PROTOTYPE_INFO_OFFSET);
    return Store(type, glue, hClass, offset, protoChange);
}

GateRef Stub::SetLayoutToHClass(GateRef glue, GateRef hClass, GateRef attr)
{
    GateRef offset = GetArchRelateConstant(JSHClass::LAYOUT_OFFSET);
    return Store(MachineType::TAGGED_POINTER, glue, hClass, offset, attr);
}

GateRef Stub::SetParentToHClass(MachineType type, GateRef glue, GateRef hClass, GateRef parent)
{
    GateRef offset = GetArchRelateConstant(JSHClass::PARENT_OFFSET);
    return Store(type, glue, hClass, offset, parent);
}

GateRef Stub::SetEnumCacheToHClass(MachineType type, GateRef glue, GateRef hClass, GateRef key)
{
    GateRef offset = GetArchRelateConstant(JSHClass::ENUM_CACHE_OFFSET);
    return Store(type, glue, hClass, offset, key);
}

GateRef Stub::SetTransitionsToHClass(MachineType type, GateRef glue, GateRef hClass, GateRef transition)
{
    GateRef offset = GetArchRelateConstant(JSHClass::TRANSTIONS_OFFSET);
    return Store(type, glue, hClass, offset, transition);
}

void Stub::SetIsProtoTypeToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef oldValue = ZExtInt1ToInt64(value);
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    GateRef mask = Word64LSL(
        GetWord64Constant((1LLU << JSHClass::IsPrototypeBit::SIZE) - 1),
        GetWord64Constant(JSHClass::IsPrototypeBit::START_BIT));
    GateRef newVal = Word64Or(Word64And(bitfield, Word64Not(mask)),
        Word64LSL(oldValue, GetWord64Constant(JSHClass::IsPrototypeBit::START_BIT)));
    SetBitFieldToHClass(glue, hClass, newVal);
}

GateRef Stub::IsProtoTypeHClass(GateRef hClass)
{
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    return TruncInt64ToInt1(Word64And(Word64LSR(bitfield,
        GetWord64Constant(JSHClass::IsPrototypeBit::START_BIT)),
        GetWord64Constant((1LLU << JSHClass::IsPrototypeBit::SIZE) - 1)));
}

void Stub::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset)
{
    GateRef bitfield = Load(MachineType::UINT32, hClass,
                            GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Word32And(Word32LSR(bitfield,
        GetInt32Constant(JSHClass::InlinedPropsStartBits::START_BIT)),
        GetInt32Constant((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(
        Int32Add(inlinedPropsStart, attrOffset), GetInt32Constant(JSTaggedValue::TaggedTypeSize()));

    // NOTE: need to translate MarkingBarrier
    Store(MachineType::TAGGED, glue, obj, ChangeInt32ToUintPtr(propOffset), value);
}

void Stub::IncNumberOfProps(GateRef glue, GateRef hClass)
{
    GateRef propNums = GetNumberOfPropsFromHClass(hClass);
    SetNumberOfPropsToHClass(glue, hClass, Int32Add(propNums, GetInt32Constant(1)));
}

GateRef Stub::GetNumberOfPropsFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(MachineType::UINT32, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET));
    return Word32And(Word32LSR(bitfield,
        GetInt32Constant(JSHClass::NumberOfPropsBits::START_BIT)),
        GetInt32Constant((1LLU << JSHClass::NumberOfPropsBits::SIZE) - 1));
}

void Stub::SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield1 = Load(MachineType::UINT32, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET));
    GateRef oldWithMask = Word32And(bitfield1,
        GetInt32Constant(~static_cast<int32_t>(JSHClass::NumberOfPropsBits::Mask())));
    GateRef newValue = Word32LSR(value, GetInt32Constant(JSHClass::NumberOfPropsBits::START_BIT));
    Store(MachineType::UINT32, glue, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET),
        Word32Or(oldWithMask, newValue));
}

GateRef Stub::GetInlinedPropertiesFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(MachineType::UINT32, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET));
    GateRef objectSizeInWords = Word32And(Word32LSR(bitfield,
        GetInt32Constant(JSHClass::ObjectSizeInWordsBits::START_BIT)),
        GetInt32Constant((1LU << JSHClass::ObjectSizeInWordsBits::SIZE) - 1));
    GateRef inlinedPropsStart = Word32And(Word32LSR(bitfield,
        GetInt32Constant(JSHClass::InlinedPropsStartBits::START_BIT)),
        GetInt32Constant((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    return Int32Sub(objectSizeInWords, inlinedPropsStart);
}

GateRef Stub::GetObjectSizeFromHClass(GateRef hClass) // NOTE: need to add special case for string and TAGGED_ARRAY
{
    GateRef bitfield = Load(MachineType::UINT32, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET));
    GateRef objectSizeInWords = Word32And(Word32LSR(bitfield,
        GetInt32Constant(JSHClass::ObjectSizeInWordsBits::START_BIT)),
        GetInt32Constant((1LU << JSHClass::ObjectSizeInWordsBits::SIZE) - 1));
    return ArchRelatePtrMul(ChangeInt32ToUintPtr(objectSizeInWords),
        GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()));
}

GateRef Stub::GetInlinedPropsStartFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(MachineType::UINT32, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD1_OFFSET));
    return Word32And(Word32LSR(bitfield,
        GetInt32Constant(JSHClass::InlinedPropsStartBits::START_BIT)),
        GetInt32Constant((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
}

void Stub::SetValueToTaggedArray(MachineType valType, GateRef glue, GateRef array, GateRef index, GateRef val)
{
    // NOTE: need to translate MarkingBarrier
    GateRef offset =
        ArchRelatePtrMul(ChangeInt32ToUintPtr(index), GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, GetArchRelateConstant(TaggedArray::GetDataOffset()));
    Store(valType, glue, array, dataOffset, val);
}

GateRef Stub::GetValueFromTaggedArray(MachineType returnType, GateRef array, GateRef index)
{
    GateRef offset =
        ArchRelatePtrMul(ChangeInt32ToUintPtr(index), GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, GetArchRelateConstant(TaggedArray::GetDataOffset()));
    return Load(returnType, array, dataOffset);
}

GateRef Stub::GetElementRepresentation(GateRef hClass)
{
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(MachineType::UINT32, hClass, bitfieldOffset);
    return Word32And(
        Word32LSR(bitfield, GetInt32Constant(JSHClass::ElementRepresentationBits::START_BIT)),
        GetInt32Constant(((1LLU << JSHClass::ElementRepresentationBits::SIZE) - 1)));
}

void Stub::SetElementRepresentation(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);
    GateRef oldValue = Load(MachineType::UINT32, hClass, bitfieldOffset);
    GateRef oldWithMask = Word32And(oldValue,
        GetInt32Constant(~static_cast<int32_t>(JSHClass::ElementRepresentationBits::Mask())));
    GateRef newValue = Word32LSR(value, GetInt32Constant(JSHClass::ElementRepresentationBits::START_BIT));
    Store(MachineType::UINT32, glue, hClass, bitfieldOffset, Word32Or(oldWithMask, newValue));
}

void Stub::UpdateAndStoreRepresention(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef rep = GetElementRepresentation(hClass);
    GateRef newRep = UpdateRepresention(rep, value);
    SetElementRepresentation(glue, hClass, newRep);
}

void Stub::UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index, GateRef value, GateRef attr)
{
    GateRef arrayIndex =
        Int32Add(GetInt32Constant(NameDictionary::TABLE_HEADER_SIZE),
                 Int32Mul(index, GetInt32Constant(NameDictionary::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, GetInt32Constant(NameDictionary::ENTRY_VALUE_INDEX));
    GateRef attributesIndex =
        Int32Add(arrayIndex, GetInt32Constant(NameDictionary::ENTRY_DETAILS_INDEX));
    SetValueToTaggedArray(MachineType::TAGGED, glue, elements, valueIndex, value);
    GateRef attroffset =
        ArchRelatePtrMul(ChangeInt32ToUintPtr(attributesIndex), GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(attroffset, GetArchRelateConstant(TaggedArray::GetDataOffset()));
    Store(MachineType::INT64, glue, elements, dataOffset, IntBuildTaggedWithNoGC(attr));
}

GateRef Stub::IsSpecialIndexedObj(GateRef jsType)
{
    return Int32GreaterThan(jsType, GetInt32Constant(static_cast<int32_t>(JSType::JS_ARRAY)));
}

GateRef Stub::IsAccessorInternal(GateRef value)
{
    return Word32Equal(GetObjectType(LoadHClass(value)),
                       GetInt32Constant(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR)));
}

template<typename DictionaryT>
GateRef Stub::GetAttributesFromDictionary(GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
    Int32Add(GetInt32Constant(DictionaryT::TABLE_HEADER_SIZE),
             Int32Mul(entry, GetInt32Constant(DictionaryT::ENTRY_SIZE)));
    GateRef attributesIndex =
        Int32Add(arrayIndex, GetInt32Constant(DictionaryT::ENTRY_DETAILS_INDEX));
    return TaggedCastToInt32(GetValueFromTaggedArray(MachineType::UINT64, elements, attributesIndex));
}

template<typename DictionaryT>
GateRef Stub::GetValueFromDictionary(MachineType returnType, GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
        Int32Add(GetInt32Constant(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, GetInt32Constant(DictionaryT::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, GetInt32Constant(DictionaryT::ENTRY_VALUE_INDEX));
    return GetValueFromTaggedArray(returnType, elements, valueIndex);
}

template<typename DictionaryT>
GateRef Stub::GetKeyFromDictionary(MachineType returnType, GateRef elements, GateRef entry)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->PushCurrentLabel(&subentry);
    Label exit(env);
    DEFVARIABLE(result, returnType, GetUndefinedConstant());
    Label ltZero(env);
    Label notLtZero(env);
    Label gtLength(env);
    Label notGtLength(env);
    GateRef dictionaryLength =
        Load(MachineType::INT32, elements, GetArchRelateConstant(panda::coretypes::Array::GetLengthOffset()));
    GateRef arrayIndex =
        Int32Add(GetInt32Constant(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, GetInt32Constant(DictionaryT::ENTRY_SIZE)));
    Branch(Int32LessThan(arrayIndex, GetInt32Constant(0)), &ltZero, &notLtZero);
    Bind(&ltZero);
    Jump(&exit);
    Bind(&notLtZero);
    Branch(Int32GreaterThan(arrayIndex, dictionaryLength), &gtLength, &notGtLength);
    Bind(&gtLength);
    Jump(&exit);
    Bind(&notGtLength);
    result = GetValueFromTaggedArray(returnType, elements, arrayIndex);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry)
{
    GateRef index = Int32Add(Int32Add(GetInt32Constant(LayoutInfo::ELEMENTS_START_INDEX),
        Word32LSL(entry, GetInt32Constant(1))), GetInt32Constant(1));
    return GetValueFromTaggedArray(MachineType::UINT64, layout, index);
}

GateRef Stub::GetPropertyMetaDataFromAttr(GateRef attr)
{
    return Word32And(Word32LSR(attr, GetInt32Constant(PropertyAttributes::PropertyMetaDataField::START_BIT)),
        GetInt32Constant((1LLU << PropertyAttributes::PropertyMetaDataField::SIZE) - 1));
}

GateRef Stub::GetKeyFromLayoutInfo(GateRef layout, GateRef entry)
{
    GateRef index = Int32Add(
        GetInt32Constant(LayoutInfo::ELEMENTS_START_INDEX),
        Word32LSL(entry, GetInt32Constant(1)));
    return GetValueFromTaggedArray(MachineType::TAGGED, layout, index);
}

GateRef Stub::GetPropertiesAddrFromLayoutInfo(GateRef layout)
{
    GateRef eleStartIdx = ArchRelatePtrMul(GetArchRelateConstant(LayoutInfo::ELEMENTS_START_INDEX),
        GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()));
    return PtrAdd(layout, PtrAdd(GetArchRelateConstant(TaggedArray::GetDataOffset()), eleStartIdx));
}

GateRef Stub::TaggedCastToInt64(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    return Word64And(tagged, GetWord64Constant(~JSTaggedValue::TAG_MASK));
}

GateRef Stub::TaggedCastToInt32(GateRef x)
{
    return ChangeInt64ToInt32(TaggedCastToInt64(x));
}

GateRef Stub::TaggedCastToDouble(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    GateRef val = Int64Sub(tagged, GetWord64Constant(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    return CastInt64ToFloat64(val);
}

GateRef Stub::TaggedCastToWeakReferentUnChecked(GateRef x)
{
    return Word64And(x, GetWord64Constant(~JSTaggedValue::TAG_WEAK_MASK));
}

GateRef Stub::ChangeInt32ToFloat64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_TO_FLOAT64), x);
}

GateRef Stub::ChangeFloat64ToInt32(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_TO_INT32), x);
}

GateRef Stub::ChangeTaggedPointerToInt64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TAGGED_POINTER_TO_INT64), x);
}

GateRef Stub::CastInt64ToFloat64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::BITCAST_INT64_TO_FLOAT64), x);
}

GateRef Stub::SExtInt32ToInt64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT32_TO_INT64), x);
}

GateRef Stub::SExtInt1ToInt64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT1_TO_INT64), x);
}

GateRef Stub::SExtInt1ToInt32(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT1_TO_INT32), x);
}

GateRef Stub::ZExtInt32ToInt64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT32_TO_INT64), x);
}

GateRef Stub::ZExtInt1ToInt64(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT1_TO_INT64), x);
}

GateRef Stub::ZExtInt1ToInt32(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT1_TO_INT32), x);
}

GateRef Stub::ZExtInt8ToInt32(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT8_TO_INT32), x);
}

GateRef Stub::ZExtInt16ToInt32(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT16_TO_INT32), x);
}

GateRef Stub::TruncInt64ToInt32(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT32), x);
}

GateRef Stub::TruncInt64ToInt1(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT1), x);
}

GateRef Stub::TruncInt32ToInt1(GateRef x)
{
    return env_.GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT32_TO_INT1), x);
}

GateRef Stub::GetGlobalConstantAddr(GateRef index)
{
    return Int64Mul(GetWord64Constant(sizeof(JSTaggedValue)), index);
}

GateRef Stub::GetGlobalConstantString(ConstantIndex index)
{
    if (env_.IsArm32()) {
        return Int32Mul(GetInt32Constant(sizeof(JSTaggedValue)), GetInt32Constant(static_cast<int>(index)));
    } else {
        return Int64Mul(GetWord64Constant(sizeof(JSTaggedValue)), GetWord64Constant(static_cast<int>(index)));
    }
}

GateRef Stub::IsCallable(GateRef obj)
{
    GateRef hclass = LoadHClass(obj);
    GateRef bitfieldOffset = GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET);

    // decode
    if (env_.IsArm32()) {
        GateRef bitfield = Load(MachineType::NATIVE_POINTER, hclass, ZExtInt32ToInt64(bitfieldOffset));
        return Word32NotEqual(
            Word32And(Word32LSR(bitfield, GetInt32Constant(JSHClass::CallableBit::START_BIT)),
                      GetInt32Constant((1LLU << JSHClass::CallableBit::SIZE) - 1)),
            GetInt32Constant(0));
    } else {
        GateRef bitfield = Load(MachineType::NATIVE_POINTER, hclass, bitfieldOffset);
        return Word64NotEqual(
            Word64And(Word64LSR(bitfield, GetWord64Constant(JSHClass::CallableBit::START_BIT)),
                      GetWord64Constant((1LLU << JSHClass::CallableBit::SIZE) - 1)),
            GetWord64Constant(0));
    }
}

// GetOffset func in property_attribute.h
GateRef Stub::GetOffsetFieldInPropAttr(GateRef attr)
{
    return Word32And(
        Word32LSR(attr, GetInt32Constant(PropertyAttributes::OffsetField::START_BIT)),
        GetInt32Constant((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetOffset func in property_attribute.h
GateRef Stub::SetOffsetFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Word32LSL(
        GetInt32Constant((1LLU << PropertyAttributes::OffsetField::SIZE) - 1),
        GetInt32Constant(PropertyAttributes::OffsetField::START_BIT));
    GateRef newVal = Word32Or(Word32And(attr, Word32Not(mask)),
        Word32LSL(value, GetInt32Constant(PropertyAttributes::OffsetField::START_BIT)));
    return newVal;
}

// SetIsInlinedProps func in property_attribute.h
GateRef Stub::SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Word32LSL(
        GetInt32Constant((1LU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1),
        GetInt32Constant(PropertyAttributes::IsInlinedPropsField::START_BIT));
    GateRef newVal = Word32Or(Word32And(attr, Word32Not(mask)),
        Word32LSL(value, GetInt32Constant(PropertyAttributes::IsInlinedPropsField::START_BIT)));
    return newVal;
}

void Stub::SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield = Load(MachineType::UINT32, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET));
    GateRef mask = Word32LSL(
        GetInt32Constant((1LU << JSHClass::HasConstructorBits::SIZE) - 1),
        GetInt32Constant(JSHClass::HasConstructorBits::START_BIT));
    GateRef newVal = Word32Or(Word32And(bitfield, Word32Not(mask)),
        Word32LSL(value, GetInt32Constant(JSHClass::HasConstructorBits::START_BIT)));
    Store(MachineType::UINT32, glue, hClass, GetArchRelateConstant(JSHClass::BIT_FIELD_OFFSET), newVal);
}

void Stub::UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value)
{
    GateRef arrayIndex = Int32Add(GetInt32Constant(NameDictionary::TABLE_HEADER_SIZE),
        Int32Mul(index, GetInt32Constant(NameDictionary::ENTRY_SIZE)));
    GateRef valueIndex = Int32Add(arrayIndex, GetInt32Constant(NameDictionary::ENTRY_VALUE_INDEX));
    SetValueToTaggedArray(MachineType::TAGGED, glue, elements, valueIndex, value);
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_STUB_INL_H
