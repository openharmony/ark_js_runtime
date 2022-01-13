#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H

namespace panda::ecmascript::kungfu {

#define INTERPRETER_STUB_HELPER_LIST(V) \
    V(AsmInterpreterEntry, 7)

#define INTERPRETER_STUB_LIST(V)                \
    V(SingleStepDebugging, 7)                   \
    V(HandleLdnanPref, 7)                       \
    V(HandleLdInfinityPref, 7)                  \
    V(HandleLdUndefinedPref, 7)                 \
    V(HandleLdNullPref, 7)                      \
    V(HandleLdTruePref, 7)                      \
    V(HandleLdFalsePref, 7)                     \
    V(HandleLdaDynV8, 7)                        \
    V(HandleStaDynV8, 7)                        \
    V(HandleJmpImm8, 7)                         \
    V(HandleJmpImm16, 7)                        \
    V(HandleJmpImm32, 7)                        \
    V(HandleLdLexVarDynPrefImm4Imm4, 7)         \
    V(HandleLdLexVarDynPrefImm8Imm8, 7)         \
    V(HandleLdLexVarDynPrefImm16Imm16, 7)       \
    V(HandleStLexVarDynPrefImm4Imm4V8, 7)       \
    V(HandleStLexVarDynPrefImm8Imm8V8, 7)       \
    V(HandleStLexVarDynPrefImm16Imm16V8, 7)     \
    V(HandleNegDynPrefV8, 7)                    \
    V(HandleIncdynPrefV8, 7)                    \
    V(HandleDecdynPrefV8, 7)                    \
    V(HandleExpdynPrefV8, 7)                    \
    V(HandleIsindynPrefV8, 7)                   \
    V(HandleInstanceofdynPrefV8, 7)             \
    V(HandleStrictnoteqdynPrefV8, 7)            \
    V(HandleStricteqdynPrefV8, 7)               \
    V(HandleResumegeneratorPrefV8, 7)           \
    V(HandleGetresumemodePrefV8, 7)             \
    V(HandleCreategeneratorobjPrefV8, 7)        \
    V(HandleStConstToGlobalRecordPrefId32, 7)   \
    V(HandleStLetToGlobalRecordPrefId32, 7)     \
    V(HandleStClassToGlobalRecordPrefId32, 7)   \
    V(HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8, 7)

enum InterpreterStubId {
#define DEF_STUB(name, counter) name##Id,
    INTERPRETER_STUB_LIST(DEF_STUB) INTERPRETER_STUB_MAXCOUNT,
#undef DEF_STUB
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
