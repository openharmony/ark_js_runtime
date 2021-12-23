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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/compiler/stub_descriptor.h"

namespace panda::ecmascript::kungfu {
class CircuitBuilder {
public:
    explicit CircuitBuilder(Circuit *circuit) : circuit_(circuit) {}
    ~CircuitBuilder() = default;
    NO_MOVE_SEMANTIC(CircuitBuilder);
    NO_COPY_SEMANTIC(CircuitBuilder);
    GateRef NewArguments(size_t index);
    GateRef NewMerge(GateRef *in, size_t controlCount);
    GateRef NewSelectorGate(OpCode opcode, GateRef control, int valueCounts,
                              MachineType type = MachineType::NONE);
    GateRef NewSelectorGate(OpCode opcode, GateRef control, std::vector<GateRef> &values, int valueCounts,
                              MachineType type = MachineType::NONE);
    GateRef NewIntegerConstant(int32_t value);
    GateRef NewInteger64Constant(int64_t value);
    GateRef NewWord64Constant(uint64_t val);
    GateRef NewBooleanConstant(bool value);
    GateRef NewDoubleConstant(double value);
    GateRef UndefineConstant(TypeCode type);
    GateRef HoleConstant(TypeCode type);
    GateRef NullConstant(TypeCode type);
    GateRef ExceptionConstant(TypeCode type);
    GateRef Alloca(int size, TypeCode type);
    GateRef Branch(GateRef state, GateRef condition);
    GateRef SwitchBranch(GateRef state, GateRef index, int caseCounts);
    GateRef Return(GateRef state, GateRef depend, GateRef value);
    GateRef ReturnVoid(GateRef state, GateRef depend);
    GateRef Goto(GateRef state);
    GateRef LoopBegin(GateRef state);
    GateRef LoopEnd(GateRef state);
    GateRef NewIfTrue(GateRef ifBranch);
    GateRef NewIfFalse(GateRef ifBranch);
    GateRef NewSwitchCase(GateRef switchBranch, int32_t value);
    GateRef NewDefaultCase(GateRef switchBranch);
    GateRef NewLoadGate(MachineType type, GateRef val, GateRef depend);
    GateRef NewStoreGate(MachineType type, GateRef ptr, GateRef val, GateRef depend);
    GateRef NewDependRelay(GateRef state, GateRef depend);
    GateRef NewDependAnd(std::initializer_list<GateRef> args);
    GateRef NewArithMeticGate(OpCode opcode, GateRef left, GateRef right);
    GateRef NewArithMeticGate(OpCode opcode, GateRef value);
    GateRef NewLogicGate(OpCode opcode, GateRef left, GateRef right);
    GateRef NewLogicGate(OpCode opcode, GateRef value);
    GateRef NewCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
                                 std::initializer_list<GateRef> args);
    GateRef NewCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
                                 GateRef depend, std::initializer_list<GateRef> args);
    static OpCode GetLoadOpCodeFromMachineType(MachineType type);
    static OpCode GetStoreOpCodeFromMachineType(MachineType type);
    static OpCode GetSelectOpCodeFromMachineType(MachineType type);
    static OpCode GetCallOpCodeFromMachineType(MachineType type);

    static TypeCode MachineType2TypeCode(MachineType type)
    {
        switch (type) {
            case MachineType::TAGGED_POINTER:
                return TypeCode::TAGGED_POINTER;
            case MachineType::TAGGED:
                return TypeCode::JS_ANY;
            default:
                return TypeCode::NOTYPE;
        }
    }

private:
    Circuit *circuit_;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H