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

#ifndef ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
#define ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H

#include <numeric>
#include <tuple>
#include <utility>
#include <vector>
#include <variant>

#include "circuit.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_method.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::kungfu {
using VRegIDType = uint16_t;
using ImmValueType = uint64_t;
using StringIdType = uint32_t;
using MethodIdType = uint16_t;

class VirtualRegister {
public:
    explicit VirtualRegister(VRegIDType id) : id_(id)
    {
    }
    ~VirtualRegister() = default;

    void SetId(VRegIDType id)
    {
        id_ = id;
    }

    VRegIDType GetId() const
    {
        return id_;
    }

private:
    VRegIDType id_;
};

class Immediate {
public:
    explicit Immediate(ImmValueType value) : value_(value)
    {
    }
    ~Immediate() = default;

    void SetValue(ImmValueType value)
    {
        value_ = value;
    }

    ImmValueType ToJSTaggedValueInt() const
    {
        return value_ | JSTaggedValue::TAG_INT;
    }

    ImmValueType ToJSTaggedValueDouble() const
    {
        return JSTaggedValue(bit_cast<double>(value_)).GetRawData();
    }

    ImmValueType GetValue() const
    {
        return value_;
    }

private:
    ImmValueType value_;
};

class StringId {
public:
    explicit StringId(StringIdType id) : id_(id)
    {
    }
    ~StringId() = default;

    void SetId(StringIdType id)
    {
        id_ = id;
    }

    StringIdType GetId() const
    {
        return id_;
    }

private:
    StringIdType id_;
};

class MethodId {
public:
    explicit MethodId(MethodIdType id) : id_(id)
    {
    }
    ~MethodId() = default;

    void SetId(MethodIdType id)
    {
        id_ = id;
    }

    MethodIdType GetId() const
    {
        return id_;
    }

private:
    MethodIdType id_;
};

enum class SplitKind : uint8_t {
    DEFAULT,
    START,
    END
};

enum class VisitState : uint8_t {
    UNVISITED,
    PENDING,
    VISITED
};

struct CfgInfo {
    uint8_t *pc {nullptr};
    SplitKind splitKind {SplitKind::DEFAULT};
    std::vector<uint8_t *> succs {};
    CfgInfo(uint8_t *startOrEndPc, SplitKind kind, std::vector<uint8_t *> successors)
        : pc(startOrEndPc), splitKind(kind), succs(successors) {}

    bool operator<(const CfgInfo &rhs) const
    {
        if (this->pc != rhs.pc) {
            return this->pc < rhs.pc;
        } else {
            return this->splitKind < rhs.splitKind;
        }
    }

    bool operator==(const CfgInfo &rhs) const
    {
        return this->pc == rhs.pc && this->splitKind == rhs.splitKind;
    }
};

struct BytecodeRegion {
    size_t id {0};
    uint8_t *start {nullptr};
    uint8_t *end {nullptr};
    std::vector<BytecodeRegion *> preds {}; // List of predessesor blocks
    std::vector<BytecodeRegion *> succs {}; // List of successors blocks
    std::vector<BytecodeRegion *> trys {}; // List of trys blocks
    std::vector<BytecodeRegion *> catchs {}; // List of catches blocks
    std::vector<BytecodeRegion *> immDomBlocks {}; // List of dominated blocks
    BytecodeRegion *iDominator {nullptr}; // Block that dominates the current block
    std::vector<BytecodeRegion *> domFrontiers {}; // List of dominace frontiers
    std::set<size_t> loopbackBlocks {}; // List of loopback block ids
    bool isDead {false};
    std::set<uint16_t> phi {}; // phi node
    bool phiAcc {false};
    size_t numOfStatePreds {0};
    size_t numOfLoopBacks {0};
    size_t statePredIndex {0};
    size_t forwardIndex {0};
    size_t loopBackIndex {0};
    std::vector<std::tuple<size_t, const uint8_t *, bool>> expandedPreds {};
    kungfu::GateRef stateStart {kungfu::Circuit::NullGate()};
    kungfu::GateRef dependStart {kungfu::Circuit::NullGate()};
    kungfu::GateRef mergeForwardEdges {kungfu::Circuit::NullGate()};
    kungfu::GateRef mergeLoopBackEdges {kungfu::Circuit::NullGate()};
    kungfu::GateRef depForward {kungfu::Circuit::NullGate()};
    kungfu::GateRef depLoopBack {kungfu::Circuit::NullGate()};
    std::map<uint16_t, kungfu::GateRef> vregToValSelectorGate {}; // corresponding ValueSelector gates of vregs
    kungfu::GateRef valueSelectorAccGate {kungfu::Circuit::NullGate()};

    bool operator <(const BytecodeRegion &target) const
    {
        return id < target.id;
    }
};

