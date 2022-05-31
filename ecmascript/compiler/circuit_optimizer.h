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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_OPTIMIZER_H_
#define ECMASCRIPT_COMPILER_CIRCUIT_OPTIMIZER_H_

#include <cstdint>
#include <deque>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
enum class LatticeStatus {
    TOP,
    MID,
    BOT,
};

class ValueLattice {
public:
    explicit ValueLattice();
    explicit ValueLattice(LatticeStatus status);
    explicit ValueLattice(uint64_t value);
    [[nodiscard]] bool IsTop() const;
    [[nodiscard]] bool IsMid() const;
    [[nodiscard]] bool IsBot() const;
    [[nodiscard]] LatticeStatus GetStatus() const;
    [[nodiscard]] std::optional<uint64_t> GetValue() const;
    ValueLattice Meet(const ValueLattice &other);
    bool operator==(const ValueLattice &other) const;
    bool operator!=(const ValueLattice &other) const;
    bool operator<(const ValueLattice &other) const;
    bool operator>(const ValueLattice &other) const;
    bool operator<=(const ValueLattice &other) const;
    bool operator>=(const ValueLattice &other) const;
    [[nodiscard]] ValueLattice Implies(const ValueLattice &other) const;
    void Print(std::ostream &os) const;

private:
    uint64_t value_;
    LatticeStatus status_;
};

class ReachabilityLattice {
public:
    explicit ReachabilityLattice();
    explicit ReachabilityLattice(bool reachable);
    [[nodiscard]] bool IsReachable() const;
    [[nodiscard]] bool IsUnreachable() const;
    bool operator==(const ReachabilityLattice &other) const;
    bool operator!=(const ReachabilityLattice &other) const;
    ReachabilityLattice operator+(const ReachabilityLattice &other) const;
    ReachabilityLattice operator*(const ReachabilityLattice &other) const;
    [[nodiscard]] ValueLattice Implies(const ValueLattice &other) const;
    void Print(std::ostream &os) const;

private:
    bool reachable_;
};

class LatticeUpdateRule {
public:
    void Initialize(Circuit *circuit,
                    std::function<ValueLattice &(GateRef)> valueLattice,
                    std::function<ReachabilityLattice &(GateRef)> reachabilityLattice);
    bool UpdateValueLattice(GateRef gate, const ValueLattice &valueLattice);
    bool UpdateReachabilityLattice(GateRef gate, const ReachabilityLattice &reachabilityLattice);
    virtual bool Run(GateRef gate) = 0;

protected:
    std::function<ValueLattice &(GateRef)> valueLatticeMap_;
    std::function<ReachabilityLattice &(GateRef)> reachabilityLatticeMap_;
    Circuit *circuit_ = nullptr;
};

