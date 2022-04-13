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

#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub.h"

namespace panda::ecmascript::kungfu {
#define IGNORE_BC_STUB(...)
#define ASM_INTERPRETER_BC_STUB_LIST(V, T, I)               \
    T(HandleLdNanPref, 7)                                   \
    T(HandleLdInfinityPref, 7)                              \
    T(HandleLdGlobalThisPref, 7)                            \
    T(HandleLdUndefinedPref, 7)                             \
    T(HandleLdNullPref, 7)                                  \
    T(HandleLdSymbolPref, 7)                                \
    T(HandleLdGlobalPref, 7)                                \
    T(HandleLdTruePref, 7)                                  \
    T(HandleLdFalsePref, 7)                                 \
    T(HandleThrowDynPref, 7)                                \
    T(HandleTypeOfDynPref, 7)                               \
    T(HandleLdLexEnvDynPref, 7)                             \
    T(HandlePopLexEnvDynPref, 7)                            \
    T(HandleGetUnmappedArgsPref, 7)                         \
    T(HandleGetPropIteratorPref, 7)                         \
    T(HandleAsyncFunctionEnterPref, 7)                      \
    T(HandleLdHolePref, 7)                                  \
    T(HandleReturnUndefinedPref, 7)                         \
    T(HandleCreateEmptyObjectPref, 7)                       \
    T(HandleCreateEmptyArrayPref, 7)                        \
    T(HandleGetIteratorPref, 7)                             \
    T(HandleThrowThrowNotExistsPref, 7)                     \
    T(HandleThrowPatternNonCoerciblePref, 7)                \
    T(HandleLdHomeObjectPref, 7)                            \
    T(HandleThrowDeleteSuperPropertyPref, 7)                \
    T(HandleDebuggerPref, 7)                                \
    T(HandleAdd2DynPrefV8, 7)                               \
    T(HandleSub2DynPrefV8, 7)                               \
    T(HandleMul2DynPrefV8, 7)                               \
    T(HandleDiv2DynPrefV8, 7)                               \
    T(HandleMod2DynPrefV8, 7)                               \
    T(HandleEqDynPrefV8, 7)                                 \
    T(HandleNotEqDynPrefV8, 7)                              \
    T(HandleLessDynPrefV8, 7)                               \
    T(HandleLessEqDynPrefV8, 7)                             \
    T(HandleGreaterDynPrefV8, 7)                            \
    T(HandleGreaterEqDynPrefV8, 7)                          \
    T(HandleShl2DynPrefV8, 7)                               \
    T(HandleAshr2DynPrefV8, 7)                              \
    T(HandleShr2DynPrefV8, 7)                               \
    T(HandleAnd2DynPrefV8, 7)                               \
    T(HandleOr2DynPrefV8, 7)                                \
    T(HandleXOr2DynPrefV8, 7)                               \
    T(HandleToNumberPrefV8, 7)                              \
    T(HandleNegDynPrefV8, 7)                                \
    T(HandleNotDynPrefV8, 7)                                \
    T(HandleIncDynPrefV8, 7)                                \
    T(HandleDecDynPrefV8, 7)                                \
    T(HandleExpDynPrefV8, 7)                                \
    T(HandleIsInDynPrefV8, 7)                               \
    T(HandleInstanceOfDynPrefV8, 7)                         \
    T(HandleStrictNotEqDynPrefV8, 7)                        \
    T(HandleStrictEqDynPrefV8, 7)                           \
    T(HandleResumeGeneratorPrefV8, 7)                       \
    T(HandleGetResumeModePrefV8, 7)                         \
    T(HandleCreateGeneratorObjPrefV8, 7)                    \
    T(HandleThrowConstAssignmentPrefV8, 7)                  \
    T(HandleGetTemplateObjectPrefV8, 7)                     \
    T(HandleGetNextPropNamePrefV8, 7)                       \
    T(HandleCallArg0DynPrefV8, 7)                           \
    T(HandleThrowIfNotObjectPrefV8, 7)                      \
    T(HandleIterNextPrefV8, 7)                              \
    T(HandleCloseIteratorPrefV8, 7)                         \
    T(HandleCopyModulePrefV8, 7)                            \
    T(HandleSuperCallSpreadPrefV8, 7)                       \
    T(HandleDelObjPropPrefV8V8, 7)                          \
    T(HandleNewObjSpreadDynPrefV8V8, 7)                     \
    T(HandleCreateIterResultObjPrefV8V8, 7)                 \
    T(HandleSuspendGeneratorPrefV8V8, 7)                    \
    T(HandleAsyncFunctionAwaitUncaughtPrefV8V8, 7)          \
    T(HandleThrowUndefinedIfHolePrefV8V8, 7)                \
    T(HandleCallArg1DynPrefV8V8, 7)                         \
    T(HandleCopyDataPropertiesPrefV8V8, 7)                  \
    T(HandleStArraySpreadPrefV8V8, 7)                       \
    T(HandleGetIteratorNextPrefV8V8, 7)                     \
    T(HandleSetObjectWithProtoPrefV8V8, 7)                  \
    T(HandleLdObjByValuePrefV8V8, 7)                        \
    T(HandleStObjByValuePrefV8V8, 7)                        \
    T(HandleStOwnByValuePrefV8V8, 7)                        \
    T(HandleLdSuperByValuePrefV8V8, 7)                      \
    T(HandleStSuperByValuePrefV8V8, 7)                      \
    T(HandleLdObjByIndexPrefV8Imm32, 7)                     \
    T(HandleStObjByIndexPrefV8Imm32, 7)                     \
    T(HandleStOwnByIndexPrefV8Imm32, 7)                     \
    T(HandleCallSpreadDynPrefV8V8V8, 7)                     \
    T(HandleAsyncFunctionResolvePrefV8V8V8, 7)              \
    T(HandleAsyncFunctionRejectPrefV8V8V8, 7)               \
    T(HandleCallArgs2DynPrefV8V8V8, 7)                      \
    T(HandleCallArgs3DynPrefV8V8V8V8, 7)                    \
    T(HandleDefineGetterSetterByValuePrefV8V8V8V8, 7)       \
    T(HandleNewObjDynRangePrefImm16V8, 7)                   \
    T(HandleCallIRangeDynPrefImm16V8, 7)                    \
    T(HandleCallIThisRangeDynPrefImm16V8, 7)                \
    T(HandleSuperCallPrefImm16V8, 7)                        \
    T(HandleCreateObjectWithExcludedKeysPrefImm16V8V8, 7)   \
    T(HandleDefineFuncDynPrefId16Imm16V8, 7)                \
    T(HandleDefineNCFuncDynPrefId16Imm16V8, 7)              \
    T(HandleDefineGeneratorFuncPrefId16Imm16V8, 7)          \
    T(HandleDefineAsyncFuncPrefId16Imm16V8, 7)              \
    T(HandleDefineMethodPrefId16Imm16V8, 7)                 \
    T(HandleNewLexEnvDynPrefImm16, 7)                       \
    T(HandleCopyRestArgsPrefImm16, 7)                       \
    T(HandleCreateArrayWithBufferPrefImm16, 7)              \
    T(HandleCreateObjectHavingMethodPrefImm16, 7)           \
    T(HandleThrowIfSuperNotCorrectCallPrefImm16, 7)         \
    T(HandleCreateObjectWithBufferPrefImm16, 7)             \
    T(HandleLdLexVarDynPrefImm4Imm4, 7)                     \
    T(HandleLdLexVarDynPrefImm8Imm8, 7)                     \
    T(HandleLdLexVarDynPrefImm16Imm16, 7)                   \
    T(HandleStLexVarDynPrefImm4Imm4V8, 7)                   \
    T(HandleStLexVarDynPrefImm8Imm8V8, 7)                   \
    T(HandleStLexVarDynPrefImm16Imm16V8, 7)                 \
    T(HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8, 7) \
    T(HandleGetModuleNamespacePrefId32, 7)                  \
    T(HandleStModuleVarPrefId32, 7)                         \
    T(HandleTryLdGlobalByNamePrefId32, 7)                   \
    T(HandleTryStGlobalByNamePrefId32, 7)                   \
    T(HandleLdGlobalVarPrefId32, 7)                         \
    T(HandleStGlobalVarPrefId32, 7)                         \
    T(HandleLdObjByNamePrefId32V8, 7)                       \
    T(HandleStObjByNamePrefId32V8, 7)                       \
    T(HandleStOwnByNamePrefId32V8, 7)                       \
    T(HandleLdSuperByNamePrefId32V8, 7)                     \
    T(HandleStSuperByNamePrefId32V8, 7)                     \
    T(HandleLdModuleVarPrefId32Imm8, 7)                     \
    T(HandleCreateRegExpWithLiteralPrefId32Imm8, 7)         \
    T(HandleIsTruePref, 7)                                  \
    T(HandleIsFalsePref, 7)                                 \
    T(HandleStConstToGlobalRecordPrefId32, 7)               \
    T(HandleStLetToGlobalRecordPrefId32, 7)                 \
    T(HandleStClassToGlobalRecordPrefId32, 7)               \
    T(HandleStOwnByValueWithNameSetPrefV8V8, 7)             \
    T(HandleStOwnByNameWithNameSetPrefId32V8, 7)            \
    T(HandleLdFunctionPref, 7)                              \
    T(HandleNewLexEnvWithNameDynPrefImm16Imm16, 7)          \
    T(HandleLdBigIntPrefId32, 7)                            \
    T(HandleMovDynV8V8, 7)                                  \
    T(HandleMovDynV16V16, 7)                                \
    T(HandleLdaStrId32, 7)                                  \
    T(HandleLdaiDynImm32, 7)                                \
    T(HandleFldaiDynImm64, 7)                               \
    T(HandleJmpImm8, 7)                                     \
    T(HandleJmpImm16, 7)                                    \
    T(HandleJmpImm32, 7)                                    \
    T(HandleJeqzImm8, 7)                                    \
    T(HandleJeqzImm16, 7)                                   \
    T(HandleLdaDynV8, 7)                                    \
    T(HandleStaDynV8, 7)                                    \
    T(HandleReturnDyn, 7)                                   \
    T(HandleMovV4V4, 7)                                     \
    T(HandleJnezImm8, 7)                                    \
    T(HandleJnezImm16, 7)                                   \
    T(ExceptionHandler, 7)

#define INTERPRETER_IGNORED_BC_STUB_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(IGNORE_BC_STUB, IGNORE_BC_STUB, V)

#define INTERPRETER_BC_STUB_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(IGNORE_BC_STUB, V, V)

#define ASM_INTERPRETER_BC_STUB_ID_LIST(V) \
    ASM_INTERPRETER_BC_STUB_LIST(V, V, V)

class InterpreterStub : public Stub {
public:
    InterpreterStub(const char* name, int argCount, Circuit *circuit)
        : Stub(name, argCount, circuit) {}
    ~InterpreterStub() = default;
    NO_MOVE_SEMANTIC(InterpreterStub);
    NO_COPY_SEMANTIC(InterpreterStub);
    virtual void GenerateCircuit(const CompilationConfig *cfg) = 0;

