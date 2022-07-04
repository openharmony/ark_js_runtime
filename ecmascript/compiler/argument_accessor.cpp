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

#include "ecmascript/compiler/argument_accessor.h"

namespace panda::ecmascript::kungfu {
void ArgumentAccessor::NewCommonArg(const CommonArgIdx argIndex, MachineType machineType, GateType gateType)
{
    circuit_->NewGate(OpCode(OpCode::ARG), machineType, static_cast<size_t>(argIndex), { argRoot_ }, gateType);
}

void ArgumentAccessor::NewArg(const size_t argIndex)
{
    circuit_->NewGate(OpCode(OpCode::ARG), MachineType::I64, argIndex, { argRoot_ }, GateType::TaggedValue());
}

// jsmethod must be set
size_t ArgumentAccessor::GetActualNumArgs() const
{
    ASSERT(method_ != nullptr);
    auto numArgs = method_->GetNumArgsWithCallField();
    return static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS) + numArgs;
}

// jsmethod must be set
GateRef ArgumentAccessor::GetArgGate(const size_t currentVreg) const
{
    ASSERT(method_ != nullptr);
    const size_t offsetArgs = method_->GetNumVregs();
    ASSERT(currentVreg >= offsetArgs && currentVreg < offsetArgs + method_->GetNumArgs());
    auto reg = currentVreg - offsetArgs;
    auto haveFunc = method_->HaveFuncWithCallField();
    auto haveNewTarget = method_->HaveNewTargetWithCallField();
    auto haveThis = method_->HaveThisWithCallField();
    auto index = GetFunctionArgIndex(reg, haveFunc, haveNewTarget, haveThis);
    auto argsArray = GetFunctionArgs();
    return argsArray.at(index);
}

GateRef ArgumentAccessor::GetCommonArgGate(const CommonArgIdx arg) const
{
    auto argsArray = GetFunctionArgs();
    return argsArray.at(static_cast<size_t>(arg));
}

std::vector<GateRef> ArgumentAccessor::GetFunctionArgs() const
{
    auto argsArray = circuit_->GetOutVector(argRoot_);
    std::reverse(argsArray.begin(), argsArray.end());
    return argsArray;
}

size_t ArgumentAccessor::GetFunctionArgIndex(const size_t currentVreg, const bool haveFunc,
                                             const bool haveNewTarget, const bool haveThis) const
{
    size_t numCommonArgs = haveFunc + haveNewTarget + haveThis;
    // 2: number of common args
    if (numCommonArgs == 2) {
        if (!haveFunc && currentVreg == 0) {
            return static_cast<size_t>(CommonArgIdx::NEW_TARGET);
        }
        if (!haveFunc && currentVreg == 1) {
            return static_cast<size_t>(CommonArgIdx::THIS);
        }
        if (!haveNewTarget && currentVreg == 0) {
            return static_cast<size_t>(CommonArgIdx::FUNC);
        }
        if (!haveNewTarget && currentVreg == 1) {
            return static_cast<size_t>(CommonArgIdx::THIS);
        }
        if (!haveThis && currentVreg == 0) {
            return static_cast<size_t>(CommonArgIdx::FUNC);
        }
        if (!haveThis && currentVreg == 1) {
            return static_cast<size_t>(CommonArgIdx::NEW_TARGET);
        }
    }
    // 1: number of common args, 0: the index of currentVreg
    if (numCommonArgs == 1 && currentVreg == 0) {
        if (haveFunc) {
            return static_cast<size_t>(CommonArgIdx::FUNC);
        }
        if (haveNewTarget) {
            return static_cast<size_t>(CommonArgIdx::NEW_TARGET);
        }
        if (haveThis) {
            return static_cast<size_t>(CommonArgIdx::THIS);
        }
    }
    return currentVreg - numCommonArgs + static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS);
}
}  // namespace panda::ecmascript::kungfu
