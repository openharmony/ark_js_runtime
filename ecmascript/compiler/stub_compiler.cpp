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

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/stub-inl.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "generated/base_options.h"
#include "interpreter_stub-inl.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"

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
    bool Run(StubPassData *data, [[maybe_unused]] bool enableLog)
    {
        auto stub = data->GetStub();
        COMPILER_LOG(INFO) << "Stub Name: " << stub->GetMethodName();
        stub->GenerateCircuit(data->GetCompilationConfig());
        return true;
    }
};

class StubLLVMIRGenPass {
public:
    void CreateCodeGen(LLVMModule *module, bool enableLog)
    {
        llvmImpl_ = std::make_unique<LLVMIRGeneratorImpl>(module, enableLog);
    }
    bool Run(StubPassData *data, bool enableLog, size_t index)
    {
        auto stubModule = data->GetStubModule();
        CreateCodeGen(stubModule, enableLog);
        CodeGenerator codegen(llvmImpl_);
        codegen.RunForStub(data->GetCircuit(), data->GetScheduleResult(), index, data->GetCompilationConfig());
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

void StubCompiler::RunPipeline(LLVMModule *module)
{
    auto callSigns = module->GetCSigns();
    const CompilerLog *log = GetLog();
    bool enableLog = log->IsAlwaysEnabled();

    for (size_t i = 0; i < callSigns.size(); i++) {
        Circuit circuit;
        ASSERT(callSigns[i]->HasConstructor());
        Stub* stub = static_cast<Stub*>(callSigns[i]->GetConstructor()(reinterpret_cast<void*>(&circuit)));

        if (!log->IsAlwaysEnabled() && !log->IsAlwaysDisabled()) {  // neither "all" nor "none"
            enableLog = log->IncludesMethod(stub->GetMethodName());
        }

        StubPassData data(stub, module);
        PassRunner<StubPassData> pipeline(&data, enableLog);
        pipeline.RunPass<StubBuildCircuitPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<StubLLVMIRGenPass>(i);
        delete stub;
    }
}

bool StubCompiler::BuildStubModuleAndSave(const std::string &triple, const std::string &stubFile, size_t optLevel)
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    size_t res = 0;
    const CompilerLog *log = GetLog();
    StubFileGenerator generator(log);
    if (!stubFile.empty()) {
        COMPILER_LOG(INFO) << "compiling common stubs";
        LLVMModule comStubModule("com_stub", triple);
        LLVMAssembler comStubAssembler(comStubModule.GetModule(), LOptions(optLevel, true));
        comStubModule.SetUpForCommonStubs();
        RunPipeline(&comStubModule);
        generator.AddModule(&comStubModule, &comStubAssembler);
        res++;
        COMPILER_LOG(INFO) << "compiling bytecode handler stubs";
        LLVMModule bcStubModule("bc_stub", triple);
        LLVMAssembler bcStubAssembler(bcStubModule.GetModule(), LOptions(optLevel, false));
        bcStubModule.SetUpForBytecodeHandlerStubs();
        RunPipeline(&bcStubModule);
        generator.AddModule(&bcStubModule, &bcStubAssembler);
        res++;
        generator.SaveStubFile(stubFile);
    }
    return (res > 0);
}
}  // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    panda::Span<const char *> sp(argv, argc);
    panda::ecmascript::JSRuntimeOptions runtimeOptions;
    panda::base_options::Options baseOptions(sp[0]);
    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print options");
    panda::PandArgParser paParser;

    runtimeOptions.AddOptions(&paParser);
    baseOptions.AddOptions(&paParser);

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

    panda::Logger::Initialize(baseOptions);
    panda::Logger::SetLevel(panda::Logger::Level::INFO);
    panda::Logger::ResetComponentMask();  // disable all Component
    panda::Logger::EnableComponent(panda::Logger::Component::ECMASCRIPT);  // enable ECMASCRIPT

    panda::ecmascript::EcmaVM *vm = panda::JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        COMPILER_LOG(INFO) << "Can't Create EcmaVM";
        return -1;
    }
    std::string tripleString = runtimeOptions.GetTargetTriple();
    std::string stubFile = runtimeOptions.WasStubFileSet() ? runtimeOptions.GetStubFile() : "";
    std::string logMethods = vm->GetJSOptions().GetlogCompiledMethods();
    size_t optLevel = runtimeOptions.GetOptLevel();
    panda::ecmascript::kungfu::CompilerLog log(logMethods);
    panda::ecmascript::kungfu::StubCompiler compiler(&log);

    bool res = compiler.BuildStubModuleAndSave(tripleString, stubFile, optLevel);
    COMPILER_LOG(INFO) << "stub compiler run finish, result condition(T/F):" << std::boolalpha << res;
    panda::JSNApi::DestroyJSVM(vm);
    return res ? 0 : -1;
}
