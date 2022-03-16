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
#include "ecmascript/compiler/fast_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "include/coretypes/tagged_value.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::kungfu {
using TaggedValue = panda::coretypes::TaggedValue;
GateRef CircuitBuilder::NewArguments(size_t index)
{
    auto argListOfCircuit = Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST));
    return circuit_->NewGate(OpCode(OpCode::ARG), MachineType::I64, index, {argListOfCircuit}, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewMerge(GateRef *inList, size_t controlCount)
{
    return circuit_->NewGate(OpCode(OpCode::MERGE), controlCount, controlCount, inList, GateType::EMPTY);
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opCode, GateRef control, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(Circuit::NullGate());
    }

    return circuit_->NewGate(opCode, valueCounts, inList, VariableType2GateType(type));
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opCode, GateRef control, std::vector<GateRef> &values,
                                        int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(values[i]);
    }

    return circuit_->NewGate(opCode, valueCounts, inList, VariableType2GateType(type));
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opcode, MachineType machineType, GateRef control, int valueCounts,
                                        VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(Circuit::NullGate());
    }

    return circuit_->NewGate(opcode, machineType, valueCounts, inList, VariableType2GateType(type));
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opcode, MachineType machineType, GateRef control,
                                        std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(values[i]);
    }

    return circuit_->NewGate(opcode, machineType, valueCounts, inList, VariableType2GateType(type));
}

GateRef CircuitBuilder::NewInt8Constant(int8_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I8, val, {constantList}, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewInt16Constant(int16_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I16, val, {constantList}, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewIntegerConstant(int32_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I32, val, {constantList}, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewInteger64Constant(int64_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, val, {constantList}, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewRelocatableData(uint64_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::RELOCATABLE_DATA), val, {constantList}, GateType::EMPTY);
}

GateRef CircuitBuilder::NewBooleanConstant(bool val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I32, val ? 1 : 0, {constantList},
                             GateType::C_VALUE);
}

GateRef CircuitBuilder::NewDoubleConstant(double val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::F64, bit_cast<int64_t>(val), {constantList},
                             GateType::C_VALUE);
}

GateRef CircuitBuilder::UndefineConstant(GateType type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, TaggedValue::VALUE_UNDEFINED,
                             { constantList }, type);
}

GateRef CircuitBuilder::HoleConstant(GateType type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, TaggedValue::VALUE_HOLE, { constantList },
                             type);
}

GateRef CircuitBuilder::NullConstant(GateType type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, TaggedValue::VALUE_NULL, { constantList },
                             type);
}

GateRef CircuitBuilder::ExceptionConstant(GateType type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, TaggedValue::VALUE_EXCEPTION,
                             { constantList }, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition)
{
    return circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, { state, condition }, GateType::EMPTY);
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_BRANCH), caseCounts, { state, index }, GateType::EMPTY);
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN), 0, { state, depend, value, returnList }, GateType::EMPTY);
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN_VOID), 0, { state, depend, returnList }, GateType::EMPTY);
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0, { state }, GateType::EMPTY);
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(OpCode(OpCode::LOOP_BEGIN), 0, { state, nullGate }, GateType::EMPTY);
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::LOOP_BACK), 0, { state }, GateType::EMPTY);
}

GateRef CircuitBuilder::NewIfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, { ifBranch }, GateType::EMPTY);
}

GateRef CircuitBuilder::NewIfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, { ifBranch }, GateType::EMPTY);
}

GateRef CircuitBuilder::NewSwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_CASE), value, { switchBranch }, GateType::EMPTY);
}

GateRef CircuitBuilder::NewDefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(OpCode(OpCode::DEFAULT_CASE), 0, { switchBranch }, GateType::EMPTY);
}

MachineType CircuitBuilder::GetStoreMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

MachineType CircuitBuilder::GetLoadMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

MachineType CircuitBuilder::GetMachineTypeFromVariableType(VariableType stubType)
{
    return stubType.GetMachineType();
}

