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

#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"

namespace panda::ecmascript::kungfu {
constexpr size_t ONE_DEPEND = 1;
constexpr size_t MANY_DEPEND = 2;
constexpr size_t NO_DEPEND = 0;
// NOLINTNEXTLINE(readability-function-size)
Properties OpCode::GetProperties() const
{
// general schema: [STATE]s + [DEPEND]s + [VALUE]s + [ROOT]
// GENERAL_STATE for any opcode match in
// {IF_TRUE, IF_FALSE, SWITCH_CASE, DEFAULT_CASE, MERGE, LOOP_BEGIN, STATE_ENTRY}
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STATE(...) (std::make_pair(std::vector<OpCode>{__VA_ARGS__}, false))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VALUE(...) (std::make_pair(std::vector<MachineType>{__VA_ARGS__}, false))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MANY_STATE(...) (std::make_pair(std::vector<OpCode>{__VA_ARGS__}, true))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MANY_VALUE(...) (std::make_pair(std::vector<MachineType>{__VA_ARGS__}, true))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_STATE (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_VALUE (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_ROOT (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GENERAL_STATE (NOP)
    switch (op_) {
        // SHARED
        case NOP:
        case CIRCUIT_ROOT:
            return {NOVALUE, NO_STATE, NO_DEPEND, NO_VALUE, NO_ROOT};
        case STATE_ENTRY:
        case DEPEND_ENTRY:
        case FRAMESTATE_ENTRY:
        case RETURN_LIST:
        case THROW_LIST:
        case CONSTANT_LIST:
        case ALLOCA_LIST:
        case ARG_LIST:
            return {NOVALUE, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CIRCUIT_ROOT)};
        case RETURN:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, VALUE(ANYVALUE), OpCode(RETURN_LIST)};
        case RETURN_VOID:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, NO_VALUE, OpCode(RETURN_LIST)};
        case THROW:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, VALUE(JSMachineType()), OpCode(THROW_LIST)};
        case ORDINARY_BLOCK:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case IF_BRANCH:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, VALUE(I1), NO_ROOT};
        case SWITCH_BRANCH:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case IF_TRUE:
        case IF_FALSE:
            return {NOVALUE, STATE(OpCode(IF_BRANCH)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case SWITCH_CASE:
        case DEFAULT_CASE:
            return {NOVALUE, STATE(OpCode(SWITCH_BRANCH)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case MERGE:
            return {NOVALUE, MANY_STATE(OpCode(GENERAL_STATE)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case LOOP_BEGIN:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE), OpCode(LOOP_BACK)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case LOOP_BACK:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case VALUE_SELECTOR:
            return {FLEX, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(FLEX), NO_ROOT};
        case DEPEND_SELECTOR:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), MANY_DEPEND, NO_VALUE, NO_ROOT};
        case DEPEND_RELAY:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, NO_VALUE, NO_ROOT};
        case DEPEND_AND:
            return {NOVALUE, NO_STATE, MANY_DEPEND, NO_VALUE, NO_ROOT};
        // High Level IR
        case JS_BYTECODE:
            return {FLEX, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT};
        case IF_SUCCESS:
        case IF_EXCEPTION:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case GET_EXCEPTION:
            return {I64, NO_STATE, ONE_DEPEND, NO_VALUE, NO_ROOT};
        case CHECK_POINT:
            return {NOVALUE, NO_STATE, ONE_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT};
        case FRAME_STATE:
            return {NOVALUE, NO_STATE, NO_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT};
        // Middle Level IR
        case RUNTIME_CALL:
        case NOGC_RUNTIME_CALL:
        case BYTECODE_CALL:
        case DEBUGGER_BYTECODE_CALL:
        case CALL:
        case RUNTIME_CALL_WITH_ARGV:
            return {FLEX, NO_STATE, ONE_DEPEND, MANY_VALUE(ANYVALUE, ANYVALUE), NO_ROOT};
        case ALLOCA:
            return {ARCH, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ALLOCA_LIST)};
        case ARG:
            return {FLEX, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case MUTABLE_DATA:
        case CONST_DATA:
            return {ARCH, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case RELOCATABLE_DATA:
            return {ARCH, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case CONSTANT:
            return {FLEX, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case ZEXT_TO_INT64:
            return {I64, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case ZEXT_TO_INT32:
            return {I32, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case ZEXT_TO_INT16:
            return {I16, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case ZEXT_TO_ARCH:
            return {ARCH, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case SEXT_TO_INT64:
            return {I64, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case SEXT_TO_INT32:
            return {I32, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case SEXT_TO_ARCH:
            return {ARCH, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case TRUNC_TO_INT32:
            return {I32, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case TRUNC_TO_INT1:
            return {I1, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case TRUNC_TO_INT16:
            return {I16, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case REV:
            return {FLEX, NO_STATE, NO_DEPEND, VALUE(FLEX), NO_ROOT};
        case ADD:
        case SUB:
        case MUL:
        case EXP:
        case SDIV:
        case SMOD:
        case UDIV:
        case UMOD:
        case FDIV:
        case FMOD:
        case AND:
        case XOR:
        case OR:
        case LSL:
        case LSR:
        case ASR:
            return {FLEX, NO_STATE, NO_DEPEND, VALUE(FLEX, FLEX), NO_ROOT};
        case SLT:
        case SLE:
        case SGT:
        case SGE:
        case ULT:
        case ULE:
        case UGT:
        case UGE:
        case FLT:
        case FLE:
        case FGT:
        case FGE:
        case EQ:
        case NE:
            return {I1, NO_STATE, NO_DEPEND, VALUE(ANYVALUE, ANYVALUE), NO_ROOT};
        case LOAD:
            return {FLEX, NO_STATE, ONE_DEPEND, VALUE(ARCH), NO_ROOT};
        case STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(ANYVALUE, ARCH), NO_ROOT};
        case TAGGED_TO_INT64:
            return {I64, NO_STATE, NO_DEPEND, VALUE(I64), NO_ROOT};
        case INT64_TO_TAGGED:
            return {I64, NO_STATE, NO_DEPEND, VALUE(I64), NO_ROOT};
        case SIGNED_INT_TO_FLOAT:
        case UNSIGNED_INT_TO_FLOAT:
            return {FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case FLOAT_TO_SIGNED_INT:
        case UNSIGNED_FLOAT_TO_INT:
            return {FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        case BITCAST:
            return {FLEX, NO_STATE, NO_DEPEND, VALUE(ANYVALUE), NO_ROOT};
        default:
            COMPILER_LOG(ERROR) << "Please complete OpCode properties (OpCode=" << op_ << ")";
            UNREACHABLE();
    }
#undef STATE
#undef VALUE
#undef MANY_STATE
#undef MANY_VALUE
#undef NO_STATE
#undef NO_VALUE
#undef NO_ROOT
#undef GENERAL_STATE
}

std::string OpCode::Str() const
{
    const std::map<GateOp, const char *> strMap = {
        {NOP, "NOP"},
        {CIRCUIT_ROOT, "CIRCUIT_ROOT"},
        {STATE_ENTRY, "STATE_ENTRY"},
        {DEPEND_ENTRY, "DEPEND_ENTRY"},
        {FRAMESTATE_ENTRY, "FRAMESTATE_ENTRY"},
        {RETURN_LIST, "RETURN_LIST"},
        {THROW_LIST, "THROW_LIST"},
        {CONSTANT_LIST, "CONSTANT_LIST"},
        {ALLOCA_LIST, "ALLOCA_LIST"},
        {ARG_LIST, "ARG_LIST"},
        {RETURN, "RETURN"},
        {RETURN_VOID, "RETURN_VOID"},
        {THROW, "THROW"},
        {ORDINARY_BLOCK, "ORDINARY_BLOCK"},
        {IF_BRANCH, "IF_BRANCH"},
        {SWITCH_BRANCH, "SWITCH_BRANCH"},
        {IF_TRUE, "IF_TRUE"},
        {IF_FALSE, "IF_FALSE"},
        {SWITCH_CASE, "SWITCH_CASE"},
        {DEFAULT_CASE, "DEFAULT_CASE"},
        {MERGE, "MERGE"},
        {LOOP_BEGIN, "LOOP_BEGIN"},
        {LOOP_BACK, "LOOP_BACK"},
        {VALUE_SELECTOR, "VALUE_SELECTOR"},
        {DEPEND_SELECTOR, "DEPEND_SELECTOR"},
        {DEPEND_RELAY, "DEPEND_RELAY"},
        {DEPEND_AND, "DEPEND_AND"},
        {JS_BYTECODE, "JS_BYTECODE"},
        {IF_SUCCESS, "IF_SUCCESS"},
        {IF_EXCEPTION, "IF_EXCEPTION"},
        {GET_EXCEPTION, "GET_EXCEPTION"},
        {CHECK_POINT, "CHECK_POINT"},
        {FRAME_STATE, "FRAME_STATE"},
        {RUNTIME_CALL, "RUNTIME_CALL"},
        {NOGC_RUNTIME_CALL, "NOGC_RUNTIME_CALL"},
        {CALL, "CALL"},
        {BYTECODE_CALL, "BYTECODE_CALL"},
        {DEBUGGER_BYTECODE_CALL, "DEBUGGER_BYTECODE_CALL"},
        {ALLOCA, "ALLOCA"},
        {ARG, "ARG"},
        {MUTABLE_DATA, "MUTABLE_DATA"},
        {RELOCATABLE_DATA, "RELOCATABLE_DATA"},
        {CONST_DATA, "CONST_DATA"},
        {CONSTANT, "CONSTANT"},
        {ZEXT_TO_INT64, "ZEXT_TO_INT64"},
        {ZEXT_TO_INT32, "ZEXT_TO_INT32"},
        {ZEXT_TO_INT16, "ZEXT_TO_INT16"},
        {ZEXT_TO_ARCH, "ZEXT_TO_ARCH"},
        {SEXT_TO_INT64, "SEXT_TO_INT64"},
        {SEXT_TO_INT32, "SEXT_TO_INT32"},
        {SEXT_TO_ARCH, "SEXT_TO_ARCH"},
        {TRUNC_TO_INT32, "TRUNC_TO_INT32"},
        {TRUNC_TO_INT1, "TRUNC_TO_INT1"},
        {TRUNC_TO_INT16, "TRUNC_TO_INT16"},
        {REV, "REV"},
        {ADD, "ADD"},
        {SUB, "SUB"},
        {MUL, "MUL"},
        {EXP, "EXP"},
        {SDIV, "SDIV"},
        {SMOD, "SMOD"},
        {UDIV, "UDIV"},
        {UMOD, "UMOD"},
        {FDIV, "FDIV"},
        {FMOD, "FMOD"},
        {AND, "AND"},
        {XOR, "XOR"},
        {OR, "OR"},
        {LSL, "LSL"},
        {LSR, "LSR"},
        {ASR, "ASR"},
        {SLT, "SLT"},
        {SLE, "SLE"},
        {SGT, "SGT"},
        {SGE, "SGE"},
        {ULT, "ULT"},
        {ULE, "ULE"},
        {UGT, "UGT"},
        {UGE, "UGE"},
        {FLT, "FLT"},
        {FLE, "FLE"},
        {FGT, "FGT"},
        {FGE, "FGE"},
        {EQ, "EQ"},
        {NE, "NE"},
        {LOAD, "LOAD"},
        {STORE, "STORE"},
        {TAGGED_TO_INT64, "TAGGED_TO_INT64"},
        {INT64_TO_TAGGED, "INT64_TO_TAGGED"},
        {SIGNED_INT_TO_FLOAT, "SIGNED_INT_TO_FLOAT"},
        {UNSIGNED_INT_TO_FLOAT, "UNSIGNED_INT_TO_FLOAT"},
        {FLOAT_TO_SIGNED_INT, "FLOAT_TO_SIGNED_INT"},
        {UNSIGNED_FLOAT_TO_INT, "UNSIGNED_FLOAT_TO_INT"},
        {BITCAST, "BITCAST"},
    };
    if (strMap.count(op_) > 0) {
        return strMap.at(op_);
    }
    return "OP-" + std::to_string(op_);
}

size_t OpCode::GetStateCount(BitField bitfield) const
{
    auto properties = GetProperties();
    auto stateProp = properties.statesIn;
    return stateProp.has_value() ? (stateProp->second ? bitfield : stateProp->first.size()) : 0;
}

size_t OpCode::GetDependCount(BitField bitfield) const
{
    const size_t manyDepend = 2;
    auto properties = GetProperties();
    auto dependProp = properties.dependsIn;
    return (dependProp == manyDepend) ? bitfield : dependProp;
}

size_t OpCode::GetInValueCount(BitField bitfield) const
{
    auto properties = GetProperties();
    auto valueProp = properties.valuesIn;
    return valueProp.has_value() ? (valueProp->second ? bitfield : valueProp->first.size()) : 0;
}

size_t OpCode::GetRootCount([[maybe_unused]] BitField bitfield) const
{
    auto properties = GetProperties();
    auto rootProp = properties.root;
    return rootProp.has_value() ? 1 : 0;
}

size_t OpCode::GetOpCodeNumIns(BitField bitfield) const
{
    return GetStateCount(bitfield) + GetDependCount(bitfield) + GetInValueCount(bitfield) + GetRootCount(bitfield);
}

MachineType OpCode::GetMachineType() const
{
    return GetProperties().returnValue;
}

MachineType OpCode::GetInMachineType(BitField bitfield, size_t idx) const
{
    auto valueProp = GetProperties().valuesIn;
    idx -= GetStateCount(bitfield);
    idx -= GetDependCount(bitfield);
    ASSERT(valueProp.has_value());
    if (valueProp->second) {
        return valueProp->first.at(std::min(idx, valueProp->first.size() - 1));
    }
    return valueProp->first.at(idx);
}

OpCode OpCode::GetInStateCode(size_t idx) const
{
    auto stateProp = GetProperties().statesIn;
    ASSERT(stateProp.has_value());
    if (stateProp->second) {
        return stateProp->first.at(std::min(idx, stateProp->first.size() - 1));
    }
    return stateProp->first.at(idx);
}

std::string MachineTypeToStr(MachineType machineType)
{
    switch (machineType) {
        case NOVALUE:
            return "NOVALUE";
        case ANYVALUE:
            return "ANYVALUE";
        case I1:
            return "I1";
        case I8:
            return "I8";
        case I16:
            return "I16";
        case I32:
            return "I32";
        case I64:
            return "I64";
        case F32:
            return "F32";
        case F64:
            return "F64";
        default:
            return "???";
    }
}

std::optional<std::pair<std::string, size_t>> Gate::CheckNullInput() const
{
    const auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        if (IsInGateNull(idx)) {
            return std::make_pair("In list contains null", idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckStateInput() const
{
    size_t stateStart = 0;
    size_t stateEnd = GetStateCount();
    for (size_t idx = stateStart; idx < stateEnd; idx++) {
        auto stateProp = GetOpCode().GetProperties().statesIn;
        ASSERT(stateProp.has_value());
        auto expectedIn = GetOpCode().GetInStateCode(idx);
        auto actualIn = GetInGateConst(idx)->GetOpCode();
        if (expectedIn == OpCode::NOP) {  // general
            if (!actualIn.IsGeneralState()) {
                return std::make_pair(
                    "State input does not match (expected:<General State> actual:" + actualIn.Str() + ")", idx);
            }
        } else {
            if (expectedIn != actualIn) {
                return std::make_pair(
                    "State input does not match (expected:" + expectedIn.Str() + " actual:" + actualIn.Str() + ")",
                    idx);
            }
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckValueInput() const
{
    size_t valueStart = GetStateCount() + GetDependCount();
    size_t valueEnd = valueStart + GetInValueCount();
    for (size_t idx = valueStart; idx < valueEnd; idx++) {
        auto expectedIn = GetOpCode().GetInMachineType(GetBitField(), idx);
        auto actualIn = GetInGateConst(idx)->GetOpCode().GetMachineType();
        if (expectedIn == MachineType::FLEX) {
            expectedIn = GetMachineType();
        }
        if (actualIn == MachineType::FLEX) {
            actualIn = GetInGateConst(idx)->GetMachineType();
        }

        if ((expectedIn != actualIn) && (expectedIn != ANYVALUE)) {
            return std::make_pair("Value input does not match (expected: " + MachineTypeToStr(expectedIn) +
                    " actual: " + MachineTypeToStr(actualIn) + ")",
                idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckDependInput() const
{
    size_t dependStart = GetStateCount();
    size_t dependEnd = dependStart + GetDependCount();
    for (size_t idx = dependStart; idx < dependEnd; idx++) {
        if (GetInGateConst(idx)->GetDependCount() == 0 &&
            GetInGateConst(idx)->GetOpCode() != OpCode::DEPEND_ENTRY) {
            return std::make_pair("Depend input is side-effect free", idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckStateOutput() const
{
    if (GetOpCode().IsState()) {
        size_t cnt = 0;
        const Gate *curGate = this;
        if (!curGate->IsFirstOutNull()) {
            const Out *curOut = curGate->GetFirstOutConst();
            if (curOut->IsStateEdge() && curOut->GetGateConst()->GetOpCode().IsState()) {
                cnt++;
            }
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                if (curOut->IsStateEdge() && curOut->GetGateConst()->GetOpCode().IsState()) {
                    cnt++;
                }
            }
        }
        size_t expected = 0;
        bool needCheck = true;
        if (GetOpCode().IsTerminalState()) {
            expected = 0;
        } else if (GetOpCode() == OpCode::IF_BRANCH || GetOpCode() == OpCode::JS_BYTECODE) {
            expected = 2; // 2: expected number of state out branches
        } else if (GetOpCode() == OpCode::SWITCH_BRANCH) {
            needCheck = false;
        } else {
            expected = 1;
        }
        if (needCheck && cnt != expected) {
            return std::make_pair("Number of state out branches is not valid (expected:" + std::to_string(expected) +
                    " actual:" + std::to_string(cnt) + ")",
                -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckBranchOutput() const
{
    std::map<std::pair<OpCode, BitField>, size_t> setOfOps;
    if (GetOpCode() == OpCode::IF_BRANCH || GetOpCode() == OpCode::SWITCH_BRANCH) {
        size_t cnt = 0;
        const Gate *curGate = this;
        if (!curGate->IsFirstOutNull()) {
            const Out *curOut = curGate->GetFirstOutConst();
            if (curOut->GetGateConst()->GetOpCode().IsState() && curOut->IsStateEdge()) {
                ASSERT(!curOut->GetGateConst()->GetOpCode().IsFixed());
                setOfOps[{curOut->GetGateConst()->GetOpCode(), curOut->GetGateConst()->GetBitField()}]++;
                cnt++;
            }
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                if (curOut->GetGateConst()->GetOpCode().IsState() && curOut->IsStateEdge()) {
                    ASSERT(!curOut->GetGateConst()->GetOpCode().IsFixed());
                    setOfOps[{curOut->GetGateConst()->GetOpCode(), curOut->GetGateConst()->GetBitField()}]++;
                    cnt++;
                }
            }
        }
        if (setOfOps.size() != cnt) {
            return std::make_pair("Duplicate state out branches", -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckNOP() const
{
    if (GetOpCode() == OpCode::NOP) {
        if (!IsFirstOutNull()) {
            return std::make_pair("NOP gate used by other gates", -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckSelector() const
{
    if (GetOpCode() == OpCode::VALUE_SELECTOR || GetOpCode() == OpCode::DEPEND_SELECTOR) {
        auto stateOp = GetInGateConst(0)->GetOpCode();
        if (stateOp == OpCode::MERGE || stateOp == OpCode::LOOP_BEGIN) {
            if (GetInGateConst(0)->GetNumIns() != GetNumIns() - 1) {
                if (GetOpCode() == OpCode::DEPEND_SELECTOR) {
                    return std::make_pair("Number of depend flows does not match control flows (expected:" +
                            std::to_string(GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(GetNumIns() - 1) + ")",
                        -1);
                } else {
                    return std::make_pair("Number of data flows does not match control flows (expected:" +
                            std::to_string(GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(GetNumIns() - 1) + ")",
                        -1);
                }
            }
        } else {
            return std::make_pair(
                "State input does not match (expected:[MERGE|LOOP_BEGIN] actual:" + stateOp.Str() + ")", 0);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckRelay() const
{
    if (GetOpCode() == OpCode::DEPEND_RELAY) {
        auto stateOp = GetInGateConst(0)->GetOpCode();
        if (!(stateOp == OpCode::IF_TRUE || stateOp == OpCode::IF_FALSE || stateOp == OpCode::SWITCH_CASE ||
            stateOp == OpCode::DEFAULT_CASE || stateOp == OpCode::IF_SUCCESS || stateOp == OpCode::IF_EXCEPTION ||
            stateOp == OpCode::ORDINARY_BLOCK)) {
            return std::make_pair("State input does not match ("
                "expected:[IF_TRUE|IF_FALSE|SWITCH_CASE|DEFAULT_CASE|IF_SUCCESS|IF_EXCEPTION|ORDINARY_BLOCK] actual:" +
                stateOp.Str() + ")", 0);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::SpecialCheck() const
{
    {
        auto ret = CheckNOP();
        if (ret.has_value()) {
            return ret;
        }
    }
    {
        auto ret = CheckSelector();
        if (ret.has_value()) {
            return ret;
        }
    }
    {
        auto ret = CheckRelay();
        if (ret.has_value()) {
            return ret;
        }
    }
    return std::nullopt;
}

bool Gate::Verify() const
{
    std::string errorString;
    size_t highlightIdx = -1;
    bool failed = false;
    {
        auto ret = CheckNullInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckStateInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckValueInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckDependInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckStateOutput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckBranchOutput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = SpecialCheck();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (failed) {
        COMPILER_LOG(ERROR) << "[Verifier][Error] Gate level input list schema verify failed";
        Print("", true, highlightIdx);
        COMPILER_LOG(ERROR) << "Note: " << errorString;
    }
    return !failed;
}

MachineType JSMachineType()
{
    return MachineType::I64;
}

size_t GetOpCodeNumIns(OpCode opcode, BitField bitfield)
{
    return opcode.GetOpCodeNumIns(bitfield);
}

void Out::SetNextOut(const Out *ptr)
{
    nextOut_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Out *Out::GetNextOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + nextOut_);
}

const Out *Out::GetNextOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + nextOut_);
}

void Out::SetPrevOut(const Out *ptr)
{
    prevOut_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Out *Out::GetPrevOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + prevOut_);
}

const Out *Out::GetPrevOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + prevOut_);
}

void Out::SetIndex(OutIdx idx)
{
    idx_ = idx;
}

OutIdx Out::GetIndex() const
{
    return idx_;
}

Gate *Out::GetGate()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>(&this[idx_ + 1]);
}

const Gate *Out::GetGateConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>(&this[idx_ + 1]);
}

void Out::SetPrevOutNull()
{
    prevOut_ = 0;
}

bool Out::IsPrevOutNull() const
{
    return prevOut_ == 0;
}

void Out::SetNextOutNull()
{
    nextOut_ = 0;
}

bool Out::IsNextOutNull() const
{
    return nextOut_ == 0;
}

bool Out::IsStateEdge() const
{
    return idx_ < GetGateConst()->GetStateCount();
}

void In::SetGate(const Gate *ptr)
{
    gatePtr_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Gate *In::GetGate()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>((reinterpret_cast<uint8_t *>(this)) + gatePtr_);
}

const Gate *In::GetGateConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>((reinterpret_cast<const uint8_t *>(this)) + gatePtr_);
}

void In::SetGateNull()
{
    gatePtr_ = 0;
}

bool In::IsGateNull() const
{
    return gatePtr_ == 0;
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
Gate::Gate(GateId id, OpCode opcode, MachineType bitValue, BitField bitfield, Gate *inList[], GateType type,
           MarkCode mark)
    : id_(id), opcode_(opcode), bitValue_(bitValue), type_(type), stamp_(1), mark_(mark), bitfield_(bitfield),
    firstOut_(0)
{
    auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto in = inList[idx];
        if (in == nullptr) {
            GetIn(idx)->SetGateNull();
        } else {
            NewIn(idx, in);
        }
        auto curOut = GetOut(idx);
        curOut->SetIndex(idx);
    }
}

Gate::Gate(GateId id, OpCode opcode, BitField bitfield, Gate *inList[], GateType type, MarkCode mark)
    : id_(id), opcode_(opcode), type_(type), stamp_(1), mark_(mark), bitfield_(bitfield), firstOut_(0)
{
    auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto in = inList[idx];
        if (in == nullptr) {
            GetIn(idx)->SetGateNull();
        } else {
            NewIn(idx, in);
        }
        auto curOut = GetOut(idx);
        curOut->SetIndex(idx);
    }
}

size_t Gate::GetOutListSize(size_t numIns)
{
    return numIns * sizeof(Out);
}

size_t Gate::GetOutListSize() const
{
    return Gate::GetOutListSize(GetNumIns());
}

size_t Gate::GetInListSize(size_t numIns)
{
    return numIns * sizeof(In);
}

size_t Gate::GetInListSize() const
{
    return Gate::GetInListSize(GetNumIns());
}

size_t Gate::GetGateSize(size_t numIns)
{
    return Gate::GetOutListSize(numIns) + Gate::GetInListSize(numIns) + sizeof(Gate);
}

size_t Gate::GetGateSize() const
{
    return Gate::GetGateSize(GetNumIns());
}

void Gate::NewIn(size_t idx, Gate *in)
{
    GetIn(idx)->SetGate(in);
    auto curOut = GetOut(idx);
    if (in->IsFirstOutNull()) {
        curOut->SetNextOutNull();
    } else {
        curOut->SetNextOut(in->GetFirstOut());
        in->GetFirstOut()->SetPrevOut(curOut);
    }
    curOut->SetPrevOutNull();
    in->SetFirstOut(curOut);
}

void Gate::ModifyIn(size_t idx, Gate *in)
{
    DeleteIn(idx);
    NewIn(idx, in);
}

void Gate::DeleteIn(size_t idx)
{
    if (!GetOut(idx)->IsNextOutNull() && !GetOut(idx)->IsPrevOutNull()) {
        GetOut(idx)->GetPrevOut()->SetNextOut(GetOut(idx)->GetNextOut());
        GetOut(idx)->GetNextOut()->SetPrevOut(GetOut(idx)->GetPrevOut());
    } else if (GetOut(idx)->IsNextOutNull() && !GetOut(idx)->IsPrevOutNull()) {
        GetOut(idx)->GetPrevOut()->SetNextOutNull();
    } else if (!GetOut(idx)->IsNextOutNull()) {  // then GetOut(idx)->IsPrevOutNull() is true
        GetIn(idx)->GetGate()->SetFirstOut(GetOut(idx)->GetNextOut());
        GetOut(idx)->GetNextOut()->SetPrevOutNull();
    } else {  // only this out now
        GetIn(idx)->GetGate()->SetFirstOutNull();
    }
    GetIn(idx)->SetGateNull();
}

void Gate::DeleteGate()
{
    auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        DeleteIn(idx);
    }
    SetOpCode(OpCode(OpCode::NOP));
}

Out *Gate::GetOut(size_t idx)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<Out *>(this)[-1 - idx];
}

Out *Gate::GetFirstOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + firstOut_);
}

const Out *Gate::GetFirstOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + firstOut_);
}

void Gate::SetFirstOutNull()
{
    firstOut_ = 0;
}

bool Gate::IsFirstOutNull() const
{
    return firstOut_ == 0;
}

void Gate::SetFirstOut(const Out *firstOut)
{
    firstOut_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>(reinterpret_cast<const uint8_t *>(firstOut) - reinterpret_cast<const uint8_t *>(this));
}

In *Gate::GetIn(size_t idx)
{
#ifndef NDEBUG
    if (idx >= GetNumIns()) {
        COMPILER_LOG(INFO) << std::dec << "Gate In access out-of-bound! (idx=" << idx << ")";
        Print();
        ASSERT(false);
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<In *>(this + 1)[idx];
}

const In *Gate::GetInConst(size_t idx) const
{
#ifndef NDEBUG
    if (idx >= GetNumIns()) {
        COMPILER_LOG(INFO) << std::dec << "Gate In access out-of-bound! (idx=" << idx << ")";
        Print();
        ASSERT(false);
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<const In *>(this + 1)[idx];
}

Gate *Gate::GetInGate(size_t idx)
{
    return GetIn(idx)->GetGate();
}

const Gate *Gate::GetInGateConst(size_t idx) const
{
    return GetInConst(idx)->GetGateConst();
}

bool Gate::IsInGateNull(size_t idx) const
{
    return GetInConst(idx)->IsGateNull();
}

GateId Gate::GetId() const
{
    return id_;
}

OpCode Gate::GetOpCode() const
{
    return opcode_;
}

MachineType Gate::GetMachineType() const
{
    return bitValue_;
}

void Gate::SetMachineType(MachineType MachineType)
{
    bitValue_ = MachineType;
}

void Gate::SetOpCode(OpCode opcode)
{
    opcode_ = opcode;
}

GateType Gate::GetGateType() const
{
    return type_;
}

size_t Gate::GetNumIns() const
{
    return GetOpCodeNumIns(GetOpCode(), GetBitField());
}

size_t Gate::GetStateCount() const
{
    return GetOpCode().GetStateCount(GetBitField());
}

size_t Gate::GetDependCount() const
{
    return GetOpCode().GetDependCount(GetBitField());
}

size_t Gate::GetInValueCount() const
{
    return GetOpCode().GetInValueCount(GetBitField());
}

size_t Gate::GetRootCount() const
{
    return GetOpCode().GetRootCount(GetBitField());
}

BitField Gate::GetBitField() const
{
    return bitfield_;
}

void Gate::SetBitField(BitField bitfield)
{
    bitfield_ = bitfield;
}

std::string Gate::MachineTypeStr(MachineType machineType) const
{
    const std::map<MachineType, const char *> strMap = {
            {NOVALUE, "NOVALUE"},
            {ANYVALUE, "ANYVALUE"},
            {ARCH, "ARCH"},
            {FLEX, "FLEX"},
            {I1, "I1"},
            {I8, "I8"},
            {I16, "I16"},
            {I32, "I32"},
            {I64, "I64"},
            {F32, "F32"},
            {F64, "F64"},
    };
    if (strMap.count(machineType) > 0) {
        return strMap.at(machineType);
    }
    return "MachineType-" + std::to_string(machineType);
}

std::string Gate::GateTypeStr(GateType gateType) const
{
    const std::map<GateType, const char *> strMap = {
        {GateType::NJSValue(), "NJS_VALUE"},
        {GateType::TaggedValue(), "TAGGED_VALUE"},
        {GateType::TaggedPointer(), "TAGGED_POINTER"},
        {GateType::TaggedNPointer(), "TAGGED_NPOINTER"},
        {GateType::Empty(), "EMPTY"},
        {GateType::AnyType(), "ANY_TYPE"},
    };

    if (strMap.count(gateType) > 0) {
        return strMap.at(gateType);
    }
    return "GateType-" + std::to_string(gateType.GetType());
}

void Gate::Print(std::string bytecode, bool inListPreview, size_t highlightIdx) const
{
    if (GetOpCode() != OpCode::NOP) {
        std::string log("(id=" + std::to_string(id_) + ", op=" + GetOpCode().Str() + ", ");
        log += ((bytecode.compare("") == 0) ? "" : "bytecode=") + bytecode;
        log += ((bytecode.compare("") == 0) ? "" : ", ");
        log += "machineType=" + MachineTypeStr(GetMachineType()) + ", ";
        log += "bitfield=" + std::to_string(bitfield_) + ", ";
        log += "type=" + GateTypeStr(type_) + ", ";
        log += "stamp=" + std::to_string(static_cast<uint32_t>(stamp_)) + ", ";
        log += "mark=" + std::to_string(static_cast<uint32_t>(mark_)) + ", ";
        log += "in=[";

        size_t idx = 0;
        auto stateSize = GetStateCount();
        auto dependSize = GetDependCount();
        auto valueSize = GetInValueCount();
        auto rootSize = GetRootCount();
        idx = PrintInGate(stateSize, idx, 0, inListPreview, highlightIdx, log);
        idx = PrintInGate(stateSize + dependSize, idx, stateSize, inListPreview, highlightIdx, log);
        idx = PrintInGate(stateSize + dependSize + valueSize, idx, stateSize + dependSize,
                          inListPreview, highlightIdx, log);
        PrintInGate(stateSize + dependSize + valueSize + rootSize, idx, stateSize + dependSize + valueSize,
                    inListPreview, highlightIdx, log, true);

        log += "], out=[";

        if (!IsFirstOutNull()) {
            const Out *curOut = GetFirstOutConst();
            log += std::to_string(curOut->GetGateConst()->GetId()) +
                    (inListPreview ? std::string(":" + curOut->GetGateConst()->GetOpCode().Str()) : std::string(""));

            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                log += " " +  std::to_string(curOut->GetGateConst()->GetId()) +
                       (inListPreview ? std::string(":" + curOut->GetGateConst()->GetOpCode().Str())
                                       : std::string(""));
            }
        }
        log += "])";
        log += "\n";
        COMPILER_LOG(INFO) << std::dec << log;
    }
}

size_t Gate::PrintInGate(size_t numIns, size_t idx, size_t size, bool inListPreview, size_t highlightIdx,
                         std::string &log, bool isEnd) const
{
    log += "[";
    for (; idx < numIns; idx++) {
        log += ((idx == size) ? "" : " ");
        log += ((idx == highlightIdx) ? "\033[4;31m" : "");
        log += ((IsInGateNull(idx)
                       ? "N"
                       : (std::to_string(GetInGateConst(idx)->GetId()) +
                          (inListPreview ? std::string(":" + GetInGateConst(idx)->GetOpCode().Str())
                                         : std::string("")))));
        log += ((idx == highlightIdx) ? "\033[0m" : "");
    }
    log += "]";
    log += ((isEnd) ? "" : ", ");
    return idx;
}

void Gate::PrintByteCode(std::string bytecode) const
{
    Print(bytecode);
}

MarkCode Gate::GetMark(TimeStamp stamp) const
{
    return (stamp_ == stamp) ? mark_ : MarkCode::NO_MARK;
}

void Gate::SetMark(MarkCode mark, TimeStamp stamp)
{
    stamp_ = stamp;
    mark_ = mark;
}

bool OpCode::IsRoot() const
{
    return (GetProperties().root == OpCode::CIRCUIT_ROOT) || (op_ == OpCode::CIRCUIT_ROOT);
}

bool OpCode::IsProlog() const
{
    return (GetProperties().root == OpCode::ARG_LIST);
}

bool OpCode::IsFixed() const
{
    return (op_ == OpCode::VALUE_SELECTOR) || (op_ == OpCode::DEPEND_SELECTOR) || (op_ == OpCode::DEPEND_RELAY);
}

bool OpCode::IsSchedulable() const
{
    return (op_ != OpCode::NOP) && (!IsProlog()) && (!IsRoot()) && (!IsFixed()) && (GetStateCount(1) == 0);
}

bool OpCode::IsState() const
{
    return (op_ != OpCode::NOP) && (!IsProlog()) && (!IsRoot()) && (!IsFixed()) && (GetStateCount(1) > 0);
}

bool OpCode::IsGeneralState() const
{
    return ((op_ == OpCode::IF_TRUE) || (op_ == OpCode::IF_FALSE) || (op_ == OpCode::JS_BYTECODE) ||
            (op_ == OpCode::IF_SUCCESS) || (op_ == OpCode::IF_EXCEPTION) || (op_ == OpCode::SWITCH_CASE) ||
            (op_ == OpCode::DEFAULT_CASE) || (op_ == OpCode::MERGE) || (op_ == OpCode::LOOP_BEGIN) ||
            (op_ == OpCode::ORDINARY_BLOCK) || (op_ == OpCode::STATE_ENTRY));
}

bool OpCode::IsTerminalState() const
{
    return ((op_ == OpCode::RETURN) || (op_ == OpCode::THROW) || (op_ == OpCode::RETURN_VOID));
}

bool OpCode::IsCFGMerge() const
{
    return (op_ == OpCode::MERGE) || (op_ == OpCode::LOOP_BEGIN);
}

bool OpCode::IsControlCase() const
{
    return (op_ == OpCode::IF_BRANCH) || (op_ == OpCode::SWITCH_BRANCH) || (op_ == OpCode::IF_TRUE) ||
           (op_ == OpCode::IF_FALSE) || (op_ == OpCode::IF_SUCCESS) || (op_ == OpCode::IF_EXCEPTION) ||
           (op_ == OpCode::SWITCH_CASE) || (op_ == OpCode::DEFAULT_CASE);
}

bool OpCode::IsLoopHead() const
{
    return (op_ == OpCode::LOOP_BEGIN);
}

bool OpCode::IsNop() const
{
    return (op_ == OpCode::NOP);
}
}  // namespace panda::ecmascript::kungfu
