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

GateRef CircuitBuilder::NewCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
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

GateRef CircuitBuilder::NewCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
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
    StubDescriptor *descriptor = GET_STUBDESCRIPTOR(CallRuntimeTrampoline);
    MachineType machineType = GetCallMachineTypeFromVariableType(descriptor->GetReturnType());
    GateType type = VariableType2GateType(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(opcode, machineType, args.size() + 2, inputs, type);
}

GateRef CircuitBuilder::NewBytecodeCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
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
}  // namespace panda::ecmascript::kungfu