GateRef CircuitBuilder::NewDependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::DEPEND_RELAY), 0, { state, depend }, GateType::EMPTY);
}

GateRef CircuitBuilder::NewDependAnd(std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    return circuit_->NewGate(OpCode(OpCode::DEPEND_AND), args.size(), inputs, GateType::EMPTY);
}

GateRef CircuitBuilder::NewLoadGate(VariableType type, GateRef val, GateRef depend)
{
    MachineType machineType = GetLoadMachineTypeFromVariableType(type);
    return circuit_->NewGate(OpCode(OpCode::LOAD), machineType, 0, { depend, val },
                             VariableType2GateType(type));
}

GateRef CircuitBuilder::NewStoreGate(VariableType type, GateRef ptr, GateRef val, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::STORE), 0, { depend, val, ptr }, VariableType2GateType(type));
}

GateRef CircuitBuilder::NewArithmeticGate(OpCode opcode, MachineType machineType, GateRef left, GateRef right)
{
    GateType type = circuit_->LoadGatePtr(left)->GetGateType();
    return circuit_->NewGate(opcode, machineType, 0, { left, right }, type);
}

GateRef CircuitBuilder::NewNumberGate(OpCode opcode, GateRef value)
{
    return circuit_->NewGate(opcode, 0, { value }, GateType::TAGGED_VALUE);
}

