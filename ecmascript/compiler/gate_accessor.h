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

#ifndef ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
#define ECMASCRIPT_COMPILER_GATE_ACCESSOR_H

#include "circuit.h"
#include "gate.h"

namespace panda::ecmascript::kungfu {
class UseIterator {
public:
    explicit UseIterator(Circuit *circuit, GateRef gate);
    ~UseIterator() = default;

    GateRef GetUse() const
    {
        return current_;
    }

    size_t GetIdx() const
    {
        return currentIdx_;
    }

    bool IsEnd() const
    {
        Gate *curGatePtr = circuit_->LoadGatePtr(current_);
        Out *use = curGatePtr->GetOut(currentIdx_);
        return use->IsNextOutNull();
    }

    UseIterator &operator++()
    {
        ASSERT(!IsEnd());
        Gate *curGatePtr = circuit_->LoadGatePtr(current_);
        Out *use = curGatePtr->GetOut(currentIdx_);
        Out *nextUse = use->GetNextOut();
        current_ = circuit_->SaveGatePtr(nextUse->GetGate());
        currentIdx_ = nextUse->GetIndex();
        return *this;
    }

private:
    Circuit *circuit_;
    GateRef current_;
    size_t currentIdx_;
};

class GateAccessor {
public:
    explicit GateAccessor(Circuit *circuit) : circuit_(circuit) {}
    ~GateAccessor() = default;

    [[nodiscard]] bool HasUse(GateRef gate);
    [[nodiscard]] size_t GetNumIns(GateRef gate);
    [[nodiscard]] OpCode GetOpCode(GateRef gate);
    void SetOpCode(GateRef gate, OpCode::Op opcode);
    [[nodiscard]] GateId GetId(GateRef gate);
    [[nodiscard]] GateRef GetValueIn(GateRef gate, size_t idx);
    [[nodiscard]] size_t GetNumValueIn(GateRef gate);
    [[nodiscard]] GateRef GetIn(GateRef gate, size_t idx);
    [[nodiscard]] GateRef GetState(GateRef gate, size_t idx = 0);
    [[nodiscard]] GateRef GetDep(GateRef gate, size_t idx = 0);
    void SetDep(GateRef gate, GateRef depGate, size_t idx = 0);
    [[nodiscard]] size_t GetFirstUseIdx(GateRef gate);
    void ReplaceIn(UseIterator &it, GateRef replaceGate);

private:
    Circuit *circuit_;
};
}
#endif  // ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
