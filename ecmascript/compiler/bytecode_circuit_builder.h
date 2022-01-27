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

#include "circuit.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_method.h"

namespace panda::ecmascript::kungfu {
using VRegIDType = uint16_t;

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
        : pc(startOrEndPc), splitKind(kind), succs(successors) {};

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
    std::vector<VRegIDType> vregIn {}; // read register
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

class BytecodeCircuitBuilder {
public:
    explicit BytecodeCircuitBuilder() = default;
    ~BytecodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(BytecodeCircuitBuilder);
    NO_MOVE_SEMANTIC(BytecodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit(const std::vector<uint8_t *> &pcArray, const panda_file::File &pf,
                                      const JSMethod *method);

    [[nodiscard]] kungfu::Circuit& GetCircuit()
    {
        return circuit_;
    }

    [[nodiscard]] const kungfu::Circuit& GetCircuit() const
    {
        return circuit_;
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

private:
    void PUBLIC_API CollectBytecodeBlockInfo(uint8_t* pc, std::vector<CfgInfo> &bytecodeBlockInfos);

    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> CollectTryCatchBlockInfo(
        const panda_file::File &file, const JSMethod *method, std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
        std::vector<CfgInfo> &bytecodeBlockInfos);

    void CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
                                   std::vector<CfgInfo> &bytecodeBlockInfos);

    void BuildBasicBlocks(const JSMethod *method,
                          std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
                          std::vector<CfgInfo> &bytecodeBlockInfo,
                          std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc);
    void ComputeDominatorTree(BytecodeGraph &byteCodeGraph);
    void BuildImmediateDominator(std::vector<int32_t> &immDom, BytecodeGraph &byteCodeGraph);
    void ComputeDomFrontiers(std::vector<int32_t> &immDom, BytecodeGraph &byteCodeGraph);
    BytecodeInfo GetBytecodeInfo(uint8_t *pc);
    void RemoveDeadRegions(const std::map<size_t, size_t> &dfsTimestamp, BytecodeGraph &byteCodeGraph);
    void InsertPhi(BytecodeGraph &byteCodeGraph);
    void UpdateCFG(BytecodeGraph &byteCodeGraph);
    void BuildCircuit(BytecodeGraph &byteCodeGraph);
    void PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos);
    void PrintGraph(std::vector<BytecodeRegion> &graph);
    void PrintBytecodeInfo(std::vector<BytecodeRegion> &graph);
    void PrintBBInfo(std::vector<BytecodeRegion> &graph);
    static bool IsJump(EcmaOpcode opcode);
    static bool IsCondJump(EcmaOpcode opcode);
    static bool IsMov(EcmaOpcode opcode);
    static bool IsReturn(EcmaOpcode opcode);
    static bool IsThrow(EcmaOpcode opcode);
    static bool IsGeneral(EcmaOpcode opcode);

    kungfu::Circuit circuit_;
    std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> jsgateToBytecode_;
    std::map<uint8_t *, kungfu::GateRef> byteCodeToJSGate_;
    std::map<int32_t, BytecodeRegion *> bbIdToBasicBlock_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H