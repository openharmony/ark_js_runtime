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
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/stub-inl.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
#define DECLARE_ASM_HANDLER(name)                                                         \
void name##Stub::GenerateCircuit(const CompilationConfig *cfg)                            \
{                                                                                         \
    Stub::GenerateCircuit(cfg);                                                           \
    GateRef glue = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::GLUE));      \
    GateRef sp = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::SP));          \
    GateRef pc = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::PC));          \
    GateRef constpool = TaggedPointerArgument(                                            \
        static_cast<size_t>(InterpreterHandlerInputs::CONSTPOOL));                        \
    GateRef profileTypeInfo = TaggedPointerArgument(                                      \
        static_cast<size_t>(InterpreterHandlerInputs::PROFILE_TYPE_INFO));                \
    GateRef acc = TaggedArgument(static_cast<size_t>(InterpreterHandlerInputs::ACC));     \
    GateRef hotnessCounter = Int32Argument(                                               \
        static_cast<size_t>(InterpreterHandlerInputs::HOTNESS_COUNTER));                  \
    DebugPrint(glue, { Int32(GET_MESSAGE_STRING_ID(name)) });                             \
    GenerateCircuitImpl(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);   \
}                                                                                         \
void name##Stub::GenerateCircuitImpl(GateRef glue, GateRef sp, GateRef pc,                \
                                     GateRef constpool, GateRef profileTypeInfo,          \
                                     GateRef acc, GateRef hotnessCounter)
#else
#define DECLARE_ASM_HANDLER(name)                                                         \
void name##Stub::GenerateCircuit(const CompilationConfig *cfg)                            \
{                                                                                         \
    Stub::GenerateCircuit(cfg);                                                           \
    GateRef glue = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::GLUE));      \
    GateRef sp = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::SP));          \
    GateRef pc = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::PC));          \
    GateRef constpool = TaggedPointerArgument(                                            \
        static_cast<size_t>(InterpreterHandlerInputs::CONSTPOOL));                        \
    GateRef profileTypeInfo = TaggedPointerArgument(                                      \
        static_cast<size_t>(InterpreterHandlerInputs::PROFILE_TYPE_INFO));                \
    GateRef acc = TaggedArgument(static_cast<size_t>(InterpreterHandlerInputs::ACC));     \
    GateRef hotnessCounter = Int32Argument(                                               \
        static_cast<size_t>(InterpreterHandlerInputs::HOTNESS_COUNTER));                  \
    GenerateCircuitImpl(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);   \
}                                                                                         \
void name##Stub::GenerateCircuitImpl(GateRef glue, GateRef sp, GateRef pc,                \
                                     GateRef constpool, GateRef profileTypeInfo,          \
                                     GateRef acc, GateRef hotnessCounter)
#endif

#define DISPATCH(format)                                                                  \
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter,               \
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::format)))

#define DISPATCH_WITH_ACC(format)                                                         \
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter,           \
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::format)))

#define DISPATCH_LAST()                                                                   \
    DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter)           \

