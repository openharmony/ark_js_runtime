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

namespace kungfu {
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
#define VALUE(...) (std::make_pair(std::vector<ValueCode>{__VA_ARGS__}, false))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MANY_STATE(...) (std::make_pair(std::vector<OpCode>{__VA_ARGS__}, true))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MANY_VALUE(...) (std::make_pair(std::vector<ValueCode>{__VA_ARGS__}, true))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_STATE (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_VALUE (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NO_ROOT (std::nullopt)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GENERAL_STATE (NOP)
    switch (this->op) {
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
        case THROW:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, VALUE(JSValueCode()), OpCode(THROW_LIST)};
        case ORDINARY_BLOCK:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, NO_VALUE, NO_ROOT};
        case IF_BRANCH:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, VALUE(INT1), NO_ROOT};
        case SWITCH_BRANCH:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, VALUE(INT64), NO_ROOT};
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
        case VALUE_SELECTOR_JS:
            return {JSValueCode(), STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(JSValueCode()), NO_ROOT};
        case VALUE_SELECTOR_INT1:
            return {INT1, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(INT1), NO_ROOT};
        case VALUE_SELECTOR_INT8:
            return {INT8, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(INT8), NO_ROOT};
        case VALUE_SELECTOR_INT16:
            return {INT16, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(INT16), NO_ROOT};
        case VALUE_SELECTOR_INT32:
            return {INT32, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(INT32), NO_ROOT};
        case VALUE_SELECTOR_INT64:
            return {INT64, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(INT64), NO_ROOT};
        case VALUE_SELECTOR_FLOAT32:
            return {FLOAT32, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(FLOAT32), NO_ROOT};
        case VALUE_SELECTOR_FLOAT64:
            return {FLOAT64, STATE(OpCode(GENERAL_STATE)), NO_DEPEND, MANY_VALUE(FLOAT64), NO_ROOT};
        case DEPEND_SELECTOR:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), MANY_DEPEND, NO_VALUE, NO_ROOT};
        case DEPEND_RELAY:
            return {NOVALUE, STATE(OpCode(GENERAL_STATE)), ONE_DEPEND, NO_VALUE, NO_ROOT};
        case DEPEND_AND:
            return {NOVALUE, NO_STATE, MANY_DEPEND, NO_VALUE, NO_ROOT};
        // High Level IR
        case JS_CALL:
            return {JSValueCode(), NO_STATE, ONE_DEPEND, MANY_VALUE(ANYVALUE), NO_ROOT};
        case JS_CONSTANT:
            return {JSValueCode(), NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case JS_ARG:
            return {JSValueCode(), NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case JS_ADD:
        case JS_SUB:
        case JS_MUL:
        case JS_EXP:
        case JS_DIV:
        case JS_MOD:
        case JS_AND:
        case JS_XOR:
        case JS_OR:
        case JS_LSL:
        case JS_LSR:
        case JS_ASR:
        case JS_LOGIC_AND:
        case JS_LOGIC_OR:
        case JS_LT:
        case JS_LE:
        case JS_GT:
        case JS_GE:
        case JS_EQ:
        case JS_NE:
        case JS_STRICT_EQ:
        case JS_STRICT_NE:
            return {JSValueCode(), NO_STATE, NO_DEPEND, VALUE(JSValueCode(), JSValueCode()), NO_ROOT};
        case JS_LOGIC_NOT:
            return {JSValueCode(), NO_STATE, NO_DEPEND, VALUE(JSValueCode()), NO_ROOT};
        // Middle Level IR
        case CALL:
            return {NOVALUE, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case INT1_CALL:
            return {INT1, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case INT8_CALL:
            return {INT8, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case INT16_CALL:
            return {INT16, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case INT32_CALL:
            return {INT32, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case INT64_CALL:
            return {INT64, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case FLOAT32_CALL:
            return {FLOAT32, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case FLOAT64_CALL:
            return {FLOAT64, NO_STATE, ONE_DEPEND, MANY_VALUE(PtrValueCode(), ANYVALUE), NO_ROOT};
        case ALLOCA:
            return {PtrValueCode(), NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ALLOCA_LIST)};
        case INT1_ARG:
            return {INT1, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case INT8_ARG:
            return {INT8, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case INT16_ARG:
            return {INT16, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case INT32_ARG:
            return {INT32, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case INT64_ARG:
            return {INT64, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case FLOAT32_ARG:
            return {FLOAT32, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case FLOAT64_ARG:
            return {FLOAT64, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(ARG_LIST)};
        case MUTABLE_DATA:
        case CONST_DATA:
            return {PtrValueCode(), NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case INT1_CONSTANT:
            return {INT1, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case INT8_CONSTANT:
            return {INT8, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case INT16_CONSTANT:
            return {INT16, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case INT32_CONSTANT:
            return {INT32, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case INT64_CONSTANT:
            return {INT64, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case FLOAT32_CONSTANT:
            return {FLOAT32, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case FLOAT64_CONSTANT:
            return {FLOAT64, NO_STATE, NO_DEPEND, NO_VALUE, OpCode(CONSTANT_LIST)};
        case ZEXT_INT32_TO_INT64:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(INT32), NO_ROOT};
        case ZEXT_INT1_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT1), NO_ROOT};
        case ZEXT_INT8_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT8), NO_ROOT};
        case ZEXT_INT16_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT16), NO_ROOT};
        case ZEXT_INT1_TO_INT64:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(INT1), NO_ROOT};
        case SEXT_INT32_TO_INT64:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(INT32), NO_ROOT};
        case SEXT_INT1_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT1), NO_ROOT};
        case SEXT_INT8_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT8), NO_ROOT};
        case SEXT_INT16_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT8), NO_ROOT};
        case SEXT_INT1_TO_INT64:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(INT1), NO_ROOT};
        case TRUNC_INT64_TO_INT32:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT64), NO_ROOT};
        case TRUNC_INT64_TO_INT1:
            return {INT1, NO_STATE, NO_DEPEND, VALUE(INT64), NO_ROOT};
        case TRUNC_INT32_TO_INT1:
            return {INT1, NO_STATE, NO_DEPEND, VALUE(INT32), NO_ROOT};
        case INT32_REV:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT32), NO_ROOT};
        case INT32_ADD:
        case INT32_SUB:
        case INT32_MUL:
        case INT32_EXP:
        case INT32_SDIV:
        case INT32_SMOD:
        case INT32_UDIV:
        case INT32_UMOD:
        case INT32_AND:
        case INT32_XOR:
        case INT32_OR:
        case INT32_LSL:
        case INT32_LSR:
        case INT32_ASR:
            return {INT32, NO_STATE, NO_DEPEND, VALUE(INT32, INT32), NO_ROOT};
        case INT32_SLT:
        case INT32_SLE:
        case INT32_SGT:
        case INT32_SGE:
        case INT32_ULT:
        case INT32_ULE:
        case INT32_UGT:
        case INT32_UGE:
        case INT32_EQ:
        case INT32_NE:
            return {INT1, NO_STATE, NO_DEPEND, VALUE(INT32, INT32), NO_ROOT};
        case INT64_REV:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(INT64), NO_ROOT};
        case INT64_ADD:
        case INT64_SUB:
        case INT64_MUL:
        case INT64_EXP:
        case INT64_SDIV:
        case INT64_SMOD:
        case INT64_UDIV:
        case INT64_UMOD:
        case INT64_AND:
        case INT64_XOR:
        case INT64_OR:
        case INT64_LSL:
        case INT64_LSR:
        case INT64_ASR:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(INT64, INT64), NO_ROOT};
        case INT64_SLT:
        case INT64_SLE:
        case INT64_SGT:
        case INT64_SGE:
        case INT64_ULT:
        case INT64_ULE:
        case INT64_UGT:
        case INT64_UGE:
        case INT64_EQ:
        case INT64_NE:
            return {INT1, NO_STATE, NO_DEPEND, VALUE(INT64, INT64), NO_ROOT};
        case FLOAT64_ADD:
        case FLOAT64_SUB:
        case FLOAT64_MUL:
        case FLOAT64_DIV:
        case FLOAT64_EXP:
        case FLOAT64_SMOD:
            return {FLOAT64, NO_STATE, NO_DEPEND, VALUE(FLOAT64, FLOAT64), NO_ROOT};
        case FLOAT64_EQ:
            return {INT1, NO_STATE, NO_DEPEND, VALUE(FLOAT64, FLOAT64), NO_ROOT};
        case INT8_LOAD:
            return {INT8, NO_STATE, ONE_DEPEND, VALUE(PtrValueCode()), NO_ROOT};
        case INT16_LOAD:
            return {INT16, NO_STATE, ONE_DEPEND, VALUE(PtrValueCode()), NO_ROOT};
        case INT32_LOAD:
            return {INT32, NO_STATE, ONE_DEPEND, VALUE(PtrValueCode()), NO_ROOT};
        case INT64_LOAD:
            return {INT64, NO_STATE, ONE_DEPEND, VALUE(PtrValueCode()), NO_ROOT};
        case FLOAT32_LOAD:
            return {FLOAT32, NO_STATE, ONE_DEPEND, VALUE(PtrValueCode()), NO_ROOT};
        case FLOAT64_LOAD:
            return {FLOAT64, NO_STATE, ONE_DEPEND, VALUE(PtrValueCode()), NO_ROOT};
        case INT8_STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(INT8, PtrValueCode()), NO_ROOT};
        case INT16_STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(INT16, PtrValueCode()), NO_ROOT};
        case INT32_STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(INT32, PtrValueCode()), NO_ROOT};
        case INT64_STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(INT64, PtrValueCode()), NO_ROOT};
        case FLOAT32_STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(FLOAT32, PtrValueCode()), NO_ROOT};
        case FLOAT64_STORE:
            return {NOVALUE, NO_STATE, ONE_DEPEND, VALUE(FLOAT64, PtrValueCode()), NO_ROOT};
        case INT32_TO_FLOAT64:
            return {FLOAT64, NO_STATE, NO_DEPEND, VALUE(INT32), NO_ROOT};
        case INT64_TO_FLOAT64:
            return {FLOAT64, NO_STATE, NO_DEPEND, VALUE(INT64), NO_ROOT};
        case FLOAT64_TO_INT64:
            return {INT64, NO_STATE, NO_DEPEND, VALUE(FLOAT64), NO_ROOT};
        default:
            std::cerr << "Please complete OpCode properties (OpCode=" << this->op << ")" << std::endl;
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
        // SHARED
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
        {VALUE_SELECTOR_JS, "VALUE_SELECTOR_JS"},
        {VALUE_SELECTOR_INT1, "VALUE_SELECTOR_INT1"},
        {VALUE_SELECTOR_INT8, "VALUE_SELECTOR_INT8"},
        {VALUE_SELECTOR_INT16, "VALUE_SELECTOR_INT16"},
        {VALUE_SELECTOR_INT32, "VALUE_SELECTOR_INT32"},
        {VALUE_SELECTOR_INT64, "VALUE_SELECTOR_INT64"},
        {VALUE_SELECTOR_FLOAT32, "VALUE_SELECTOR_FLOAT32"},
        {VALUE_SELECTOR_FLOAT64, "VALUE_SELECTOR_FLOAT64"},
        {DEPEND_SELECTOR, "DEPEND_SELECTOR"},
        {DEPEND_RELAY, "DEPEND_RELAY"},
        {DEPEND_AND, "DEPEND_AND"},
        // High Level IR
        {JS_CALL, "JS_CALL"},
        {JS_CONSTANT, "JS_CONSTANT"},
        {JS_ARG, "JS_ARG"},
        {JS_ADD, "JS_ADD"},
        {JS_SUB, "JS_SUB"},
        {JS_MUL, "JS_MUL"},
        {JS_EXP, "JS_EXP"},
        {JS_DIV, "JS_DIV"},
        {JS_MOD, "JS_MOD"},
        {JS_AND, "JS_AND"},
        {JS_XOR, "JS_XOR"},
        {JS_OR, "JS_OR"},
        {JS_LSL, "JS_LSL"},
        {JS_LSR, "JS_LSR"},
        {JS_ASR, "JS_ASR"},
        {JS_LOGIC_AND, "JS_LOGIC_AND"},
        {JS_LOGIC_OR, "JS_LOGIC_OR"},
        {JS_LT, "JS_LT"},
        {JS_LE, "JS_LE"},
        {JS_GT, "JS_GT"},
        {JS_GE, "JS_GE"},
        {JS_EQ, "JS_EQ"},
        {JS_NE, "JS_NE"},
        {JS_STRICT_EQ, "JS_STRICT_EQ"},
        {JS_STRICT_NE, "JS_STRICT_NE"},
        {JS_LOGIC_NOT, "JS_LOGIC_NOT"},
        // Middle Level IR
        {CALL, "CALL"},
        {INT1_CALL, "INT1_CALL"},
        {INT8_CALL, "INT8_CALL"},
        {INT16_CALL, "INT16_CALL"},
        {INT32_CALL, "INT32_CALL"},
        {INT64_CALL, "INT64_CALL"},
        {FLOAT32_CALL, "FLOAT32_CALL"},
        {FLOAT64_CALL, "FLOAT64_CALL"},
        {ALLOCA, "ALLOCA"},
        {INT1_ARG, "INT1_ARG"},
        {INT8_ARG, "INT8_ARG"},
        {INT16_ARG, "INT16_ARG"},
        {INT32_ARG, "INT32_ARG"},
        {INT64_ARG, "INT64_ARG"},
        {FLOAT32_ARG, "FLOAT32_ARG"},
        {FLOAT64_ARG, "FLOAT64_ARG"},
        {MUTABLE_DATA, "MUTABLE_DATA"},
        {CONST_DATA, "CONST_DATA"},
        {INT1_CONSTANT, "INT1_CONSTANT"},
        {INT8_CONSTANT, "INT8_CONSTANT"},
        {INT16_CONSTANT, "INT16_CONSTANT"},
        {INT32_CONSTANT, "INT32_CONSTANT"},
        {INT64_CONSTANT, "INT64_CONSTANT"},
        {FLOAT32_CONSTANT, "FLOAT32_CONSTANT"},
        {FLOAT64_CONSTANT, "FLOAT64_CONSTANT"},
        {ZEXT_INT32_TO_INT64, "ZEXT_INT32_TO_INT64"},
        {ZEXT_INT1_TO_INT32, "ZEXT_INT1_TO_INT32"},
        {ZEXT_INT8_TO_INT32, "ZEXT_INT8_TO_INT32"},
        {ZEXT_INT16_TO_INT32, "ZEXT_INT16_TO_INT32"},
        {ZEXT_INT1_TO_INT64, "ZEXT_INT1_TO_INT64"},
        {SEXT_INT32_TO_INT64, "SEXT_INT32_TO_INT64"},
        {SEXT_INT1_TO_INT32, "SEXT_INT1_TO_INT32"},
        {SEXT_INT8_TO_INT32, "SEXT_INT8_TO_INT32"},
        {SEXT_INT16_TO_INT32, "SEXT_INT16_TO_INT32"},
        {SEXT_INT1_TO_INT64, "SEXT_INT1_TO_INT64"},
        {TRUNC_INT64_TO_INT32, "TRUNC_INT64_TO_INT32"},
        {TRUNC_INT64_TO_INT1, "TRUNC_INT64_TO_INT1"},
        {TRUNC_INT32_TO_INT1, "TRUNC_INT32_TO_INT1"},
        {INT32_REV, "INT32_REV"},
        {INT32_ADD, "INT32_ADD"},
        {INT32_SUB, "INT32_SUB"},
        {INT32_MUL, "INT32_MUL"},
        {INT32_EXP, "INT32_EXP"},
        {INT32_SDIV, "INT32_SDIV"},
        {INT32_SMOD, "INT32_SMOD"},
        {INT32_UDIV, "INT32_UDIV"},
        {INT32_UMOD, "INT32_UMOD"},
        {INT32_AND, "INT32_AND"},
        {INT32_XOR, "INT32_XOR"},
        {INT32_OR, "INT32_OR"},
        {INT32_LSL, "INT32_LSL"},
        {INT32_LSR, "INT32_LSR"},
        {INT32_ASR, "INT32_ASR"},
        {INT32_SLT, "INT32_SLT"},
        {INT32_SLE, "INT32_SLE"},
        {INT32_SGT, "INT32_SGT"},
        {INT32_SGE, "INT32_SGE"},
        {INT32_ULT, "INT32_ULT"},
        {INT32_ULE, "INT32_ULE"},
        {INT32_UGT, "INT32_UGT"},
        {INT32_UGE, "INT32_UGE"},
        {INT32_EQ, "INT32_EQ"},
        {INT32_NE, "INT32_NE"},
        {INT64_ADD, "INT64_ADD"},
        {INT64_SUB, "INT64_SUB"},
        {INT64_MUL, "INT64_MUL"},
        {INT64_EXP, "INT64_EXP"},
        {INT64_SDIV, "INT64_SDIV"},
        {INT64_SMOD, "INT64_SMOD"},
        {INT64_UDIV, "INT64_UDIV"},
        {INT64_UMOD, "INT64_UMOD"},
        {INT64_AND, "INT64_AND"},
        {INT64_XOR, "INT64_XOR"},
        {INT64_OR, "INT64_OR"},
        {INT64_LSL, "INT64_LSL"},
        {INT64_LSR, "INT64_LSR"},
        {INT64_ASR, "INT64_ASR"},
        {INT64_SLT, "INT64_SLT"},
        {INT64_SLE, "INT64_SLE"},
        {INT64_SGT, "INT64_SGT"},
        {INT64_SGE, "INT64_SGE"},
        {INT64_ULT, "INT64_ULT"},
        {INT64_ULE, "INT64_ULE"},
        {INT64_UGT, "INT64_UGT"},
        {INT64_UGE, "INT64_UGE"},
        {INT64_EQ, "INT64_EQ"},
        {INT64_NE, "INT64_NE"},
        {INT64_REV, "INT64_REV"},
        {FLOAT64_ADD, "FLOAT64_ADD"},
        {FLOAT64_SUB, "FLOAT64_SUB"},
        {FLOAT64_MUL, "FLOAT64_MUL"},
        {FLOAT64_DIV, "FLOAT64_DIV"},
        {FLOAT64_EXP, "FLOAT64_EXP"},
        {FLOAT64_EQ, "FLOAT64_EQ"},
        {INT8_LOAD, "INT8_LOAD"},
        {INT16_LOAD, "INT16_LOAD"},
        {INT32_LOAD, "INT32_LOAD"},
        {INT64_LOAD, "INT64_LOAD"},
        {FLOAT32_LOAD, "FLOAT32_LOAD"},
        {FLOAT64_LOAD, "FLOAT64_LOAD"},
        {INT8_STORE, "INT8_STORE"},
        {INT16_STORE, "INT16_STORE"},
        {INT32_STORE, "INT32_STORE"},
        {INT64_STORE, "INT64_STORE"},
        {FLOAT32_STORE, "FLOAT32_STORE"},
        {FLOAT64_STORE, "FLOAT64_STORE"},
        {INT32_TO_FLOAT64, "INT32_TO_FLOAT64"},
        {INT64_TO_FLOAT64, "INT64_TO_FLOAT64"},
        {FLOAT64_TO_INT64, "FLOAT64_TO_INT64"},
    };
    if (strMap.count(this->op) > 0) {
        return strMap.at(this->op);
    }
    return "OP-" + std::to_string(this->op);
}
// 4 : 4 means that there are 4 args in total
std::array<size_t, 4> OpCode::GetOpCodeNumInsArray(BitField bitfield) const
{
    const size_t manyDepend = 2;
    auto properties = this->GetProperties();
    auto stateProp = properties.statesIn;
    auto dependProp = properties.dependsIn;
    auto valueProp = properties.valuesIn;
    auto rootProp = properties.states;
    size_t stateSize = stateProp.has_value() ? (stateProp->second ? bitfield : stateProp->first.size()) : 0;
    size_t dependSize = (dependProp == manyDepend) ? bitfield : dependProp;
    size_t valueSize = valueProp.has_value() ? (valueProp->second ? bitfield : valueProp->first.size()) : 0;
    size_t rootSize = rootProp.has_value() ? 1 : 0;
    return {stateSize, dependSize, valueSize, rootSize};
}

