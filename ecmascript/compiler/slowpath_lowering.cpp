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
        auto op = circuit_->LoadGatePtrConst(gate)->GetOpCode();
        if (op == OpCode::JS_BYTECODE) {
            auto pc = builder_->GetJSBytecode(gate);
            Lower(gate, static_cast<EcmaOpcode>(*pc));
        } else if (op == OpCode::GET_EXCEPTION) {
            LowerExceptionHandler(gate);
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

void SlowPathLowering::LowerHirToThrowCall(CircuitBuilder &cirBuilder, GateRef hirGate, GateRef callGate)
{
    GateAccessor acc(circuit_);
    GateRef stateInGate = acc.GetState(hirGate);
    GateRef dependInGate = acc.GetDep(hirGate);
    acc.SetDep(callGate, dependInGate);

    GateRef ifBranch = cirBuilder.Branch(stateInGate, cirBuilder.NewBooleanConstant(true));
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

void SlowPathLowering::Lower(GateRef gate, EcmaOpcode op)
{
    GateRef glue = builder_->GetCommonArgByIndex(CommonArgIdx::GLUE);

    switch (op) {
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
        case INCDYN_PREF_V8:
            LowerIncDyn(gate, glue);
            break;
        case DECDYN_PREF_V8:
            LowerDecDyn(gate, glue);
            break;
        case GETPROPITERATOR_PREF:
            LowerGetPropIterator(gate, glue);
            break;
        case ITERNEXT_PREF_V8:
            LowerIterNext(gate, glue);
            break;
        case CLOSEITERATOR_PREF_V8:
            LowerCloseIterator(gate, glue);
            break;
        case ADD2DYN_PREF_V8:
            LowerAdd2Dyn(gate, glue);
            break;
        case SUB2DYN_PREF_V8:
            LowerSub2Dyn(gate, glue);
            break;
        case MUL2DYN_PREF_V8:
            LowerMul2Dyn(gate, glue);
            break;
        case DIV2DYN_PREF_V8:
            LowerDiv2Dyn(gate, glue);
            break;
        case MOD2DYN_PREF_V8:
            LowerMod2Dyn(gate, glue);
            break;
        case EQDYN_PREF_V8:
            LowerEqDyn(gate, glue);
            break;
        case NOTEQDYN_PREF_V8:
            LowerNotEqDyn(gate, glue);
            break;
        case LESSDYN_PREF_V8:
            LowerLessDyn(gate, glue);
            break;
        case LESSEQDYN_PREF_V8:
            LowerLessEqDyn(gate, glue);
            break;
        case GREATERDYN_PREF_V8:
            LowerGreaterDyn(gate, glue);
            break;
        case GREATEREQDYN_PREF_V8:
            LowerGreaterEqDyn(gate, glue);
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
        case NEWOBJSPREADDYN_PREF_V8_V8:
            LowerNewObjSpreadDyn(gate, glue);
            break;
        case THROWDYN_PREF:
            LowerThrowDyn(gate, glue);
            break;
        case THROWCONSTASSIGNMENT_PREF_V8:
            LowerThrowConstAssignment(gate, glue);
            break;
        case THROWTHROWNOTEXISTS_PREF:
            LowerThrowThrowNotExists(gate, glue);
            break;
        case THROWPATTERNNONCOERCIBLE_PREF:
            LowerThrowPatternNonCoercible(gate, glue);
            break;
        case THROWIFNOTOBJECT_PREF_V8:
            break;
        case THROWUNDEFINEDIFHOLE_PREF_V8_V8:
            break;
        case THROWIFSUPERNOTCORRECTCALL_PREF_IMM16:
            LowerThrowIfSuperNotCorrectCall(gate, glue);
            break;
        case THROWDELETESUPERPROPERTY_PREF:
            LowerThrowDeleteSuperProperty(gate, glue);
            break;
        case LDGLOBALTHIS_PREF:
            LowerLdGlobal(gate, glue);
            break;
        case LDSYMBOL_PREF:
            LowerLdSymbol(gate, glue);
            break;
        case LDGLOBAL_PREF:
            LowerLdGlobal(gate, glue);
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
    for (size_t i = 1; i < numArgs; i++) { // skip imm
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
    for (size_t i = 1; i < numArgs; i++) { // skip imm
        vec.emplace_back(gateAcc.GetValueIn(gate, i));
    }
    GateRef newGate = cirBuilder.CallRuntimeVariadic(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                     vec);
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerNewObjSpreadDyn(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(NewObjSpreadDyn));
    // 3: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 3);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue, acc.GetValueIn(gate, 0), 
                                                     acc.GetValueIn(gate, 1),
                                                     acc.GetValueIn(gate, 2)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerThrowDyn(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);

    GateRef exception = acc.GetValueIn(gate, 0);
    GateRef setException = cirBuilder.NewStoreGate(VariableType::INT64(), glue, exception,
                                                   Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)));
    LowerHirToThrowCall(cirBuilder, gate, setException);
}

void SlowPathLowering::LowerThrowConstAssignment(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(ThrowConstAssignment));
    // 1: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue, acc.GetValueIn(gate, 0)});
    LowerHirToThrowCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerThrowThrowNotExists(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(ThrowThrowNotExists));
    // 1: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue, acc.GetValueIn(gate, 0)});
    LowerHirToThrowCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerThrowPatternNonCoercible(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(ThrowPatternNonCoercible));
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue});
    LowerHirToThrowCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerThrowIfSuperNotCorrectCall(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(ThrowIfSuperNotCorrectCall));
    // 2: number of value inputs
    ASSERT(acc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue, acc.GetValueIn(gate, 0), acc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerThrowDeleteSuperProperty(GateRef gate, GateRef glue)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(ThrowDeleteSuperProperty));
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {glue});
    LowerHirToThrowCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerExceptionHandler(GateRef hirGate)
{
    GateAccessor acc(circuit_);
    CircuitBuilder cirBuilder(circuit_);

    GateRef glue = builder_->GetCommonArgByIndex(CommonArgIdx::GLUE);
    GateRef depend = acc.GetDep(hirGate);
    GateRef loadException = cirBuilder.NewLoadGate(VariableType::JS_ANY(), glue, depend);
    acc.SetDep(loadException, depend);
    GateRef holeCst = cirBuilder.HoleConstant(CircuitBuilder::VariableType2GateType(VariableType::JS_ANY()));
    GateRef clearException = cirBuilder.NewStoreGate(VariableType::INT64(), glue, holeCst, loadException);
    auto uses = acc.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc.GetDep(*it) == hirGate) {
            acc.ReplaceIn(it, clearException);
        } else {
            acc.ReplaceIn(it, loadException);
        }
    }
    circuit_->DeleteGate(hirGate);
}

