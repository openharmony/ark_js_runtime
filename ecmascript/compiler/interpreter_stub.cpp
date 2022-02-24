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

#include "interpreter_stub-inl.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table-inl.h"

namespace panda::ecmascript::kungfu {
#define DECLARE_ASM_HANDLER(name)                                                         \
void name##Stub::GenerateCircuit(const CompilationConfig *cfg)                            \
{                                                                                         \
    Stub::GenerateCircuit(cfg);                                                           \
    GateRef glue = PtrArgument(0);                                                        \
    GateRef pc = PtrArgument(1);                                                          \
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */                         \
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */        \
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */  \
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */                      \
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */           \
    GenerateCircuitImpl(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);   \
}                                                                                         \
void name##Stub::GenerateCircuitImpl(GateRef glue, GateRef pc, GateRef sp,                \
                                     GateRef constpool, GateRef profileTypeInfo,          \
                                     GateRef acc, GateRef hotnessCounter)

#define DISPATCH(format)                                                                  \
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,               \
             GetIntPtrConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::format)))

#define DISPATCH_WITH_ACC(format)                                                         \
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter,           \
             GetIntPtrConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::format)))

#define DISPATCH_LAST()                                                                   \
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter)           \

#define DISPATCH_LAST()                                                                   \
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter)           \

#define UPDATE_HOTNESS(_sp)                                                               \
    varHotnessCounter = Int32Add(offset, *varHotnessCounter);                             \
    Branch(Int32LessThan(*varHotnessCounter, GetInt32Constant(0)), &slowPath, &dispatch); \
    Bind(&slowPath);                                                                      \
    {                                                                                     \
        StubDescriptor *subDescriptor = GET_STUBDESCRIPTOR(UpdateHotnessCounter);         \
        varProfileTypeInfo = CallRuntime(subDescriptor, glue,                             \
            GetInt64Constant(FAST_STUB_ID(UpdateHotnessCounter)), { glue, (_sp) });       \
        varHotnessCounter = GetInt32Constant(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);  \
        Jump(&dispatch);                                                                  \
    }                                                                                     \
    Bind(&dispatch);                                                                      \

DECLARE_ASM_HANDLER(HandleLdNanPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::NAN_VALUE));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdInfinityPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::POSITIVE_INFINITY));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdUndefinedPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = GetUndefinedConstant();
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdNullPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = GetNullConstant();
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdTruePref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = ChangeInt64ToTagged(GetInt64Constant(JSTaggedValue::VALUE_TRUE));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdFalsePref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = ChangeInt64ToTagged(GetInt64Constant(JSTaggedValue::VALUE_FALSE));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleThrowDynPref)
{
    StubDescriptor *throwDyn = GET_STUBDESCRIPTOR(ThrowDyn);
    CallRuntime(throwDyn, glue, GetInt64Constant(FAST_STUB_ID(ThrowDyn)),
                {glue, acc});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleTypeOfDynPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    auto env = GetEnvironment();
    
    GateRef gConstOffset = IntPtrAdd(glue, GetIntPtrConstant(env->GetGlueOffset(JSThread::GlueID::GLOBAL_CONST)));
    GateRef booleanIndex = GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX);
    GateRef gConstUndefindStr = Load(StubMachineType::TAGGED_POINTER, gConstOffset, booleanIndex);
    DEFVARIABLE(resultRep, StubMachineType::TAGGED_POINTER, gConstUndefindStr);
    Label objIsTrue(env);
    Label objNotTrue(env);
    Label dispatch(env);
    Label defaultLabel(env);
    GateRef gConstBooleanStr = Load(
        StubMachineType::TAGGED_POINTER, gConstOffset, GetGlobalConstantString(ConstantIndex::BOOLEAN_STRING_INDEX));
    Branch(Int64Equal(*varAcc, GetInt64Constant(JSTaggedValue::VALUE_TRUE)), &objIsTrue, &objNotTrue);
    Bind(&objIsTrue);
    {
        resultRep = gConstBooleanStr;
        Jump(&dispatch);
    }
    Bind(&objNotTrue);
    {
        Label objIsFalse(env);
        Label objNotFalse(env);
        Branch(Int64Equal(*varAcc, GetInt64Constant(JSTaggedValue::VALUE_FALSE)), &objIsFalse, &objNotFalse);
        Bind(&objIsFalse);
        {
            resultRep = gConstBooleanStr;
            Jump(&dispatch);
        }
        Bind(&objNotFalse);
        {
            Label objIsNull(env);
            Label objNotNull(env);
            Branch(Int64Equal(*varAcc, GetInt64Constant(JSTaggedValue::VALUE_NULL)), &objIsNull, &objNotNull);
            Bind(&objIsNull);
            {
                resultRep = Load(
                    StubMachineType::TAGGED_POINTER, gConstOffset,
                    GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                Jump(&dispatch);
            }
            Bind(&objNotNull);
            {
                Label objIsUndefined(env);
                Label objNotUndefined(env);
                Branch(Int64Equal(*varAcc, GetInt64Constant(JSTaggedValue::VALUE_UNDEFINED)), &objIsUndefined,
                    &objNotUndefined);
                Bind(&objIsUndefined);
                {
                    resultRep = Load(StubMachineType::TAGGED_POINTER, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX));
                    Jump(&dispatch);
                }
                Bind(&objNotUndefined);
                Jump(&defaultLabel);
            }
        }
    }
    Bind(&defaultLabel);
    {
        Label objIsHeapObject(env);
        Label objNotHeapObject(env);
        Branch(TaggedIsHeapObject(*varAcc), &objIsHeapObject, &objNotHeapObject);
        Bind(&objIsHeapObject);
        {
            Label objIsString(env);
            Label objNotString(env);
            Branch(IsString(*varAcc), &objIsString, &objNotString);
            Bind(&objIsString);
            {
                resultRep = Load(
                    StubMachineType::TAGGED_POINTER, gConstOffset,
                    GetGlobalConstantString(ConstantIndex::STRING_STRING_INDEX));
                Jump(&dispatch);
            }
            Bind(&objNotString);
            {
                Label objIsSymbol(env);
                Label objNotSymbol(env);
                Branch(IsSymbol(*varAcc), &objIsSymbol, &objNotSymbol);
                Bind(&objIsSymbol);
                {
                    resultRep = Load(StubMachineType::TAGGED_POINTER, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::SYMBOL_STRING_INDEX));
                    Jump(&dispatch);
                }
                Bind(&objNotSymbol);
                {
                    Label objIsCallable(env);
                    Label objNotCallable(env);
                    Branch(IsCallable(*varAcc), &objIsCallable, &objNotCallable);
                    Bind(&objIsCallable);
                    {
                        resultRep = Load(
                            StubMachineType::TAGGED_POINTER, gConstOffset,
                            GetGlobalConstantString(ConstantIndex::FUNCTION_STRING_INDEX));
                        Jump(&dispatch);
                    }
                    Bind(&objNotCallable);
                    {
                        resultRep = Load(
                            StubMachineType::TAGGED_POINTER, gConstOffset,
                            GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                        Jump(&dispatch);
                    }
                }
            }
        }
        Bind(&objNotHeapObject);
        {
            Label objIsNum(env);
            Label objNotNum(env);
            Branch(TaggedIsNumber(*varAcc), &objIsNum, &objNotNum);
            Bind(&objIsNum);
            {
                resultRep = Load(
                    StubMachineType::TAGGED_POINTER, gConstOffset,
                    GetGlobalConstantString(ConstantIndex::NUMBER_STRING_INDEX));
                Jump(&dispatch);
            }
            Bind(&objNotNum);
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    varAcc = *resultRep;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdLexEnvDynPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef state = GetFrame(sp);
    varAcc = GetEnvFromFrame(state);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandlePopLexEnvDynPref)
{
    GateRef state = GetFrame(sp);
    GateRef currentLexEnv = GetEnvFromFrame(state);
    GateRef parentLexEnv = GetParentEnv(currentLexEnv);
    SetEnvToFrame(glue, state, parentLexEnv);
    DISPATCH(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleGetPropIteratorPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    auto env = GetEnvironment();
    
    StubDescriptor *getPropIterator = GET_STUBDESCRIPTOR(GetPropIterator);
    GateRef res = CallRuntime(getPropIterator, glue, GetInt64Constant(FAST_STUB_ID(GetPropIterator)),
                              {glue, *varAcc});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleAsyncFunctionEnterPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    auto env = GetEnvironment();
    StubDescriptor *asyncFunctionEnter = GET_STUBDESCRIPTOR(AsyncFunctionEnter);
    GateRef res = CallRuntime(asyncFunctionEnter, glue, GetInt64Constant(FAST_STUB_ID(AsyncFunctionEnter)),
                              {glue});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdHolePref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    varAcc = GetHoleConstant();
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleGetIteratorPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    auto env = GetEnvironment();
    
    Label isGeneratorObj(env);
    Label notGeneratorObj(env);
    Label dispatch(env);
    Branch(TaggedIsGeneratorObject(*varAcc), &isGeneratorObj, &notGeneratorObj);
    Bind(&isGeneratorObj);
    {
        Jump(&dispatch);
    }
    Bind(&notGeneratorObj);
    {
        StubDescriptor *getIterator = GET_STUBDESCRIPTOR(GetIterator);
        GateRef res = CallRuntime(getIterator, glue, GetInt64Constant(FAST_STUB_ID(GetIterator)),
                                  {glue, *varAcc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        varAcc = res;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleThrowThrowNotExistsPref)
{
    StubDescriptor *throwThrowNotExists = GET_STUBDESCRIPTOR(ThrowThrowNotExists);
    CallRuntime(throwThrowNotExists, glue, GetInt64Constant(FAST_STUB_ID(ThrowThrowNotExists)),
                {glue});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleThrowPatternNonCoerciblePref)
{
    StubDescriptor *throwPatternNonCoercible = GET_STUBDESCRIPTOR(ThrowPatternNonCoercible);
    CallRuntime(throwPatternNonCoercible, glue, GetInt64Constant(FAST_STUB_ID(ThrowPatternNonCoercible)),
                {glue});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleLdHomeObjectPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef thisFunc = GetFunctionFromFrame(GetFrame(sp));
    varAcc = GetHomeObjectFromJSFunction(thisFunc);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleThrowDeleteSuperPropertyPref)
{
    StubDescriptor *throwDeleteSuperProperty = GET_STUBDESCRIPTOR(ThrowDeleteSuperProperty);
    CallRuntime(throwDeleteSuperProperty, glue, GetInt64Constant(FAST_STUB_ID(ThrowDeleteSuperProperty)),
                {glue});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleDebuggerPref)
{
    DISPATCH(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleMul2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    DEFVARIABLE(result, StubMachineType::TAGGED, GetHoleConstant());
    // fast path
    result = FastMul(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        StubDescriptor *mul2Dyn = GET_STUBDESCRIPTOR(Mul2Dyn);
        result = CallRuntime(mul2Dyn, glue, GetInt64Constant(FAST_STUB_ID(Mul2Dyn)),
                             {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = *result;
            Jump(&dispatch);
        }
    }
    Bind(&notHole);
    {
        varAcc = *result;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleDiv2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    DEFVARIABLE(result, StubMachineType::TAGGED, GetHoleConstant());
    // fast path
    result = FastDiv(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        StubDescriptor *div2Dyn = GET_STUBDESCRIPTOR(Div2Dyn);
        result = CallRuntime(div2Dyn, glue, GetInt64Constant(FAST_STUB_ID(Div2Dyn)),
                             {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = *result;
            Jump(&dispatch);
        }
    }
    Bind(&notHole);
    {
        varAcc = *result;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleMod2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    DEFVARIABLE(result, StubMachineType::TAGGED, GetHoleConstant());
    // fast path
    result = FastMod(glue, left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        StubDescriptor *mod2Dyn = GET_STUBDESCRIPTOR(Mod2Dyn);
        result = CallRuntime(mod2Dyn, glue, GetInt64Constant(FAST_STUB_ID(Mod2Dyn)),
                             {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = *result;
            Jump(&dispatch);
        }
    }
    Bind(&notHole);
    {
        varAcc = *result;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleEqDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    // fast path
    DEFVARIABLE(result, StubMachineType::TAGGED, GetHoleConstant());
    result = FastEqual(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        StubDescriptor *eqDyn = GET_STUBDESCRIPTOR(EqDyn);
        result = CallRuntime(eqDyn, glue, GetInt64Constant(FAST_STUB_ID(EqDyn)),
                             {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = *result;
            Jump(&dispatch);
        }
    }
    Bind(&notHole);
    {
        varAcc = *result;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleNotEqDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    // fast path
    DEFVARIABLE(result, StubMachineType::TAGGED, GetHoleConstant());
    result = FastEqual(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        // slow path
        StubDescriptor *notEqDyn = GET_STUBDESCRIPTOR(NotEqDyn);
        result = CallRuntime(notEqDyn, glue, GetInt64Constant(FAST_STUB_ID(NotEqDyn)),
                             {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = *result;
            Jump(&dispatch);
        }
    }
    Bind(&notHole);
    {
        Label resultIsTrue(env);
        Label resultNotTrue(env);
        Branch(TaggedIsTrue(*result), &resultIsTrue, &resultNotTrue);
        Bind(&resultIsTrue);
        {
            varAcc = ChangeInt64ToTagged(TaggedFalse());
            Jump(&dispatch);
        }
        Bind(&resultNotTrue);
        {
            varAcc = ChangeInt64ToTagged(TaggedTrue());
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleLessDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef right = acc;
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftLessRight(env);
    Label leftNotLessRight(env);
    Label slowPath(env);
    Label dispatch(env);
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32LessThan(intLeft, intRight), &leftLessRight, &leftNotLessRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, StubMachineType::FLOAT64, GetDoubleConstant(0));
                DEFVARIABLE(doubleRight, StubMachineType::FLOAT64, GetDoubleConstant(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleLessThan(*doubleLeft, *doubleRight), &leftLessRight, &leftNotLessRight);
                }
            }
        }
    }
    Bind(&leftLessRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedTrue());
        Jump(&dispatch);
    }
    Bind(&leftNotLessRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedFalse());
        Jump(&dispatch);
    }
    Bind(&slowPath);
    {
        // slow path
        StubDescriptor *lessDyn = GET_STUBDESCRIPTOR(LessDyn);
        GateRef result = CallRuntime(lessDyn, glue, GetInt64Constant(FAST_STUB_ID(LessDyn)),
                                     {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = result;
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleLessEqDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef right = acc;
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftLessEqRight(env);
    Label leftNotLessEqRight(env);
    Label slowPath(env);
    Label dispatch(env);
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32LessThanOrEqual(intLeft, intRight), &leftLessEqRight, &leftNotLessEqRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, StubMachineType::FLOAT64, GetDoubleConstant(0));
                DEFVARIABLE(doubleRight, StubMachineType::FLOAT64, GetDoubleConstant(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleLessThanOrEqual(*doubleLeft, *doubleRight), &leftLessEqRight, &leftNotLessEqRight);
                }
            }
        }
    }
    Bind(&leftLessEqRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedTrue());
        Jump(&dispatch);
    }
    Bind(&leftNotLessEqRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedFalse());
        Jump(&dispatch);
    }
    Bind(&slowPath);
    {
        // slow path
        StubDescriptor *lessEqDyn = GET_STUBDESCRIPTOR(LessEqDyn);
        GateRef result = CallRuntime(lessEqDyn, glue, GetInt64Constant(FAST_STUB_ID(LessEqDyn)),
                                     {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = result;
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleGreaterDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef right = acc;
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftGreaterRight(env);
    Label leftNotGreaterRight(env);
    Label slowPath(env);
    Label dispatch(env);
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32GreaterThan(intLeft, intRight), &leftGreaterRight, &leftNotGreaterRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, StubMachineType::FLOAT64, GetDoubleConstant(0));
                DEFVARIABLE(doubleRight, StubMachineType::FLOAT64, GetDoubleConstant(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleGreaterThan(*doubleLeft, *doubleRight), &leftGreaterRight, &leftNotGreaterRight);
                }
            }
        }
    }
    Bind(&leftGreaterRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedTrue());
        Jump(&dispatch);
    }
    Bind(&leftNotGreaterRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedFalse());
        Jump(&dispatch);
    }
    Bind(&slowPath);
    {
        // slow path
        StubDescriptor *greaterDyn = GET_STUBDESCRIPTOR(GreaterDyn);
        GateRef result = CallRuntime(greaterDyn, glue, GetInt64Constant(FAST_STUB_ID(GreaterDyn)),
                                     {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = result;
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleGreaterEqDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef right = acc;
    Label leftIsInt(env);
    Label leftOrRightNotInt(env);
    Label leftGreaterEqRight(env);
    Label leftNotGreaterEQRight(env);
    Label slowPath(env);
    Label dispatch(env);
    Branch(TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        Bind(&rightIsInt);
        {
            GateRef intLeft = TaggedGetInt(left);
            GateRef intRight = TaggedGetInt(right);
            Branch(Int32GreaterThanOrEqual(intLeft, intRight), &leftGreaterEqRight, &leftNotGreaterEQRight);
        }
    }
    Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &slowPath);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &slowPath);
            Bind(&rightIsNumber);
            {
                // fast path
                DEFVARIABLE(doubleLeft, StubMachineType::FLOAT64, GetDoubleConstant(0));
                DEFVARIABLE(doubleRight, StubMachineType::FLOAT64, GetDoubleConstant(0));
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Label exit1(env);
                Label exit2(env);
                Label rightIsInt1(env);
                Label rightNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedGetInt(left));
                    Jump(&exit1);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    Jump(&exit1);
                }
                Bind(&exit1);
                {
                    Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                }
                Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedGetInt(right));
                    Jump(&exit2);
                }
                Bind(&rightNotInt1);
                {
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&exit2);
                }
                Bind(&exit2);
                {
                    Branch(DoubleGreaterThanOrEqual(*doubleLeft, *doubleRight),
                        &leftGreaterEqRight, &leftNotGreaterEQRight);
                }
            }
        }
    }
    Bind(&leftGreaterEqRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedTrue());
        Jump(&dispatch);
    }
    Bind(&leftNotGreaterEQRight);
    {
        varAcc = ChangeInt64ToTagged(TaggedFalse());
        Jump(&dispatch);
    }
    Bind(&slowPath);
    {
        // slow path
        StubDescriptor *greaterEqDyn = GET_STUBDESCRIPTOR(GreaterEqDyn);
        GateRef result = CallRuntime(greaterEqDyn, glue, GetInt64Constant(FAST_STUB_ID(GreaterEqDyn)),
                                     {glue, left, acc});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            varAcc = result;
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(AsmInterpreterEntry)
{
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetIntPtrConstant(0));
}