size_t OpCode::GetOpCodeNumIns(BitField bitfield) const
{
    auto numInsArray = this->GetOpCodeNumInsArray(bitfield);
    // 2 : 2 means the third element.
    // 3 : 3 means the fourth element.
    return numInsArray[0] + numInsArray[1] + numInsArray[2] + numInsArray[3];
}

ValueCode OpCode::GetValueCode() const
{
    return this->GetProperties().returnValue;
}

ValueCode OpCode::GetInValueCode(BitField bitfield, size_t idx) const
{
    auto numInsArray = this->GetOpCodeNumInsArray(bitfield);
    auto valueProp = this->GetProperties().valuesIn;
    idx -= numInsArray[0];
    idx -= numInsArray[1];
    ASSERT(valueProp.has_value());
    if (valueProp->second) {
        return valueProp->first.at(std::min(idx, valueProp->first.size() - 1));
    }
    return valueProp->first.at(idx);
}

OpCode OpCode::GetInStateCode(size_t idx) const
{
    auto stateProp = this->GetProperties().statesIn;
    ASSERT(stateProp.has_value());
    if (stateProp->second) {
        return stateProp->first.at(std::min(idx, stateProp->first.size() - 1));
    }
    return stateProp->first.at(idx);
}

std::string ValueCodeToStr(ValueCode valueCode)
{
    switch (valueCode) {
        case NOVALUE:
            return "NOVALUE";
        case ANYVALUE:
            return "ANYVALUE";
        case INT1:
            return "INT1";
        case INT8:
            return "INT8";
        case INT16:
            return "INT16";
        case INT32:
            return "INT32";
        case INT64:
            return "INT64";
        case FLOAT32:
            return "FLOAT32";
        case FLOAT64:
            return "FLOAT64";
        default:
            return "???";
    }
}

