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
#define CREATE_DOUBLE_EXIT(SuccessLabel, FailLabel)               \
    std::vector<GateRef> successControl;                          \
    std::vector<GateRef> failControl;                             \
    builder_.Bind(&SuccessLabel);                                 \
    {                                                             \
        successControl.emplace_back(builder_.GetState());         \
        successControl.emplace_back(builder_.GetDepend());        \
    }                                                             \
    builder_.Bind(&FailLabel);                                    \
    {                                                             \
        failControl.emplace_back(builder_.GetState());            \
        failControl.emplace_back(builder_.GetDepend());           \
    }

void SlowPathLowering::CallRuntimeLowering()
{
    const auto &gateList = circuit_->GetAllGates();
    for (const auto &gate : gateList) {
        auto op = circuit_->LoadGatePtrConst(gate)->GetOpCode();
        if (op == OpCode::JS_BYTECODE) {
            auto pc = bcBuilder_->GetJSBytecode(gate);
            Lower(gate, static_cast<EcmaOpcode>(*pc));
        } else if (op == OpCode::GET_EXCEPTION) {
            LowerExceptionHandler(gate);
        }
    }
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    std::cout << "=========================================================" << std::endl;
    circuit_->PrintAllGates(*bcBuilder_);
#endif
}

/*
 * lower to slowpath call like this pattern:
 * res = Call(...);
 * if (res == VALUE_EXCEPTION) {
 *     goto exception_handle;
 * }
 * Set(res);
 *
 */
void SlowPathLowering::LowerHirToCall(GateRef hirGate, GateRef callGate)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    // exception value
    GateRef exceptionVal = builder_.ExceptionConstant(GateType::TAGGED_NO_POINTER);

    // compare with trampolines result
    GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);

    GateRef ifBranch = builder_.Branch(stateInGate, equal);
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                acc_.ReplaceIn(it, ifBranch);
            } else {
                acc_.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    circuit_->DeleteGate(hirGate);
}

/*
 * lower to fast call like this pattern:
 * res = Call(...);
 * Set(res);
 *
 */
void SlowPathLowering::LowerHirToFastCall(GateRef hirGate, GateRef callGate)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                acc_.ReplaceIn(it, ifBranch);
            } else {
                acc_.ReplaceIn(it, callGate);
            }
        }
    }

    circuit_->DeleteGate(hirGate);
}


/*
 * lower to throw call like this pattern:
 * if (condition) {
 *     Call(...);
 *     goto exception_handle;
 * }
 *
 */
void SlowPathLowering::LowerHirToThrowCall(GateRef hirGate, GateRef condGate, GateRef callGate)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch = builder_.Branch(stateInGate, condGate);
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                acc_.ReplaceIn(it, ifBranch);
            } else {
                acc_.ReplaceIn(it, callGate);
            }
        }
    }

    circuit_->DeleteGate(hirGate);
}

/*
 * lower to condition call like this pattern:
 * if (condition) {
 *     res = Call(...);
 *     if (res == VALUE_EXCEPTION) {
 *         goto exception_handle;
 *     }
 *     Set(res);
 * }
 *
 */
