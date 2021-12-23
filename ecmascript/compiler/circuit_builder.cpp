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
    return circuit_->NewGate(OpCode(OpCode::JS_ARG), index, {argListOfCircuit}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewMerge(GateRef *inList, size_t controlCount)
{
    return circuit_->NewGate(OpCode(OpCode::MERGE), controlCount, controlCount, inList, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opCode, GateRef control, int valueCounts, MachineType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(Circuit::NullGate());
    }

    return circuit_->NewGate(opCode, valueCounts, inList, MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opCode, GateRef control, std::vector<GateRef> &values,
                                        int valueCounts, MachineType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(values[i]);
    }

    return circuit_->NewGate(opCode, valueCounts, inList, MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewIntegerConstant(int32_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::INT32_CONSTANT), val, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewInteger64Constant(int64_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::INT64_CONSTANT), val, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewBooleanConstant(bool val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::INT32_CONSTANT), val ? 1 : 0, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewDoubleConstant(double val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::FLOAT64_CONSTANT), bit_cast<int64_t>(val), {constantList},
                             TypeCode::NOTYPE);
}

GateRef CircuitBuilder::UndefineConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::INT64_CONSTANT), TaggedValue::VALUE_UNDEFINED, { constantList }, type);
}

GateRef CircuitBuilder::HoleConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::JS_CONSTANT), TaggedValue::VALUE_HOLE, { constantList },
                             type);
}

GateRef CircuitBuilder::NullConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::JS_CONSTANT), TaggedValue::VALUE_NULL, { constantList },
                             type);
}

GateRef CircuitBuilder::ExceptionConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::JS_CONSTANT), TaggedValue::VALUE_EXCEPTION, { constantList }, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition)
{
    return circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, { state, condition }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_BRANCH), caseCounts, { state, index }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN), 0, { state, depend, value, returnList }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN_VOID), 0, { state, depend, returnList }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0, { state }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(OpCode(OpCode::LOOP_BEGIN), 0, { state, nullGate }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::LOOP_BACK), 0, { state }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewIfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, { ifBranch }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewIfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, { ifBranch }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewSwitchCase(GateRef switchBranch, int32_t value)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_CASE), value, { switchBranch }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewDefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(OpCode(OpCode::DEFAULT_CASE), 0, { switchBranch }, TypeCode::NOTYPE);
}

OpCode CircuitBuilder::GetStoreOpCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::INT8:
            return OpCode(OpCode::INT8_STORE);
        case MachineType::INT16:
            return OpCode(OpCode::INT16_STORE);
        case MachineType::INT32:
            return OpCode(OpCode::INT32_STORE);
        case MachineType::INT64:
            return OpCode(OpCode::INT64_STORE);
        case MachineType::BOOL:
            return OpCode(OpCode::INT32_STORE);
        case MachineType::UINT8:
            return OpCode(OpCode::INT8_STORE);
        case MachineType::UINT16:
            return OpCode(OpCode::INT16_STORE);
        case MachineType::UINT32:
            return OpCode(OpCode::INT32_STORE);
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return OpCode(OpCode::INT64_STORE);
        case MachineType::FLOAT32:
            return OpCode(OpCode::FLOAT32_STORE);
        case MachineType::FLOAT64:
            return OpCode(OpCode::FLOAT64_STORE);
        default:
            UNREACHABLE();
    }
}

OpCode CircuitBuilder::GetLoadOpCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::INT8:
            return OpCode(OpCode::INT8_LOAD);
        case MachineType::INT16:
            return OpCode(OpCode::INT16_LOAD);
        case MachineType::INT32:
            return OpCode(OpCode::INT32_LOAD);
        case MachineType::INT64:
            return OpCode(OpCode::INT64_LOAD);
        case MachineType::BOOL:
            return OpCode(OpCode::INT32_LOAD);
        case MachineType::UINT8:
            return OpCode(OpCode::INT8_LOAD);
        case MachineType::UINT16:
            return OpCode(OpCode::INT16_LOAD);
        case MachineType::UINT32:
            return OpCode(OpCode::INT32_LOAD);
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return OpCode(OpCode::INT64_LOAD);
        case MachineType::FLOAT32:
            return OpCode(OpCode::FLOAT32_LOAD);
        case MachineType::FLOAT64:
            return OpCode(OpCode::FLOAT64_LOAD);
        default:
            UNREACHABLE();
    }
}

