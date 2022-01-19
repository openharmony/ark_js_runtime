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

struct ByteCodeRegion {
    int32_t id {-1};
    uint8_t *start {nullptr};
    uint8_t *end {nullptr};
    std::vector<ByteCodeRegion *> preds {}; // List of predessesor blocks
    std::vector<ByteCodeRegion *> succs {}; // List of successors blocks
    std::vector<ByteCodeRegion *> trys {}; // List of trys blocks
    std::vector<ByteCodeRegion *> catchs {}; // List of catches blocks
    std::vector<ByteCodeRegion *> immDomBlocks {}; // List of dominated blocks
    ByteCodeRegion *iDominator {nullptr}; // Block that dominates the current block
    std::vector<ByteCodeRegion *> domFrontiers {}; // List of dominace frontiers
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

    bool operator <(const ByteCodeRegion &target) const
    {
        return id < target.id;
    }
};

struct ByteCodeInfo {
    std::vector<VRegIDType> vregIn {}; // read register
    std::vector<VRegIDType> vregOut {}; // write register
    bool accIn{false}; // read acc
    bool accOut{false}; // write acc
    uint8_t opcode{0};
    uint16_t offset{0};
};

struct ByteCodeGraph {
    std::vector<ByteCodeRegion> graph{};
    const JSMethod *method;
};

enum ByteCodeOffset {
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

class ByteCodeCircuitBuilder {
public:
    explicit ByteCodeCircuitBuilder() = default;
    ~ByteCodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(ByteCodeCircuitBuilder);
    NO_MOVE_SEMANTIC(ByteCodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit(const std::vector<uint8_t *> &pcArray, const panda_file::File &pf, const JSMethod *method);

    [[nodiscard]] kungfu::Circuit GetCircuit() const
    {
        return circuit_;
    }

    [[nodiscard]] std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> GetGateToByteCode() const
    {
        return jsgateToByteCode_;
    }

    [[nodiscard]] std::map<uint8_t *, kungfu::GateRef> GetByteCodeToGate() const
    {
        return byteCodeToJSGate_;
    }

    [[nodiscard]] std::string GetByteCodeStr(kungfu::GateRef gate) const
    {
        auto pc = jsgateToByteCode_.at(gate).second;
        return GetEcmaOpcodeStr(static_cast<EcmaOpcode>(*pc));
    }

private:
    void CollectBytecodeBlockInfo(uint8_t* pc, std::vector<CfgInfo> &bytecodeBlockInfos);

    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> CollectTryCatchBlockInfo(
        const panda_file::File &file, const JSMethod *method, std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
        std::vector<CfgInfo> &bytecodeBlockInfos);

    void CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
                                   std::vector<CfgInfo> &bytecodeBlockInfos);

    void BuildBasicBlocks(const JSMethod *method,
                          std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
                          std::vector<CfgInfo> &bytecodeBlockInfo,
                          std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc);
    void ComputeDominatorTree(ByteCodeGraph &byteCodeGraph);
    void BuildImmediateDominator(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph);
    void ComputeDomFrontiers(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph);
    ByteCodeInfo GetByteCodeInfo(uint8_t *pc);
    void RemoveDeadRegions(const std::map<size_t, size_t> &dfsTimestamp, ByteCodeGraph &byteCodeGraph);
    void InsertPhi(ByteCodeGraph &byteCodeGraph);
    void UpdateCFG(ByteCodeGraph &byteCodeGraph);
    void BuildCircuit(ByteCodeGraph &byteCodeGraph);
    void PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos);
    void PrintGraph(std::vector<ByteCodeRegion> &graph);
    void PrintByteCodeInfo(std::vector<ByteCodeRegion> &graph);
    void PrintBBInfo(std::vector<ByteCodeRegion> &graph);
    static bool IsJump(EcmaOpcode opcode);
    static bool IsCondJump(EcmaOpcode opcode);
    static bool IsMov(EcmaOpcode opcode);
    static bool IsReturn(EcmaOpcode opcode);
    static bool IsThrow(EcmaOpcode opcode);
    static bool IsGeneral(EcmaOpcode opcode);

    kungfu::Circuit circuit_;
    std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> jsgateToByteCode_;
    std::map<uint8_t *, kungfu::GateRef> byteCodeToJSGate_;
    std::map<int32_t, ByteCodeRegion *> bbIdToBasicBlock_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H