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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_HELPER_INL_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_HELPER_INL_H

#include "ecmascript/compiler/circuit_builder_helper.h"

namespace panda::ecmascript::kungfu {
using LabelImpl = Label::LabelImpl;

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

TypeCode LabelManager::GetTypeCode(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetTypeCode();
}

Label LabelManager::GetLabelFromSelector(GateRef sel)
{
    LabelImpl *rawlabel = phiToLabels_[sel];
    return Label(rawlabel);
}

void LabelManager::AddSelectorToLabel(GateRef sel, Label label)
{
    phiToLabels_[sel] = label.GetRawLabel();
}

LabelImpl *LabelManager::NewLabel(LabelManager *lm, GateRef control)
{
    auto impl = new LabelImpl(lm, control);
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
} // panda::ecmascript::kungfu

#endif