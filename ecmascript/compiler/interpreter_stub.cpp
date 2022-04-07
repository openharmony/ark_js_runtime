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
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table-inl.h"
#include "stub-inl.h"

namespace panda::ecmascript::kungfu {
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
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
    DebugPrint(glue, { Int32(GET_MESSAGE_STRING_ID(name)) });                             \
    GenerateCircuitImpl(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter);   \
}                                                                                         \
void name##Stub::GenerateCircuitImpl(GateRef glue, GateRef pc, GateRef sp,                \
                                     GateRef constpool, GateRef profileTypeInfo,          \
                                     GateRef acc, GateRef hotnessCounter)
#else
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
#endif

#define DISPATCH(format)                                                                  \
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,               \
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::format)))

#define DISPATCH_WITH_ACC(format)                                                         \
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter,           \
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::format)))

#define DISPATCH_LAST()                                                                   \
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter)           \

#define DISPATCH_LAST_WITH_ACC()                                                          \
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *varAcc, hotnessCounter)       \

#define UPDATE_HOTNESS(_sp)                                                               \
    varHotnessCounter = Int32Add(offset, *varHotnessCounter);                             \
    Branch(Int32LessThan(*varHotnessCounter, Int32(0)), &slowPath, &dispatch);            \
    Bind(&slowPath);                                                                      \
    {                                                                                     \
        varProfileTypeInfo = CallRuntime(glue, RTSTUB_ID(UpdateHotnessCounter), {});      \
        varHotnessCounter = Int32(InterpreterAssembly::METHOD_HOTNESS_THRESHOLD);         \
        Jump(&dispatch);                                                                  \
    }                                                                                     \
    Bind(&dispatch);

DECLARE_ASM_HANDLER(HandleLdNanPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = DoubleBuildTaggedWithNoGC(Double(base::NAN_VALUE));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdInfinityPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = DoubleBuildTaggedWithNoGC(Double(base::POSITIVE_INFINITY));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdGlobalThisPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = GetGlobalObject(glue);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdUndefinedPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Undefined();
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdNullPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Null();
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdSymbolPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = CallRuntime(glue, RTSTUB_ID(GetSymbolFunction), {});
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdGlobalPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = GetGlobalObject(glue);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdTruePref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = ChangeInt64ToTagged(Int64(JSTaggedValue::VALUE_TRUE));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdFalsePref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = ChangeInt64ToTagged(Int64(JSTaggedValue::VALUE_FALSE));
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleThrowDynPref)
{
    SetPcToFrame(glue, GetFrame(sp), pc);
    CallRuntime(glue, RTSTUB_ID(ThrowDyn), { acc });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleTypeOfDynPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = FastTypeOf(glue, acc);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdLexEnvDynPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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

DECLARE_ASM_HANDLER(HandleGetUnmappedArgsPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(GetUnmapedArgs), {});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleCopyRestArgsPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef restIdx = ZExtInt16ToInt32(ReadInst16_1(pc));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CopyRestArgs), { IntBuildTaggedTypeWithNoGC(restIdx) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleCreateArrayWithBufferPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef result = GetObjectFromConstPool(constpool, imm);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateArrayWithBuffer), { result });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleCreateObjectWithBufferPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef result = GetObjectFromConstPool(constpool, imm);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { result });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleCreateObjectWithExcludedKeysPrefImm16V8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef numKeys = ReadInst16_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_4(pc));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectWithExcludedKeys),
        { Int16BuildTaggedTypeWithNoGC(numKeys), obj, Int16BuildTaggedTypeWithNoGC(firstArgRegIdx) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_IMM16_V8_V8);
}

DECLARE_ASM_HANDLER(HandleCreateObjectHavingMethodPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef imm = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef result = GetObjectFromConstPool(constpool, imm);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod), { result, acc, constpool });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleThrowIfSuperNotCorrectCallPrefImm16)
{
    auto env = GetEnvironment();
    GateRef imm = ReadInst16_1(pc);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        { Int16BuildTaggedTypeWithNoGC(imm), acc }); // acc is thisValue
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    DISPATCH(PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleNewLexEnvDynPrefImm16)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef numVars = ReadInst16_1(pc);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvDyn), { Int16BuildTaggedTypeWithNoGC(numVars) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    SetEnvToFrame(glue, GetFrame(sp), res);
    DISPATCH_WITH_ACC(PREF_IMM16);
}

DECLARE_ASM_HANDLER(HandleNewObjDynRangePrefImm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef numArgs = ReadInst16_1(pc);
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef firstArgOffset = Int16(2);
    GateRef func = GetVregValue(sp, ZExtInt16ToPtr(firstArgRegIdx));
    GateRef newTarget = GetVregValue(sp, IntPtrAdd(ZExtInt16ToPtr(firstArgRegIdx), IntPtr(1)));
    GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
    GateRef length = Int16Sub(numArgs, firstArgOffset);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewObjDynRange),
        { func, newTarget, Int16BuildTaggedTypeWithNoGC(firstArgIdx), Int16BuildTaggedTypeWithNoGC(length) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_IMM16_V8);
}

DECLARE_ASM_HANDLER(HandleDefineFuncDynPrefId16Imm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst16_3(pc);
    GateRef v0 = ReadInst8_5(pc);
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId)));
    Label isResolved(env);
    Label notResolved(env);
    Label defaultLabel(env);
    Branch(FunctionIsResolved(*result), &isResolved, &notResolved);
    Bind(&isResolved);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        result = CallRuntime(glue, RTSTUB_ID(DefinefuncDyn), { *result });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            SetConstantPoolToFunction(glue, *result, constpool);
            Jump(&defaultLabel);
        }
    }
    Bind(&notResolved);
    {
        SetResolvedToFunction(glue, *result, Boolean(true));
        Jump(&defaultLabel);
    }
    Bind(&defaultLabel);
    {
        GateRef hClass = LoadHClass(*result);
        SetPropertyInlinedProps(glue, *result, hClass, Int16BuildTaggedWithNoGC(length),
            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef envHandle = GetVregValue(sp, ZExtInt8ToPtr(v0));
        SetLexicalEnvToFunction(glue, *result, envHandle);
        GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
        SetModuleToFunction(glue, *result, GetModuleFromFunction(currentFunc));
        varAcc = *result;
        DISPATCH_WITH_ACC(PREF_ID16_IMM16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineNCFuncDynPrefId16Imm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst16_3(pc);
    GateRef v0 = ReadInst8_5(pc);
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId)));
    Label isResolved(env);
    Label notResolved(env);
    Label defaultLabel(env);
    Branch(FunctionIsResolved(*result), &isResolved, &notResolved);
    Bind(&isResolved);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        result = CallRuntime(glue, RTSTUB_ID(DefineNCFuncDyn), { *result });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            SetConstantPoolToFunction(glue, *result, constpool);
            Jump(&defaultLabel);
        }
    }
    Bind(&notResolved);
    {
        SetResolvedToFunction(glue, *result, Boolean(true));
        Jump(&defaultLabel);
    }
    Bind(&defaultLabel);
    {
        GateRef hClass = LoadHClass(*result);
        SetPropertyInlinedProps(glue, *result, hClass, Int16BuildTaggedWithNoGC(length),
            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef lexEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
        SetLexicalEnvToFunction(glue, *result, lexEnv);
        SetHomeObjectToFunction(glue, *result, acc);
        GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
        SetModuleToFunction(glue, *result, GetModuleFromFunction(currentFunc));
        varAcc = *result;
        DISPATCH_WITH_ACC(PREF_ID16_IMM16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineGeneratorFuncPrefId16Imm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst16_3(pc);
    GateRef v0 = ReadInst8_5(pc);
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId)));
    Label isResolved(env);
    Label notResolved(env);
    Label defaultLabel(env);
    Branch(FunctionIsResolved(*result), &isResolved, &notResolved);
    Bind(&isResolved);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        result = CallRuntime(glue, RTSTUB_ID(DefineGeneratorFunc), { *result });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            SetConstantPoolToFunction(glue, *result, constpool);
            Jump(&defaultLabel);
        }
    }
    Bind(&notResolved);
    {
        SetResolvedToFunction(glue, *result, Boolean(true));
        Jump(&defaultLabel);
    }
    Bind(&defaultLabel);
    {
        GateRef hClass = LoadHClass(*result);
        SetPropertyInlinedProps(glue, *result, hClass, Int16BuildTaggedWithNoGC(length),
            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef lexEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
        SetLexicalEnvToFunction(glue, *result, lexEnv);
        GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
        SetModuleToFunction(glue, *result, GetModuleFromFunction(currentFunc));
        varAcc = *result;
        DISPATCH_WITH_ACC(PREF_ID16_IMM16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineAsyncFuncPrefId16Imm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst16_3(pc);
    GateRef v0 = ReadInst8_5(pc);
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId)));
    Label isResolved(env);
    Label notResolved(env);
    Label defaultLabel(env);
    Branch(FunctionIsResolved(*result), &isResolved, &notResolved);
    Bind(&isResolved);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        result = CallRuntime(glue, RTSTUB_ID(DefineAsyncFunc), { *result });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            SetConstantPoolToFunction(glue, *result, constpool);
            Jump(&defaultLabel);
        }
    }
    Bind(&notResolved);
    {
        SetResolvedToFunction(glue, *result, Boolean(true));
        Jump(&defaultLabel);
    }
    Bind(&defaultLabel);
    {
        GateRef hClass = LoadHClass(*result);
        SetPropertyInlinedProps(glue, *result, hClass, Int16BuildTaggedWithNoGC(length),
            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef lexEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
        SetLexicalEnvToFunction(glue, *result, lexEnv);
        GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
        SetModuleToFunction(glue, *result, GetModuleFromFunction(currentFunc));
        varAcc = *result;
        DISPATCH_WITH_ACC(PREF_ID16_IMM16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineMethodPrefId16Imm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef methodId = ReadInst16_1(pc);
    GateRef length = ReadInst16_3(pc);
    GateRef v0 = ReadInst8_5(pc);
    DEFVARIABLE(result, VariableType::JS_POINTER(),
        GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId)));
    Label isResolved(env);
    Label notResolved(env);
    Label defaultLabel(env);
    Branch(FunctionIsResolved(*result), &isResolved, &notResolved);
    Bind(&isResolved);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        result = CallRuntime(glue, RTSTUB_ID(DefineMethod), { *result, acc });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(*result), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST();
        }
        Bind(&notException);
        {
            SetConstantPoolToFunction(glue, *result, constpool);
            Jump(&defaultLabel);
        }
    }
    Bind(&notResolved);
    {
        SetHomeObjectToFunction(glue, *result, acc);
        SetResolvedToFunction(glue, *result, Boolean(true));
        Jump(&defaultLabel);
    }
    Bind(&defaultLabel);
    {
        GateRef hClass = LoadHClass(*result);
        SetPropertyInlinedProps(glue, *result, hClass, Int16BuildTaggedWithNoGC(length),
            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef lexEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
        SetLexicalEnvToFunction(glue, *result, lexEnv);
        GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
        SetModuleToFunction(glue, *result, GetModuleFromFunction(currentFunc));
        varAcc = *result;
        DISPATCH_WITH_ACC(PREF_ID16_IMM16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleCallSpreadDynPrefV8V8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef array = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CallSpreadDyn), { func, obj, array });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    {
        varAcc = res;
        DISPATCH_WITH_ACC(PREF_V8_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleAsyncFunctionResolvePrefV8V8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedTrue() });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    {
        varAcc = res;
        DISPATCH_WITH_ACC(PREF_V8_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleAsyncFunctionRejectPrefV8V8V8)
{
    auto env = GetEnvironment();
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue,
                              RTSTUB_ID(AsyncFunctionResolveOrReject),
                              { asyncFuncObj, value, TaggedFalse() });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    {
        DISPATCH(PREF_V8_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineGetterSetterByValuePrefV8V8V8V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_2(pc)));
    GateRef getter = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef setter = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_4(pc)));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(DefineGetterSetterByValue),
                              { obj, prop, getter, setter, acc }); // acc is flag
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    {
        varAcc = res;
        DISPATCH_WITH_ACC(PREF_V8_V8_V8_V8);
    }
}

