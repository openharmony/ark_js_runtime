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

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_function.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "include/coretypes/tagged_value.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::kungfu {
using TaggedValue = panda::coretypes::TaggedValue;
#define DEF_CALL_GATE(OpName, CallSignature)                                      \
    std::vector<GateRef> inputs;                                                  \
    inputs.push_back(depend);                                                     \
    inputs.push_back(target);                                                     \
    inputs.push_back(glue);                                                       \
    for (auto arg : args) {                                                       \
        inputs.push_back(arg);                                                    \
    }                                                                             \
    OpCode opcode(OpName);                                                        \
    MachineType machineType = CallSignature->GetReturnType().GetMachineType();    \
    GateType type = CallSignature->GetReturnType().GetGateType();                 \
    return GetCircuit()->NewGate(opcode, machineType, args.size() + 2, inputs, type)

CircuitBuilder::~CircuitBuilder()
{
    if (lm_ != nullptr) {
        delete lm_;
        lm_ = nullptr;
    }
}

GateRef CircuitBuilder::Arguments(size_t index)
{
    auto argListOfCircuit = Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::ARG), MachineType::I64, index, {argListOfCircuit}, GateType::C_VALUE);
}

GateRef CircuitBuilder::Merge(GateRef *inList, size_t controlCount)
{
    return lowBuilder_.Merge(inList, controlCount);
}

GateRef CircuitBuilder::Selector(OpCode opcode, MachineType machineType, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    return lowBuilder_.Selector(opcode, machineType, control, values, valueCounts, type);
}

GateRef CircuitBuilder::Selector(OpCode opcode, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    return lowBuilder_.Selector(opcode, control, values, valueCounts, type);
}

GateRef CircuitBuilder::Int8(int8_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I8, val, GateType::C_VALUE);
}

GateRef CircuitBuilder::Int16(int16_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I16, val, GateType::C_VALUE);
}

GateRef CircuitBuilder::Int32(int32_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I32, static_cast<BitField>(val), GateType::C_VALUE);
}

GateRef CircuitBuilder::Int64(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, val, GateType::C_VALUE);
}

GateRef CircuitBuilder::IntPtr(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::ARCH, val, GateType::C_VALUE);
}

GateRef CircuitBuilder::RelocatableData(uint64_t val)
{
    return GetCircuit()->NewGate(OpCode(OpCode::RELOCATABLE_DATA), val, {constantList}, GateType::EMPTY);
}

GateRef CircuitBuilder::Boolean(bool val)
{
    return GetCircuit()->GetConstantGate(MachineType::I1, val ? 1 : 0, GateType::C_VALUE);
}

GateRef CircuitBuilder::Double(double val)
{
    return GetCircuit()->GetConstantGate(MachineType::F64, bit_cast<int64_t>(val), GateType::C_VALUE);
}

GateRef CircuitBuilder::UndefineConstant(GateType type)
{
    return lowBuilder_.UndefineConstant(type);
}

GateRef CircuitBuilder::HoleConstant(GateType type)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, TaggedValue::VALUE_HOLE, type);
}

GateRef CircuitBuilder::NullConstant(GateType type)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, TaggedValue::VALUE_NULL, type);
}

GateRef CircuitBuilder::ExceptionConstant(GateType type)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, TaggedValue::VALUE_EXCEPTION, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition)
{
    return lowBuilder_.Branch(state, condition);
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return lowBuilder_.SwitchBranch(state, index, caseCounts);
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    return lowBuilder_.Return(state, depend, value);
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    return lowBuilder_.ReturnVoid(state, depend);
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return lowBuilder_.Goto(state);
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    return lowBuilder_.LoopBegin(state);
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return lowBuilder_.LoopEnd(state);
}

GateRef CircuitBuilder::IfTrue(GateRef ifBranch)
{
    return lowBuilder_.IfTrue(ifBranch);
}