    inline void SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val);
    inline GateRef GetVregValue(GateRef sp, GateRef idx);
    inline GateRef ReadInst4_0(GateRef pc);
    inline GateRef ReadInst4_1(GateRef pc);
    inline GateRef ReadInst4_2(GateRef pc);
    inline GateRef ReadInst4_3(GateRef pc);
    inline GateRef ReadInst8_0(GateRef pc);
    inline GateRef ReadInst8_1(GateRef pc);
    inline GateRef ReadInst8_2(GateRef pc);
    inline GateRef ReadInst8_3(GateRef pc);
    inline GateRef ReadInst8_4(GateRef pc);
    inline GateRef ReadInst8_5(GateRef pc);
    inline GateRef ReadInst8_6(GateRef pc);
    inline GateRef ReadInst8_7(GateRef pc);
    inline GateRef ReadInst8_8(GateRef pc);
    inline GateRef ReadInst16_0(GateRef pc);
    inline GateRef ReadInst16_1(GateRef pc);
    inline GateRef ReadInst16_2(GateRef pc);
    inline GateRef ReadInst16_3(GateRef pc);
    inline GateRef ReadInst16_5(GateRef pc);
    inline GateRef ReadInstSigned8_0(GateRef pc);
    inline GateRef ReadInstSigned16_0(GateRef pc);
    inline GateRef ReadInstSigned32_0(GateRef pc);
    inline GateRef ReadInst32_0(GateRef pc);
    inline GateRef ReadInst32_1(GateRef pc);
    inline GateRef ReadInst32_2(GateRef pc);
    inline GateRef ReadInst64_0(GateRef pc);

