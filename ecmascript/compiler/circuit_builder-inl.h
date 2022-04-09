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
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/remembered_set.h"

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

// memory
GateRef CircuitBuilder::Load(VariableType type, GateRef base, GateRef offset)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef val = IntPtrAdd(base, offset);
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
    return Int64And(tagged, Int64(~JSTaggedValue::TAG_MASK));
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
GateRef CircuitBuilder::TaggedSpecialValueChecker(GateRef x, JSTaggedType type)
{
    return Equal(x, Int64(type));
}
GateRef CircuitBuilder::TaggedIsInt(GateRef x)
{
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MASK)),
                 Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::TaggedIsDouble(GateRef x)
{
    return BoolAnd(TaggedIsNumber(x), BoolNot(TaggedIsInt(x)));
}

GateRef CircuitBuilder::TaggedIsObject(GateRef x)
{
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MASK)),
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
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(Equal(Int64And(x,
        Int64(~JSTaggedValue::TAG_SPECIAL_MASK)),
        Int64(0))), Int32Or(SExtInt1ToInt32(NotEqual(Int64And(x,
        Int64(JSTaggedValue::TAG_SPECIAL_VALUE)),
        Int64(0))), SExtInt1ToInt32(TaggedSpecialValueChecker(x, JSTaggedValue::VALUE_HOLE)))));
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
        Int64(JSTaggedValue::TAG_WEAK_MASK)),
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
    return TruncInt32ToInt1(Int32Or(SExtInt1ToInt32(TaggedSpecialValueChecker(x, JSTaggedValue::VALUE_UNDEFINED)),
        SExtInt1ToInt32(TaggedSpecialValueChecker(x, JSTaggedValue::VALUE_NULL))));
}

GateRef CircuitBuilder::TaggedIsBoolean(GateRef x)
{
    return TruncInt32ToInt1(Int32Or(SExtInt1ToInt32(TaggedSpecialValueChecker(x, JSTaggedValue::VALUE_TRUE)),
        SExtInt1ToInt32(TaggedSpecialValueChecker(x, JSTaggedValue::VALUE_FALSE))));
}

GateRef CircuitBuilder::TaggedGetInt(GateRef x)
{
    return TruncInt64ToInt32(Int64And(x, Int64(~JSTaggedValue::TAG_MASK)));
}