void SlowPathLowering::LowerHirToConditionCall(GateRef hirGate, GateRef condGate, GateRef callGate)
{
    // state connect
    // condition branch
    GateRef stateInGate = acc_.GetState(hirGate);
    GateRef condBranch = builder_.Branch(stateInGate, condGate);
    GateRef condTrue = builder_.IfTrue(condBranch);
    GateRef condFalse = builder_.IfFalse(condBranch);
    // exception branch
    GateRef exceptionVal = builder_.ExceptionConstant(GateType::TAGGED_NO_POINTER);
    GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);
    GateRef exceptionBranch = builder_.Branch(condTrue, equal);
    GateRef exceptionFalse = builder_.IfFalse(exceptionBranch);
    // merge condition false and exception false state
    std::vector<GateRef> mergeGates = {condFalse, exceptionFalse};
    GateRef stateMerge = builder_.Merge(mergeGates.data(), mergeGates.size());

    // depend connect
    GateRef dependInGate = acc_.GetDep(hirGate);
    GateRef condTrueRelay = builder_.DependRelay(condTrue, dependInGate);
    acc_.SetDep(callGate, condTrueRelay);
    GateRef condFalseRelay = builder_.DependRelay(condFalse, dependInGate);
    // collect condition false and condition true depend
    std::vector<GateRef> dependGates = {callGate, condFalseRelay};
    GateRef dependPhi = builder_.Selector(OpCode(OpCode::DEPEND_SELECTOR),
        stateMerge, dependGates, dependGates.size());

    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            auto usesSuccess = acc_.Uses(*it);
            for (auto itSuccess = usesSuccess.begin(); itSuccess != usesSuccess.end(); itSuccess++) {
                acc_.ReplaceIn(itSuccess, stateMerge);
            }
            circuit_->DeleteGate(*it);
        } else if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
            acc_.SetOpCode(*it, OpCode::IF_TRUE);
            acc_.ReplaceIn(it, exceptionBranch);
        } else if (((acc_.GetOpCode(*it) == OpCode::DEPEND_SELECTOR) ||
                   (acc_.GetOpCode(*it) == OpCode::DEPEND_RELAY)) &&
                   (acc_.GetOpCode(acc_.GetIn(acc_.GetIn(*it, 0),
                   it.GetIndex() - 1)) != OpCode::IF_EXCEPTION)) {
            acc_.ReplaceIn(it, dependPhi);
        } else {
            acc_.ReplaceIn(it, callGate);
        }
    }
    circuit_->DeleteGate(hirGate);
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetObjectFromConstPool(GateRef index)
{
    GateRef jsFunc = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::FUNC);
    GateRef constantPool = builder_.Load(VariableType::JS_ANY(), jsFunc,
        builder_.IntPtr(JSFunction::CONSTANT_POOL_OFFSET));
    GateRef offset = builder_.IntPtrMul(builder_.ChangeInt32ToIntPtr(index),
        builder_.IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = builder_.IntPtrAdd(offset, builder_.IntPtr(TaggedArray::DATA_OFFSET));
    return builder_.Load(VariableType::JS_ANY(), constantPool, dataOffset);
}

GateRef SlowPathLowering::GetValueFromConstStringTable(GateRef glue, GateRef gate, uint32_t inIndex)
{
    GateRef id = builder_.Int64(RTSTUB_ID(LoadValueFromConstantStringTable));
    auto idGate = acc_.GetValueIn(gate, inIndex);
    return builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {idGate});
}

void SlowPathLowering::Lower(GateRef gate, EcmaOpcode op)
{
    GateRef glue = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::GLUE);

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
            LowerThrowIfNotObject(gate, glue);
            break;
        case THROWUNDEFINEDIFHOLE_PREF_V8_V8:
            LowerThrowUndefinedIfHole(gate, glue);
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
        case TONUMBER_PREF_V8:
            LowerToNumber(gate, glue);
            break;
        case NEGDYN_PREF_V8:
            LowerNegDyn(gate, glue);
            break;
        case NOTDYN_PREF_V8:
            LowerNotDyn(gate, glue);
            break;
        case SHL2DYN_PREF_V8:
            LowerShl2Dyn(gate, glue);
            break;
        case SHR2DYN_PREF_V8:
            LowerShr2Dyn(gate, glue);
            break;
        case ASHR2DYN_PREF_V8:
            LowerAshr2Dyn(gate, glue);
            break;
        case AND2DYN_PREF_V8:
            LowerAnd2Dyn(gate, glue);
            break;
        case OR2DYN_PREF_V8:
            LowerOr2Dyn(gate, glue);
            break;
        case XOR2DYN_PREF_V8:
            LowerXor2Dyn(gate, glue);
            break;
        case DELOBJPROP_PREF_V8_V8:
            LowerDelObjProp(gate, glue);
            break;
        case EXPDYN_PREF_V8:
            LowerExpDyn(gate, glue);
            break;
        case ISINDYN_PREF_V8:
            LowerIsInDyn(gate, glue);
            break;
        case INSTANCEOFDYN_PREF_V8:
            LowerInstanceofDyn(gate, glue);
            break;
        case STRICTNOTEQDYN_PREF_V8:
            LowerFastStrictNotEqual(gate, glue);
            break;
        case STRICTEQDYN_PREF_V8:
            LowerFastStrictEqual(gate, glue);
            break;
        case CREATEEMPTYARRAY_PREF:
            LowerCreateEmptyArray(gate, glue);
            break;
        case CREATEEMPTYOBJECT_PREF:
            LowerCreateEmptyObject(gate, glue);
            break;
        case CREATEOBJECTWITHBUFFER_PREF_IMM16:
            LowerCreateObjectWithBuffer(gate, glue);
            break;
        case CREATEARRAYWITHBUFFER_PREF_IMM16:
            LowerCreateArrayWithBuffer(gate, glue);
            break;
        case STMODULEVAR_PREF_ID32:
            LowerStModuleVar(gate, glue);
            break;
        case GETTEMPLATEOBJECT_PREF_V8:
            LowerGetTemplateObject(gate, glue);
            break;
        case SETOBJECTWITHPROTO_PREF_V8_V8:
            LowerSetObjectWithProto(gate, glue);
            break;
        case LDBIGINT_PREF_ID32:
            LowerLdBigInt(gate, glue);
            break;
        case LDMODULEVAR_PREF_ID32_IMM8:
            LowerLdModuleVar(gate, glue);
            break;
        case GETMODULENAMESPACE_PREF_ID32:
            LowerGetModuleNamespace(gate, glue);
            break;
        default:
            break;
    }

    // clear temporary used label manager
    builder_.SetLabelManager(nullptr);
}

