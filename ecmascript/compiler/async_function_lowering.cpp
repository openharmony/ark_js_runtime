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

#include "ecmascript/compiler/async_function_lowering.h"

namespace panda::ecmascript::kungfu {
void AsyncFunctionLowering::ProcessAll()
{
    ProcessJumpTable();
}

void AsyncFunctionLowering::ProcessJumpTable()
{
    GateRef newTarget = argAccessor_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    GateRef isEqual = builder_.Equal(newTarget, builder_.Undefined());
    GateRef stateEntryState = *accessor_.ConstUses(stateEntry_).begin();
    GateRef ifBranchCondition = builder_.Branch(stateEntry_, isEqual);
    GateRef ifTrueCondition = builder_.IfTrue(ifBranchCondition);
    GateRef ifFalseCondition = builder_.IfFalse(ifBranchCondition);
    accessor_.ReplaceStateIn(stateEntryState, ifTrueCondition);

    GateRef contextOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET);
    GateRef val = builder_.PtrAdd(newTarget, contextOffset);
    GateRef contextGate = circuit_->NewGate(OpCode(OpCode::LOAD), MachineType::I64, 0, {dependEntry_, val},
                                            GateType::TaggedPointer());
    GateRef bcOffset = builder_.IntPtr(GeneratorContext::GENERATOR_BC_OFFSET_OFFSET);
    val = builder_.PtrAdd(contextGate, bcOffset);
    GateRef restoreOffsetGate = circuit_->NewGate(OpCode(OpCode::LOAD), MachineType::I32, 0, {contextGate, val},
                                                  GateType::NJSValue());
    GateRef firstState = Circuit::NullGate();
    const auto &suspendAndResumeGates = bcBuilder_->GetAsyncRelatedGates();
    for (const auto &gate : suspendAndResumeGates) {
        auto curInfo = bcBuilder_->GetByteCodeInfo(gate);
        if (curInfo.IsBc(EcmaOpcode::RESUMEGENERATOR_PREF_V8)) {
            RebuildGeneratorCfg(gate, restoreOffsetGate, ifFalseCondition, newTarget, firstState);
        }
    }
}