using BytecodeGraph = std::vector<BytecodeRegion>;

struct BytecodeInfo {
    // set of id, immediate and read register
    std::vector<std::variant<StringId, MethodId, Immediate, VirtualRegister>> inputs {};
    std::vector<VRegIDType> vregOut {}; // write register
    bool accIn {false}; // read acc
    bool accOut {false}; // write acc
    uint8_t opcode {0};
    uint16_t offset {0};

    bool IsOut(VRegIDType reg, uint32_t index) const
    {
        bool isDefined = (!vregOut.empty() && (reg == vregOut.at(index)));
        return isDefined;
    }

    bool IsMov() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::MOV_V4_V4:
            case EcmaOpcode::MOV_DYN_V8_V8:
            case EcmaOpcode::MOV_DYN_V16_V16:
            case EcmaOpcode::LDA_DYN_V8:
            case EcmaOpcode::STA_DYN_V8:
                return true;
            default:
                return false;
        }
    }

    bool IsJump() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::JMP_IMM8:
            case EcmaOpcode::JMP_IMM16:
            case EcmaOpcode::JMP_IMM32:
            case EcmaOpcode::JEQZ_IMM8:
            case EcmaOpcode::JEQZ_IMM16:
            case EcmaOpcode::JNEZ_IMM8:
            case EcmaOpcode::JNEZ_IMM16:
                return true;
            default:
                return false;
        }
    }

    bool IsCondJump() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::JEQZ_IMM8:
            case EcmaOpcode::JEQZ_IMM16:
            case EcmaOpcode::JNEZ_IMM8:
            case EcmaOpcode::JNEZ_IMM16:
                return true;
            default:
                return false;
        }
    }

    bool IsReturn() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::RETURN_DYN:
            case EcmaOpcode::RETURNUNDEFINED_PREF:
                return true;
            default:
                return false;
        }
    }

    bool IsThrow() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::THROWDYN_PREF:
            case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8:
            case EcmaOpcode::THROWTHROWNOTEXISTS_PREF:
            case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF:
            case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF:
                return true;
            default:
                return false;
        }
    }

    bool IsDiscarded() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::COPYMODULE_PREF_V8:
            case EcmaOpcode::DEBUGGER_PREF:
                return true;
            default:
                return false;
        }
    }

    bool IsSetConstant() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::LDNAN_PREF:
            case EcmaOpcode::LDINFINITY_PREF:
            case EcmaOpcode::LDUNDEFINED_PREF:
            case EcmaOpcode::LDNULL_PREF:
            case EcmaOpcode::LDTRUE_PREF:
            case EcmaOpcode::LDFALSE_PREF:
            case EcmaOpcode::LDHOLE_PREF:
            case EcmaOpcode::LDAI_DYN_IMM32:
            case EcmaOpcode::FLDAI_DYN_IMM64:
            case EcmaOpcode::LDFUNCTION_PREF:
                return true;
            default:
                return false;
        }
    }

    bool IsGeneral() const
    {
        return !IsMov() && !IsJump() && !IsReturn() && !IsSetConstant() && !IsDiscarded();
    }

    bool IsCall() const
    {
        auto ecmaOpcode = static_cast<EcmaOpcode>(opcode);
        switch (ecmaOpcode) {
            case EcmaOpcode::CALLARG0DYN_PREF_V8:
            case EcmaOpcode::CALLARG1DYN_PREF_V8_V8:
            case EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8:
            case EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8:
            case EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8:
            case EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8:
                return true;
            default:
                return false;
        }
    }

    size_t ComputeBCOffsetInputCount() const
    {
        return IsCall() ? 1 : 0;
    }

    size_t ComputeValueInputCount() const
    {
        return (accIn ? 1 : 0) + inputs.size();
    }

    size_t ComputeOutCount() const
    {
        return accOut ? 1 : 0;
    }

    size_t ComputeTotalValueCount() const
    {
        return ComputeValueInputCount() + ComputeBCOffsetInputCount();
    }
};

enum BytecodeOffset {
    ONE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN
};

enum CommonArgIdx : uint8_t {
    GLUE = 0,
    LEXENV,
    ACTUAL_ARGC,
    FUNC,
    NEW_TARGET,
    THIS,
    NUM_OF_ARGS,
};