void SlowPathLowering::LowerLdSymbol(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(GetSymbolFunction));
    GateRef newGate =
        cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerLdGlobal(GateRef gate, GateRef glue)
{
    CircuitBuilder cirBuilder(circuit_);
    GateRef offset = cirBuilder.NewInteger64Constant(JSThread::GlueData::GetGlobalObjOffset(false));
    GateRef val = cirBuilder.NewArithmeticGate(OpCode(OpCode::ADD), MachineType::I64, glue, offset);
    GateRef newGate = cirBuilder.NewLoadGate(VariableType::JS_ANY(), val,
                                             Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)));
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerSub2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(Sub2Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerMul2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(Mul2Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerDiv2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(Div2Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerMod2Dyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(Mod2Dyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerEqDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(EqDyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerNotEqDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(NotEqDyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerLessDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(LessDyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerLessEqDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(LessEqDyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerGreaterDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(GreaterDyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerGreaterEqDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RTSTUB_ID(GreaterEqDyn));
    // 2: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 2);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0), gateAcc.GetValueIn(gate, 1)});
}

void SlowPathLowering::LowerGetPropIterator(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RUNTIME_CALL_ID(GetPropIterator));
    // 1: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerIterNext(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RUNTIME_CALL_ID(IterNext));
    // 1: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerCloseIterator(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RUNTIME_CALL_ID(CloseIterator));
    // 1: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerIncDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RUNTIME_CALL_ID(IncDyn));
    // 1: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}

void SlowPathLowering::LowerDecDyn(GateRef gate, GateRef glue)
{
    GateAccessor gateAcc(circuit_);
    CircuitBuilder cirBuilder(circuit_);
    GateRef id = cirBuilder.NewInteger64Constant(RUNTIME_CALL_ID(DecDyn));
    // 1: number of value inputs
    ASSERT(gateAcc.GetNumValueIn(gate) == 1);
    GateRef newGate = cirBuilder.NewRuntimeCallGate(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                                    {gateAcc.GetValueIn(gate, 0)});
    LowerHirToCall(cirBuilder, gate, newGate);
}
}  // namespace panda::ecmascript
