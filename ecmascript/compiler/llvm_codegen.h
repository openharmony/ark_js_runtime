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
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_thread.h"
#include "llvm_ir_builder.h"
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
    CodeInfo()
    {
        machineCode_ = static_cast<uint8_t *>(mmap(nullptr, MAX_MACHINE_CODE_SIZE, protRWX, flags, -1, 0));
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

    size_t GetStackMapsSize() const
    {
        return stackMapsSize_;
    }

    std::vector<std::pair<uint8_t *, uintptr_t>> GetCodeInfo() const
    {
        return codeInfo_;
    }

    uint8_t *GetCodeBuff() const
    {
        return machineCode_;
    }

    size_t GetCodeSize() const
    {
        return codeBufferPos_;
    }

private:
    BufferList dataSectionList_ {};
    StringList dataSectionNames_ {};
    StringList codeSectionNames_ {};
    uint8_t *machineCode_ {nullptr};
    const size_t MAX_MACHINE_CODE_SIZE = (1 << 20);  // 1M
    static constexpr int protRWX = PROT_READ | PROT_WRITE | PROT_EXEC;  // NOLINT(hicpp-signed-bitwise)
    static constexpr int flags = MAP_ANONYMOUS | MAP_SHARED;            // NOLINT(hicpp-signed-bitwise)
    size_t codeBufferPos_ {0};
    /* <addr, size > for asssembler */
    std::vector<std::pair<uint8_t *, uintptr_t>> codeInfo_ {};
    /* stack map */
    uint8_t *stackMapsSection_ {nullptr};
    size_t stackMapsSize_ {0};
};

class LLVMAssembler {
public:
    LLVMAssembler(LLVMModuleRef module, bool isFpElim = false);
    virtual ~LLVMAssembler();
    void Run();
    const LLVMExecutionEngineRef &GetEngine()
    {
        return engine_;
    }
    void Disassemble(const std::map<uint64_t, std::string> &addr2name) const;
    uint8_t *GetStackMapsSection() const
    {
        return codeInfo_.GetStackMapsSection();
    }

    size_t GetStackMapsSize() const
    {
        return codeInfo_.GetStackMapsSize();
    }

    uint8_t *GetCodeBuffer() const
    {
        return codeInfo_.GetCodeBuff();
    }

    size_t GetCodeSize() const
    {
        return codeInfo_.GetCodeSize();
    }

    void *GetFuncPtrFromCompiledModule(LLVMValueRef function)
    {
        return LLVMGetPointerToGlobal(engine_, function);
    }

private:
    void UseRoundTripSectionMemoryManager();
    bool BuildMCJITEngine();
    void BuildAndRunPasses();
    void Initialize(bool isFpElim);

    LLVMMCJITCompilerOptions options_ {};
    LLVMModuleRef module_;
    LLVMExecutionEngineRef engine_ {nullptr};
    char *error_ {nullptr};
    struct CodeInfo codeInfo_ {};
};

class LLVMIRGeneratorImpl : public CodeGeneratorImpl {
public:
    explicit LLVMIRGeneratorImpl(LLVMModule *module) : module_(module) {}
    ~LLVMIRGeneratorImpl() = default;
    void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                             const CompilationConfig *cfg) override;
    void GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
        const JSMethod *method) override;

private:
    LLVMModule *module_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