DECLARE_ASM_HANDLER(HandleSuperCallPrefImm16V8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef range = ReadInst16_1(pc);
    GateRef v0 = ZExtInt8ToInt16(ReadInst8_3(pc));
    SetPcToFrame(glue, GetFrame(sp), pc);
    // acc is thisFunc
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuperCall),
        { acc, Int16BuildTaggedTypeWithNoGC(v0), Int16BuildTaggedTypeWithNoGC(range) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    {
        varAcc = res;
        DISPATCH_WITH_ACC(PREF_IMM16_V8);
    }
}

DECLARE_ASM_HANDLER(HandleGetPropIteratorPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(GetPropIterator), { *varAcc });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleAsyncFunctionEnterPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionEnter), {});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleLdHolePref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Hole();
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleCreateEmptyObjectPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateEmptyObject), {});
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleCreateEmptyArrayPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateEmptyArray), {});
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleGetIteratorPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef res = CallRuntime(glue, RTSTUB_ID(GetIterator), { *varAcc });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    CallRuntime(glue, RTSTUB_ID(ThrowThrowNotExists), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleThrowPatternNonCoerciblePref)
{
    SetPcToFrame(glue, GetFrame(sp), pc);
    CallRuntime(glue, RTSTUB_ID(ThrowPatternNonCoercible), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleLdHomeObjectPref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef thisFunc = GetFunctionFromFrame(GetFrame(sp));
    varAcc = GetHomeObjectFromJSFunction(thisFunc);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleThrowDeleteSuperPropertyPref)
{
    SetPcToFrame(glue, GetFrame(sp), pc);
    CallRuntime(glue, RTSTUB_ID(ThrowDeleteSuperProperty), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleDebuggerPref)
{
    DISPATCH(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleMul2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    // fast path
    result = FastMul(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Mul2Dyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    // fast path
    result = FastDiv(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Div2Dyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    // fast path
    result = FastMod(glue, left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(Mod2Dyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    // fast path
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(EqDyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    // fast path
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    result = FastEqual(left, acc);
    Label isHole(env);
    Label notHole(env);
    Label dispatch(env);
    Branch(TaggedIsHole(*result), &isHole, &notHole);
    Bind(&isHole);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        result = CallRuntime(glue, RTSTUB_ID(NotEqDyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
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
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        GateRef result = CallRuntime(glue, RTSTUB_ID(LessDyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
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
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        GateRef result = CallRuntime(glue, RTSTUB_ID(LessEqDyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
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
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        GateRef result = CallRuntime(glue, RTSTUB_ID(GreaterDyn), { left, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
                DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
                DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
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
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        GateRef result = CallRuntime(glue, RTSTUB_ID(GreaterEqDyn), { left, acc });
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
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, IntPtr(0));
}

DECLARE_ASM_HANDLER(SingleStepDebugging)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef tmpFrame = GetFrame(*varSp);
    SetPcToFrame(glue, tmpFrame, *varPc);
    varPc = TaggedCastToIntPtr(CallRuntime(glue,
                                           RTSTUB_ID(JumpToCInterpreter),
                                           { constpool, profileTypeInfo, acc,
                                             IntBuildTaggedTypeWithNoGC(hotnessCounter)}));
    Label shouldReturn(env);
    Label shouldContinue(env);

    Branch(IntPtrEqual(*varPc, IntPtr(0)), &shouldReturn, &shouldContinue);
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
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(VariableType::INT32(), method,
                                 IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET));
    }
    Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
             *varHotnessCounter, IntPtr(0));
}

DECLARE_ASM_HANDLER(HandleOverflow)
{
    FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(OPCODE_OVERFLOW)) });
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, IntPtr(0));
}

DECLARE_ASM_HANDLER(HandleLdaDynV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef offset = ReadInstSigned8_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    UPDATE_HOTNESS(sp);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleJmpImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef offset = ReadInstSigned16_0(pc);
    Label dispatch(env);
    Label slowPath(env);

    UPDATE_HOTNESS(sp);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleJmpImm32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    GateRef offset = ReadInstSigned32_0(pc);
    Label dispatch(env);
    Label slowPath(env);
    UPDATE_HOTNESS(sp);
    Dispatch(glue, pc, sp, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
}

DECLARE_ASM_HANDLER(HandleLdLexVarDynPrefImm4Imm4)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
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
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
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
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
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
    DEFVARIABLE(currentEnv, VariableType::JS_ANY(), GetEnvFromFrame(state));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, Int32(1));
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label slowPath(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueNoOverflow(env);
        Branch(Int32Equal(valueInt, Int32(INT32_MAX)), &valueNotInt, &valueNoOverflow);
        Bind(&valueNoOverflow);
        {
            varAcc = IntBuildTaggedWithNoGC(Int32Add(valueInt, Int32(1)));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    if (!env->IsAmd64()) {
        Label valueIsDouble(env);
        Label valueNotDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = TaggedCastToDouble(value);
            varAcc = DoubleBuildTaggedWithNoGC(DoubleAdd(valueDouble, Double(1.0)));
            Jump(&accDispatch);
        }
        Bind(&valueNotDouble);
        Jump(&slowPath);
    } else {
        Jump(&slowPath);
    }
    Bind(&slowPath);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        GateRef result = CallRuntime(glue, RTSTUB_ID(IncDyn), { value });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label slowPath(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueNoOverflow(env);
        Branch(Int32Equal(valueInt, Int32(INT32_MIN)), &valueNotInt, &valueNoOverflow);
        Bind(&valueNoOverflow);
        {
            varAcc = IntBuildTaggedWithNoGC(Int32Sub(valueInt, Int32(1)));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    if (!env->IsAmd64()) {
        Label valueIsDouble(env);
        Label valueNotDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = TaggedCastToDouble(value);
            varAcc = DoubleBuildTaggedWithNoGC(DoubleSub(valueDouble, Double(1.0)));
            Jump(&accDispatch);
        }
        Bind(&valueNotDouble);
        Jump(&slowPath);
    } else {
        Jump(&slowPath);
    }
    Bind(&slowPath);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        // slow path
        GateRef result = CallRuntime(glue, RTSTUB_ID(DecDyn), { value });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef base = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(ExpDyn), { base, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(IsInDyn), { prop, acc }); // acc is obj
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(InstanceOfDyn), { obj, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(FastStrictNotEqual), { left, acc });
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleStrictEqDynPrefV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(FastStrictEqual), { left, acc }); // acc is right
    varAcc = result;
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleResumeGeneratorPrefV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef vs = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(vs));
    GateRef resumeResultOffset = IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
    varAcc = Load(VariableType::JS_ANY(), obj, resumeResultOffset);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleGetResumeModePrefV8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef vs = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(vs));
    varAcc = IntBuildTaggedWithNoGC(GetResumeModeFromGeneratorObject(obj));
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleCreateGeneratorObjPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef genFunc = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateGeneratorObj), { genFunc });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    CallRuntime(glue, RTSTUB_ID(ThrowConstAssignment), { value });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleGetTemplateObjectPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef literal = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetTemplateObject), { literal });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetNextPropName), { iter });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    CallRuntime(glue, RTSTUB_ID(ThrowIfNotObject), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleIterNextPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(IterNext), { iter });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CloseIterator), { iter });
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
    DISPATCH(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleSuperCallSpreadPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef array = GetVregValue(sp, ZExtInt8ToPtr(v0));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(SuperCallSpread), { acc, array });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(DelObjProp), { obj, prop });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef newTarget = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(NewObjSpreadDyn), { func, newTarget, acc }); // acc is array
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef flag = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CreateIterResultObj), { value, flag });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(AsyncFunctionAwaitUncaught), { asyncFuncObj, value });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    // assert obj.IsString()
    CallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), { obj });
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleCopyDataPropertiesPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef dst = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef src = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(CopyDataProperties), { dst, src });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef dst = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef index = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StArraySpread), { dst, index, acc }); // acc is res
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef method = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(GetIteratorNext), { obj, method });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(SetObjectWithProto), { proto, obj });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    Label receiverIsHeapObject(env);
    Label slowPath(env);
    Label isException(env);
    Label accDispatch(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label loadElement(env);
                Label tryPoly(env);
                GateRef secondValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo,
                    Int32Add(slotId, Int32(1)));
                DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
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
            GateRef result = CallStub(glue,
                                      CommonStubCSigns::GetPropertyByValue,
                                      { glue, receiver, propKey });
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
        GateRef result = CallRuntime(glue, RTSTUB_ID(LoadICByValue),
                                     { profileTypeInfo, receiver, propKey, IntBuildTaggedTypeWithNoGC(slotId) });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label storeElement(env);
                Label tryPoly(env);
                GateRef secondValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo,
                    Int32Add(slotId, Int32(1)));
                DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
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
            GateRef result = CallStub(glue,
                CommonStubCSigns::SetPropertyByValue,
                { glue, receiver, propKey, acc }); // acc is value
            Label notHole(env);
            Branch(TaggedIsHole(result), &slowPath, &notHole);
            Bind(&notHole);
            Branch(TaggedIsException(result), &isException, &notException);
        }
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StoreICByValue),
                                     { profileTypeInfo, receiver, propKey, acc, IntBuildTaggedTypeWithNoGC(slotId) });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = CallStub(glue, CommonStubCSigns::SetPropertyByValue,
                                  { glue, receiver, propKey, acc }); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetVregValue(sp, ZExtInt8ToPtr(v1));
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), {  receiver, propKey }); // sp for thisFunc
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
     // acc is value, sp for thisFunc
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
    Label isException(env);
    Label notException(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isException(env);
    Label dispatch(env);

    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ReadInst8_5(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(LdSuperByValue), { receiver, propKey });
    Branch(TaggedIsException(result), &isException, &dispatch);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StSuperByValue), { receiver, propKey, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef index = ReadInst32_2(pc);
    Label fastPath(env);
    Label slowPath(env);
    Label isException(env);
    Label accDispatch(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = CallStub(glue, CommonStubCSigns::GetPropertyByIndex,
                                  { glue, receiver, index });
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
        GateRef result = CallRuntime(glue, RTSTUB_ID(LdObjByIndex),
                                     { receiver, IntBuildTaggedTypeWithNoGC(index), TaggedFalse(), Undefined() });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        GateRef result = CallStub(glue, CommonStubCSigns::SetPropertyByIndex,
                                  { glue, receiver, index, acc }); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StObjByIndex),
                                     { receiver, IntBuildTaggedTypeWithNoGC(index), acc });
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
    SetPcToFrame(glue, GetFrame(sp), pc);
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
        GateRef result = CallStub(glue, CommonStubCSigns::SetPropertyByIndex,
                                  { glue, receiver, index, acc }); // acc is value
        Label notHole(env);
        Branch(TaggedIsHole(result), &slowPath, &notHole);
        Bind(&notHole);
        Branch(TaggedIsException(result), &isException, &notException);
    }
    Bind(&slowPath);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(StOwnByIndex),
                                     { receiver, IntBuildTaggedTypeWithNoGC(index), acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedTrue() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&notException);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStLetToGlobalRecordPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord),
                                 { propKey, *varAcc, TaggedFalse() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&notException);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStClassToGlobalRecordPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef result = CallRuntime(glue, RTSTUB_ID(StGlobalRecord), { propKey, *varAcc, TaggedFalse() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&notException);
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleNegDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

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
        Branch(Int32Equal(valueInt, Int32(0)), &valueIsZero, &valueNotZero);
        Bind(&valueIsZero);
        {
            // Format::PREF_V8： size = 3
            varAcc = DoubleBuildTaggedWithNoGC(Double(-0.0));
            Jump(&accDispatch);
        }
        Bind(&valueNotZero);
        {
            varAcc = IntBuildTaggedWithNoGC(Int32Sub(Int32(0), valueInt));
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
            varAcc = DoubleBuildTaggedWithNoGC(DoubleSub(Double(0), valueDouble));
            Jump(&accDispatch);
        }
        Label isException(env);
        Label notException(env);
        Bind(&valueNotDouble);
        {
            SetPcToFrame(glue, GetFrame(sp), pc);
            // slow path
            GateRef result = CallRuntime(glue, RTSTUB_ID(NegDyn), { value });
            Branch(TaggedIsException(result), &isException, &notException);
            Bind(&isException);
            {
                DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef vsrc = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(vsrc));
    DEFVARIABLE(number, VariableType::INT32(), Int32(0));
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
            number = DoubleToInt(glue, valueDouble);
            varAcc = IntBuildTaggedWithNoGC(Int32Not(*number));
            Jump(&accDispatch);
        }
        Bind(&numberNotDouble);
        {
            SetPcToFrame(glue, GetFrame(sp), pc);
            // slow path
            GateRef result = CallRuntime(glue, RTSTUB_ID(NotDyn), { value });
            Label isException(env);
            Label notException(env);
            Branch(TaggedIsException(result), &isException, &notException);
            Bind(&isException);
            {
                DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

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
                    opNumber1 = DoubleToInt(glue, rightDouble);
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
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(And2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

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
                    opNumber1 = DoubleToInt(glue, rightDouble);
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
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Or2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

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
                    opNumber1 = DoubleToInt(glue, rightDouble);
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
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Xor2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

    Label accDispatch(env);
    Label doShr(env);
    Label overflow(env);
    Label notOverflow(env);
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
                    Jump(&doShr);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    opNumber0 = TaggedCastToInt32(left);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&doShr);
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
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&doShr);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&doShr);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Ashr2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            Jump(&accDispatch);
        }
    }
    Bind(&doShr);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef ret = UInt32LSR(*opNumber0, shift);
        auto condition = UInt32GreaterThan(ret, Int32(INT32_MAX));
        Branch(condition, &overflow, &notOverflow);
        Bind(&overflow);
        {
            varAcc = DoubleBuildTaggedWithNoGC(ChangeUInt32ToFloat64(ret));
            Jump(&accDispatch);
        }
        Bind(&notOverflow);
        {
            varAcc = IntBuildTaggedWithNoGC(ret);
            Jump(&accDispatch);
        }
    }
    Bind(&accDispatch);
    {
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleShr2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

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
                    opNumber1 = DoubleToInt(glue, rightDouble);
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
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Shr2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
        }
        Bind(&notException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef ret = Int32ASR(*opNumber0, shift);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}
