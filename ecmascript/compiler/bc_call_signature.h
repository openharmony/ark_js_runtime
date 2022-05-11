/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BC_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_BC_CALL_SIGNATURE_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/rt_call_signature.h"

namespace panda::ecmascript::kungfu {
#define IGNORE_BC_STUB(...)
#define ASM_INTERPRETER_BC_STUB_LIST(V, T, I)            \
    T(HandleLdNanPref)                                   \
    T(HandleLdInfinityPref)                              \
    T(HandleLdGlobalThisPref)                            \
    T(HandleLdUndefinedPref)                             \
    T(HandleLdNullPref)                                  \
    T(HandleLdSymbolPref)                                \
    T(HandleLdGlobalPref)                                \
    T(HandleLdTruePref)                                  \
    T(HandleLdFalsePref)                                 \
    T(HandleThrowDynPref)                                \
    T(HandleTypeOfDynPref)                               \
    T(HandleLdLexEnvDynPref)                             \
    T(HandlePopLexEnvDynPref)                            \
    T(HandleGetUnmappedArgsPref)                         \
    T(HandleGetPropIteratorPref)                         \
    T(HandleAsyncFunctionEnterPref)                      \
    T(HandleLdHolePref)                                  \
    T(HandleReturnUndefinedPref)                         \
    T(HandleCreateEmptyObjectPref)                       \
    T(HandleCreateEmptyArrayPref)                        \
    T(HandleGetIteratorPref)                             \
    T(HandleThrowThrowNotExistsPref)                     \
    T(HandleThrowPatternNonCoerciblePref)                \
    T(HandleLdHomeObjectPref)                            \
    T(HandleThrowDeleteSuperPropertyPref)                \
    T(HandleDebuggerPref)                                \
    T(HandleAdd2DynPrefV8)                               \
    T(HandleSub2DynPrefV8)                               \
    T(HandleMul2DynPrefV8)                               \
    T(HandleDiv2DynPrefV8)                               \
    T(HandleMod2DynPrefV8)                               \
    T(HandleEqDynPrefV8)                                 \
    T(HandleNotEqDynPrefV8)                              \
    T(HandleLessDynPrefV8)                               \
    T(HandleLessEqDynPrefV8)                             \
    T(HandleGreaterDynPrefV8)                            \
    T(HandleGreaterEqDynPrefV8)                          \
    T(HandleShl2DynPrefV8)                               \
    T(HandleAshr2DynPrefV8)                              \
    T(HandleShr2DynPrefV8)                               \
    T(HandleAnd2DynPrefV8)                               \
    T(HandleOr2DynPrefV8)                                \
    T(HandleXOr2DynPrefV8)                               \
    T(HandleToNumberPrefV8)                              \
    T(HandleNegDynPrefV8)                                \
    T(HandleNotDynPrefV8)                                \
    T(HandleIncDynPrefV8)                                \
    T(HandleDecDynPrefV8)                                \
    T(HandleExpDynPrefV8)                                \
    T(HandleIsInDynPrefV8)                               \
    T(HandleInstanceOfDynPrefV8)                         \
    T(HandleStrictNotEqDynPrefV8)                        \
    T(HandleStrictEqDynPrefV8)                           \
    T(HandleResumeGeneratorPrefV8)                       \
    T(HandleGetResumeModePrefV8)                         \
    T(HandleCreateGeneratorObjPrefV8)                    \
    T(HandleThrowConstAssignmentPrefV8)                  \
    T(HandleGetTemplateObjectPrefV8)                     \
    T(HandleGetNextPropNamePrefV8)                       \
    T(HandleCallArg0DynPrefV8)                           \
    T(HandleThrowIfNotObjectPrefV8)                      \
    T(HandleIterNextPrefV8)                              \
    T(HandleCloseIteratorPrefV8)                         \
    T(HandleCopyModulePrefV8)                            \
    T(HandleSuperCallSpreadPrefV8)                       \
    T(HandleDelObjPropPrefV8V8)                          \
    T(HandleNewObjSpreadDynPrefV8V8)                     \
    T(HandleCreateIterResultObjPrefV8V8)                 \
    T(HandleSuspendGeneratorPrefV8V8)                    \
    T(HandleAsyncFunctionAwaitUncaughtPrefV8V8)          \
    T(HandleThrowUndefinedIfHolePrefV8V8)                \
    T(HandleCallArg1DynPrefV8V8)                         \
    T(HandleCopyDataPropertiesPrefV8V8)                  \
    T(HandleStArraySpreadPrefV8V8)                       \
    T(HandleGetIteratorNextPrefV8V8)                     \
    T(HandleSetObjectWithProtoPrefV8V8)                  \
    T(HandleLdObjByValuePrefV8V8)                        \
    T(HandleStObjByValuePrefV8V8)                        \
    T(HandleStOwnByValuePrefV8V8)                        \
    T(HandleLdSuperByValuePrefV8V8)                      \
    T(HandleStSuperByValuePrefV8V8)                      \
    T(HandleLdObjByIndexPrefV8Imm32)                     \
    T(HandleStObjByIndexPrefV8Imm32)                     \
    T(HandleStOwnByIndexPrefV8Imm32)                     \
    T(HandleCallSpreadDynPrefV8V8V8)                     \
    T(HandleAsyncFunctionResolvePrefV8V8V8)              \
    T(HandleAsyncFunctionRejectPrefV8V8V8)               \
    T(HandleCallArgs2DynPrefV8V8V8)                      \
    T(HandleCallArgs3DynPrefV8V8V8V8)                    \
    T(HandleDefineGetterSetterByValuePrefV8V8V8V8)       \
    T(HandleNewObjDynRangePrefImm16V8)                   \
    T(HandleCallIRangeDynPrefImm16V8)                    \
    T(HandleCallIThisRangeDynPrefImm16V8)                \
    T(HandleSuperCallPrefImm16V8)                        \
    T(HandleCreateObjectWithExcludedKeysPrefImm16V8V8)   \
    T(HandleDefineFuncDynPrefId16Imm16V8)                \
    T(HandleDefineNCFuncDynPrefId16Imm16V8)              \
    T(HandleDefineGeneratorFuncPrefId16Imm16V8)          \
    T(HandleDefineAsyncFuncPrefId16Imm16V8)              \
    T(HandleDefineMethodPrefId16Imm16V8)                 \
    T(HandleNewLexEnvDynPrefImm16)                       \
    T(HandleCopyRestArgsPrefImm16)                       \
    T(HandleCreateArrayWithBufferPrefImm16)              \
    T(HandleCreateObjectHavingMethodPrefImm16)           \
    T(HandleThrowIfSuperNotCorrectCallPrefImm16)         \
    T(HandleCreateObjectWithBufferPrefImm16)             \
    T(HandleLdLexVarDynPrefImm4Imm4)                     \
    T(HandleLdLexVarDynPrefImm8Imm8)                     \
    T(HandleLdLexVarDynPrefImm16Imm16)                   \
    T(HandleStLexVarDynPrefImm4Imm4V8)                   \
    T(HandleStLexVarDynPrefImm8Imm8V8)                   \
    T(HandleStLexVarDynPrefImm16Imm16V8)                 \
    T(HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8) \
    T(HandleGetModuleNamespacePrefId32)                  \
    T(HandleStModuleVarPrefId32)                         \
    T(HandleTryLdGlobalByNamePrefId32)                   \
    T(HandleTryStGlobalByNamePrefId32)                   \
    T(HandleLdGlobalVarPrefId32)                         \
    T(HandleStGlobalVarPrefId32)                         \
    T(HandleLdObjByNamePrefId32V8)                       \
    T(HandleStObjByNamePrefId32V8)                       \
    T(HandleStOwnByNamePrefId32V8)                       \
    T(HandleLdSuperByNamePrefId32V8)                     \
    T(HandleStSuperByNamePrefId32V8)                     \
    T(HandleLdModuleVarPrefId32Imm8)                     \
    T(HandleCreateRegExpWithLiteralPrefId32Imm8)         \
    T(HandleIsTruePref)                                  \
    T(HandleIsFalsePref)                                 \
    T(HandleStConstToGlobalRecordPrefId32)               \
    T(HandleStLetToGlobalRecordPrefId32)                 \
    T(HandleStClassToGlobalRecordPrefId32)               \
    T(HandleStOwnByValueWithNameSetPrefV8V8)             \
    T(HandleStOwnByNameWithNameSetPrefId32V8)            \
    T(HandleLdFunctionPref)                              \
    T(HandleNewLexEnvWithNameDynPrefImm16Imm16)          \
    T(HandleLdBigIntPrefId32)                            \
    T(HandleMovDynV8V8)                                  \
    T(HandleMovDynV16V16)                                \
    T(HandleLdaStrId32)                                  \
    T(HandleLdaiDynImm32)                                \
    T(HandleFldaiDynImm64)                               \
    T(HandleJmpImm8)                                     \
    T(HandleJmpImm16)                                    \
    T(HandleJmpImm32)                                    \
    T(HandleJeqzImm8)                                    \
    T(HandleJeqzImm16)                                   \
    T(HandleLdaDynV8)                                    \
    T(HandleStaDynV8)                                    \
    T(HandleReturnDyn)                                   \
    T(HandleMovV4V4)                                     \
    T(HandleJnezImm8)                                    \
    T(HandleJnezImm16)                                   \
    T(ExceptionHandler)

#define ASM_INTERPRETER_BC_HELPER_STUB_LIST(V)           \
    V(SingleStepDebugging)                               \
    V(HandleOverflow)                                    \
    V(BCDebuggerEntry)                                   \
    V(BCDebuggerExceptionEntry)

#define INTERPRETER_IGNORED_BC_STUB_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)

#define INTERPRETER_BC_STUB_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(IGNORE_BC_STUB, V, V)

#define ASM_INTERPRETER_BC_STUB_ID_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(V, V, V)

class BytecodeStubCSigns {
public:
    // all valid stub, include normal and helper stub
    enum ValidID {
#define DEF_VALID_BC_STUB_ID(name) name,
        INTERPRETER_BC_STUB_LIST(DEF_VALID_BC_STUB_ID)
        ASM_INTERPRETER_BC_HELPER_STUB_LIST(DEF_VALID_BC_STUB_ID)
#undef DEF_VALID_BC_STUB_ID
        NUM_OF_VALID_STUBS
    };

