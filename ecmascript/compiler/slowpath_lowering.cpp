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

void SlowPathLowering::LowerHirToCall(CircuitBuilder &cirBuilder, GateRef hirGate, GateRef callGate)
{
    GateAccessor acc(circuit_);
    GateRef stateInGate = acc.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc.GetDep(hirGate);
    acc.SetDep(callGate, dependInGate);

    // exception value
    GateRef exceptionVal = cirBuilder.ExceptionConstant(GateType::TAGGED_NO_POINTER);

    // compare with trampolines result
    GateRef equal = cirBuilder.NewLogicGate(OpCode(OpCode::EQ), callGate, exceptionVal);

    GateRef ifBranch = cirBuilder.Branch(stateInGate, equal);
    auto uses = acc.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc.SetOpCode(*it, OpCode::IF_FALSE);
            acc.ReplaceIn(it, ifBranch);
        } else {
            if (acc.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc.SetOpCode(*it, OpCode::IF_TRUE);
                acc.ReplaceIn(it, ifBranch);
            } else {
                acc.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    circuit_->DeleteGate(hirGate);
}

/*
 * lower condition call like this pattern:
 * if (condition) {
 *     res = Call(...);
 *     if (res == VALUE_EXCEPTION) {
 *         goto exception_handle;
 *     }
 *     Set(res);
 * }
 *
 */
void SlowPathLowering::LowerHirToConditionCall(CircuitBuilder &cirBuilder, GateRef hirGate,
                                               GateRef condGate, GateRef callGate)
{
    GateAccessor acc(circuit_);

    // state connect
    // condition branch
    GateRef stateInGate = acc.GetState(hirGate);
    GateRef condBranch = cirBuilder.Branch(stateInGate, condGate);
    GateRef condTrue = cirBuilder.NewIfTrue(condBranch);
    GateRef condFalse = cirBuilder.NewIfFalse(condBranch);
    // exception branch
    GateRef exceptionVal = cirBuilder.ExceptionConstant(GateType::TAGGED_NO_POINTER);
    GateRef equal = cirBuilder.NewLogicGate(OpCode(OpCode::EQ), callGate, exceptionVal);
    GateRef exceptionBranch = cirBuilder.Branch(condTrue, equal);
    GateRef exceptionFalse = cirBuilder.NewIfFalse(exceptionBranch);
    // merge condition false and exception false state
    std::vector<GateRef> mergeGates = {condFalse, exceptionFalse};
    GateRef stateMerge = cirBuilder.NewMerge(mergeGates.data(), mergeGates.size());

    // depend connect
    GateRef dependInGate = acc.GetDep(hirGate);
    GateRef condTrueRelay = cirBuilder.NewDependRelay(condTrue, dependInGate);
    acc.SetDep(callGate, condTrueRelay);
    GateRef condFalseRelay = cirBuilder.NewDependRelay(condFalse, dependInGate);
    // collect condition false and condition true depend
    std::vector<GateRef> dependGates = {callGate, condFalseRelay};
    GateRef dependPhi = cirBuilder.NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), stateMerge,
                                                   dependGates, dependGates.size());

    auto uses = acc.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            auto usesSuccess = acc.Uses(*it);
            for (auto itSuccess = usesSuccess.begin(); itSuccess != usesSuccess.end(); itSuccess++) {
                acc.ReplaceIn(itSuccess, stateMerge);
            }
            circuit_->DeleteGate(*it);
        } else if (acc.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
            acc.SetOpCode(*it, OpCode::IF_TRUE);
            acc.ReplaceIn(it, exceptionBranch);
        } else if (((acc.GetOpCode(*it) == OpCode::DEPEND_SELECTOR) || (acc.GetOpCode(*it) == OpCode::DEPEND_RELAY))
                   && (acc.GetOpCode(acc.GetIn(acc.GetIn(*it, 0), it.GetIndex() - 1)) != OpCode::IF_EXCEPTION)) {
            acc.ReplaceIn(it, dependPhi);
        } else {
            acc.ReplaceIn(it, callGate);
        }
    }
    circuit_->DeleteGate(hirGate);
}

