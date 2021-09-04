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

#ifndef PANDA_RUNTIME_ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
#define PANDA_RUNTIME_ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/compiler/stub_interface.h"

namespace kungfu {
class CircuitBuilder {
public:
    explicit CircuitBuilder(Circuit *circuit) : circuit_(circuit) {}
    ~CircuitBuilder() = default;
    NO_MOVE_SEMANTIC(CircuitBuilder);
    NO_COPY_SEMANTIC(CircuitBuilder);
    AddrShift NewArguments(size_t index);
    AddrShift NewMerge(AddrShift *in, size_t controlCount);
    AddrShift NewSelectorGate(OpCode opcode, AddrShift control, int valueCounts);
    AddrShift NewIntegerConstant(int32_t value);
    AddrShift NewInteger64Constant(int64_t value);
    AddrShift NewWord64Constant(uint64_t val);
    AddrShift NewBooleanConstant(bool value);
    AddrShift NewDoubleConstant(double value);
    AddrShift UndefineConstant();
    AddrShift HoleConstant();
    AddrShift Alloca(int size);
    AddrShift Branch(AddrShift state, AddrShift condition);
    AddrShift SwitchBranch(AddrShift state, AddrShift index, int caseCounts);
    AddrShift Return(AddrShift state, AddrShift value);
    AddrShift Goto(AddrShift state);
    AddrShift LoopBegin(AddrShift state);
    AddrShift LoopEnd(AddrShift state);
    AddrShift NewIfTrue(AddrShift ifBranch);
    AddrShift NewIfFalse(AddrShift ifBranch);
    AddrShift NewSwitchCase(AddrShift switchBranch, int32_t value);
    AddrShift NewDefaultCase(AddrShift switchBranch);
    AddrShift NewLoadGate(MachineType type, AddrShift val);
    AddrShift NewStoreGate(MachineType type, AddrShift ptr, AddrShift val);
    AddrShift NewArithMeticGate(OpCode opcode, AddrShift left, AddrShift right);
    AddrShift NewArithMeticGate(OpCode opcode, AddrShift value);
    AddrShift NewLogicGate(OpCode opcode, AddrShift left, AddrShift right);
    AddrShift NewLogicGate(OpCode opcode, AddrShift value);
    AddrShift NewCallGate(StubInterfaceDescriptor *descriptor, AddrShift target,
        std::initializer_list<AddrShift> args);
    AddrShift NewCallGate(StubInterfaceDescriptor *descriptor,  AddrShift target, AddrShift depend,
        std::initializer_list<AddrShift> args);
    static OpCode GetLoadOpCodeFromMachineType(MachineType type);
    static OpCode GetStoreOpCodeFromMachineType(MachineType type);
    static OpCode GetSelectOpCodeFromMachineType(MachineType type);
    static OpCode GetCallOpCodeFromMachineType(MachineType type);

private:
    Circuit *circuit_;
};
}  // namespace kungfu

#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H