DECLARE_ASM_HANDLER(SingleStepDebugging)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, StubMachineType::NATIVE_POINTER, pc);
    DEFVARIABLE(varSp, StubMachineType::NATIVE_POINTER, sp);
    DEFVARIABLE(varConstpool, StubMachineType::TAGGED_POINTER, constpool);
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    auto stubDescriptor = GET_STUBDESCRIPTOR(JumpToCInterpreter);
    varPc = CallRuntime(stubDescriptor, glue, GetInt64Constant(FAST_STUB_ID(JumpToCInterpreter)), {
        glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter
    });
    Label shouldReturn(env);
    Label shouldContinue(env);

    Branch(IntPtrEqual(*varPc, GetIntPtrConstant(0)), &shouldReturn, &shouldContinue);
    Bind(&shouldReturn);
    {
        Return();
    }
    Bind(&shouldContinue);
    {
        varSp = GetCurrentSpFrame(glue);
        GateRef frame = GetFrame(*varSp);
        varAcc = GetAccFromFrame(frame);

        GateRef function = GetFunctionFromFrame(frame);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        varConstpool = GetConstpoolFromFunction(function);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(StubMachineType::INT32, method,
                                 GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET));
    }
    Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
             *varHotnessCounter, GetIntPtrConstant(0));
}

DECLARE_ASM_HANDLER(HandleLdaDynV8)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef vsrc = ReadInst8_0(pc);
    varAcc = GetVregValue(sp, ZExtInt8ToPtr(vsrc));
    DISPATCH_WITH_ACC(V8);
}

DECLARE_ASM_HANDLER(HandleStaDynV8)
{
    GateRef vdst = ReadInst8_0(pc);
    SetVregValue(glue, sp, ZExtInt8ToPtr(vdst), acc);
    DISPATCH(V8);
}

DECLARE_ASM_HANDLER(HandleJmpImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned8_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    UPDATE_HOTNESS(sp);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleJmpImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned16_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    UPDATE_HOTNESS(sp);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleJmpImm32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned32_0(pc);
    Label dispatch(env);
    Label slowPath(env);
    UPDATE_HOTNESS(sp);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleLdLexVarDynPrefImm4Imm4)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, StubMachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, StubMachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    varAcc = variable;

    DISPATCH_WITH_ACC(PREF_IMM4_IMM4);
}

DECLARE_ASM_HANDLER(HandleLdLexVarDynPrefImm8Imm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, StubMachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, StubMachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    varAcc = variable;
    DISPATCH_WITH_ACC(PREF_IMM8_IMM8);
}

DECLARE_ASM_HANDLER(HandleLdLexVarDynPrefImm16Imm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, StubMachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, StubMachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    varAcc = variable;
    DISPATCH_WITH_ACC(PREF_IMM16_IMM16);
}

DECLARE_ASM_HANDLER(HandleStLexVarDynPrefImm4Imm4V8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));
    GateRef v0 = ReadInst8_2(pc);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, StubMachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, StubMachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(PREF_IMM4_IMM4_V8);
}

DECLARE_ASM_HANDLER(HandleStLexVarDynPrefImm8Imm8V8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));
    GateRef v0 = ReadInst8_3(pc);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, StubMachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, StubMachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(PREF_IMM8_IMM8_V8);
}

