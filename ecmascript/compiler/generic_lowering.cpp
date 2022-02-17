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

#include "generic_lowering.h"

namespace panda::ecmascript::kungfu {
void GenericLowering::CallRuntimeLowering()
{
    const auto &gateList = circuit_->GetAllGates();
    for (const auto &gate : gateList) {
        if (circuit_->LoadGatePtrConst(gate)->GetOpCode() == OpCode::JS_BYTECODE) {
            auto pc = builder_->GetJSBytecode(gate);
            EcmaOpcode op = static_cast<EcmaOpcode>(*pc);
            Lower(gate, op);
        }
    }
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    std::cout << "=========================================================" << std::endl;
    circuit_->PrintAllGates(*builder_);
#endif
}

void GenericLowering::Lower(GateRef gate, EcmaOpcode bytecode)
{
    switch (bytecode) {
        case ADD2DYN_PREF_V8:
            LowerAdd2Dyn(gate, builder_->GetCommonArgByIndex(CommonArgIdx::GLUE));
            break;
        default:
            break;
    }
}

void GenericLowering::LowerAdd2Dyn(GateRef gate, GateRef glue)
{
    StubDescriptor* getAdd2DynPtr = GET_STUBDESCRIPTOR(Add2Dyn);
    GateRef id = GetLoweringInt64Constant(FAST_STUB_ID(Add2Dyn));
    CircuitBuilder circuitBuilder(circuit_);
    GateAccessor acc(circuit_);
    GateRef newGate = circuitBuilder.NewCallGate(getAdd2DynPtr, glue, id, {glue,
                                                                           acc.GetValueIn(gate, 0),
                                                                           acc.GetValueIn(gate, 1)});
    LowerHIR(circuitBuilder, gate, newGate);
}

void GenericLowering::LowerHIR(CircuitBuilder &builder, GateRef oldGate, GateRef newGate)
{
    GateAccessor acc(circuit_);
    // set depend wire for dst gate
    GateRef dependInGate = acc.GetIn(oldGate, 1);
    acc.ModifyIn(newGate, 0, dependInGate);

    // exception value
    GateRef exceptionVal = builder.ExceptionConstant(GateType::TAGGED_NO_POINTER);

    // compare with trampolines result
    GateRef equal = builder.NewLogicGate(OpCode(OpCode::EQ), newGate, exceptionVal);

    // if branch
    GateRef ifBranch = builder.Branch(acc.GetIn(oldGate, 0), equal);

    while (acc.hasUseList(oldGate)) {
        GateRef outGate = acc.GetUseList(oldGate);
        size_t numIns = acc.GetNumIns(outGate);
        for (size_t i = 0; i < numIns; i++) {
            GateRef in = acc.GetIn(outGate, i);
            if (acc.GetId(in) == acc.GetId(oldGate)) {
                if (acc.GetOpCode(outGate) == OpCode::IF_SUCCESS) {
                    acc.SetOpCode(outGate, OpCode::IF_FALSE);
                    acc.ModifyIn(outGate, i, ifBranch);
                } else if (acc.GetOpCode(outGate) == OpCode::IF_EXCEPTION) {
                    acc.SetOpCode(outGate, OpCode::IF_TRUE);
                    acc.ModifyIn(outGate, i, ifBranch);
                } else {
                    acc.ModifyIn(outGate, i, newGate);
                }
            }
        }
    }

    // delete src gate
    circuit_->DeleteGate(oldGate);
}
}  // namespace panda::ecmascript
