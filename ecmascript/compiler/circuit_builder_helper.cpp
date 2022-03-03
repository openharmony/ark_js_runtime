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

#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/circuit_builder_helper-inl.h"

namespace panda::ecmascript::kungfu {
using LabelImpl = Label::LabelImpl;

LabelManager::LabelManager(GateRef hir, Circuit *circuit) : circuit_(circuit), builder_(circuit)
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
    : circuit_(circuit), builder_(circuit)
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

void LabelImpl::Seal()
{
    for (auto &[variable, gate] : incompletePhis_) {
        variable->AddPhiOperand(gate);
    }
    isSealed_ = true;
}

void LabelImpl::WriteVariable(Variable *var, GateRef value)
{
    valueMap_[var] = value;
}

GateRef LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        if (!lm_->GetCircuit()->GetOpCode(result).IsNop()) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    MachineType MachineType = CircuitBuilder::GetMachineTypeFromVariableType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;
        if (MachineType == MachineType::NOVALUE) {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), MachineType, predeControl_,
                                                           valueCounts, var->Type());
        } else {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::VALUE_SELECTOR), MachineType, predeControl_,
                                                           valueCounts, var->Type());
        }
        lm_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (MachineType == MachineType::NOVALUE) {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), MachineType, predeControl_,
                                                           this->predecessors_.size(), var->Type());
        } else {
            val = lm_->GetCircuitBuilder()->NewSelectorGate(OpCode(OpCode::VALUE_SELECTOR), MachineType, predeControl_,
                                                           this->predecessors_.size(), var->Type());
        }
        lm_->AddSelectorToLabel(val, Label(this));
        WriteVariable(var, val);
        val = var->AddPhiOperand(val);
    }
    WriteVariable(var, val);
    return val;
}

void LabelImpl::Bind()
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

void LabelImpl::MergeAllControl()
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

void LabelImpl::MergeAllDepend()
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

void LabelImpl::AppendPredecessor(LabelImpl *predecessor)
{
    if (predecessor != nullptr) {
        predecessors_.push_back(predecessor);
    }
}

bool LabelImpl::IsNeedSeal() const
{
    auto control = lm_->GetCircuit()->LoadGatePtr(predeControl_);
    auto stateCount = control->GetOpCode().GetStateCount(control->GetBitField());
    return predecessors_.size() >= stateCount;
}

bool LabelImpl::IsLoopHead() const
{
    return lm_->GetCircuit()->IsLoopHead(predeControl_);
}

bool LabelImpl::IsControlCase() const
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
}