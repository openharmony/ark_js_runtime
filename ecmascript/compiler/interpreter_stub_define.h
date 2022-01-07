#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H

namespace panda::ecmascript::kungfu {

#define INTERPRETER_STUB_HELPER_LIST(V) \
    V(AsmInterpreterEntry, 7)

#define INTERPRETER_STUB_LIST(V) \
    V(SingleStepDebugging, 7)    \
    V(HandleLdnanPref, 7)        \
    V(HandleLdaDyn, 7)           \
    V(HandleStaDyn, 7)           \

enum InterpreterStubId {
#define DEF_STUB(name, counter) name##Id,
    INTERPRETER_STUB_LIST(DEF_STUB) INTERPRETER_STUB_MAXCOUNT,
#undef DEF_STUB
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_DEFINE_H
