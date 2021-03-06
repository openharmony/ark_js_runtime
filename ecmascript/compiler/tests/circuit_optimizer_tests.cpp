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

#include "ecmascript/compiler/circuit_optimizer.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class CircuitOptimizerTests : public testing::Test {
};

using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;

HWTEST_F_L0(CircuitOptimizerTests, TestLatticeEquationsSystemSolverFramework)
{
    // construct a circuit
    Circuit circuit;
    auto n = circuit.NewGate(OpCode(OpCode::ARG),
                             MachineType::I64,
                             0,
                             {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                             GateType::NJSValue());
    auto constantA = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     1,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantB = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     2,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantC = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     1,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto constantD = circuit.NewGate(OpCode(OpCode::CONSTANT),
                                     MachineType::I64,
                                     0,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                     GateType::NJSValue());
    auto loopBegin = circuit.NewGate(OpCode(OpCode::LOOP_BEGIN),
                                     0,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY)), Circuit::NullGate()},
                                     GateType::Empty());
    auto selectorA = circuit.NewGate(OpCode(OpCode::VALUE_SELECTOR),
                                     MachineType::I64,
                                     2,
                                     {loopBegin, constantA, Circuit::NullGate()},
                                     GateType::NJSValue());
    auto selectorB = circuit.NewGate(OpCode(OpCode::VALUE_SELECTOR),
                                     MachineType::I64,
                                     2,
                                     {loopBegin, n, Circuit::NullGate()},
                                     GateType::NJSValue());
    auto newX = circuit.NewGate(OpCode(OpCode::SUB),
                                MachineType::I64,
                                0,
                                {constantB, selectorA},
                                GateType::NJSValue());
    circuit.NewIn(selectorA, 2, newX);
    circuit.NewIn(selectorB,
                  2,
                  circuit.NewGate(OpCode(OpCode::SUB),
                                  MachineType::I64,
                                  0,
                                  {selectorB, constantC},
                                  GateType::NJSValue()));
    auto predicate = circuit.NewGate(OpCode(OpCode::NE),
                                     0,
                                     {selectorB, constantD},
                                     GateType::NJSValue());
    auto ifBranch = circuit.NewGate(OpCode(OpCode::IF_BRANCH),
                                    0,
                                    {loopBegin, predicate},
                                    GateType::Empty());
    auto ifTrue = circuit.NewGate(OpCode(OpCode::IF_TRUE),
                                  0,
                                  {ifBranch},
                                  GateType::Empty());
    auto ifFalse = circuit.NewGate(OpCode(OpCode::IF_FALSE),
                                   0,
                                   {ifBranch},
                                   GateType::Empty());
    auto loopBack = circuit.NewGate(OpCode(OpCode::LOOP_BACK),
                                    0,
                                    {ifTrue},
                                    GateType::Empty());
    circuit.NewIn(loopBegin, 1, loopBack);
    auto ret = circuit.NewGate(OpCode(OpCode::RETURN),
                               0,
                               {ifFalse,
                                Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                newX,
                                Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                               GateType::Empty());
    // verify the circuit
    {
        auto verifyResult = ecmascript::kungfu::Verifier::Run(&circuit);
        EXPECT_EQ(verifyResult, true);
    }
    {
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 2)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_TRUE(solver.GetValueLattice(circuit.GetIn(ret, 2)).GetValue() == 1);
    }
    {
        // modify the initial value of x to 2
        circuit.SetBitField(constantA, 2);
    }
    {
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is not constant)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_TRUE(solver.GetValueLattice(circuit.GetIn(ret, 2)).IsBot());
    }
    {
        // set the initial value of n to fixed value 0 (instead of function argument)
        circuit.SetBitField(n, 0);
        circuit.SetOpCode(n, OpCode(OpCode::CONSTANT));
        circuit.ModifyIn(n, 0, Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST)));
    }
    {
        ecmascript::kungfu::LatticeUpdateRuleSCCP rule;
        ecmascript::kungfu::LatticeEquationsSystemSolverFramework solver(&rule);
        // optimize the circuit
        auto optimizeResult = solver.Run(&circuit, false);
        EXPECT_EQ(optimizeResult, true);
        // check optimization result (returned value is constant 0)
        EXPECT_TRUE(solver.GetReachabilityLattice(ret).IsReachable());
        EXPECT_TRUE(solver.GetValueLattice(circuit.GetIn(ret, 2)).GetValue() == 0);
    }
}

HWTEST_F_L0(CircuitOptimizerTests, TestSubgraphRewriteFramework)
{
    Circuit circuit;
    const uint64_t numOfConstants = 100;
    const uint64_t numOfUses = 10;
    std::random_device randomDevice;
    std::mt19937_64 rng(randomDevice());
    std::multimap<uint64_t, ecmascript::kungfu::GateRef> constantsSet;
    for (uint64_t iter = 0; iter < numOfUses; iter++) {
        for (uint64_t idx = 0; idx < numOfConstants; idx++) {
            constantsSet.insert(
                std::make_pair(rng(),
                               circuit.GetConstantGate(MachineType::I64,
                                                       idx,
                                                       GateType::NJSValue())));
        }
    }
    while (constantsSet.size() > 1) {
        const auto elementA = constantsSet.begin();
        const auto operandA = elementA->second;
        constantsSet.erase(elementA);
        const auto elementB = constantsSet.begin();
        const auto operandB = elementB->second;
        constantsSet.erase(elementB);
        constantsSet.insert(
            std::make_pair(rng(),
                           circuit.NewGate(OpCode(OpCode::ADD),
                                           MachineType::I64,
                                           0,
                                           {operandA,
                                            operandB},
                                           GateType::NJSValue())));
    }
    auto ret = circuit.NewGate(OpCode(OpCode::RETURN),
                               0,
                               {Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY)),
                                Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                constantsSet.begin()->second,
                                Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                               GateType::Empty());
    ecmascript::kungfu::SubgraphRewriteRuleCP rule;
    ecmascript::kungfu::SubGraphRewriteFramework rewriter(&rule);
    rewriter.Run(&circuit);
    auto returnValue = circuit.GetIn(ret, 2);
    EXPECT_TRUE(circuit.GetOpCode(returnValue) == OpCode(OpCode::CONSTANT));
    EXPECT_TRUE(circuit.GetBitField(returnValue) == (numOfUses) * (numOfConstants) * (numOfConstants - 1) / 2);
}
} // namespace panda::test