GateRef CircuitBuilder::IfFalse(GateRef ifBranch)
{
    return lowBuilder_.IfFalse(ifBranch);
}

GateRef CircuitBuilder::SwitchCase(GateRef switchBranch, int64_t value)
{
    return lowBuilder_.SwitchCase(switchBranch, value);
}

GateRef CircuitBuilder::DefaultCase(GateRef switchBranch)
{
    return lowBuilder_.DefaultCase(switchBranch);
}

MachineType CircuitBuilder::GetMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

GateRef CircuitBuilder::DependRelay(GateRef state, GateRef depend)
{
    return lowBuilder_.DependRelay(state, depend);
}

GateRef CircuitBuilder::DependAnd(std::initializer_list<GateRef> args)
{
    return lowBuilder_.DependAnd(args);
}

GateRef CircuitBuilder::BinaryArithmetic(OpCode opcode, MachineType machineType, GateRef left, GateRef right)
{
    auto circuit = GetCircuit();
    GateType type = circuit->LoadGatePtr(left)->GetGateType();
    return circuit->NewGate(opcode, machineType, 0, { left, right }, type);
}

GateRef CircuitBuilder::TaggedNumber(OpCode opcode, GateRef value)
{
    return lowBuilder_.GetCircuit()->NewGate(opcode, 0, { value }, GateType::TAGGED_VALUE);
}

GateRef CircuitBuilder::UnaryArithmetic(OpCode opcode, MachineType machineType, GateRef value)
{
    return lowBuilder_.GetCircuit()->NewGate(opcode, machineType, 0, { value }, GateType::C_VALUE);
}

GateRef CircuitBuilder::UnaryArithmetic(OpCode opcode, GateRef value)
{
    return lowBuilder_.GetCircuit()->NewGate(opcode, 0, { value }, GateType::C_VALUE);
}

GateRef CircuitBuilder::BinaryLogic(OpCode opcode, GateRef left, GateRef right)
{
    return lowBuilder_.GetCircuit()->NewGate(opcode, 0, { left, right }, GateType::C_VALUE);
}

GateRef CircuitBuilder::Call(const CallSignature *signature, GateRef glue, GateRef target,
                             const std::vector<GateRef> &args, GateRef depend)
{
    DEF_CALL_GATE(OpCode::CALL, signature);
}

GateRef CircuitBuilder::RuntimeCall(GateRef glue, GateRef target,
                                    GateRef depend, const std::vector<GateRef> &args)
{
    const CallSignature *signature = RuntimeStubCSigns::Get(RTSTUB_ID(OptimizedCallRuntime));
    DEF_CALL_GATE(OpCode::RUNTIME_CALL, signature);
}

GateRef CircuitBuilder::NoGcRuntimeCall(const CallSignature *signature, GateRef glue, GateRef target,
                                        GateRef depend, const std::vector<GateRef> &args)
{
    DEF_CALL_GATE(OpCode::NOGC_RUNTIME_CALL, signature);
}

GateRef CircuitBuilder::BytecodeCall(const CallSignature *signature, GateRef glue, GateRef target,
                                     GateRef depend, const std::vector<GateRef> &args)
{
    DEF_CALL_GATE(OpCode::BYTECODE_CALL, signature);
}

GateRef CircuitBuilder::VariadicRuntimeCall(GateRef glue, GateRef target, GateRef depend,
                                            const std::vector<GateRef> &args)
{
    std::vector<GateRef> inputs {depend, target, glue};
    inputs.insert(inputs.end(), args.begin(), args.end());
    OpCode opcode(OpCode::RUNTIME_CALL);
    const CallSignature *signature = RuntimeStubCSigns::Get(RTSTUB_ID(OptimizedCallRuntime));
    MachineType machineType = signature->GetReturnType().GetMachineType();
    GateType type = signature->GetReturnType().GetGateType();
    // 2 : 2 means extra two input gates (target glue)
    constexpr size_t extraparamCnt = 2;
    return GetCircuit()->NewGate(opcode, machineType, args.size() + extraparamCnt, inputs, type);
}

