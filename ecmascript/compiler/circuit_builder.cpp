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
    return circuit_->NewGate(OpCode(OpCode::ARG), ValueCode::INT64, index, {argListOfCircuit}, TypeCode::NOTYPE);
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

GateRef CircuitBuilder::NewInt8Constant(int8_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT8, val, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewInt16Constant(int16_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT16, val, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opcode, ValueCode valCode, GateRef control, int valueCounts,
                                        MachineType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(Circuit::NullGate());
    }

    return circuit_->NewGate(opcode, valCode, valueCounts, inList, MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewSelectorGate(OpCode opcode, ValueCode valCode, GateRef control, std::vector<GateRef> &values,
                                        int valueCounts, MachineType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    for (int i = 0; i < valueCounts; i++) {
        inList.push_back(values[i]);
    }

    return circuit_->NewGate(opcode, valCode, valueCounts, inList, MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewIntegerConstant(int32_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT32, val, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewInteger64Constant(int64_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT64, val, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewBooleanConstant(bool val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT32, val ? 1 : 0, {constantList}, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewDoubleConstant(double val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::FLOAT64, bit_cast<int64_t>(val), {constantList},
                             TypeCode::NOTYPE);
}

GateRef CircuitBuilder::UndefineConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT64, TaggedValue::VALUE_UNDEFINED, { constantList },
                             type);
}

GateRef CircuitBuilder::HoleConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT64, TaggedValue::VALUE_HOLE, { constantList },
                             type);
}

GateRef CircuitBuilder::NullConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT64, TaggedValue::VALUE_NULL, { constantList },
                             type);
}

GateRef CircuitBuilder::ExceptionConstant(TypeCode type)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    // NOTE: add bitfield value here
    return circuit_->NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT64, TaggedValue::VALUE_EXCEPTION, { constantList },
                             type);
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

GateRef CircuitBuilder::NewSwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_CASE), value, { switchBranch }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewDefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(OpCode(OpCode::DEFAULT_CASE), 0, { switchBranch }, TypeCode::NOTYPE);
}

ValueCode CircuitBuilder::GetStoreValueCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::INT8:
            return ValueCode::INT8;
        case MachineType::INT16:
            return ValueCode::INT16;
        case MachineType::INT32:
            return ValueCode::INT32;
        case MachineType::INT64:
            return ValueCode::INT64;
        case MachineType::BOOL:
            return ValueCode::INT32;
        case MachineType::UINT8:
            return ValueCode::INT8;
        case MachineType::UINT16:
            return ValueCode::INT16;
        case MachineType::UINT32:
            return ValueCode::INT32;
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return ValueCode::INT64;
        case MachineType::FLOAT32:
            return ValueCode::FLOAT32;
        case MachineType::FLOAT64:
            return ValueCode::FLOAT64;
        default:
            UNREACHABLE();
    }
}

ValueCode CircuitBuilder::GetLoadValueCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::INT8:
            return ValueCode::INT8;
        case MachineType::INT16:
            return ValueCode::INT16;
        case MachineType::INT32:
            return ValueCode::INT32;
        case MachineType::INT64:
            return ValueCode::INT64;
        case MachineType::BOOL:
            return ValueCode::INT32;
        case MachineType::UINT8:
            return ValueCode::INT8;
        case MachineType::UINT16:
            return ValueCode::INT16;
        case MachineType::UINT32:
            return ValueCode::INT32;
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return ValueCode::INT64;
        case MachineType::FLOAT32:
            return ValueCode::FLOAT32;
        case MachineType::FLOAT64:
            return ValueCode::FLOAT64;
        default:
            UNREACHABLE();
    }
}

ValueCode CircuitBuilder::GetValueCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::NONE:
            return ValueCode::NOVALUE;
        case MachineType::INT8:
            return ValueCode::INT8;
        case MachineType::INT16:
            return ValueCode::INT16;
        case MachineType::INT32:
            return ValueCode::INT32;
        case MachineType::INT64:
            return ValueCode::INT64;
        case MachineType::BOOL:
            return ValueCode::INT1;
        case MachineType::UINT8:
            return ValueCode::INT8;
        case MachineType::UINT16:
            return ValueCode::INT16;
        case MachineType::UINT32:
            return ValueCode::INT32;
        case MachineType::NATIVE_POINTER:
            return ValueCode::ARCH;
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return ValueCode::INT64;
        case MachineType::FLOAT32:
            return ValueCode::FLOAT32;
        case MachineType::FLOAT64:
            return ValueCode::FLOAT64;
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
    ValueCode valCode = GetLoadValueCodeFromMachineType(type);
    return circuit_->NewGate(OpCode(OpCode::LOAD), valCode, static_cast<BitField>(type), { depend, val },
                             MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewStoreGate(MachineType type, GateRef ptr, GateRef val, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::STORE), static_cast<BitField>(type), { depend, val, ptr },
                             MachineType2TypeCode(type));
}

