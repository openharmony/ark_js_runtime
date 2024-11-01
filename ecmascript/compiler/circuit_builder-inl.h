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
#include "ecmascript/js_method.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript::kungfu {
// constant
GateRef CircuitBuilder::True()
{
    return TruncInt32ToInt1(Int32(1));
}

GateRef CircuitBuilder::False()
{
    return TruncInt32ToInt1(Int32(0));
}

GateRef CircuitBuilder::Undefined(VariableType type)
{
    return UndefineConstant(type.GetGateType());
}

// memory
GateRef CircuitBuilder::Load(VariableType type, GateRef base, GateRef offset)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef val = PtrAdd(base, offset);
    GateRef result = GetCircuit()->NewGate(OpCode(OpCode::LOAD), type.GetMachineType(),
                                           0, { depend, val }, type.GetGateType());
    label->SetDepend(result);
    return result;
}

// Js World
// cast operation
GateRef CircuitBuilder::TaggedCastToInt64(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    return Int64And(tagged, Int64(~JSTaggedValue::TAG_MARK));
}

GateRef CircuitBuilder::TaggedCastToInt32(GateRef x)
{
    return ChangeInt64ToInt32(TaggedCastToInt64(x));
}

GateRef CircuitBuilder::TaggedCastToIntPtr(GateRef x)
{
    ASSERT(cmpCfg_ != nullptr);
    return cmpCfg_->Is32Bit() ? TaggedCastToInt32(x) : TaggedCastToInt64(x);
}

GateRef CircuitBuilder::TaggedCastToDouble(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    GateRef val = Int64Sub(tagged, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    return CastInt64ToFloat64(val);
}

GateRef CircuitBuilder::ChangeTaggedPointerToInt64(GateRef x)
{
    return UnaryArithmetic(OpCode(OpCode::TAGGED_TO_INT64), x);
}

GateRef CircuitBuilder::ChangeInt64ToTagged(GateRef x)
{
    return TaggedNumber(OpCode(OpCode::INT64_TO_TAGGED), x);
}

// bit operation
GateRef CircuitBuilder::IsSpecial(GateRef x, JSTaggedType type)
{
    return Equal(x, Int64(type));
}
GateRef CircuitBuilder::TaggedIsInt(GateRef x)
{
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                 Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::TaggedIsDouble(GateRef x)
{
    return BoolAnd(TaggedIsNumber(x), BoolNot(TaggedIsInt(x)));
}

GateRef CircuitBuilder::TaggedIsObject(GateRef x)
{
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                 Int64(JSTaggedValue::TAG_OBJECT));
}

GateRef CircuitBuilder::TaggedIsNumber(GateRef x)
{
    return BoolNot(TaggedIsObject(x));
}

GateRef CircuitBuilder::TaggedIsHole(GateRef x)
{
    return Equal(x, Int64(JSTaggedValue::VALUE_HOLE));
}

GateRef CircuitBuilder::TaggedIsNotHole(GateRef x)
{
    return NotEqual(x, Int64(JSTaggedValue::VALUE_HOLE));
}

GateRef CircuitBuilder::TaggedIsUndefined(GateRef x)
{
    return Equal(x, Int64(JSTaggedValue::VALUE_UNDEFINED));
}

GateRef CircuitBuilder::TaggedIsException(GateRef x)
{
    return Equal(x, Int64(JSTaggedValue::VALUE_EXCEPTION));
}

GateRef CircuitBuilder::TaggedIsSpecial(GateRef x)
{
    return BoolOr(
        Equal(Int64And(x, Int64(JSTaggedValue::TAG_SPECIAL_MARK)), Int64(JSTaggedValue::TAG_SPECIAL)),
        TaggedIsHole(x));
}

GateRef CircuitBuilder::TaggedIsHeapObject(GateRef x)
{
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(TaggedIsObject(x)),
        SExtInt1ToInt32(Equal(SExtInt1ToInt32(TaggedIsSpecial(x)),
        Int32(0)))));
}

GateRef CircuitBuilder::TaggedIsGeneratorObject(GateRef x)
{
    GateRef isHeapObj = SExtInt1ToInt32(TaggedIsHeapObject(x));
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isGeneratorObj = Int32Or(SExtInt1ToInt32(Equal(objType,
        Int32(static_cast<int32_t>(JSType::JS_GENERATOR_OBJECT)))),
        SExtInt1ToInt32(Equal(objType,
        Int32(static_cast<int32_t>(JSType::JS_ASYNC_FUNC_OBJECT)))));
    return TruncInt32ToInt1(Int32And(isHeapObj, isGeneratorObj));
}