DECLARE_ASM_HANDLER(HandleShl2DynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;
    DEFVARIABLE(opNumber0, VariableType::INT32(), Int32(0));
    DEFVARIABLE(opNumber1, VariableType::INT32(), Int32(0));

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
                    opNumber1 = DoubleToInt(glue, rightDouble);
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
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = TaggedCastToInt32(right);
                    Jump(&accDispatch);
                }
                Bind(&rightIsDouble);
                {
                    GateRef rightDouble = TaggedCastToDouble(right);
                    GateRef leftDouble = TaggedCastToDouble(left);
                    opNumber0 = DoubleToInt(glue, leftDouble);
                    opNumber1 = DoubleToInt(glue, rightDouble);
                    Jump(&accDispatch);
                }
            }
        }
    }
    // slow path
    Bind(&leftNotNumberOrRightNotNumber);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Shl2Dyn), { left, right });
        Label IsException(env);
        Label NotException(env);
        Branch(TaggedIsException(taggedNumber), &IsException, &NotException);
        Bind(&IsException);
        {
            DISPATCH_LAST_WITH_ACC();
        }
        Bind(&NotException);
        {
            varAcc = taggedNumber;
            DISPATCH_WITH_ACC(PREF_V8);
        }
    }
    Bind(&accDispatch);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef ret = Int32LSL(*opNumber0, shift);
        varAcc = IntBuildTaggedWithNoGC(ret);
        DISPATCH_WITH_ACC(PREF_V8);
    }
}

DECLARE_ASM_HANDLER(HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef methodId = ReadInst16_1(pc);
    GateRef literalId = ReadInst16_3(pc);
    GateRef length = ReadInst16_5(pc);
    GateRef v0 = ReadInst8_7(pc);
    GateRef v1 = ReadInst8_8(pc);

    GateRef classTemplate = GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId));
    GateRef literalBuffer = GetObjectFromConstPool(constpool, ZExtInt16ToInt32(literalId));
    GateRef lexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v1));

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label isResolved(env);
    Label isNotResolved(env);
    Label afterCheckResolved(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(FunctionIsResolved(classTemplate), &isResolved, &isNotResolved);
    Bind(&isResolved);
    {
        res = CallRuntime(glue, RTSTUB_ID(CloneClassFromTemplate), { classTemplate, proto, lexicalEnv, constpool });
        Jump(&afterCheckResolved);
    }
    Bind(&isNotResolved);
    {
        res = CallRuntime(glue, RTSTUB_ID(ResolveClass),
                          { classTemplate, literalBuffer, proto, lexicalEnv, constpool });
        Jump(&afterCheckResolved);
    }
    Bind(&afterCheckResolved);
    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(*res), &isException, &isNotException);
    Bind(&isException);
    {
        DISPATCH_LAST_WITH_ACC();
    }
    Bind(&isNotException);
    GateRef newLexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));  // slow runtime may gc
    SetLexicalEnvToFunction(glue, *res, newLexicalEnv);
    CallRuntime(glue, RTSTUB_ID(SetClassConstructorLength), { *res, Int16BuildTaggedTypeWithNoGC(length) });
    varAcc = *res;
    DISPATCH_WITH_ACC(PREF_ID16_IMM16_IMM16_V8_V8);
}