GateRef CircuitBuilder::NewArithmeticGate(OpCode opcode, MachineType machineType, GateRef value)
{
    return circuit_->NewGate(opcode, machineType, 0, { value }, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewArithmeticGate(OpCode opcode, GateRef value)
{
    return circuit_->NewGate(opcode, 0, { value }, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, MachineType machineType, GateRef left, GateRef right)
{
    return circuit_->NewGate(opcode, machineType, 0, { left, right },
                             GateType::C_VALUE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, GateRef left, GateRef right)
{
    return circuit_->NewGate(opcode, 0, { left, right }, GateType::C_VALUE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, MachineType machineType, GateRef value)
{
    return circuit_->NewGate(opcode, machineType, 0, { value }, GateType::C_VALUE);
}

MachineType CircuitBuilder::GetCallMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

GateRef CircuitBuilder::NewCallGate(const CallSignature *descriptor, GateRef glue, GateRef target,
                                    std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    // 2 means extra two input gates (target glue)
    const size_t extraparamCnt = 2;
    auto dependEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    inputs.push_back(dependEntry);
    inputs.push_back(target);
    inputs.push_back(glue);
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    MachineType machineType = GetCallMachineTypeFromVariableType(descriptor->GetReturnType());
    GateType type = VariableType2GateType(descriptor->GetReturnType());
    return circuit_->NewGate(OpCode(OpCode::CALL), machineType, args.size() + extraparamCnt, inputs, type);
}

GateRef CircuitBuilder::NewCallGate(const CallSignature *descriptor, GateRef glue, GateRef target,
                                    GateRef depend, std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    inputs.push_back(depend);
    inputs.push_back(target);
    inputs.push_back(glue);
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    MachineType machineType = GetCallMachineTypeFromVariableType(descriptor->GetReturnType());
    GateType type = VariableType2GateType(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(OpCode(OpCode::CALL), machineType, args.size() + 2, inputs, type);
}

GateRef CircuitBuilder::NewRuntimeCallGate(GateRef glue, GateRef target,
                                           GateRef depend, std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    inputs.push_back(depend);
    inputs.push_back(target);
    inputs.push_back(glue);
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    OpCode opcode(OpCode::RUNTIME_CALL);
    const CallSignature *descriptor = RuntimeStubCSigns::Get(RTSTUB_ID(RuntimeCallTrampolineAot));
    MachineType machineType = GetCallMachineTypeFromVariableType(descriptor->GetReturnType());
    GateType type = VariableType2GateType(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(opcode, machineType, args.size() + 2, inputs, type);
}

GateRef CircuitBuilder::CallRuntimeVariadic(GateRef glue, GateRef target, GateRef depend,
                                            const std::vector<GateRef> &args)
{
    std::vector<GateRef> inputs {depend, target, glue};
    inputs.insert(inputs.end(), args.begin(), args.end());
    OpCode opcode(OpCode::RUNTIME_CALL);
    const CallSignature *descriptor = RuntimeStubCSigns::Get(RTSTUB_ID(RuntimeCallTrampolineAot));
    MachineType machineType = GetCallMachineTypeFromVariableType(descriptor->GetReturnType());
    GateType type = VariableType2GateType(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    static constexpr size_t extraparamCnt = 2;
    return circuit_->NewGate(opcode, machineType, args.size() + extraparamCnt, inputs, type);
}

GateRef CircuitBuilder::NewBytecodeCallGate(const CallSignature *descriptor, GateRef glue, GateRef target,
                                            GateRef depend, std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    inputs.push_back(depend);
    inputs.push_back(target);
    inputs.push_back(glue);
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    OpCode opcode(OpCode::BYTECODE_CALL);
    MachineType machineType = GetCallMachineTypeFromVariableType(descriptor->GetReturnType());
    GateType type = VariableType2GateType(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(opcode, machineType, args.size() + 2, inputs, type);
}

GateRef CircuitBuilder::Alloca(int size)
{
    auto allocaList = Circuit::GetCircuitRoot(OpCode(OpCode::ALLOCA_LIST));
    return circuit_->NewGate(OpCode(OpCode::ALLOCA), size, { allocaList }, GateType::C_VALUE);
}

LabelManager::LabelManager(GateRef hir, Circuit *circuit) : circuit_(circuit), builder_(circuit, this)
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
    : circuit_(circuit), builder_(circuit, this)
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
    auto jump = builder_.Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    SetCurrentLabel(nullptr);
}

void LabelManager::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = builder_.Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = builder_.NewIfTrue(ifBranch);
    trueLabel->AppendPredecessor(GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = builder_.NewIfFalse(ifBranch);
    falseLabel->AppendPredecessor(GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    SetCurrentLabel(nullptr);
}

void LabelManager::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = builder_.SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = builder_.NewSwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = builder_.NewDefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    SetCurrentLabel(nullptr);
}

void LabelManager::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = builder_.LoopBegin(loopHead->GetControl());
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
    auto loopend = builder_.LoopEnd(currentControl);
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
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR),
                                                            MachineType, predeControl_,
                                                            valueCounts, var->Type());
        } else {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::VALUE_SELECTOR),
                                                            MachineType, predeControl_,
                                                            valueCounts, var->Type());
        }
        lm_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (MachineType == MachineType::NOVALUE) {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), MachineType,
                                                            predeControl_, this->predecessors_.size(),
                                                            var->Type());
        } else {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::VALUE_SELECTOR), MachineType,
                                                            predeControl_, this->predecessors_.size(),
                                                            var->Type());
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
        loopDepend_ = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), predeControl_, 2);
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

    GateRef merge = lm_->GetCircuitBuilder()->NewMerge(inGates.data(), inGates.size());
    predeControl_ = merge;
    control_ = merge;
}

void Label::LabelImpl::MergeAllDepend()
{
    if (IsControlCase()) {
        // Add depend_relay to current label
        auto denpendEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        dependRelay_ = lm_->GetCircuitBuilder()->NewDependRelay(predeControl_, denpendEntry);
    }

    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (dependRelay_ != -1) {
            depend_ = lm_->GetCircuitBuilder()->NewDependAnd({ depend_, dependRelay_ });
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
    depend_ = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), predeControl_,
                                                        dependsList, dependsList.size());
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
        same = lm_->GetCircuit()->LoadGatePtr(lm_->GetCircuitBuilder()->UndefineConstant(type));
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
