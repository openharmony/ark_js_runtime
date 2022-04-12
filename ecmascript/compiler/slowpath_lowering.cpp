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
GateRef SlowPathLowering::GetConstPool(GateRef jsFunc)
{
    return builder_.Load(VariableType::JS_ANY(), jsFunc, builder_.IntPtr(JSFunction::CONSTANT_POOL_OFFSET));
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constantPool = GetConstPool(jsFunc);
    GateRef offset = builder_.IntPtrMul(builder_.ChangeInt32ToIntPtr(index),
        builder_.IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = builder_.IntPtrAdd(offset, builder_.IntPtr(TaggedArray::DATA_OFFSET));
    return builder_.Load(VariableType::JS_ANY(), constantPool, dataOffset);
}

GateRef SlowPathLowering::GetValueFromConstStringTable(GateRef glue, GateRef gate, uint32_t inIndex)
{
    GateRef id = builder_.Int64(RTSTUB_ID(LoadValueFromConstantStringTable));
    auto idGate = acc_.GetValueIn(gate, inIndex);
    GateRef dependGate = acc_.GetDep(gate);
    return builder_.RuntimeCall(glue, id, dependGate, {idGate});
}

void SlowPathLowering::Lower(GateRef gate, EcmaOpcode op)
{
    GateRef glue = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::GLUE);
    GateRef newTarget = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::NEW_TARGET);
    GateRef jsFunc = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::FUNC);

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
            LowerCreateObjectWithBuffer(gate, glue, jsFunc);
            break;
        case CREATEARRAYWITHBUFFER_PREF_IMM16:
            LowerCreateArrayWithBuffer(gate, glue, jsFunc);
            break;
        case STMODULEVAR_PREF_ID32:
            LowerStModuleVar(gate, glue, jsFunc);
            break;
        case GETTEMPLATEOBJECT_PREF_V8:
            LowerGetTemplateObject(gate, glue);
            break;
        case SETOBJECTWITHPROTO_PREF_V8_V8:
            LowerSetObjectWithProto(gate, glue);
            break;
        case LDBIGINT_PREF_ID32:
            LowerLdBigInt(gate, glue, jsFunc);
            break;
        case LDMODULEVAR_PREF_ID32_IMM8:
            LowerLdModuleVar(gate, glue, jsFunc);
            break;
        case GETMODULENAMESPACE_PREF_ID32:
            LowerGetModuleNamespace(gate, glue, jsFunc);
            break;
        case NEWOBJDYNRANGE_PREF_IMM16_V8:
            LowerNewObjDynRange(gate, glue);
            break;
        case JEQZ_IMM8:
        case JEQZ_IMM16:
            LowerConditionJump(gate, true);
            break;
        case JNEZ_IMM8:
        case JNEZ_IMM16:
            LowerConditionJump(gate, false);
            break;
        case GETITERATORNEXT_PREF_V8_V8:
            LowerGetIteratorNext(gate, glue);
            break;
        case SUPERCALL_PREF_IMM16_V8:
            LowerSuperCall(gate, glue, newTarget);
            break;
        case SUPERCALLSPREAD_PREF_V8:
            LowerSuperCallSpread(gate, glue, newTarget);
            break;
        case ISTRUE_PREF:
            LowerIsTrueOrFalse(gate, glue, true);
            break;
        case ISFALSE_PREF:
            LowerIsTrueOrFalse(gate, glue, false);
            break;
        case GETNEXTPROPNAME_PREF_V8:
            LowerGetNextPropName(gate, glue);
            break;
        case COPYDATAPROPERTIES_PREF_V8_V8:
            LowerCopyDataProperties(gate, glue);
            break;
        case CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
            LowerCreateObjectWithExcludedKeys(gate, glue);
            break;
        case CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8:
            LowerCreateRegExpWithLiteral(gate, glue);
            break;
        case STOWNBYVALUE_PREF_V8_V8:
            LowerStOwnByValue(gate, glue);
            break;
        case STOWNBYINDEX_PREF_V8_IMM32:
            LowerStOwnByIndex(gate, glue);
            break;
        case STOWNBYNAME_PREF_ID32_V8:
            LowerStOwnByName(gate, glue, jsFunc);
            break;
        case DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
            LowerDefineGeneratorFunc(gate, glue, jsFunc);
            break;
        case DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
            LowerDefineAsyncFunc(gate, glue, jsFunc);
            break;
        case NEWLEXENVDYN_PREF_IMM16:
            LowerNewLexicalEnvDyn(gate, glue);
            break;
        case NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16:
            LowerNewLexicalEnvWithNameDyn(gate, glue);
            break;
        case POPLEXENVDYN_PREF:
            LowerPopLexicalEnv(gate, glue);
            break;
        case LDSUPERBYVALUE_PREF_V8_V8:
            LowerLdSuperByValue(gate, glue, jsFunc);
            break;
        case STSUPERBYVALUE_PREF_V8_V8:
            LowerStSuperByValue(gate, glue, jsFunc);
            break;
        case TRYSTGLOBALBYNAME_PREF_ID32:
            LowerTryStGlobalByName(gate, glue);
            break;
        case STCONSTTOGLOBALRECORD_PREF_ID32:
            LowerStConstToGlobalRecord(gate, glue);
            break;
        case STLETTOGLOBALRECORD_PREF_ID32:
            LowerStLetToGlobalRecord(gate, glue, jsFunc);
            break;
        case STCLASSTOGLOBALRECORD_PREF_ID32:
            LowerStClassToGlobalRecord(gate, glue, jsFunc);
            break;
        case STOWNBYVALUEWITHNAMESET_PREF_V8_V8:
            LowerStOwnByValueWithNameSet(gate, glue);
            break;
        case STOWNBYNAMEWITHNAMESET_PREF_ID32_V8:
            LowerStOwnByNameWithNameSet(gate, glue, jsFunc);
            break;
        case LDGLOBALVAR_PREF_ID32:
            LowerLdGlobalVar(gate, glue);
            break;
        case LDOBJBYNAME_PREF_ID32_V8:
            LowerLdObjByName(gate, glue, jsFunc);
            break;
        case STOBJBYNAME_PREF_ID32_V8:
            LowerStObjByName(gate, glue, jsFunc);
            break;
        case LDSUPERBYNAME_PREF_ID32_V8:
            LowerLdSuperByName(gate, glue, jsFunc);
            break;
        case STSUPERBYNAME_PREF_ID32_V8:
            LowerStSuperByName(gate, glue, jsFunc);
            break;
        case CREATEGENERATOROBJ_PREF_V8:
            LowerCreateGeneratorObj(gate, glue);
            break;
        case STARRAYSPREAD_PREF_V8_V8:
            LowerStArraySpread(gate, glue);
            break;
        default:
            break;
    }

    // clear temporary used label manager
    builder_.DeleteCurrentLabelManager();
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
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
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
    LowerHirToFastCall(gate, newGate);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    acc_.SetDep(gate, prop);
    GateRef id = builder_.Int64(RTSTUB_ID(TryLdGlobalByName));
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {prop});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    acc_.SetDep(gate, prop);
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
    builder_.NewLabelManager(gate);
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
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateEmptyObject), {});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(jsFunc, builder_.TruncInt64ToInt32(index));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateArrayWithBuffer), { obj });
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateObjectWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(jsFunc, builder_.TruncInt64ToInt32(index));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { obj });
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStModuleVar(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
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

void SlowPathLowering::LowerLdBigInt(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef numberBigInt = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdBigInt), {numberBigInt});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLdModuleVar(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef key = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef inner = builder_.IntBuildTaggedTypeWithNoGC(acc_.GetValueIn(gate, 1));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdModuleVar), {key, inner});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetModuleNamespace(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef localName = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), {localName});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetIteratorNext(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetIteratorNext));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef method = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        { obj, method });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCall(GateRef gate, GateRef glue, GateRef newTarget)
{
    GateRef id = builder_.Int64(RTSTUB_ID(SuperCall));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef func = acc_.GetValueIn(gate, 2);
    GateRef firstVRegIdx = acc_.GetValueIn(gate, 1);
    GateRef length = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        { func, newTarget, builder_.Int64BuildTaggedTypeNGC(firstVRegIdx),
          builder_.Int64BuildTaggedTypeNGC(length) });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCallSpread(GateRef gate, GateRef glue, GateRef newTarget)
{
    GateRef id = builder_.Int64(RTSTUB_ID(SuperCallSpread));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        { func, newTarget, array });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsTrueOrFalse(GateRef gate, GateRef glue, bool flag)
{
    builder_.NewLabelManager(gate);
    Label isTrue(&builder_);
    Label isFalse(&builder_);
    Label successExit(&builder_);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), acc_.GetValueIn(gate, 0));
    GateRef callResult = builder_.CallRuntime(glue, RTSTUB_ID(ToBoolean), { *result });
    builder_.Branch(builder_.TaggedSpecialValueChecker(callResult, JSTaggedValue::VALUE_TRUE),
        &isTrue, &isFalse);
    builder_.Bind(&isTrue);
    {
        auto trueResult = flag ? builder_.TaggedTrue() : builder_.TaggedFalse();
        result = builder_.ChangeInt64ToTagged(trueResult);
        builder_.Jump(&successExit);
    }
    builder_.Bind(&isFalse);
    {
        auto falseResult = flag ? builder_.TaggedFalse() : builder_.TaggedTrue();
        result = builder_.ChangeInt64ToTagged(falseResult);
        builder_.Jump(&successExit);
    }
    builder_.Bind(&successExit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<true>(gate, *result, successControl, exceptionControl);
}