#define DISPATCH_LAST_WITH_ACC()                                                          \
    DispatchLast(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter)       \

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
    GateRef state = GetFrame(sp);
    auto parent = GetEnvFromFrame(state);
    GateRef res = NewLexicalEnv(glue, ZExtInt16ToInt32(numVars), parent);
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
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    auto env = GetEnvironment();
    GateRef numArgs = ReadInst16_1(pc);
    GateRef firstArgRegIdx = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef firstArgOffset = Int16(2);
    GateRef ctor = GetVregValue(sp, ZExtInt16ToPtr(firstArgRegIdx));
    GateRef actualNumArgs = ZExtInt16ToPtr(Int16Sub(numArgs, firstArgOffset));

    Label ctorIsHeapObject(env);
    Label ctorIsJSFunction(env);
    Label fastPath(env);
    Label slowPath(env);
    Label checkResult(env);
    Label dispatch(env);
    Label ctorIsBase(env);
    Label ctorNotBase(env);
    Label isException(env);
    Label callRuntime(env);
    Label newObject(env);
    Label newObjectCheckException(env);

    GateRef isBase = IsBase(ctor);
    Branch(TaggedIsHeapObject(ctor), &ctorIsHeapObject, &slowPath);
    Bind(&ctorIsHeapObject);
    Branch(IsJSFunction(ctor), &ctorIsJSFunction, &slowPath);
    Bind(&ctorIsJSFunction);
    Branch(IsConstructor(ctor), &fastPath, &slowPath);
    Bind(&fastPath);
    {
        Branch(isBase, &ctorIsBase, &ctorNotBase);
        Bind(&ctorIsBase);
        {
            Label notHole(env);
            Label checkJSObject(env);
            auto protoOrHclass = Load(VariableType::JS_ANY(), ctor,
                IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            Branch(TaggedIsHole(protoOrHclass), &callRuntime, &notHole);
            Bind(&notHole);
            Branch(IsJSHClass(protoOrHclass), &checkJSObject, &callRuntime);
            Bind(&checkJSObject);
            auto objectType = GetObjectType(protoOrHclass);
            Branch(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT))),
                &newObject, &callRuntime);
            Bind(&newObject);
            {
                thisObj = NewJSObject(glue, protoOrHclass);
                Jump(&newObjectCheckException);
            }
            Bind(&callRuntime);
            {
                thisObj = CallRuntime(glue, RTSTUB_ID(NewThisObject), {ctor});
                Jump(&newObjectCheckException);
            }
            Bind(&newObjectCheckException);
            Branch(TaggedIsException(*res), &isException, &ctorNotBase);
        }
        Bind(&ctorNotBase);
        GateRef argv = PtrAdd(sp, PtrMul(
            PtrAdd(firstArgRegIdx, firstArgOffset), IntPtr(8))); // 8: skip function&this
        res = JSCallDispatch(glue, ctor, actualNumArgs,
                             JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV,
                             { actualNumArgs, argv, *thisObj });
        Jump(&checkResult);
    }
    Bind(&slowPath);
    GateRef firstArgIdx = Int16Add(firstArgRegIdx, firstArgOffset);
    GateRef length = Int16Sub(numArgs, firstArgOffset);
    res = CallRuntime(glue, RTSTUB_ID(NewObjDynRange),
        { ctor, ctor, Int16BuildTaggedTypeWithNoGC(firstArgIdx), Int16BuildTaggedTypeWithNoGC(length) });
    Jump(&checkResult);
    Bind(&checkResult);
    Branch(TaggedIsException(*res), &isException, &dispatch);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&dispatch);
    varAcc = *res;
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
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    auto env = GetEnvironment();
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_1(pc)));
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_3(pc)));
    GateRef res = CallRuntime(glue, RTSTUB_ID(AsyncFunctionResolveOrReject),
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
        varAcc = res;
        DISPATCH_WITH_ACC(PREF_V8_V8_V8);
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
    CallRuntime(glue, RTSTUB_ID(ThrowThrowNotExists), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleThrowPatternNonCoerciblePref)
{
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

DECLARE_ASM_HANDLER(SingleStepDebugging)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(varConstpool, VariableType::JS_POINTER(), constpool);
    DEFVARIABLE(varProfileTypeInfo, VariableType::JS_POINTER(), profileTypeInfo);
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(varHotnessCounter, VariableType::INT32(), hotnessCounter);

    varPc = TaggedCastToIntPtr(CallRuntime(glue,
                                           RTSTUB_ID(JumpToCInterpreter),
                                           { constpool, profileTypeInfo, acc,
                                             IntBuildTaggedTypeWithNoGC(hotnessCounter)}));
    Label shouldReturn(env);
    Label shouldContinue(env);

    Branch(IntPtrEqual(*varPc, IntPtr(0)), &shouldReturn, &shouldContinue);
    Bind(&shouldReturn);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { Undefined() });
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
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = GetHotnessCounterFromMethod(method);
    }
    Dispatch(glue, *varSp, *varPc, *varConstpool, *varProfileTypeInfo, *varAcc,
             *varHotnessCounter, IntPtr(0));
}