void SlowPathLowering::Lower(GateRef gate, EcmaOpcode bytecode)
{
    GateRef glue = builder_->GetCommonArgByIndex(CommonArgIdx::GLUE);

    switch (bytecode) {
        case LDA_STR_ID32:
            LowerLoadStr(gate, glue);
            break;
        case CALLARG0DYN_PREF_V8:
            LowerCallArg0Dyn(gate, glue);
            break;
        case CALLARG1DYN_PREF_V8_V8:
            LowerCallArg1Dyn(gate, glue);
            break;
        case CALLARGS2DYN_PREF_V8_V8_V8:
            LowerCallArgs2Dyn(gate, glue);
            break;
        case CALLARGS3DYN_PREF_V8_V8_V8_V8:
            LowerCallArgs3Dyn(gate, glue);
            break;
        case CALLITHISRANGEDYN_PREF_IMM16_V8:
            LowerCallIThisRangeDyn(gate, glue);
            break;
        case CALLSPREADDYN_PREF_V8_V8_V8:
            LowerCallSpreadDyn(gate, glue);
            break;
        case CALLIRANGEDYN_PREF_IMM16_V8:
            LowerCallIRangeDyn(gate, glue);
            break;
        case LDLEXENVDYN_PREF:
            LowerLexicalEnv(gate, glue);
            break;
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
        case TRYLDGLOBALBYNAME_PREF_ID32:
            LowerTryLdGlobalByName(gate, glue);
            break;
        case STGLOBALVAR_PREF_ID32:
            LowerStGlobalVar(gate, glue);
            break;
        case GETITERATOR_PREF:
            LowerGetIterator(gate, glue);
            break;
        default:
            break;
    }
}

void SlowPathLowering::LowerAdd2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(Add2Dyn));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CreateIterResultObj));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(SuspendGenerator));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(AsyncFunctionAwaitUncaught));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef tureConst = cirBuilder.NewBooleanConstant(true);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1),
                                                     tureConst});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef falseConst = cirBuilder.NewBooleanConstant(false);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1), falseConst});
    LowerHirToCall(cirBuilder, gate, newGate);
}

GateRef SlowPathLowering::GetValueFromConstantStringTable(CircuitBuilder &builder, GateRef glue,
                                                          GateRef gate, uint32_t inIndex)
{
    GateAccessor accessor(circuit_);
    GateRef id = builder.NewInteger64Constant(RTSTUB_ID(LoadValueFromConstantStringTable));
    auto idGate = accessor.GetValueIn(gate, inIndex);
    return builder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {idGate});
}

void SlowPathLowering::LowerLoadStr(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef newGate = GetValueFromConstantStringTable(cirBuilder, glue, gate, 0);
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerLexicalEnv(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(GetLexicalEnv));
    GateRef newGate =
        cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef prop = GetValueFromConstantStringTable(cirBuilder, glue, gate, 0);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(TryLdGlobalByName));
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate =
        cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {prop});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef prop = GetValueFromConstantStringTable(cirBuilder, glue, gate, 0);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(StGlobalVar));
    ASSERT(acc.GetNumValueIn(gate) == 2); // 2: number of value inputs
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {prop, acc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerGetIterator(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(GetIterator));
    // 1: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue, acc.GetValueIn(gate, 0)});
    GateRef isNotGeneratorObject = cirBuilder.NewBooleanConstant(true);
    LowerHirToConditionCall(cirBuilder, gate, isNotGeneratorObject, newGate);
}

void SlowPathLowering::LowerCallArg0Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallArg0Dyn));

    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCallArg1Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallArg1Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0),
                                                     gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCallArgs2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallArgs2Dyn));
    // 3: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 3);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0),
                                                     gateAcc.GetValueIn(gate, 1),
                                                     gateAcc.GetValueIn(gate, 2)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCallArgs3Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallArgs3Dyn));
    // 4: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 4);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0),
                                                     gateAcc.GetValueIn(gate, 1),
                                                     gateAcc.GetValueIn(gate, 2),
                                                     gateAcc.GetValueIn(gate, 3)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCallIThisRangeDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallIThisRangeDyn));
    std::vector<GateRef> vec;
    size_t numArgs = gateAcc.GetNumValueIn(gate);
    for (size_t i = 0; i < numArgs; i++) {
        vec.emplace_back(gateAcc.GetValueIn(gate, i));
    }
    GateRef newGate = cirBuilder.CallRuntimeVariadic(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                     vec);
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCallSpreadDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallSpreadDyn));
    // 3: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 3);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0),
                                                     gateAcc.GetValueIn(gate, 1),
                                                     gateAcc.GetValueIn(gate, 2)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCallIRangeDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(CallIRangeDyn));
    std::vector<GateRef> vec;
    size_t numArgs = gateAcc.GetNumValueIn(gate);
    for (size_t i = 0; i < numArgs; i++) {
        vec.emplace_back(gateAcc.GetValueIn(gate, i));
    }
    GateRef newGate = cirBuilder.CallRuntimeVariadic(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                     vec);
    LowerHirToCall(cirBuilder, gate, newGate);
}
}  // namespace panda::ecmascript
