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

#include "stub_aot_compiler.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "fast_stub.h"
#include "llvm_codegen.h"
#include "scheduler.h"
#include "stub.h"
#include "verifier.h"

namespace kungfu {
class PassPayLoad {
public:
    explicit PassPayLoad(Circuit *circuit, LLVMStubModule *module) : circuit_(circuit), module_(module) {}
    ~PassPayLoad() = default;
    const ControlFlowGraph &GetScheduleResult() const
    {
        return cfg_;
    }

    void SetScheduleResult(const ControlFlowGraph &result)
    {
        cfg_ = result;
    }

    Circuit *GetCircuit() const
    {
        return circuit_;
    }

    LLVMStubModule *GetStubModule() const
    {
        return module_;
    }

private:
    Circuit *circuit_;
    LLVMStubModule *module_;
    ControlFlowGraph cfg_;
};

class PassRunner {
public:
    explicit PassRunner(PassPayLoad *data) : data_(data) {}
    ~PassRunner() = default;
    template <typename T, typename... Args>
    bool RunPass(Args... args)
    {
        T pass;
        return pass.Run(data_, std::forward<Args>(args)...);
    }

private:
    PassPayLoad *data_;
};

class VerifierPass {
public:
    bool Run(PassPayLoad *data)
    {
        Verifier::Run(data->GetCircuit());
        return true;
    }
};

class SchedulerPass {
public:
    bool Run(PassPayLoad *data)
    {
        data->SetScheduleResult(Scheduler::Run(data->GetCircuit()));
        return true;
    }
};

class LLVMCodegenPass {
public:
    bool Run(PassPayLoad *data, int index)
    {
        auto stubModule = data->GetStubModule();
        LLVMCodeGeneratorImpl llvmImpl(stubModule);
        CodeGenerator codegen(&llvmImpl);
        codegen.Run(data->GetCircuit(), data->GetScheduleResult(), index);
        return true;
    }
};

void StubAotCompiler::BuildStubModule(panda::ecmascript::StubModule *module)
{
    LLVMStubModule stubModule("fast_stubs");
    stubModule.Initialize();
    for (int i = 0; i < FAST_STUB_MAXCOUNT; i++) {
        auto Stub = stubs_[i];
        if (Stub != nullptr) {
            Stub->GenerateCircuit();
            auto circuit = Stub->GetEnvironment()->GetCircuit();
            PassPayLoad data(circuit, &stubModule);
            PassRunner pipeline(&data);
            pipeline.RunPass<VerifierPass>();
            pipeline.RunPass<SchedulerPass>();
            pipeline.RunPass<LLVMCodegenPass>(i);
        }
    }

    LLVMModuleAssembler assembler(&stubModule);
    assembler.AssembleModule();
    assembler.CopyAssembleCodeToModule(module);
}
}  // namespace kungfu

int main(int argc, char *argv[])
{
    int opt;
    // 3 means ark_stub_opt -f stub.m
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " -f stub.m" << std::endl;
        return -1;
    }
    std::string moduleFilename;
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                moduleFilename += optarg;
                break;
            default: /* '?' */
                std::cerr << "Usage: " << argv[0] << " -f stub.m" << std::endl;
                return -1;
        }
    }

    kungfu::StubAotCompiler mouldeBuilder;
    panda::ecmascript::StubModule stubModule;
    /* Set Stub into module */
    kungfu::Circuit fastaddCircuit;
    kungfu::FastAddStub fastaddStub(&fastaddCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(FastAdd), &fastaddStub);

    kungfu::Circuit fastsubCircuit;
    kungfu::FastSubStub fastsubStub(&fastsubCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(FastSub), &fastsubStub);

    kungfu::Circuit fastmulCircuit;
    kungfu::FastMulStub fastmulStub(&fastmulCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(FastMul), &fastmulStub);

    kungfu::Circuit fastdivCircuit;
    kungfu::FastDivStub fastdivStub(&fastdivCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(FastDiv), &fastdivStub);

    kungfu::Circuit fastFindOwnElementCircuit;
    kungfu::FastFindOwnElementStub fastFindOwnElementStub(&fastFindOwnElementCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(FindOwnElement), &fastFindOwnElementStub);

    kungfu::Circuit fastGetElementCircuit;
    kungfu::FastGetElementStub fastGetElementStub(&fastGetElementCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(GetElement), &fastGetElementStub);

    kungfu::Circuit fastFindOwnElement2Circuit;
    kungfu::FastFindOwnElement2Stub fastFindOwnElement2Stub(&fastFindOwnElement2Circuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(FindOwnElement2), &fastFindOwnElement2Stub);

    kungfu::Circuit fastSetElementCircuit;
    kungfu::FastSetElementStub fastSetElementStub(&fastSetElementCircuit);
    mouldeBuilder.SetStub(FAST_STUB_ID(SetElement), &fastSetElementStub);

    mouldeBuilder.BuildStubModule(&stubModule);
    stubModule.Save(moduleFilename);
    exit(0);
}