DECLARE_ASM_HANDLER(BCDebuggerEntry)
{
    GateRef frame = GetFrame(sp);
    SetPcToFrame(glue, frame, pc);
    // NOTIFY_DEBUGGER_EVENT()
    CallRuntime(glue, RTSTUB_ID(NotifyBytecodePcChanged), {});
    // goto normal handle stub
    DispatchDebugger(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
}

DECLARE_ASM_HANDLER(BCDebuggerExceptionEntry)
{
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    varAcc = Hole();
    GateRef frame = GetFrame(sp);
    SetPcToFrame(glue, frame, pc);
    // NOTIFY_DEBUGGER_EVENT()
    CallRuntime(glue, RTSTUB_ID(NotifyBytecodePcChanged), {});
    // goto last handle stub
    DispatchDebuggerLast(glue, sp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter);
}

DECLARE_ASM_HANDLER(HandleOverflow)
{
    FatalPrint(glue, { Int32(GET_MESSAGE_STRING_ID(OPCODE_OVERFLOW)) });
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, IntPtr(0));
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
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
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
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
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
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
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
    {
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
    }
    Bind(&slowPath);
    {
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
    {
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
    }
    Bind(&slowPath);
    {
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
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));

    Label strictEqual(env);
    Label notStrictEqual(env);
    Label dispatch(env);
    Branch(FastStrictEqual(glue, left, acc), &strictEqual, &notStrictEqual);
    Bind(&strictEqual);
    {
        varAcc = ChangeInt64ToTagged(Int64(JSTaggedValue::VALUE_FALSE));
        Jump(&dispatch);
    }

    Bind(&notStrictEqual);
    {
        varAcc = ChangeInt64ToTagged(Int64(JSTaggedValue::VALUE_TRUE));
        Jump(&dispatch);
    }
    Bind(&dispatch);
    DISPATCH_WITH_ACC(PREF_V8);
}

DECLARE_ASM_HANDLER(HandleStrictEqDynPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));

    Label strictEqual(env);
    Label notStrictEqual(env);
    Label dispatch(env);
    Branch(FastStrictEqual(glue, left, acc), &strictEqual, &notStrictEqual);
    Bind(&strictEqual);
    {
        varAcc = ChangeInt64ToTagged(Int64(JSTaggedValue::VALUE_TRUE));
        Jump(&dispatch);
    }

    Bind(&notStrictEqual);
    {
        varAcc = ChangeInt64ToTagged(Int64(JSTaggedValue::VALUE_FALSE));
        Jump(&dispatch);
    }
    Bind(&dispatch);
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
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(value), &isHeapObject, &notEcmaObject);
    Bind(&isHeapObject);
    Branch(TaggedObjectIsEcmaObject(value), &isEcmaObject, &notEcmaObject);
    Bind(&isEcmaObject);
    {
        DISPATCH(PREF_V8);
    }
    Bind(&notEcmaObject);
    CallRuntime(glue, RTSTUB_ID(ThrowIfNotObject), {});
    DISPATCH_LAST();
}