void AsyncFunctionLowering::RebuildGeneratorCfg(GateRef resumeGate, GateRef restoreOffsetGate, GateRef ifFalseCondition,
                                                GateRef newTarget, GateRef &firstState)
{
    GateRef ifSuccess = accessor_.GetState(resumeGate);
    GateRef suspendGate = accessor_.GetState(ifSuccess);
    GateRef firstRestoreRegGate = GetFirstRestoreRegister(resumeGate);
    GateRef offsetConstantGate = accessor_.GetValueIn(suspendGate);
    offsetConstantGate = builder_.TruncInt64ToInt32(offsetConstantGate);
    auto stateInGate = accessor_.GetState(resumeGate);
    bool flag = true;
    GateRef prevLoopBeginGate = Circuit::NullGate();
    GateRef loopBeginStateIn = Circuit::NullGate();
    GateRef prevBcOffsetPhiGate = Circuit::NullGate();
    while (true) {
        auto opcode = accessor_.GetOpCode(stateInGate);
        if (opcode == OpCode::STATE_ENTRY) {
            GateRef condition = circuit_->NewGate(OpCode(OpCode::EQ), 0, {offsetConstantGate, restoreOffsetGate},
                                                  GateType::NJSValue());
            GateRef ifBranch = circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, { ifFalseCondition, condition },
                                                 GateType::Empty());
            GateRef ifTrue = circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, {ifBranch}, GateType::Empty());
            GateRef ifFalse = circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, {ifBranch}, GateType::Empty());
            if (flag) {
                accessor_.ReplaceStateIn(resumeGate, ifTrue);
                accessor_.ReplaceValueIn(resumeGate, newTarget);
                accessor_.ReplaceDependIn(firstRestoreRegGate, restoreOffsetGate);
                circuit_->NewGate(OpCode(OpCode::RETURN), 0,
                                  {ifSuccess, suspendGate, suspendGate,
                                   Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                  GateType::AnyType());
            } else {
                loopBeginStateIn = ifTrue;
            }
            accessor_.ReplaceStateIn(ifBranch, ifFalseCondition);
            if (firstState != Circuit::NullGate()) {
                accessor_.ReplaceStateIn(firstState, ifFalse);
            } else {
                auto constant = builder_.UndefineConstant();
                circuit_->NewGate(OpCode(OpCode::RETURN), 0,
                                  {ifFalse, restoreOffsetGate, constant,
                                   Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                  GateType::AnyType());
            }
            firstState = ifBranch;
        }

        if (opcode == OpCode::LOOP_BEGIN) {
            // This constant gate must be created by the NewGate method to distinguish whether the while
            // loop needs to modify the phi node or not.
            GateRef emptyOffsetGate = circuit_->NewGate(OpCode(OpCode::CONSTANT), MachineType::I32,
                                                        static_cast<BitField>(-1),
                                                        {Circuit::GetCircuitRoot(OpCode(OpCode::CIRCUIT_ROOT))},
                                                        GateType::NJSValue());
            GateRef bcOffsetPhiGate = circuit_->NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I32, 2,
                                                        {stateInGate, restoreOffsetGate, emptyOffsetGate},
                                                        GateType::NJSValue());

            GateRef condition = circuit_->NewGate(OpCode(OpCode::EQ), 0, {offsetConstantGate, bcOffsetPhiGate},
                                                  GateType::NJSValue());
            GateRef ifBranch = circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, {stateInGate, condition},
                                                 GateType::Empty());
            GateRef ifTrue = circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, {ifBranch}, GateType::Empty());
            GateRef ifFalse = circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, {ifBranch}, GateType::Empty());

            GateRef resumeStateGate = accessor_.GetState(resumeGate);
            if (accessor_.GetOpCode(resumeStateGate) != OpCode::IF_TRUE) {
                accessor_.ReplaceStateIn(resumeGate, ifTrue);
                accessor_.ReplaceValueIn(resumeGate, newTarget);
                accessor_.ReplaceDependIn(firstRestoreRegGate, bcOffsetPhiGate);
                circuit_->NewGate(OpCode(OpCode::RETURN), 0,
                                  {ifSuccess, suspendGate, suspendGate,
                                   Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                  GateType::AnyType());
            } else {
                // Handling multi-layer for loops
                UpdateValueSelector(prevLoopBeginGate, ifTrue, prevBcOffsetPhiGate);
                accessor_.ReplaceValueIn(prevBcOffsetPhiGate, bcOffsetPhiGate);
            }
            accessor_.ReplaceStateIn(ifBranch, stateInGate);

            // Find the node with LOOP_BEGIN as State input and modify its
            // state input to the newly created IF_FALSE node.
            auto uses = accessor_.Uses(stateInGate);
            for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                if (accessor_.GetOpCode(*useIt).IsState() && *useIt != ifBranch) {
                    accessor_.ReplaceIn(useIt, ifFalse);
                }
            }

            prevLoopBeginGate = stateInGate;
            prevBcOffsetPhiGate = bcOffsetPhiGate;
            stateInGate = accessor_.GetState(stateInGate);
            flag = false;
            continue;
        }
        if (loopBeginStateIn != Circuit::NullGate()) {
            UpdateValueSelector(prevLoopBeginGate, loopBeginStateIn, prevBcOffsetPhiGate);
            break;
        }
        if (accessor_.GetOpCode(stateInGate) == OpCode::STATE_ENTRY) {
            break;
        }
        stateInGate = accessor_.GetState(stateInGate);
    }
}

void AsyncFunctionLowering::UpdateValueSelector(GateRef prevLoopBeginGate,
                                                GateRef controlStateGate,
                                                GateRef prevBcOffsetPhiGate)
{
    GateRef loopBeginFirstState = accessor_.GetState(prevLoopBeginGate);
    GateRef newGate = circuit_->NewGate(OpCode(OpCode::MERGE), 2,
                                        {controlStateGate, loopBeginFirstState}, GateType::Empty());
    accessor_.ReplaceStateIn(prevLoopBeginGate, newGate);
    auto loopBeginUses = accessor_.Uses(prevLoopBeginGate);
    for (auto use : loopBeginUses) {
        if (accessor_.GetOpCode(use) == OpCode::VALUE_SELECTOR && use != prevBcOffsetPhiGate) {
            auto machineType = accessor_.GetMachineType(use);
            auto gateType = accessor_.GetGateType(use);
            auto undefinedGate =
                accessor_.GetConstantGate(machineType, JSTaggedValue::VALUE_UNDEFINED, gateType);
            auto firstValueGate = accessor_.GetValueIn(use, 0);
            auto newValueSelector = circuit_->NewGate(OpCode(OpCode::VALUE_SELECTOR), machineType,
                                                      2, {newGate, undefinedGate, firstValueGate},
                                                      gateType);
            accessor_.ReplaceValueIn(use, newValueSelector);
        }
    }
}

bool AsyncFunctionLowering::IsAsyncRelated() const
{
    return  bcBuilder_->GetAsyncRelatedGates().size() > 0;
}

GateRef AsyncFunctionLowering::GetFirstRestoreRegister(GateRef gate) const
{
    GateRef firstRestoreGate = 0;
    GateRef curRestoreGate = accessor_.GetDep(gate);
    while (accessor_.GetOpCode(curRestoreGate) == OpCode::RESTORE_REGISTER) {
        firstRestoreGate = curRestoreGate;
        curRestoreGate = accessor_.GetDep(curRestoreGate);
    }
    return firstRestoreGate;
}
}  // panda::ecmascript::kungfu