GateRef CircuitBuilder::TaggedIsPropertyBox(GateRef x)
{
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
        SExtInt1ToInt32(IsJsType(x, JSType::PROPERTY_BOX))));
}

GateRef CircuitBuilder::TaggedIsWeak(GateRef x)
{
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
        SExtInt1ToInt32(Equal(Int64And(x,
        Int64(JSTaggedValue::TAG_WEAK)),
        Int64(1)))));
}

GateRef CircuitBuilder::TaggedIsPrototypeHandler(GateRef x)
{
    return IsJsType(x, JSType::PROTOTYPE_HANDLER);
}

GateRef CircuitBuilder::TaggedIsTransitionHandler(GateRef x)
{
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(TaggedIsHeapObject(x)),
        SExtInt1ToInt32(IsJsType(x, JSType::TRANSITION_HANDLER))));
}

GateRef CircuitBuilder::TaggedIsUndefinedOrNull(GateRef x)
{
    return TruncInt32ToInt1(Int32Or(SExtInt1ToInt32(IsSpecial(x, JSTaggedValue::VALUE_UNDEFINED)),
        SExtInt1ToInt32(IsSpecial(x, JSTaggedValue::VALUE_NULL))));
}

GateRef CircuitBuilder::TaggedIsTrue(GateRef x)
{
    return Equal(x, Int64(JSTaggedValue::VALUE_TRUE));
}

GateRef CircuitBuilder::TaggedIsFalse(GateRef x)
{
    return Equal(x, Int64(JSTaggedValue::VALUE_FALSE));
}

GateRef CircuitBuilder::TaggedIsNull(GateRef x)
{
    return Equal(x, Int64(JSTaggedValue::VALUE_NULL));
}

GateRef CircuitBuilder::TaggedIsBoolean(GateRef x)
{
    return TruncInt32ToInt1(Int32Or(SExtInt1ToInt32(IsSpecial(x, JSTaggedValue::VALUE_TRUE)),
        SExtInt1ToInt32(IsSpecial(x, JSTaggedValue::VALUE_FALSE))));
}

GateRef CircuitBuilder::TaggedGetInt(GateRef x)
{
    return TruncInt64ToInt32(Int64And(x, Int64(~JSTaggedValue::TAG_MARK)));
}

GateRef CircuitBuilder::TaggedTypeNGC(GateRef x)
{
    return Int64Or(x, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::TaggedNGC(GateRef x)
{
    return ChangeInt64ToTagged(Int64Or(x, Int64(JSTaggedValue::TAG_INT)));
}

GateRef CircuitBuilder::DoubleToTaggedNGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return ChangeInt64ToTagged(Int64Add(val,
        Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET)));
}

GateRef CircuitBuilder::DoubleToTaggedTypeNGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef CircuitBuilder::Tagged(GateRef x)
{
    GetCircuit()->SetGateType(x, GateType::TaggedValue());
    return Int64Or(x, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::DoubleToTagged(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    GetCircuit()->SetGateType(val, GateType::TaggedValue());
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef CircuitBuilder::TaggedTrue()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_TRUE, GateType::NJSValue());
}

GateRef CircuitBuilder::TaggedFalse()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_FALSE, GateType::NJSValue());
}

GateRef CircuitBuilder::GetValueFromTaggedArray(VariableType returnType, GateRef array, GateRef index)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label exit(env_);
    Label isUndefined(env_);
    Label notUndefined(env_);
    GateRef initVal = GetCircuit()->GetConstantGate(returnType.GetMachineType(), JSTaggedValue::VALUE_UNDEFINED,
        returnType.GetGateType());
    DEFVAlUE(result, env_, returnType, initVal);
    Branch(TaggedIsUndefined(array), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    {
        Jump(&exit);
    }
    Bind(&notUndefined);
    {
        GateRef offset = PtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
        GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
        result = Load(returnType, array, dataOffset);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

void CircuitBuilder::SetValueToTaggedArray(VariableType valType, GateRef glue,
                                           GateRef array, GateRef index, GateRef val)
{
    GateRef offset = PtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(valType, glue, array, dataOffset, val);
}

GateRef CircuitBuilder::GetGlobalConstantString(ConstantIndex index)
{
    return PtrMul(IntPtr(sizeof(JSTaggedValue)), IntPtr(static_cast<int>(index)));
}

// object operation
GateRef CircuitBuilder::LoadHClass(GateRef object)
{
    GateRef offset = Int32(0);
    return Load(VariableType::JS_POINTER(), object, offset);
}

GateRef CircuitBuilder::IsJsType(GateRef obj, JSType type)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Equal(objectType, Int32(static_cast<int32_t>(type)));
}

GateRef CircuitBuilder::GetObjectType(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return Int32And(bitfield, Int32((1LU << JSHClass::ObjectTypeBits::SIZE) - 1));
}

GateRef CircuitBuilder::IsDictionaryModeByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsDictionaryBit::START_BIT)),
        Int32((1LU << JSHClass::IsDictionaryBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsDictionaryElement(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::DictionaryElementBits::START_BIT)),
        Int32((1LU << JSHClass::DictionaryElementBits::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ClassConstructorBit::START_BIT)),
        Int32((1LU << JSHClass::ClassConstructorBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassPrototype(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ClassPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::ClassPrototypeBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsExtensible(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ExtensibleBit::START_BIT)),
        Int32((1LU << JSHClass::ExtensibleBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::TaggedObjectIsEcmaObject(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    auto ret = Int32And(ZExtInt1ToInt32(Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_END)))),
        ZExtInt1ToInt32(Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_BEGIN)))));
    return TruncInt32ToInt1(ret);
}

