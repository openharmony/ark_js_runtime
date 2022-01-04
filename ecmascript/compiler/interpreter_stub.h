#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/stub-inl.h"

namespace panda::ecmascript::kungfu {
class HandleLdnanPrefStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit HandleLdnanPrefStub(Circuit *circuit) : Stub("HandleLdnanPref", 7, circuit)
    {
        circuit->SetFrameType(FrameType::INTERPRETER_FRAME);
    }
    ~HandleLdnanPrefStub() = default;
    NO_MOVE_SEMANTIC(HandleLdnanPrefStub);
    NO_COPY_SEMANTIC(HandleLdnanPrefStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SingleStepDebuggingStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit SingleStepDebuggingStub(Circuit *circuit) : Stub("SingleStepDebugging", 7, circuit)
    {
        circuit->SetFrameType(FrameType::INTERPRETER_FRAME);
    }
    ~SingleStepDebuggingStub() = default;
    NO_MOVE_SEMANTIC(SingleStepDebuggingStub);
    NO_COPY_SEMANTIC(SingleStepDebuggingStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