    enum ID {
#define DEF_BC_STUB_ID(name) ID_##name,
        ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_BC_STUB_ID)
#undef DEF_BC_STUB_ID
        NUM_OF_ALL_NORMAL_STUBS
    };

    enum HelperID {
#define DEF_BC_STUB_ID(name) HELPER_ID_##name,
        ASM_INTERPRETER_BC_HELPER_STUB_LIST(DEF_BC_STUB_ID)
#undef DEF_BC_STUB_ID
        NUM_OF_ALL_HELPER_STUBS
    };

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& outCSigns);

    static const CallSignature* Get(size_t index)
    {
        ASSERT(index < NUM_OF_VALID_STUBS);
        return &callSigns_[index];
    }
private:
    static CallSignature callSigns_[NUM_OF_VALID_STUBS];
};

enum class InterpreterHandlerInputs : size_t {
    GLUE = 0,
    SP,
    PC,
    CONSTPOOL,
    PROFILE_TYPE_INFO,
    ACC,
    HOTNESS_COUNTER,
    NUM_OF_INPUTS
};

#define BYTECODE_STUB_BEGIN_ID BytecodeStubCSigns::ID_HandleLdNanPref
#define BYTECODE_STUB_END_ID BytecodeStubCSigns::ID_ExceptionHandler
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BC_CALL_SIGNATURE_H
