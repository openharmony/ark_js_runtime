/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_ASYNC_FUNCTION_LOWRING_H_
#define ECMASCRIPT_COMPILER_ASYNC_FUNCTION_LOWRING_H_

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/circuit_builder.h"

namespace panda::ecmascript::kungfu {
class AsyncFunctionLowering {
public:
    AsyncFunctionLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg,
                          bool enableLog)
        : bcBuilder_(bcBuilder), circuit_(circuit), builder_(circuit, cmpCfg), enableLog_(enableLog),
        stateEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY))),
        dependEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY))),
        accessor_(circuit), argAccessor_(circuit)
    {
    }
    ~AsyncFunctionLowering() = default;

    void ProcessAll();

    bool IsAsyncRelated() const;

private:
    void ProcessJumpTable();

    void RebuildGeneratorCfg(GateRef resumeGate, GateRef restoreOffsetGate, GateRef ifFalseCondition, GateRef newTarget,
                             GateRef &firstState);

    void UpdateValueSelector(GateRef prevLoopBeginGate, GateRef controlStateGate, GateRef prevBcOffsetPhiGate);

    GateRef GetFirstRestoreRegister(GateRef gate) const;

    BytecodeCircuitBuilder *bcBuilder_;
    Circuit *circuit_;
    CircuitBuilder builder_;
    [[maybe_unused]] bool enableLog_ {false};
    GateRef stateEntry_ {Circuit::NullGate()};
    GateRef dependEntry_ {Circuit::NullGate()};
    GateAccessor accessor_;
    ArgumentAccessor argAccessor_;
};
}  // panda::ecmascript::kungfu

#endif // ECMASCRIPT_COMPILER_ASYNC_FUNCTION_LOWRING_H_
