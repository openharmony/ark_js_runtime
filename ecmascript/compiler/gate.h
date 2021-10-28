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

#ifndef ECMASCRIPT_COMPILER_GATE_H
#define ECMASCRIPT_COMPILER_GATE_H

#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "libpandabase/macros.h"
#include "ecmascript/compiler/type.h"

namespace kungfu {
using GateId = uint32_t;
using GateOp = uint8_t;
using GateMark = uint8_t;
using TimeStamp = uint8_t;
using AddrShift = int32_t;
using BitField = uint64_t;
using OutIdx = uint32_t;
class Gate;

enum ValueCode {
    NOVALUE,
    ANYVALUE,
    INT1,
    INT8,
    INT16,
    INT32,
    INT64,
    FLOAT32,
    FLOAT64,
    TAGGED_POINTER,
};

std::string ValueCodeToStr(ValueCode valueCode);

using Properties = struct Properties;

class OpCode {
public:
    enum Op : GateOp {
        // SHARED
        NOP,
        CIRCUIT_ROOT,
        STATE_ENTRY,
        DEPEND_ENTRY,
        FRAMESTATE_ENTRY,
        RETURN_LIST,
        THROW_LIST,
        CONSTANT_LIST,
        ALLOCA_LIST,
        ARG_LIST,
        RETURN,
        THROW,
        ORDINARY_BLOCK,
        IF_BRANCH,
        SWITCH_BRANCH,
        IF_TRUE,
        IF_FALSE,
        SWITCH_CASE,
        DEFAULT_CASE,
        MERGE,
        LOOP_BEGIN,
        LOOP_BACK,
        VALUE_SELECTOR_JS,
        VALUE_SELECTOR_INT1,
        VALUE_SELECTOR_INT8,
        VALUE_SELECTOR_INT16,
        VALUE_SELECTOR_INT32,
        VALUE_SELECTOR_INT64,
        VALUE_SELECTOR_FLOAT32,
        VALUE_SELECTOR_FLOAT64,
        DEPEND_SELECTOR,
        DEPEND_RELAY,
        DEPEND_AND,
        // High Level IR
        JS_CALL,
        JS_CONSTANT,
        JS_ARG,
        JS_ADD,
        JS_SUB,
        JS_MUL,
        JS_EXP,
        JS_DIV,
        JS_MOD,
        JS_AND,
        JS_XOR,
        JS_OR,
        JS_LSL,
        JS_LSR,
        JS_ASR,
        JS_LOGIC_AND,
        JS_LOGIC_OR,
        JS_LT,
        JS_LE,
        JS_GT,
        JS_GE,
        JS_EQ,
        JS_NE,
        JS_STRICT_EQ,
        JS_STRICT_NE,
        JS_LOGIC_NOT,
        // Middle Level IR
        CALL,
        INT1_CALL,
        INT8_CALL,
        INT16_CALL,
        INT32_CALL,
        INT64_CALL,
        FLOAT32_CALL,
        FLOAT64_CALL,
        TAGGED_POINTER_CALL,
        ALLOCA,
        INT1_ARG,
        INT8_ARG,
        INT16_ARG,
        INT32_ARG,
        INT64_ARG,
        FLOAT32_ARG,
        FLOAT64_ARG,
        MUTABLE_DATA,
        CONST_DATA,
        INT1_CONSTANT,
        INT8_CONSTANT,
        INT16_CONSTANT,
        INT32_CONSTANT,
        INT64_CONSTANT,
        FLOAT32_CONSTANT,
        FLOAT64_CONSTANT,
        ZEXT_INT32_TO_INT64,
        ZEXT_INT1_TO_INT32,
        ZEXT_INT1_TO_INT64,
        SEXT_INT32_TO_INT64,
        SEXT_INT1_TO_INT32,
        SEXT_INT1_TO_INT64,
        TRUNC_INT64_TO_INT32,
        TRUNC_INT64_TO_INT1,
        TRUNC_INT32_TO_INT1,
        INT32_REV,
        INT32_ADD,
        INT32_SUB,
        INT32_MUL,
        INT32_EXP,
        INT32_SDIV,
        INT32_SMOD,
        INT32_UDIV,
        INT32_UMOD,
        INT32_AND,
        INT32_XOR,
        INT32_OR,
        INT32_LSL,
        INT32_LSR,
        INT32_ASR,
        INT32_SLT,
        INT32_SLE,
        INT32_SGT,
        INT32_SGE,
        INT32_ULT,
        INT32_ULE,
        INT32_UGT,
        INT32_UGE,
        INT32_EQ,
        INT32_NE,
        INT64_REV,
        INT64_ADD,
        INT64_SUB,
        INT64_MUL,
        INT64_EXP,
        INT64_SDIV,
        INT64_SMOD,
        INT64_UDIV,
        INT64_UMOD,
        INT64_AND,
        INT64_XOR,
        INT64_OR,
        INT64_LSL,
        INT64_LSR,
        INT64_ASR,
        INT64_SLT,
        INT64_SLE,
        INT64_SGT,
        INT64_SGE,
        INT64_ULT,
        INT64_ULE,
        INT64_UGT,
        INT64_UGE,
        INT64_EQ,
        INT64_NE,
        FLOAT64_ADD,
        FLOAT64_SUB,
        FLOAT64_MUL,
        FLOAT64_DIV,
        FLOAT64_EXP,
        FLOAT64_EQ,
        FLOAT64_SMOD,
        INT8_LOAD,
        INT16_LOAD,
        INT32_LOAD,
        INT64_LOAD,
        FLOAT32_LOAD,
        FLOAT64_LOAD,
        INT8_STORE,
        INT16_STORE,
        INT32_STORE,
        INT64_STORE,
        FLOAT32_STORE,
        FLOAT64_STORE,
        INT32_TO_FLOAT64,
        INT64_TO_FLOAT64,
        FLOAT64_TO_INT64,
        TAG64_TO_INT1,
    };

