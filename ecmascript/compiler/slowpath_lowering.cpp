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
    while (gateAcc.Uses(oldGate).begin() != gateAcc.Uses(oldGate).end()) {
        auto useIt = gateAcc.Uses(oldGate).begin();
        if (gateAcc.GetOpCode(*useIt) == OpCode::IF_SUCCESS) {
            gateAcc.SetOpCode(*useIt, OpCode::IF_FALSE);
            gateAcc.ReplaceIn(useIt, ifBranch);
        } else {
            if (gateAcc.GetOpCode(*useIt) == OpCode::IF_EXCEPTION) {
                gateAcc.SetOpCode(*useIt, OpCode::IF_TRUE);
                gateAcc.ReplaceIn(useIt, ifBranch);
            } else {
                gateAcc.ReplaceIn(useIt, newGate);
            }
        }
    }

    // delete old gate
    circuit_->DeleteGate(oldGate);
}

void SlowPathLowering::Lower(GateRef gate, EcmaOpcode bytecode)
{
    GateRef glue = builder_->GetCommonArgByIndex(CommonArgIdx::GLUE);

    switch (bytecode) {
        case LDA_STR_ID32:LowerLoadStr(gate, glue);
            break;
        case LDLEXENVDYN_PREF:LowerLexicalEnv(gate, glue);
            break;
        case ADD2DYN_PREF_V8:LowerAdd2Dyn(gate, glue);
            break;
        case CREATEITERRESULTOBJ_PREF_V8_V8:LowerCreateIterResultObj(gate, glue);
            break;
        case SUSPENDGENERATOR_PREF_V8_V8:LowerSuspendGenerator(gate, glue);
            break;
        case ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:LowerAsyncFunctionAwaitUncaught(gate, glue);
            break;
        case ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:LowerAsyncFunctionResolve(gate, glue);
            break;
        case ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:LowerAsyncFunctionReject(gate, glue);
            break;
        case TRYLDGLOBALBYNAME_PREF_ID32:LowerTryLdGlobalByName(gate, glue);
            break;
        case STGLOBALVAR_PREF_ID32:LowerStGlobalVar(gate, glue);
            break;
        default:break;
    }
}

void SlowPathLowering::LowerAdd2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(Add2Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(CreateIterResultObj));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(SuspendGenerator));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(AsyncFunctionAwaitUncaught));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef tureConst = cirBuilder.NewBooleanConstant(true);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1),
                                                     tureConst});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef falseConst = cirBuilder.NewBooleanConstant(false);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1),
                                                     falseConst});
    LowerHIR(cirBuilder, gate, newGate);
}

GateRef SlowPathLowering::GetValueFromConstPool(CircuitBuilder &builder, GateRef gate, GateRef glue)
{
    GateRef id = builder.NewInteger64Constant(FAST_STUB_ID(LoadValueFromConstantPool));
    auto bytecodeInfo = builder_->GetBytecodeInfo(builder_->GetJSBytecode(gate));
    GateRef stringId = builder.NewInteger64Constant(bytecodeInfo.imm);
    return builder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                      {builder_->GetCommonArgByIndex(CommonArgIdx::FUNC), stringId});
}

void SlowPathLowering::LowerLoadStr(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef newGate = GetValueFromConstPool(cirBuilder, gate, glue);
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerLexicalEnv(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(GetLexicalEnv));
    GateRef newGate =
        cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef prop = GetValueFromConstPool(cirBuilder, gate, glue);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(TryLdGlobalByName));
    GateRef newGate =
        cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {prop});
    LowerHIR(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef prop = GetValueFromConstPool(cirBuilder, gate, glue);
    GateRef id = cirBuilder.NewInteger64Constant(FAST_STUB_ID(StGlobalVar));
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {prop, gateAcc.GetValueIn(gate, 0)});
    LowerHIR(cirBuilder, gate, newGate);
}
}  // namespace panda::ecmascript