class BytecodeCircuitBuilder {
public:
    explicit BytecodeCircuitBuilder(const BytecodeTranslationInfo &translationInfo, size_t index,
                                    TSLoader *tsLoader, bool enableLog)
        : tsLoader_(tsLoader), file_(translationInfo.jsPandaFile), pf_(translationInfo.jsPandaFile->GetPandaFile()),
          method_(translationInfo.methodPcInfos[index].method),
          pcArray_(translationInfo.methodPcInfos[index].pcArray),
          constantPool_(translationInfo.constantPool),
          enableLog_(enableLog)
    {
    }
    ~BytecodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(BytecodeCircuitBuilder);
    NO_MOVE_SEMANTIC(BytecodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit();

    [[nodiscard]] kungfu::Circuit* GetCircuit()
    {
        return &circuit_;
    }

    [[nodiscard]] const std::map<kungfu::GateRef, std::pair<size_t, const uint8_t *>>& GetGateToBytecode() const
    {
        return jsgateToBytecode_;
    }

    [[nodiscard]] const std::map<const uint8_t *, kungfu::GateRef>& GetBytecodeToGate() const
    {
        return byteCodeToJSGate_;
    }

    [[nodiscard]] std::string GetBytecodeStr(kungfu::GateRef gate) const
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return GetEcmaOpcodeStr(static_cast<EcmaOpcode>(*pc));
    }

    [[nodiscard]] EcmaOpcode GetByteCodeOpcode(kungfu::GateRef gate) const
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return static_cast<EcmaOpcode>(*pc);
    }

    [[nodiscard]] const uint8_t* GetJSBytecode(GateRef gate) const
    {
        return jsgateToBytecode_.at(gate).second;
    }

    [[nodiscard]] GateRef GetCommonArgByIndex(CommonArgIdx idx)
    {
        return commonArgs_[idx];
    }

    BytecodeInfo GetBytecodeInfo(const uint8_t *pc);
    // for external users, circuit must be built
    BytecodeInfo GetByteCodeInfo(const GateRef gate)
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return GetBytecodeInfo(pc);
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

private:
    void PUBLIC_API CollectBytecodeBlockInfo(uint8_t* pc, std::vector<CfgInfo> &bytecodeBlockInfos);

    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> CollectTryCatchBlockInfo(
        std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc, std::vector<CfgInfo> &bytecodeBlockInfos);

    void CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
                                   std::vector<CfgInfo> &bytecodeBlockInfos);

    void BuildBasicBlocks(std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
                          std::vector<CfgInfo> &bytecodeBlockInfo,
                          std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc);
    void ComputeDominatorTree();
    void BuildImmediateDominator(const std::vector<size_t> &immDom);
    void ComputeDomFrontiers(const std::vector<size_t> &immDom);
    void RemoveDeadRegions(const std::map<size_t, size_t> &dfsTimestamp);
    void InsertPhi();
    void UpdateCFG();
    // build circuit
    void BuildCircuitArgs();
    void CollectPredsInfo();
    void NewMerge(GateRef &state, GateRef &depend, size_t numOfIns);
    void NewLoopBegin(BytecodeRegion &bb);
    void BuildBlockCircuitHead();
    std::vector<GateRef> CreateGateInList(const BytecodeInfo &info);
    void SetBlockPred(BytecodeRegion &bbNext, const GateRef &state, const GateRef &depend, bool isLoopBack);
    GateRef NewConst(const BytecodeInfo &info);
    void NewJSGate(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewJump(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewReturn(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewByteCode(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void AddBytecodeOffsetInfo(GateRef &gate, const BytecodeInfo &info, size_t bcOffsetIndex, uint8_t *pc);
    void BuildSubCircuit();
    void NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi);
    GateRef RenameVariable(const size_t bbId, const uint8_t *end,
        const uint16_t reg, const bool acc, GateType gateType = GateType::AnyType());
    void BuildCircuit();

    void PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos);
    size_t GetFunctionArgIndex(size_t currentVreg, size_t numVregs) const;
    void PrintGraph();
    void PrintBytecodeInfo();
    void PrintBBInfo();
    GateType GetRealGateType(const uint16_t reg, const GateType gateType);
    size_t GetActualNumArgs(size_t numArgs)
    {
        return numArgs + CommonArgIdx::NUM_OF_ARGS;
    }

    kungfu::Circuit circuit_;
    std::map<kungfu::GateRef, std::pair<size_t, const uint8_t *>> jsgateToBytecode_;
    std::map<const uint8_t *, kungfu::GateRef> byteCodeToJSGate_;
    BytecodeGraph graph_;
    std::array<GateRef, CommonArgIdx::NUM_OF_ARGS> commonArgs_ {};
    std::vector<GateRef> actualArgs_ {};
    TSLoader *tsLoader_ {nullptr};
    const JSPandaFile *file_ {nullptr};
    const panda_file::File *pf_ {nullptr};
    const JSMethod *method_ {nullptr};
    const std::vector<uint8_t *> pcArray_;
    JSHandle<JSTaggedValue> constantPool_;
    bool enableLog_ {false};
    std::map<uint8_t *, int32_t> pcToBCOffset_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