    OpCode() = default;
    explicit constexpr OpCode(Op op) : op(op) {}
    operator Op() const
    {
        return this->op;
    }
    explicit operator bool() const = delete;
    [[nodiscard]] Properties GetProperties() const;
    [[nodiscard]] std::array<size_t, 4> GetOpCodeNumInsArray(BitField bitfield) const;
    [[nodiscard]] size_t GetOpCodeNumIns(BitField bitfield) const;
    [[nodiscard]] ValueCode GetValueCode() const;
    [[nodiscard]] ValueCode GetInValueCode(BitField bitfield, size_t idx) const;
    [[nodiscard]] OpCode GetInStateCode(size_t idx) const;
    [[nodiscard]] std::string Str() const;
    [[nodiscard]] bool IsRoot() const;
    [[nodiscard]] bool IsProlog() const;
    [[nodiscard]] bool IsFixed() const;
    [[nodiscard]] bool IsSchedulable() const;
    [[nodiscard]] bool IsState() const;  // note: IsState(STATE_ENTRY) == false
    [[nodiscard]] bool IsGeneralState() const;
    [[nodiscard]] bool IsTerminalState() const;
    [[nodiscard]] bool IsCFGMerge() const;
    [[nodiscard]] bool IsControlCase() const;
    [[nodiscard]] bool IsLoopHead() const;
    [[nodiscard]] bool IsNop() const;
    ~OpCode() = default;

private:
    Op op;
};

struct Properties {
    ValueCode returnValue;
    std::optional<std::pair<std::vector<OpCode>, bool>> statesIn;
    size_t dependsIn;
    std::optional<std::pair<std::vector<ValueCode>, bool>> valuesIn;
    std::optional<OpCode> states;
};

enum MarkCode : GateMark {
    EMPTY,
    VISITED,
    FINISHED,
};

ValueCode JSValueCode();
ValueCode PtrValueCode();
size_t GetValueBits(ValueCode valueCode);

class Out {
public:
    Out() = default;
    void SetNextOut(const Out *ptr);
    [[nodiscard]] Out *GetNextOut();
    [[nodiscard]] const Out *GetNextOutConst() const;
    void SetPrevOut(const Out *ptr);
    [[nodiscard]] Out *GetPrevOut();
    [[nodiscard]] const Out *GetPrevOutConst() const;
    void SetIndex(OutIdx idx);
    [[nodiscard]] OutIdx GetIndex() const;
    [[nodiscard]] Gate *GetGate();
    [[nodiscard]] const Gate *GetGateConst() const;
    void SetPrevOutNull();
    [[nodiscard]] bool IsPrevOutNull() const;
    void SetNextOutNull();
    [[nodiscard]] bool IsNextOutNull() const;
    ~Out() = default;

private:
    OutIdx idx;
    AddrShift nextOut;
    AddrShift prevOut;
};

class In {
public:
    In() = default;
    void SetGate(const Gate *ptr);
    [[nodiscard]] Gate *GetGate();
    [[nodiscard]] const Gate *GetGateConst() const;
    void SetGateNull();
    [[nodiscard]] bool IsGateNull() const;
    ~In() = default;

private:
    AddrShift gatePtr;
};

class Gate {
public:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    Gate(GateId id, OpCode opcode, BitField bitfield, Gate *inList[], TypeCode type, MarkCode mark);
    [[nodiscard]] static size_t GetGateSize(size_t numIns);
    [[nodiscard]] size_t GetGateSize() const;
    [[nodiscard]] static size_t GetOutListSize(size_t numIns);
    [[nodiscard]] size_t GetOutListSize() const;
    [[nodiscard]] static size_t GetInListSize(size_t numIns);
    [[nodiscard]] size_t GetInListSize() const;
    void NewIn(size_t idx, Gate *in);
    void ModifyIn(size_t idx, Gate *in);
    void DeleteIn(size_t idx);
    void DeleteGate();
    [[nodiscard]] Out *GetOut(size_t idx);
    [[nodiscard]] Out *GetFirstOut();
    [[nodiscard]] const Out *GetFirstOutConst() const;
    // note: GetFirstOut() is not equal to GetOut(0)
    // note: behavior of GetFirstOut() is undefined when there are no Outs
    // note: use IsFirstOutNull() to check first if there may be no Outs
    void SetFirstOut(const Out *firstOut);
    void SetFirstOutNull();
    [[nodiscard]] bool IsFirstOutNull() const;
    [[nodiscard]] In *GetIn(size_t idx);
    [[nodiscard]] const In *GetInConst(size_t idx) const;
    [[nodiscard]] Gate *GetInGate(size_t idx);
    [[nodiscard]] const Gate *GetInGateConst(size_t idx) const;
    // note: behavior of GetInGate(idx) is undefined when Ins[idx] is deleted or not assigned
    // note: use IsInGateNull(idx) to check first if Ins[idx] may be deleted or not assigned
    [[nodiscard]] bool IsInGateNull(size_t idx) const;
    [[nodiscard]] OpCode GetOpCode() const;
    void SetOpCode(OpCode opcode);
    [[nodiscard]] GateId GetId() const;
    [[nodiscard]] size_t GetNumIns() const;
    [[nodiscard]] std::array<size_t, 4> GetNumInsArray() const;
    [[nodiscard]] BitField GetBitField() const;
    void SetBitField(BitField bitfield);
    void AppendIn(const Gate *in);  // considered very slow
    void Print(bool inListPreview = false, size_t highlightIdx = -1) const;
    std::optional<std::pair<std::string, size_t>> CheckNullInput() const;
    std::optional<std::pair<std::string, size_t>> CheckStateInput() const;
    std::optional<std::pair<std::string, size_t>> CheckValueInput() const;
    std::optional<std::pair<std::string, size_t>> CheckDependInput() const;
    std::optional<std::pair<std::string, size_t>> CheckStateOutput() const;
    std::optional<std::pair<std::string, size_t>> CheckBranchOutput() const;
    std::optional<std::pair<std::string, size_t>> CheckNOP() const;
    std::optional<std::pair<std::string, size_t>> CheckSelector() const;
    std::optional<std::pair<std::string, size_t>> CheckRelay() const;
    std::optional<std::pair<std::string, size_t>> SpecialCheck() const;
    [[nodiscard]] bool Verify() const;
    [[nodiscard]] MarkCode GetMark(TimeStamp stamp) const;
    void SetMark(MarkCode mark, TimeStamp stamp);
    TypeCode GetTypeCode() const;
    ~Gate() = default;

private:
    // ...
    // out(2)
    // out(1)
    // out(0)
    GateId id;
    OpCode opcode;
    TypeCode type;
    TimeStamp stamp;
    MarkCode mark;
    BitField bitfield;
    AddrShift firstOut;
    // in(0)
    // in(1)
    // in(2)
    // ...
};
}  // namespace kungfu

#endif  // ECMASCRIPT_COMPILER_GATE_H