// call operation
GateRef CircuitBuilder::CallRuntime(GateRef glue, int index,
    const std::vector<GateRef> &args)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef target = Int64(index);
    GateRef result = RuntimeCall(glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, size_t index,
    const std::vector<GateRef> &args)
{
    const CallSignature *signature = RuntimeStubCSigns::Get(index);
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = NoGcRuntimeCall(signature, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallStub(GateRef glue, size_t index,
    const std::vector<GateRef> &args)
{
    const CallSignature *signature = CommonStubCSigns::Get(index);
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(signature, glue, target, args, depend);
    label->SetDepend(result);
    return result;
}

// memory
void CircuitBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef ptr = IntPtrAdd(base, offset);
    GateRef result = GetCircuit()->NewGate(OpCode(OpCode::STORE), 0, { depend, value, ptr }, type.GetGateType());
    label->SetDepend(result);
    if (type == VariableType::JS_POINTER() || type == VariableType::JS_ANY()) {
        CallStub(glue, CommonStubCSigns::SetValueWithBarrier, {glue, base, offset, value});
    }
    return;
}

GateRef CircuitBuilder::Alloca(int size)
{
    auto allocaList = Circuit::GetCircuitRoot(OpCode(OpCode::ALLOCA_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::ALLOCA), size, { allocaList }, GateType::C_VALUE);
}

GateRef CircuitBuilder::TaggedIsString(GateRef obj)
{
    Label entry(lm_);
    lm_->PushCurrentLabel(&entry);
    Label exit(lm_);
    DEFVAlUE(result, lm_, VariableType::BOOL(), False());
    Label isHeapObject(lm_);
    lm_->Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    lm_->Bind(&isHeapObject);
    {
        result = Equal(GetObjectType(LoadHClass(obj)),
            Int32(static_cast<int32_t>(JSType::STRING)));
        lm_->Jump(&exit);
    }
    lm_->Bind(&exit);
    auto ret = *result;
    lm_->PopCurrentLabel();
    return ret;
}

GateRef CircuitBuilder::TaggedIsStringOrSymbol(GateRef obj)
{
    Label entry(lm_);
    lm_->PushCurrentLabel(&entry);
    Label exit(lm_);
    DEFVAlUE(result, lm_, VariableType::BOOL(), False());
    Label isHeapObject(lm_);
    lm_->Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    lm_->Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Equal(objType, Int32(static_cast<int32_t>(JSType::STRING)));
        Label isString(lm_);
        Label notString(lm_);
        lm_->Branch(*result, &exit, &notString);
        lm_->Bind(&notString);
        {
            result = Equal(objType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
            lm_->Jump(&exit);
        }
    }
    lm_->Bind(&exit);
    auto ret = *result;
    lm_->PopCurrentLabel();
    return ret;
}

GateRef CircuitBuilder::GetFunctionBitFieldFromJSFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::BIT_FIELD_OFFSET);
    return Load(VariableType::INT32(), function, offset);
}

GateRef CircuitBuilder::GetModuleFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::FunctionIsResolved(GateRef function)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
    return NotEqual(Int32And(UInt32LSR(bitfield, Int32(JSFunction::ResolvedBits::START_BIT)),
        Int32((1LU << JSFunction::ResolvedBits::SIZE) - 1)), Int32(0));
}

void CircuitBuilder::SetResolvedToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
    GateRef mask = Int32(~(((1<<JSFunction::ResolvedBits::SIZE) - 1) << JSFunction::ResolvedBits::START_BIT));
    GateRef result = Int32Or(Int32And(bitfield, mask),
        Int32LSL(ZExtInt1ToInt32(value), Int32(JSFunction::ResolvedBits::START_BIT)));
    Store(VariableType::INT32(), glue, function, IntPtr(JSFunction::BIT_FIELD_OFFSET), result);
}