class LatticeUpdateRuleSCCP : public LatticeUpdateRule {
public:
    bool Run(GateRef gate) override;
    bool RunCircuitRoot(GateRef gate);
    bool RunStateEntry(GateRef gate);
    bool RunDependEntry(GateRef gate);
    bool RunFrameStateEntry(GateRef gate);
    bool RunReturnList(GateRef gate);
    bool RunThrowList(GateRef gate);
    bool RunConstantList(GateRef gate);
    bool RunAllocaList(GateRef gate);
    bool RunArgList(GateRef gate);
    bool RunReturn(GateRef gate);
    bool RunReturnVoid(GateRef gate);
    bool RunThrow(GateRef gate);
    bool RunOrdinaryBlock(GateRef gate);
    bool RunIfBranch(GateRef gate);
    bool RunSwitchBranch(GateRef gate);
    bool RunIfTrue(GateRef gate);
    bool RunIfFalse(GateRef gate);
    bool RunSwitchCase(GateRef gate);
    bool RunDefaultCase(GateRef gate);
    bool RunMerge(GateRef gate);
    bool RunLoopBegin(GateRef gate);
    bool RunLoopBack(GateRef gate);
    bool RunValueSelector(GateRef gate);
    bool RunDependSelector(GateRef gate);
    bool RunDependRelay(GateRef gate);
    bool RunDependAnd(GateRef gate);
    bool RunJSBytecode(GateRef gate);
    bool RunIfSuccess(GateRef gate);
    bool RunIfException(GateRef gate);
    bool RunGetException(GateRef gate);
    bool RunRuntimeCall(GateRef gate);
    bool RunNoGCRuntimeCall(GateRef gate);
    bool RunBytecodeCall(GateRef gate);
    bool RunDebuggerBytecodeCall(GateRef gate);
    bool RunCall(GateRef gate);
    bool RunRuntimeCallWithArgv(GateRef gate);
    bool RunAlloca(GateRef gate);
    bool RunArg(GateRef gate);
    bool RunMutableData(GateRef gate);
    bool RunConstData(GateRef gate);
    bool RunRelocatableData(GateRef gate);
    bool RunConstant(GateRef gate);
    bool RunZExtToInt64(GateRef gate);
    bool RunZExtToInt32(GateRef gate);
    bool RunZExtToInt16(GateRef gate);
    bool RunZExtToArch(GateRef gate);
    bool RunSExtToInt64(GateRef gate);
    bool RunSExtToInt32(GateRef gate);
    bool RunSExtToArch(GateRef gate);
    bool RunTruncToInt32(GateRef gate);
    bool RunTruncToInt1(GateRef gate);
    bool RunTruncToInt16(GateRef gate);
    bool RunRev(GateRef gate);
    bool RunAdd(GateRef gate);
    bool RunSub(GateRef gate);
    bool RunMul(GateRef gate);
    bool RunExp(GateRef gate);
    bool RunSDiv(GateRef gate);
    bool RunSMod(GateRef gate);
    bool RunUDiv(GateRef gate);
    bool RunUMod(GateRef gate);
    bool RunFDiv(GateRef gate);
    bool RunFMod(GateRef gate);
    bool RunAnd(GateRef gate);
    bool RunXor(GateRef gate);
    bool RunOr(GateRef gate);
    bool RunLSL(GateRef gate);
    bool RunLSR(GateRef gate);
    bool RunASR(GateRef gate);
    bool RunSLT(GateRef gate);
    bool RunSLE(GateRef gate);
    bool RunSGT(GateRef gate);
    bool RunSGE(GateRef gate);
    bool RunULT(GateRef gate);
    bool RunULE(GateRef gate);
    bool RunUGT(GateRef gate);
    bool RunUGE(GateRef gate);
    bool RunFLT(GateRef gate);
    bool RunFLE(GateRef gate);
    bool RunFGT(GateRef gate);
    bool RunFGE(GateRef gate);
    bool RunEQ(GateRef gate);
    bool RunNE(GateRef gate);
    bool RunLoad(GateRef gate);
    bool RunStore(GateRef gate);
    bool RunTaggedToInt64(GateRef gate);
    bool RunInt64ToTagged(GateRef gate);
    bool RunSignedIntToFloat(GateRef gate);
    bool RunUnsignedIntToFloat(GateRef gate);
    bool RunFloatToSignedInt(GateRef gate);
    bool RunUnsignedFloatToInt(GateRef gate);
    bool RunBitCast(GateRef gate);
};

class SubgraphRewriteRule {
public:
    void Initialize(Circuit *circuit);
    virtual bool Run(GateRef gate) = 0;

protected:
    Circuit *circuit_ = nullptr;
};

class SubgraphRewriteRuleCP : public SubgraphRewriteRule {
public:
    bool Run(GateRef gate) override;
    bool RunAdd(GateRef gate);
    bool RunSub(GateRef gate);
};

class LatticeEquationsSystemSolverFramework {
public:
    explicit LatticeEquationsSystemSolverFramework(LatticeUpdateRule *latticeUpdateRule);
    bool Run(Circuit *circuit, bool enableLogging = false);
    [[nodiscard]] const ValueLattice &GetValueLattice(GateRef gate) const;
    [[nodiscard]] const ReachabilityLattice &GetReachabilityLattice(GateRef gate) const;

private:
    Circuit *circuit_;
    LatticeUpdateRule *latticeUpdateRule_;
    std::map<GateRef, ValueLattice> valueLatticesMap_;
    std::map<GateRef, ReachabilityLattice> reachabilityLatticesMap_;
};

class SubGraphRewriteFramework {
public:
    explicit SubGraphRewriteFramework(SubgraphRewriteRule *subgraphRewriteRule);
    bool Run(Circuit *circuit, bool enableLogging = false);

private:
    Circuit *circuit_;
    SubgraphRewriteRule *subgraphRewriteRule_;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_OPTIMIZER_H_
