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

#include "circuit_optimizer.h"

#include <utility>

namespace panda::ecmascript::kungfu {
ValueLattice::ValueLattice() : value_(0), status_(LatticeStatus::TOP)
{
}

ValueLattice::ValueLattice(LatticeStatus status) : value_(0), status_(status)
{
}

ValueLattice::ValueLattice(uint64_t value) : value_(value), status_(LatticeStatus::MID)
{
}

bool ValueLattice::IsTop() const
{
    return GetStatus() == LatticeStatus::TOP;
}

bool ValueLattice::IsMid() const
{
    return GetStatus() == LatticeStatus::MID;
}

bool ValueLattice::IsBot() const
{
    return GetStatus() == LatticeStatus::BOT;
}

LatticeStatus ValueLattice::GetStatus() const
{
    return status_;
}

std::optional<uint64_t> ValueLattice::GetValue() const
{
    if (IsTop() || IsBot()) {
        return std::nullopt;
    }
    return value_;
}

ValueLattice ValueLattice::Meet(const ValueLattice &other)
{
    if (this->IsTop()) {
        return other;
    }
    if (other.IsTop()) {
        return *this;
    }
    if (this->IsBot() || other.IsBot()) {
        return ValueLattice(LatticeStatus::BOT);
    }
    // both are single
    if (this->GetValue().value() != other.GetValue().value()) {
        return ValueLattice(LatticeStatus::BOT);
    }
    return *this;
}

bool ValueLattice::operator==(const ValueLattice &other) const
{
    if (this->IsTop() && other.IsTop()) {
        return true;
    }
    if (this->IsBot() && other.IsBot()) {
        return true;
    }
    if (this->IsMid() && other.IsMid()) {
        return this->GetValue().value() == other.GetValue().value();
    }
    return false;
}

bool ValueLattice::operator!=(const ValueLattice &other) const
{
    return !(*this == other);
}

bool ValueLattice::operator<(const ValueLattice &other) const
{
    if (this->IsMid() && other.IsTop()) {
        return true;
    }
    if (this->IsBot() && other.IsMid()) {
        return true;
    }
    if (this->IsBot() && other.IsTop()) {
        return true;
    }
    return false;
}

bool ValueLattice::operator>(const ValueLattice &other) const
{
    return !(*this < other);
}

bool ValueLattice::operator<=(const ValueLattice &other) const
{
    return (*this < other) || (*this == other);
}

bool ValueLattice::operator>=(const ValueLattice &other) const
{
    return (*this > other) || (*this == other);
}

ValueLattice ValueLattice::Implies(const ValueLattice &other) const
{
    if (!this->IsTop()) {
        return other;
    }
    return ValueLattice(LatticeStatus::TOP);
}

void ValueLattice::Print(std::ostream &os) const
{
    if (IsTop()) {
        os << "TOP";
    } else {
        if (IsBot()) {
            os << "BOT";
        } else {
            os << GetValue().value();
        }
    }
}

ReachabilityLattice::ReachabilityLattice() : reachable_(false)
{
}

ReachabilityLattice::ReachabilityLattice(bool reachable) : reachable_(reachable)
{
}

bool ReachabilityLattice::IsReachable() const
{
    return reachable_;
}

bool ReachabilityLattice::IsUnreachable() const
{
    return !reachable_;
}

bool ReachabilityLattice::operator==(const ReachabilityLattice &other) const
{
    return this->IsReachable() == other.IsReachable();
}

bool ReachabilityLattice::operator!=(const ReachabilityLattice &other) const
{
    return !(*this == other);
}

ReachabilityLattice ReachabilityLattice::operator+(const ReachabilityLattice &other) const
{
    return ReachabilityLattice(this->IsReachable() || other.IsReachable());
}

ReachabilityLattice ReachabilityLattice::operator*(const ReachabilityLattice &other) const
{
    return ReachabilityLattice(this->IsReachable() && other.IsReachable());
}

ValueLattice ReachabilityLattice::Implies(const ValueLattice &other) const
{
    if (this->IsReachable()) {
        return other;
    }
    return ValueLattice(LatticeStatus::TOP);
}

void ReachabilityLattice::Print(std::ostream &os) const
{
    if (this->IsReachable()) {
        os << "reachable";
    } else {
        os << "unreachable";
    }
}

void LatticeUpdateRule::Initialize(Circuit *circuit,
                                   std::function<ValueLattice &(GateRef)> valueLattice,
                                   std::function<ReachabilityLattice &(GateRef)> reachabilityLattice)
{
    circuit_ = circuit;
    valueLatticeMap_ = std::move(valueLattice);
    reachabilityLatticeMap_ = std::move(reachabilityLattice);
}

bool LatticeUpdateRule::UpdateValueLattice(GateRef gate, const ValueLattice &valueLattice)
{
    if (valueLatticeMap_(gate) != valueLattice) {
        valueLatticeMap_(gate) = valueLattice;
        return true;
    }
    return false;
}

bool LatticeUpdateRule::UpdateReachabilityLattice(GateRef gate, const ReachabilityLattice &reachabilityLattice)
{
    if (reachabilityLatticeMap_(gate) != reachabilityLattice) {
        reachabilityLatticeMap_(gate) = reachabilityLattice;
        return true;
    }
    return false;
}

bool LatticeUpdateRuleSCCP::Run(GateRef gate)
{
    const std::map<OpCode::Op, std::function<bool(void)>> functionTable = {
        {OpCode::CIRCUIT_ROOT, [&]() -> bool { return RunCircuitRoot(gate); }},
        {OpCode::STATE_ENTRY, [&]() -> bool { return RunStateEntry(gate); }},
        {OpCode::DEPEND_ENTRY, [&]() -> bool { return RunDependEntry(gate); }},
        {OpCode::FRAMESTATE_ENTRY, [&]() -> bool { return RunFrameStateEntry(gate); }},
        {OpCode::RETURN_LIST, [&]() -> bool { return RunReturnList(gate); }},
        {OpCode::THROW_LIST, [&]() -> bool { return RunThrowList(gate); }},
        {OpCode::CONSTANT_LIST, [&]() -> bool { return RunConstantList(gate); }},
        {OpCode::ALLOCA_LIST, [&]() -> bool { return RunAllocaList(gate); }},
        {OpCode::ARG_LIST, [&]() -> bool { return RunArgList(gate); }},
        {OpCode::RETURN, [&]() -> bool { return RunReturn(gate); }},
        {OpCode::RETURN_VOID, [&]() -> bool { return RunReturnVoid(gate); }},
        {OpCode::THROW, [&]() -> bool { return RunThrow(gate); }},
        {OpCode::ORDINARY_BLOCK, [&]() -> bool { return RunOrdinaryBlock(gate); }},
        {OpCode::IF_BRANCH, [&]() -> bool { return RunIfBranch(gate); }},
        {OpCode::SWITCH_BRANCH, [&]() -> bool { return RunSwitchBranch(gate); }},
        {OpCode::IF_TRUE, [&]() -> bool { return RunIfTrue(gate); }},
        {OpCode::IF_FALSE, [&]() -> bool { return RunIfFalse(gate); }},
        {OpCode::SWITCH_CASE, [&]() -> bool { return RunSwitchCase(gate); }},
        {OpCode::DEFAULT_CASE, [&]() -> bool { return RunDefaultCase(gate); }},
        {OpCode::MERGE, [&]() -> bool { return RunMerge(gate); }},
        {OpCode::LOOP_BEGIN, [&]() -> bool { return RunLoopBegin(gate); }},
        {OpCode::LOOP_BACK, [&]() -> bool { return RunLoopBack(gate); }},
        {OpCode::VALUE_SELECTOR, [&]() -> bool { return RunValueSelector(gate); }},
        {OpCode::DEPEND_SELECTOR, [&]() -> bool { return RunDependSelector(gate); }},
        {OpCode::DEPEND_RELAY, [&]() -> bool { return RunDependRelay(gate); }},
        {OpCode::DEPEND_AND, [&]() -> bool { return RunDependAnd(gate); }},
        {OpCode::JS_BYTECODE, [&]() -> bool { return RunJSBytecode(gate); }},
        {OpCode::IF_SUCCESS, [&]() -> bool { return RunIfSuccess(gate); }},
        {OpCode::IF_EXCEPTION, [&]() -> bool { return RunIfException(gate); }},
        {OpCode::GET_EXCEPTION, [&]() -> bool { return RunGetException(gate); }},
        {OpCode::RUNTIME_CALL, [&]() -> bool { return RunRuntimeCall(gate); }},
        {OpCode::NOGC_RUNTIME_CALL, [&]() -> bool { return RunNoGCRuntimeCall(gate); }},
        {OpCode::BYTECODE_CALL, [&]() -> bool { return RunBytecodeCall(gate); }},
        {OpCode::DEBUGGER_BYTECODE_CALL, [&]() -> bool { return RunDebuggerBytecodeCall(gate); }},
        {OpCode::CALL, [&]() -> bool { return RunCall(gate); }},
        {OpCode::RUNTIME_CALL_WITH_ARGV, [&]() -> bool { return RunRuntimeCallWithArgv(gate); }},
        {OpCode::ALLOCA, [&]() -> bool { return RunAlloca(gate); }},
        {OpCode::ARG, [&]() -> bool { return RunArg(gate); }},
        {OpCode::MUTABLE_DATA, [&]() -> bool { return RunMutableData(gate); }},
        {OpCode::CONST_DATA, [&]() -> bool { return RunConstData(gate); }},
        {OpCode::RELOCATABLE_DATA, [&]() -> bool { return RunRelocatableData(gate); }},
        {OpCode::CONSTANT, [&]() -> bool { return RunConstant(gate); }},
        {OpCode::ZEXT_TO_INT64, [&]() -> bool { return RunZExtToInt64(gate); }},
        {OpCode::ZEXT_TO_INT32, [&]() -> bool { return RunZExtToInt32(gate); }},
        {OpCode::ZEXT_TO_INT16, [&]() -> bool { return RunZExtToInt16(gate); }},
        {OpCode::ZEXT_TO_ARCH, [&]() -> bool { return RunZExtToArch(gate); }},
        {OpCode::SEXT_TO_INT64, [&]() -> bool { return RunSExtToInt64(gate); }},
        {OpCode::SEXT_TO_INT32, [&]() -> bool { return RunSExtToInt32(gate); }},
        {OpCode::SEXT_TO_ARCH, [&]() -> bool { return RunSExtToArch(gate); }},
        {OpCode::TRUNC_TO_INT32, [&]() -> bool { return RunTruncToInt32(gate); }},
        {OpCode::TRUNC_TO_INT1, [&]() -> bool { return RunTruncToInt1(gate); }},
        {OpCode::TRUNC_TO_INT16, [&]() -> bool { return RunTruncToInt16(gate); }},
        {OpCode::REV, [&]() -> bool { return RunRev(gate); }},
        {OpCode::ADD, [&]() -> bool { return RunAdd(gate); }},
        {OpCode::SUB, [&]() -> bool { return RunSub(gate); }},
        {OpCode::MUL, [&]() -> bool { return RunMul(gate); }},
        {OpCode::EXP, [&]() -> bool { return RunExp(gate); }},
        {OpCode::SDIV, [&]() -> bool { return RunSDiv(gate); }},
        {OpCode::SMOD, [&]() -> bool { return RunSMod(gate); }},
        {OpCode::UDIV, [&]() -> bool { return RunUDiv(gate); }},
        {OpCode::UMOD, [&]() -> bool { return RunUMod(gate); }},
        {OpCode::FDIV, [&]() -> bool { return RunFDiv(gate); }},
        {OpCode::FMOD, [&]() -> bool { return RunFMod(gate); }},
        {OpCode::AND, [&]() -> bool { return RunAnd(gate); }},
        {OpCode::XOR, [&]() -> bool { return RunXor(gate); }},
        {OpCode::OR, [&]() -> bool { return RunOr(gate); }},
        {OpCode::LSL, [&]() -> bool { return RunLSL(gate); }},
        {OpCode::LSR, [&]() -> bool { return RunLSR(gate); }},
        {OpCode::ASR, [&]() -> bool { return RunASR(gate); }},
        {OpCode::SLT, [&]() -> bool { return RunSLT(gate); }},
        {OpCode::SLE, [&]() -> bool { return RunSLE(gate); }},
        {OpCode::SGT, [&]() -> bool { return RunSGT(gate); }},
        {OpCode::SGE, [&]() -> bool { return RunSGE(gate); }},
        {OpCode::ULT, [&]() -> bool { return RunULT(gate); }},
        {OpCode::ULE, [&]() -> bool { return RunULE(gate); }},
        {OpCode::UGT, [&]() -> bool { return RunUGT(gate); }},
        {OpCode::UGE, [&]() -> bool { return RunUGE(gate); }},
        {OpCode::FLT, [&]() -> bool { return RunFLT(gate); }},
        {OpCode::FLE, [&]() -> bool { return RunFLE(gate); }},
        {OpCode::FGT, [&]() -> bool { return RunFGT(gate); }},
        {OpCode::FGE, [&]() -> bool { return RunFGE(gate); }},
        {OpCode::EQ, [&]() -> bool { return RunEQ(gate); }},
        {OpCode::NE, [&]() -> bool { return RunNE(gate); }},
        {OpCode::LOAD, [&]() -> bool { return RunLoad(gate); }},
        {OpCode::STORE, [&]() -> bool { return RunStore(gate); }},
        {OpCode::TAGGED_TO_INT64, [&]() -> bool { return RunTaggedToInt64(gate); }},
        {OpCode::INT64_TO_TAGGED, [&]() -> bool { return RunInt64ToTagged(gate); }},
        {OpCode::SIGNED_INT_TO_FLOAT, [&]() -> bool { return RunSignedIntToFloat(gate); }},
        {OpCode::UNSIGNED_INT_TO_FLOAT, [&]() -> bool { return RunUnsignedIntToFloat(gate); }},
        {OpCode::FLOAT_TO_SIGNED_INT, [&]() -> bool { return RunFloatToSignedInt(gate); }},
        {OpCode::UNSIGNED_FLOAT_TO_INT, [&]() -> bool { return RunUnsignedFloatToInt(gate); }},
        {OpCode::BITCAST, [&]() -> bool { return RunBitCast(gate); }},
    };
    return functionTable.at(GateAccessor(circuit_).GetOpCode(gate))();
}

bool LatticeUpdateRuleSCCP::RunCircuitRoot([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunStateEntry(GateRef gate)
{
    return UpdateReachabilityLattice(gate, ReachabilityLattice(true));
}

bool LatticeUpdateRuleSCCP::RunDependEntry(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunFrameStateEntry([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunReturnList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunThrowList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunConstantList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunAllocaList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunArgList([[maybe_unused]] GateRef gate)
{
    return false;
}

bool LatticeUpdateRuleSCCP::RunReturn(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunReturnVoid(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunThrow(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunOrdinaryBlock(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunIfBranch(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunSwitchBranch(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunIfTrue(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    const bool predicateMayBeTrue =
        valueLatticeMap_(GateAccessor(circuit_).GetIn(previousState, 1)) <= ValueLattice(1);
    return UpdateReachabilityLattice(gate,
                                     reachabilityLatticeMap_(previousState)
                                         * ReachabilityLattice(predicateMayBeTrue));
}

bool LatticeUpdateRuleSCCP::RunIfFalse(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    const bool predicateMayBeFalse =
        valueLatticeMap_(GateAccessor(circuit_).GetIn(previousState, 1)) <= ValueLattice(0);
    return UpdateReachabilityLattice(gate,
                                     reachabilityLatticeMap_(previousState)
                                         * ReachabilityLattice(predicateMayBeFalse));
}

bool LatticeUpdateRuleSCCP::RunSwitchCase(GateRef gate)
{
    const bool valueMayMatch =
        valueLatticeMap_(GateAccessor(circuit_).GetIn(GateAccessor(circuit_).GetIn(gate, 0), 1))
            <= ValueLattice(GateAccessor(circuit_).GetBitField(gate));
    return UpdateReachabilityLattice(gate,
                                     reachabilityLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0))
                                         * ReachabilityLattice(valueMayMatch));
}

bool LatticeUpdateRuleSCCP::RunDefaultCase(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunMerge(GateRef gate)
{
    ReachabilityLattice reachable;
    for (const auto &input : GateAccessor(circuit_).Ins(gate)) {
        reachable = reachable + reachabilityLatticeMap_(input);
    }
    return UpdateReachabilityLattice(gate, reachable);
}

bool LatticeUpdateRuleSCCP::RunLoopBegin(GateRef gate)
{
    ReachabilityLattice reachable;
    for (const auto &input : GateAccessor(circuit_).Ins(gate)) {
        reachable = reachable + reachabilityLatticeMap_(input);
    }
    return UpdateReachabilityLattice(gate, reachable);
}

bool LatticeUpdateRuleSCCP::RunLoopBack(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunValueSelector(GateRef gate)
{
    const auto relatedState = GateAccessor(circuit_).GetIn(gate, 0);
    ValueLattice value;
    size_t cnt = 0;
    for (const auto &input : GateAccessor(circuit_).Ins(gate)) {
        if (cnt > 0) {
            value = value.Meet(reachabilityLatticeMap_(
                GateAccessor(circuit_).GetIn(relatedState, cnt - 1)).Implies(valueLatticeMap_(input)));
        }
        cnt++;
    }
    return UpdateValueLattice(gate, value);
}

bool LatticeUpdateRuleSCCP::RunDependSelector(GateRef gate)
{
    const auto relatedState = GateAccessor(circuit_).GetIn(gate, 0);
    ValueLattice value;
    size_t cnt = 0;
    for (const auto &input : GateAccessor(circuit_).Ins(gate)) {
        if (cnt > 0) {
            value = value.Meet(reachabilityLatticeMap_(
                GateAccessor(circuit_).GetIn(relatedState, cnt - 1)).Implies(valueLatticeMap_(input)));
        }
        cnt++;
    }
    if (!value.IsTop()) {
        value = ValueLattice(LatticeStatus::BOT);
    }
    return UpdateValueLattice(gate, value);
}

bool LatticeUpdateRuleSCCP::RunDependRelay(GateRef gate)
{
    const auto relatedState = GateAccessor(circuit_).GetIn(gate, 0);
    ValueLattice value = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (!value.IsTop()) {
        value = ValueLattice(LatticeStatus::BOT);
    }
    return UpdateValueLattice(gate, reachabilityLatticeMap_(relatedState).Implies(value));
}

bool LatticeUpdateRuleSCCP::RunDependAnd(GateRef gate)
{
    ValueLattice value = ValueLattice(LatticeStatus::BOT);
    for (const auto &input : GateAccessor(circuit_).Ins(gate)) {
        if (valueLatticeMap_(input).IsTop()) {
            value = ValueLattice(LatticeStatus::TOP);
        }
    }
    return UpdateValueLattice(gate, value);
}

bool LatticeUpdateRuleSCCP::RunJSBytecode(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState))
        || UpdateValueLattice(gate, reachabilityLatticeMap_(gate).Implies(ValueLattice(LatticeStatus::BOT)));
}

bool LatticeUpdateRuleSCCP::RunIfSuccess(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunIfException(GateRef gate)
{
    const auto previousState = GateAccessor(circuit_).GetIn(gate, 0);
    return UpdateReachabilityLattice(gate, reachabilityLatticeMap_(previousState));
}

bool LatticeUpdateRuleSCCP::RunGetException(GateRef gate)
{
    return UpdateValueLattice(gate, valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0)).Implies(
        ValueLattice(LatticeStatus::BOT)));
}

bool LatticeUpdateRuleSCCP::RunRuntimeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunNoGCRuntimeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunBytecodeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunDebuggerBytecodeCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunCall(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunRuntimeCallWithArgv(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunAlloca(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunArg(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunMutableData(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunConstData(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunRelocatableData(GateRef gate)
{
    return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
}

bool LatticeUpdateRuleSCCP::RunConstant(GateRef gate)
{
    const auto constantValue = ValueLattice(GateAccessor(circuit_).GetBitField(gate));
    return UpdateValueLattice(gate, constantValue);
}

bool LatticeUpdateRuleSCCP::RunZExtToInt64(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunZExtToInt32(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunZExtToInt16(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunZExtToArch(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunSExtToInt64(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunSExtToInt32(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunSExtToArch(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunTruncToInt32(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunTruncToInt1(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunTruncToInt16(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunRev(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    if (operandA.IsMid()) {
        return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value()));
    }
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunAdd(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() + operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunSub(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() - operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunMul(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() * operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunExp(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    auto fastPow = [](uint64_t a, uint64_t b) {
        uint64_t result = 1;
        uint64_t power = a;
        while (b) {
            if (b & 1) {
                result = result * power;
            }
            power = power * power;
            b >>= 1;
        }
        return result;
    };
    return UpdateValueLattice(gate, ValueLattice(fastPow(operandA.GetValue().value(), operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSDiv(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               / static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSMod(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               % static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunUDiv(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() / operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunUMod(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() % operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunFDiv(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(bit_cast<uint64_t>((bit_cast<double>(operandA.GetValue().value()) /
                                  bit_cast<double>(operandB.GetValue().value())))));
}

bool LatticeUpdateRuleSCCP::RunFMod(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(bit_cast<uint64_t>(fmod(bit_cast<double>(operandA.GetValue().value()),
                                                                   bit_cast<double>(operandB.GetValue().value())))));
}

bool LatticeUpdateRuleSCCP::RunAnd(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() & operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunXor(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() ^ operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunOr(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() | operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunLSL(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() << operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunLSR(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() >> operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunASR(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               >> static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSLT(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               < static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSLE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               <= static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSGT(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               > static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunSGE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<int64_t>(operandA.GetValue().value())
                                               >= static_cast<int64_t>(operandB.GetValue().value())));
}

bool LatticeUpdateRuleSCCP::RunULT(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() < operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunULE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() <= operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunUGT(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() > operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunUGE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() >= operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunFLT(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<uint64_t>(bit_cast<double>(operandA.GetValue().value())
                                  < bit_cast<double>(operandB.GetValue().value()))));
}

bool LatticeUpdateRuleSCCP::RunFLE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<uint64_t>(bit_cast<double>(operandA.GetValue().value())
                                  <= bit_cast<double>(operandB.GetValue().value()))));
}

bool LatticeUpdateRuleSCCP::RunFGT(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<uint64_t>(bit_cast<double>(operandA.GetValue().value())
                                  > bit_cast<double>(operandB.GetValue().value()))));
}

bool LatticeUpdateRuleSCCP::RunFGE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate,
                              ValueLattice(static_cast<uint64_t>(bit_cast<double>(operandA.GetValue().value())
                                  >= bit_cast<double>(operandB.GetValue().value()))));
}

bool LatticeUpdateRuleSCCP::RunEQ(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() == operandB.GetValue().value()));
}

bool LatticeUpdateRuleSCCP::RunNE(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    const ValueLattice &operandB = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 1));
    if (operandA.IsTop() || operandB.IsTop()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::TOP));
    }
    if (operandA.IsBot() || operandB.IsBot()) {
        return UpdateValueLattice(gate, ValueLattice(LatticeStatus::BOT));
    }
    return UpdateValueLattice(gate, ValueLattice(operandA.GetValue().value() != operandB.GetValue().value() ? 1 : 0));
}

bool LatticeUpdateRuleSCCP::RunLoad(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunStore(GateRef gate)
{
    return LatticeUpdateRuleSCCP::RunDependAnd(gate);
}

bool LatticeUpdateRuleSCCP::RunTaggedToInt64(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunInt64ToTagged(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunSignedIntToFloat(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunUnsignedIntToFloat(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunFloatToSignedInt(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunUnsignedFloatToInt(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

bool LatticeUpdateRuleSCCP::RunBitCast(GateRef gate)
{
    const ValueLattice &operandA = valueLatticeMap_(GateAccessor(circuit_).GetIn(gate, 0));
    return UpdateValueLattice(gate, operandA);
}

void SubgraphRewriteRule::Initialize(Circuit *circuit)
{
    circuit_ = circuit;
}

bool SubgraphRewriteRuleCP::Run(GateRef gate)
{
    const std::map<OpCode::Op, std::function<bool(void)>> functionTable = {
        {OpCode::ADD, [&]() -> bool { return RunAdd(gate); }},
        {OpCode::SUB, [&]() -> bool { return RunSub(gate); }},
    };
    if (!functionTable.count(GateAccessor(circuit_).GetOpCode(gate))) {
        return false;
    }
    return functionTable.at(GateAccessor(circuit_).GetOpCode(gate))();
}

bool SubgraphRewriteRuleCP::RunAdd(GateRef gate)
{
    const auto &operandA = GateAccessor(circuit_).GetIn(gate, 0);
    const auto &operandB = GateAccessor(circuit_).GetIn(gate, 1);
    if (GateAccessor(circuit_).GetOpCode(operandA) == OpCode(OpCode::CONSTANT)
        && GateAccessor(circuit_).GetOpCode(operandB) == OpCode(OpCode::CONSTANT)) {
        circuit_->DeleteIn(gate, 0);
        circuit_->DeleteIn(gate, 1);
        GateAccessor(circuit_).SetOpCode(gate, OpCode(OpCode::CONSTANT));
        circuit_->NewIn(gate, 0, Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST)));
        const auto valueA = GateAccessor(circuit_).GetBitField(operandA);
        const auto valueB = GateAccessor(circuit_).GetBitField(operandB);
        GateAccessor(circuit_).SetBitField(gate, valueA + valueB);
        return true;
    }
    return false;
}

bool SubgraphRewriteRuleCP::RunSub(GateRef gate)
{
    const auto &operandA = GateAccessor(circuit_).GetIn(gate, 0);
    const auto &operandB = GateAccessor(circuit_).GetIn(gate, 1);
    if (GateAccessor(circuit_).GetOpCode(operandA) == OpCode(OpCode::CONSTANT)
        && GateAccessor(circuit_).GetOpCode(operandB) == OpCode(OpCode::CONSTANT)) {
        circuit_->DeleteIn(gate, 0);
        circuit_->DeleteIn(gate, 1);
        GateAccessor(circuit_).SetOpCode(gate, OpCode(OpCode::CONSTANT));
        circuit_->NewIn(gate, 0, Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST)));
        const auto valueA = GateAccessor(circuit_).GetBitField(operandA);
        const auto valueB = GateAccessor(circuit_).GetBitField(operandB);
        GateAccessor(circuit_).SetBitField(gate, valueA - valueB);
        return true;
    }
    return false;
}

LatticeEquationsSystemSolverFramework::LatticeEquationsSystemSolverFramework(LatticeUpdateRule *latticeUpdateRule)
    : circuit_(nullptr), latticeUpdateRule_(latticeUpdateRule)
{
}

bool LatticeEquationsSystemSolverFramework::Run(Circuit *circuit, bool enableLogging)
{
    circuit_ = circuit;
    auto valueLatticeMapFunction = [&](GateRef gate) -> ValueLattice & {
        return valueLatticesMap_[gate];
    };
    auto reachabilityLatticeMapFunction = [&](GateRef gate) -> ReachabilityLattice & {
        return reachabilityLatticesMap_[gate];
    };
    latticeUpdateRule_->Initialize(circuit_, valueLatticeMapFunction, reachabilityLatticeMapFunction);
    std::deque<GateRef> workList;
    std::set<GateRef> workSet;
    for (auto gate : circuit_->GetAllGates()) {
        workList.push_back(gate);
        workSet.insert(gate);
    }
    while (!workList.empty()) {
        const auto gate = workList.front();
        workList.pop_front();
        workSet.erase(gate);
        if (latticeUpdateRule_->Run(gate) || GateAccessor(circuit_).GetOpCode(gate).IsCFGMerge()) {
            for (const auto &output : GateAccessor(circuit_).Uses(gate)) {
                if (!workSet.count(output)) {
                    workList.push_back(output);  // work queue
                    workSet.insert(output);
                }
            }
        }
    }
    if (enableLogging) {
        for (auto gate : circuit_->GetAllGates()) {
            if (valueLatticesMap_.count(gate)) {
                if (valueLatticesMap_.at(gate).IsTop()) {
                    std::cerr << "[Top]";
                } else if (valueLatticesMap_.at(gate).IsBot()) {
                    std::cerr << "[Bot]";
                } else {
                    std::cerr << "[" << valueLatticesMap_.at(gate).GetValue().value() << "]";
                }
            }
            if (reachabilityLatticesMap_.count(gate)) {
                if (reachabilityLatticesMap_.at(gate).IsReachable()) {
                    std::cerr << "[Reachable]";
                } else {
                    std::cerr << "[Unreachable]";
                }
            }
            std::cerr << " ";
            GateAccessor(circuit_).Print(gate);
        }
    }
    return true;
}

const ValueLattice &LatticeEquationsSystemSolverFramework::GetValueLattice(GateRef gate) const
{
    return valueLatticesMap_.at(gate);
}

const ReachabilityLattice &LatticeEquationsSystemSolverFramework::GetReachabilityLattice(GateRef gate) const
{
    return reachabilityLatticesMap_.at(gate);
}

SubGraphRewriteFramework::SubGraphRewriteFramework(SubgraphRewriteRule *subgraphRewriteRule)
    : circuit_(nullptr), subgraphRewriteRule_(subgraphRewriteRule)
{
}

bool SubGraphRewriteFramework::Run(Circuit *circuit, bool enableLogging)
{
    circuit_ = circuit;
    subgraphRewriteRule_->Initialize(circuit_);
    std::deque<GateRef> workList;
    std::set<GateRef> workSet;
    for (auto gate : circuit_->GetAllGates()) {
        workList.push_back(gate);
        workSet.insert(gate);
    }
    while (!workList.empty()) {
        const auto gate = workList.front();
        workList.pop_front();
        workSet.erase(gate);
        if (subgraphRewriteRule_->Run(gate)) {
            for (const auto &output : GateAccessor(circuit_).Uses(gate)) {
                if (!workSet.count(output)) {
                    workList.push_front(output);  // work stack
                    workSet.insert(output);
                }
            }
        }
    }
    if (enableLogging) {
        for (auto gate : circuit_->GetAllGates()) {
            GateAccessor(circuit_).Print(gate);
        }
    }
    return true;
}
}  // namespace panda::ecmascript::kungfu