DECLARE_ASM_HANDLER(HandleStLexVarDynPrefImm16Imm16V8)
{
    auto env = GetEnvironment();
    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));
    GateRef v0 = ReadInst8_5(pc);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, StubMachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, StubMachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    DISPATCH(PREF_IMM16_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleIncDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueOverflow(env);
        Label valueNoOverflow(env);
        Branch(Int32Equal(valueInt, GetInt32Constant(INT32_MAX)), &valueOverflow, &valueNoOverflow);
        Bind(&valueOverflow);
        {
            GateRef valueDoubleFromInt = ChangeInt32ToFloat64(valueInt);
            varAcc = DoubleBuildTaggedWithNoGC(DoubleAdd(valueDoubleFromInt, GetDoubleConstant(1.0)));
            Jump(&accDispatch);
        }
        Bind(&valueNoOverflow);
        {
            varAcc = IntBuildTaggedWithNoGC(Int32Add(valueInt, GetInt32Constant(1)));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    Label valueIsDouble(env);
    Label valueNotDouble(env);
    Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
    Bind(&valueIsDouble);
    {
        GateRef valueDouble = TaggedCastToDouble(value);
        varAcc = DoubleBuildTaggedWithNoGC(DoubleAdd(valueDouble, GetDoubleConstant(1.0)));
        Jump(&accDispatch);
    }
    Bind(&valueNotDouble);
    {
        // slow path
        StubDescriptor *incDyn = GET_STUBDESCRIPTOR(IncDyn);
        GateRef result = CallRuntime(incDyn, glue, GetInt64Constant(FAST_STUB_ID(IncDyn)),
                                     {glue, value});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        varAcc = result;
        Jump(&accDispatch);
    }

    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleDecDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueOverflow(env);
        Label valueNoOverflow(env);
        Branch(Int32Equal(valueInt, GetInt32Constant(INT32_MIN)), &valueOverflow, &valueNoOverflow);
        Bind(&valueOverflow);
        {
            GateRef valueDoubleFromInt = ChangeInt32ToFloat64(valueInt);
            varAcc = DoubleBuildTaggedWithNoGC(DoubleSub(valueDoubleFromInt, GetDoubleConstant(1.0)));
            Jump(&accDispatch);
        }
        Bind(&valueNoOverflow);
        {
            varAcc = IntBuildTaggedWithNoGC(Int32Sub(valueInt, GetInt32Constant(1)));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    Label valueIsDouble(env);
    Label valueNotDouble(env);
    Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
    Bind(&valueIsDouble);
    {
        GateRef valueDouble = TaggedCastToDouble(value);
        varAcc = DoubleBuildTaggedWithNoGC(DoubleSub(valueDouble, GetDoubleConstant(1.0)));
        Jump(&accDispatch);
    }
    Bind(&valueNotDouble);
    {
        // slow path
        StubDescriptor *decDyn = GET_STUBDESCRIPTOR(DecDyn);
        GateRef result = CallRuntime(decDyn, glue, GetInt64Constant(FAST_STUB_ID(DecDyn)),
                                     {glue, value});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        varAcc = result;
        Jump(&accDispatch);
    }

    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleExpDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef base = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *expDyn = GET_STUBDESCRIPTOR(ExpDyn);
    GateRef result = CallRuntime(expDyn, glue, GetInt64Constant(FAST_STUB_ID(ExpDyn)),
                                 {glue, base, acc}); // acc is exponent
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleIsInDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *isInDyn = GET_STUBDESCRIPTOR(IsInDyn);
    GateRef result = CallRuntime(isInDyn, glue, GetInt64Constant(FAST_STUB_ID(IsInDyn)),
                                 {glue, prop, acc}); // acc is obj
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleInstanceOfDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *instanceOfDyn = GET_STUBDESCRIPTOR(InstanceOfDyn);
    GateRef result = CallRuntime(instanceOfDyn, glue, GetInt64Constant(FAST_STUB_ID(InstanceOfDyn)),
                                 {glue, obj, acc}); // acc is target
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleStrictNotEqDynPrefV8)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *fastStrictNotEqual = GET_STUBDESCRIPTOR(FastStrictNotEqual);
    GateRef result = CallRuntime(fastStrictNotEqual, glue, GetInt64Constant(FAST_STUB_ID(FastStrictNotEqual)),
                                 {left, acc}); // acc is right
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleStrictEqDynPrefV8)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *fastStrictEqual = GET_STUBDESCRIPTOR(FastStrictEqual);
    GateRef result = CallRuntime(fastStrictEqual, glue, GetInt64Constant(FAST_STUB_ID(FastStrictEqual)),
                                 {left, acc}); // acc is right
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleResumeGeneratorPrefV8)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef vs = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(vs));
    GateRef resumeResultOffset = GetIntPtrConstant(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
    varAcc = Load(StubMachineType::TAGGED, obj, resumeResultOffset);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleGetResumeModePrefV8)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef vs = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(vs));
    varAcc = ChangeInt64ToTagged(ZExtInt32ToInt64(GetGeneratorObjectResumeMode(obj)));
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleCreateGeneratorObjPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef genFunc = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *createGeneratorObj = GET_STUBDESCRIPTOR(CreateGeneratorObj);
    GateRef result = CallRuntime(createGeneratorObj, glue, GetInt64Constant(FAST_STUB_ID(CreateGeneratorObj)),
                                 {glue, genFunc});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleThrowConstAssignmentPrefV8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *throwConstAssignment = GET_STUBDESCRIPTOR(ThrowConstAssignment);
    CallRuntime(throwConstAssignment, glue, GetInt64Constant(FAST_STUB_ID(ThrowConstAssignment)),
                {glue, value});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleGetTemplateObjectPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef literal = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *getTemplateObject = GET_STUBDESCRIPTOR(GetTemplateObject);
    GateRef result = CallRuntime(getTemplateObject, glue, GetInt64Constant(FAST_STUB_ID(GetTemplateObject)),
                                 {glue, literal});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleGetNextPropNamePrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *getNextPropName = GET_STUBDESCRIPTOR(GetNextPropName);
    GateRef result = CallRuntime(getNextPropName, glue, GetInt64Constant(FAST_STUB_ID(GetNextPropName)),
                                 {glue, iter});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleThrowIfNotObjectPrefV8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Branch(IsEcmaObject(value), &isEcmaObject, &notEcmaObject);
    Bind(&isEcmaObject);
    {
        DISPATCH(PREF_V8);
    }
    Bind(&notEcmaObject);
    StubDescriptor *throwIfNotObject = GET_STUBDESCRIPTOR(ThrowIfNotObject);
    CallRuntime(throwIfNotObject, glue, GetInt64Constant(FAST_STUB_ID(ThrowIfNotObject)),
                {glue});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleIterNextPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *iterNext = GET_STUBDESCRIPTOR(IterNext);
    GateRef result = CallRuntime(iterNext, glue, GetInt64Constant(FAST_STUB_ID(IterNext)),
                                 {glue, iter});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleCloseIteratorPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *closeIterator = GET_STUBDESCRIPTOR(CloseIterator);
    GateRef result = CallRuntime(closeIterator, glue, GetInt64Constant(FAST_STUB_ID(CloseIterator)),
                                 {glue, iter});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleCopyModulePrefV8)
{
    GateRef v0 = ReadInst8_1(pc);
    GateRef srcModule = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *copyModule = GET_STUBDESCRIPTOR(CopyModule);
    CallRuntime(copyModule, glue, GetInt64Constant(FAST_STUB_ID(CopyModule)),
                {glue, srcModule});
    DISPATCH(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleSuperCallSpreadPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef array = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *superCallSpread = GET_STUBDESCRIPTOR(SuperCallSpread);
    GateRef result = CallRuntime(superCallSpread, glue, GetInt64Constant(FAST_STUB_ID(SuperCallSpread)),
                                 {glue, acc, sp, array}); // acc is thisFunc, sp for newTarget
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleDelObjPropPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *delObjProp = GET_STUBDESCRIPTOR(DelObjProp);
    GateRef result = CallRuntime(delObjProp, glue, GetInt64Constant(FAST_STUB_ID(DelObjProp)),
                                 {glue, obj, prop});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleNewObjSpreadDynPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef newTarget = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *newObjSpreadDyn = GET_STUBDESCRIPTOR(NewObjSpreadDyn);
    GateRef result = CallRuntime(newObjSpreadDyn, glue, GetInt64Constant(FAST_STUB_ID(NewObjSpreadDyn)),
                                 {glue, func, newTarget, acc}); // acc is array
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleCreateIterResultObjPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef flag = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *createIterResultObj = GET_STUBDESCRIPTOR(CreateIterResultObj);
    GateRef result = CallRuntime(createIterResultObj, glue, GetInt64Constant(FAST_STUB_ID(CreateIterResultObj)),
                                 {glue, value, flag});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleAsyncFunctionAwaitUncaughtPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *asyncFunctionAwaitUncaught = GET_STUBDESCRIPTOR(AsyncFunctionAwaitUncaught);
    GateRef result = CallRuntime(asyncFunctionAwaitUncaught, glue,
                                 GetInt64Constant(FAST_STUB_ID(AsyncFunctionAwaitUncaught)),
                                 {glue, asyncFuncObj, value});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleThrowUndefinedIfHolePrefV8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef hole = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(hole), &isHole, &notHole);
    Bind(&notHole);
    {
        DISPATCH(PREF_V8_V8);
    }
    Bind(&isHole);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    // assert obj.IsString()
    StubDescriptor *throwUndefinedIfHole = GET_STUBDESCRIPTOR(ThrowUndefinedIfHole);
    CallRuntime(throwUndefinedIfHole, glue, GetInt64Constant(FAST_STUB_ID(ThrowUndefinedIfHole)),
                {glue, obj});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleCopyDataPropertiesPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef dst = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef src = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *copyDataProperties = GET_STUBDESCRIPTOR(CopyDataProperties);
    GateRef result = CallRuntime(copyDataProperties, glue, GetInt64Constant(FAST_STUB_ID(CopyDataProperties)),
                                 {glue, dst, src});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleStArraySpreadPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef dst = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef index = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *stArraySpread = GET_STUBDESCRIPTOR(StArraySpread);
    GateRef result = CallRuntime(stArraySpread, glue, GetInt64Constant(FAST_STUB_ID(StArraySpread)),
                                 {glue, dst, index, acc}); // acc is res
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleGetIteratorNextPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef method = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *getIteratorNext = GET_STUBDESCRIPTOR(GetIteratorNext);
    GateRef result = CallRuntime(getIteratorNext, glue, GetInt64Constant(FAST_STUB_ID(GetIteratorNext)),
                                 {glue, obj, method});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleSetObjectWithProtoPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *setObjectWithProto = GET_STUBDESCRIPTOR(SetObjectWithProto);
    GateRef result = CallRuntime(setObjectWithProto, glue, GetInt64Constant(FAST_STUB_ID(SetObjectWithProto)),
                                 {glue, proto, obj});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleLdObjByValuePrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    Label receiverIsHeapObject(env);
    Label slowPath(env);
    Label isException(env);
    Label accDispatch(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label loadElement(env);
                Label tryPoly(env);
                GateRef secondValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo,
                    Int32Add(slotId, GetInt32Constant(1)));
                DEFVARIABLE(cachedHandler, StubMachineType::TAGGED, secondValue);
                GateRef hclass = LoadHClass(receiver);
                Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
                       &loadElement, &tryPoly);
                Bind(&loadElement);
                {
                    GateRef result = LoadElement(receiver, propKey);
                    Label notHole(env);
                    Branch(TaggedIsHole(result), &slowPath, &notHole);
                    Bind(&notHole);
                    Label notException(env);
                    Branch(TaggedIsException(result), &isException, &notException);
                    Bind(&notException);
                    varAcc = result;
                    Jump(&accDispatch);
                }
                Bind(&tryPoly);
                {
                    Label firstIsKey(env);
                    Branch(Int64Equal(firstValue, propKey), &firstIsKey, &slowPath);
                    Bind(&firstIsKey);
                    Label loadWithHandler(env);
                    cachedHandler = CheckPolyHClass(secondValue, hclass);
                    Branch(TaggedIsHole(*cachedHandler), &slowPath, &loadWithHandler);
                    Bind(&loadWithHandler);
                    GateRef result = LoadICWithHandler(glue, receiver, receiver, *cachedHandler);
                    Label notHole(env);
                    Branch(TaggedIsHole(result), &slowPath, &notHole);
                    Bind(&notHole);
                    Label notException(env);
                    Branch(TaggedIsException(result), &isException, &notException);
                    Bind(&notException);
                    varAcc = result;
                    Jump(&accDispatch);
                }
            }
        }
        Bind(&tryFastPath);
        {
            StubDescriptor *getPropertyByValue = GET_STUBDESCRIPTOR(GetPropertyByValue);
            GateRef result = CallRuntime(getPropertyByValue, glue, GetInt64Constant(FAST_STUB_ID(GetPropertyByValue)),
                                         {glue, receiver, propKey});
            Label notHole(env);
            Branch(TaggedIsHole(result), &slowPath, &notHole);
            Bind(&notHole);
            Label notException(env);
            Branch(TaggedIsException(result), &isException, &notException);
            Bind(&notException);
            varAcc = result;
            Jump(&accDispatch);
        }
    }
    Bind(&slowPath);
    {
        StubDescriptor *loadICByValue = GET_STUBDESCRIPTOR(LoadICByValue);
        GateRef result = CallRuntime(loadICByValue, glue, GetInt64Constant(FAST_STUB_ID(LoadICByValue)),
                                     {glue, profileTypeInfo, receiver, propKey, slotId});
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&notException);
        varAcc = result;
        Jump(&accDispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleStObjByValuePrefV8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    // slotId = READ_INST_8_0()
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    Label receiverIsHeapObject(env);
    Label slowPath(env);
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label storeElement(env);
                Label tryPoly(env);
                GateRef secondValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo,
                    Int32Add(slotId, GetInt32Constant(1)));
                DEFVARIABLE(cachedHandler, StubMachineType::TAGGED, secondValue);
                GateRef hclass = LoadHClass(receiver);
                Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
                       &storeElement, &tryPoly);
                Bind(&storeElement);
                {
                    // acc is value
                    GateRef result = ICStoreElement(glue, receiver, propKey, acc, secondValue);
                    Label notHole(env);
                    Branch(TaggedIsHole(result), &slowPath, &notHole);
                    Bind(&notHole);
                    Branch(TaggedIsException(result), &isException, &notException);
                }
                Bind(&tryPoly);
                {
                    Label firstIsKey(env);
                    Branch(Int64Equal(firstValue, propKey), &firstIsKey, &slowPath);
                    Bind(&firstIsKey);
                    Label loadWithHandler(env);
                    cachedHandler = CheckPolyHClass(secondValue, hclass);
                    Branch(TaggedIsHole(*cachedHandler), &slowPath, &loadWithHandler);
                    Bind(&loadWithHandler);
                    GateRef result = StoreICWithHandler(glue, receiver, receiver, acc, *cachedHandler); // acc is value
                    Label notHole(env);
                    Branch(TaggedIsHole(result), &slowPath, &notHole);
                    Bind(&notHole);
                    Branch(TaggedIsException(result), &isException, &notException);
                }
            }
        }
        Bind(&tryFastPath);
        {
            StubDescriptor *setPropertyByValue = GET_STUBDESCRIPTOR(SetPropertyByValue);
            GateRef result = CallRuntime(setPropertyByValue, glue, GetInt64Constant(FAST_STUB_ID(SetPropertyByValue)),
                                         {glue, receiver, propKey, acc}); // acc is value
            Label notHole(env);
            Branch(TaggedIsHole(result), &slowPath, &notHole);
            Bind(&notHole);
            Branch(TaggedIsException(result), &isException, &notException);
        }
    }
    Bind(&slowPath);
    {
        StubDescriptor *storeICByValue = GET_STUBDESCRIPTOR(StoreICByValue);
        GateRef result = CallRuntime(storeICByValue, glue, GetInt64Constant(FAST_STUB_ID(StoreICByValue)),
                                     {glue, profileTypeInfo, receiver, propKey, acc, slotId}); // acc is value
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    DISPATCH(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleStOwnByValuePrefV8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    Label isHeapObject(env);
    Label slowPath(env);
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        StubDescriptor *setPropertyByValue = GET_STUBDESCRIPTOR(SetPropertyByValue);
        GateRef result = CallRuntime(setPropertyByValue, glue, GetInt64Constant(FAST_STUB_ID(SetPropertyByValue)),
                                     {glue, receiver, propKey, acc}); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&slowPath);
    {
        StubDescriptor *stOwnByValue = GET_STUBDESCRIPTOR(StOwnByValue);
        GateRef result = CallRuntime(stOwnByValue, glue, GetInt64Constant(FAST_STUB_ID(StOwnByValue)),
                                     {glue, receiver, propKey, acc}); // acc is value
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    DISPATCH(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleLdSuperByValuePrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *ldSuperByValue = GET_STUBDESCRIPTOR(LdSuperByValue);
    GateRef result = CallRuntime(ldSuperByValue, glue, GetInt64Constant(FAST_STUB_ID(LdSuperByValue)),
                                 {glue, receiver, propKey, sp}); // sp for thisFunc
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleStSuperByValuePrefV8V8)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *stSuperByValue = GET_STUBDESCRIPTOR(StSuperByValue);
    GateRef result = CallRuntime(stSuperByValue, glue, GetInt64Constant(FAST_STUB_ID(StSuperByValue)),
                                 {glue, receiver, propKey, acc, sp}); // acc is value, sp for thisFunc
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    DISPATCH(PREF_V8_V8);
}

DECLARE_ASM_HANDLER(HandleLdSuperByNamePrefId32V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    Label isException(env);
    Label dispatch(env);

    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ReadInst8_5(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);

    StubDescriptor *ldSuperByValue = GET_STUBDESCRIPTOR(LdSuperByValue);
    GateRef result = CallRuntime(ldSuperByValue, glue, GetInt64Constant(FAST_STUB_ID(LdSuperByValue)),
                                 {glue, receiver, propKey, sp}); // sp for thisFunc
    Branch(TaggedIsException(result), &isException, &dispatch);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&dispatch);
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_ID32_V8);
}