void SlowPathLowering::LowerAdd2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Add2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CreateIterResultObj));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(SuspendGenerator));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(AsyncFunctionAwaitUncaught));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef tureConst = builder_.Boolean(true);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), tureConst});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(AsyncFunctionResolveOrReject));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef falseConst = builder_.Boolean(false);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), falseConst});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLoadStr(GateRef gate, GateRef glue)
{
    GateRef newGate = GetValueFromConstStringTable(glue, gate, 0);
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLexicalEnv(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetLexicalEnv));
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    GateRef id = builder_.Int64(RTSTUB_ID(TryLdGlobalByName));
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {prop});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    GateRef id = builder_.Int64(RTSTUB_ID(StGlobalVar));
    ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: number of value inputs
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {prop, acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGetIterator(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetIterator));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {glue, acc_.GetValueIn(gate, 0)});
    GateRef isGeneratorObject = builder_.TaggedIsGeneratorObject(acc_.GetValueIn(gate, 0));
    GateRef isNotGeneratorObject = builder_.BoolNot(isGeneratorObject);
    LowerHirToConditionCall(gate, isNotGeneratorObject, newGate);
}

void SlowPathLowering::LowerCallArg0Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallArg0Dyn));

    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallArg1Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallArg1Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallArgs2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallArgs2Dyn));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallArgs3Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallArgs3Dyn));
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1),
        acc_.GetValueIn(gate, 2), acc_.GetValueIn(gate, 3)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallIThisRangeDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallIThisRangeDyn));
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate);
    for (size_t i = 1; i < numArgs; i++) { // skip imm
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef newGate = builder_.VariadicRuntimeCall(
        glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), vec);
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallSpreadDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallSpreadDyn));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallIRangeDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CallIRangeDyn));
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate);
    for (size_t i = 1; i < numArgs; i++) { // skip imm
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef newGate = builder_.VariadicRuntimeCall(
        glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), vec);
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNewObjSpreadDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(NewObjSpreadDyn));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerThrowDyn(GateRef gate, GateRef glue)
{
    GateRef exception = acc_.GetValueIn(gate, 0);
    GateRef setException = circuit_->NewGate(OpCode(OpCode::STORE), 0,
        {Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), exception, glue}, VariableType::INT64().GetGateType());
    LowerHirToThrowCall(gate, builder_.Boolean(true), setException);
}

void SlowPathLowering::LowerThrowConstAssignment(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowConstAssignment));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {glue, acc_.GetValueIn(gate, 0)});
    LowerHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowThrowNotExists(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowThrowNotExists));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {glue, acc_.GetValueIn(gate, 0)});
    LowerHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowPatternNonCoercible(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowPatternNonCoercible));
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {glue});
    LowerHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowIfNotObject(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowIfNotObject));
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {glue});
    GateRef value = builder_.ZExtInt8ToPtr(acc_.GetValueIn(gate, 0));
    GateRef isEcmaObject = builder_.IsEcmaObject(value);
    GateRef isNotEcmaObject = builder_.BoolNot(isEcmaObject);
    LowerHirToThrowCall(gate, isNotEcmaObject, newGate);
}

void SlowPathLowering::LowerThrowUndefinedIfHole(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowUndefinedIfHole));
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {glue});
    GateRef hole = builder_.ZExtInt8ToPtr(acc_.GetValueIn(gate, 0));
    GateRef isHole = builder_.TaggedIsHole(hole);
    LowerHirToThrowCall(gate, isHole, newGate);
}