OpCode CircuitBuilder::GetSelectOpCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::NONE:
            return OpCode(OpCode::DEPEND_SELECTOR);
        case MachineType::INT8:
            return OpCode(OpCode::VALUE_SELECTOR_INT8);
        case MachineType::INT16:
            return OpCode(OpCode::VALUE_SELECTOR_INT16);
        case MachineType::INT32:
            return OpCode(OpCode::VALUE_SELECTOR_INT32);
        case MachineType::INT64:
            return OpCode(OpCode::VALUE_SELECTOR_INT64);
        case MachineType::BOOL:
            return OpCode(OpCode::VALUE_SELECTOR_INT1);
        case MachineType::UINT8:
            return OpCode(OpCode::VALUE_SELECTOR_INT8);
        case MachineType::UINT16:
            return OpCode(OpCode::VALUE_SELECTOR_INT16);
        case MachineType::UINT32:
            return OpCode(OpCode::VALUE_SELECTOR_INT32);
        case MachineType::NATIVE_POINTER:
            return OpCode(OpCode::VALUE_SELECTOR_ANYVALUE);
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return OpCode(OpCode::VALUE_SELECTOR_INT64);
        case MachineType::FLOAT32:
            return OpCode(OpCode::VALUE_SELECTOR_FLOAT32);
        case MachineType::FLOAT64:
            return OpCode(OpCode::VALUE_SELECTOR_FLOAT64);
        default:
            UNREACHABLE();
    }
}

GateRef CircuitBuilder::NewDependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::DEPEND_RELAY), 0, { state, depend }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewDependAnd(std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    return circuit_->NewGate(OpCode(OpCode::DEPEND_AND), args.size(), inputs, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewLoadGate(MachineType type, GateRef val, GateRef depend)
{
    OpCode op = GetLoadOpCodeFromMachineType(type);
    return circuit_->NewGate(op, static_cast<BitField>(type), { depend, val }, MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewStoreGate(MachineType type, GateRef ptr, GateRef val, GateRef depend)
{
    OpCode op = GetStoreOpCodeFromMachineType(type);
    return circuit_->NewGate(op, static_cast<BitField>(type), { depend, val, ptr }, MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewArithMeticGate(OpCode opcode, GateRef left, GateRef right)
{
    TypeCode type = circuit_->LoadGatePtr(left)->GetTypeCode();
    return circuit_->NewGate(opcode, 0, { left, right }, type);
}

GateRef CircuitBuilder::NewArithMeticGate(OpCode opcode, GateRef value)
{
    return circuit_->NewGate(opcode, 0, { value }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, GateRef left, GateRef right)
{
    return circuit_->NewGate(opcode, static_cast<BitField>(MachineType::BOOL), { left, right }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, GateRef value)
{
    return circuit_->NewGate(opcode, static_cast<BitField>(MachineType::BOOL), { value }, TypeCode::NOTYPE);
}

OpCode CircuitBuilder::GetCallOpCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::NONE:
            return OpCode(OpCode::CALL);
        case MachineType::INT8:
            return OpCode(OpCode::INT8_CALL);
        case MachineType::INT16:
            return OpCode(OpCode::INT16_CALL);
        case MachineType::INT32:
            return OpCode(OpCode::INT32_CALL);
        case MachineType::INT64:
            return OpCode(OpCode::INT64_CALL);
        case MachineType::BOOL:
            return OpCode(OpCode::INT1_CALL);
        case MachineType::UINT8:
            return OpCode(OpCode::INT8_CALL);
        case MachineType::UINT16:
            return OpCode(OpCode::INT16_CALL);
        case MachineType::UINT32:
            return OpCode(OpCode::INT32_CALL);
        case MachineType::NATIVE_POINTER:
            return OpCode(OpCode::ANYVALUE_CALL);
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return OpCode(OpCode::INT64_CALL);
        case MachineType::FLOAT32:
            return OpCode(OpCode::FLOAT32_CALL);
        case MachineType::FLOAT64:
            return OpCode(OpCode::FLOAT64_CALL);
        default:
            UNREACHABLE();
    }
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
    OpCode opcode = GetCallOpCodeFromMachineType(descriptor->GetReturnType());
    TypeCode type = MachineType2TypeCode(descriptor->GetReturnType());
    return circuit_->NewGate(opcode, args.size() + extraparamCnt, inputs, type);
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
    OpCode opcode = GetCallOpCodeFromMachineType(descriptor->GetReturnType());
    TypeCode type = MachineType2TypeCode(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(opcode, args.size() + 2, inputs, type);
}

GateRef CircuitBuilder::Alloca(int size, TypeCode type)
{
    auto allocaList = Circuit::GetCircuitRoot(OpCode(OpCode::ALLOCA_LIST));
    return circuit_->NewGate(OpCode(OpCode::ALLOCA), size, { allocaList }, type);
}
}  // namespace panda::ecmascript::kungfu