DECLARE_ASM_HANDLER(HandleStSuperByNamePrefId32V8)
{
    auto env = GetEnvironment();

    Label isException(env);
    Label dispatch(env);

    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ReadInst8_5(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);

    StubDescriptor *stSuperByValue = GET_STUBDESCRIPTOR(StSuperByValue);
    GateRef result = CallRuntime(stSuperByValue, glue, GetInt64Constant(FAST_STUB_ID(StSuperByValue)),
                                 {glue, receiver, propKey, acc, sp}); // sp for thisFunc
    Branch(TaggedIsException(result), &isException, &dispatch);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    DISPATCH(PREF_ID32_V8);
}

DECLARE_ASM_HANDLER(HandleLdObjByIndexPrefV8Imm32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef index = ReadInst32_2(pc);
    Label fastPath(env);
    Label slowPath(env);
    Label isException(env);
    Label accDispatch(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        StubDescriptor *getPropertyByIndex = GET_STUBDESCRIPTOR(GetPropertyByIndex);
        GateRef result = CallRuntime(getPropertyByIndex, glue, GetInt64Constant(FAST_STUB_ID(GetPropertyByIndex)),
                                     {glue, receiver, index});
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&notException);
        varAcc = result;
        Jump(&accDispatch);
    }
    Bind(&slowPath);
    {
        StubDescriptor *ldObjByIndex = GET_STUBDESCRIPTOR(LdObjByIndex);
        GateRef result = CallRuntime(ldObjByIndex, glue, GetInt64Constant(FAST_STUB_ID(LdObjByIndex)),
                                     {glue, receiver, index, FalseConstant(), GetUndefinedConstant()});
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&notException);
        varAcc = result;
        Jump(&accDispatch);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8_IMM32);
}

DECLARE_ASM_HANDLER(HandleStObjByIndexPrefV8Imm32)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef index = ReadInst32_2(pc);
    Label fastPath(env);
    Label slowPath(env);
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        StubDescriptor *setPropertyByIndex = GET_STUBDESCRIPTOR(SetPropertyByIndex);
        GateRef result = CallRuntime(setPropertyByIndex, glue, GetInt64Constant(FAST_STUB_ID(SetPropertyByIndex)),
                                     {glue, receiver, index, acc}); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&slowPath);
    {
        StubDescriptor *stObjByIndex = GET_STUBDESCRIPTOR(StObjByIndex);
        GateRef result = CallRuntime(stObjByIndex, glue, GetInt64Constant(FAST_STUB_ID(StObjByIndex)),
                                     {glue, receiver, index, acc}); // acc is value
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    DISPATCH(PREF_V8_IMM32);
}

DECLARE_ASM_HANDLER(HandleStOwnByIndexPrefV8Imm32)
{
    auto env = GetEnvironment();

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef index = ReadInst32_2(pc);
    Label isHeapObject(env);
    Label slowPath(env);
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    Label notClassConstructor(env);
    Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    Bind(&notClassConstructor);
    Label notClassPrototype(env);
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        StubDescriptor *setPropertyByIndex = GET_STUBDESCRIPTOR(SetPropertyByIndex);
        GateRef result = CallRuntime(setPropertyByIndex, glue, GetInt64Constant(FAST_STUB_ID(SetPropertyByIndex)),
                                     {glue, receiver, index, acc}); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&slowPath);
    {
        StubDescriptor *stOwnByIndex = GET_STUBDESCRIPTOR(StOwnByIndex);
        GateRef result = CallRuntime(stOwnByIndex, glue, GetInt64Constant(FAST_STUB_ID(StOwnByIndex)),
                                     {glue, receiver, index, acc}); // acc is value
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    DISPATCH(PREF_V8_IMM32);
}

DECLARE_ASM_HANDLER(HandleStConstToGlobalRecordPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
    StubDescriptor *stGlobalRecord = GET_STUBDESCRIPTOR(StGlobalRecord);
    GateRef result = CallRuntime(stGlobalRecord, glue, GetInt64Constant(FAST_STUB_ID(StGlobalRecord)),
                                 { glue, propKey, *varAcc, TrueConstant() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&notException);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStLetToGlobalRecordPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
    StubDescriptor *stGlobalRecord = GET_STUBDESCRIPTOR(StGlobalRecord);
    GateRef result = CallRuntime(stGlobalRecord, glue, GetInt64Constant(FAST_STUB_ID(StGlobalRecord)),
                                 { glue, propKey, *varAcc, FalseConstant() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&notException);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStClassToGlobalRecordPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
    StubDescriptor *stGlobalRecord = GET_STUBDESCRIPTOR(StGlobalRecord);
    GateRef result = CallRuntime(stGlobalRecord, glue, GetInt64Constant(FAST_STUB_ID(StGlobalRecord)),
                                 { glue, propKey, *varAcc, FalseConstant() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&notException);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleNegDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef vsrc = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(vsrc));

    Label valueIsInt(env);
    Label valueNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueIsZero(env);
        Label valueNotZero(env);
        Branch(Int32Equal(valueInt, GetInt32Constant(0)), &valueIsZero, &valueNotZero);
        Bind(&valueIsZero);
        {
            // Format::PREF_V8： size = 3
            varAcc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(-0.0));
            Jump(&accDispatch);
        }
        Bind(&valueNotZero);
        {
            varAcc = IntBuildTaggedWithNoGC(Int32Sub(GetInt32Constant(0), valueInt));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Label valueNotDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = TaggedCastToDouble(value);
            varAcc = DoubleBuildTaggedWithNoGC(DoubleSub(GetDoubleConstant(0), valueDouble));
            Jump(&accDispatch);
        }
        Label isException(env);
        Label notException(env);
        Bind(&valueNotDouble);
        {
            // slow path
            StubDescriptor *NegDyn = GET_STUBDESCRIPTOR(NegDyn);
            GateRef result = CallRuntime(NegDyn, glue, GetInt64Constant(FAST_STUB_ID(NegDyn)),
                                         {glue, value});
            Branch(TaggedIsException(result), &isException, &notException);
            Bind(&isException);
            {
                DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
            }
            Bind(&notException);
            varAcc = result;
            Jump(&accDispatch);
        }
    }

    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleNotDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef vsrc = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(vsrc));
    DEFVARIABLE(number, StubMachineType::INT32, GetInt32Constant(0));
    Label numberIsInt(env);
    Label numberNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &numberIsInt, &numberNotInt);
    Bind(&numberIsInt);
    {
        number = TaggedCastToInt32(value);
        varAcc = IntBuildTaggedWithNoGC(Int32Not(*number));
        Jump(&accDispatch);
    }
    Bind(&numberNotInt);
    {
        Label numberIsDouble(env);
        Label numberNotDouble(env);
        Branch(TaggedIsDouble(value), &numberIsDouble, &numberNotDouble);
        Bind(&numberIsDouble);
        {
            GateRef valueDouble = TaggedCastToDouble(value);
            number = ChangeFloat64ToInt32(valueDouble);
            varAcc = IntBuildTaggedWithNoGC(Int32Not(*number));
            Jump(&accDispatch);
        }
        Bind(&numberNotDouble);
        {
            // slow path
            StubDescriptor *NotDyn = GET_STUBDESCRIPTOR(NotDyn);
            GateRef result = CallRuntime(NotDyn, glue, GetInt64Constant(FAST_STUB_ID(NotDyn)),
                                        {glue, value});
            Label isException(env);
            Label notException(env);
            Branch(TaggedIsException(result), &isException, &notException);
            Bind(&isException);
            {
                DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
            }
            Bind(&notException);
            varAcc = result;
            Jump(&accDispatch);
        }
    }

    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleAnd2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *ChangeTwoInt32AndToJSTaggedValue = GET_STUBDESCRIPTOR(ChangeTwoInt32AndToJSTaggedValue);
        GateRef taggedNumber = CallRuntime(ChangeTwoInt32AndToJSTaggedValue, glue,
            GetInt64Constant(FAST_STUB_ID(ChangeTwoInt32AndToJSTaggedValue)), {glue, left, right});

        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef ret = Int32And(*opNumber0, *opNumber1);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleOr2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *ChangeTwoInt32OrToJSTaggedValue = GET_STUBDESCRIPTOR(ChangeTwoInt32OrToJSTaggedValue);
        GateRef taggedNumber = CallRuntime(ChangeTwoInt32OrToJSTaggedValue, glue,
            GetInt64Constant(FAST_STUB_ID(ChangeTwoInt32OrToJSTaggedValue)), {glue, left, right});

        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef ret = Int32Or(*opNumber0, *opNumber1);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleXOr2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *ChangeTwoInt32XorToJSTaggedValue = GET_STUBDESCRIPTOR(ChangeTwoInt32XorToJSTaggedValue);
        GateRef taggedNumber = CallRuntime(ChangeTwoInt32XorToJSTaggedValue, glue,
            GetInt64Constant(FAST_STUB_ID(ChangeTwoInt32XorToJSTaggedValue)), {glue, left, right});

        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef ret = Int32Xor(*opNumber0, *opNumber1);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleAshr2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *ChangeTwoUint32AndToJSTaggedValue = GET_STUBDESCRIPTOR(ChangeTwoUint32AndToJSTaggedValue);
        GateRef taggedNumber = CallRuntime(ChangeTwoUint32AndToJSTaggedValue, glue,
            GetInt64Constant(FAST_STUB_ID(ChangeTwoUint32AndToJSTaggedValue)), {glue, left, right});

        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef shift = Int32And(*opNumber1, GetInt32Constant(0x1f));
        GateRef ret = UInt32LSR(*opNumber0, shift);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleShr2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *ChangeUintAndIntShrToJSTaggedValue = GET_STUBDESCRIPTOR(ChangeUintAndIntShrToJSTaggedValue);
        GateRef taggedNumber = CallRuntime(ChangeUintAndIntShrToJSTaggedValue, glue,
            GetInt64Constant(FAST_STUB_ID(ChangeUintAndIntShrToJSTaggedValue)), {glue, left, right});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef shift = Int32And(*opNumber1, GetInt32Constant(0x1f));
        GateRef ret = UInt32LSR(*opNumber0, shift);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}
