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
#include <vector>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_method.h"

namespace panda::ecmascript {
struct ByteCodeBasicBlock{
    size_t id;
    uint8_t *start;
    uint8_t *end;
    std::vector<ByteCodeBasicBlock *> preds{}; // List of predessesor blocks
    std::vector<ByteCodeBasicBlock *> succs{}; // List of successors blocks
    std::vector<ByteCodeBasicBlock *> trys{};
    std::vector<ByteCodeBasicBlock *> catchs{}; // List of catches blocks
    std::vector<ByteCodeBasicBlock *> immDomBlocks{}; // List of dominated blocks
    ByteCodeBasicBlock *iDominator{nullptr}; // Block that dominates the current block
    std::vector<ByteCodeBasicBlock *> domFrontiers{}; // List of dominace frontiers
    bool isDead{false};
    std::set<uint16_t> phi{}; // phi node
    bool phiAcc{false};
    size_t numStatePred{0};
    size_t cntStatePred{0};
    std::vector<std::tuple<size_t, uint8_t *, bool>> realPreds{};
    kungfu::GateRef stateStart = kungfu::Circuit::NullGate();
    kungfu::GateRef dependStart = kungfu::Circuit::NullGate();
    std::map<uint16_t, kungfu::GateRef> valueSelector{};
    kungfu::GateRef valueSelectorAcc = kungfu::Circuit::NullGate();

    bool operator <(const ByteCodeBasicBlock &target) const
    {
        return id < target.id;
    }
};

struct ByteCodeInfo {
    std::vector<uint16_t> vregIn{}; // read register
    std::vector<uint16_t> vregOut{}; // write register
    bool accIn{false}; // read acc
    bool accOut{false}; // write acc
    uint8_t opcode{-1};
    uint16_t offset{-1};
};

struct ByteCodeGraph{
    std::vector<ByteCodeBasicBlock> graph{};
    const JSMethod *method;
};

enum class SplitPoint : uint8_t {
    DEFAULT,
    START,
    END
};

enum ByteCodeOffset {
    ONE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN
};

class ByteCodeCircuitBuilder {
public:
    explicit ByteCodeCircuitBuilder() = default;
    ~ByteCodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(ByteCodeCircuitBuilder);
    NO_MOVE_SEMANTIC(ByteCodeCircuitBuilder);
    void BytecodeToCircuit(std::vector<uint8_t *> pcArr, const panda_file::File &pf, const JSMethod *method);

    kungfu::Circuit GetCircuit() {
        return circuit_;
    }

    std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> GetGateToByteCode() {
        return gateToByteCode_;
    }

    std::map<uint8_t *, kungfu::GateRef> GetByteCodeToGate() {
        return byteCodeToGate_;
    }

private:
    void CollectBytecodeBlockInfo(uint8_t* pc,
        std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo);

    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> CollectTryCatchBlockInfo(
        const panda_file::File &file, const JSMethod *method, std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
        std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo);

    void CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
        std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo);

    void BuildBasicBlocks(const JSMethod *method,
                          std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
                          std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo,
                          std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc);
    void ComputeDominatorTree(ByteCodeGraph &byteCodeGraph);
    void BuildImmediateDominator(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph);
    void ComputeDomFrontiers(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph);
    void GetByteCodeInfo(ByteCodeGraph &byteCodeGraph);
    ByteCodeInfo GetByteCodeInfo(uint8_t *pc);
    void DeadCodeRemove(const std::map<size_t, size_t> &dfsTimestamp, ByteCodeGraph &byteCodeGraph);
    void InsertPhi(ByteCodeGraph &byteCodeGraph);
    static bool IsJump(EcmaOpcode opcode);
    static bool IsCondJump(EcmaOpcode opcode);
    static bool IsMov(EcmaOpcode opcode);
    static bool IsReturn(EcmaOpcode opcode);
    static bool IsThrow(EcmaOpcode opcode);
    static bool IsGeneral(EcmaOpcode opcode);
    void BuildCircuit(ByteCodeGraph &byteCodeGraph);
    void Sort(std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &markOffset);
    void PrintCollectBlockInfo(
        std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo);
    void PrintGraph(std::vector<ByteCodeBasicBlock> &blocks);
    void PrintByteCodeInfo(std::vector<ByteCodeBasicBlock> &graph);
    void PrintBBInfo(std::vector<ByteCodeBasicBlock> &graph);

    kungfu::Circuit circuit_;
    std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> gateToByteCode_;
    std::map<uint8_t *, kungfu::GateRef> byteCodeToGate_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H