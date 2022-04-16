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
        auto op = circuit_->GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
        } else if (op == OpCode::GET_EXCEPTION) {
            LowerExceptionHandler(gate);
        }
    }

    if (IsLogEnabled()) {
        COMPILER_LOG(INFO) << "=========================================================";
        circuit_->PrintAllGates(*bcBuilder_);
    }
}

void SlowPathLowering::ReplaceHirControlGate(GateAccessor::UsesIterator &useIt, GateRef newGate, bool noThrow)
{
    GateAccessor acc(circuit_);
    ASSERT(acc.GetOpCode(*useIt) == OpCode::IF_SUCCESS || acc.GetOpCode(*useIt) == OpCode::IF_EXCEPTION);
    if (!noThrow) {
        auto firstUse = acc.Uses(*useIt).begin();
        circuit_->ModifyIn(*firstUse, firstUse.GetIndex(), newGate);
    }
    acc.DeleteGate(useIt);
}

void SlowPathLowering::ReplaceHirToSubCfg(GateRef hir, GateRef outir,
                                          const std::vector<GateRef> &successControl,
                                          const std::vector<GateRef> &exceptionControl,
                                          bool noThrow)
{
    GateAccessor acc(circuit_);
    if (outir != Circuit::NullGate()) {
        acc.SetGateType(outir, acc.GetGateType(hir));
    }
    auto uses = acc.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        // replace HIR:IF_SUCCESS/IF_EXCEPTION with control flow in Label successExit/failExit of MIR Circuit
        if (acc.GetOpCode(*useIt) == OpCode::IF_SUCCESS) {
            ReplaceHirControlGate(useIt, successControl[0]);
        } else if (acc.GetOpCode(*useIt) == OpCode::IF_EXCEPTION) {
            ReplaceHirControlGate(useIt, exceptionControl[0], noThrow);
        // change depend flow in catch block from HIR:JS_BYTECODE to depend flow in MIR Circuit
        } else if (acc.GetOpCode(*useIt) == OpCode::DEPEND_SELECTOR) {
            if (acc.GetOpCode(acc.GetIn(acc.GetIn(*useIt, 0), useIt.GetIndex() - 1)) == OpCode::IF_EXCEPTION) {
                noThrow ? acc.DeleteExceptionDep(useIt) : acc.ReplaceIn(useIt, exceptionControl[1]);
            } else {
                acc.ReplaceIn(useIt, successControl[1]);
            }
        } else if (acc.GetOpCode(*useIt) == OpCode::DEPEND_RELAY) {
            if (acc.GetOpCode(acc.GetIn(*useIt, 0)) == OpCode::IF_EXCEPTION) {
                acc.ReplaceIn(useIt, exceptionControl[1]);
            } else {
                acc.ReplaceIn(useIt, successControl[1]);
            }
        // replace normal depend
        } else if ((acc.GetOpCode(*useIt) == OpCode::JS_BYTECODE) && useIt.GetIndex() == 1) {
            acc.ReplaceIn(useIt, successControl[1]);
        // if no catch block, just throw exception(RETURN)
        } else if ((acc.GetOpCode(*useIt) == OpCode::RETURN) &&
                    acc.GetOpCode(acc.GetIn(*useIt, 0)) == OpCode::IF_EXCEPTION) {
            noThrow ? acc.DeleteExceptionDep(useIt) : acc.ReplaceIn(useIt, exceptionControl[1]);
        // if isThrow..
        } else if (useIt.GetIndex() == 1) {
            acc.ReplaceIn(useIt, successControl[1]);
        // replace data flow with data output in label successExit(valueSelector...)
        } else {
            acc.ReplaceIn(useIt, outir);
        }
    }

    circuit_->DeleteGate(hir);
}


/*
 * lower to slowpath call like this pattern:
 * have throw:
 * res = Call(...);
 * if (res == VALUE_EXCEPTION) {
 *     goto exception_handle;
 * }
 * Set(res);
 *
 * no throw:
 * res = Call(...);
 * Set(res);
 */
