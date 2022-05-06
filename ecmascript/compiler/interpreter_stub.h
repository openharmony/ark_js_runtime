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
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub.h"

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
    inline GateRef GetLastLeaveFrame(GateRef glue);
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
    inline GateRef GetHotnessCounterFromMethod(GateRef method);

    inline void SetHotnessCounter(GateRef glue, GateRef method, GateRef value);
    inline void SetCurrentSpFrame(GateRef glue, GateRef sp);
    inline void SetLastLeaveFrame(GateRef glue, GateRef sp);
    inline void SetPcToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetCallSizeToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetAccToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetEnvToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetConstantPoolToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetResolvedToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetModuleToFunction(GateRef glue, GateRef function, GateRef value);

    inline void Dispatch(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                         GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter, GateRef format);
    inline void DispatchLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                             GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);
    inline void DispatchDebugger(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                 GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);
    inline void DispatchDebuggerLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
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

#define DECLARE_HANDLE_STUB_CLASS(name)                                                         \
    class name##Stub : public InterpreterStub {                                                 \
    public:                                                                                     \
        explicit name##Stub(Circuit *circuit) : InterpreterStub(#name,                          \
            static_cast<int>(InterpreterHandlerInputs::NUM_OF_INPUTS), circuit)                 \
        {                                                                                       \
            circuit->SetFrameType(FrameType::INTERPRETER_FRAME);                                \
        }                                                                                       \
        ~name##Stub() = default;                                                                \
        NO_MOVE_SEMANTIC(name##Stub);                                                           \
        NO_COPY_SEMANTIC(name##Stub);                                                           \
        void GenerateCircuit(const CompilationConfig *cfg) override;                            \
                                                                                                \
    private:                                                                                    \
        void GenerateCircuitImpl(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,       \
                                 GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter); \
    };
    INTERPRETER_BC_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)
    ASM_INTERPRETER_BC_HELPER_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)
#undef DECLARE_HANDLE_STUB_CLASS
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