void SlowPathLowering::LowerThrowIfSuperNotCorrectCall(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowIfSuperNotCorrectCall));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowDeleteSuperProperty(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ThrowDeleteSuperProperty));
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {glue});
    LowerHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerExceptionHandler(GateRef hirGate)
{
    GateRef glue = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::GLUE);
    GateRef depend = acc_.GetDep(hirGate);
    GateRef loadException = circuit_->NewGate(OpCode(OpCode::LOAD), VariableType::JS_ANY().GetMachineType(),
        0, { depend, glue }, VariableType::JS_ANY().GetGateType());
    acc_.SetDep(loadException, depend);
    GateRef holeCst = builder_.HoleConstant(VariableType::JS_ANY().GetGateType());
    GateRef clearException = circuit_->NewGate(OpCode(OpCode::STORE), 0,
        { loadException, holeCst, glue }, VariableType::INT64().GetGateType());
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetDep(*it) == hirGate) {
            acc_.ReplaceIn(it, clearException);
        } else {
            acc_.ReplaceIn(it, loadException);
        }
    }
    circuit_->DeleteGate(hirGate);
}

void SlowPathLowering::LowerLdSymbol(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetSymbolFunction));
    GateRef newGate =
        builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdGlobal(GateRef gate, GateRef glue)
{
    GateRef offset = builder_.Int64(JSThread::GlueData::GetGlobalObjOffset(false));
    GateRef val = builder_.Int64Add(glue, offset);
    GateRef newGate = circuit_->NewGate(OpCode(OpCode::LOAD), VariableType::JS_ANY().GetMachineType(),
        0, { Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), val }, VariableType::JS_ANY().GetGateType());
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSub2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Sub2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerMul2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Mul2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDiv2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Div2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerMod2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Mod2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerEqDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(EqDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNotEqDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(NotEqDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLessDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(LessDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLessEqDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(LessEqDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGreaterDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GreaterDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGreaterEqDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GreaterEqDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGetPropIterator(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetPropIterator));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIterNext(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(IterNext));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCloseIterator(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CloseIterator));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIncDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(IncDyn));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDecDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(DecDyn));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerToNumber(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ToNumber));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNegDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(NegDyn));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNotDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(NotDyn));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerShl2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Shl2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerShr2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Shr2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAshr2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Ashr2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAnd2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(And2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerOr2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Or2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerXor2Dyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(Xor2Dyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDelObjProp(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(DelObjProp));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerExpDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(ExpDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsInDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(IsInDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerInstanceofDyn(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(InstanceOfDyn));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerFastStrictNotEqual(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(FastStrictNotEqual));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToFastCall(gate, newGate);
}

void SlowPathLowering::LowerFastStrictEqual(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(FastStrictEqual));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    LowerHirToFastCall(gate, newGate);
}
void SlowPathLowering::LowerCreateEmptyArray(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateEmptyArray), {});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateEmptyObject(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateEmptyObject), {});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    Label successExit(&lm);
    Label exceptionExit(&lm);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(builder_.TruncInt64ToInt32(index));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateArrayWithBuffer), { obj });
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &successExit, &exceptionExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateObjectWithBuffer(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    Label successExit(&lm);
    Label exceptionExit(&lm);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(builder_.TruncInt64ToInt32(index));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { obj });
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &successExit, &exceptionExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStModuleVar(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StModuleVar), { prop, acc_.GetValueIn(gate, 1) });
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetTemplateObject(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetTemplateObject));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef literal = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.RuntimeCall(glue, id,
        Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), { literal });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSetObjectWithProto(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(SetObjectWithProto));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef proto = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.RuntimeCall(glue, id,
        Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), { proto, obj });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdBigInt(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef numberBigInt = GetObjectFromConstPool(acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdBigInt), {numberBigInt});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLdModuleVar(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef key = GetObjectFromConstPool(acc_.GetValueIn(gate, 0));
    GateRef inner = builder_.IntBuildTaggedTypeWithNoGC(acc_.GetValueIn(gate, 1));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdModuleVar), {key, inner});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetModuleNamespace(GateRef gate, GateRef glue)
{
    LabelManager lm(gate, circuit_);
    builder_.SetLabelManager(&lm);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef localName = GetObjectFromConstPool(acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), {localName});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}
}  // namespace panda::ecmascript