std::optional<std::pair<std::string, size_t>> Gate::CheckNullInput() const
{
    const auto numIns = this->GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        if (this->IsInGateNull(idx)) {
            return std::make_pair("In list contains null", idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckStateInput() const
{
    const auto numInsArray = this->GetOpCode().GetOpCodeNumInsArray(this->GetBitField());
    size_t stateStart = 0;
    size_t stateEnd = numInsArray[0];
    for (size_t idx = stateStart; idx < stateEnd; idx++) {
        auto stateProp = this->GetOpCode().GetProperties().statesIn;
        ASSERT(stateProp.has_value());
        auto expectedIn = this->GetOpCode().GetInStateCode(idx);
        auto actualIn = this->GetInGateConst(idx)->GetOpCode();
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
    const auto numInsArray = this->GetOpCode().GetOpCodeNumInsArray(this->GetBitField());
    size_t valueStart = numInsArray[0] + numInsArray[1];
    size_t valueEnd = numInsArray[0] + numInsArray[1] + numInsArray[2]; // 2 : 2 means the third element.
    for (size_t idx = valueStart; idx < valueEnd; idx++) {
        auto expectedIn = this->GetOpCode().GetInValueCode(this->GetBitField(), idx);
        auto actualIn = this->GetInGateConst(idx)->GetOpCode().GetValueCode();
        if ((expectedIn != actualIn) && (expectedIn != ANYVALUE)) {
            return std::make_pair("Value input does not match (expected: " + ValueCodeToStr(expectedIn) +
                    " actual: " + ValueCodeToStr(actualIn) + ")",
                idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckDependInput() const
{
    const auto numInsArray = this->GetOpCode().GetOpCodeNumInsArray(this->GetBitField());
    size_t dependStart = numInsArray[0];
    size_t dependEnd = dependStart + numInsArray[1];
    for (size_t idx = dependStart; idx < dependEnd; idx++) {
        if (this->GetInGateConst(idx)->GetNumInsArray()[1] == 0 &&
            this->GetInGateConst(idx)->GetOpCode() != OpCode::DEPEND_ENTRY) {
            return std::make_pair("Depend input is side-effect free", idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckStateOutput() const
{
    if (this->GetOpCode().IsState()) {
        size_t cnt = 0;
        const Gate *curGate = this;
        if (!curGate->IsFirstOutNull()) {
            const Out *curOut = curGate->GetFirstOutConst();
            if (curOut->GetGateConst()->GetOpCode().IsState()) {
                cnt++;
            }
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                if (curOut->GetGateConst()->GetOpCode().IsState()) {
                    cnt++;
                }
            }
        }
        size_t expected = 0;
        bool needCheck = true;
        if (this->GetOpCode().IsTerminalState()) {
            expected = 0;
        } else if (this->GetOpCode() == OpCode::IF_BRANCH) {
            expected = 2; // 2: expected number of state out branches
        } else if (this->GetOpCode() == OpCode::SWITCH_BRANCH) {
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
    if (this->GetOpCode() == OpCode::IF_BRANCH || this->GetOpCode() == OpCode::SWITCH_BRANCH) {
        size_t cnt = 0;
        const Gate *curGate = this;
        if (!curGate->IsFirstOutNull()) {
            const Out *curOut = curGate->GetFirstOutConst();
            if (curOut->GetGateConst()->GetOpCode().IsState()) {
                setOfOps[{curOut->GetGateConst()->GetOpCode(), curOut->GetGateConst()->GetBitField()}]++;
                cnt++;
            }
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                if (curOut->GetGateConst()->GetOpCode().IsState()) {
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
    if (this->GetOpCode() == OpCode::NOP) {
        if (!this->IsFirstOutNull()) {
            return std::make_pair("NOP gate used by other gates", -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckSelector() const
{
    if (this->GetOpCode() == OpCode::VALUE_SELECTOR_JS ||
        (OpCode::VALUE_SELECTOR_INT1 <= this->GetOpCode() && this->GetOpCode() <= OpCode::VALUE_SELECTOR_FLOAT64) ||
        this->GetOpCode() == OpCode::DEPEND_SELECTOR) {
        auto stateOp = this->GetInGateConst(0)->GetOpCode();
        if (stateOp == OpCode::MERGE || stateOp == OpCode::LOOP_BEGIN) {
            if (this->GetInGateConst(0)->GetNumIns() != this->GetNumIns() - 1) {
                if (this->GetOpCode() == OpCode::DEPEND_SELECTOR) {
                    return std::make_pair("Number of depend flows does not match control flows (expected:" +
                            std::to_string(this->GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(this->GetNumIns() - 1) + ")",
                        -1);
                } else {
                    return std::make_pair("Number of data flows does not match control flows (expected:" +
                            std::to_string(this->GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(this->GetNumIns() - 1) + ")",
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
    if (this->GetOpCode() == OpCode::DEPEND_RELAY) {
        auto stateOp = this->GetInGateConst(0)->GetOpCode();
        if (!(stateOp == OpCode::IF_TRUE || stateOp == OpCode::IF_FALSE || stateOp == OpCode::SWITCH_CASE ||
            stateOp == OpCode::DEFAULT_CASE)) {
            return std::make_pair(
                "State input does not match (expected:[IF_TRUE|IF_FALSE|SWITCH_CASE|DEFAULT_CASE] actual:" +
                    stateOp.Str() + ")",
                0);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::SpecialCheck() const
{
    {
        auto ret = this->CheckNOP();
        if (ret.has_value()) {
            return ret;
        }
    }
    {
        auto ret = this->CheckSelector();
        if (ret.has_value()) {
            return ret;
        }
    }
    {
        auto ret = this->CheckRelay();
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
        auto ret = this->CheckNullInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = this->CheckStateInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = this->CheckValueInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = this->CheckDependInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = this->CheckStateOutput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = this->CheckBranchOutput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = this->SpecialCheck();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (failed) {
        std::cerr << "[Verifier][Error] Gate level input list schema verify failed" << std::endl;
        this->Print(true, highlightIdx);
        std::cerr << "Note: " << errorString << std::endl;
    }
    return !failed;
}

ValueCode JSValueCode()
{
    return ValueCode::INT64;
}

ValueCode PtrValueCode()
{
#ifdef PANDA_TARGET_AMD64
    return ValueCode::INT64;
#endif
#ifdef PANDA_TARGET_X86
    return ValueCode::INT32;
#endif
#ifdef PANDA_TARGET_ARM64
    return ValueCode::INT64;
#endif
#ifdef PANDA_TARGET_ARM32
    return ValueCode::INT32;
#endif
}

size_t GetValueBits(ValueCode valueCode)
{
    switch (valueCode) {
        case NOVALUE:
            return 0;  // 0: 0 means 0 bits
        case INT1:
            return 1;  // 1: 1 means 1 bits
        case INT8:
            return 8;  // 8: 8 means 8 bits
        case INT16:
            return 16;  // 16: 16 means 16 bits
        case INT32:
            return 32;  // 32: 32 means 32 bits
        case INT64:
            return 64;  // 64: 64 means 64 bits
        case FLOAT32:
            return 32;  // 32: 32 means 32 bits
        case FLOAT64:
            return 64;  // 64: 64 means 64 bits
        default:
            UNREACHABLE();
    }
}

size_t GetOpCodeNumIns(OpCode opcode, BitField bitfield)
{
    return opcode.GetOpCodeNumIns(bitfield);
}

void Out::SetNextOut(const Out *ptr)
{
    this->nextOut =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<AddrShift>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Out *Out::GetNextOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + this->nextOut);
}

const Out *Out::GetNextOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + this->nextOut);
}

void Out::SetPrevOut(const Out *ptr)
{
    this->prevOut =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<AddrShift>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Out *Out::GetPrevOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + this->prevOut);
}

const Out *Out::GetPrevOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + this->prevOut);
}

void Out::SetIndex(OutIdx idx)
{
    this->idx = idx;
}

OutIdx Out::GetIndex() const
{
    return this->idx;
}

Gate *Out::GetGate()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>(&this[this->idx + 1]);
}

const Gate *Out::GetGateConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>(&this[this->idx + 1]);
}

void Out::SetPrevOutNull()
{
    this->prevOut = 0;
}

bool Out::IsPrevOutNull() const
{
    return this->prevOut == 0;
}

void Out::SetNextOutNull()
{
    this->nextOut = 0;
}

bool Out::IsNextOutNull() const
{
    return this->nextOut == 0;
}

void In::SetGate(const Gate *ptr)
{
    this->gatePtr =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<AddrShift>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Gate *In::GetGate()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>((reinterpret_cast<uint8_t *>(this)) + this->gatePtr);
}

const Gate *In::GetGateConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>((reinterpret_cast<const uint8_t *>(this)) + this->gatePtr);
}

void In::SetGateNull()
{
    this->gatePtr = 0;
}

bool In::IsGateNull() const
{
    return this->gatePtr == 0;
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
Gate::Gate(GateId id, OpCode opcode, BitField bitfield, Gate *inList[], TypeCode type, MarkCode mark)
    : id(id), opcode(opcode), type(type), stamp(1), mark(mark), bitfield(bitfield), firstOut(0)
{
    auto numIns = this->GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto in = inList[idx];
        if (in == nullptr) {
            this->GetIn(idx)->SetGateNull();
        } else {
            this->NewIn(idx, in);
        }
        auto curOut = this->GetOut(idx);
        curOut->SetIndex(idx);
    }
}

size_t Gate::GetOutListSize(size_t numIns)
{
    return numIns * sizeof(Out);
}

size_t Gate::GetOutListSize() const
{
    return Gate::GetOutListSize(this->GetNumIns());
}

size_t Gate::GetInListSize(size_t numIns)
{
    return numIns * sizeof(In);
}

size_t Gate::GetInListSize() const
{
    return Gate::GetInListSize(this->GetNumIns());
}

size_t Gate::GetGateSize(size_t numIns)
{
    return Gate::GetOutListSize(numIns) + Gate::GetInListSize(numIns) + sizeof(Gate);
}

size_t Gate::GetGateSize() const
{
    return Gate::GetGateSize(this->GetNumIns());
}

void Gate::NewIn(size_t idx, Gate *in)
{
    this->GetIn(idx)->SetGate(in);
    auto curOut = this->GetOut(idx);
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
    this->DeleteIn(idx);
    this->NewIn(idx, in);
}

void Gate::DeleteIn(size_t idx)
{
    if (!this->GetOut(idx)->IsNextOutNull() && !this->GetOut(idx)->IsPrevOutNull()) {
        this->GetOut(idx)->GetPrevOut()->SetNextOut(this->GetOut(idx)->GetNextOut());
        this->GetOut(idx)->GetNextOut()->SetPrevOut(this->GetOut(idx)->GetPrevOut());
    } else if (this->GetOut(idx)->IsNextOutNull() && !this->GetOut(idx)->IsPrevOutNull()) {
        this->GetOut(idx)->GetPrevOut()->SetNextOutNull();
    } else if (!this->GetOut(idx)->IsNextOutNull()) {  // then this->GetOut(idx)->IsPrevOutNull() is true
        this->GetIn(idx)->GetGate()->SetFirstOut(this->GetOut(idx)->GetNextOut());
        this->GetOut(idx)->GetNextOut()->SetPrevOutNull();
    } else {  // only this out now
        this->GetIn(idx)->GetGate()->SetFirstOutNull();
    }
    this->GetIn(idx)->SetGateNull();
}

void Gate::DeleteGate()
{
    auto numIns = this->GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        this->DeleteIn(idx);
    }
    this->SetOpCode(OpCode(OpCode::NOP));
}

Out *Gate::GetOut(size_t idx)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<Out *>(this)[-1 - idx];
}

Out *Gate::GetFirstOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + this->firstOut);
}

const Out *Gate::GetFirstOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + this->firstOut);
}

void Gate::SetFirstOutNull()
{
    this->firstOut = 0;
}

bool Gate::IsFirstOutNull() const
{
    return this->firstOut == 0;
}

void Gate::SetFirstOut(const Out *firstOut)
{
    this->firstOut =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<AddrShift>(reinterpret_cast<const uint8_t *>(firstOut) - reinterpret_cast<const uint8_t *>(this));
}

In *Gate::GetIn(size_t idx)
{
#ifndef NDEBUG
    if (idx >= this->GetNumIns()) {
        std::cerr << std::dec << "Gate In access out-of-bound! (idx=" << idx << ")" << std::endl;
        this->Print();
        ASSERT(false);
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<In *>(this + 1)[idx];
}

const In *Gate::GetInConst(size_t idx) const
{
#ifndef NDEBUG
    if (idx >= this->GetNumIns()) {
        std::cerr << std::dec << "Gate In access out-of-bound! (idx=" << idx << ")" << std::endl;
        this->Print();
        ASSERT(false);
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<const In *>(this + 1)[idx];
}

Gate *Gate::GetInGate(size_t idx)
{
    return this->GetIn(idx)->GetGate();
}

const Gate *Gate::GetInGateConst(size_t idx) const
{
    return this->GetInConst(idx)->GetGateConst();
}

bool Gate::IsInGateNull(size_t idx) const
{
    return this->GetInConst(idx)->IsGateNull();
}

GateId Gate::GetId() const
{
    return id;
}

OpCode Gate::GetOpCode() const
{
    return this->opcode;
}

void Gate::SetOpCode(OpCode opcode)
{
    this->opcode = opcode;
}

size_t Gate::GetNumIns() const
{
    return GetOpCodeNumIns(this->GetOpCode(), this->GetBitField());
}

std::array<size_t, 4> Gate::GetNumInsArray() const // 4 : 4 means that there are 4 args.
{
    return this->GetOpCode().GetOpCodeNumInsArray(this->GetBitField());
}

BitField Gate::GetBitField() const
{
    return this->bitfield;
}

void Gate::SetBitField(BitField bitfield)
{
    this->bitfield = bitfield;
}

void Gate::Print(bool inListPreview, size_t highlightIdx) const
{
    if (this->GetOpCode() != OpCode::NOP) {
        std::cerr << std::dec << "("
                  << "id=" << this->id << ", "
                  << "op=" << this->GetOpCode().Str() << ", "
                  << "bitfield=" << std::to_string(this->bitfield) << ", "
                  << "type=" << static_cast<uint32_t>(this->type) << ", "
                  << "stamp=" << static_cast<uint32_t>(this->stamp) << ", "
                  << "mark=" << static_cast<uint32_t>(this->mark) << ", ";
        std::cerr << "in="
                  << "[";
        for (size_t idx = 0; idx < this->GetNumIns(); idx++) {
            std::cerr << std::dec << ((idx == 0) ? "" : " ") << ((idx == highlightIdx) ? "\033[4;31m" : "")
                      << ((this->IsInGateNull(idx)
                                 ? "N"
                                 : (std::to_string(this->GetInGateConst(idx)->GetId()) +
                                       (inListPreview ? std::string(":" + this->GetInGateConst(idx)->GetOpCode().Str())
                                                      : std::string("")))))
                      << ((idx == highlightIdx) ? "\033[0m" : "");
        }
        std::cerr << "]"
                  << ", ";
        std::cerr << "out="
                  << "[";
        if (!this->IsFirstOutNull()) {
            const Out *curOut = this->GetFirstOutConst();
            std::cerr << std::dec << ""
                      << std::to_string(curOut->GetGateConst()->GetId()) +
                    (inListPreview ? std::string(":" + curOut->GetGateConst()->GetOpCode().Str()) : std::string(""));
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                std::cerr << std::dec << " "
                          << std::to_string(curOut->GetGateConst()->GetId()) +
                        (inListPreview ? std::string(":" + curOut->GetGateConst()->GetOpCode().Str())
                                       : std::string(""));
            }
        }
        std::cerr << "]"
                  << ")" << std::endl;
    }
}

MarkCode Gate::GetMark(TimeStamp stamp) const
{
    return (this->stamp == stamp) ? this->mark : MarkCode::EMPTY;
}

void Gate::SetMark(MarkCode mark, TimeStamp stamp)
{
    this->stamp = stamp;
    this->mark = mark;
}

bool OpCode::IsRoot() const
{
    return (this->GetProperties().states == OpCode::CIRCUIT_ROOT) || (this->op == OpCode::CIRCUIT_ROOT);
}

bool OpCode::IsProlog() const
{
    return (this->GetProperties().states == OpCode::ARG_LIST);
}

bool OpCode::IsFixed() const
{
    return (this->GetOpCodeNumInsArray(1)[0] > 0) &&
        ((this->GetValueCode() != NOVALUE) ||
            ((this->GetOpCodeNumInsArray(1)[1] > 0) && (this->GetOpCodeNumInsArray(1)[2] == 0)));
}

bool OpCode::IsSchedulable() const
{
    return (this->op != OpCode::NOP) && (!this->IsProlog()) && (!this->IsRoot()) && (!this->IsFixed()) &&
        (this->GetOpCodeNumInsArray(1)[0] == 0);
}

bool OpCode::IsState() const
{
    return (this->op != OpCode::NOP) && (!this->IsProlog()) && (!this->IsRoot()) && (!this->IsFixed()) &&
        (this->GetOpCodeNumInsArray(1)[0] > 0);
}

bool OpCode::IsGeneralState() const
{
    return ((this->op == OpCode::IF_TRUE) || (this->op == OpCode::IF_FALSE) || (this->op == OpCode::SWITCH_CASE) ||
        (this->op == OpCode::DEFAULT_CASE) || (this->op == OpCode::MERGE) || (this->op == OpCode::LOOP_BEGIN) ||
        (this->op == OpCode::ORDINARY_BLOCK) || (this->op == OpCode::STATE_ENTRY));
}

bool OpCode::IsTerminalState() const
{
    return ((this->op == OpCode::RETURN) || (this->op == OpCode::THROW));
}

bool OpCode::IsCFGMerge() const
{
    return (this->op == OpCode::MERGE) || (this->op == OpCode::LOOP_BEGIN);
}

bool OpCode::IsControlCase() const
{
    return (this->op == OpCode::IF_BRANCH) || (this->op == OpCode::SWITCH_BRANCH) || (this->op == OpCode::IF_TRUE) ||
           (this->op == OpCode::IF_FALSE) || (this->op == OpCode::SWITCH_CASE) || (this->op == OpCode::DEFAULT_CASE);
}

bool OpCode::IsLoopHead() const
{
    return (this->op == OpCode::LOOP_BEGIN);
}

bool OpCode::IsNop() const
{
    return (this->op == OpCode::NOP);
}
}  // namespace kungfu