DECLARE_ASM_HANDLER(HandleShl2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = ChangeFloat64ToInt32(leftDouble);
                    opNumber1 = ChangeFloat64ToInt32(rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *ChangeUintAndIntShlToJSTaggedValue = GET_STUBDESCRIPTOR(ChangeUintAndIntShlToJSTaggedValue);
        GateRef taggedNumber = CallRuntime(ChangeUintAndIntShlToJSTaggedValue, glue,
            GetInt64Constant(FAST_STUB_ID(ChangeUintAndIntShlToJSTaggedValue)), {glue, left, right});
        Label IsException(env);
        Label NotException(env);
        Branch(TaggedIsException(taggedNumber), &IsException, &NotException);
        Bind(&IsException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&NotException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef shift = Int32And(*opNumber1, GetInt32Constant(0x1f));
        GateRef ret = Int32LSL(*opNumber0, shift);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef methodId = ReadInst16_1(pc);
    GateRef literalId = ReadInst16_3(pc);
    GateRef length = ReadInst16_5(pc);
    GateRef v0 = ReadInst8_7(pc);
    GateRef v1 = ReadInst8_8(pc);

    GateRef classTemplate = GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId));
    GateRef literalBuffer = GetObjectFromConstPool(constpool, ZExtInt16ToInt32(literalId));
    GateRef lexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v1));

    DEFVARIABLE(res, StubMachineType::TAGGED, GetUndefinedConstant());

    Label isResolved(env);
    Label isNotResolved(env);
    Label afterCheckResolved(env);
    Branch(FunctionIsResolved(classTemplate), &isResolved, &isNotResolved);
    Bind(&isResolved);
    {
        StubDescriptor *cloneClassFromTemplate = GET_STUBDESCRIPTOR(CloneClassFromTemplate);
        res = CallRuntime(cloneClassFromTemplate, glue, GetInt64Constant(FAST_STUB_ID(CloneClassFromTemplate)),
                          { glue, classTemplate, proto, lexicalEnv, constpool });
        Jump(&afterCheckResolved);
    }
    Bind(&isNotResolved);
    {
        StubDescriptor *resolveClass = GET_STUBDESCRIPTOR(ResolveClass);
        res = CallRuntime(resolveClass, glue, GetInt64Constant(FAST_STUB_ID(ResolveClass)),
                          { glue, classTemplate, literalBuffer, proto, lexicalEnv, constpool });
        Jump(&afterCheckResolved);
    }
    Bind(&afterCheckResolved);
    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(*res), &isException, &isNotException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
    }
    Bind(&isNotException);
    GateRef newLexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));  // slow runtime may gc
    SetLexicalEnvToFunction(glue, *res, newLexicalEnv);
    StubDescriptor *setClassConstructorLength = GET_STUBDESCRIPTOR(SetClassConstructorLength);
    CallRuntime(setClassConstructorLength, glue, GetInt64Constant(FAST_STUB_ID(SetClassConstructorLength)),
                { glue, *res, length });
    varAcc = *res;
    DISPATCH_WITH_ACC(PREF_ID16_IMM16_IMM16_V8_V8);
}

DECLARE_ASM_HANDLER(HandleLdObjByNamePrefId32V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    Label receiverIsHeapObject(env);
    Label dispatch(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label loadWithHandler(env);
                GateRef secondValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo,
                    Int32Add(slotId, GetInt32Constant(1)));
                DEFVARIABLE(cachedHandler, StubMachineType::TAGGED, secondValue);
                GateRef hclass = LoadHClass(receiver);
                Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
                       &loadWithHandler, &tryPoly);

                Bind(&tryPoly);
                {
                    cachedHandler = CheckPolyHClass(firstValue, hclass);
                    Branch(TaggedIsHole(*cachedHandler), &slowPath, &loadWithHandler);
                }

                Bind(&loadWithHandler);
                {
                    Label notHole(env);

                    GateRef result = LoadICWithHandler(glue, receiver, receiver, *cachedHandler);
                    Branch(TaggedIsHole(result), &slowPath, &notHole);
                    Bind(&notHole);
                    varAcc = result;
                    Jump(&dispatch);
                }
            }
        }
        Bind(&tryFastPath);
        {
            Label notHole(env);
            GateRef stringId = ReadInst32_1(pc);
            GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
            auto stubDescriptor = GET_STUBDESCRIPTOR(GetPropertyByName);
            GateRef result = CallRuntime(stubDescriptor, glue, GetInt64Constant(FAST_STUB_ID(GetPropertyByName)), {
                glue, receiver, propKey
            });
            Branch(TaggedIsHole(result), &slowPath, &notHole);

            Bind(&notHole);
            varAcc = result;
            Jump(&dispatch);
        }
    }
    Bind(&slowPath);
    {
        Label isException(env);
        Label noException(env);
        GateRef stringId = ReadInst32_1(pc);
        GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
        auto stubDescriptor = GET_STUBDESCRIPTOR(LoadICByName);
        GateRef result = CallRuntime(stubDescriptor, glue, GetInt64Constant(FAST_STUB_ID(LoadICByName)), {
            glue, profileTypeInfo, receiver, propKey, slotId
        });
        Branch(TaggedIsException(result), &isException, &noException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&noException);
        varAcc = result;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_ID32_V8);
}

DECLARE_ASM_HANDLER(HandleStObjByNamePrefId32V8)
{
    auto env = GetEnvironment();

    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    DEFVARIABLE(result, StubMachineType::UINT64, GetHoleConstant(StubMachineType::UINT64));

    Label checkResult(env);
    Label dispatch(env);
    Label slowPath(env);

    Label receiverIsHeapObject(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label storeWithHandler(env);
                GateRef secondValue = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo,
                    Int32Add(slotId, GetInt32Constant(1)));
                DEFVARIABLE(cachedHandler, StubMachineType::TAGGED, secondValue);
                GateRef hclass = LoadHClass(receiver);
                Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
                    &storeWithHandler, &tryPoly);
                Bind(&tryPoly);
                {
                    cachedHandler = CheckPolyHClass(firstValue, hclass);
                    Branch(TaggedIsHole(*cachedHandler), &slowPath, &storeWithHandler);
                }
                Bind(&storeWithHandler);
                {
                    result = StoreICWithHandler(glue, receiver, receiver, acc, *cachedHandler);
                    Branch(TaggedIsHole(*result), &slowPath, &checkResult);
                }
            }
        }
        Bind(&tryFastPath);
        {
            GateRef stringId = ReadInst32_1(pc);
            GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
            auto stubDescriptor = GET_STUBDESCRIPTOR(SetPropertyByName);
            result = CallRuntime(stubDescriptor, glue, GetInt64Constant(FAST_STUB_ID(SetPropertyByName)), {
                glue, receiver, propKey, acc
            });
            Branch(TaggedIsHole(*result), &slowPath, &checkResult);
        }
    }
    Bind(&slowPath);
    {
        GateRef stringId = ReadInst32_1(pc);
        GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
        auto stubDescriptor = GET_STUBDESCRIPTOR(StoreICByName);
        result = CallRuntime(stubDescriptor, glue, GetInt64Constant(FAST_STUB_ID(StoreICByName)), {
            glue, profileTypeInfo, receiver, propKey, acc, slotId
        });
        Jump(&checkResult);
    }
    Bind(&checkResult);
    {
        Label isException(env);
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
    }
    Bind(&dispatch);
    DISPATCH(PREF_ID32_V8);
}

