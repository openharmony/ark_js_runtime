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
#include "ecmascript/ecma_vm.h"
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
    int32_t id {-1};
    uint8_t *start {nullptr};
    uint8_t *end {nullptr};
    std::vector<BytecodeRegion *> preds {}; // List of predessesor blocks
    std::vector<BytecodeRegion *> succs {}; // List of successors blocks
    std::vector<BytecodeRegion *> trys {}; // List of trys blocks
    std::vector<BytecodeRegion *> catchs {}; // List of catches blocks
    std::vector<BytecodeRegion *> immDomBlocks {}; // List of dominated blocks
    BytecodeRegion *iDominator {nullptr}; // Block that dominates the current block
    std::vector<BytecodeRegion *> domFrontiers {}; // List of dominace frontiers
    bool isDead {false};
    std::set<uint16_t> phi {}; // phi node
    bool phiAcc {false};
    int32_t numOfStatePreds {0};
    int32_t statePredIndex {0};
    std::vector<std::tuple<size_t, uint8_t *, bool>> expandedPreds {};
    kungfu::GateRef stateStart {kungfu::Circuit::NullGate()};
    kungfu::GateRef dependStart {kungfu::Circuit::NullGate()};
    std::map<uint16_t, kungfu::GateRef> vregToValSelectorGate {}; // corresponding ValueSelector gates of vregs
    kungfu::GateRef valueSelectorAccGate {kungfu::Circuit::NullGate()};

    bool operator <(const BytecodeRegion &target) const
    {
        return id < target.id;
    }
};

struct BytecodeInfo {
    // set of id, immediate and read register
    std::vector<std::variant<StringId, MethodId, Immediate, VirtualRegister>> inputs {};
    std::vector<VRegIDType> vregOut {}; // write register
    bool accIn {false}; // read acc
    bool accOut {false}; // write acc
    uint8_t opcode {0};
    uint16_t offset {0};
};

struct BytecodeGraph {
    std::vector<BytecodeRegion> graph {};
    const JSMethod *method;
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
    ACTUAL_ARGC,
    FUNC,
    NEW_TARGET,
    THIS,
    NUM_OF_ARGS,
};

class BytecodeCircuitBuilder {
public:
    explicit BytecodeCircuitBuilder(EcmaVM *vm, const BytecodeTranslationInfo &translationInfo, size_t index)
        : vm_(vm), file_(translationInfo.jsPandaFile), method_(translationInfo.methodPcInfos[index].method),
        pcArray_(translationInfo.methodPcInfos[index].pcArray), constantPool_(translationInfo.constantPool)
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

    [[nodiscard]] const std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>>& GetGateToBytecode() const
    {
        return jsgateToBytecode_;
    }

    [[nodiscard]] const std::map<uint8_t *, kungfu::GateRef>& GetBytecodeToGate() const
    {
        return byteCodeToJSGate_;
    }

    [[nodiscard]] std::string GetBytecodeStr(kungfu::GateRef gate) const
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return GetEcmaOpcodeStr(static_cast<EcmaOpcode>(*pc));
    }

    [[nodiscard]] uint8_t* GetJSBytecode(GateRef gate) const
    {
        return jsgateToBytecode_.at(gate).second;
    }

    [[nodiscard]] GateRef GetCommonArgByIndex(CommonArgIdx idx)
    {
        return commonArgs_[idx];
    }

    BytecodeInfo GetBytecodeInfo(uint8_t *pc);

private:
    void PUBLIC_API CollectBytecodeBlockInfo(uint8_t* pc, std::vector<CfgInfo> &bytecodeBlockInfos);

    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> CollectTryCatchBlockInfo(
        std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc, std::vector<CfgInfo> &bytecodeBlockInfos);

    void CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
                                   std::vector<CfgInfo> &bytecodeBlockInfos);

    void BuildBasicBlocks(std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
                          std::vector<CfgInfo> &bytecodeBlockInfo,
                          std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc);
    void ComputeDominatorTree(BytecodeGraph &byteCodeGraph);
    void BuildImmediateDominator(std::vector<int32_t> &immDom, BytecodeGraph &byteCodeGraph);
    void ComputeDomFrontiers(std::vector<int32_t> &immDom, BytecodeGraph &byteCodeGraph);
    void RemoveDeadRegions(const std::map<size_t, size_t> &dfsTimestamp, BytecodeGraph &byteCodeGraph);
    void InsertPhi(BytecodeGraph &byteCodeGraph);
    void UpdateCFG(BytecodeGraph &byteCodeGraph);
    void BuildCircuit(BytecodeGraph &byteCodeGraph);
    GateRef SetGateConstant(const BytecodeInfo &info);
    void PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos);
    size_t GetFunctionArgIndex(size_t currentVreg, size_t numVregs) const;
    void PrintGraph(std::vector<BytecodeRegion> &graph);
    void PrintBytecodeInfo(std::vector<BytecodeRegion> &graph);
    void PrintBBInfo(std::vector<BytecodeRegion> &graph);
    static bool IsJump(EcmaOpcode opcode);
    static bool IsCondJump(EcmaOpcode opcode);
    static bool IsMov(EcmaOpcode opcode);
    static bool IsReturn(EcmaOpcode opcode);
    static bool IsThrow(EcmaOpcode opcode);
    static bool IsDiscarded(EcmaOpcode opcode);
    static bool IsGeneral(EcmaOpcode opcode);
    static bool IsSetConstant(EcmaOpcode opcode);
    size_t GetActualNumArgs(size_t numArgs)
    {
        return numArgs + CommonArgIdx::NUM_OF_ARGS;
    }

    kungfu::Circuit circuit_;
    std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> jsgateToBytecode_;
    std::map<uint8_t *, kungfu::GateRef> byteCodeToJSGate_;
    std::map<int32_t, BytecodeRegion *> bbIdToBasicBlock_;
    std::array<GateRef, CommonArgIdx::NUM_OF_ARGS> commonArgs_ {};
    EcmaVM* vm_;
    const JSPandaFile* file_ {nullptr};
    const JSMethod* method_ {nullptr};
    const std::vector<uint8_t *> pcArray_;
    JSHandle<JSTaggedValue> constantPool_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H