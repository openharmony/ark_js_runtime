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

#include "stub_compiler.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "ecmascript/js_thread.h"
#include "ecmascript/base/config.h"
#include "common_stubs.h"
#include "ecmascript/compiler/aot_file_manager.h"
#include "interpreter_stub-inl.h"
#include "generated/stub_aot_options_gen.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"
#include "llvm_codegen.h"
#include "pass.h"
#include "scheduler.h"
#include "stub-inl.h"
#include "verifier.h"

namespace panda::ecmascript::kungfu {
class StubPassData : public PassData {
public:
    explicit StubPassData(Stub *stub, LLVMModule *module) : PassData(nullptr), module_(module), stub_(stub) {}
    ~StubPassData() = default;

    const CompilationConfig *GetCompilationConfig() const
    {
        return module_->GetCompilationConfig();
    }

    Circuit *GetCircuit() const
    {
        return stub_->GetEnvironment()->GetCircuit();
    }

    LLVMModule *GetStubModule() const
    {
        return module_;
    }

    Stub *GetStub() const
    {
        return stub_;
    }

private:
    LLVMModule *module_;
    Stub *stub_;
};

class StubBuildCircuitPass {
public:
    bool Run(StubPassData *data)
    {
        auto stub = data->GetStub();
        std::cerr << "Stub Name: " << stub->GetMethodName() << std::endl;
        stub->GenerateCircuit(data->GetCompilationConfig());
        return true;
    }
};

class StubLLVMIRGenPass {
public:
    void CreateCodeGen(LLVMModule *module)
    {
        llvmImpl_ = std::make_unique<LLVMIRGeneratorImpl>(module);
    }
    bool Run(StubPassData *data, size_t index)
    {
        auto stubModule = data->GetStubModule();
        CreateCodeGen(stubModule);
        CodeGenerator codegen(llvmImpl_);
        codegen.RunForStub(data->GetCircuit(), data->GetScheduleResult(), index, data->GetCompilationConfig());
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

void StubCompiler::RunPipeline(LLVMModule &module)
{
    auto callSigns = module.GetCSigns();
    for (size_t i = 0; i < callSigns.size(); i++) {
        Circuit circuit;
        if (!callSigns[i]->HasConstructor()) {
            continue;
        }
        Stub* stub = static_cast<Stub*>(callSigns[i]->GetConstructor()(reinterpret_cast<void*>(&circuit)));
        StubPassData data(stub, &module);
        PassRunner<StubPassData> pipeline(&data);
        pipeline.RunPass<StubBuildCircuitPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<StubLLVMIRGenPass>(i);
        delete stub;
    }
}

bool StubCompiler::BuildStubModuleAndSave(const std::string &triple, const std::string &commonStubFile,
    const std::string &bcHandlerStubFile)
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    size_t res = 0;
    if (!commonStubFile.empty()) {
        std::cerr << "compiling common stubs" << std::endl;
        LLVMModule commonStubModule("com_stub", triple);
        commonStubModule.SetUpForCommonStubs();
        RunPipeline(commonStubModule);
        AotFileManager manager(&commonStubModule);
        manager.SaveStubFile(commonStubFile);
        std::cerr << "finish" << std::endl;
        res++;
    }

    if (!bcHandlerStubFile.empty()) {
        std::cerr << "compiling bytecode handler stubs" << std::endl;
        LLVMModule bcHandlerStubModule("bc_stub", triple);
        bcHandlerStubModule.SetUpForBytecodeHandlerStubs();
        RunPipeline(bcHandlerStubModule);
        AotFileManager manager(&bcHandlerStubModule, false);
        manager.SaveStubFile(bcHandlerStubFile);
        std::cerr << "finish" << std::endl;
        res++;
    }
    return (res > 0);
}
}  // namespace panda::ecmascript::kungfu

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
        std::cerr << "Usage: " << "ark_stub_compiler" << " [OPTIONS]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "optional arguments:" << std::endl;

        std::cerr << paParser.GetHelpString() << std::endl;
        return 1;
    }

    std::string tripleString = stubOptions.GetTargetTriple();
    std::string commonStubFile = stubOptions.WasSetComStubOut() ? stubOptions.GetComStubOut() : "";
    std::string bcHandlerFile = stubOptions.WasSetBcStubOut() ? stubOptions.GetBcStubOut() : "";
    std::string compiledStubList = stubOptions.GetCompiledStubs();

    panda::ecmascript::kungfu::StubCompiler compiler;
    bool res = compiler.BuildStubModuleAndSave(tripleString, commonStubFile, bcHandlerFile);
    std::cerr << "stub compiler run finish, result condition(T/F):" << std::boolalpha << res << std::endl;
    return 0;
}
