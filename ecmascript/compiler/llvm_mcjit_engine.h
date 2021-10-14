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

#ifndef ECMASCRIPT_COMPILER_LLVM_MCJINT_ENGINE_H
#define ECMASCRIPT_COMPILER_LLVM_MCJINT_ENGINE_H

#include <iostream>
#include <list>
#include <map>
#include <sys/mman.h>
#include <vector>

#include "ecmascript/ecma_macros.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"

namespace kungfu {
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
    }
    ~CodeInfo()
    {
        Reset();
        munmap(machineCode_, MAX_MACHINE_CODE_SIZE);
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
        std::vector<uint8_t> codeBuffer(machineCode_[codeBufferPos_], size);
        LOG_ECMA(INFO) << " codeBuffer size: " << codeBuffer.size();
        codeSectionNames_.push_back(sectionName);
        addr = machineCode_ + codeBufferPos_;
        LOG_ECMA(INFO) << "AllocaCodeSection addr:" << std::hex << reinterpret_cast<std::uintptr_t>(addr);
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
            LOG_ECMA(INFO) << "llvm_stackmaps : " << addr;
            stackMapsSection_ = addr;
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
};
class LLVMAssembler {
public:
    explicit LLVMAssembler(LLVMModuleRef module, const char* triple);
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
    const char *AMD64_TRIPLE = "x86_64-unknown-linux-gnu";
    const char *ARM64_TRIPLE = "aarch64-unknown-linux-gnu";
    const char *ARM32_TRIPLE = "arm-unknown-linux-gnu";
private:
    void UseRoundTripSectionMemoryManager();
    bool BuildMCJITEngine();
    void BuildAndRunPasses() const;
    void BuildSimpleFunction();
    void Initialize();
    void InitMember();

    LLVMMCJITCompilerOptions options_;
    LLVMModuleRef module_;
    LLVMExecutionEngineRef engine_;
    std::string hostTriple_;
    char *error_;
    struct CodeInfo codeInfo_;
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_MCJINT_ENGINE_H