DECLARE_ASM_HANDLER(HandleIterNextPrefV8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
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
            Bind(&notHeapObject);
            {
                Branch(TaggedIsUndefined(firstValue), &slowPath, &tryFastPath);
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
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
            Bind(&notHeapObject);
            {
                Branch(TaggedIsUndefined(firstValue), &slowPath, &tryFastPath);
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
    Branch(IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    Bind(&notClassPrototype);
    {
        // fast path
        GateRef result = CallStub(glue, CommonStubCSigns::SetPropertyByValueWithOwn,
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
        GateRef result = CallStub(glue, CommonStubCSigns::SetPropertyByIndexWithOwn,
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
            Jump(&accDispatch);
        }
    }
    Bind(&doShr);
    {
        GateRef shift = Int32And(*opNumber1, Int32(0x1f));
        GateRef ret = Int32LSR(*opNumber0, shift);
        auto condition = Int32UnsignedGreaterThan(ret, Int32(INT32_MAX));
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
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(TaggedIsUndefined(profileTypeInfo), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
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
            Bind(&notHeapObject);
            {
                Branch(TaggedIsUndefined(firstValue), &slowPath, &tryFastPath);
            }
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
        Bind(&tryFastPath);
        {
            DispatchWithId(glue, sp, pc, constpool, profileTypeInfo, receiver, hotnessCounter,
                BCSTUB_ID(InterpreterGetPropertyByName));
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
            Label notHeapObject(env);
            GateRef firstValue = GetValueFromTaggedArray(VariableType::JS_ANY(), profileTypeInfo, slotId);
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &notHeapObject);
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
            Bind(&notHeapObject);
            {
                Branch(TaggedIsUndefined(firstValue), &slowPath, &tryFastPath);
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
                    CommonStubCSigns::SetPropertyByValueWithOwn,
                    { glue, receiver, propKey, acc });
                Branch(TaggedIsHole(res), &slowPath, &notHole);
                Bind(&notHole);
                {
                    Branch(TaggedIsException(res), &isException, &notException);
                    Bind(&isException);
                    {
                        DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
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
            DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
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
                result = CallStub(glue, CommonStubCSigns::SetPropertyByNameWithOwn,
                                  { glue, receiver, propKey, acc });
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
    Branch(IsJSObject(receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        Branch(IsClassConstructor(receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef res = CallStub(glue, CommonStubCSigns::SetPropertyByNameWithOwn,
                                       { glue, receiver, propKey, acc });
                Branch(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    Branch(TaggedIsException(res), &isException, &notException);
                    Bind(&isException);
                    DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
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
        DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
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
        Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
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
        Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
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
        Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
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
        Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, sp, pc, constpool, *varProfileTypeInfo, acc, *varHotnessCounter,
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16)));
}

DECLARE_ASM_HANDLER(HandleReturnDyn)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
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
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(VariableType::NATIVE_POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame, IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                       { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                         *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleReturnUndefinedPref)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
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
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(VariableType::NATIVE_POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    varAcc = Undefined();
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                       { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                         *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(HandleSuspendGeneratorPrefV8V8)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
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
    GateRef res = CallRuntime(glue, RTSTUB_ID(SuspendGenerator), { genObj, value });
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, *varSp, *varPc, *varConstpool, *varProfileTypeInfo, *varAcc, *varHotnessCounter);
    }
    Bind(&notException);
    varAcc = res;
    Branch(TaggedIsUndefined(*varProfileTypeInfo), &updateHotness, &tryContinue);
    Bind(&updateHotness);
    {
        GateRef function = GetFunctionFromFrame(frame);
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        GateRef fistPC = Load(VariableType::NATIVE_POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef offset = Int32Not(TruncPtrToInt32(PtrSub(*varPc, fistPC)));
        UPDATE_HOTNESS(*varSp);
        SetHotnessCounter(glue, method, *varHotnessCounter);
        Jump(&tryContinue);
    }

    Bind(&tryContinue);
    GateRef currentSp = *varSp;
    varSp = Load(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));
    GateRef prevState = GetFrame(*varSp);
    varPc = GetPcFromFrame(prevState);
    Branch(IntPtrEqual(*varPc, IntPtr(0)), &pcEqualNullptr, &pcNotEqualNullptr);
    Bind(&pcEqualNullptr);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndReturn), { *varAcc });
        Return();
    }
    Bind(&pcNotEqualNullptr);
    {
        GateRef function = GetFunctionFromFrame(prevState);
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        GateRef jumpSize = GetCallSizeFromFrame(prevState);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeRspAndDispatch),
                    { glue, currentSp, *varPc, *varConstpool, *varProfileTypeInfo,
                      *varAcc, *varHotnessCounter, jumpSize });
        Return();
    }
}

DECLARE_ASM_HANDLER(ExceptionHandler)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varPc, VariableType::NATIVE_POINTER(), pc);
    DEFVARIABLE(varSp, VariableType::NATIVE_POINTER(), sp);
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
        CallNGCRuntime(glue, RTSTUB_ID(ResumeUncaughtFrameAndReturn), { glue });
        Return();
    }
    Bind(&pcNotInvalid);
    {
        varSp = GetCurrentFrame(glue);
        varAcc = exception;
        // clear exception
        Store(VariableType::INT64(), glue, glue, IntPtr(0), Hole());
        GateRef function = GetFunctionFromFrame(GetFrame(*varSp));
        varConstpool = GetConstpoolFromFunction(function);
        varProfileTypeInfo = GetProfileTypeInfoFromFunction(function);
        GateRef method = Load(VariableType::NATIVE_POINTER(), function,
            IntPtr(JSFunctionBase::METHOD_OFFSET));
        varHotnessCounter = GetHotnessCounterFromMethod(method);
        CallNGCRuntime(glue, RTSTUB_ID(ResumeCaughtFrameAndDispatch), {
            glue, *varSp, *varPc, *varConstpool,
            *varProfileTypeInfo, *varAcc, *varHotnessCounter});
        Return();
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

DECLARE_ASM_HANDLER(HandleCallArg0DynPrefV8)
{
    auto env = GetEnvironment();
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG0);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef jumpSize = IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8));
    GateRef res = JSCallDispatch(glue, func, actualNumArgs, JSCallMode::CALL_ARG0, {});
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, res, hotnessCounter, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallArg1DynPrefV8V8)
{
    auto env = GetEnvironment();
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARG1);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef jumpSize = IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8));
    GateRef res = JSCallDispatch(glue, func, actualNumArgs, JSCallMode::CALL_ARG1, { a0Value });
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, res, hotnessCounter, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallArgs2DynPrefV8V8V8)
{
    auto env = GetEnvironment();
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS2);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));
    GateRef jumpSize = IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8));
    GateRef res = JSCallDispatch(glue, func, actualNumArgs,
                                 JSCallMode::CALL_ARG2, { a0Value, a1Value });
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, res, hotnessCounter, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallArgs3DynPrefV8V8V8V8)
{
    auto env = GetEnvironment();
    GateRef actualNumArgs = Int32(InterpreterAssembly::ActualNumArgsOfCall::CALLARGS3);
    GateRef funcReg = ReadInst8_1(pc);
    GateRef a0 = ReadInst8_2(pc);
    GateRef a1 = ReadInst8_3(pc);
    GateRef a2 = ReadInst8_4(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef a0Value = GetVregValue(sp, ZExtInt8ToPtr(a0));
    GateRef a1Value = GetVregValue(sp, ZExtInt8ToPtr(a1));
    GateRef a2Value = GetVregValue(sp, ZExtInt8ToPtr(a2));
    GateRef jumpSize = IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8_V8));
    GateRef res = JSCallDispatch(glue, func, actualNumArgs,
                                 JSCallMode::CALL_ARG3, { a0Value, a1Value, a2Value });
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, res, hotnessCounter, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallIRangeDynPrefImm16V8)
{
    auto env = GetEnvironment();
    GateRef actualNumArgs = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef funcReg = ReadInst8_3(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(funcReg));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(ZExtInt8ToPtr(funcReg), IntPtr(1)), IntPtr(8))); // 1: skip function
    GateRef jumpSize = IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8));
    GateRef numArgs = ChangeInt32ToIntPtr(actualNumArgs);
    GateRef res = JSCallDispatch(glue, func, actualNumArgs,
                                 JSCallMode::CALL_WITH_ARGV, { numArgs, argv });
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, res, hotnessCounter, jumpSize);
}

