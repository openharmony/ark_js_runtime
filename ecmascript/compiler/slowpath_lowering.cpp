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

#include "slowpath_lowering.h"

namespace panda::ecmascript::kungfu {
void SlowPathLowering::CallRuntimeLowering()
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

void SlowPathLowering::LowerHIR(CircuitBuilder &cirBuilder, GateRef oldGate, GateRef newGate)
{
    GateAccessor gateAcc(circuit_);
    GateRef stateInGate = gateAcc.GetState(oldGate);
    // copy depend-wire of oldGate to newGate
    GateRef dependInGate = gateAcc.GetDep(oldGate);
    gateAcc.SetDep(newGate, dependInGate);

    // exception value
    GateRef exceptionVal = cirBuilder.ExceptionConstant(GateType::TAGGED_NO_POINTER);

    // compare with trampolines result
    GateRef equal = cirBuilder.NewLogicGate(OpCode(OpCode::EQ), newGate, exceptionVal);

    // if branch
    GateRef ifBranch = cirBuilder.Branch(stateInGate, equal);

    while (gateAcc.HasUse(oldGate)) {
        UseIterator it(circuit_, oldGate);
        GateRef use = it.GetUse();
        if (gateAcc.GetOpCode(use) == OpCode::IF_SUCCESS) {
            gateAcc.SetOpCode(use, OpCode::IF_FALSE);
            gateAcc.ReplaceIn(it, ifBranch);
        } else if (gateAcc.GetOpCode(use) == OpCode::IF_EXCEPTION) {
            gateAcc.SetOpCode(use, OpCode::IF_TRUE);
            gateAcc.ReplaceIn(it, ifBranch);
        } else {
            gateAcc.ReplaceIn(it, newGate);
        }
    }

    // delete old gate
    circuit_->DeleteGate(oldGate);
}

void SlowPathLowering::Lower(GateRef gate, EcmaOpcode bytecode)
{
    GateRef glue = builder_->GetCommonArgByIndex(CommonArgIdx::GLUE);

    switch (bytecode) {
        case ADD2DYN_PREF_V8:
            LowerAdd2Dyn(gate, glue);
            break;
        case CREATEITERRESULTOBJ_PREF_V8_V8:
            LowerCreateIterResultObj(gate, glue);
            break;
        case SUSPENDGENERATOR_PREF_V8_V8:
            LowerSuspendGenerator(gate, glue);
            break;
        case ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
            LowerAsyncFunctionAwaitUncaught(gate, glue);
            break;
        case ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
            LowerAsyncFunctionResolve(gate, glue);
            break;
        case ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
            LowerAsyncFunctionReject(gate, glue);
            break;
        default:
            break;
    }
}

void SlowPathLowering::LowerAdd2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    StubDescriptor* descriptorPtr = GET_STUBDESCRIPTOR(Add2Dyn);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(Add2Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewCallGate(descriptorPtr, glue, id,
                                             {glue,
                                              gateAcc.GetValueIn(gate, 0),
                                              gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    StubDescriptor* descriptorPtr = GET_STUBDESCRIPTOR(CreateIterResultObj);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(CreateIterResultObj));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewCallGate(descriptorPtr, glue, id,
                                             {glue,
                                              gateAcc.GetValueIn(gate, 0),
                                              gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    StubDescriptor* descriptorPtr = GET_STUBDESCRIPTOR(SuspendGenerator);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(SuspendGenerator));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewCallGate(descriptorPtr, glue, id,
                                             {glue,
                                              gateAcc.GetValueIn(gate, 0),
                                              gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    StubDescriptor* descriptorPtr = GET_STUBDESCRIPTOR(AsyncFunctionAwaitUncaught);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(AsyncFunctionAwaitUncaught));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewCallGate(descriptorPtr, glue, id,
                                             {glue,
                                              gateAcc.GetValueIn(gate, 0),
                                              gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    StubDescriptor* descriptorPtr = GET_STUBDESCRIPTOR(AsyncFunctionResolveOrReject);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef tureConst = cirBuilder.NewBooleanConstant(true);
    GateRef newGate = cirBuilder.NewCallGate(descriptorPtr, glue, id,
                                             {glue,
                                              gateAcc.GetValueIn(gate, 0),
                                              gateAcc.GetValueIn(gate, 1),
                                              tureConst});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    StubDescriptor* descriptorPtr = GET_STUBDESCRIPTOR(AsyncFunctionResolveOrReject);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef falseConst = cirBuilder.NewBooleanConstant(false);
    GateRef newGate = cirBuilder.NewCallGate(descriptorPtr, glue, id,
                                             {glue,
                                              gateAcc.GetValueIn(gate, 0),
                                              gateAcc.GetValueIn(gate, 1),
                                              falseConst});
    LowerHIR(cirBuilder, gate, newGate);
}
}  // namespace panda::ecmascript