void SlowPathLowering::LowerNewObjDynRange(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(NewObjDynRange));
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    // 2 : 2 first argument offset
    GateRef firstArgOffset = builder_.Int64(2);
    GateRef firstArgRegIdx = acc_.GetValueIn(gate, 1);
    GateRef firstArgIdx = builder_.Int64Add(firstArgRegIdx, firstArgOffset);
    GateRef numArgs = acc_.GetValueIn(gate, 0);
    GateRef length = builder_.Int64Sub(numArgs, firstArgOffset);
    // 2 : 2 input value
    GateRef ctor = acc_.GetValueIn(gate, 2);
    // 3 : 3 input value
    GateRef newTarget = acc_.GetValueIn(gate, 3);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                           {ctor, newTarget, builder_.Int64BuildTaggedTypeNGC(firstArgIdx),
                                            builder_.Int64BuildTaggedTypeNGC(length)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerConditionJump(GateRef gate, bool isEqualJump)
{
    std::vector<GateRef> trueState;
    GateRef value = acc_.GetValueIn(gate, 0);
    // GET_ACC() == JSTaggedValue::False()
    GateRef condition = builder_.TaggedSpecialValueChecker(value, JSTaggedValue::VALUE_FALSE);
    GateRef ifBranch = builder_.Branch(acc_.GetState(gate), condition);
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    trueState.emplace_back(ifTrue);
    GateRef ifFalse = builder_.IfFalse(ifBranch);

    // (GET_ACC().IsInt() && GET_ACC().GetInt())
    std::vector<GateRef> intFalseState;
    ifBranch = builder_.Branch(ifFalse, builder_.TaggedIsInt(value));
    GateRef isInt = builder_.IfTrue(ifBranch);
    GateRef notInt = builder_.IfFalse(ifBranch);
    intFalseState.emplace_back(notInt);
    condition = builder_.Equal(builder_.TaggedGetInt(value), builder_.Int32(0));
    ifBranch = builder_.Branch(isInt, condition);
    GateRef isZero = builder_.IfTrue(ifBranch);
    trueState.emplace_back(isZero);
    GateRef notZero = builder_.IfFalse(ifBranch);
    if (isEqualJump) {
        intFalseState.emplace_back(notZero);
    } else {
        intFalseState.emplace_back(isZero);
    }
    auto mergeIntState = builder_.Merge(intFalseState.data(), intFalseState.size());

    // (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)
    std::vector<GateRef> doubleFalseState;
    ifBranch = builder_.Branch(mergeIntState, builder_.TaggedIsDouble(value));
    GateRef isDouble = builder_.IfTrue(ifBranch);
    GateRef notDouble = builder_.IfFalse(ifBranch);
    doubleFalseState.emplace_back(notDouble);
    condition = builder_.Equal(builder_.TaggedCastToDouble(value), builder_.Double(0));
    ifBranch = builder_.Branch(isDouble, condition);
    GateRef isDoubleZero = builder_.IfTrue(ifBranch);
    trueState.emplace_back(isDoubleZero);
    GateRef notDoubleZero = builder_.IfFalse(ifBranch);
    if (isEqualJump) {
        doubleFalseState.emplace_back(notDoubleZero);
    } else {
        doubleFalseState.emplace_back(isDoubleZero);
    }
    auto mergeFalseState = builder_.Merge(doubleFalseState.data(), doubleFalseState.size());

    GateRef mergeTrueState = builder_.Merge(trueState.data(), trueState.size());
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::IF_TRUE) {
            acc_.SetOpCode(*it, OpCode::ORDINARY_BLOCK);
            acc_.ReplaceIn(it, mergeTrueState);
        } else if (acc_.GetOpCode(*it) == OpCode::IF_FALSE) {
            acc_.SetOpCode(*it, OpCode::ORDINARY_BLOCK);
            acc_.ReplaceIn(it, mergeFalseState);
        } else if (((acc_.GetOpCode(*it) == OpCode::DEPEND_SELECTOR) ||
                    (acc_.GetOpCode(*it) == OpCode::DEPEND_RELAY)) &&
                    (acc_.GetOpCode(acc_.GetIn(acc_.GetIn(*it, 0), it.GetIndex() - 1)) != OpCode::IF_EXCEPTION)) {
            acc_.ReplaceIn(it, acc_.GetDep(gate));
        } else {
            abort();
        }
    }
    // delete old gate
    circuit_->DeleteGate(gate);
}