DECLARE_ASM_HANDLER(HandleCallIThisRangeDynPrefImm16V8)
{
    auto env = GetEnvironment();
    GateRef actualNumArgs = Int32Sub(ZExtInt16ToInt32(ReadInst16_1(pc)), Int32(1));  // 1: exclude this
    GateRef funcReg = ReadInst8_3(pc);
    funcReg = ZExtInt8ToPtr(funcReg);
    GateRef func = GetVregValue(sp, funcReg);
    GateRef thisValue = GetVregValue(sp, PtrAdd(funcReg, IntPtr(1)));
    GateRef argv = PtrAdd(sp, PtrMul(
        PtrAdd(funcReg, IntPtr(2)), IntPtr(8))); // 2: skip function&this
    GateRef jumpSize = IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8));
    GateRef numArgs = ChangeInt32ToIntPtr(actualNumArgs);
    GateRef res = JSCallDispatch(glue, func, actualNumArgs,
                                 JSCallMode::CALL_THIS_WITH_ARGV, { numArgs, argv, thisValue });
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST();
    }
    Bind(&notException);
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, res, hotnessCounter, jumpSize);
}

DECLARE_ASM_HANDLER(HandleLdBigIntPrefId32)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    GateRef stringId = ReadInst32_1(pc);
    GateRef numberBigInt = GetObjectFromConstPool(constpool, stringId);
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