GateRef CircuitBuilder::IsJsObject(GateRef obj)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label exit(env_);
    Label isHeapObject(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objectType = GetObjectType(LoadHClass(obj));
        auto ret1 = Int32And(ZExtInt1ToInt32(Int32LessThanOrEqual(objectType,
            Int32(static_cast<int32_t>(JSType::JS_OBJECT_END)))),
            ZExtInt1ToInt32(Int32GreaterThanOrEqual(objectType,
            Int32(static_cast<int32_t>(JSType::JS_OBJECT_BEGIN)))));
        result = TruncInt32ToInt1(ret1);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::BothAreString(GateRef x, GateRef y)
{
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(IsJsType(x, JSType::STRING)),
        SExtInt1ToInt32(IsJsType(y, JSType::STRING))));
}

GateRef CircuitBuilder::IsCallable(GateRef obj)
{
    GateRef hclass = LoadHClass(obj);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hclass, bitfieldOffset);
    return NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::CallableBit::START_BIT)),
            Int32((1LU << JSHClass::CallableBit::SIZE) - 1)),
        Int32(0));
}

int CircuitBuilder::NextVariableId()
{
    return env_->NextVariableId();
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit, VariableType type)
{
    Branch(Equal(result, ExceptionConstant(type.GetGateType())), fail, success);
    Bind(fail);
    {
        Jump(exit);
    }
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal)
{
    Branch(Equal(result, exceptionVal), fail, success);
    Bind(fail);
    {
        Jump(exit);
    }
}

void CircuitBuilder::SubCfgEntry(Label *entry)
{
    ASSERT(env_ != nullptr);
    env_->SubCfgEntry(entry);
}

void CircuitBuilder::SubCfgExit()
{
    ASSERT(env_ != nullptr);
    env_->SubCfgExit();
}

GateRef CircuitBuilder::Return(GateRef value)
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return Return(control, depend, value);
}

GateRef CircuitBuilder::Return()
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return ReturnVoid(control, depend);
}

void CircuitBuilder::Bind(Label *label)
{
    label->Bind();
    env_->SetCurrentLabel(label);
}

void CircuitBuilder::Bind(Label *label, bool justSlowPath)
{
    if (!justSlowPath) {
        label->Bind();
        env_->SetCurrentLabel(label);
    }
}

Label *CircuitBuilder::GetCurrentLabel() const
{
    return GetCurrentEnvironment()->GetCurrentLabel();
}

GateRef CircuitBuilder::GetState() const
{
    return GetCurrentLabel()->GetControl();
}

GateRef CircuitBuilder::GetDepend() const
{
    return GetCurrentLabel()->GetDepend();
}

void CircuitBuilder::SetDepend(GateRef depend)
{
    GetCurrentLabel()->SetDepend(depend);
}

void Label::Seal()
{
    return impl_->Seal();
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

GateType Environment::GetGateType(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetGateType();
}

Label Environment::GetLabelFromSelector(GateRef sel)
{
    Label::LabelImpl *rawlabel = phiToLabels_[sel];
    return Label(rawlabel);
}

void Environment::AddSelectorToLabel(GateRef sel, Label label)
{
    phiToLabels_[sel] = label.GetRawLabel();
}

Label::LabelImpl *Environment::NewLabel(Environment *env, GateRef control)
{
    auto impl = new Label::LabelImpl(env, control);
    rawLabels_.emplace_back(impl);
    return impl;
}

void Environment::SubCfgEntry(Label *entry)
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

void Environment::SubCfgExit()
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

GateRef Environment::GetInput(size_t index) const
{
    return inputList_.at(index);
}
} // namespace panda::ecmascript::kungfu

#endif
