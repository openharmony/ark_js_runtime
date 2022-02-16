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

#include "gate_accessor.h"

namespace panda::ecmascript::kungfu {
UseIterator::UseIterator(Circuit *circuit, GateRef gate) : circuit_(circuit)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    Out *firstUse = gatePtr->GetFirstOut();
    current_ = circuit_->SaveGatePtr(firstUse->GetGate());
    currentIdx_ = firstUse->GetIndex();
}

bool GateAccessor::HasUse(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    if (gatePtr->IsFirstOutNull()) {
        return false;
    }
    return true;
}

size_t GateAccessor::GetNumIns(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetNumIns();
}

OpCode GateAccessor::GetOpCode(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOpCode();
}

GateId GateAccessor::GetId(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetId();
}

void GateAccessor::SetOpCode(GateRef gate, OpCode::Op opcode)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->SetOpCode(OpCode(opcode));
}

GateRef GateAccessor::GetValueIn(GateRef gate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetNumInsArray()[2]); // 2: number of value inputs
    size_t valueIndex = gatePtr->GetNumInsArray()[0] + gatePtr->GetNumInsArray()[1];
    return circuit_->GetIn(gate, valueIndex + idx);
}

size_t GateAccessor::GetNumValueIn(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetNumInsArray()[2]; // 2: number of value inputs
}

GateRef GateAccessor::GetIn(GateRef gate, size_t idx)
{
    return circuit_->GetIn(gate, idx);
}

GateRef GateAccessor::GetState(GateRef gate, size_t idx)
{
    ASSERT(idx < circuit_->LoadGatePtr(gate)->GetNumInsArray()[0]); // 0: number of state inputs
    return circuit_->GetIn(gate, idx);
}

GateRef GateAccessor::GetDep(GateRef gate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetNumInsArray()[1]); // 1: number of depend inputs
    size_t dependIndex = gatePtr->GetNumInsArray()[0];
    return circuit_->GetIn(gate, dependIndex + idx);
}

void GateAccessor::SetDep(GateRef gate, GateRef depGate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetNumInsArray()[1]); // 1: number of depend inputs
    size_t dependIndex = gatePtr->GetNumInsArray()[0];
    gatePtr->ModifyIn(dependIndex + idx, circuit_->LoadGatePtr(depGate));
}

size_t GateAccessor::GetFirstUseIdx(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetFirstOut()->GetIndex();
}

void GateAccessor::ReplaceIn(UseIterator &it, GateRef replaceGate)
{
    GateRef curGate = it.GetUse();
    size_t idx = it.GetIdx();
    Gate *curGatePtr = circuit_->LoadGatePtr(curGate);
    Gate *replaceGatePtr = circuit_->LoadGatePtr(replaceGate);
    curGatePtr->ModifyIn(idx, replaceGatePtr);
}
}