DECLARE_ASM_HANDLER(HandleLdObjByNamePrefId32V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());

    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    Label receiverIsHeapObject(env);
    Label dispatch(env);
    Label slowPath(env);
    Label notHole(env);
    Label hasException(env);
    Label notException(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label loadWithHandler(env);
                GateRef secondValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo,
                    Int32Add(slotId, Int32(1)));
                DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
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
                    result = LoadICWithHandler(glue, receiver, receiver, *cachedHandler);
                    Branch(TaggedIsHole(*result), &slowPath, &notHole);
                }
            }
        }
        Bind(&tryFastPath);
        {
            GateRef stringId = ReadInst32_1(pc);
            GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
            result = CallStub(glue,
                CommonStubCSigns::GetPropertyByName, {
                glue, receiver, propKey
            });
            Branch(TaggedIsHole(*result), &slowPath, &notHole);
        }
        Bind(&notHole);
        {
            Branch(TaggedIsException(*result), &hasException, &notException);
            Bind(&hasException);
            {
                DISPATCH_LAST_WITH_ACC();
            }
            Bind(&notException);
            varAcc = *result;
            Jump(&dispatch);
        }
    }
    Bind(&slowPath);
    {
        Label isException(env);
        Label noException(env);
        GateRef stringId = ReadInst32_1(pc);
        GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
        result = CallRuntime(glue, RTSTUB_ID(LoadICByName),
                             { profileTypeInfo, receiver, propKey, IntBuildTaggedTypeWithNoGC(slotId) });
        Branch(TaggedIsException(*result), &isException, &noException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
        }
        Bind(&noException);
        varAcc = *result;
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
    DEFVARIABLE(result, VariableType::INT64(), Hole(VariableType::INT64()));
    SetPcToFrame(glue, GetFrame(sp), pc);

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
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label storeWithHandler(env);
                GateRef secondValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo,
                    Int32Add(slotId, Int32(1)));
                DEFVARIABLE(cachedHandler, VariableType::JS_ANY(), secondValue);
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
            GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
            result = CallStub(glue, CommonStubCSigns::SetPropertyByName, {
                glue, receiver, propKey, acc
            });
            Branch(TaggedIsHole(*result), &slowPath, &checkResult);
        }
    }
    Bind(&slowPath);
    {
        GateRef stringId = ReadInst32_1(pc);
        GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
        result = ChangeTaggedPointerToInt64(CallRuntime(glue, RTSTUB_ID(StoreICByName),
                                                        { profileTypeInfo, receiver, propKey, acc,
                                                          IntBuildTaggedTypeWithNoGC(slotId) }));
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
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    Bind(&isHeapObject);
    {
        Branch(IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = CallStub(glue,
                    CommonStubCSigns::SetPropertyByValue,
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
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    DISPATCH(PREF_V8_V8);
                }
            }
        }
    }
    Bind(&slowPath);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, acc });
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
    GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    DEFVARIABLE(result, VariableType::INT64(), Hole(VariableType::INT64()));
    SetPcToFrame(glue, GetFrame(sp), pc);
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
        result = ChangeTaggedPointerToInt64(CallRuntime(glue, RTSTUB_ID(StOwnByName), { receiver, propKey, acc }));
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
    GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label isException(env);
    Label notException(env);
    Label isException1(env);
    Label notException1(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
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
                    CallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { acc, propKey });
                    DISPATCH(PREF_ID32_V8);
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        GateRef res = CallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_0(pc);
    varAcc = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    DISPATCH_WITH_ACC(ID32);
}

DECLARE_ASM_HANDLER(HandleLdaiDynImm32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef imm = ReadInst32_0(pc);
    varAcc = IntBuildTaggedWithNoGC(imm);
    DISPATCH_WITH_ACC(IMM32);
}

DECLARE_ASM_HANDLER(HandleFldaiDynImm64)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef imm = CastInt64ToFloat64(ReadInst64_0(pc));
    varAcc = DoubleBuildTaggedWithNoGC(imm);
    DISPATCH_WITH_ACC(IMM64);
}

DECLARE_ASM_HANDLER(HandleJeqzImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), Double(0)), &accEqualFalse, &last);
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
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

DECLARE_ASM_HANDLER(HandleJeqzImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), Double(0)), &accEqualFalse, &last);
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
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16)));
}

DECLARE_ASM_HANDLER(HandleJnezImm8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), Double(0)), &last, &accEqualTrue);
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
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

DECLARE_ASM_HANDLER(HandleJnezImm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_ANY(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
            Branch(Int32Equal(TaggedGetInt(acc), Int32(0)), &accNotInt, &accEqualTrue);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), Double(0)), &last, &accEqualTrue);
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
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16)));
}

DECLARE_ASM_HANDLER(HandleReturnDyn)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(VariableType::POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(IntPtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        Store(VariableType::INT32(), glue, method,
              IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET), *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    varSp = Load(VariableType::POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        SetAccToFrame(glue, frame, acc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
        DispatchCommonCall<RTSTUB_ID(ResumeRspAndReturn)>(glue, *varSp);
#else
        Return();
#endif
    }
    Bind(&pcNotEqualNullptr);
    {
        SetCurrentSpFrame(glue, *varSp);
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(VariableType::INT32(), method,
                                 IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET));
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
        DispatchCommonCall<RTSTUB_ID(ResumeRspAndDispatch)>(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo,
            acc, *varHotnessCounter, jumpSize);
#else
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, acc,
                 *varHotnessCounter, jumpSize);
#endif
    }
}

DECLARE_ASM_HANDLER(HandleReturnUndefinedPref)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(VariableType::POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(IntPtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        Store(VariableType::INT32(), glue, method,
              IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET), *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    varSp = Load(VariableType::POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    varAcc = Undefined();
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        SetAccToFrame(glue, frame, *varAcc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
        DispatchCommonCall<RTSTUB_ID(ResumeRspAndReturn)>(glue, *varSp);
#else
        Return();
#endif
    }
    Bind(&pcNotEqualNullptr);
    {
        SetCurrentSpFrame(glue, *varSp);
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(VariableType::INT32(), method,
                                 IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET));
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
        DispatchCommonCall<RTSTUB_ID(ResumeRspAndDispatch)>(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo,
            *varAcc, *varHotnessCounter, jumpSize);
#else
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
                 *varHotnessCounter, jumpSize);
#endif
    }
}

DECLARE_ASM_HANDLER(HandleSuspendGeneratorPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

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
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
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
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(VariableType::POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(IntPtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        Store(VariableType::INT32(), glue, method,
              IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET), *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    varSp = Load(VariableType::POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
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
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(VariableType::INT32(), method,
                                 IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET));
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
                 *varHotnessCounter, jumpSize);
    }
}

DECLARE_ASM_HANDLER(ExceptionHandler)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    Label pcIsInvalid(env);
    Label pcNotInvalid(env);
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    varPc = TaggedCastToIntPtr(CallRuntime(glue, RTSTUB_ID(UpFrame), {}));
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcIsInvalid, &pcNotInvalid);
    Bind(&pcIsInvalid);
    {
        Return();
    }
    Bind(&pcNotInvalid);
    {
        varSp = GetCurrentSpFrame(glue);
        varAcc = exception;
        // clear exception
        Store(VariableType::INT64(), glue, glue, IntPtr(0), Hole());
        GateRef function = GetFunctionFromFrame(GetFrame(*varSp));
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = Load(VariableType::INT32(), method,
                                 IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET));
        Dispatch(glue, *varPc, *varSp, *varConstpool, *varProfileTypeInfo, *varAcc,
                 *varHotnessCounter, IntPtr(0));
    }
}

DECLARE_ASM_HANDLER(HandleGetModuleNamespacePrefId32)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);
    GateRef moduleRef = CallRuntime(glue, RTSTUB_ID(GetModuleNamespace), { prop });
    varAcc = moduleRef;
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleStModuleVarPrefId32)
{
    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);
    GateRef value = acc;

    CallRuntime(glue, RTSTUB_ID(StModuleVar), { prop, value });
    DISPATCH(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleLdModuleVarPrefId32Imm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef flag = ReadInst8_5(pc);
    GateRef key = GetObjectFromConstPool(constpool, stringId);
    GateRef innerFlag = ZExtInt8ToInt32(flag);
    GateRef moduleVar = CallRuntime(glue, RTSTUB_ID(LdModuleVar), { key, IntBuildTaggedTypeWithNoGC(innerFlag) });
    varAcc = moduleVar;
    DISPATCH_WITH_ACC(PREF_ID32_IMM8);
}

