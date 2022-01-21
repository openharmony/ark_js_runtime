#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/stub-inl.h"

namespace panda::ecmascript::kungfu {

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
    inline GateRef GetFunctionFromFrame(GateRef frame);
    inline GateRef GetAccFromFrame(GateRef frame);
    inline GateRef GetEnvFromFrame(GateRef frame);
    inline GateRef GetConstpoolFromFunction(GateRef function);
    inline GateRef GetProfileTypeInfoFromFunction(GateRef function);

    inline void SetCurrentSpFrame(GateRef glue, GateRef sp);
    inline void SetPcToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetAccToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetEnvToFrame(GateRef glue, GateRef frame, GateRef value);

    inline void Dispatch(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                         GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter, GateRef format);
    inline void DispatchLast(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,
                             GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);

    inline GateRef FunctionIsResolved(GateRef object);
    inline GateRef GetObjectFromConstPool(GateRef constpool, GateRef index);
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
       public:                                                                                  \
        explicit name##Stub(Circuit *circuit) : InterpreterStub(#name, argc, circuit)           \
        {                                                                                       \
            circuit->SetFrameType(FrameType::INTERPRETER_FRAME);                                \
        }                                                                                       \
        ~name##Stub() = default;                                                                \
        NO_MOVE_SEMANTIC(name##Stub);                                                           \
        NO_COPY_SEMANTIC(name##Stub);                                                           \
        void GenerateCircuit(const CompilationConfig *cfg) override;                            \
                                                                                                \
       private:                                                                                 \
        void GenerateCircuitImpl(GateRef glue, GateRef pc, GateRef sp, GateRef constpool,       \
                                 GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter); \
    };
    INTERPRETER_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)
    DECLARE_HANDLE_STUB_CLASS(SingleStepDebugging, 7)
#undef DECLARE_HANDLE_STUB_CLASS
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
