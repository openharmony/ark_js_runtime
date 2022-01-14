#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H

namespace panda::ecmascript::kungfu {

#define INTERPRETER_STUB_HELPER_LIST(V) \
    V(AsmInterpreterEntry, 7)           \
    V(SingleStepDebugging, 7)

#define IGNORE_STUB(...)

#define INTERPRETER_STUB_LIST(V) \
    ASM_INTERPRETER_STUB_LIST(IGNORE_STUB, V)

#define ASM_INTERPRETER_ID_LIST(V) \
    ASM_INTERPRETER_STUB_LIST(V, V)

#define ASM_INTERPRETER_STUB_LIST(V, T)                     \
    T(HandleLdNanPref, 7)                                   \
    T(HandleLdInfinityPref, 7)                              \
    V(HandleLdGlobalThisPref, 7)                            \
    T(HandleLdUndefinedPref, 7)                             \
    T(HandleLdNullPref, 7)                                  \
    V(HandleLdSymbolPref, 7)                                \
    V(HandleLdGlobalPref, 7)                                \
    T(HandleLdTruePref, 7)                                  \
    T(HandleLdFalsePref, 7)                                 \
    T(HandleThrowDynPref, 7)                                \
    V(HandleTypeOfDynPref, 7)                               \
    T(HandleLdLexEnvDynPref, 7)                             \
    T(HandlePopLexEnvDynPref, 7)                            \
    V(HandleGetUnmappedArgsPref, 7)                         \
    V(HandleGetPropIteratorPref, 7)                         \
    V(HandleAsyncFunctionEnterPref, 7)                      \
    T(HandleLdHolePref, 7)                                  \
    V(HandleReturnUndefinedPref, 7)                         \
    V(HandleCreateEmptyObjectPref, 7)                       \
    V(HandleCreateEmptyArrayPref, 7)                        \
    V(HandleGetIteratorPref, 7)                             \
    V(HandleThrowThrowNotExistsPref, 7)                     \
    V(HandleThrowPatternNonCoerciblePref, 7)                \
    T(HandleLdHomeObjectPref, 7)                            \
    V(HandleThrowDeleteSuperPropertyPref, 7)                \
    T(HandleDebuggerPref, 7)                                \
    V(HandleAdd2DynPrefV8, 7)                               \
    V(HandleSub2DynPrefV8, 7)                               \
    V(HandleMul2DynPrefV8, 7)                               \
    V(HandleDiv2DynPrefV8, 7)                               \
    V(HandleMod2DynPrefV8, 7)                               \
    V(HandleEqDynPrefV8, 7)                                 \
    V(HandleNotEqDynPrefV8, 7)                              \
    V(HandleLessDynPrefV8, 7)                               \
    V(HandleLessEqDynPrefV8, 7)                             \
    V(HandleGreaterDynPrefV8, 7)                            \
    V(HandleGreaterEqDynPrefV8, 7)                          \
    V(HandleShl2DynPrefV8, 7)                               \
    V(HandleShr2DynPrefV8, 7)                               \
    V(HandleAshr2DynPrefV8, 7)                              \
    V(HandleAnd2DynPrefV8, 7)                               \
    V(HandleOr2DynPrefV8, 7)                                \
    V(HandleXOr2DynPrefV8, 7)                               \
    V(HandleToNumberPrefV8, 7)                              \
    T(HandleNegDynPrefV8, 7)                                \
    V(HandleNotDynPrefV8, 7)                                \
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
    V(HandleCallArg0DynPrefV8, 7)                           \
    T(HandleThrowIfNotObjectPrefV8, 7)                      \
    T(HandleIterNextPrefV8, 7)                              \
    T(HandleCloseIteratorPrefV8, 7)                         \
    V(HandleCopyModulePrefV8, 7)                            \
    V(HandleSuperCallSpreadPrefV8, 7)                       \
    V(HandleDelObjPropPrefV8V8, 7)                          \
    V(HandleNewObjSpreadDynPrefV8V8, 7)                     \
    V(HandleCreateIterResultObjPrefV8V8, 7)                 \
    V(HandleSuspendGeneratorPrefV8V8, 7)                    \
    V(HandleAsyncFunctionAwaitUncaughtPrefV8V8, 7)          \
    V(HandleThrowUndefinedIfHolePrefV8V8, 7)                \
    V(HandleCallArg1DynPrefV8V8, 7)                         \
    V(HandleCopyDataPropertiesPrefV8V8, 7)                  \
    V(HandleStArraySpreadPrefV8V8, 7)                       \
    V(HandleGetIteratorNextPrefV8V8, 7)                     \
    V(HandleSetObjectWithProtoPrefV8V8, 7)                  \
    V(HandleLdObjByValuePrefV8V8, 7)                        \
    V(HandleStObjByValuePrefV8V8, 7)                        \
    V(HandleStOwnByValuePrefV8V8, 7)                        \
    V(HandleLdSuperByValuePrefV8V8, 7)                      \
    V(HandleStSuperByValuePrefV8V8, 7)                      \
    V(HandleLdObjByIndexPrefV8Imm32, 7)                     \
    V(HandleStObjByIndexPrefV8Imm32, 7)                     \
    V(HandleStOwnByIndexPrefV8Imm32, 7)                     \
    V(HandleCallSpreadDynPrefV8V8V8, 7)                     \
    V(HandleAsyncFunctionResolvePrefV8V8V8, 7)              \
    V(HandleAsyncFunctionRejectPrefV8V8V8, 7)               \
    V(HandleCallArgs2DynPrefV8V8V8, 7)                      \
    V(HandleCallArgs3DynPrefV8V8V8V8, 7)                    \
    V(HandleDefineGetterSetterByValuePrefV8V8V8V8, 7)       \
    V(HandleNewObjDynRangePrefImm16V8, 7)                   \
    V(HandleCallIRangeDynPrefImm16V8, 7)                    \
    V(HandleCallIThisRangeDynPrefImm16V8, 7)                \
    V(HandleSuperCallPrefImm16V8, 7)                        \
    V(HandleCreateObjectWithExcludedKeysPrefImm16V8V8, 7)   \
    V(HandleDefineFuncDynPrefId16Imm16V8, 7)                \
    V(HandleDefineNCFuncDynPrefId16Imm16V8, 7)              \
    V(HandleDefineGeneratorFuncPrefId16Imm16V8, 7)          \
    V(HandleDefineAsyncFuncPrefId16Imm16V8, 7)              \
    V(HandleDefineMethodPrefId16Imm16V8, 7)                 \
    V(HandleNewLexEnvDynPrefImm16, 7)                       \
    V(HandleCopyRestArgsPrefImm16, 7)                       \
    V(HandleCreateArrayWithBufferPrefImm16, 7)              \
    V(HandleCreateObjectHavingMethodPrefImm16, 7)           \
    V(HandleThrowIfSuperNotCorrectCallPrefImm16, 7)         \
    V(HandleCreateObjectWithBufferPrefImm16, 7)             \
    T(HandleLdLexVarDynPrefImm4Imm4, 7)                     \
    T(HandleLdLexVarDynPrefImm8Imm8, 7)                     \
    T(HandleLdLexVarDynPrefImm16Imm16, 7)                   \
    T(HandleStLexVarDynPrefImm4Imm4V8, 7)                   \
    T(HandleStLexVarDynPrefImm8Imm8V8, 7)                   \
    T(HandleStLexVarDynPrefImm16Imm16V8, 7)                 \
    T(HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8, 7) \
    T(HandleImportModulePrefId32, 7)                        \
    T(HandleStModuleVarPrefId32, 7)                         \
    V(HandleTryLdGlobalByNamePrefId32, 7)                   \
    V(HandleTryStGlobalByNamePrefId32, 7)                   \
    V(HandleLdGlobalVarPrefId32, 7)                         \
    V(HandleStGlobalVarPrefId32, 7)                         \
    T(HandleLdObjByNamePrefId32V8, 7)                       \
    V(HandleStObjByNamePrefId32V8, 7)                       \
    V(HandleStOwnByNamePrefId32V8, 7)                       \
    V(HandleLdSuperByNamePrefId32V8, 7)                     \
    V(HandleStSuperByNamePrefId32V8, 7)                     \
    T(HandleLdModVarByNamePrefId32V8, 7)                    \
    V(HandleCreateRegExpWithLiteralPrefId32Imm8, 7)         \
    V(HandleIsTruePref, 7)                                  \
    V(HandleIsFalsePref, 7)                                 \
    T(HandleStConstToGlobalRecordPrefId32, 7)               \
    T(HandleStLetToGlobalRecordPrefId32, 7)                 \
    T(HandleStClassToGlobalRecordPrefId32, 7)               \
    T(HandleStOwnByValueWithNameSetPrefV8V8, 7)             \
    T(HandleStOwnByNameWithNameSetPrefId32V8, 7)            \
    T(HandleLdFunctionPref, 7)                              \
    T(HandleMovDynV8V8, 7)                                  \
    T(HandleMovDynV16V16, 7)                                \
    V(HandleLdaStrId32, 7)                                  \
    V(HandleLdaiDynImm32, 7)                                \
    V(HandleFldaiDynImm64, 7)                               \
    T(HandleJmpImm8, 7)                                     \
    T(HandleJmpImm16, 7)                                    \
    T(HandleJmpImm32, 7)                                    \
    V(HandleJeqzImm8, 7)                                    \
    V(HandleJeqzImm16, 7)                                   \
    T(HandleLdaDynV8, 7)                                    \
    T(HandleStaDynV8, 7)                                    \
    V(HandleReturnDyn, 7)                                   \
    T(HandleMovV4V4, 7)                                     \
    V(HandleJnezImm8, 7)                                    \
    V(HandleJnezImm16, 7)                                   \
    V(ExceptionHandler, 7)                                  \

enum InterpreterStubId {
#define DEF_STUB(name, counter) name##Id,
    ASM_INTERPRETER_ID_LIST(DEF_STUB) INTERPRETER_STUB_MAXCOUNT,
#undef DEF_STUB
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