DECLARE_ASM_HANDLER(HandleTryLdGlobalByNamePrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);

    Label dispatch(env);
    Label icAvailable(env);
    Label icNotAvailable(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        DEFVARIABLE(icResult, VariableType::JS_ANY(), Undefined());
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
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
            icResult = CallRuntime(glue, RTSTUB_ID(LoadMiss),
                                   { profileTypeInfo, globalObject, prop, IntBuildTaggedTypeWithNoGC(slotId),
                                     IntBuildTaggedTypeWithNoGC(Int32(static_cast<int>(ICKind::NamedGlobalLoadIC))) });
            Jump(&icResultCheck);
        }
        Bind(&icResultCheck);
        {
            Label isException(env);
            Label isNotException(env);
            Branch(TaggedIsException(*icResult), &isException, &isNotException);
            Bind(&isException);
            {
                DISPATCH_LAST_WITH_ACC();
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
        GateRef recordResult = CallRuntime(glue, RTSTUB_ID(LdGlobalRecord), { prop });
        Label isFound(env);
        Label isNotFound(env);
        Branch(TaggedIsUndefined(recordResult), &isNotFound, &isFound);
        Bind(&isNotFound);
        {
            GateRef globalResult = CallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { prop });
            Label isFoundInGlobal(env);
            Label slowPath(env);
            Branch(TaggedIsHole(globalResult), &slowPath, &isFoundInGlobal);
            Bind(&slowPath);
            {
                GateRef slowResult = CallRuntime(glue, RTSTUB_ID(TryLdGlobalByName), { prop });
                Label isException(env);
                Label isNotException(env);
                Branch(TaggedIsException(slowResult), &isException, &isNotException);
                Bind(&isException);
                {
                    DISPATCH_LAST_WITH_ACC();
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
            varAcc = Load(VariableType::JS_ANY(), recordResult, IntPtr(PropertyBox::VALUE_OFFSET));
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
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label checkResult(env);
    Label dispatch(env);

    Label icAvailable(env);
    Label icNotAvailable(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
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
            result = CallRuntime(glue, RTSTUB_ID(StoreMiss),
                                 { profileTypeInfo, globalObject, propKey, acc, IntBuildTaggedTypeWithNoGC(slotId),
                                   IntBuildTaggedTypeWithNoGC(Int32(static_cast<int>(ICKind::NamedGlobalStoreIC))) });
            Jump(&checkResult);
        }
    }
    Bind(&icNotAvailable);
    // order: 1. global record 2. global object
    // if we find a way to get global record, we can inline LdGlobalRecord directly
    GateRef recordInfo = CallRuntime(glue, RTSTUB_ID(LdGlobalRecord), { propKey });
    Label isFound(env);
    Label isNotFound(env);
    Branch(TaggedIsUndefined(recordInfo), &isNotFound, &isFound);
    Bind(&isFound);
    {
        result = CallRuntime(glue, RTSTUB_ID(TryUpdateGlobalRecord), { propKey, acc });
        Jump(&checkResult);
    }
    Bind(&isNotFound);
    {
        Label foundInGlobal(env);
        Label notFoundInGlobal(env);
        GateRef globalResult = CallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { propKey });
        Branch(TaggedIsHole(globalResult), &notFoundInGlobal, &foundInGlobal);
        Bind(&notFoundInGlobal);
        {
            result = CallRuntime(glue, RTSTUB_ID(ThrowReferenceError), { propKey });
            DISPATCH_LAST();
        }
        Bind(&foundInGlobal);
        {
            result = CallRuntime(glue, RTSTUB_ID(StGlobalVar), { propKey, acc });
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetObjectFromConstPool(constpool, stringId);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label checkResult(env);
    Label dispatch(env);
    Label slowPath(env);
    GateRef globalObject = GetGlobalObject(glue);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Label icAvailable(env);
    Label icNotAvailable(env);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
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
            result = CallRuntime(glue, RTSTUB_ID(LoadMiss),
                                 { profileTypeInfo, globalObject, propKey, IntBuildTaggedTypeWithNoGC(slotId),
                                   IntBuildTaggedTypeWithNoGC(Int32(static_cast<int>(ICKind::NamedGlobalLoadIC))) });
            Jump(&checkResult);
        }
    }
    Bind(&icNotAvailable);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { propKey });
        Branch(TaggedIsHole(*result), &slowPath, &dispatch);
        Bind(&slowPath);
        {
            result = CallRuntime(glue, RTSTUB_ID(LdGlobalVar), { globalObject, propKey });
            Jump(&checkResult);
        }
    }
    Bind(&checkResult);
    {
        Label isException(env);
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label checkResult(env);
    Label dispatch(env);

    Label icAvailable(env);
    Label icNotAvailable(env);
    SetPcToFrame(glue, GetFrame(sp), pc);
    Branch(TaggedIsUndefined(profileTypeInfo), &icNotAvailable, &icAvailable);
    Bind(&icAvailable);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        GateRef handler = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
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
            result = CallRuntime(glue, RTSTUB_ID(StoreMiss),
                                 { profileTypeInfo, globalObject, propKey, acc, IntBuildTaggedTypeWithNoGC(slotId),
                                   IntBuildTaggedTypeWithNoGC(Int32(static_cast<int>(ICKind::NamedGlobalStoreIC))) });
            Jump(&checkResult);
        }
    }
    Bind(&icNotAvailable);
    {
        result = CallRuntime(glue, RTSTUB_ID(StGlobalVar), { propKey, acc });
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

DECLARE_ASM_HANDLER(HandleCreateRegExpWithLiteralPrefId32Imm8)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef stringId = ReadInst32_1(pc);
    GateRef pattern = GetObjectFromConstPool(constpool, stringId);
    GateRef flags = ReadInst8_5(pc);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(CreateRegExpWithLiteral),
                              { pattern, Int8BuildTaggedTypeWithNoGC(flags) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    {
        varAcc = res;
        DISPATCH_WITH_ACC(PREF_ID32_IMM8);
    }
}

DECLARE_ASM_HANDLER(HandleIsTruePref)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    varAcc = FastToBoolean(*varAcc);
    DISPATCH_WITH_ACC(PREF_NONE);
}

DECLARE_ASM_HANDLER(HandleIsFalsePref)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isTrue(env);
    Label isFalse(env);
    Label dispatch(env);
    auto result = FastToBoolean(*varAcc);
    Branch(TaggedIsTrue(result), &isTrue, &isFalse);
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
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
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef res = CallRuntime(glue, RTSTUB_ID(ToNumber), { value });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;

    GateRef result = FastAdd(left, right);
    Label notHole(env), slowPath(env);
    Label accDispatch(env);
    Branch(TaggedIsHole(result), &slowPath, &notHole);
    Bind(&notHole);
    {
        varAcc = result;
        Jump(&accDispatch);
    }
    // slow path
    Bind(&slowPath);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Add2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef right = *varAcc;

    GateRef result = FastSub(left, right);
    Label notHole(env), slowPath(env);
    Label accDispatch(env);
    Branch(TaggedIsHole(result), &slowPath, &notHole);
    Bind(&notHole);
    {
        varAcc = result;
        Jump(&accDispatch);
    }
    // slow path
    Bind(&slowPath);
    {
        SetPcToFrame(glue, GetFrame(sp), pc);
        GateRef taggedNumber = CallRuntime(glue, RTSTUB_ID(Sub2Dyn), { left, right });
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(taggedNumber), &isException, &notException);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
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

#define CALL_INITIALIZE()                                                                     \
    SetPcToFrame(glue, GetFrame(sp), pc);                                                     \
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));                                  \
    Label funcIsHeapObject(env);                                                              \
    Label funcIsCallable(env);                                                                \
    Label funcNotCallable(env);                                                               \
    Branch(TaggedIsHeapObject(func), &funcIsHeapObject, &funcNotCallable);                    \
    Bind(&funcIsHeapObject);                                                                  \
    Branch(IsCallable(func), &funcIsCallable, &funcNotCallable);                              \
    Bind(&funcNotCallable);                                                                   \
    {                                                                                         \
        CallRuntime(glue, RTSTUB_ID(ThrowNotCallableException), {});                          \
        DISPATCH_LAST();                                                                      \
    }                                                                                         \
    Bind(&funcIsCallable);                                                                    \
    DEFVARIABLE(methodOffset, VariableType::INT32(), Int32(0));                               \
    /* method = func->GetCallTarget() */                                                      \
    /* ASSERT(JSTaggedValue(func).IsJSFunctionBase() || JSTaggedValue(func).IsJSProxy()) */   \
    Label funcIsJSFunctionBase(env);                                                          \
    Label funcIsJSProxy(env);                                                                 \
    Label getMethod(env);                                                                     \
    Branch(IsJSFunctionBase(func), &funcIsJSFunctionBase, &funcIsJSProxy);                    \
    Bind(&funcIsJSFunctionBase);                                                              \
    {                                                                                         \
        methodOffset = Int32(JSFunctionBase::METHOD_OFFSET);                                  \
        Jump(&getMethod);                                                                     \
    }                                                                                         \
    Bind(&funcIsJSProxy);                                                                     \
    {                                                                                         \
        methodOffset = Int32(JSProxy::METHOD_OFFSET);                                         \
        Jump(&getMethod);                                                                     \
    }                                                                                         \
    Bind(&getMethod);                                                                         \
    GateRef method = Load(VariableType::POINTER(), func, ChangeInt32ToIntPtr(*methodOffset)); \
    GateRef callFieldOffset = IntPtr(JSMethod::GetCallFieldOffset(env->IsArch32Bit()));       \
    GateRef callField = Load(VariableType::INT64(), method, callFieldOffset);                 \
    DEFVARIABLE(newSp, VariableType::POINTER(),                                               \
                PointerSub(sp, IntPtr(AsmInterpretedFrame::GetSize(env->IsArch32Bit()))))

#define CALL_PUSH_UNDEFINED(n)                                            \
    i = Int32(0);                                                         \
    Label pushUndefined(env);                                             \
    Label pushUndefinedAgain(env);                                        \
    Label pushUndefinedEnd(env);                                          \
    Branch(Int32LessThan(*i, n), &pushUndefined, &pushUndefinedEnd);      \
    LoopBegin(&pushUndefined);                                            \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));             \
    Store(VariableType::INT64(), glue, *newSp, IntPtr(0),                 \
          Int64(JSTaggedValue::VALUE_UNDEFINED));                         \
    i = Int32Add(*i, Int32(1));                                           \
    Branch(Int32LessThan(*i, n), &pushUndefinedAgain, &pushUndefinedEnd); \
    Bind(&pushUndefinedAgain);                                            \
    LoopEnd(&pushUndefined);                                              \
    Bind(&pushUndefinedEnd)

