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
    V(HandleLdaDyn, 7)                          \
    V(HandleStaDyn, 7)                          \
    V(HandleLdLexVarDynPrefImm4Imm4, 7)         \
    V(HandleLdLexVarDynPrefImm8Imm8, 7)         \
    V(HandleLdLexVarDynPrefImm16Imm16, 7)       \
    V(HandleStLexVarDynPrefImm4Imm4V8, 7)       \
    V(HandleStLexVarDynPrefImm8Imm8V8, 7)       \
    V(HandleStLexVarDynPrefImm16Imm16V8, 7)     \
    V(HandleIncdynPrefV8, 7)                    \
    V(HandleStConstToGlobalRecordPrefId32, 7)   \
    V(HandleStLetToGlobalRecordPrefId32, 7)     \
    V(HandleStClassToGlobalRecordPrefId32, 7)

enum InterpreterStubId {
#define DEF_STUB(name, counter) name##Id,
    INTERPRETER_STUB_LIST(DEF_STUB) INTERPRETER_STUB_MAXCOUNT,
#undef DEF_STUB
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
