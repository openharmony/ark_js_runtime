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

#ifndef ECMASCRIPT_COMPILER_LOW_LEVEL_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_LOW_LEVEL_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/call_signature.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

class LCircuitBuilder {
public:
    explicit LCircuitBuilder(Circuit *circuit) : circuit_(circuit) {}
    ~LCircuitBuilder() = default;
    NO_MOVE_SEMANTIC(LCircuitBuilder);
    NO_COPY_SEMANTIC(LCircuitBuilder);
    GateRef Merge(GateRef *in, size_t controlCount);
    GateRef Selector(OpCode opcode, MachineType machineType, GateRef control,
        const std::vector<GateRef> &values, int valueCounts, VariableType type = VariableType::VOID());
    GateRef Selector(OpCode opcode, GateRef control,
        const std::vector<GateRef> &values, int valueCounts, VariableType type = VariableType::VOID());
    GateRef UndefineConstant(GateType type);
    GateRef Branch(GateRef state, GateRef condition);
    GateRef SwitchBranch(GateRef state, GateRef index, int caseCounts);
    GateRef Return(GateRef state, GateRef depend, GateRef value);
    GateRef ReturnVoid(GateRef state, GateRef depend);
    GateRef Goto(GateRef state);
    GateRef LoopBegin(GateRef state);
    GateRef LoopEnd(GateRef state);
    GateRef IfTrue(GateRef ifBranch);
    GateRef IfFalse(GateRef ifBranch);
    GateRef SwitchCase(GateRef switchBranch, int64_t value);
    GateRef DefaultCase(GateRef switchBranch);
    GateRef DependRelay(GateRef state, GateRef depend);
    GateRef DependAnd(std::initializer_list<GateRef> args);
    Circuit *GetCircuit() const
    {
        return circuit_;
    }
private:
    Circuit *circuit_;
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_LOW_LEVEL_CIRCUIT_BUILDER_H