DECLARE_ASM_HANDLER(NewObjectDynRangeReturn)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);

    Label isHeapObject(env);
    Label notUndefined(env);
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Label throwError(env);
    Label returnObject(env);
    Label dispatch(env);

    auto frame = GetFrame(sp);
    auto newSp = Load(VariableType::NATIVE_POINTER(), frame, IntPtr(AsmInterpretedFrame::GetBaseOffset(env->IsArch32Bit())));

    Branch(TaggedIsHeapObject(*varAcc), &isHeapObject, &notEcmaObject);
    Bind(&isHeapObject);
    Branch(TaggedObjectIsEcmaObject(*varAcc), &dispatch, &notEcmaObject);
    Bind(&notEcmaObject);
    {
        Branch(TaggedIsUndefined(*varAcc), &returnObject, &notUndefined);
        Bind(&notUndefined);
        auto constructor = GetFunctionFromFrame(frame);
        Branch(IsBase(constructor), &returnObject, &throwError);
        Bind(&throwError);
        {
            CallRuntime(glue, RTSTUB_ID(ThrowDerivedMustReturnException), {});
            DispatchLast(glue, newSp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
        }
        Bind(&returnObject);
        {
            auto fp = Load(VariableType::JS_POINTER(), frame,
                IntPtr(AsmInterpretedFrame::GetFpOffset(GetEnvironment()->IsArch32Bit())));
            auto thisObj = GetThisObjectFromFastNewFrame(fp);
            varAcc = thisObj;
            Jump(&dispatch);
        }
    }
    Bind(&dispatch);
    Dispatch(glue, newSp, pc, constpool, profileTypeInfo, *varAcc, hotnessCounter,
             IntPtr(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8)));
}

DECLARE_ASM_HANDLER(InterpreterGetPropertyByName)
{
    auto env = GetEnvironment();
    DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());

    Label checkResult(env);
    Label dispatch(env);
    Label slowPath(env);
    Label isException(env);

    GateRef receiver = acc;
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, stringId);
    result = GetPropertyByName(glue, receiver, propKey);

    Branch(TaggedIsHole(*result), &slowPath, &checkResult);
    Bind(&slowPath);
    {
        GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
        result = CallRuntime(glue, RTSTUB_ID(LoadICByName),
                             { profileTypeInfo, receiver, propKey, IntBuildTaggedTypeWithNoGC(slotId) });
        Jump(&checkResult);
    }
    Bind(&checkResult);
    {
        Branch(TaggedIsException(*result), &isException, &dispatch);
        Bind(&isException);
        {
            DISPATCH_LAST_WITH_ACC();
        }
    }
    Bind(&dispatch);
    varAcc = *result;
    DISPATCH_WITH_ACC(PREF_ID32_V8);
}
#undef DECLARE_ASM_HANDLER
#undef DISPATCH
#undef DISPATCH_WITH_ACC
#undef DISPATCH_LAST
#undef DISPATCH_LAST_WITH_ACC
#undef USE_PARAMS
}  // namespace panda::ecmascript::kungfu
