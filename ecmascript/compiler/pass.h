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

#ifndef ECMASCRIPT_COMPILER_PASS_H
#define ECMASCRIPT_COMPILER_PASS_H

#include "bytecode_circuit_builder.h"
#include "verifier.h"
#include "scheduler.h"
#include "fast_stub.h"
#include "generic_lowering.h"
#include "llvm_codegen.h"

namespace panda::ecmascript::kungfu {
class PassData {
public:
    explicit PassData(Circuit* circuit) : circuit_(circuit) {}
    ~PassData() = default;
    const ControlFlowGraph &GetScheduleResult() const
    {
        return cfg_;
    }

    void SetScheduleResult(const ControlFlowGraph &result)
    {
        cfg_ = result;
    }

    virtual Circuit* GetCircuit() const
    {
        return circuit_;
    }

private:
    Circuit* circuit_;
    ControlFlowGraph cfg_;
};

template<typename T1>
class PassRunner {
public:
    explicit PassRunner(T1* data) : data_(data) {}
    ~PassRunner() = default;
    template<typename T2, typename... Args>
    bool RunPass(Args... args)
    {
        T2 pass;
        return pass.Run(data_, std::forward<Args>(args)...);
    }

private:
    T1* data_;
};

class GenericLoweringPass {
public:
    bool Run(PassData* data, BytecodeCircuitBuilder *builder)
    {
        GenericLowering lowering(builder, data->GetCircuit());
        lowering.CallRuntimeLowering();
        return true;
    }
};

class VerifierPass {
public:
    bool Run(PassData* data)
    {
        Verifier::Run(data->GetCircuit());
        return true;
    }
};

class SchedulingPass {
public:
    bool Run(PassData* data)
    {
        data->SetScheduleResult(Scheduler::Run(data->GetCircuit()));
        return true;
    }
};

class LLVMIRGenPass {
public:
    bool Run(PassData* data)
    {
        return true;
    }

private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};
}
#endif
