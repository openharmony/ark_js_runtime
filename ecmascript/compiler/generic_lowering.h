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

#ifndef ECMASCRIPT_COMPILER_GENERIC_LOWERING_H
#define ECMASCRIPT_COMPILER_GENERIC_LOWERING_H

#include "circuit.h"
#include "bytecode_circuit_builder.h"
#include "circuit_builder.h"
#include "gate_accessor.h"

namespace panda::ecmascript::kungfu {
class GenericLowering {
public:
    explicit GenericLowering(BytecodeCircuitBuilder *builder, Circuit *circuit)
        : builder_(builder), circuit_(circuit) {}
    ~GenericLowering() = default;
    void CallRuntimeLowering();

private:
    void Lower(GateRef gate, EcmaOpcode bytecode);
    void LowerAdd2Dyn(GateRef gate, GateRef glue);
    void LowerHIR(CircuitBuilder &builder, GateRef oldGate, GateRef newGate);

    inline GateRef GetLoweringInt64Constant(int64_t value)
    {
        auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
        return circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, value, {constantList}, GateType::C_VALUE);
    }

    BytecodeCircuitBuilder *builder_;
    Circuit *circuit_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_GENERIC_LOWERING_H