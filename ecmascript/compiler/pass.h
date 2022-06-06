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

#include "ecmascript/compiler/async_function_lowering.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/slowpath_lowering.h"
#include "ecmascript/compiler/type_inference/type_infer.h"
#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/compiler/verifier.h"

namespace panda::ecmascript::kungfu {
class PassData {
public:
    explicit PassData(Circuit* circuit) : circuit_(circuit) {}
    virtual ~PassData() = default;
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
    explicit PassRunner(T1* data, bool enableLog = false)
        : data_(data), enableLog_(enableLog) {}
    virtual ~PassRunner() = default;
    template<typename T2, typename... Args>
    bool RunPass(Args... args)
    {
        T2 pass;
        return pass.Run(data_, enableLog_, std::forward<Args>(args)...);
    }

private:
    T1* data_;
    bool enableLog_ {false};
};

class TypeInferPass {
public:
    bool Run(PassData* data, bool enableLog, BytecodeCircuitBuilder *builder, TSLoader *tsLoader)
    {
        TypeInfer typeInfer(builder, data->GetCircuit(), tsLoader, enableLog);
        typeInfer.TraverseCircuit();
        return true;
    }
};

class TypeLoweringPass {
public:
    bool Run(PassData *data, bool enableLog, BytecodeCircuitBuilder *builder, CompilationConfig *cmpCfg,
             TSLoader *tsLoader)
    {
        TypeLowering lowering(builder, data->GetCircuit(), cmpCfg, tsLoader, enableLog);
        lowering.RunTypeLowering();
        return true;
    }
};

class SlowPathLoweringPass {
public:
    bool Run(PassData* data, bool enableLog, BytecodeCircuitBuilder *builder, CompilationConfig *cmpCfg)
    {
        SlowPathLowering lowering(builder, data->GetCircuit(), cmpCfg, enableLog);
        lowering.CallRuntimeLowering();
        return true;
    }
};

class VerifierPass {
public:
    bool Run(PassData* data, bool enableLog)
    {
        Verifier::Run(data->GetCircuit(), enableLog);
        return true;
    }
};

class SchedulingPass {
public:
    bool Run(PassData* data, bool enableLog)
    {
        data->SetScheduleResult(Scheduler::Run(data->GetCircuit(), enableLog));
        return true;
    }
};

class LLVMIRGenPass {
public:
    void CreateCodeGen(LLVMModule *module, bool enableLog)
    {
        llvmImpl_ = std::make_unique<LLVMIRGeneratorImpl>(module, enableLog);
    }
    bool Run(PassData *data, bool enableLog, LLVMModule *module, const JSMethod *method)
    {
        CreateCodeGen(module, enableLog);
        CodeGenerator codegen(llvmImpl_);
        codegen.Run(data->GetCircuit(), data->GetScheduleResult(), module->GetCompilationConfig(), method);
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

class AsyncFunctionLoweringPass {
public:
    bool Run(PassData* data, bool enableLog, BytecodeCircuitBuilder *builder, CompilationConfig *cmpCfg)
    {
        AsyncFunctionLowering lowering(builder, data->GetCircuit(), cmpCfg, enableLog);
        if (lowering.IsAsyncRelated()) {
            lowering.ProcessAll();
        }
        return true;
    }
};
} // namespace panda::ecmascript::kungfu
#endif