#define CALL_PUSH_ARGS(format)                                                                                \
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));                                                          \
    GateRef isNativeMask = Int64(static_cast<uint64_t>(1) << JSMethod::IsNativeBit::START_BIT);               \
    Label methodIsNative(env);                                                                                \
    Label methodNotNative(env);                                                                               \
    Branch(Int64NotEqual(Int64And(callField, isNativeMask), Int64(0)), &methodIsNative, &methodNotNative);    \
    Bind(&methodIsNative);                                                                                    \
    {                                                                                                         \
        CALL_PUSH_ARGS_##format();                                                                            \
        SET_VREGS_AND_FRAME_NATIVE(format);                                                                   \
    }                                                                                                         \
    Bind(&methodNotNative);                                                                                   \
    GateRef numArgsOffset = Int64(JSMethod::NumArgsBits::START_BIT);                                          \
    GateRef numArgsMask = Int64((static_cast<uint64_t>(1) << JSMethod::NumArgsBits::SIZE) - 1);               \
    GateRef declaredNumArgs = ChangeInt64ToInt32(Int64And(UInt64LSR(callField, numArgsOffset), numArgsMask)); \
    Label fastPath(env);                                                                                      \
    Label slowPath(env);                                                                                      \
    Label setVregsAndFrameNotNative(env);                                                                     \
    Branch(Int32Equal(actualNumArgs, declaredNumArgs), &fastPath, &slowPath);                                 \
    Bind(&fastPath);                                                                                          \
    {                                                                                                         \
        CALL_PUSH_ARGS_##format();                                                                            \
        Jump(&setVregsAndFrameNotNative);                                                                     \
    }                                                                                                         \
    Bind(&slowPath);                                                                                          \
    GateRef haveExtraMask = Int64(static_cast<uint64_t>(1) << JSMethod::HaveExtraBit::START_BIT);             \
    Label methodNoExtra(env);                                                                                 \
    Label methodHaveExtra(env);                                                                               \
    Branch(Int64NotEqual(Int64And(callField, haveExtraMask), Int64(0)), &methodHaveExtra, &methodNoExtra);    \
    Bind(&methodNoExtra);                                                                                     \
    {                                                                                                         \
        GateRef undefinedNumArgs = Int32Sub(declaredNumArgs, actualNumArgs);                                  \
        CALL_PUSH_UNDEFINED(undefinedNumArgs);                                                                \
        CALL_PUSH_ARGS_NO_EXTRA_##format();                                                                   \
        Jump(&setVregsAndFrameNotNative);                                                                     \
    }                                                                                                         \
    Bind(&methodHaveExtra);                                                                                   \
    {                                                                                                         \
        newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                             \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), IntBuildTaggedTypeWithNoGC(actualNumArgs));     \
        GateRef undefinedNumArgs = Int32Sub(declaredNumArgs, actualNumArgs);                                  \
        CALL_PUSH_UNDEFINED(undefinedNumArgs);                                                                \
        CALL_PUSH_ARGS_##format();                                                                            \
        Jump(&setVregsAndFrameNotNative);                                                                     \
    }                                                                                                         \
    Bind(&setVregsAndFrameNotNative);                                                                         \
    SET_VREGS_AND_FRAME_NOT_NATIVE(format)

#define SET_VREGS_AND_FRAME_NATIVE(format)                                                        \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                     \
    Label pushThis(env);                                                                          \
    Label pushThisUndefined(env);                                                                 \
    Label pushNewTarget(env);                                                                     \
    Branch(callThis, &pushThis, &pushThisUndefined);                                              \
    Bind(&pushThis);                                                                              \
    {                                                                                             \
        GateRef thisValue = GetVregValue(sp, IntPtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)));       \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), thisValue);                         \
        Jump(&pushNewTarget);                                                                     \
    }                                                                                             \
    Bind(&pushThisUndefined);                                                                     \
    {                                                                                             \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0),                                     \
              Int64(JSTaggedValue::VALUE_UNDEFINED));                                             \
        Jump(&pushNewTarget);                                                                     \
    }                                                                                             \
    Bind(&pushNewTarget);                                                                         \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                     \
    Store(VariableType::INT64(), glue, *newSp, IntPtr(0),                                         \
          Int64(JSTaggedValue::VALUE_UNDEFINED));                                                 \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                     \
    Store(VariableType::INT64(), glue, *newSp, IntPtr(0), func);                                  \
    /* ASSERT(JSMethod::NumVregsBits::Decode(callField) == 0) */                                  \
    /* thread->DoStackOverflowCheck(newSp) */                                                     \
    GateRef frameBaseOffset = IntPtr(JSThread::GlueData::GetFrameBaseOffset(env->IsArch32Bit())); \
    GateRef frameBase = Load(VariableType::POINTER(), glue, frameBaseOffset);                     \
    Label stackOverflow(env);                                                                     \
    Label stackNotOverflow(env);                                                                  \
    Branch(UInt64LessThanOrEqual(*newSp, IntPtrAdd(frameBase,                                     \
        /* 2: double size in case */                                                              \
        IntPtr(JSThread::RESERVE_STACK_SIZE * sizeof(JSTaggedType) * 2))),                        \
        &stackOverflow, &stackNotOverflow);                                                       \
    Bind(&stackOverflow);                                                                         \
    {                                                                                             \
        CallRuntime(glue, RTSTUB_ID(ThrowStackOverflowException), {});                            \
        DISPATCH_LAST();                                                                          \
    }                                                                                             \
    Bind(&stackNotOverflow);                                                                      \
    GateRef state = GetFrame(*newSp);                                                             \
    GateRef prevOffset = IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit()));          \
    Store(VariableType::POINTER(), glue, state, prevOffset, sp);                                  \
    GateRef frameTypeOffset = IntPtrAdd(prevOffset, IntPtrSize());                                \
    Store(VariableType::INT64(), glue, state, frameTypeOffset,                                    \
          Int64(static_cast<uint64_t>(FrameType::INTERPRETER_FRAME)));                            \
    SetPcToFrame(glue, state, IntPtr(0));                                                         \
    SetFunctionToFrame(glue, state, func);                                                        \
    SetCurrentSpFrame(glue, *newSp);                                                              \
    GateRef numArgs = Int32Add(Int32(NUM_MANDATORY_JSFUNC_ARGS), actualNumArgs);                  \
    GateRef retValue = CallRuntime(glue, RTSTUB_ID(CallNative),                                   \
                                   {IntBuildTaggedTypeWithNoGC(numArgs), *newSp, method});        \
    Label hasPendingException(env);                                                               \
    Label noPendingException(env);                                                                \
    Branch(TaggedIsException(retValue), &hasPendingException, &noPendingException);               \
    Bind(&hasPendingException);                                                                   \
    {                                                                                             \
        SetCurrentSpFrame(glue, sp);  /* currentSp will be used in UpFrame, therefore use sp. */  \
        DISPATCH_LAST();                                                                          \
    }                                                                                             \
    Bind(&noPendingException);                                                                    \
    SetCurrentSpFrame(glue, sp);                                                                  \
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), retValue);                                        \
    DISPATCH_WITH_ACC(format)

#define SET_VREGS_AND_FRAME_NOT_NATIVE(format)                                                                  \
    Label funcIsClassConstructor(env);                                                                          \
    Label funcNotClassConstructor(env);                                                                         \
    Branch(IsClassConstructor(func), &funcIsClassConstructor, &funcNotClassConstructor);                        \
    Bind(&funcIsClassConstructor);                                                                              \
    {                                                                                                           \
        CallRuntime(glue, RTSTUB_ID(ThrowCallConstructorException), {});                                        \
        DISPATCH_LAST();                                                                                        \
    }                                                                                                           \
    Bind(&funcNotClassConstructor);                                                                             \
    Label notNormalCallType(env);                                                                               \
    Label isNormalCallType(env);                                                                                \
    Branch(Int64Equal(Int64And(callField, Int64(CALL_TYPE_MASK)), Int64(0)),                                    \
           &isNormalCallType, &notNormalCallType);                                                              \
    Bind(&notNormalCallType);                                                                                   \
    {                                                                                                           \
        GateRef haveThisMask = Int64(static_cast<uint64_t>(1) << JSMethod::HaveThisBit::START_BIT);             \
        Label methodHaveThis(env);                                                                              \
        Label methodNoThis(env);                                                                                \
        Branch(Int64NotEqual(Int64And(callField, haveThisMask), Int64(0)),                                      \
               &methodHaveThis, &methodNoThis);                                                                 \
        Bind(&methodHaveThis);                                                                                  \
        {                                                                                                       \
            newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                           \
            Label pushThis(env);                                                                                \
            Label pushThisUndefined(env);                                                                       \
            Branch(callThis, &pushThis, &pushThisUndefined);                                                    \
            Bind(&pushThis);                                                                                    \
            {                                                                                                   \
                GateRef thisValue = GetVregValue(sp, IntPtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)));             \
                Store(VariableType::INT64(), glue, *newSp, IntPtr(0), thisValue);                               \
                Jump(&methodNoThis);                                                                            \
            }                                                                                                   \
            Bind(&pushThisUndefined);                                                                           \
            {                                                                                                   \
                Store(VariableType::INT64(), glue, *newSp, IntPtr(0),                                           \
                      Int64(JSTaggedValue::VALUE_UNDEFINED));                                                   \
                Jump(&methodNoThis);                                                                            \
            }                                                                                                   \
        }                                                                                                       \
        Bind(&methodNoThis);                                                                                    \
        GateRef haveNewTargetMask = Int64(static_cast<uint64_t>(1) << JSMethod::HaveNewTargetBit::START_BIT);   \
        Label methodHaveNewTarget(env);                                                                         \
        Label methodNoNewTarget(env);                                                                           \
        Branch(Int64NotEqual(Int64And(callField, haveNewTargetMask), Int64(0)),                                 \
               &methodHaveNewTarget, &methodNoNewTarget);                                                       \
        Bind(&methodHaveNewTarget);                                                                             \
        {                                                                                                       \
            newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                           \
            Store(VariableType::INT64(), glue, *newSp, IntPtr(0),                                               \
                  Int64(JSTaggedValue::VALUE_UNDEFINED));                                                       \
            Jump(&methodNoNewTarget);                                                                           \
        }                                                                                                       \
        Bind(&methodNoNewTarget);                                                                               \
        GateRef haveFuncMask = Int64(static_cast<uint64_t>(1) << JSMethod::HaveFuncBit::START_BIT);             \
        Label methodHaveFunc(env);                                                                              \
        Label methodNoFunc(env);                                                                                \
        Branch(Int64NotEqual(Int64And(callField, haveFuncMask), Int64(0)),                                      \
               &methodHaveFunc, &methodNoFunc);                                                                 \
        Bind(&methodHaveFunc);                                                                                  \
        {                                                                                                       \
            newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                           \
            Store(VariableType::INT64(), glue, *newSp, IntPtr(0), func);                                        \
            Jump(&methodNoFunc);                                                                                \
        }                                                                                                       \
        Bind(&methodNoFunc);                                                                                    \
        Jump(&isNormalCallType);                                                                                \
    }                                                                                                           \
    Bind(&isNormalCallType);                                                                                    \
    {                                                                                                           \
        GateRef numVregsOffset = Int64(JSMethod::NumVregsBits::START_BIT);                                      \
        GateRef numVregsMask = Int64((static_cast<uint64_t>(1) << JSMethod::NumVregsBits::SIZE) - 1);           \
        GateRef numVregs = ChangeInt64ToInt32(Int64And(UInt64LSR(callField, numVregsOffset), numVregsMask));    \
        CALL_PUSH_UNDEFINED(numVregs);                                                                          \
        /* thread->DoStackOverflowCheck(newSp) */                                                               \
        GateRef frameBaseOffset = IntPtr(JSThread::GlueData::GetFrameBaseOffset(env->IsArch32Bit()));           \
        GateRef frameBase = Load(VariableType::POINTER(), glue, frameBaseOffset);                               \
        Label stackOverflow(env);                                                                               \
        Label stackNotOverflow(env);                                                                            \
        Branch(UInt64LessThanOrEqual(*newSp, IntPtrAdd(frameBase,                                               \
            /* 2: double size in case */                                                                        \
            IntPtr(JSThread::RESERVE_STACK_SIZE * sizeof(JSTaggedType) * 2))),                                  \
            &stackOverflow, &stackNotOverflow);                                                                 \
        Bind(&stackOverflow);                                                                                   \
        {                                                                                                       \
            CallRuntime(glue, RTSTUB_ID(ThrowStackOverflowException), {});                                      \
            DISPATCH_LAST();                                                                                    \
        }                                                                                                       \
        Bind(&stackNotOverflow);                                                                                \
        SetCallSizeToFrame(glue, GetFrame(sp),                                                                  \
            IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::format)));                            \
        GateRef state = GetFrame(*newSp);                                                                       \
        GateRef prevOffset = IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit()));                    \
        Store(VariableType::POINTER(), glue, state, prevOffset, sp);                                            \
        GateRef frameTypeOffset = IntPtrAdd(prevOffset, IntPtr(                                                 \
            env->IsArch32Bit() ? InterpretedFrameBase::TYPE_OFFSET_32 : InterpretedFrameBase::TYPE_OFFSET_64)); \
        Store(VariableType::INT64(), glue, state, frameTypeOffset,                                              \
              Int64(static_cast<uint64_t>(FrameType::INTERPRETER_FRAME)));                                      \
        GateRef bytecodeArrayOffset = IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit()));             \
        GateRef bytecodeArray = Load(VariableType::POINTER(), method, bytecodeArrayOffset);                     \
        SetPcToFrame(glue, state, bytecodeArray);                                                               \
        SetFunctionToFrame(glue, state, func);                                                                  \
        SetAccToFrame(glue, state, Hole(VariableType::JS_ANY()));                                               \
        GateRef newEnv = GetEnvFromFunction(func);                                                              \
        SetEnvToFrame(glue, state, newEnv);                                                                     \
        SetCurrentSpFrame(glue, *newSp);                                                                        \
        GateRef newConstpool = GetConstpoolFromFunction(func);                                                  \
        GateRef newProfileTypeInfo = GetProfileTypeInfoFromFunction(func);                                      \
        GateRef newHotnessCounter = Load(VariableType::INT32(), method,                                         \
                                         IntPtr(JSMethod::HOTNESS_COUNTER_OFFSET));                             \
        Dispatch(glue, bytecodeArray, *newSp, newConstpool, newProfileTypeInfo,                                 \
                 Hole(VariableType::JS_ANY()), newHotnessCounter, IntPtr(0));                                   \
    }

