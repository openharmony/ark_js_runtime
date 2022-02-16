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
#include "gate.h"

namespace panda::ecmascript::kungfu {
GateRef GateAccessor::GetUseList(GateRef gate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    Gate *outGatePtr = gatePtr->GetFirstOut()->GetGate();
    return circuit_->SaveGatePtr(outGatePtr);
}

bool GateAccessor::hasUseList(GateRef gate)
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

void GateAccessor::ModifyIn(GateRef gate, size_t idx, GateRef inGate)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    Gate *inGatePtr = circuit_->LoadGatePtr(inGate);
    gatePtr->ModifyIn(idx, inGatePtr);
}

GateRef GateAccessor::GetValueIn(GateRef gate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetNumInsArray()[2]); // 2: number of value inputs
    size_t valueIndex = gatePtr->GetNumInsArray()[0] + gatePtr->GetNumInsArray()[1];
    return circuit_->GetIn(gate, valueIndex + idx);
}

GateRef GateAccessor::GetIn(GateRef gate, size_t idx)
{
    return circuit_->GetIn(gate, idx);
}
}