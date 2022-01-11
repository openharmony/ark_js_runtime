#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/stub-inl.h"

namespace panda::ecmascript::kungfu {

#define DECLARE_HANDLE_STUB_CLASS(name, argc)                                    \
    class name##Stub : public Stub {                                             \
        public:                                                                  \
            /* 2 : 2 means argument counts */                                    \
            explicit name##Stub(Circuit *circuit) : Stub(#name, 7, circuit) \
        {                                                                        \
            circuit->SetFrameType(FrameType::INTERPRETER_FRAME);                 \
        }                                                                        \
        ~name##Stub() = default;                                                 \
        NO_MOVE_SEMANTIC(name##Stub);                                            \
        NO_COPY_SEMANTIC(name##Stub);                                            \
        void GenerateCircuit(const CompilationConfig *cfg) override;             \
    };
    INTERPRETER_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)
#undef DECLARE_HANDLE_STUB_CLASS
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