void SlowPathLowering::ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch;
    if (!noThrow) {
        // exception value
        GateRef exceptionVal = builder_.ExceptionConstant(GateType::TAGGED_NO_POINTER);
        // compare with trampolines result
        GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);
        ifBranch = builder_.Branch(stateInGate, equal);
    } else {
        ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    }

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
 * lower to throw call like this pattern:
 * if (condition) {
 *     Call(...);
 *     goto exception_handle;
 * }
 *
 */
void SlowPathLowering::ReplaceHirToThrowCall(GateRef hirGate, GateRef condGate, GateRef callGate)
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

// labelmanager must be initialized
GateRef SlowPathLowering::GetConstPool(GateRef jsFunc)
{
    return builder_.Load(VariableType::JS_ANY(), jsFunc, builder_.IntPtr(JSFunction::CONSTANT_POOL_OFFSET));
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetCurrentEnv(GateRef jsFunc)
{
    return builder_.Load(VariableType::JS_ANY(), jsFunc, builder_.IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = GetConstPool(jsFunc);
    return builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), constPool, index);
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetHomeObjectFromJSFunction(GateRef jsFunc)
{
    GateRef offset = builder_.IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return builder_.Load(VariableType::JS_ANY(), jsFunc, offset);
}

GateRef SlowPathLowering::GetValueFromConstStringTable(GateRef glue, GateRef gate, uint32_t inIndex)
{
    GateRef id = RTSTUB_ID(LoadValueFromConstantStringTable);
    auto idGate = acc_.GetValueIn(gate, inIndex);
    GateRef dependGate = acc_.GetDep(gate);
    return builder_.CallRuntimeWithDepend(glue, id, dependGate, {idGate});
}

void SlowPathLowering::Lower(GateRef gate)
{
    GateRef glue = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::GLUE);
    GateRef newTarget = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::NEW_TARGET);
    GateRef jsFunc = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::FUNC);
    GateRef thisObj = bcBuilder_->GetCommonArgByIndex(CommonArgIdx::THIS);

    auto pc = bcBuilder_->GetJSBytecode(gate);
    EcmaOpcode op = static_cast<EcmaOpcode>(*pc);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
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
            LowerCallIThisRangeDyn(gate, glue, thisObj);
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
            LowerStModuleVar(gate, glue);
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
            LowerLdModuleVar(gate, glue);
            break;
        case GETMODULENAMESPACE_PREF_ID32:
            LowerGetModuleNamespace(gate, glue);
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
            LowerStOwnByName(gate, glue);
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
            LowerStLetToGlobalRecord(gate, glue);
            break;
        case STCLASSTOGLOBALRECORD_PREF_ID32:
            LowerStClassToGlobalRecord(gate, glue);
            break;
        case STOWNBYVALUEWITHNAMESET_PREF_V8_V8:
            LowerStOwnByValueWithNameSet(gate, glue);
            break;
        case STOWNBYNAMEWITHNAMESET_PREF_ID32_V8:
            LowerStOwnByNameWithNameSet(gate, glue);
            break;
        case LDGLOBALVAR_PREF_ID32:
            LowerLdGlobalVar(gate, glue);
            break;
        case LDOBJBYNAME_PREF_ID32_V8:
            LowerLdObjByName(gate, glue);
            break;
        case STOBJBYNAME_PREF_ID32_V8:
            LowerStObjByName(gate, glue);
            break;
        case LDSUPERBYNAME_PREF_ID32_V8:
            LowerLdSuperByName(gate, glue);
            break;
        case STSUPERBYNAME_PREF_ID32_V8:
            LowerStSuperByName(gate, glue);
            break;
        case CREATEGENERATOROBJ_PREF_V8:
            LowerCreateGeneratorObj(gate, glue);
            break;
        case STARRAYSPREAD_PREF_V8_V8:
            LowerStArraySpread(gate, glue);
            break;
        case LDLEXVARDYN_PREF_IMM4_IMM4:
        case LDLEXVARDYN_PREF_IMM8_IMM8:
        case LDLEXVARDYN_PREF_IMM16_IMM16:
            LowerLdLexVarDyn(gate, jsFunc);
            break;
        case STLEXVARDYN_PREF_IMM4_IMM4_V8:
        case STLEXVARDYN_PREF_IMM8_IMM8_V8:
        case STLEXVARDYN_PREF_IMM16_IMM16_V8:
            LowerStLexVarDyn(gate, glue, jsFunc);
            break;
        case CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
            LowerCreateObjectHavingMethod(gate, glue, jsFunc);
            break;
        case LDHOMEOBJECT_PREF:
            LowerLdHomeObject(gate, thisObj);
            break;
        case DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
            LowerDefineClassWithBuffer(gate, glue, jsFunc);
            break;
        default:
            break;
    }
}