GateRef CircuitBuilder::NewArithmeticGate(OpCode opcode, ValueCode valCode, GateRef left, GateRef right)
{
    TypeCode type = circuit_->LoadGatePtr(left)->GetTypeCode();
    return circuit_->NewGate(opcode, valCode, 0, { left, right }, type);
}

GateRef CircuitBuilder::NewNumberGate(OpCode opcode, GateRef value)
{
    return circuit_->NewGate(opcode, 0, { value }, TypeCode::JS_NUMBER);
}

GateRef CircuitBuilder::NewArithmeticGate(OpCode opcode, ValueCode valCode, GateRef value)
{
    return circuit_->NewGate(opcode, valCode, 0, { value }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewArithmeticGate(OpCode opcode, GateRef value)
{
    return circuit_->NewGate(opcode, 0, { value }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, ValueCode valCode, GateRef left, GateRef right)
{
    return circuit_->NewGate(opcode, valCode, static_cast<BitField>(MachineType::BOOL), { left, right },
                             TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, GateRef left, GateRef right)
{
    return circuit_->NewGate(opcode, static_cast<BitField>(MachineType::BOOL), { left, right }, TypeCode::NOTYPE);
}

GateRef CircuitBuilder::NewLogicGate(OpCode opcode, ValueCode valCode, GateRef value)
{
    return circuit_->NewGate(opcode, valCode, static_cast<BitField>(MachineType::BOOL), { value }, TypeCode::NOTYPE);
}

ValueCode CircuitBuilder::GetCallValueCodeFromMachineType(MachineType type)
{
    switch (type) {
        case MachineType::NONE:
            return ValueCode::NOVALUE;
        case MachineType::INT8:
            return ValueCode::INT8;
        case MachineType::INT16:
            return ValueCode::INT64;
        case MachineType::INT32:
            return ValueCode::INT32;
        case MachineType::INT64:
            return ValueCode::INT64;
        case MachineType::BOOL:
            return ValueCode::INT1;
        case MachineType::UINT8:
            return ValueCode::INT8;
        case MachineType::UINT16:
            return ValueCode::INT16;
        case MachineType::UINT32:
            return ValueCode::INT32;
        case MachineType::NATIVE_POINTER:
            return ValueCode::ARCH;
        case MachineType::UINT64:
        case MachineType::TAGGED:
        case MachineType::TAGGED_POINTER:
            return ValueCode::INT64;
        case MachineType::FLOAT32:
            return ValueCode::FLOAT32;
        case MachineType::FLOAT64:
            return ValueCode::FLOAT64;
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
    ValueCode valCode = GetCallValueCodeFromMachineType(descriptor->GetReturnType());
    TypeCode type = MachineType2TypeCode(descriptor->GetReturnType());
    return circuit_->NewGate(OpCode(OpCode::CALL), valCode, args.size() + extraparamCnt, inputs, type);
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
    ValueCode valCode = GetCallValueCodeFromMachineType(descriptor->GetReturnType());
    TypeCode type = MachineType2TypeCode(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(OpCode(OpCode::CALL), valCode, args.size() + 2, inputs, type);
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
    TypeCode type = MachineType2TypeCode(descriptor->GetReturnType());
    // 2 : 2 means extra two input gates (target glue)
    return circuit_->NewGate(opcode, args.size() + 2, inputs, type);
}

GateRef CircuitBuilder::Alloca(int size)
{
    auto allocaList = Circuit::GetCircuitRoot(OpCode(OpCode::ALLOCA_LIST));
    return circuit_->NewGate(OpCode(OpCode::ALLOCA), size, { allocaList }, TypeCode::NOTYPE);
}
}  // namespace panda::ecmascript::kungfu