#define CALL_PUSH_ARGS_PREF_V8() \
    static_cast<void>(0) // do nothing when 0 arg

#define CALL_PUSH_ARGS_NO_EXTRA_PREF_V8() \
    static_cast<void>(0) // do nothing when 0 arg

#define CALL_PUSH_ARGS_PREF_V8_V8()                           \
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));    \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType))); \
    Store(VariableType::INT64(), glue, *newSp, IntPtr(0), a0Value)

#define CALL_PUSH_ARGS_NO_EXTRA_PREF_V8_V8()                                         \
    Label push0(env);                                                                \
    Label skip0(env);                                                                \
    Branch(Int32GreaterThanOrEqual(declaredNumArgs,                                  \
        Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG1)), &push0, &skip0); \
    Bind(&push0);                                                                    \
    {                                                                                \
        GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));                       \
        newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                    \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), a0Value);              \
        Jump(&skip0);                                                                \
    }                                                                                \
    Bind(&skip0)

#define CALL_PUSH_ARGS_PREF_V8_V8_V8()                              \
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));          \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));       \
    Store(VariableType::INT64(), glue, *newSp, IntPtr(0), a1Value); \
    CALL_PUSH_ARGS_PREF_V8_V8()

#define CALL_PUSH_ARGS_NO_EXTRA_PREF_V8_V8_V8()                                       \
    Label push1(env);                                                                 \
    Label skip1(env);                                                                 \
    Branch(Int32GreaterThanOrEqual(declaredNumArgs,                                   \
        Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS2)), &push1, &skip1); \
    Bind(&push1);                                                                     \
    {                                                                                 \
        GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));                        \
        newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                     \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), a1Value);               \
        Jump(&skip1);                                                                 \
    }                                                                                 \
    Bind(&skip1);                                                                     \
    CALL_PUSH_ARGS_NO_EXTRA_PREF_V8_V8()

#define CALL_PUSH_ARGS_PREF_V8_V8_V8_V8()                           \
    GateRef a2Value = GetVregValue(sp, ZExtInt8ToPtr(a2));          \
    newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));       \
    Store(VariableType::INT64(), glue, *newSp, IntPtr(0), a2Value); \
    CALL_PUSH_ARGS_PREF_V8_V8_V8()

#define CALL_PUSH_ARGS_NO_EXTRA_PREF_V8_V8_V8_V8()                                    \
    Label push2(env);                                                                 \
    Label skip2(env);                                                                 \
    Branch(Int32GreaterThanOrEqual(declaredNumArgs,                                   \
        Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS3)), &push2, &skip2); \
    Bind(&push2);                                                                     \
    {                                                                                 \
        GateRef a2Value = GetVregValue(sp, ZExtInt8ToPtr(a2));                        \
        newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                     \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), a2Value);               \
        Jump(&skip2);                                                                 \
    }                                                                                 \
    Bind(&skip2);                                                                     \
    CALL_PUSH_ARGS_NO_EXTRA_PREF_V8_V8_V8()

#define CALL_PUSH_ARGS_PREF_IMM16_V8() \
    i = actualNumArgs;                 \
    CALL_PUSH_ARGS_I()

#define CALL_PUSH_ARGS_NO_EXTRA_PREF_IMM16_V8()                                          \
    /* i = std::min(actualNumArgs, declaredNumArgs) */                                   \
    i = actualNumArgs;                                                                   \
    Label declaredNumArgsSmaller(env);                                                   \
    Label callPushArgsI(env);                                                            \
    Branch(Int32LessThan(*i, declaredNumArgs), &callPushArgsI, &declaredNumArgsSmaller); \
    Bind(&declaredNumArgsSmaller);                                                       \
    i = declaredNumArgs;                                                                 \
    Jump(&callPushArgsI);                                                                \
    Bind(&callPushArgsI);                                                                \
    CALL_PUSH_ARGS_I()

#define CALL_PUSH_ARGS_I()                                                                             \
    Label pushWithThis(env);                                                                           \
    Label pushWithoutThis(env);                                                                        \
    Label pushArgsEnd(env);                                                                            \
    Branch(callThis, &pushWithThis, &pushWithoutThis);                                                 \
    Bind(&pushWithThis);                                                                               \
    {                                                                                                  \
        i = Int32Add(*i, Int32(1)); /* 1: skip this */                                                 \
        Label pushArgs(env);                                                                           \
        Label pushArgsAgain(env);                                                                      \
        Branch(Int32GreaterThan(*i, Int32(1)), &pushArgs, &pushArgsEnd);                               \
        LoopBegin(&pushArgs);                                                                          \
        GateRef aValue = GetVregValue(sp, IntPtrAdd(ZExtInt8ToPtr(funcReg), ChangeInt32ToIntPtr(*i))); \
        newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                      \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), aValue);                                 \
        i = Int32Sub(*i, Int32(1));                                                                    \
        Branch(Int32GreaterThan(*i, Int32(1)), &pushArgsAgain, &pushArgsEnd);                          \
        Bind(&pushArgsAgain);                                                                          \
        LoopEnd(&pushArgs);                                                                            \
    }                                                                                                  \
    Bind(&pushWithoutThis);                                                                            \
    {                                                                                                  \
        Label pushArgs(env);                                                                           \
        Label pushArgsAgain(env);                                                                      \
        Branch(Int32GreaterThan(*i, Int32(0)), &pushArgs, &pushArgsEnd);                               \
        LoopBegin(&pushArgs);                                                                          \
        GateRef aValue = GetVregValue(sp, IntPtrAdd(ZExtInt8ToPtr(funcReg), ChangeInt32ToIntPtr(*i))); \
        newSp = PointerSub(*newSp, IntPtr(sizeof(JSTaggedType)));                                      \
        Store(VariableType::INT64(), glue, *newSp, IntPtr(0), aValue);                                 \
        i = Int32Sub(*i, Int32(1));                                                                    \
        Branch(Int32GreaterThan(*i, Int32(0)), &pushArgsAgain, &pushArgsEnd);                          \
        Bind(&pushArgsAgain);                                                                          \
        LoopEnd(&pushArgs);                                                                            \
    }                                                                                                  \
    Bind(&pushArgsEnd)

