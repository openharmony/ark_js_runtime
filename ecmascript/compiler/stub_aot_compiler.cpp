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
#include <iostream>
#include <unistd.h>

#include "fast_stub.h"
#include "generated/stub_aot_options_gen.h"
#include "js_thread_offset_table.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"
#include "llvm_codegen.h"
#include "scheduler.h"
#include "stub-inl.h"
#include "triple.h"
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
    template<typename T, typename... Args>
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
    void CreateCodeGen(const char *targetTriple, LLVMStubModule *module)
    {
        if (targetTriple == TripleConst::GetLLVMArm64Triple()) {
            llvmImpl_ = std::make_unique<LLVMAarch64CodeGeneratorImpl>(module);
        } else if (targetTriple == TripleConst::GetLLVMArm32Triple()) {
            llvmImpl_ = std::make_unique<LLVMArm32CodeGeneratorImpl>(module);
        } else {
            llvmImpl_ = std::make_unique<LLVMCodeGeneratorImpl>(module);
        }
    }
    bool Run(PassPayLoad *data, int index, const char* triple)
    {
        auto stubModule = data->GetStubModule();
        CreateCodeGen(stubModule->GetTargetTriple(), stubModule);
        CodeGenerator codegen(llvmImpl_, triple);
        codegen.Run(data->GetCircuit(), data->GetScheduleResult(), index);
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_{nullptr};
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
            pipeline.RunPass<LLVMCodegenPass>(i, triple);
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

    delete[] code;
}
}  // namespace kungfu

#define SET_STUB_TO_MODULE(module, name, triple) \
    kungfu::Circuit name##Circuit; \
    kungfu::name##Stub name##Stub(& name##Circuit, triple); \
    module.SetStub(FAST_STUB_ID(name), &name##Stub);
#ifdef ECMASCRIPT_ENABLE_SPECIFIC_STUBS
#define SET_ALL_STUB_TO_MODEULE(module, triple)                     \
    SET_STUB_TO_MODULE(module, FastAdd, triple)                     \
    SET_STUB_TO_MODULE(module, FastSub, triple)                     \
    SET_STUB_TO_MODULE(module, FastMul, triple)                     \
    SET_STUB_TO_MODULE(module, FastDiv, triple)                     \
    SET_STUB_TO_MODULE(module, FastMod, triple)                     \
    SET_STUB_TO_MODULE(module, FastTypeOf, triple)                  \
    SET_STUB_TO_MODULE(module, FastEqual, triple)                   \
    SET_STUB_TO_MODULE(module, FindOwnElement2, triple)             \
    SET_STUB_TO_MODULE(module, GetPropertyByIndex, triple)          \
    SET_STUB_TO_MODULE(module, SetPropertyByIndex, triple)          \
    SET_STUB_TO_MODULE(module, GetPropertyByName, triple)           \
    SET_STUB_TO_MODULE(module, GetPropertyByValue, triple)          \
    SET_STUB_TO_MODULE(module, SetPropertyByName, triple)           \
    SET_STUB_TO_MODULE(module, SetPropertyByNameWithOwn, triple)    \
    SET_STUB_TO_MODULE(module, TryLoadICByName, triple)             \
    SET_STUB_TO_MODULE(module, TryLoadICByValue, triple)            \
    SET_STUB_TO_MODULE(module, TryStoreICByName, triple)            \
    SET_STUB_TO_MODULE(module, TryStoreICByValue, triple)
#else
#define SET_ALL_STUB_TO_MODEULE(module, triple)                     \
    SET_STUB_TO_MODULE(module, FastAdd, triple)                     \
    SET_STUB_TO_MODULE(module, FastSub, triple)                     \
    SET_STUB_TO_MODULE(module, FastMul, triple)                     \
    SET_STUB_TO_MODULE(module, FastDiv, triple)                     \
    SET_STUB_TO_MODULE(module, FastMod, triple)                     \
    SET_STUB_TO_MODULE(module, FastTypeOf, triple)                  \
    SET_STUB_TO_MODULE(module, FastEqual, triple)
#endif

#ifndef NDEBUG
#define SET_TEST_STUB_TO_MODEULE(module, hostTriple)                \
    SET_STUB_TO_MODULE(module, FastMulGCTest, hostTriple)
#endif

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

    std::string tripleString = stubOptions.GetTargetTriple();
    const char *triple = kungfu::TripleConst::StringTripleToConst(tripleString);
    std::string moduleFilename = stubOptions.GetStubOutputFile();

    kungfu::OffsetTable::CreateInstance(triple);
    kungfu::StubAotCompiler mouldeBuilder;
    SET_ALL_STUB_TO_MODEULE(mouldeBuilder, triple);
#ifndef NDEBUG
    SET_TEST_STUB_TO_MODEULE(mouldeBuilder, triple);
#endif

    panda::ecmascript::StubModule stubModule;
    mouldeBuilder.BuildStubModuleAndSave(triple, &stubModule, moduleFilename);
    kungfu::OffsetTable::Destroy();

    std::cout << "BuildStubModuleAndSave success" << std::endl;
    return 0;
}