    inline GateRef GetFrame(GateRef frame);
    inline GateRef GetCurrentSpFrame(GateRef glue);
    inline GateRef GetPcFromFrame(GateRef frame);
    inline GateRef GetCallSizeFromFrame(GateRef frame);
    inline GateRef GetFunctionFromFrame(GateRef frame);
    inline GateRef GetAccFromFrame(GateRef frame);
    inline GateRef GetEnvFromFrame(GateRef frame);
    inline GateRef GetEnvFromFunction(GateRef frame);
    inline GateRef GetConstpoolFromFunction(GateRef function);
    inline GateRef GetProfileTypeInfoFromFunction(GateRef function);
    inline GateRef GetModuleFromFunction(GateRef function);
    inline GateRef GetResumeModeFromGeneratorObject(GateRef obj);

    inline void SetCurrentSpFrame(GateRef glue, GateRef sp);
    inline void SetPcToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetCallSizeToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetAccToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetEnvToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetConstantPoolToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetResolvedToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetModuleToFunction(GateRef glue, GateRef function, GateRef value);

    inline void Dispatch(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                         GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter, GateRef format);
    inline void DispatchLast(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                             GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);
    template<RuntimeStubCSigns::ID id, typename... Args>
    void DispatchCommonCall(GateRef glue, GateRef function, Args... args);
    template<RuntimeStubCSigns::ID id, typename... Args>
    GateRef CommonCallNative(GateRef glue, GateRef function, Args... args);
    inline GateRef FunctionIsResolved(GateRef object);
    inline GateRef GetObjectFromConstPool(GateRef constpool, GateRef index);

private:
    template<typename... Args>
    void DispatchBase(GateRef bcOffset, const CallSignature *signature, GateRef glue, Args... args);
};

