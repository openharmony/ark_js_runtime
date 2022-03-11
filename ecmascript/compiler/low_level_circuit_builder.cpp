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

#include "ecmascript/compiler/low_level_circuit_builder.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "include/coretypes/tagged_value.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::kungfu {
using TaggedValue = panda::coretypes::TaggedValue;
GateRef LCircuitBuilder::Merge(GateRef *inList, size_t controlCount)
{
    return circuit_->NewGate(OpCode(OpCode::MERGE), controlCount, controlCount, inList, GateType::EMPTY);
}

GateRef LCircuitBuilder::Selector(OpCode opcode, MachineType machineType, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    return circuit_->NewGate(opcode, machineType, valueCounts, inList, type.GetGateType());
}

GateRef LCircuitBuilder::Selector(OpCode opcode, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    return circuit_->NewGate(opcode, valueCounts, inList, type.GetGateType());
}

GateRef LCircuitBuilder::UndefineConstant(GateType type)
{
    return circuit_->GetConstantGate(MachineType::I64, TaggedValue::VALUE_UNDEFINED, type);
}

GateRef LCircuitBuilder::Branch(GateRef state, GateRef condition)
{
    return circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, { state, condition }, GateType::EMPTY);
}

GateRef LCircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_BRANCH), caseCounts, { state, index }, GateType::EMPTY);
}

GateRef LCircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN), 0, { state, depend, value, returnList }, GateType::EMPTY);
}

GateRef LCircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN_VOID), 0, { state, depend, returnList }, GateType::EMPTY);
}

GateRef LCircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0, { state }, GateType::EMPTY);
}

GateRef LCircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(OpCode(OpCode::LOOP_BEGIN), 0, { state, nullGate }, GateType::EMPTY);
}

GateRef LCircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::LOOP_BACK), 0, { state }, GateType::EMPTY);
}

GateRef LCircuitBuilder::IfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, { ifBranch }, GateType::EMPTY);
}

GateRef LCircuitBuilder::IfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, { ifBranch }, GateType::EMPTY);
}

GateRef LCircuitBuilder::SwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_CASE), value, { switchBranch }, GateType::EMPTY);
}

GateRef LCircuitBuilder::DefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(OpCode(OpCode::DEFAULT_CASE), 0, { switchBranch }, GateType::EMPTY);
}

GateRef LCircuitBuilder::DependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::DEPEND_RELAY), 0, { state, depend }, GateType::EMPTY);
}

GateRef LCircuitBuilder::DependAnd(std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    return circuit_->NewGate(OpCode(OpCode::DEPEND_AND), args.size(), inputs, GateType::EMPTY);
}
}  // namespace panda::ecmascript::kungfu