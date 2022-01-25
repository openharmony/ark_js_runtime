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

#include "ecmascript/compiler/type.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
using GateRef = int32_t; // for external users
using GateId = uint32_t;
using GateOp = uint8_t;
using GateMark = uint8_t;
using TimeStamp = uint8_t;
using SecondaryOp = uint8_t;
using BitField = uint64_t;
using OutIdx = uint32_t;
class Gate;
struct Properties;
class BytecodeCircuitBuilder;

enum MachineType { // bit whith
    NOVALUE,
    ANYVALUE,
    ARCH,
    FLEX,
    I1,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
};

std::string MachineTypeToStr(MachineType machineType);

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
        RETURN_VOID,
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
        VALUE_SELECTOR,
        DEPEND_SELECTOR,
        DEPEND_RELAY,
        DEPEND_AND,
        // High Level IR
        JS_BYTECODE,
        IF_SUCCESS,
        IF_EXCEPTION,
        GET_EXCEPTION,
        // Middle Level IR
        CALL,
        ALLOCA,
        ARG,
        MUTABLE_DATA,
        RELOCATABLE_DATA,
        CONST_DATA,
        CONSTANT,
        ZEXT_TO_INT64,
        ZEXT_TO_INT32,
        SEXT_TO_INT64,
        SEXT_TO_INT32,
        TRUNC_TO_INT32,
        TRUNC_TO_INT1,
        REV,
        ADD,
        SUB,
        MUL,
        EXP,
        SDIV,
        SMOD,
        UDIV,
        UMOD,
        FDIV,
        FMOD,
        AND,
        XOR,
        OR,
        LSL,
        LSR,
        ASR,
        SLT,
        SLE,
        SGT,
        SGE,
        ULT,
        ULE,
        UGT,
        UGE,
        FLT,
        FLE,
        FGT,
        FGE,
        EQ,
        NE,
        LOAD,
        STORE,
        TAGGED_TO_INT64,
        SIGNED_INT_TO_FLOAT,
        UNSIGNED_INT_TO_FLOAT,
        FLOAT_TO_SIGNED_INT,
        UNSIGNED_FLOAT_TO_INT,
        BITCAST,
    };

    OpCode() = default;
    explicit constexpr OpCode(Op op) : op_(op) {}
    operator Op() const
    {
        return op_;
    }
    explicit operator bool() const = delete;
    [[nodiscard]] Properties GetProperties() const;
    [[nodiscard]] std::array<size_t, 4> GetOpCodeNumInsArray(BitField bitfield) const;
    [[nodiscard]] size_t GetOpCodeNumIns(BitField bitfield) const;
    [[nodiscard]] MachineType GetMachineType() const;
    [[nodiscard]] MachineType GetInMachineType(BitField bitfield, size_t idx) const;
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
    Op op_;
};

struct Properties {
    MachineType returnValue;
    std::optional<std::pair<std::vector<OpCode>, bool>> statesIn;
    size_t dependsIn;
    std::optional<std::pair<std::vector<MachineType>, bool>> valuesIn;
    std::optional<OpCode> states;
};

enum MarkCode : GateMark {
    NO_MARK,
    VISITED,
    FINISHED,
};

MachineType JSMachineType();

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
    OutIdx idx_;
    GateRef nextOut_;
    GateRef prevOut_;
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
    GateRef gatePtr_;
};

// Gate structure
// for example:
// ```
// g0 = op0(...)
// g1 = op1(...)
// g2 = op2(g0, g1)
// g3 = op3(g2)
// g4 = op4(g2, g0, g1)
// g5 = op5(g3, g4)

// +---- out[1] ----+---- out[0] ----+-------- g2 --------+-- in[0] --+-- in[1] --+
// |                |                |                    |           |           |
// | prev=null      | prev=null      | ...                |           |           |
// | idx=1          | idx=0          |                    |    g0     |    g1     |
// | next=g4.out[2] | next=g4.out[1] | firstOut=g3.out[0] |           |           |
// |                |                |                    |           |           |
// +----------------+----------------+--------------------+-----------+-----------+
//       /\               /\
//       ||               ||
//       ||               ||
//       ||               ||         +---- out[0] ----+-------- g3 --------+-- in[0] --+
//       ||               ||         |                |                    |           |
//       ||               ||         | prev=null      | ...                |           |
//       ||               ||         | idx=0          |                    |    g2     |
//       ||               ||         | next=g4.out[0] | firstOut=g5.out[0] |           |
//       ||               ||         |                |                    |           |
//       ||               ||         +----------------+--------------------+-----------+
//       ||               ||               /\
//       ||               ||               ||
//       ||               ||               ||
//       \/               \/               \/
// +---- out[2] ----+---- out[1] ----+---- out[0] ----+-------- g4 --------+-- in[0] --+-- in[1] --+-- in[2] --+
// |                |                |                |                    |           |           |           |
// | prev=g2.out[1] | prev=g2.out[0] | prev=g3.out[0] | ...                |           |           |           |
// | idx=2          | idx=1          | idx=0          |                    |    g2     |    g0     |    g1     |
// | next=null      | next=null      | next=g5.out[1] | firstOut=g5.out[1] |           |           |           |
// |                |                |                |                    |           |           |           |
// +----------------+----------------+----------------+--------------------+-----------+-----------+-----------+
// ```

class Gate {
public:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    Gate(GateId id, OpCode opcode, MachineType bitValue, BitField bitfield, Gate *inList[], GateType type,
         MarkCode mark);
    Gate(GateId id, OpCode opcode, BitField bitfield, Gate *inList[], GateType type, MarkCode mark);
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
    [[nodiscard]] GateType GetGateType() const;
    void SetGateType(GateType type);
    [[nodiscard]] GateId GetId() const;
    [[nodiscard]] size_t GetNumIns() const;
    [[nodiscard]] std::array<size_t, 4> GetNumInsArray() const;
    [[nodiscard]] BitField GetBitField() const;
    void SetBitField(BitField bitfield);
    void AppendIn(const Gate *in);  // considered very slow
    void Print(std::string bytecode = "", bool inListPreview = false, size_t highlightIdx = -1) const;
    size_t PrintInGate(size_t numIns, size_t idx, size_t size, bool inListPreview, size_t highlightIdx,
                       bool isEnd = false) const;
    void PrintByteCode(std::string bytecode) const;
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
    [[nodiscard]] MachineType GetMachineType() const;
    void SetMachineType(MachineType MachineType);
    std::string MachineTypeStr(MachineType machineType) const;
    std::string GateTypeStr(GateType gateType) const;
    ~Gate() = default;

private:
    // ...
    // out(2)
    // out(1)
    // out(0)
    GateId id_ {0};
    OpCode opcode_;
    MachineType bitValue_ = MachineType::NOVALUE;
    GateType type_;
    TimeStamp stamp_;
    MarkCode mark_;
    BitField bitfield_;
    GateRef firstOut_;
    // in(0)
    // in(1)
    // in(2)
    // ...
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GATE_H