class AsmInterpreterEntryStub : public InterpreterStub {
public:
    // 7 : 7 means argument counts
    explicit AsmInterpreterEntryStub(Circuit *circuit) : InterpreterStub("AsmInterpreterEntry", 7, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~AsmInterpreterEntryStub() = default;
    NO_MOVE_SEMANTIC(AsmInterpreterEntryStub);
    NO_COPY_SEMANTIC(AsmInterpreterEntryStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;

private:
    void GenerateCircuitImpl(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                             GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);
};

#define DECLARE_HANDLE_STUB_CLASS(name, argc)                                                   \
    class name##Stub : public InterpreterStub {                                                 \
    public:                                                                                     \
        explicit name##Stub(Circuit *circuit) : InterpreterStub(#name, argc, circuit)           \
        {                                                                                       \
            circuit->SetFrameType(FrameType::INTERPRETER_FRAME);                                \
        }                                                                                       \
        ~name##Stub() = default;                                                                \
        NO_MOVE_SEMANTIC(name##Stub);                                                           \
        NO_COPY_SEMANTIC(name##Stub);                                                           \
        void GenerateCircuit(const CompilationConfig *cfg) override;                            \
                                                                                                \
    private:                                                                                    \
        void GenerateCircuitImpl(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,       \
                                 GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter); \
    };
    INTERPRETER_BC_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)
    DECLARE_HANDLE_STUB_CLASS(SingleStepDebugging, 7)
    DECLARE_HANDLE_STUB_CLASS(HandleOverflow, 7)
#undef DECLARE_HANDLE_STUB_CLASS

class BytecodeStubCSigns {
public:
    enum ValidID {
#define DEF_VALID_BC_STUB_ID(name, counter) name,
        INTERPRETER_BC_STUB_LIST(DEF_VALID_BC_STUB_ID)
#undef DEF_VALID_BC_STUB_ID
        NUM_OF_VALID_STUBS
    };

    enum ID {
#define DEF_BC_STUB_ID(name, counter) ID_##name,
        ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_BC_STUB_ID)
#undef DEF_BC_STUB_ID
        NUM_OF_ALL_STUBS
    };
    static void Initialize();

    static void GetCSigns(std::vector<CallSignature*>& outCSigns);

    static const CallSignature* Get(size_t index)
    {
        ASSERT(index < NUM_OF_VALID_STUBS);
        return &callSigns_[index];
    }
private:
    static CallSignature callSigns_[NUM_OF_VALID_STUBS];
};

#define BYTECODE_STUB_BEGIN_ID BytecodeStubCSigns::ID_HandleLdNanPref
#define BYTECODE_STUB_END_ID BytecodeStubCSigns::ID_ExceptionHandler
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