DECLARE_ASM_HANDLER(HandleStOwnByValueWithNameSetPrefV8V8)
{
    auto env = GetEnvironment();
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_2(pc)));

    Label isHeapObject(env);
    Label slowPath(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label isException(env);
    Label notException(env);
    Label isException1(env);
    Label notException1(env);

    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    {
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                StubDescriptor *setPropertyByValue = GET_STUBDESCRIPTOR(SetPropertyByValue);
                GateRef res = CallRuntime(setPropertyByValue, glue, GetInt64Constant(FAST_STUB_ID(SetPropertyByValue)),
                                          { glue, receiver, propKey, acc });
                Branch(TaggedIsHole(res), &slowPath, &notHole);
                Bind(&notHole);
                {
                    Branch(TaggedIsException(res), &isException, &notException);
                    Bind(&isException);
                    {
                        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);
                    }
                    Bind(&notException);
                    StubDescriptor *setFunctionNameNoPrefix = GET_STUBDESCRIPTOR(SetFunctionNameNoPrefix);
                    CallRuntime(setFunctionNameNoPrefix, glue, GetInt64Constant(FAST_STUB_ID(SetFunctionNameNoPrefix)),
                                { glue, acc, propKey });
                    DISPATCH(PREF_V8_V8);
                }
            }
        }
    }
    Bind(&slowPath);
    {
        StubDescriptor *stOwnByValueWithNameSet = GET_STUBDESCRIPTOR(StOwnByValueWithNameSet);
        GateRef res = CallRuntime(stOwnByValueWithNameSet, glue,
            GetInt64Constant(FAST_STUB_ID(StOwnByValueWithNameSet)), { glue, receiver, propKey, acc });
        Branch(TaggedIsException(res), &isException1, &notException1);
        Bind(&isException1);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);
        }
        Bind(&notException1);
        DISPATCH(PREF_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleStOwnByNamePrefId32V8)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    DEFVARIABLE(result, StubMachineType::UINT64, GetHoleConstant(StubMachineType::UINT64));

    Label checkResult(env);
    Label dispatch(env);

    Label isJSObject(env);
    Label slowPath(env);
    Branch(IsJSObject(receiver), &isJSObject, &slowPath);
    Bind(&isJSObject);
    {
        Label notClassConstructor(env);
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Label fastPath(env);
            Branch(IsClassPrototype(receiver), &slowPath, &fastPath);
            Bind(&fastPath);
            {
                result = SetPropertyByNameWithOwn(glue, receiver, propKey, acc);
                Branch(TaggedIsHole(*result), &slowPath, &checkResult);
            }
        }
    }
    Bind(&slowPath);
    {
        StubDescriptor *StOwnByName = GET_STUBDESCRIPTOR(StOwnByName);
        result = CallRuntime(StOwnByName, glue, GetInt64Constant(FAST_STUB_ID(StOwnByName)),
                             { glue, receiver, propKey, acc });
        Jump(&checkResult);
    }
    Bind(&checkResult);
    {
        Label isException(env);
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
    }
    Bind(&dispatch);
    DISPATCH(PREF_ID32_V8);
}

DECLARE_ASM_HANDLER(HandleStOwnByNameWithNameSetPrefId32V8)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst32_1(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    GateRef propKey = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label isException(env);
    Label notException(env);
    Label isException1(env);
    Label notException1(env);
    Branch(IsJSObject(receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        Branch(IsClassConstructor(receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = SetPropertyByNameWithOwn(glue, receiver, propKey, acc);
                Branch(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    Branch(TaggedIsException(res), &isException, &notException);
                    Bind(&isException);
                    {
                        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);
                    }
                    Bind(&notException);
                    StubDescriptor *setFunctionNameNoPrefix = GET_STUBDESCRIPTOR(SetFunctionNameNoPrefix);
                    CallRuntime(setFunctionNameNoPrefix, glue, GetInt64Constant(FAST_STUB_ID(SetFunctionNameNoPrefix)),
                                { glue, acc, propKey });
                    DISPATCH(PREF_ID32_V8);
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        StubDescriptor *stOwnByNameWithNameSet = GET_STUBDESCRIPTOR(StOwnByNameWithNameSet);
        GateRef res = CallRuntime(stOwnByNameWithNameSet, glue, GetInt64Constant(FAST_STUB_ID(StOwnByNameWithNameSet)),
                                  { glue, receiver, propKey, acc });
        Branch(TaggedIsException(res), &isException1, &notException1);
        Bind(&isException1);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);
        }
        Bind(&notException1);
        DISPATCH(PREF_ID32_V8);
    }
}

DECLARE_ASM_HANDLER(HandleLdFunctionPref)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    varAcc = GetFunctionFromFrame(GetFrame(sp));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleMovV4V4)
{
    GateRef vdst = ZExtInt8ToPtr(ReadInst4_0(pc));
    GateRef vsrc = ZExtInt8ToPtr(ReadInst4_1(pc));
    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    DISPATCH(V4_V4);
}

DECLARE_ASM_HANDLER(HandleMovDynV8V8)
{
    GateRef vdst = ZExtInt8ToPtr(ReadInst8_0(pc));
    GateRef vsrc = ZExtInt8ToPtr(ReadInst8_1(pc));
    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    DISPATCH(V8_V8);
}

DECLARE_ASM_HANDLER(HandleMovDynV16V16)
{
    GateRef vdst = ZExtInt16ToPtr(ReadInst16_0(pc));
    GateRef vsrc = ZExtInt16ToPtr(ReadInst16_2(pc));
    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    DISPATCH(V16_V16);
}

DECLARE_ASM_HANDLER(HandleLdaStrId32)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef stringId = ReadInst32_0(pc);
    varAcc = GetValueFromTaggedArray(StubMachineType::TAGGED, constpool, stringId);
    DISPATCH_WITH_ACC(ID32);
}

DECLARE_ASM_HANDLER(HandleLdaiDynImm32)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef imm = ReadInst32_0(pc);
    varAcc = IntBuildTaggedWithNoGC(imm);
    DISPATCH_WITH_ACC(IMM32);
}

DECLARE_ASM_HANDLER(HandleFldaiDynImm64)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef imm = CastInt64ToFloat64(ReadInst64_0(pc));
    varAcc = DoubleBuildTaggedWithNoGC(imm);
    DISPATCH_WITH_ACC(IMM64);
}