void CircuitBuilder::SetConstPoolToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::CONSTANT_POOL_OFFSET);
    Store(VariableType::INT64(), glue, function, offset, value);
}

void CircuitBuilder::SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

void CircuitBuilder::SetModuleToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value);
}

void CircuitBuilder::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset, VariableType type)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Int32And(UInt32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(Int32Add(inlinedPropsStart, attrOffset),
        Int32(JSTaggedValue::TaggedTypeSize()));
    Store(type, glue, obj, ChangeInt32ToIntPtr(propOffset), value);
}

void CircuitBuilder::NewLabelManager(GateRef hir)
{
    if (lm_ == nullptr) {
        lm_ = new LabelManager(hir, GetCircuit());
    } else {
        delete lm_;
        lm_ = new LabelManager(hir, GetCircuit());
    }
}

void CircuitBuilder::DeleteCurrentLabelManager()
{
    if (lm_ != nullptr) {
        delete lm_;
        lm_ = nullptr;
    }
}

void CircuitBuilder::Jump(Label *label)
{
    ASSERT(lm_ != nullptr);
    lm_->Jump(label);
}

void CircuitBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    ASSERT(lm_ != nullptr);
    lm_->Branch(condition, trueLabel, falseLabel);
}

void CircuitBuilder::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue,
                            Label *keysLabel, int numberOfKeys)
{
    ASSERT(lm_ != nullptr);
    lm_->Switch(index, defaultLabel, keysValue, keysLabel, numberOfKeys);
}

void CircuitBuilder::LoopBegin(Label *loopHead)
{
    ASSERT(lm_ != nullptr);
    lm_->LoopBegin(loopHead);
}

void CircuitBuilder::LoopEnd(Label *loopHead)
{
    ASSERT(lm_ != nullptr);
    lm_->LoopEnd(loopHead);
}

LabelManager::LabelManager(GateRef hir, Circuit *circuit)
    : circuit_(circuit), lBuilder_(circuit)
{
    auto hirGate = circuit_->LoadGatePtr(hir);
    entry_ = Label(NewLabel(this, circuit_->SaveGatePtr(hirGate->GetInGate(0))));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto dependEntry = circuit_->SaveGatePtr(hirGate->GetInGate(1));
    currentLabel_->SetDepend(dependEntry);
    for (size_t i = 2; i < hirGate->GetNumIns(); i++) {
        inputList_.emplace_back(circuit_->SaveGatePtr(hirGate->GetInGate(i)));
    }
}

LabelManager::LabelManager(GateRef stateEntry, GateRef dependEntry, std::vector<GateRef>& inlist, Circuit *circuit)
    : circuit_(circuit), lBuilder_(circuit)
{
    entry_ = Label(NewLabel(this, stateEntry));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    currentLabel_->SetDepend(dependEntry);
    for (auto in : inlist) {
        inputList_.emplace_back(in);
    }
}

LabelManager::~LabelManager()
{
    for (auto label : rawLabels_) {
        delete label;
    }
}

void LabelManager::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = lBuilder_.Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    SetCurrentLabel(nullptr);
}

void LabelManager::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = lBuilder_.Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = lBuilder_.IfTrue(ifBranch);
    trueLabel->AppendPredecessor(GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = lBuilder_.IfFalse(ifBranch);
    falseLabel->AppendPredecessor(GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    SetCurrentLabel(nullptr);
}

void LabelManager::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = lBuilder_.SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = lBuilder_.SwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = lBuilder_.DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    SetCurrentLabel(nullptr);
}

void LabelManager::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = lBuilder_.LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    SetCurrentLabel(loopHead);
}

void LabelManager::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = lBuilder_.LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    SetCurrentLabel(nullptr);
}

Label::Label(LabelManager *lm)
{
    impl_ = lm->NewLabel(lm);
}