void SlowPathLowering::LowerGetNextPropName(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(GetNextPropName));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef iter = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.RuntimeCall(glue, id,
        Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), { iter });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCopyDataProperties(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CopyDataProperties));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef dst = acc_.GetValueIn(gate, 0);
    GateRef src = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.RuntimeCall(glue, id,
        Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), { dst, src });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateObjectWithExcludedKeys(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CreateObjectWithExcludedKeys));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef numKeys = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef firstArgRegIdx = acc_.GetValueIn(gate, 2);
    GateRef newGate = builder_.RuntimeCall(glue, id,
        Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        { builder_.Int64BuildTaggedTypeNGC(numKeys),
        obj, builder_.Int64BuildTaggedTypeNGC(firstArgRegIdx) });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateRegExpWithLiteral(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CreateRegExpWithLiteral));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef pattern = GetValueFromConstStringTable(glue, gate, 0);
    GateRef flags = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.RuntimeCall(glue, id,
        Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        { pattern, builder_.Int64BuildTaggedTypeNGC(flags) });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStOwnByValue(GateRef gate, GateRef glue)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    Label notClassConstructor(&builder_);
    builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    builder_.Bind(&notClassConstructor);
    Label notClassPrototype(&builder_);
    builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    builder_.Bind(&notClassPrototype);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByValue, { glue, receiver, propKey, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByValue),
            { receiver, propKey, accValue });
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // stOwnByValue will not be inValue to other hir gates, result gate will be ignored
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByIndex(GateRef gate, GateRef glue)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    Label notClassConstructor(&builder_);
    builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    builder_.Bind(&notClassConstructor);
    Label notClassPrototype(&builder_);
    builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    builder_.Bind(&notClassPrototype);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByIndex, { glue, receiver, index, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByIndex),
            {receiver, builder_.Int64BuildTaggedTypeNGC(index), accValue });
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByIndex will not be inValue to other hir gates, result gate will be ignored
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isJsObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.IsJsObject(receiver), &isJsObject, &slowPath);
    builder_.Bind(&isJsObject);
    Label notClassConstructor(&builder_);
    builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    builder_.Bind(&notClassConstructor);
    Label notClassPrototype(&builder_);
    builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    builder_.Bind(&notClassPrototype);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByNameWithOwn,
            { glue, receiver, propKey, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByName),
            {receiver, propKey, accValue });
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByName will not be inValue to other hir gates, result gate will be ignored
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerDefineGeneratorFunc(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    DEFVAlUE(method, (&builder_), VariableType::JS_POINTER(),
        GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0)));
    GateRef length = acc_.GetValueIn(gate, 1);
    GateRef lexEnv = acc_.GetValueIn(gate, 2);
    GateRef result;
    Label isResolved(&builder_);
    Label notResolved(&builder_);
    Label defaultLabel(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.FunctionIsResolved(*method), &isResolved, &notResolved);
    builder_.Bind(&isResolved);
    {
        method = builder_.CallRuntime(glue, RTSTUB_ID(DefineGeneratorFunc), { *method });
        Label notException(&builder_);
        builder_.Branch(builder_.TaggedSpecialValueChecker(*method, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &notException);
        builder_.Bind(&notException);
        {
            builder_.SetConstPoolToFunction(glue, *method, GetConstPool(jsFunc));
            builder_.Jump(&defaultLabel);
        }
    }
    builder_.Bind(&notResolved);
    {
        builder_.SetResolvedToFunction(glue, *method, builder_.Boolean(true));
        builder_.Jump(&defaultLabel);
    }
    builder_.Bind(&defaultLabel);
    {
        GateRef hclass = builder_.LoadHClass(*method);
        builder_.SetPropertyInlinedProps(glue, *method, hclass, builder_.Int64BuildTaggedNGC(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        builder_.SetLexicalEnvToFunction(glue, *method, lexEnv);
        builder_.SetModuleToFunction(glue, *method, builder_.GetModuleFromFunction(jsFunc));
        result = *method;
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerDefineAsyncFunc(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    DEFVAlUE(method, (&builder_), VariableType::JS_POINTER(),
             GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0)));
    GateRef length = acc_.GetValueIn(gate, 1);
    GateRef lexEnv = acc_.GetValueIn(gate, 2);
    GateRef result;
    Label isResolved(&builder_);
    Label notResolved(&builder_);
    Label defaultLabel(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.FunctionIsResolved(*method), &isResolved, &notResolved);
    builder_.Bind(&isResolved);
    {
        method = builder_.CallRuntime(glue, RTSTUB_ID(DefineAsyncFunc), { *method });
        Label notException(&builder_);
        builder_.Branch(builder_.TaggedSpecialValueChecker(*method, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &notException);
        builder_.Bind(&notException);
        {
            builder_.SetConstPoolToFunction(glue, *method, GetConstPool(jsFunc));
            builder_.Jump(&defaultLabel);
        }
    }
    builder_.Bind(&notResolved);
    {
        builder_.SetResolvedToFunction(glue, *method, builder_.Boolean(true));
        builder_.Jump(&defaultLabel);
    }
    builder_.Bind(&defaultLabel);
    {
        GateRef hclass = builder_.LoadHClass(*method);
        builder_.SetPropertyInlinedProps(glue, *method, hclass, builder_.Int64BuildTaggedNGC(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        builder_.SetLexicalEnvToFunction(glue, *method, lexEnv);
        builder_.SetModuleToFunction(glue, *method, builder_.GetModuleFromFunction(jsFunc));
        result = *method;
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnvDyn(GateRef gate, GateRef glue)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(NewLexicalEnvDyn),
        {builder_.Int16BuildTaggedTypeWithNoGC(acc_.GetValueIn(gate, 0))});
    builder_.CallRuntime(glue, RTSTUB_ID(SetLexicalEnv), {result});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnvWithNameDyn(GateRef gate, GateRef glue)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithNameDyn),
        {builder_.Int16BuildTaggedTypeWithNoGC(acc_.GetValueIn(gate, 0)),
        builder_.Int16BuildTaggedTypeWithNoGC(acc_.GetValueIn(gate, 1))});
    builder_.CallRuntime(glue, RTSTUB_ID(SetLexicalEnv), {result});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<true>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerPopLexicalEnv(GateRef gate, GateRef glue)
{
    builder_.NewLabelManager(gate);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(GetLexicalEnv), {});
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    GateRef parentLexEnv = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), result, index);
    builder_.CallRuntime(glue, RTSTUB_ID(SetLexicalEnv), {parentLexEnv});
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    builder_.MergeMirCircuit<false>(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLdSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc)
{
    GateRef id = builder_.Int64(RTSTUB_ID(LdSuperByValue));
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                           { receiver, propKey, jsFunc });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc)
{
    GateRef id = builder_.Int64(RTSTUB_ID(StSuperByValue));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                           { receiver, propKey, value, jsFunc});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTryStGlobalByName(GateRef gate, GateRef glue)
{
    // order: 1. global record 2. global object
    builder_.NewLabelManager(gate);
    DEFVAlUE(res, (&builder_), VariableType::JS_ANY(), builder_.Int64(JSTaggedValue::VALUE_HOLE));
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef stringId = acc_.GetValueIn(gate, 0);
    GateRef propKey = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable), { stringId });
    Label isUndefined(&builder_);
    Label notUndefined(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);

    // order: 1. global record 2. global object
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef recordInfo = builder_.CallRuntime(glue, RTSTUB_ID(LdGlobalRecord), { propKey });
    builder_.Branch(builder_.TaggedSpecialValueChecker(recordInfo, JSTaggedValue::VALUE_UNDEFINED),
                    &isUndefined, &notUndefined);
    builder_.Bind(&notUndefined);
    {
        res = builder_.CallRuntime(glue, RTSTUB_ID(TryUpdateGlobalRecord), { propKey, value });
        builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
    }
    builder_.Bind(&isUndefined);
    {
        Label isHole(&builder_);
        Label notHole(&builder_);
        GateRef globalResult = builder_.CallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { propKey });
        builder_.Branch(builder_.TaggedSpecialValueChecker(globalResult, JSTaggedValue::VALUE_HOLE), &isHole, &notHole);
        builder_.Bind(&isHole);
        {
            res = builder_.CallRuntime(glue, RTSTUB_ID(ThrowReferenceError), { propKey });
            builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
                            &exceptionExit, &successExit);
        }
        builder_.Bind(&notHole);
        {
            res = builder_.CallRuntime(glue, RTSTUB_ID(StGlobalVar), { propKey, value });
            builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
                            &exceptionExit, &successExit);
        }
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStConstToGlobalRecord(GateRef gate, GateRef glue)
{
    GateRef propKey = GetValueFromConstStringTable(glue, gate, 0);
    acc_.SetDep(gate, propKey);
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef id = builder_.Int64(RTSTUB_ID(StGlobalRecord));
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef isConst = builder_.Int64(JSTaggedValue::VALUE_TRUE);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
                                           { propKey, value, isConst });
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStLetToGlobalRecord(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef falseConst = builder_.Boolean(false);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
        {prop,  acc_.GetValueIn(gate, 1), falseConst});
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStClassToGlobalRecord(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef falseConst = builder_.Boolean(false);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
        {prop,  acc_.GetValueIn(gate, 1), falseConst});
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByValueWithNameSet(GateRef gate, GateRef glue)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label notClassConstructor(&builder_);
    Label notClassPrototype(&builder_);
    Label notHole(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef res;
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    {
        builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        builder_.Bind(&notClassConstructor);
        {
            builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
            builder_.Bind(&notClassPrototype);
            {
                res = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByValue,
                    { glue, receiver, propKey, accValue });
                builder_.Branch(builder_.TaggedSpecialValueChecker(res, JSTaggedValue::VALUE_HOLE),
                    &slowPath, &notHole);
                builder_.Bind(&notHole);
                {
                    Label notexception(&builder_);
                    builder_.Branch(builder_.TaggedSpecialValueChecker(res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &notexception);
                    builder_.Bind(&notexception);
                    res = builder_.CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix),
                        { accValue, propKey });
                    builder_.Branch(builder_.TaggedSpecialValueChecker(res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
                }
            }
        }
    }
    builder_.Bind(&slowPath);
    {
        res = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, accValue });
        builder_.Branch(builder_.TaggedSpecialValueChecker(res, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByNameWithNameSet(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    GateRef result;
    Label isJsObject(&builder_);
    Label notJSObject(&builder_);
    Label notClassConstructor(&builder_);
    Label notClassPrototype(&builder_);
    Label notHole(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.IsJsObject(receiver), &isJsObject, &notJSObject);
    builder_.Bind(&isJsObject);
    {
        builder_.Branch(builder_.IsClassConstructor(receiver), &notJSObject, &notClassConstructor);
        builder_.Bind(&notClassConstructor);
        {
            builder_.Branch(builder_.IsClassPrototype(receiver), &notJSObject, &notClassPrototype);
            builder_.Bind(&notClassPrototype);
            {
                result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByNameWithOwn,
                    { glue, receiver, propKey, accValue });
                builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_HOLE),
                    &notJSObject, &notHole);
                builder_.Bind(&notHole);
                {
                    Label notException(&builder_);
                    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &notException);
                    builder_.Bind(&notException);
                    result = builder_.CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), {accValue, propKey});
                    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
                }
            }
        }
    }
    builder_.Bind(&notJSObject);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, accValue });
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
}