void SlowPathLowering::LowerAdd2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Add2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(CreateIterResultObj);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(SuspendGenerator);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(AsyncFunctionAwaitUncaught);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(AsyncFunctionResolveOrReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef tureConst = builder_.Boolean(true);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), tureConst});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(AsyncFunctionResolveOrReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef falseConst = builder_.Boolean(false);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), falseConst});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLoadStr(GateRef gate, GateRef glue)
{
    GateRef newGate = GetValueFromConstStringTable(glue, gate, 0);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLexicalEnv(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GetLexicalEnv);
    GateRef newGate = builder_.CallRuntime(glue, id, {});
    ReplaceHirToCall(gate, newGate, true);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    acc_.SetDep(gate, prop);
    int id = RTSTUB_ID(TryLdGlobalByName);
    GateRef newGate = builder_.CallRuntime(glue, id, {prop});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    acc_.SetDep(gate, prop);
    int id = RTSTUB_ID(StGlobalVar);
    ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: number of value inputs
    GateRef newGate = builder_.CallRuntime(glue, id, {prop, acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGetIterator(GateRef gate, GateRef glue)
{
    Label condTrue(&builder_);
    Label condFalse(&builder_);
    GateRef isGeneratorObject = builder_.TaggedIsGeneratorObject(acc_.GetValueIn(gate, 0));
    GateRef isNotGeneratorObject = builder_.BoolNot(isGeneratorObject);
    builder_.Branch(isNotGeneratorObject, &condTrue, &condFalse);
    CREATE_DOUBLE_EXIT(condTrue, condTrue)
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    int id = RTSTUB_ID(GetIterator);
    GateRef result = builder_.CallRuntime(glue, id, {glue, acc_.GetValueIn(gate, 0)}, true);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerToJSCall(GateRef gate, GateRef glue, const std::vector<GateRef> &args)
{
    const CallSignature *descriptor = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCall));
    GateRef newGate = builder_.NoGcRuntimeCall(descriptor, glue, target, dependEntry_, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallArg0Dyn(GateRef gate, GateRef glue)
{
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef actualArgc = builder_.Int32(acc_.GetNumValueIn(gate) - 1 + NUM_MANDATORY_JSFUNC_ARGS);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    LowerToJSCall(gate, glue, {glue, actualArgc, acc_.GetValueIn(gate, 0), newTarget, thisObj});
}

void SlowPathLowering::LowerCallArg1Dyn(GateRef gate, GateRef glue)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef actualArgc = builder_.Int32(acc_.GetNumValueIn(gate) - 1 + NUM_MANDATORY_JSFUNC_ARGS);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    LowerToJSCall(gate, glue, {glue, actualArgc,
        acc_.GetValueIn(gate, 0), newTarget, thisObj, acc_.GetValueIn(gate, 1)});
}

void SlowPathLowering::LowerCallArgs2Dyn(GateRef gate, GateRef glue)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef actualArgc = builder_.Int32(acc_.GetNumValueIn(gate) - 1 + NUM_MANDATORY_JSFUNC_ARGS);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    LowerToJSCall(gate, glue, {glue, actualArgc,
        acc_.GetValueIn(gate, 0), newTarget, thisObj, acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
}

void SlowPathLowering::LowerCallArgs3Dyn(GateRef gate, GateRef glue)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef actualArgc = builder_.Int32(acc_.GetNumValueIn(gate) - 1 + NUM_MANDATORY_JSFUNC_ARGS);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    LowerToJSCall(gate, glue, {glue, actualArgc,
        acc_.GetValueIn(gate, 0), newTarget, thisObj, acc_.GetValueIn(gate, 1),
        acc_.GetValueIn(gate, 2), acc_.GetValueIn(gate, 3)});
}

void SlowPathLowering::LowerCallIThisRangeDyn(GateRef gate, GateRef glue, GateRef thisObj)
{
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate) - 1;
    GateRef actualArgc = builder_.Int32(numArgs + NUM_MANDATORY_JSFUNC_ARGS);
    GateRef callTarget = acc_.GetValueIn(gate, 0);
    GateRef newTarget = builder_.Undefined();
    vec.emplace_back(glue);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);

    for (size_t i = 1; i < numArgs; i++) { // skip imm
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    LowerToJSCall(gate, glue, vec);
}

void SlowPathLowering::LowerCallSpreadDyn(GateRef gate, GateRef glue)
{
    // need to fixed in later
    int id = RTSTUB_ID(CallSpreadDyn);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.CallRuntime(glue, id,
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallIRangeDyn(GateRef gate, GateRef glue)
{
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate) - 1;
    GateRef actualArgc = builder_.Int32(numArgs + NUM_MANDATORY_JSFUNC_ARGS);
    GateRef callTarget = acc_.GetValueIn(gate, 0);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    vec.emplace_back(glue);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);

    for (size_t i = 1; i < numArgs; i++) { // skip imm
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    LowerToJSCall(gate, glue, vec);
}

void SlowPathLowering::LowerNewObjSpreadDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(NewObjSpreadDyn);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.CallRuntime(glue, id,
        {glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerThrowDyn(GateRef gate, GateRef glue)
{
    GateRef exception = acc_.GetValueIn(gate, 0);
    GateRef setException = circuit_->NewGate(OpCode(OpCode::STORE), 0,
        {dependEntry_, exception, glue}, VariableType::INT64().GetGateType());
    ReplaceHirToThrowCall(gate, builder_.Boolean(true), setException);
}

void SlowPathLowering::LowerThrowConstAssignment(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowConstAssignment);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id,
        {glue, acc_.GetValueIn(gate, 0)});
    ReplaceHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowThrowNotExists(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowThrowNotExists);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {glue, acc_.GetValueIn(gate, 0)});
    ReplaceHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowPatternNonCoercible(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowPatternNonCoercible);
    GateRef newGate = builder_.CallRuntime(glue, id, {glue});
    ReplaceHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowIfNotObject(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowIfNotObject);
    GateRef newGate = builder_.CallRuntime(glue, id, {glue});
    GateRef value = builder_.ZExtInt8ToPtr(acc_.GetValueIn(gate, 0));
    GateRef isEcmaObject = builder_.IsEcmaObject(value);
    GateRef isNotEcmaObject = builder_.BoolNot(isEcmaObject);
    ReplaceHirToThrowCall(gate, isNotEcmaObject, newGate);
}

void SlowPathLowering::LowerThrowUndefinedIfHole(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowUndefinedIfHole);
    GateRef newGate = builder_.CallRuntime(glue, id, {glue});
    GateRef hole = acc_.GetValueIn(gate, 0);
    GateRef isHole = builder_.TaggedIsHole(hole);
    ReplaceHirToThrowCall(gate, isHole, newGate);
}

void SlowPathLowering::LowerThrowIfSuperNotCorrectCall(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowIfSuperNotCorrectCall);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToThrowCall(gate, builder_.Boolean(true), newGate);
}