Label::Label(CircuitBuilder *cirBuilder)
{
    auto lm = cirBuilder->GetCurrentLabelManager();
    impl_ = lm->NewLabel(lm);
}

void Label::LabelImpl::Seal()
{
    for (auto &[variable, gate] : incompletePhis_) {
        variable->AddPhiOperand(gate);
    }
    isSealed_ = true;
}

void Label::LabelImpl::WriteVariable(Variable *var, GateRef value)
{
    valueMap_[var] = value;
}

GateRef Label::LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        if (!lm_->GetCircuit()->GetOpCode(result).IsNop()) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef Label::LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    MachineType MachineType = CircuitBuilder::GetMachineTypeFromVariableType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;
        if (MachineType == MachineType::NOVALUE) {
            val = lm_->GetLCircuitBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
                predeControl_, {}, valueCounts, var->Type());
        } else {
            val = lm_->GetLCircuitBuilder()->Selector(OpCode(OpCode::VALUE_SELECTOR),
                MachineType, predeControl_, {}, valueCounts, var->Type());
        }
        lm_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (MachineType == MachineType::NOVALUE) {
            val = lm_->GetLCircuitBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
                predeControl_, {}, this->predecessors_.size(), var->Type());
        } else {
            val = lm_->GetLCircuitBuilder()->Selector(OpCode(OpCode::VALUE_SELECTOR), MachineType,
                predeControl_, {}, this->predecessors_.size(), var->Type());
        }
        lm_->AddSelectorToLabel(val, Label(this));
        WriteVariable(var, val);
        val = var->AddPhiOperand(val);
    }
    WriteVariable(var, val);
    return val;
}

void Label::LabelImpl::Bind()
{
    ASSERT(!predecessors_.empty());
    if (IsLoopHead()) {
        // 2 means input number of depend selector gate
        loopDepend_ = lm_->GetLCircuitBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR), predeControl_, {}, 2);
        lm_->GetCircuit()->NewIn(loopDepend_, 1, predecessors_[0]->GetDepend());
        depend_ = loopDepend_;
    }
    if (IsNeedSeal()) {
        Seal();
        MergeAllControl();
        MergeAllDepend();
    }
}

void Label::LabelImpl::MergeAllControl()
{
    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        return;
    }

    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        ASSERT(otherPredeControls_.size() == 1);
        lm_->GetCircuit()->NewIn(predeControl_, 1, otherPredeControls_[0]);
        return;
    }

    // merge all control of predecessors_
    std::vector<GateRef> inGates(predecessors_.size());
    size_t i = 0;
    ASSERT(predeControl_ != -1);
    ASSERT((otherPredeControls_.size() + 1) == predecessors_.size());
    inGates[i++] = predeControl_;
    for (auto in : otherPredeControls_) {
        inGates[i++] = in;
    }

    GateRef merge = lm_->GetLCircuitBuilder()->Merge(inGates.data(), inGates.size());
    predeControl_ = merge;
    control_ = merge;
}

void Label::LabelImpl::MergeAllDepend()
{
    if (IsControlCase()) {
        // Add depend_relay to current label
        auto denpendEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        dependRelay_ = lm_->GetLCircuitBuilder()->DependRelay(predeControl_, denpendEntry);
    }

    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (dependRelay_ != -1) {
            depend_ = lm_->GetLCircuitBuilder()->DependAnd({ depend_, dependRelay_ });
        }
        return;
    }
    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        // Add loop depend to in of depend_seclector
        ASSERT(loopDepend_ != -1);
        // 2 mean 3rd input gate for loopDepend_(depend_selector)
        lm_->GetCircuit()->NewIn(loopDepend_, 2, predecessors_[1]->GetDepend());
        return;
    }

    //  Merge all depends to depend_seclector
    std::vector<GateRef> dependsList;
    for (auto prede : this->GetPredecessors()) {
        dependsList.push_back(prede->GetDepend());
    }
    depend_ = lm_->GetLCircuitBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
        predeControl_, dependsList, dependsList.size());
}

