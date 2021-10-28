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
#include "generated/stub_aot_options_gen.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"
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

void StubAotCompiler::BuildStubModuleAndSave(const char *triple, panda::ecmascript::StubModule *module,
                                             const std::string &filename)
{
    LLVMStubModule stubModule("fast_stubs", triple);
    stubModule.Initialize();
    for (int i = 0; i < FAST_STUB_MAXCOUNT; i++) {
        auto stub = stubs_[i];
        if (stub != nullptr) {
            std::cout << "Stub Name: " << stub->GetMethodName() << std::endl;
            stub->GenerateCircuit();
            auto circuit = stub->GetEnvironment()->GetCircuit();
            PassPayLoad data(circuit, &stubModule);
            PassRunner pipeline(&data);
            pipeline.RunPass<VerifierPass>();
            pipeline.RunPass<SchedulerPass>();
            pipeline.RunPass<LLVMCodegenPass>(i);
        }
    }

    LLVMModuleAssembler assembler(&stubModule, triple);
    assembler.AssembleModule();
    assembler.AssembleStubModule(module);

    auto codeSize = assembler.GetCodeSize();
    panda::ecmascript::MachineCode *code = reinterpret_cast<panda::ecmascript::MachineCode *>(
        new uint64_t[(panda::ecmascript::MachineCode::SIZE + codeSize) / sizeof(uint64_t) + 1]);
    code->SetInstructionSizeInBytes(nullptr, panda::ecmascript::JSTaggedValue(codeSize),
                                    panda::ecmascript::SKIP_BARRIER);

    assembler.CopyAssemblerToCode(code);

    module->SetCode(code);
    module->Save(filename);

    delete code;
}
}  // namespace kungfu

#define SET_STUB_TO_MODULE(module, name) \
    kungfu::Circuit name##Circuit; \
    kungfu::name##Stub name##Stub(& name##Circuit); \
    module.SetStub(FAST_STUB_ID(name), & name##Stub);
#define SET_ALL_STUB_TO_MODEULE(module) \
    SET_STUB_TO_MODULE(module, FastAdd) \
    SET_STUB_TO_MODULE(module, FastSub) \
    SET_STUB_TO_MODULE(module, FastMul) \
    SET_STUB_TO_MODULE(module, FastMulGC) \
    SET_STUB_TO_MODULE(module, FastDiv) \
    SET_STUB_TO_MODULE(module, FastMod) \
    SET_STUB_TO_MODULE(module, FastTypeOf) \
    SET_STUB_TO_MODULE(module, FindOwnElement) \
    SET_STUB_TO_MODULE(module, GetElement) \
    SET_STUB_TO_MODULE(module, FindOwnElement2) \
    SET_STUB_TO_MODULE(module, SetElement) \
    SET_STUB_TO_MODULE(module, GetPropertyByIndex) \
    SET_STUB_TO_MODULE(module, SetPropertyByIndex) \
    SET_STUB_TO_MODULE(module, FunctionCallInternal) \
    SET_STUB_TO_MODULE(module, GetPropertyByName)

int main(const int argc, const char **argv)
{
    panda::Span<const char *> sp(argv, argc);
    panda::Stub_Aot_Options stubOptions(sp[0]);
    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print compiler options");
    panda::PandArgParser paParser;

    stubOptions.AddOptions(&paParser);
    paParser.Add(&help);
    paParser.Add(&options);

    if (!paParser.Parse(argc, argv) || help.GetValue()) {
        std::cerr << paParser.GetErrorString() << std::endl;
        std::cerr << "Usage: " << "ark_stub_opt" << " [OPTIONS]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "optional arguments:" << std::endl;
        std::cerr << paParser.GetHelpString() << std::endl;
        return 1;
    }

    std::string moduleFilename = stubOptions.GetStubOutputFile();
    std::string tripes = stubOptions.GetTargetTriple();

    kungfu::StubAotCompiler mouldeBuilder;
    SET_ALL_STUB_TO_MODEULE(mouldeBuilder);

    panda::ecmascript::StubModule stubModule;
    mouldeBuilder.BuildStubModuleAndSave(tripes.c_str(), &stubModule, moduleFilename);

    std::cout << "BuildStubModuleAndSave success" << std::endl;
    return 0;
}