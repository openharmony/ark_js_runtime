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

#ifndef ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
#define ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H

#include "ecmascript/base/config.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/frames.h"

#if ECMASCRIPT_COMPILE_INTERPRETER_ASM
namespace panda::ecmascript {
using TaggedType = coretypes::TaggedType;
using DispatchEntryPoint =
    void (*)(JSThread *, const uint8_t *, JSTaggedType *, JSTaggedValue, JSTaggedValue, JSTaggedValue, int32_t);
using AsmDispatchEntryPoint =
    void (*)(uintptr_t, const uint8_t *, JSTaggedType *, JSTaggedValue, JSTaggedValue, JSTaggedValue, int32_t);
class ConstantPool;
class ECMAObject;
class GeneratorContext;

class InterpreterAssembly {
public:
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };
    static void RunInternal(JSThread *thread, ConstantPool *constpool, const uint8_t *pc, JSTaggedType *sp);
    static inline uint32_t FindCatchBlock(JSMethod *caller, uint32_t pc);
    static inline size_t GetJumpSizeAfterCall(const uint8_t *prevPc);

    static inline JSTaggedValue UpdateHotnessCounter(JSThread* thread, TaggedType *sp);
    static inline void InterpreterFrameCopyArgs(JSTaggedType *newSp, uint32_t numVregs, uint32_t numActualArgs,
                                                uint32_t numDeclaredArgs, bool haveExtraArgs = true);
    static inline JSTaggedValue GetThisFunction(JSTaggedType *sp);
    static inline JSTaggedValue GetNewTarget(JSTaggedType *sp);
    static inline uint32_t GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx);

    static void HandleMovV4V4(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleMovDynV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleMovDynV16V16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdaStrId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJmpImm8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJmpImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJmpImm32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJeqzImm8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJeqzImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJnezImm8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleJnezImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdaDynV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStaDynV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdaiDynImm32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleFldaiDynImm64(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallArg0DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallArg1DynPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallArgs2DynPrefV8V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallArgs3DynPrefV8V8V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallIThisRangeDynPrefImm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallSpreadDynPrefV8V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCallIRangeDynPrefImm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleReturnDyn(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleReturnUndefinedPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdNanPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdInfinityPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdGlobalThisPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdUndefinedPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdNullPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdSymbolPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdGlobalPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdTruePref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdFalsePref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdLexEnvDynPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetUnmappedArgsPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAsyncFunctionEnterPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleToNumberPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNegDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNotDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleIncDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDecDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowDynPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleTypeOfDynPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetPropIteratorPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleResumeGeneratorPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetResumeModePrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetIteratorPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowConstAssignmentPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowThrowNotExistsPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowPatternNonCoerciblePref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowIfNotObjectPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleIternextPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCloseIteratorPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAdd2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleSub2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleMul2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDiv2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleMod2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleEqDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNotEqDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLessDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLessEqDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGreaterDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGreaterEqDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleShl2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleShr2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAshr2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAnd2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleOr2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleXOr2DynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDelObjPropPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineFuncDynPrefId16Imm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineNCFuncDynPrefId16Imm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineMethodPrefId16Imm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNewObjDynRangePrefImm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleExpDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleIsInDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleInstanceOfDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStrictNotEqDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStrictEqDynPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdLexVarDynPrefImm16Imm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdLexVarDynPrefImm8Imm8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdLexVarDynPrefImm4Imm4(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStLexVarDynPrefImm16Imm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStLexVarDynPrefImm8Imm8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStLexVarDynPrefImm4Imm4V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNewLexEnvDynPrefImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandlePopLexEnvDynPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateIterResultObjPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleSuspendGeneratorPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAsyncFunctionAwaitUncaughtPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAsyncFunctionResolvePrefV8V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleAsyncFunctionRejectPrefV8V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNewObjSpreadDynPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowUndefinedIfHolePrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStOwnByNamePrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateEmptyArrayPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateEmptyObjectPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateObjectWithBufferPrefImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleSetObjectWithProtoPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateArrayWithBufferPrefImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleImportModulePrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStModuleVarPrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCopyModulePrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdModVarByNamePrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateRegExpWithLiteralPrefId32Imm8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetTemplateObjectPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetNextPropNamePrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCopyDataPropertiesPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStOwnByIndexPrefV8Imm32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStOwnByValuePrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateObjectWithExcludedKeysPrefImm16V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineGeneratorFuncPrefId16Imm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineAsyncFuncPrefId16Imm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdHolePref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCopyRestArgsPrefImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineGetterSetterByValuePrefV8V8V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdObjByIndexPrefV8Imm32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStObjByIndexPrefV8Imm32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdObjByValuePrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStObjByValuePrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdSuperByValuePrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStSuperByValuePrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleTryLdGlobalByNamePrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleTryStGlobalByNamePrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStConstToGlobalRecordPrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStLetToGlobalRecordPrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStClassToGlobalRecordPrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStOwnByValueWithNameSetPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStOwnByNameWithNameSetPrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdFunctionPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleNewLexEnvWithNameDynPrefImm16Imm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdGlobalVarPrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdObjByNamePrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStObjByNamePrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdSuperByNamePrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStSuperByNamePrefId32V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStGlobalVarPrefId32(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateGeneratorObjPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleStArraySpreadPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleGetIteratorNextPrefV8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleSuperCallPrefImm16V8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleSuperCallSpreadPrefV8(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleCreateObjectHavingMethodPrefImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowIfSuperNotCorrectCallPrefImm16(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleLdHomeObjectPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleThrowDeleteSuperPropertyPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleDebuggerPref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleIsTruePref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleIsFalsePref(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void ExceptionHandler(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
    static void HandleOverflow(
        JSThread *thread, const uint8_t *pc, JSTaggedType *sp, JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
        JSTaggedValue acc, int32_t hotnessCounter);
};

static std::array<DispatchEntryPoint, BCHandlers::MAX_BYTECODE_HANDLERS> asmDispatchTable {
    InterpreterAssembly::HandleLdNanPref,
    InterpreterAssembly::HandleLdInfinityPref,
    InterpreterAssembly::HandleLdGlobalThisPref,
    InterpreterAssembly::HandleLdUndefinedPref,
    InterpreterAssembly::HandleLdNullPref,
    InterpreterAssembly::HandleLdSymbolPref,
    InterpreterAssembly::HandleLdGlobalPref,
    InterpreterAssembly::HandleLdTruePref,
    InterpreterAssembly::HandleLdFalsePref,
    InterpreterAssembly::HandleThrowDynPref,
    InterpreterAssembly::HandleTypeOfDynPref,
    InterpreterAssembly::HandleLdLexEnvDynPref,
    InterpreterAssembly::HandlePopLexEnvDynPref,
    InterpreterAssembly::HandleGetUnmappedArgsPref,
    InterpreterAssembly::HandleGetPropIteratorPref,
    InterpreterAssembly::HandleAsyncFunctionEnterPref,
    InterpreterAssembly::HandleLdHolePref,
    InterpreterAssembly::HandleReturnUndefinedPref,
    InterpreterAssembly::HandleCreateEmptyObjectPref,
    InterpreterAssembly::HandleCreateEmptyArrayPref,
    InterpreterAssembly::HandleGetIteratorPref,
    InterpreterAssembly::HandleThrowThrowNotExistsPref,
    InterpreterAssembly::HandleThrowPatternNonCoerciblePref,
    InterpreterAssembly::HandleLdHomeObjectPref,
    InterpreterAssembly::HandleThrowDeleteSuperPropertyPref,
    InterpreterAssembly::HandleDebuggerPref,
    InterpreterAssembly::HandleAdd2DynPrefV8,
    InterpreterAssembly::HandleSub2DynPrefV8,
    InterpreterAssembly::HandleMul2DynPrefV8,
    InterpreterAssembly::HandleDiv2DynPrefV8,
    InterpreterAssembly::HandleMod2DynPrefV8,
    InterpreterAssembly::HandleEqDynPrefV8,
    InterpreterAssembly::HandleNotEqDynPrefV8,
    InterpreterAssembly::HandleLessDynPrefV8,
    InterpreterAssembly::HandleLessEqDynPrefV8,
    InterpreterAssembly::HandleGreaterDynPrefV8,
    InterpreterAssembly::HandleGreaterEqDynPrefV8,
    InterpreterAssembly::HandleShl2DynPrefV8,
    InterpreterAssembly::HandleShr2DynPrefV8,
    InterpreterAssembly::HandleAshr2DynPrefV8,
    InterpreterAssembly::HandleAnd2DynPrefV8,
    InterpreterAssembly::HandleOr2DynPrefV8,
    InterpreterAssembly::HandleXOr2DynPrefV8,
    InterpreterAssembly::HandleToNumberPrefV8,
    InterpreterAssembly::HandleNegDynPrefV8,
    InterpreterAssembly::HandleNotDynPrefV8,
    InterpreterAssembly::HandleIncDynPrefV8,
    InterpreterAssembly::HandleDecDynPrefV8,
    InterpreterAssembly::HandleExpDynPrefV8,
    InterpreterAssembly::HandleIsInDynPrefV8,
    InterpreterAssembly::HandleInstanceOfDynPrefV8,
    InterpreterAssembly::HandleStrictNotEqDynPrefV8,
    InterpreterAssembly::HandleStrictEqDynPrefV8,
    InterpreterAssembly::HandleResumeGeneratorPrefV8,
    InterpreterAssembly::HandleGetResumeModePrefV8,
    InterpreterAssembly::HandleCreateGeneratorObjPrefV8,
    InterpreterAssembly::HandleThrowConstAssignmentPrefV8,
    InterpreterAssembly::HandleGetTemplateObjectPrefV8,
    InterpreterAssembly::HandleGetNextPropNamePrefV8,
    InterpreterAssembly::HandleCallArg0DynPrefV8,
    InterpreterAssembly::HandleThrowIfNotObjectPrefV8,
    InterpreterAssembly::HandleIternextPrefV8,
    InterpreterAssembly::HandleCloseIteratorPrefV8,
    InterpreterAssembly::HandleCopyModulePrefV8,
    InterpreterAssembly::HandleSuperCallSpreadPrefV8,
    InterpreterAssembly::HandleDelObjPropPrefV8V8,
    InterpreterAssembly::HandleNewObjSpreadDynPrefV8V8,
    InterpreterAssembly::HandleCreateIterResultObjPrefV8V8,
    InterpreterAssembly::HandleSuspendGeneratorPrefV8V8,
    InterpreterAssembly::HandleAsyncFunctionAwaitUncaughtPrefV8V8,
    InterpreterAssembly::HandleThrowUndefinedIfHolePrefV8V8,
    InterpreterAssembly::HandleCallArg1DynPrefV8V8,
    InterpreterAssembly::HandleCopyDataPropertiesPrefV8V8,
    InterpreterAssembly::HandleStArraySpreadPrefV8V8,
    InterpreterAssembly::HandleGetIteratorNextPrefV8V8,
    InterpreterAssembly::HandleSetObjectWithProtoPrefV8V8,
    InterpreterAssembly::HandleLdObjByValuePrefV8V8,
    InterpreterAssembly::HandleStObjByValuePrefV8V8,
    InterpreterAssembly::HandleStOwnByValuePrefV8V8,
    InterpreterAssembly::HandleLdSuperByValuePrefV8V8,
    InterpreterAssembly::HandleStSuperByValuePrefV8V8,
    InterpreterAssembly::HandleLdObjByIndexPrefV8Imm32,
    InterpreterAssembly::HandleStObjByIndexPrefV8Imm32,
    InterpreterAssembly::HandleStOwnByIndexPrefV8Imm32,
    InterpreterAssembly::HandleCallSpreadDynPrefV8V8V8,
    InterpreterAssembly::HandleAsyncFunctionResolvePrefV8V8V8,
    InterpreterAssembly::HandleAsyncFunctionRejectPrefV8V8V8,
    InterpreterAssembly::HandleCallArgs2DynPrefV8V8V8,
    InterpreterAssembly::HandleCallArgs3DynPrefV8V8V8V8,
    InterpreterAssembly::HandleDefineGetterSetterByValuePrefV8V8V8V8,
    InterpreterAssembly::HandleNewObjDynRangePrefImm16V8,
    InterpreterAssembly::HandleCallIRangeDynPrefImm16V8,
    InterpreterAssembly::HandleCallIThisRangeDynPrefImm16V8,
    InterpreterAssembly::HandleSuperCallPrefImm16V8,
    InterpreterAssembly::HandleCreateObjectWithExcludedKeysPrefImm16V8V8,
    InterpreterAssembly::HandleDefineFuncDynPrefId16Imm16V8,
    InterpreterAssembly::HandleDefineNCFuncDynPrefId16Imm16V8,
    InterpreterAssembly::HandleDefineGeneratorFuncPrefId16Imm16V8,
    InterpreterAssembly::HandleDefineAsyncFuncPrefId16Imm16V8,
    InterpreterAssembly::HandleDefineMethodPrefId16Imm16V8,
    InterpreterAssembly::HandleNewLexEnvDynPrefImm16,
    InterpreterAssembly::HandleCopyRestArgsPrefImm16,
    InterpreterAssembly::HandleCreateArrayWithBufferPrefImm16,
    InterpreterAssembly::HandleCreateObjectHavingMethodPrefImm16,
    InterpreterAssembly::HandleThrowIfSuperNotCorrectCallPrefImm16,
    InterpreterAssembly::HandleCreateObjectWithBufferPrefImm16,
    InterpreterAssembly::HandleLdLexVarDynPrefImm4Imm4,
    InterpreterAssembly::HandleLdLexVarDynPrefImm8Imm8,
    InterpreterAssembly::HandleLdLexVarDynPrefImm16Imm16,
    InterpreterAssembly::HandleStLexVarDynPrefImm4Imm4V8,
    InterpreterAssembly::HandleStLexVarDynPrefImm8Imm8V8,
    InterpreterAssembly::HandleStLexVarDynPrefImm16Imm16V8,
    InterpreterAssembly::HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8,
    InterpreterAssembly::HandleImportModulePrefId32,
    InterpreterAssembly::HandleStModuleVarPrefId32,
    InterpreterAssembly::HandleTryLdGlobalByNamePrefId32,
    InterpreterAssembly::HandleTryStGlobalByNamePrefId32,
    InterpreterAssembly::HandleLdGlobalVarPrefId32,
    InterpreterAssembly::HandleStGlobalVarPrefId32,
    InterpreterAssembly::HandleLdObjByNamePrefId32V8,
    InterpreterAssembly::HandleStObjByNamePrefId32V8,
    InterpreterAssembly::HandleStOwnByNamePrefId32V8,
    InterpreterAssembly::HandleLdSuperByNamePrefId32V8,
    InterpreterAssembly::HandleStSuperByNamePrefId32V8,
    InterpreterAssembly::HandleLdModVarByNamePrefId32V8,
    InterpreterAssembly::HandleCreateRegExpWithLiteralPrefId32Imm8,
    InterpreterAssembly::HandleIsTruePref,
    InterpreterAssembly::HandleIsFalsePref,
    InterpreterAssembly::HandleStConstToGlobalRecordPrefId32,
    InterpreterAssembly::HandleStLetToGlobalRecordPrefId32,
    InterpreterAssembly::HandleStClassToGlobalRecordPrefId32,
    InterpreterAssembly::HandleStOwnByValueWithNameSetPrefV8V8,
    InterpreterAssembly::HandleStOwnByNameWithNameSetPrefId32V8,
    InterpreterAssembly::HandleLdFunctionPref,
    InterpreterAssembly::HandleNewLexEnvWithNameDynPrefImm16Imm16,
    InterpreterAssembly::HandleMovDynV8V8,
    InterpreterAssembly::HandleMovDynV16V16,
    InterpreterAssembly::HandleLdaStrId32,
    InterpreterAssembly::HandleLdaiDynImm32,
    InterpreterAssembly::HandleFldaiDynImm64,
    InterpreterAssembly::HandleJmpImm8,
    InterpreterAssembly::HandleJmpImm16,
    InterpreterAssembly::HandleJmpImm32,
    InterpreterAssembly::HandleJeqzImm8,
    InterpreterAssembly::HandleJeqzImm16,
    InterpreterAssembly::HandleLdaDynV8,
    InterpreterAssembly::HandleStaDynV8,
    InterpreterAssembly::HandleReturnDyn,
    InterpreterAssembly::HandleMovV4V4,
    InterpreterAssembly::HandleJnezImm8,
    InterpreterAssembly::HandleJnezImm16,
    InterpreterAssembly::ExceptionHandler,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILE_INTERPRETER_ASM
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