void SlowPathLowering::LowerLdGlobalVar(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    GateRef id = builder_.Int64(RTSTUB_ID(TryLdGlobalByName));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate =
        builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)), {prop});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdObjByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef undefined = builder_.Int64(JSTaggedValue::VALUE_UNDEFINED);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LoadICByName), {undefined,
        acc_.GetValueIn(gate, 1), prop, builder_.IntBuildTaggedTypeWithNoGC(undefined)});
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef undefined = builder_.Int64(JSTaggedValue::VALUE_UNDEFINED);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StoreICByName), {undefined,
        acc_.GetValueIn(gate, 1), prop, acc_.GetValueIn(gate, 2), builder_.IntBuildTaggedTypeWithNoGC(undefined)});
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdSuperByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {prop, acc_.GetValueIn(gate, 1)});
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStSuperByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    builder_.NewLabelManager(gate);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StSuperByValue), {prop, acc_.GetValueIn(gate, 1),
        acc_.GetValueIn(gate, 2)});
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    builder_.MergeMirCircuit(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerCreateGeneratorObj(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(CreateGeneratorObj));
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0)});
    LowerHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStArraySpread(GateRef gate, GateRef glue)
{
    GateRef id = builder_.Int64(RTSTUB_ID(StArraySpread));
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.RuntimeCall(glue, id, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)),
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    LowerHirToCall(gate, newGate);
}
}  // namespace panda::ecmascript