#define DISPATCH_COMMON_CALL_NATIVE_PART(type, format, ...)                                                           \
    SetPcToFrame(glue, GetFrame(sp), pc);                                                                             \
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));                                                          \
    Label funcIsHeapObject(env);                                                                                      \
    Label funcIsCallable(env);                                                                                        \
    Label funcNotCallable(env);                                                                                       \
    Branch(TaggedIsHeapObject(func), &funcIsHeapObject, &funcNotCallable);                                            \
    Bind(&funcIsHeapObject);                                                                                          \
    Branch(IsCallable(func), &funcIsCallable, &funcNotCallable);                                                      \
    Bind(&funcNotCallable);                                                                                           \
    {                                                                                                                 \
        CallRuntime(glue, Int64(RTSTUB_ID(ThrowCallConstructorException)), {});                                       \
        DISPATCH_LAST();                                                                                              \
    }                                                                                                                 \
    Bind(&funcIsCallable);                                                                                            \
    GateRef method = GetMethodFromJSFunction(func);                                                                   \
    GateRef callField = GetCallFieldFromMethod(method);                                                               \
    GateRef isNativeMask = Int64(static_cast<uint64_t>(1) << JSMethod::IsNativeBit::START_BIT);                       \
    Label methodIsNative(env);                                                                                        \
    Label methodNotNative(env);                                                                                       \
    Branch(Int64NotEqual(Int64And(callField, isNativeMask), Int64(0)), &methodIsNative, &methodNotNative);            \
    Bind(&methodIsNative);                                                                                            \
    {                                                                                                                 \
        GateRef retValue = CommonCallNative<RTSTUB_ID(type##Native)>(                                                 \
            glue, func, sp, method, __VA_ARGS__);                                                                     \
        Label hasPendingException(env);                                                                               \
        Label noPendingException(env);                                                                                \
        Branch(TaggedIsException(retValue), &hasPendingException, &noPendingException);                               \
        Bind(&hasPendingException);                                                                                   \
        {                                                                                                             \
            SetCurrentSpFrame(glue, sp);  /* currentSp will be used in UpFrame, therefore use sp. */                  \
            DISPATCH_LAST();                                                                                          \
        }                                                                                                             \
        Bind(&noPendingException);                                                                                    \
        SetCurrentSpFrame(glue, sp);                                                                                  \
        DEFVARIABLE(varAcc, VariableType::JS_ANY(), retValue);                                                        \
        DISPATCH_WITH_ACC(format);                                                                                    \
    }                                                                                                                 \
    Bind(&methodNotNative);                                                                                           \
    GateRef numArgsOffset = Int64(JSMethod::NumArgsBits::START_BIT);                                                  \
    GateRef numArgsMask = Int64((static_cast<uint64_t>(1) << JSMethod::NumArgsBits::SIZE) - 1);                       \
    GateRef declaredNumArgs = ChangeInt64ToInt32(Int64And(UInt64LSR(callField, numArgsOffset), numArgsMask));         \
    Label fastPath(env);                                                                                              \
    Label slowPath(env);

#define DISPATCH_COMMON_CALL_ARGS_JS_PART(type, ...)                                                                 \
    Branch(Int32Equal(actualNumArgs, declaredNumArgs), &fastPath, &slowPath);                                        \
    Bind(&fastPath);                                                                                                 \
    {                                                                                                                \
        DispatchCommonCall<RTSTUB_ID(type)>(                                                                         \
            glue, func, sp, method, __VA_ARGS__);                                                                    \
    }                                                                                                                \
    Bind(&slowPath);                                                                                                 \
    DispatchCommonCall<RTSTUB_ID(type##SlowPath)>(                                                                   \
        glue, func, sp, method, __VA_ARGS__);

#define DISPATCH_COMMON_CALL_RANGE_JS_PART(type, ...)                                                                \
    Branch(Int32Equal(actualNumArgs, declaredNumArgs), &fastPath, &slowPath);                                        \
    Bind(&fastPath);                                                                                                 \
    {                                                                                                                \
        DispatchCommonCall<RTSTUB_ID(type)>(                                                                         \
            glue, func, sp, method, callField, ZExtInt32ToInt64(actualNumArgs), __VA_ARGS__);                        \
    }                                                                                                                \
    Bind(&slowPath);                                                                                                 \
    DispatchCommonCall<RTSTUB_ID(type##SlowPath)>(                                                                   \
        glue, func, sp, method, callField, ZExtInt32ToInt64(actualNumArgs), __VA_ARGS__);

DECLARE_ASM_HANDLER(HandleCallArg0DynPrefV8)
{
    auto env = GetEnvironment();

    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG0);
    GateRef funcReg = ReadInst8_1(pc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    GateRef numArgs = ZExtInt32ToInt64(actualNumArgs);
    DISPATCH_COMMON_CALL_NATIVE_PART(PushCallArgs0AndDispatch, PREF_V8, numArgs);
    DISPATCH_COMMON_CALL_ARGS_JS_PART(PushCallArgs0AndDispatch, callField);
#else
    CALL_INITIALIZE();
    GateRef callThis = False();
    CALL_PUSH_ARGS(PREF_V8);
#endif
}

DECLARE_ASM_HANDLER(HandleCallArg1DynPrefV8V8)
{
    auto env = GetEnvironment();

    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG1);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    GateRef numArgs = ZExtInt32ToInt64(actualNumArgs);
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    DISPATCH_COMMON_CALL_NATIVE_PART(PushCallArgs1AndDispatch, PREF_V8_V8, numArgs, a0Value);
    DISPATCH_COMMON_CALL_ARGS_JS_PART(PushCallArgs1AndDispatch, callField, a0Value);
#else
    CALL_INITIALIZE();
    GateRef callThis = False();
    CALL_PUSH_ARGS(PREF_V8_V8);
#endif
}

DECLARE_ASM_HANDLER(HandleCallArgs2DynPrefV8V8V8)
{
    auto env = GetEnvironment();

    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS2);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    GateRef numArgs = ZExtInt32ToInt64(actualNumArgs);
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));
    DISPATCH_COMMON_CALL_NATIVE_PART(PushCallArgs2AndDispatch, PREF_V8_V8_V8, numArgs, a0Value, a1Value);
    DISPATCH_COMMON_CALL_ARGS_JS_PART(PushCallArgs2AndDispatch, callField, a0Value, a1Value);
#else
    CALL_INITIALIZE();
    GateRef callThis = False();
    CALL_PUSH_ARGS(PREF_V8_V8_V8);
#endif
}

DECLARE_ASM_HANDLER(HandleCallArgs3DynPrefV8V8V8V8)
{
    auto env = GetEnvironment();

    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS3);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef a2 = ReadInst8_4(pc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    GateRef numArgs = ZExtInt32ToInt64(actualNumArgs);
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));
    GateRef a2Value = GetVregValue(sp, ZExtInt8ToPtr(a2));
    DISPATCH_COMMON_CALL_NATIVE_PART(PushCallArgs3AndDispatch, PREF_V8_V8_V8_V8, numArgs, a0Value, a1Value, a2Value);
    DISPATCH_COMMON_CALL_ARGS_JS_PART(PushCallArgs3AndDispatch, callField, a0Value, a1Value, a2Value);
#else
    CALL_INITIALIZE();
    GateRef callThis = False();
    CALL_PUSH_ARGS(PREF_V8_V8_V8_V8);
#endif
}

DECLARE_ASM_HANDLER(HandleCallIRangeDynPrefImm16V8)
{
    auto env = GetEnvironment();

    GateRef actualNumArgs = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef funcReg = ReadInst8_3(pc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    GateRef numArgs = ZExtInt32ToInt64(actualNumArgs);
    GateRef argv = IntPtrAdd(sp, IntPtrMul(
        IntPtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)), IntPtr(8)));
    DISPATCH_COMMON_CALL_NATIVE_PART(PushCallIRangeAndDispatch, PREF_IMM16_V8, numArgs, argv);
    DISPATCH_COMMON_CALL_RANGE_JS_PART(PushCallIRangeAndDispatch, argv);
#else
    CALL_INITIALIZE();
    GateRef callThis = False();
    CALL_PUSH_ARGS(PREF_IMM16_V8);
#endif
}

DECLARE_ASM_HANDLER(HandleCallIThisRangeDynPrefImm16V8)
{
    auto env = GetEnvironment();

    GateRef actualNumArgs = Int32Sub(ZExtInt16ToInt32(ReadInst16_1(pc)), Int32(1));  // 1: exclude this
    GateRef funcReg = ReadInst8_3(pc);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    GateRef numArgs = ZExtInt32ToInt64(actualNumArgs);
    GateRef argv = IntPtrAdd(sp, IntPtrMul(
        IntPtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(2)), IntPtr(8)));  // 2: skip function and this
    DISPATCH_COMMON_CALL_NATIVE_PART(PushCallIThisRangeAndDispatch, PREF_IMM16_V8, numArgs, argv);
    DISPATCH_COMMON_CALL_RANGE_JS_PART(PushCallIThisRangeAndDispatch, argv);
#else
    CALL_INITIALIZE();
    GateRef callThis = True();
    CALL_PUSH_ARGS(PREF_IMM16_V8);
#endif
}

DECLARE_ASM_HANDLER(HandleLdBigIntPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef stringId = ReadInst32_1(pc);
    GateRef numberBigInt = GetObjectFromConstPool(constpool, stringId);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(LdBigInt), { numberBigInt });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    DISPATCH_WITH_ACC(PREF_ID32);
}

DECLARE_ASM_HANDLER(HandleNewLexEnvWithNameDynPrefImm16Imm16)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef numVars = ReadInst16_1(pc);
    GateRef scopeId = ReadInst16_3(pc);
    SetPcToFrame(glue, GetFrame(sp), pc);
    GateRef res = CallRuntime(glue, RTSTUB_ID(NewLexicalEnvWithNameDyn),
                              { Int16BuildTaggedTypeWithNoGC(numVars), Int16BuildTaggedTypeWithNoGC(scopeId) });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    varAcc = res;
    GateRef state = GetFrame(sp);
    SetEnvToFrame(glue, state, res);
    DISPATCH_WITH_ACC(PREF_IMM16_IMM16);
}
#undef DECLARE_ASM_HANDLER
#undef DISPATCH
#undef DISPATCH_WITH_ACC
#undef DISPATCH_LAST
#undef DISPATCH_LAST_WITH_ACC

CallSignature BytecodeStubCSigns::callSigns_[BytecodeStubCSigns::NUM_OF_VALID_STUBS];

void BytecodeStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name, counter)                                   \
    BytecodeHandlerCallSignature::Initialize(&callSigns_[name]);         \
    callSigns_[name].SetID(ID_##name);                                   \
    callSigns_[name].SetCallConv(CallSignature::CallConv::GHCCallConv);  \
    callSigns_[name].SetConstructor(                                     \
    [](void* ciruit) {                                                   \
        return static_cast<void*>(                                       \
            new name##Stub(static_cast<Circuit*>(ciruit)));              \
    });
    INTERPRETER_BC_STUB_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES
}

void BytecodeStubCSigns::GetCSigns(std::vector<CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_VALID_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