void SlowPathLowering::LowerThrowDeleteSuperProperty(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ThrowDeleteSuperProperty);
    GateRef newGate = builder_.CallRuntime(glue, id, {glue});
    ReplaceHirToThrowCall(gate, builder_.Boolean(true), newGate);
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
    int id = RTSTUB_ID(GetSymbolFunction);
    GateRef newGate =
        builder_.CallRuntime(glue, id, {});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdGlobal(GateRef gate, GateRef glue)
{
    GateRef offset = builder_.Int64(JSThread::GlueData::GetGlobalObjOffset(false));
    GateRef val = builder_.Int64Add(glue, offset);
    GateRef newGate = circuit_->NewGate(OpCode(OpCode::LOAD), VariableType::JS_ANY().GetMachineType(),
        0, { dependEntry_, val }, VariableType::JS_ANY().GetGateType());
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSub2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Sub2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerMul2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Mul2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDiv2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Div2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerMod2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Mod2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerEqDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(EqDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNotEqDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(NotEqDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLessDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(LessDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLessEqDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(LessEqDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGreaterDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GreaterDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGreaterEqDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GreaterEqDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGetPropIterator(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GetPropIterator);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIterNext(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(IterNext);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCloseIterator(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(CloseIterator);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIncDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(IncDyn);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDecDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(DecDyn);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerToNumber(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ToNumber);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNegDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(NegDyn);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNotDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(NotDyn);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerShl2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Shl2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerShr2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Shr2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAshr2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Ashr2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAnd2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(And2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerOr2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Or2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerXor2Dyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(Xor2Dyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDelObjProp(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(DelObjProp);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerExpDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(ExpDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsInDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(IsInDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerInstanceofDyn(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(InstanceOfDyn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerFastStrictNotEqual(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(FastStrictNotEqual);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate, true);
}

void SlowPathLowering::LowerFastStrictEqual(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(FastStrictEqual);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate, true);
}

void SlowPathLowering::LowerCreateEmptyArray(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateEmptyArray), {}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateEmptyObject(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateEmptyObject), {}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(jsFunc, builder_.TruncInt64ToInt32(index));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateArrayWithBuffer), { obj }, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateObjectWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(jsFunc, builder_.TruncInt64ToInt32(index));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { obj }, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStModuleVar(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StModuleVar), { prop, acc_.GetValueIn(gate, 1) }, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetTemplateObject(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GetTemplateObject);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef literal = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.CallRuntime(glue, id, { literal });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSetObjectWithProto(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(SetObjectWithProto);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef proto = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.CallRuntime(glue, id, { proto, obj });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdBigInt(GateRef gate, GateRef glue, GateRef jsFunc)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef numberBigInt = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdBigInt), {numberBigInt}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerLdModuleVar(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef key = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef inner = builder_.TaggedTypeNGC(acc_.GetValueIn(gate, 1));
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdModuleVar), {key, inner}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetModuleNamespace(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef localName = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), {localName}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetIteratorNext(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GetIteratorNext);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef method = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.CallRuntime(glue, id, { obj, method });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCall(GateRef gate, GateRef glue, GateRef newTarget)
{
    int id = RTSTUB_ID(SuperCall);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef func = acc_.GetValueIn(gate, 2);
    GateRef firstVRegIdx = acc_.GetValueIn(gate, 1);
    GateRef length = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.CallRuntime(glue, id, { func, newTarget, builder_.TaggedTypeNGC(firstVRegIdx),
          builder_.TaggedTypeNGC(length) });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCallSpread(GateRef gate, GateRef glue, GateRef newTarget)
{
    int id = RTSTUB_ID(SuperCallSpread);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.CallRuntime(glue, id, { func, newTarget, array });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsTrueOrFalse(GateRef gate, GateRef glue, bool flag)
{
    Label isTrue(&builder_);
    Label isFalse(&builder_);
    Label successExit(&builder_);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), acc_.GetValueIn(gate, 0));
    GateRef callResult = builder_.CallRuntime(glue, RTSTUB_ID(ToBoolean), { *result }, true);
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
    ReplaceHirToSubCfg(gate, *result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerNewObjDynRange(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(NewObjDynRange);
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
    GateRef newGate = builder_.CallRuntime(glue, id, {ctor, newTarget, builder_.TaggedTypeNGC(firstArgIdx),
                                            builder_.TaggedTypeNGC(length)});
    ReplaceHirToCall(gate, newGate);
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
            UNREACHABLE();
        }
    }
    // delete old gate
    circuit_->DeleteGate(gate);
}

void SlowPathLowering::LowerGetNextPropName(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(GetNextPropName);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef iter = acc_.GetValueIn(gate, 0);
    GateRef newGate = builder_.CallRuntime(glue, id, { iter });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCopyDataProperties(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(CopyDataProperties);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef dst = acc_.GetValueIn(gate, 0);
    GateRef src = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.CallRuntime(glue, id, { dst, src });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateObjectWithExcludedKeys(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(CreateObjectWithExcludedKeys);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef numKeys = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef firstArgRegIdx = acc_.GetValueIn(gate, 2);
    GateRef newGate = builder_.CallRuntime(glue, id, { builder_.TaggedTypeNGC(numKeys),
        obj, builder_.TaggedTypeNGC(firstArgRegIdx) });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateRegExpWithLiteral(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(CreateRegExpWithLiteral);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef pattern = GetValueFromConstStringTable(glue, gate, 0);
    GateRef flags = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.CallRuntime(glue, id, { pattern, builder_.TaggedTypeNGC(flags) });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStOwnByValue(GateRef gate, GateRef glue)
{
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
            { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // stOwnByValue will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByIndex(GateRef gate, GateRef glue)
{
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
            {receiver, builder_.TaggedTypeNGC(index), accValue }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByIndex will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByName(GateRef gate, GateRef glue)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
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
            {receiver, propKey, accValue }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByName will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerDefineGeneratorFunc(GateRef gate, GateRef glue, GateRef jsFunc)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef methodId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef firstMethod = GetObjectFromConstPool(jsFunc, methodId);
    DEFVAlUE(method, (&builder_), VariableType::JS_POINTER(), firstMethod);
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
        method = builder_.CallRuntime(glue, RTSTUB_ID(DefineGeneratorFunc), { *method }, true);
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
        builder_.SetPropertyInlinedProps(glue, *method, hclass, builder_.TaggedNGC(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        builder_.SetLexicalEnvToFunction(glue, *method, lexEnv);
        builder_.SetModuleToFunction(glue, *method, builder_.GetModuleFromFunction(jsFunc));
        result = *method;
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerDefineAsyncFunc(GateRef gate, GateRef glue, GateRef jsFunc)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef methodId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef firstMethod = GetObjectFromConstPool(jsFunc, methodId);
    DEFVAlUE(method, (&builder_), VariableType::JS_POINTER(), firstMethod);
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
        method = builder_.CallRuntime(glue, RTSTUB_ID(DefineAsyncFunc), { *method }, true);
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
        builder_.SetPropertyInlinedProps(glue, *method, hclass, builder_.TaggedNGC(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        builder_.SetLexicalEnvToFunction(glue, *method, lexEnv);
        builder_.SetModuleToFunction(glue, *method, builder_.GetModuleFromFunction(jsFunc));
        result = *method;
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnvDyn(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(NewLexicalEnvDyn),
        {builder_.TaggedTypeNGC(acc_.GetValueIn(gate, 0))}, true);
    builder_.CallRuntime(glue, RTSTUB_ID(SetLexicalEnv), {result}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerNewLexicalEnvWithNameDyn(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithNameDyn),
        {builder_.TaggedTypeNGC(acc_.GetValueIn(gate, 0)),
        builder_.TaggedTypeNGC(acc_.GetValueIn(gate, 1))}, true);
    builder_.CallRuntime(glue, RTSTUB_ID(SetLexicalEnv), {result}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerPopLexicalEnv(GateRef gate, GateRef glue)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(GetLexicalEnv), {}, true);
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    GateRef parentLexEnv = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), result, index);
    builder_.CallRuntime(glue, RTSTUB_ID(SetLexicalEnv), {parentLexEnv}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLdSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc)
{
    int id = RTSTUB_ID(LdSuperByValue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef newGate = builder_.CallRuntime(glue, id, { receiver, propKey, jsFunc });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc)
{
    int id = RTSTUB_ID(StSuperByValue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef newGate = builder_.CallRuntime(glue, id, { receiver, propKey, value, jsFunc});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTryStGlobalByName(GateRef gate, GateRef glue)
{
    // order: 1. global record 2. global object
    DEFVAlUE(res, (&builder_), VariableType::JS_ANY(), builder_.Int64(JSTaggedValue::VALUE_HOLE));
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef stringId = acc_.GetValueIn(gate, 0);
    GateRef propKey = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable), { stringId }, true);
    Label isUndefined(&builder_);
    Label notUndefined(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);

    // order: 1. global record 2. global object
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef recordInfo = builder_.CallRuntime(glue, RTSTUB_ID(LdGlobalRecord), { propKey }, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(recordInfo, JSTaggedValue::VALUE_UNDEFINED),
                    &isUndefined, &notUndefined);
    builder_.Bind(&notUndefined);
    {
        res = builder_.CallRuntime(glue, RTSTUB_ID(TryUpdateGlobalRecord), { propKey, value }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
    }
    builder_.Bind(&isUndefined);
    {
        Label isHole(&builder_);
        Label notHole(&builder_);
        GateRef globalResult = builder_.CallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { propKey }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(globalResult, JSTaggedValue::VALUE_HOLE), &isHole, &notHole);
        builder_.Bind(&isHole);
        {
            res = builder_.CallRuntime(glue, RTSTUB_ID(ThrowReferenceError), { propKey }, true);
            builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
                            &exceptionExit, &successExit);
        }
        builder_.Bind(&notHole);
        {
            res = builder_.CallRuntime(glue, RTSTUB_ID(StGlobalVar), { propKey, value }, true);
            builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
                            &exceptionExit, &successExit);
        }
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStConstToGlobalRecord(GateRef gate, GateRef glue)
{
    GateRef propKey = GetValueFromConstStringTable(glue, gate, 0);
    acc_.SetDep(gate, propKey);
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    int id = RTSTUB_ID(StGlobalRecord);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef isConst = builder_.Int64(JSTaggedValue::VALUE_TRUE);
    GateRef newGate = builder_.CallRuntime(glue, id, { propKey, value, isConst });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStLetToGlobalRecord(GateRef gate, GateRef glue)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef falseConst = builder_.Boolean(false);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
        {prop,  acc_.GetValueIn(gate, 1), falseConst}, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStClassToGlobalRecord(GateRef gate, GateRef glue)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef falseConst = builder_.Boolean(false);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
        {prop,  acc_.GetValueIn(gate, 1), falseConst}, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByValueWithNameSet(GateRef gate, GateRef glue)
{
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
                        { accValue, propKey }, true);
                    builder_.Branch(builder_.TaggedSpecialValueChecker(res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
                }
            }
        }
    }
    builder_.Bind(&slowPath);
    {
        res = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(res, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByNameWithNameSet(GateRef gate, GateRef glue)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
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
                    result = builder_.CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), {accValue, propKey}, true);
                    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
                }
            }
        }
    }
    builder_.Bind(&notJSObject);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
}

void SlowPathLowering::LowerLdGlobalVar(GateRef gate, GateRef glue)
{
    GateRef prop = GetValueFromConstStringTable(glue, gate, 0);
    int id = RTSTUB_ID(TryLdGlobalByName);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate =
        builder_.CallRuntime(glue, id, {prop});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdObjByName(GateRef gate, GateRef glue)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef undefined = builder_.Int64(JSTaggedValue::VALUE_UNDEFINED);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LoadICByName), {undefined,
        acc_.GetValueIn(gate, 1), prop, builder_.TaggedTypeNGC(undefined)}, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByName(GateRef gate, GateRef glue)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef undefined = builder_.Int64(JSTaggedValue::VALUE_UNDEFINED);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StoreICByName), {undefined,
        acc_.GetValueIn(gate, 1), prop, acc_.GetValueIn(gate, 2), builder_.TaggedTypeNGC(undefined)}, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdSuperByName(GateRef gate, GateRef glue)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {prop, acc_.GetValueIn(gate, 1)}, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStSuperByName(GateRef gate, GateRef glue)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = builder_.CallRuntime(glue, RTSTUB_ID(LoadValueFromConstantStringTable),
        { acc_.GetValueIn(gate, 0) }, true);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(StSuperByValue), {prop, acc_.GetValueIn(gate, 1),
        acc_.GetValueIn(gate, 2)}, true);
    builder_.Branch(builder_.TaggedSpecialValueChecker(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerCreateGeneratorObj(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(CreateGeneratorObj);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStArraySpread(GateRef gate, GateRef glue)
{
    int id = RTSTUB_ID(StArraySpread);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = builder_.CallRuntime(glue, id, {acc_.GetValueIn(gate, 0),
        acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdLexVarDyn(GateRef gate, GateRef jsFunc)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef level = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef slot = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    DEFVAlUE(currentEnv, (&builder_), VariableType::JS_ANY(), GetCurrentEnv(jsFunc));
    DEFVAlUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;

    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopHead, &afterLoop);
    builder_.LoopBegin(&loopHead);
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    currentEnv = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), *currentEnv, index);
    i = builder_.Int32Add(*i, builder_.Int32(1));
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopEnd, &afterLoop);
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);
    builder_.Bind(&afterLoop);
    GateRef valueIndex = builder_.Int32Add(slot, builder_.Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    GateRef result = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), *currentEnv, valueIndex);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerStLexVarDyn(GateRef gate, GateRef glue, GateRef jsFunc)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef level = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef slot = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    GateRef value = acc_.GetValueIn(gate, 2);
    DEFVAlUE(currentEnv, (&builder_), VariableType::JS_ANY(), GetCurrentEnv(jsFunc));
    DEFVAlUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopHead, &afterLoop);
    builder_.LoopBegin(&loopHead);
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    currentEnv = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), *currentEnv, index);
    i = builder_.Int32Add(*i, builder_.Int32(1));
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopEnd, &afterLoop);
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);
    builder_.Bind(&afterLoop);
    GateRef valueIndex = builder_.Int32Add(slot, builder_.Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, *currentEnv, valueIndex, value);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    // StLexVarDyn will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, exceptionControl, true);
}

void SlowPathLowering::LowerCreateObjectHavingMethod(GateRef gate, GateRef glue, GateRef jsFunc)
{
    int id = RTSTUB_ID(CreateObjectHavingMethod);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef imm = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef literal = GetObjectFromConstPool(jsFunc, imm);
    GateRef env = acc_.GetValueIn(gate, 1);
    GateRef constpool = GetConstPool(jsFunc);
    GateRef newGate = builder_.CallRuntime(glue, id, { literal, env, constpool });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdHomeObject(GateRef gate, GateRef thisFunc)
{
    GateRef homeObj = GetHomeObjectFromJSFunction(thisFunc);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, homeObj, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerDefineClassWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef methodId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef literalId = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    GateRef length = acc_.GetValueIn(gate, 2);

    GateRef classTemplate = GetObjectFromConstPool(jsFunc, methodId);
    GateRef literalBuffer = GetObjectFromConstPool(jsFunc, literalId);
    GateRef lexicalEnv = acc_.GetValueIn(gate, 3);
    GateRef proto = acc_.GetValueIn(gate, 4);
    GateRef constpool = GetConstPool(jsFunc);

    DEFVAlUE(res, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());

    Label isResolved(&builder_);
    Label isNotResolved(&builder_);
    Label afterCheckResolved(&builder_);
    builder_.Branch(builder_.FunctionIsResolved(classTemplate), &isResolved, &isNotResolved);
    builder_.Bind(&isResolved);
    {
        res = builder_.CallRuntime(glue, RTSTUB_ID(CloneClassFromTemplate),
            { classTemplate, proto, lexicalEnv, constpool }, true);
        builder_.Jump(&afterCheckResolved);
    }
    builder_.Bind(&isNotResolved);
    {
        res = builder_.CallRuntime(glue, RTSTUB_ID(ResolveClass),
            { classTemplate, literalBuffer, proto, lexicalEnv, constpool }, true);
        builder_.Jump(&afterCheckResolved);
    }
    builder_.Bind(&afterCheckResolved);
    Label isException(&builder_);
    Label isNotException(&builder_);
    builder_.Branch(builder_.TaggedSpecialValueChecker(*res, JSTaggedValue::VALUE_EXCEPTION),
        &isException, &isNotException);

    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    builder_.Bind(&isException);
    {
        exceptionControl.emplace_back(builder_.GetState());
        exceptionControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&isNotException);
    GateRef newLexicalEnv = acc_.GetValueIn(gate, 3);
    builder_.SetLexicalEnvToFunction(glue, *res, newLexicalEnv);
    builder_.CallRuntime(glue, RTSTUB_ID(SetClassConstructorLength),
        { *res, builder_.TaggedTypeNGC(length) }, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    ReplaceHirToSubCfg(gate, *res, successControl, exceptionControl);
}
}  // namespace panda::ecmascript
