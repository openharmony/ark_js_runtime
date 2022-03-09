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
GateRef CircuitBuilder::CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target,
    std::initializer_list<GateRef> args)
{
    auto label = lm_->GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = NewCallGate(descriptor, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target, GateRef depend,
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

GateRef CircuitBuilder::IntPtrAdd(GateRef x, GateRef y)
{
    return NewArithmeticGate(OpCode(OpCode::ADD), MachineType::ARCH, x, y);
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
    GateRef control = currentLabel_->GetControl();
    GateRef depend = currentLabel_->GetDepend();
    if (currentLabel_ != nullptr) {
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
} // namespace panda::ecmascript::kungfu

#endif