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
#ifndef ECMASCRIPT_COMPILER_FILE_GENERATORS_H
#define ECMASCRIPT_COMPILER_FILE_GENERATORS_H

#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/file_loader.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::kungfu {
class Module {
public:
    Module() = default;
    Module(LLVMModule *module, LLVMAssembler *assembler)
        : llvmModule_(module), assembler_(assembler)
    {
    }

    void CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, StubModulePackInfo &stubInfo,
        uint32_t moduleIndex)
    {
        auto codeBuff = assembler_->GetCodeBuffer();
        auto engine = assembler_->GetEngine();
        auto callSigns = llvmModule_->GetCSigns();
        for (size_t j = 0; j < llvmModule_->GetFuncCount(); j++) {
            auto cs = callSigns[j];
            LLVMValueRef func = llvmModule_->GetFunction(j);
            ASSERT(func != nullptr);
            uintptr_t entry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
            stubInfo.AddStubEntry(cs->GetTargetKind(), cs->GetID(), entry - codeBuff, moduleIndex);
            ASSERT(!cs->GetName().empty());
            addr2name[entry] = cs->GetName();
        }
    }
    void CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, AOTModulePackInfo &aotInfo,
        uint32_t moduleIndex)
    {
        auto codeBuff = assembler_->GetCodeBuffer();
        auto engine = assembler_->GetEngine();
        llvmModule_->IteratefuncIndexMap([&](size_t idx, LLVMValueRef func) {
            uint64_t funcEntry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
            uint64_t length = 0;
            std::string funcName(LLVMGetValueName2(func, &length));
            assert(length != 0);
            COMPILER_LOG(INFO) << "CollectCodeInfo for AOT func: " << funcName.c_str();
            aotInfo.AddStubEntry(CallSignature::TargetKind::JSFUNCTION, idx, funcEntry - codeBuff, moduleIndex);
            addr2name[funcEntry] = funcName;
        });
    }

    ModuleSectionDes GetModuleSectionDes()
    {
        ASSERT(assembler_ != nullptr);
        ModuleSectionDes moduleDes(
            assembler_->GetCodeBuffer(),
            assembler_->GetStackMapsSection(),
            assembler_->GetCodeSize(),
            assembler_->GetStackMapsSize()
        );
        return moduleDes;
    }

    const CompilationConfig *GetCompilationConfig()
    {
        return llvmModule_->GetCompilationConfig();
    }

    uint32_t GetCodeSize()
    {
        return assembler_->GetCodeSize();
    }

    uintptr_t GetCodeBuffer()
    {
        return assembler_->GetCodeBuffer();
    }

    uint8_t *AllocaCodeSection(uintptr_t size, const char *sectionName)
    {
        return assembler_->AllocaCodeSection(size, sectionName);
    }

    void RunAssembler()
    {
        assembler_->Run();
    }

    void DisassemblerFunc(std::map<uintptr_t, std::string> &addr2name, const CompilerLog &log)
    {
        assembler_->Disassemble(addr2name, log);
    }

    void DestoryModule()
    {
        if (llvmModule_ != nullptr) {
            delete llvmModule_;
            llvmModule_ = nullptr;
        }
        if (assembler_ != nullptr) {
            delete assembler_;
            assembler_ = nullptr;
        }
    }
private:
    LLVMModule *llvmModule_ {nullptr};
    LLVMAssembler *assembler_ {nullptr};
};

class FileGenerator {
public:
    explicit FileGenerator(const CompilerLog *log) : log_(log) {};
    virtual ~FileGenerator() = default;

    const CompilerLog *GetLog() const
    {
        return log_;
    }
protected:
    std::vector<Module> modulePackage_ {};
    const CompilerLog *log_ {nullptr};

    void RunLLVMAssembler()
    {
        for (auto m : modulePackage_) {
            m.RunAssembler();
        }
    }

    void DisassembleEachFunc(std::map<uintptr_t, std::string> &addr2name)
    {
        for (auto m : modulePackage_) {
            m.DisassemblerFunc(addr2name, *(log_));
        }
    }

    void DestoryModule()
    {
        for (auto m : modulePackage_) {
            m.DestoryModule();
        }
    }
};

class AOTFileGenerator : public FileGenerator {
public:
    explicit AOTFileGenerator(const CompilerLog *log, EcmaVM* vm) : FileGenerator(log), vm_(vm) {};
    ~AOTFileGenerator() override = default;

    void AddModule(LLVMModule *llvmModule, LLVMAssembler *assembler, const JSPandaFile *jsPandaFile)
    {
        modulePackage_.emplace_back(Module(llvmModule, assembler));
        auto hash = jsPandaFile->GetPandaFile()->GetFilenameHash();
        aotfileHashs_.emplace_back(hash);
    }

    // save function for aot files containing normal func translated from JS/TS
    void SaveAOTFile(const std::string &filename);
    void GenerateSnapshotFile();
private:
    AOTModulePackInfo aotInfo_;
    std::vector<uint32_t> aotfileHashs_ {};
    EcmaVM* vm_;

    // collect aot component info
    void CollectCodeInfo();
};

class StubFileGenerator : public FileGenerator {
public:
    explicit StubFileGenerator(const CompilerLog *log) : FileGenerator(log) {};
    ~StubFileGenerator() override = default;
    void AddModule(LLVMModule *llvmModule, LLVMAssembler *assembler)
    {
        modulePackage_.emplace_back(Module(llvmModule, assembler));
    }
    // save function funcs for aot files containing stubs
    void SaveStubFile(const std::string &filename);
private:
    StubModulePackInfo stubInfo_;
    AssemblerModule asmModule_;

    void RunAsmAssembler();
    void CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name);
    void CollectCodeInfo();
};
}  // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_FILE_GENERATORS_H
