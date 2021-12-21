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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_H
#define ECMASCRIPT_COMPILER_CIRCUIT_H

#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include "ecmascript/compiler/gate.h"
#include "ecmascript/frames.h"
#include "libpandabase/macros.h"
#include "securec.h"

namespace kungfu {
const size_t INITIAL_SPACE = 1U << 0U;  // this should be tuned
const size_t MAX_SPACE = 1U << 24U;     // this should be tuned
const size_t SCALE_RATE = 1U << 1U;     // this should be tuned
class ControlFlowBuilder;
class RegAllocLinearScan;
class Circuit {  // note: calling NewGate could make all saved Gate* invalid
public:
    Circuit();
    ~Circuit();
    Circuit(Circuit const &circuit) = default;
    Circuit &operator=(Circuit const &circuit) = default;
    Circuit(Circuit &&circuit) = default;
    Circuit &operator=(Circuit &&circuit) = default;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    GateRef NewGate(OpCode op, BitField bitfield, size_t numIns, const GateRef inList[], TypeCode type,
        MarkCode mark = MarkCode::EMPTY);
    GateRef NewGate(OpCode op, BitField bitfield, const std::vector<GateRef> &inList, TypeCode type,
        MarkCode mark = MarkCode::EMPTY);
    void PrintAllGates() const;
    [[nodiscard]] std::vector<GateRef> GetAllGates() const;
    [[nodiscard]] static GateRef GetCircuitRoot(OpCode opcode);
    void AdvanceTime() const;
    void ResetAllGateTimeStamps() const;
    [[nodiscard]] static GateRef NullGate();
    [[nodiscard]] bool IsLoopHead(GateRef gate) const;
    [[nodiscard]] bool IsSelector(GateRef gate) const;
    [[nodiscard]] bool IsControlCase(GateRef gate) const;
    [[nodiscard]] GateRef GetIn(GateRef gate, size_t idx) const;
    [[nodiscard]] bool IsInGateNull(GateRef gate, size_t idx) const;
    [[nodiscard]] bool IsFirstOutNull(GateRef gate) const;
    [[nodiscard]] std::vector<GateRef> GetInVector(GateRef gate) const;
    [[nodiscard]] std::vector<GateRef> GetOutVector(GateRef gate) const;
    void NewIn(GateRef gate, size_t idx, GateRef in);
    void ModifyIn(GateRef gate, size_t idx, GateRef in);
    void DeleteIn(GateRef gate, size_t idx);
    void DeleteGate(GateRef gate);
    [[nodiscard]] GateId GetId(GateRef gate) const;
    [[nodiscard]] BitField GetBitField(GateRef gate) const;
    void Print(GateRef gate) const;
    void SetOpCode(GateRef gate, OpCode opcode);
    void SetTypeCode(GateRef gate, TypeCode type);
    [[nodiscard]] OpCode GetOpCode(GateRef gate) const;
    [[nodiscard]] TimeStamp GetTime() const;
    [[nodiscard]] MarkCode GetMark(GateRef gate) const;
    [[nodiscard]] TypeCode GetTypeCode(GateRef gate) const;
    void SetMark(GateRef gate, MarkCode mark) const;
    [[nodiscard]] bool Verify(GateRef gate) const;
    [[nodiscard]] Gate *LoadGatePtr(GateRef shift);
    [[nodiscard]] const Gate *LoadGatePtrConst(GateRef shift) const;
    [[nodiscard]] GateRef SaveGatePtr(const Gate *gate) const;
    [[nodiscard]] std::vector<uint8_t> GetDataSection() const;
    void SetDataSection(const std::vector<uint8_t> &data);
    [[nodiscard]] size_t GetCircuitDataSize() const;
    [[nodiscard]] const void *GetSpaceDataStartPtrConst() const;
    [[nodiscard]] const void *GetSpaceDataEndPtrConst() const;
    [[nodiscard]] const uint8_t *GetDataPtrConst(size_t offset) const;
    [[nodiscard]] uint8_t *GetDataPtr(size_t offset);
    [[nodiscard]] size_t GetSpaceDataSize() const;
    void SetSpaceDataSize(size_t sz);
    panda::ecmascript::FrameType GetFrameType() const;
    void SetFrameType(panda::ecmascript::FrameType type);

private:
    uint8_t *AllocateSpace(size_t gateSize);
    Gate *AllocateGateSpace(size_t numIns);

private:
    std::vector<uint8_t> space;
    size_t circuitSize;
    size_t gateCounter;
    TimeStamp time;
    std::vector<uint8_t> dataSection;
    panda::ecmascript::FrameType frameType {panda::ecmascript::FrameType::OPTIMIZED_FRAME};
};
}  // namespace kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_H