DECLARE_ASM_HANDLER(HandleJeqzImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned8_0(pc);
    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accEqualZero(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(Int64Equal(ChangeTaggedPointerToInt64(acc), TaggedFalse()), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), GetInt32Constant(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), GetDoubleConstant(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(sp);
        Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
             GetIntPtrConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

DECLARE_ASM_HANDLER(HandleJeqzImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned16_0(pc);
    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accEqualZero(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(Int64Equal(ChangeTaggedPointerToInt64(acc), TaggedFalse()), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), GetInt32Constant(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), GetDoubleConstant(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(sp);
        Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
             GetIntPtrConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16)));
}

DECLARE_ASM_HANDLER(HandleJnezImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned8_0(pc);
    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accEqualZero(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(Int64Equal(ChangeTaggedPointerToInt64(acc), TaggedTrue()), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), GetInt32Constant(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), GetDoubleConstant(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(sp);
        Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
             GetIntPtrConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

DECLARE_ASM_HANDLER(HandleJnezImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    GateRef offset = ReadInstSigned16_0(pc);
    Label accEqualTrue(env);
    Label accNotEqualTrue(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accEqualZero(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(Int64Equal(ChangeTaggedPointerToInt64(acc), TaggedTrue()), &accEqualTrue, &accNotEqualTrue);
    Bind(&accNotEqualTrue);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Int32Equal(TaggedGetInt(acc), GetInt32Constant(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), GetDoubleConstant(0)), &last, &accEqualTrue);
            }
        }
    }
    Bind(&accEqualTrue);
    {
        Label dispatch(env);
        Label slowPath(env);
        UPDATE_HOTNESS(sp);
        Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
             GetIntPtrConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16)));
}

DECLARE_ASM_HANDLER(HandleReturnDyn)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, StubMachineType::NATIVE_POINTER, pc);
    DEFVARIABLE(varSp, StubMachineType::NATIVE_POINTER, sp);
    DEFVARIABLE(varConstpool, StubMachineType::TAGGED_POINTER, constpool);
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label updateHotness(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    GateRef frame = GetFrame(*varSp);
    Branch(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &tryContinue);
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(frame);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(StubMachineType::NATIVE_POINTER, method,
            GetIntPtrConstant(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(IntPtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        Store(StubMachineType::INT32, glue, method,
              GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET), *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    varSp = Load(StubMachineType::NATIVE_POINTER, frame,
        GetIntPtrConstant(InterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, GetIntPtrConstant(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        SetAccToFrame(glue, frame, acc);
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        SetCurrentSpFrame(glue, *varSp);
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(StubMachineType::INT32, method,
                                 GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET));
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, acc,
                 *varHotnessCounter, GetIntPtrConstant(0));
    }
}

DECLARE_ASM_HANDLER(HandleReturnUndefinedPref)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, StubMachineType::NATIVE_POINTER, pc);
    DEFVARIABLE(varSp, StubMachineType::NATIVE_POINTER, sp);
    DEFVARIABLE(varConstpool, StubMachineType::TAGGED_POINTER, constpool);
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label updateHotness(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    GateRef frame = GetFrame(*varSp);
    Branch(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &tryContinue);
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(frame);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(StubMachineType::NATIVE_POINTER, method,
            GetIntPtrConstant(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(IntPtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        Store(StubMachineType::INT32, glue, method,
              GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET), *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    varSp = Load(StubMachineType::NATIVE_POINTER, frame,
        GetIntPtrConstant(InterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    varAcc = GetUndefinedConstant();
    Branch(IntPtrEqual(*varPc, GetIntPtrConstant(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        SetAccToFrame(glue, frame, *varAcc);
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        SetCurrentSpFrame(glue, *varSp);
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(StubMachineType::INT32, method,
                                 GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET));
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
                 *varHotnessCounter, GetIntPtrConstant(0));
    }
}

DECLARE_ASM_HANDLER(HandleSuspendGeneratorPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, StubMachineType::NATIVE_POINTER, pc);
    DEFVARIABLE(varSp, StubMachineType::NATIVE_POINTER, sp);
    DEFVARIABLE(varConstpool, StubMachineType::TAGGED_POINTER, constpool);
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    Label pcEqualNullptr(env);
    Label pcNotEqualNullptr(env);
    Label updateHotness(env);
    Label tryContinue(env);
    Label dispatch(env);
    Label slowPath(env);

    GateRef genObj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef frame = GetFrame(*varSp);
    SetPcToFrame(glue, frame, pc);
    StubDescriptor *suspendGenerator = GET_STUBDESCRIPTOR(SuspendGenerator);
    GateRef res = CallRuntime(suspendGenerator, glue, GetInt64Constant(FAST_STUB_ID(SuspendGenerator)),
        { glue, genObj, value });
    
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc, *varHotnessCounter);
    }
    Bind(&notException);
    varAcc = res;
    Branch(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &tryContinue);
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(frame);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(StubMachineType::NATIVE_POINTER, method,
            GetIntPtrConstant(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(IntPtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        Store(StubMachineType::INT32, glue, method,
              GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET), *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    varSp = Load(StubMachineType::NATIVE_POINTER, frame,
        GetIntPtrConstant(InterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, GetIntPtrConstant(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        SetAccToFrame(glue, frame, acc);
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        SetCurrentSpFrame(glue, *varSp);
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(StubMachineType::INT32, method,
                                 GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET));
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, acc,
                 *varHotnessCounter, GetIntPtrConstant(0));
    }
}

DECLARE_ASM_HANDLER(ExceptionHandler)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, StubMachineType::NATIVE_POINTER, pc);
    DEFVARIABLE(varSp, StubMachineType::NATIVE_POINTER, sp);
    DEFVARIABLE(varConstpool, StubMachineType::TAGGED_POINTER, constpool);
    DEFVARIABLE(varProfileTypeInfo, StubMachineType::TAGGED_POINTER, profileTypeInfo);
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    DEFVARIABLE(varHotnessCounter, StubMachineType::INT32, hotnessCounter);

    Label pcIsInvalid(env);
    Label pcNotInvalid(env);
    GateRef exception = Load(StubMachineType::TAGGED, glue, GetIntPtrConstant(0));
    StubDescriptor *upFrame = GET_STUBDESCRIPTOR(UpFrame);
    varPc = CallRuntime(upFrame, glue, GetInt64Constant(FAST_STUB_ID(UpFrame)), { glue, sp });
    Branch(IntPtrEqual(*varPc, GetIntPtrConstant(0)), &pcIsInvalid, &pcNotInvalid);
    Bind(&pcIsInvalid);
    {
        Return();
    }
    Bind(&pcNotInvalid);
    {
        varSp = GetCurrentSpFrame(glue);
        varAcc = exception;
        // clear exception
        Store(StubMachineType::UINT64, glue, glue, GetIntPtrConstant(0), GetHoleConstant());
        GateRef function = GetFunctionFromFrame(GetFrame(*varSp));
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(StubMachineType::NATIVE_POINTER, function,
            GetIntPtrConstant(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(StubMachineType::INT32, method,
                                 GetIntPtrConstant(JSMethod::HOTNESS_COUNTER_OFFSET));
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
                 *varHotnessCounter, GetIntPtrConstant(0));
    }
}

DECLARE_ASM_HANDLER(HandleImportModulePrefId32)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);
    StubDescriptor *importModule = GET_STUBDESCRIPTOR(ImportModule);
    GateRef moduleRef = CallRuntime(importModule, glue, GetInt64Constant(FAST_STUB_ID(ImportModule)), { glue, prop });
    varAcc = moduleRef;
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStModuleVarPrefId32)
{
    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);
    GateRef value = acc;

    StubDescriptor *stModuleVar = GET_STUBDESCRIPTOR(StModuleVar);
    CallRuntime(stModuleVar, glue, GetInt64Constant(FAST_STUB_ID(StModuleVar)), { glue, prop, value });
    DISPATCH(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleLdModVarByNamePrefId32V8)
{
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ReadInst8_5(pc);
    GateRef itemName = GetObjectFromConstPool(constpool, stringId);
    GateRef moduleObj = GetVregValue(sp, ZExtInt8ToPtr(v0));

    StubDescriptor *ldModvarByName = GET_STUBDESCRIPTOR(LdModvarByName);
    GateRef moduleVar = CallRuntime(ldModvarByName, glue, GetInt64Constant(FAST_STUB_ID(LdModvarByName)),
                                    { glue, moduleObj, itemName });
    varAcc = moduleVar;
    DISPATCH_WITH_ACC(PREF_ID32_V8);
}

DECLARE_ASM_HANDLER(HandleTryLdGlobalByNamePrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);

    Label dispatch(env);
    Label icAvailable(env);
    Label icNotAvailable(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        DEFVARIABLE(icResult, StubMachineType::TAGGED, GetUndefinedConstant());
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
        Label isHeapObject(env);
        Label ldMiss(env);
        Label icResultCheck(env);
        Branch(TaggedIsHeapObject(handler), &isHeapObject, &ldMiss);
        Bind(&isHeapObject);
        {
            icResult = LoadGlobal(handler);
            Branch(TaggedIsHole(*icResult), &ldMiss, &icResultCheck);
        }
        Bind(&ldMiss);
        {
            GateRef globalObject = GetGlobalObject(glue);
            StubDescriptor *loadMiss = GET_STUBDESCRIPTOR(LoadMiss);
            icResult = CallRuntime(loadMiss, glue, GetInt64Constant(FAST_STUB_ID(LoadMiss)), {
                glue, profileTypeInfo, globalObject, prop, slotId,
                GetInt32Constant(static_cast<int>(ICKind::NamedGlobalLoadIC))
            });
            Jump(&icResultCheck);
        }
        Bind(&icResultCheck);
        {
            Label isException(env);
            Label isNotException(env);
            Branch(TaggedIsException(*icResult), &isException, &isNotException);
            Bind(&isException);
            {
                DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
            }
            Bind(&isNotException);
            varAcc = *icResult;
            Jump(&dispatch);
        }
    }
    Bind(&icNotAvailable);
    {
        // order: 1. global record 2. global object
        // if we find a way to get global record, we can inline LdGlobalRecord directly
        StubDescriptor *ldGlobalRecord = GET_STUBDESCRIPTOR(LdGlobalRecord);
        GateRef recordResult = CallRuntime(ldGlobalRecord, glue, GetInt64Constant(FAST_STUB_ID(LdGlobalRecord)),
                                           { glue, prop });
        Label isFound(env);
        Label isNotFound(env);
        Branch(TaggedIsUndefined(recordResult), &isNotFound, &isFound);
        Bind(&isNotFound);
        {
            StubDescriptor *getGlobalOwnProperty = GET_STUBDESCRIPTOR(GetGlobalOwnProperty);
            GateRef globalResult = CallRuntime(getGlobalOwnProperty, glue,
                GetInt64Constant(FAST_STUB_ID(GetGlobalOwnProperty)), { glue, prop });
            Label isFoundInGlobal(env);
            Label slowPath(env);
            Branch(TaggedIsHole(globalResult), &slowPath, &isFoundInGlobal);
            Bind(&slowPath);
            {
                StubDescriptor *tryLdGlobalByName = GET_STUBDESCRIPTOR(TryLdGlobalByName);
                GateRef slowResult = CallRuntime(tryLdGlobalByName, glue,
                    GetInt64Constant(FAST_STUB_ID(TryLdGlobalByName)), { glue, prop });
                Label isException(env);
                Label isNotException(env);
                Branch(TaggedIsException(slowResult), &isException, &isNotException);
                Bind(&isException);
                {
                    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
                }
                Bind(&isNotException);
                varAcc = slowResult;
                Jump(&dispatch);
            }
            Bind(&isFoundInGlobal);
            {
                varAcc = globalResult;
                Jump(&dispatch);
            }
        }
        Bind(&isFound);
        {
            varAcc = Load(StubMachineType::TAGGED, recordResult, GetIntPtrConstant(PropertyBox::VALUE_OFFSET));
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleTryStGlobalByNamePrefId32)
{
    auto env = GetEnvironment();
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);
    DEFVARIABLE(result, StubMachineType::TAGGED, GetUndefinedConstant());

    Label checkResult(env);
    Label dispatch(env);

    Label icAvailable(env);
    Label icNotAvailable(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
        Label isHeapObject(env);
        Label stMiss(env);
        Branch(TaggedIsHeapObject(handler), &isHeapObject, &stMiss);
        Bind(&isHeapObject);
        {
            result = StoreGlobal(glue, acc, handler);
            Branch(TaggedIsHole(*result), &stMiss, &checkResult);
        }
        Bind(&stMiss);
        {
            GateRef globalObject = GetGlobalObject(glue);
            StubDescriptor *storeMiss = GET_STUBDESCRIPTOR(StoreMiss);
            result = CallRuntime(storeMiss, glue, GetInt64Constant(FAST_STUB_ID(StoreMiss)), {
                glue, profileTypeInfo, globalObject, propKey, acc, slotId,
                GetInt32Constant(static_cast<int>(ICKind::NamedGlobalStoreIC))
            });
            Jump(&checkResult);
        }
    }
    Bind(&icNotAvailable);
    // order: 1. global record 2. global object
    // if we find a way to get global record, we can inline LdGlobalRecord directly
    StubDescriptor *ldGlobalRecord = GET_STUBDESCRIPTOR(LdGlobalRecord);
    GateRef recordInfo = CallRuntime(ldGlobalRecord, glue, GetInt64Constant(FAST_STUB_ID(LdGlobalRecord)),
                                     { glue, propKey });
    Label isFound(env);
    Label isNotFound(env);
    Branch(TaggedIsUndefined(recordInfo), &isNotFound, &isFound);
    Bind(&isFound);
    {
        StubDescriptor *tryUpdateGlobalRecord = GET_STUBDESCRIPTOR(TryUpdateGlobalRecord);
        result = CallRuntime(tryUpdateGlobalRecord, glue, GetInt64Constant(FAST_STUB_ID(TryUpdateGlobalRecord)),
                             { glue, propKey, acc });
        Jump(&checkResult);
    }
    Bind(&isNotFound);
    {
        Label foundInGlobal(env);
        Label notFoundInGlobal(env);
        StubDescriptor *getGlobalOwnProperty = GET_STUBDESCRIPTOR(GetGlobalOwnProperty);
        GateRef globalResult = CallRuntime(getGlobalOwnProperty, glue,
            GetInt64Constant(FAST_STUB_ID(GetGlobalOwnProperty)), { glue, propKey });
        Branch(TaggedIsHole(globalResult), &notFoundInGlobal, &foundInGlobal);
        Bind(&notFoundInGlobal);
        {
            StubDescriptor *throwReferenceError = GET_STUBDESCRIPTOR(ThrowReferenceError);
            result = CallRuntime(throwReferenceError, glue,
                GetInt64Constant(FAST_STUB_ID(ThrowReferenceError)), { glue, propKey });
            DISPATCH_LAST();
        }
        Bind(&foundInGlobal);
        {
            StubDescriptor *stGlobalVar = GET_STUBDESCRIPTOR(StGlobalVar);
            result = CallRuntime(stGlobalVar, glue,
                GetInt64Constant(FAST_STUB_ID(StGlobalVar)), { glue, propKey, acc });
            Jump(&checkResult);
        }
    }
    Bind(&checkResult);
    {
        Label isException(env);
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
    }
    Bind(&dispatch);
    DISPATCH(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleLdGlobalVarPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);
    DEFVARIABLE(result, StubMachineType::TAGGED, GetUndefinedConstant());

    Label checkResult(env);
    Label dispatch(env);
    Label slowPath(env);
    GateRef globalObject = GetGlobalObject(glue);

    Label icAvailable(env);
    Label icNotAvailable(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
        Label isHeapObject(env);
        Label ldMiss(env);
        Branch(TaggedIsHeapObject(handler), &isHeapObject, &ldMiss);
        Bind(&isHeapObject);
        {
            result = LoadGlobal(handler);
            Branch(TaggedIsHole(*result), &ldMiss, &checkResult);
        }
        Bind(&ldMiss);
        {
            StubDescriptor *loadMiss = GET_STUBDESCRIPTOR(LoadMiss);
            result = CallRuntime(loadMiss, glue, GetInt64Constant(FAST_STUB_ID(LoadMiss)), {
                glue, profileTypeInfo, globalObject, propKey, slotId,
                GetInt32Constant(static_cast<int>(ICKind::NamedGlobalLoadIC))
            });
            Jump(&checkResult);
        }
    }
    Bind(&icNotAvailable);
    {
        StubDescriptor *getGlobalOwnProperty = GET_STUBDESCRIPTOR(GetGlobalOwnProperty);
        result = CallRuntime(getGlobalOwnProperty, glue,
            GetInt64Constant(FAST_STUB_ID(GetGlobalOwnProperty)), { glue, propKey });
        Branch(TaggedIsHole(*result), &slowPath, &dispatch);
        Bind(&slowPath);
        {
            StubDescriptor *ldGlobalVar = GET_STUBDESCRIPTOR(LdGlobalVar);
            result = CallRuntime(ldGlobalVar, glue,
                GetInt64Constant(FAST_STUB_ID(LdGlobalVar)), { glue, globalObject, propKey });
            Jump(&checkResult);
        }
    }
    Bind(&checkResult);
    {
        Label isException(env);
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
    }
    Bind(&dispatch);
    varAcc = *result;
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStGlobalVarPrefId32)
{
    auto env = GetEnvironment();

    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);
    DEFVARIABLE(result, StubMachineType::TAGGED, GetUndefinedConstant());

    Label checkResult(env);
    Label dispatch(env);

    Label icAvailable(env);
    Label icNotAvailable(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(StubMachineType::TAGGED, profileTypeInfo, slotId);
        Label isHeapObject(env);
        Label stMiss(env);
        Branch(TaggedIsHeapObject(handler), &isHeapObject, &stMiss);
        Bind(&isHeapObject);
        {
            result = StoreGlobal(glue, acc, handler);
            Branch(TaggedIsHole(*result), &stMiss, &checkResult);
        }
        Bind(&stMiss);
        {
            GateRef globalObject = GetGlobalObject(glue);
            StubDescriptor *storeMiss = GET_STUBDESCRIPTOR(StoreMiss);
            result = CallRuntime(storeMiss, glue, GetInt64Constant(FAST_STUB_ID(StoreMiss)), {
                glue, profileTypeInfo, globalObject, propKey, acc, slotId,
                GetInt32Constant(static_cast<int>(ICKind::NamedGlobalStoreIC))
            });
            Jump(&checkResult);
        }
    }
    Bind(&icNotAvailable);
    {
        StubDescriptor *stGlobalVar = GET_STUBDESCRIPTOR(StGlobalVar);
        result = CallRuntime(stGlobalVar, glue,
            GetInt64Constant(FAST_STUB_ID(StGlobalVar)), { glue, propKey, acc });
        Jump(&checkResult);
    }
    Bind(&checkResult);
    {
        Label isException(env);
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
    }
    Bind(&dispatch);
    DISPATCH(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleIsTruePref)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    Label objNotTrue(env);
    Label dispatch(env);
    Label slowPath(env);
    Label isTrue(env);
    Label isFalse(env);

    Branch(TaggedIsTrue(*varAcc), &isTrue, &objNotTrue);
    Bind(&objNotTrue);
    {
        Branch(TaggedIsFalse(*varAcc), &isFalse, &slowPath);
    }

    Bind(&slowPath);
    StubDescriptor *toBoolean = GET_STUBDESCRIPTOR(ToBoolean);
    GateRef result = CallRuntime(toBoolean, glue,
        GetInt64Constant(FAST_STUB_ID(ToBoolean)), { *varAcc });
    Branch(result, &isTrue, &isFalse);
    Bind(&isTrue);
    {
        varAcc = ChangeInt64ToTagged(TaggedTrue());
        Jump(&dispatch);
    }
    Bind(&isFalse);
    {
        varAcc = ChangeInt64ToTagged(TaggedFalse());
        Jump(&dispatch);
    }

    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleIsFalsePref)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    Label objNotTrue(env);
    Label dispatch(env);
    Label slowPath(env);
    Label isTrue(env);
    Label isFalse(env);

    Branch(TaggedIsTrue(*varAcc), &isTrue, &objNotTrue);
    Bind(&objNotTrue);
    {
        Branch(TaggedIsFalse(*varAcc), &isFalse, &slowPath);
    }

    Bind(&slowPath);
    StubDescriptor *toBoolean = GET_STUBDESCRIPTOR(ToBoolean);
    GateRef result = CallRuntime(toBoolean, glue,
        GetInt64Constant(FAST_STUB_ID(ToBoolean)), { *varAcc });
    Branch(result, &isTrue, &isFalse);
    Bind(&isTrue);
    {
        varAcc = ChangeInt64ToTagged(TaggedFalse());
        Jump(&dispatch);
    }
    Bind(&isFalse);
    {
        varAcc = ChangeInt64ToTagged(TaggedTrue());
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleToNumberPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);
    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsNumber(env);
    Label valueNotNumber(env);
    Branch(TaggedIsNumber(value), &valueIsNumber, &valueNotNumber);
    Bind(&valueIsNumber);
    {
        varAcc = value;
        DISPATCH_WITH_ACC(PREF_V8);
    }
    Bind(&valueNotNumber);
    {
        StubDescriptor *toNumber = GET_STUBDESCRIPTOR(ToNumber);
        GateRef res = CallRuntime(toNumber, glue, GetInt64Constant(FAST_STUB_ID(ToNumber)), {glue, value});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = res;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
}

DECLARE_ASM_HANDLER(HandleAdd2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);

                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToInt32(right);
                    Label leftIsZero(env);
                    Label leftNotZero(env);
                    Branch(Int32Equal(*opNumber0, GetInt32Constant(0)), &leftIsZero, &leftNotZero);
                    Bind(&leftIsZero);
                    {
                        GateRef res = Int32Add(*opNumber0, *opNumber1);
                        varAcc = IntBuildTaggedWithNoGC(res);
                        Jump(&accDispatch);
                    }
                    Bind(&leftNotZero);
                    {
                        Label rightIsZero(env);
                        Label rightNotZero(env);
                        Branch(Int32Equal(*opNumber1, GetInt32Constant(0)), &rightIsZero, &rightNotZero);
                        Bind(&rightIsZero);
                        {
                            GateRef res = Int32Add(*opNumber0, *opNumber1);
                            varAcc = IntBuildTaggedWithNoGC(res);
                            Jump(&accDispatch);
                        }
                        Bind(&rightNotZero);
                        {
                            Label leftIsPositive(env);
                            Label leftIsNegative(env);
                            Branch(Int32GreaterThan(*opNumber0, GetInt32Constant(0)), &leftIsPositive, &leftIsNegative);
                            Bind(&leftIsPositive);
                            {
                                Label positiveOverflow(env);
                                Label notPositiveOverflow(env);
                                Branch(Int32GreaterThan(*opNumber1, Int32Sub(GetInt32Constant(INT32_MAX), *opNumber0)),
                                    &positiveOverflow, &notPositiveOverflow);
                                Bind(&positiveOverflow);
                                {
                                    GateRef opNumber0ToDouble = ChangeInt32ToFloat64(*opNumber0);
                                    GateRef opNumber1ToDouble = ChangeInt32ToFloat64(*opNumber1);
                                    GateRef res = DoubleAdd(opNumber0ToDouble, opNumber1ToDouble);
                                    varAcc = DoubleBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                                Bind(&notPositiveOverflow);
                                {
                                    GateRef res = Int32Add(*opNumber0, *opNumber1);
                                    varAcc = IntBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                            }
                            Bind(&leftIsNegative);
                            {
                                Label negativeOverflow(env);
                                Label notNegativeOverflow(env);
                                Branch(Int32LessThan(*opNumber1, Int32Sub(GetInt32Constant(INT32_MIN), *opNumber0)),
                                    &negativeOverflow, &notNegativeOverflow);
                                Bind(&negativeOverflow);
                                {
                                    GateRef opNumber0ToDouble = ChangeInt32ToFloat64(*opNumber0);
                                    GateRef opNumber1ToDouble = ChangeInt32ToFloat64(*opNumber1);
                                    GateRef res = DoubleAdd(opNumber0ToDouble, opNumber1ToDouble);
                                    varAcc = DoubleBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                                Bind(&notNegativeOverflow);
                                {
                                    GateRef res = Int32Add(*opNumber0, *opNumber1);
                                    varAcc = IntBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                            }
                        }
                    }
                }
                Bind(&rightIsDouble);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToDouble(right);
                    GateRef opNumber0ToDouble = ChangeInt32ToFloat64(*opNumber0);
                    GateRef res = DoubleAdd(opNumber0ToDouble, *opNumber1);
                    varAcc = DoubleBuildTaggedWithNoGC(res);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToDouble(left);
                    opNumber1 = TaggedCastToInt32(right);
                    GateRef opNumber1ToDouble = ChangeInt32ToFloat64(*opNumber1);
                    GateRef res = DoubleAdd(opNumber1ToDouble, *opNumber0);
                    varAcc = DoubleBuildTaggedWithNoGC(res);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    GateRef res = DoubleAdd(leftDouble, rightDouble);
                    varAcc = DoubleBuildTaggedWithNoGC(res);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *add2Dyn = GET_STUBDESCRIPTOR(Add2Dyn);
        GateRef taggedNumber = CallRuntime(add2Dyn, glue, GetInt64Constant(FAST_STUB_ID(Add2Dyn)),
                                           {glue, left, right});

        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            Jump(&accDispatch);
        }
    }

    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleSub2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, StubMachineType::TAGGED, acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, StubMachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(opNumber1, StubMachineType::INT32, GetInt32Constant(0));

    Label accDispatch(env);
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = Int32Sub(GetInt32Constant(0), TaggedCastToInt32(right));
                    Label leftIsZero(env);
                    Label leftNotZero(env);
                    Branch(Int32Equal(*opNumber0, GetInt32Constant(0)), &leftIsZero, &leftNotZero);
                    Bind(&leftIsZero);
                    {
                        GateRef res = Int32Add(*opNumber0, *opNumber1);
                        varAcc = IntBuildTaggedWithNoGC(res);
                        Jump(&accDispatch);
                    }
                    Bind(&leftNotZero);
                    {
                        Label rightIsZero(env);
                        Label rightNotZero(env);
                        Branch(Int32Equal(*opNumber1, GetInt32Constant(0)), &rightIsZero, &rightNotZero);
                        Bind(&rightIsZero);
                        {
                            GateRef res = Int32Add(*opNumber0, *opNumber1);
                            varAcc = IntBuildTaggedWithNoGC(res);
                            Jump(&accDispatch);
                        }
                        Bind(&rightNotZero);
                        {
                            Label leftIsPositive(env);
                            Label leftIsNegative(env);
                            Branch(Int32GreaterThan(*opNumber0, GetInt32Constant(0)), &leftIsPositive, &leftIsNegative);
                            Bind(&leftIsPositive);
                            {
                                Label positiveOverflow(env);
                                Label notPositiveOverflow(env);
                                Branch(Int32GreaterThan(*opNumber1, Int32Sub(GetInt32Constant(INT32_MAX), *opNumber0)),
                                    &positiveOverflow, &notPositiveOverflow);
                                Bind(&positiveOverflow);
                                {
                                    GateRef opNumber0ToDouble = ChangeInt32ToFloat64(*opNumber0);
                                    GateRef opNumber1ToDouble = ChangeInt32ToFloat64(*opNumber1);
                                    GateRef res = DoubleAdd(opNumber0ToDouble, opNumber1ToDouble);
                                    varAcc = DoubleBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                                Bind(&notPositiveOverflow);
                                {
                                    GateRef res = Int32Add(*opNumber0, *opNumber1);
                                    varAcc = IntBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                            }
                            Bind(&leftIsNegative);
                            {
                                Label negativeOverflow(env);
                                Label notNegativeOverflow(env);
                                Branch(Int32LessThan(*opNumber1, Int32Sub(GetInt32Constant(INT32_MIN), *opNumber0)),
                                    &negativeOverflow, &notNegativeOverflow);
                                Bind(&negativeOverflow);
                                {
                                    GateRef opNumber0ToDouble = ChangeInt32ToFloat64(*opNumber0);
                                    GateRef opNumber1ToDouble = ChangeInt32ToFloat64(*opNumber1);
                                    GateRef res = DoubleAdd(opNumber0ToDouble, opNumber1ToDouble);
                                    varAcc = DoubleBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                                Bind(&notNegativeOverflow);
                                {
                                    GateRef res = Int32Add(*opNumber0, *opNumber1);
                                    varAcc = IntBuildTaggedWithNoGC(res);
                                    Jump(&accDispatch);
                                }
                            }
                        }
                    }
                }
                Bind(&rightIsDouble);
                {
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = TaggedCastToDouble(right);
                    GateRef opNumber0ToDouble = ChangeInt32ToFloat64(*opNumber0);
                    GateRef res = DoubleSub(opNumber0ToDouble, *opNumber1);
                    varAcc = DoubleBuildTaggedWithNoGC(res);
                    Jump(&accDispatch);
                }
            }
            Bind(&leftIsDouble);
            {
                Label rightIsInt(env);
                Label rightIsDouble(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    opNumber0 = TaggedCastToDouble(left);
                    opNumber1 = TaggedCastToInt32(right);
                    GateRef opNumber1ToDouble = ChangeInt32ToFloat64(*opNumber1);
                    GateRef res = DoubleSub(opNumber1ToDouble, *opNumber0);
                    varAcc = DoubleBuildTaggedWithNoGC(res);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    GateRef res = DoubleSub(leftDouble, rightDouble);
                    varAcc = DoubleBuildTaggedWithNoGC(res);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        StubDescriptor *sub2Dyn = GET_STUBDESCRIPTOR(Sub2Dyn);
        GateRef taggedNumber = CallRuntime(sub2Dyn, glue, GetInt64Constant(FAST_STUB_ID(Sub2Dyn)),
                                           {glue, left, right});

        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter);
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            Jump(&accDispatch);
        }
    }

    Bind(&accDispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}
#undef DECLARE_ASM_HANDLER
#undef DISPATCH
#undef DISPATCH_WITH_ACC
#undef DISPATCH_LAST
}  // namespace panda::ecmascript::kungfu