GateRef CircuitBuilder::Int8BuildTaggedTypeWithNoGC(GateRef x)
{
    GateRef val = ZExtInt8ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::Int16BuildTaggedWithNoGC(GateRef x)
{
    GateRef val = ZExtInt16ToInt64(x);
    return ChangeInt64ToTagged(Int64Or(val, Int64(JSTaggedValue::TAG_INT)));
}

GateRef CircuitBuilder::Int16BuildTaggedTypeWithNoGC(GateRef x)
{
    GateRef val = ZExtInt16ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::Int64BuildTaggedNGC(GateRef x)
{
    return ChangeInt64ToTagged(Int64Or(x, Int64(JSTaggedValue::TAG_INT)));
}

GateRef CircuitBuilder::Int64BuildTaggedTypeNGC(GateRef x)
{
    return Int64Or(x, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::IntBuildTaggedWithNoGC(GateRef x)
{
    GateRef val = ZExtInt32ToInt64(x);
    return ChangeInt64ToTagged(Int64Or(val, Int64(JSTaggedValue::TAG_INT)));
}

GateRef CircuitBuilder::IntBuildTaggedTypeWithNoGC(GateRef x)
{
    GateRef val = ZExtInt32ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::DoubleBuildTaggedWithNoGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return ChangeInt64ToTagged(Int64Add(val,
        Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET)));
}

GateRef CircuitBuilder::DoubleBuildTaggedTypeWithNoGC(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef CircuitBuilder::IntBuildTagged(GateRef x)
{
    GateRef val = ZExtInt32ToInt64(x);
    GetCircuit()->SetGateType(val, GateType::TAGGED_VALUE);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::Int64BuildTagged(GateRef x)
{
    GetCircuit()->SetGateType(x, GateType::TAGGED_VALUE);
    return Int64Or(x, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::DoubleBuildTagged(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    GetCircuit()->SetGateType(val, GateType::TAGGED_VALUE);
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef CircuitBuilder::TaggedTrue()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_TRUE, GateType::C_VALUE);
}

GateRef CircuitBuilder::TaggedFalse()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_FALSE, GateType::C_VALUE);
}

GateRef CircuitBuilder::GetValueFromTaggedArray(VariableType returnType, GateRef array, GateRef index)
{
    GateRef offset =
        IntPtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = IntPtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(returnType, array, dataOffset);
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
    return NotEqual(Int32And(UInt32LSR(bitfield,
        Int32(JSHClass::IsDictionaryBit::START_BIT)),
        Int32((1LU << JSHClass::IsDictionaryBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsDictionaryElement(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(UInt32LSR(bitfield,
        Int32(JSHClass::DictionaryElementBits::START_BIT)),
        Int32((1LU << JSHClass::DictionaryElementBits::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(UInt32LSR(bitfield,
        Int32(JSHClass::ClassConstructorBit::START_BIT)),
        Int32((1LU << JSHClass::ClassConstructorBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassPrototype(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(UInt32LSR(bitfield,
        Int32(JSHClass::ClassPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::ClassPrototypeBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsExtensible(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(UInt32LSR(bitfield,
        Int32(JSHClass::ExtensibleBit::START_BIT)),
        Int32((1LU << JSHClass::ExtensibleBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsEcmaObject(GateRef obj)
{
    Label subentry(lm_);
    lm_->PushCurrentLabel(&subentry);
    Label exit(lm_);
    Label isHeapObject(lm_);
    DEFVAlUE(result, lm_, VariableType::BOOL(), False());
    lm_->Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    lm_->Bind(&isHeapObject);
    {
        GateRef objectType = GetObjectType(LoadHClass(obj));
        auto ret1 = Int32And(ZExtInt1ToInt32(Int32LessThanOrEqual(objectType,
            Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_END)))),
            ZExtInt1ToInt32(Int32GreaterThanOrEqual(objectType,
            Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_BEGIN)))));
        result = TruncInt32ToInt1(ret1);
        lm_->Jump(&exit);
    }
    lm_->Bind(&exit);
    auto ret = *result;
    lm_->PopCurrentLabel();
    return ret;
}

GateRef CircuitBuilder::IsJsObject(GateRef obj)
{
    Label subentry(lm_);
    PushCurrentLabel(&subentry);
    Label exit(lm_);
    Label isHeapObject(lm_);
    DEFVAlUE(result, lm_, VariableType::BOOL(), False());
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
    PopCurrentLabel();
    return ret;
}

GateRef CircuitBuilder::BothAreString(GateRef x, GateRef y)
{
    return TruncInt32ToInt1(Int32And(SExtInt1ToInt32(IsJsType(x, JSType::STRING)),
        SExtInt1ToInt32(IsJsType(y, JSType::STRING))));
}

int CircuitBuilder::NextVariableId()
{
    return lm_->NextVariableId();
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit, VariableType type)
{
    ASSERT(lm_ != nullptr);
    lm_->Branch(Equal(result, ExceptionConstant(type.GetGateType())), fail, success);
    lm_->Bind(fail);
    {
        lm_->Jump(exit);
    }
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal)
{
    ASSERT(lm_ != nullptr);
    lm_->Branch(Equal(result, exceptionVal), fail, success);
    lm_->Bind(fail);
    {
        lm_->Jump(exit);
    }
}

void CircuitBuilder::PushCurrentLabel(Label *entry)
{
    ASSERT(lm_ != nullptr);
    lm_->PushCurrentLabel(entry);
}

void CircuitBuilder::PopCurrentLabel()
{
    ASSERT(lm_ != nullptr);
    lm_->PopCurrentLabel();
}

GateRef CircuitBuilder::Return(GateRef value)
{
    ASSERT(lm_ != nullptr);
    return lm_->Return(value);
}

GateRef CircuitBuilder::Return()
{
    ASSERT(lm_ != nullptr);
    return lm_->Return();
}

void CircuitBuilder::Bind(Label *label)
{
    ASSERT(lm_ != nullptr);
    lm_->Bind(label);
}

void CircuitBuilder::Bind(Label *label, bool justSlowPath)
{
    ASSERT(lm_ != nullptr);
    lm_->Bind(label, justSlowPath);
}

template<bool noThrow>
void CircuitBuilder::MergeMirCircuit(GateRef hir, GateRef outir,
                                     const std::vector<GateRef> &successControl,
                                     const std::vector<GateRef> &exceptionControl)
{
    GateAccessor acc(GetCircuit());
    if (outir != Circuit::NullGate()) {
        acc.SetGateType(outir, acc.GetGateType(hir));
    }
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
            if (noThrow) {
                // 0 : the index of CONSTANT
                GetCircuit()->DeleteGate(acc.GetValueIn(*useIt, 0));
                acc.DeleteGate(useIt);
            } else {
                acc.ReplaceIn(useIt, exceptionControl[1]);
            }
        // if isThrow..
        } else if (useIt.GetIndex() == 1) {
            acc.ReplaceIn(useIt, successControl[1]);
        // replace data flow with data output in label successExit(valueSelector...)
        } else {
            acc.ReplaceIn(useIt, outir);
        }
    }

    GetCircuit()->DeleteGate(hir);
}

Label *CircuitBuilder::GetCurrentLabel() const
{
    return lm_->GetCurrentLabel();
}

GateRef CircuitBuilder::GetState() const
{
    return GetCurrentLabel()->GetControl();
}

GateRef CircuitBuilder::GetDepend() const
{
    return GetCurrentLabel()->GetDepend();
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

GateRef LabelManager::Return(GateRef value)
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return lBuilder_.Return(control, depend, value);
}

GateRef LabelManager::Return()
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return lBuilder_.ReturnVoid(control, depend);
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
} // namespace panda::ecmascript::kungfu

#endif