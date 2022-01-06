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

#ifndef ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
#define ECMASCRIPT_COMPILER_LLVM_CODEGEN_H

#include <iostream>
#include <list>
#include <map>
#include <sys/mman.h>
#include <vector>

#include "code_generator.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/stub_module.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm-c/Types.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Host.h"

namespace panda::ecmascript::kungfu {
struct CodeInfo {
    using ByteBuffer = std::vector<uint8_t>;
    using BufferList = std::list<ByteBuffer>;
    using StringList = std::list<std::string>;
    CodeInfo() : machineCode_(nullptr), codeBufferPos_(0), stackMapsSection_(nullptr)
    {
        Reset();
        static constexpr int prot = PROT_READ | PROT_WRITE | PROT_EXEC;  // NOLINT(hicpp-signed-bitwise)
        static constexpr int flags = MAP_ANONYMOUS | MAP_SHARED;         // NOLINT(hicpp-signed-bitwise)
        machineCode_ = static_cast<uint8_t *>(mmap(nullptr, MAX_MACHINE_CODE_SIZE, prot, flags, -1, 0));
        if (machineCode_ == reinterpret_cast<uint8_t *>(-1)) {
            machineCode_ = nullptr;
        }
        if (machineCode_ != nullptr) {
            ASAN_UNPOISON_MEMORY_REGION(machineCode_, MAX_MACHINE_CODE_SIZE);
        }
    }
    ~CodeInfo()
    {
        Reset();
        if (machineCode_ != nullptr) {
            ASAN_POISON_MEMORY_REGION(machineCode_, MAX_MACHINE_CODE_SIZE);
            munmap(machineCode_, MAX_MACHINE_CODE_SIZE);
        }
        machineCode_ = nullptr;
    }
    uint8_t *AllocaCodeSection(uintptr_t size, const char *sectionName)
    {
        uint8_t *addr = nullptr;
        if (codeBufferPos_ + size > MAX_MACHINE_CODE_SIZE) {
            LOG_ECMA(INFO) << std::hex << "AllocaCodeSection failed alloc codeBufferPos_:" << codeBufferPos_
                      << " size:" << size << "  larger MAX_MACHINE_CODE_SIZE:" << MAX_MACHINE_CODE_SIZE;
            return nullptr;
        }
        LOG_ECMA(INFO) << "AllocaCodeSection size:" << size;
        codeSectionNames_.push_back(sectionName);
        addr = machineCode_ + codeBufferPos_;
        codeInfo_.push_back({addr, size});
        codeBufferPos_ += size;
        return addr;
    }

    uint8_t *AllocaDataSection(uintptr_t size, const char *sectionName)
    {
        uint8_t *addr = nullptr;
        dataSectionList_.push_back(std::vector<uint8_t>());
        dataSectionList_.back().resize(size);
        dataSectionNames_.push_back(sectionName);
        addr = static_cast<uint8_t *>(dataSectionList_.back().data());
        if (!strcmp(sectionName, ".llvm_stackmaps")) {
            LOG_ECMA(INFO) << "llvm_stackmaps : " << addr << " size:" << size;
            stackMapsSection_ = addr;
            stackMapsSize_ = size;
        }
        return addr;
    }

    void Reset()
    {
        stackMapsSection_ = nullptr;
        codeInfo_.clear();
        dataSectionList_.clear();
        dataSectionNames_.clear();
        codeSectionNames_.clear();
        codeBufferPos_ = 0;
    }

    uint8_t *GetStackMapsSection() const
    {
        return stackMapsSection_;
    }
    int GetStackMapsSize() const
    {
        return stackMapsSize_;
    }
    std::vector<std::pair<uint8_t *, uintptr_t>> GetCodeInfo() const
    {
        return codeInfo_;
    }

    int GetCodeSize() const
    {
        return codeBufferPos_;
    }

    uint8_t *GetCodeBuff() const
    {
        return machineCode_;
    }

private:
    BufferList dataSectionList_ {};
    StringList dataSectionNames_ {};
    StringList codeSectionNames_ {};
    uint8_t *machineCode_;
    const size_t MAX_MACHINE_CODE_SIZE = (1 << 20);  // 1M
    int codeBufferPos_ = 0;
    /* <addr, size > for asssembler */
    std::vector<std::pair<uint8_t *, uintptr_t>> codeInfo_ {};
    /* stack map */
    uint8_t *stackMapsSection_ {nullptr};
    int stackMapsSize_ = 0;
};

class LLVMAssembler {
public:
    explicit LLVMAssembler(LLVMModuleRef module);
    virtual ~LLVMAssembler();
    void Run();
    const LLVMExecutionEngineRef &GetEngine()
    {
        return engine_;
    }
    void Disassemble(std::map<uint64_t, std::string> addr2name = std::map<uint64_t, std::string>()) const;
    uint8_t *GetStackMapsSection() const
    {
        return codeInfo_.GetStackMapsSection();
    }
    int GetStackMapsSize() const
    {
        return codeInfo_.GetStackMapsSize();
    }

    int GetCodeSize() const
    {
        return codeInfo_.GetCodeSize();
    }
    uint8_t *GetCodeBuffer() const
    {
        return codeInfo_.GetCodeBuff();
    }

    void *GetFuncPtrFromCompiledModule(LLVMValueRef function)
    {
        return LLVMGetPointerToGlobal(engine_, function);
    }
private:
    void UseRoundTripSectionMemoryManager();
    bool BuildMCJITEngine();
    void BuildAndRunPasses();
    void BuildSimpleFunction();
    void FillPatchPointIDs();
    void RewritePatchPointIdOfStatePoint(LLVMValueRef instruction, uint64_t &callInsNum, uint64_t &funcNum);
    void RewritePatchPointIdStoredOnThread(LLVMValueRef instruction, uint64_t id);
    void Initialize();
    void InitMember();

    LLVMMCJITCompilerOptions options_ {};
    LLVMModuleRef module_;
    LLVMExecutionEngineRef engine_ {nullptr};
    const CompilationConfig *compCfg_;
    char *error_ {nullptr};
    struct CodeInfo codeInfo_ {};
};

class LLVMCodeGeneratorImpl : public CodeGeneratorImpl {
public:
    explicit LLVMCodeGeneratorImpl(LLVMStubModule *module) : module_(module) {}
    ~LLVMCodeGeneratorImpl() = default;
    void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, int index,
                             const CompilationConfig *cfg) override;

private:
    LLVMStubModule *module_;
};

class LLVMModuleAssembler {
public:
    explicit LLVMModuleAssembler(LLVMStubModule *module)
        : stubmodule_(module), assembler_(module->GetModule()) {}
    void AssembleModule();
    void AssembleStubModule(panda::ecmascript::StubModule *module);
    int GetCodeSize() const
    {
        return assembler_.GetCodeSize();
    }
    int GetStackMapsSize() const
    {
        return assembler_.GetStackMapsSize();
    }
    void CopyAssemblerToCode(panda::ecmascript::MachineCode *code)
    {
        code->SetData(reinterpret_cast<uint8_t *>(assembler_.GetCodeBuffer()), assembler_.GetCodeSize());
    }
private:
    LLVMStubModule *stubmodule_;
    LLVMAssembler assembler_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