void Label::LabelImpl::AppendPredecessor(Label::LabelImpl *predecessor)
{
    if (predecessor != nullptr) {
        predecessors_.push_back(predecessor);
    }
}

bool Label::LabelImpl::IsNeedSeal() const
{
    auto control = lm_->GetCircuit()->LoadGatePtr(predeControl_);
    auto stateCount = control->GetOpCode().GetStateCount(control->GetBitField());
    return predecessors_.size() >= stateCount;
}

bool Label::LabelImpl::IsLoopHead() const
{
    return lm_->GetCircuit()->IsLoopHead(predeControl_);
}

bool Label::LabelImpl::IsControlCase() const
{
    return lm_->GetCircuit()->IsControlCase(predeControl_);
}

GateRef Variable::AddPhiOperand(GateRef val)
{
    ASSERT(IsSelector(val));
    Label label = lm_->GetLabelFromSelector(val);
    size_t idx = 0;
    for (auto pred : label.GetPredecessors()) {
        auto preVal = pred.ReadVariable(this);
        ASSERT(!lm_->GetCircuit()->GetOpCode(preVal).IsNop());
        idx++;
        val = AddOperandToSelector(val, idx, preVal);
    }
    return TryRemoveTrivialPhi(val);
}

GateRef Variable::AddOperandToSelector(GateRef val, size_t idx, GateRef in)
{
    lm_->GetCircuit()->NewIn(val, idx, in);
    return val;
}

GateRef Variable::TryRemoveTrivialPhi(GateRef phiVal)
{
    Gate *phi = lm_->GetCircuit()->LoadGatePtr(phiVal);
    Gate *same = nullptr;
    for (size_t i = 1; i < phi->GetNumIns(); ++i) {
        In *phiIn = phi->GetIn(i);
        Gate *op = (!phiIn->IsGateNull()) ? phiIn->GetGate() : nullptr;
        if (op == same || op == phi) {
            continue;  // unique value or self-reference
        }
        if (same != nullptr) {
            return phiVal;  // the phi merges at least two valusses: not trivial
        }
        same = op;
    }
    if (same == nullptr) {
        // the phi is unreachable or in the start block
        GateType type = lm_->GetCircuit()->GetGateType(phiVal);
        same = lm_->GetCircuit()->LoadGatePtr(lm_->GetLCircuitBuilder()->UndefineConstant(type));
    }
    auto same_addr_shift = lm_->GetCircuit()->SaveGatePtr(same);

    // remove the trivial phi
    // get all users of phi except self
    std::vector<Out *> outs;
    if (!phi->IsFirstOutNull()) {
        Out *phiOut = phi->GetFirstOut();
        while (!phiOut->IsNextOutNull()) {
            if (phiOut->GetGate() != phi) {
                // remove phi
                outs.push_back(phiOut);
            }
            phiOut = phiOut->GetNextOut();
        }
        // save last phi out
        if (phiOut->GetGate() != phi) {
            outs.push_back(phiOut);
        }
    }
    // reroute all outs of phi to same and remove phi
    RerouteOuts(outs, same);
    phi->DeleteGate();

    // try to recursiveby remove all phi users, which might have vecome trivial
    for (auto out : outs) {
        if (IsSelector(out->GetGate())) {
            auto out_addr_shift = lm_->GetCircuit()->SaveGatePtr(out->GetGate());
            auto result = TryRemoveTrivialPhi(out_addr_shift);
            if (same_addr_shift == out_addr_shift) {
                same_addr_shift = result;
            }
        }
    }
    return same_addr_shift;
}

void Variable::RerouteOuts(const std::vector<Out *> &outs, Gate *newGate)
{
    // reroute all outs to new node
    for (auto out : outs) {
        size_t idx = out->GetIndex();
        out->GetGate()->ModifyIn(idx, newGate);
    }
}
}  // namespace panda::ecmascript::kungfu
