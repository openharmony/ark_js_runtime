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
#include "compiler_log.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_thread.h"
#include "llvm_ir_builder.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/region.h"
#include "libpandabase/utils/asan_interface.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm-c/Types.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/Host.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

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
        // align machineCode for aarch64
        machineCode_ += MachineCode::DATA_OFFSET +
            AlignUp(sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
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

    uint8_t *Alloca(uintptr_t size, const char *sectionName)
    {
        // align up for rodata section
        size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        uint8_t *addr = nullptr;
        if (codeBufferPos_ + size > MAX_MACHINE_CODE_SIZE) {
            LOG_COMPILER(ERROR) << std::hex << "AllocaCodeSection failed alloc codeBufferPos_:" << codeBufferPos_
                      << " size:" << size << "  larger MAX_MACHINE_CODE_SIZE:" << MAX_MACHINE_CODE_SIZE;
            return nullptr;
        }
        codeSectionNames_.push_back(sectionName);
        addr = machineCode_ + codeBufferPos_;
        codeBufferPos_ += size;
        return addr;
    }

    uint8_t *AllocaCodeSection(uintptr_t size, const char *sectionName)
    {
        uint8_t *addr = Alloca(size, sectionName);
        codeInfo_.push_back({addr, size});
        return addr;
    }

    uint8_t *AllocaDataSection(uintptr_t size, const char *sectionName)
    {
        if (strncmp(sectionName, ".rodata", strlen(".rodata")) == 0) {
            return Alloca(size, sectionName);
        }
        uint8_t *addr = nullptr;
        dataSectionList_.push_back(std::vector<uint8_t>());
        dataSectionList_.back().resize(size);
        dataSectionNames_.push_back(sectionName);
        addr = static_cast<uint8_t *>(dataSectionList_.back().data());
        if (!strcmp(sectionName, ".llvm_stackmaps")) {
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
    /* <addr, size > for disasssembler */
    std::vector<std::pair<uint8_t *, uintptr_t>> codeInfo_ {};
    /* stack map */
    uint8_t *stackMapsSection_ {nullptr};
    size_t stackMapsSize_ {0};
};

struct LOptions {
    uint32_t optLevel : 2; // 2 bit for optimized level 0-4
    uint32_t genFp : 1; // 1 bit for whether to generated frame pointer or not
    LOptions() : optLevel(3), genFp(1) {}; // 3: default optLevel, 1: generating fp
    LOptions(size_t level, bool genFp) : optLevel(level), genFp(genFp) {};
};

class LLVMAssembler {
public:
    explicit LLVMAssembler(LLVMModuleRef module, LOptions option = LOptions());
    virtual ~LLVMAssembler();
    void Run();
    const LLVMExecutionEngineRef &GetEngine()
    {
        return engine_;
    }
    void Disassemble(const std::map<uintptr_t, std::string> &addr2name, const CompilerLog &log) const;
    static int GetFpDeltaPrevFramSp(LLVMValueRef fn, const CompilerLog &log);
    static void Disassemble(uint8_t *buf, size_t size);
    uintptr_t GetStackMapsSection() const
    {
        return reinterpret_cast<uintptr_t>(codeInfo_.GetStackMapsSection());
    }

    uint32_t GetStackMapsSize() const
    {
        return static_cast<uint32_t>(codeInfo_.GetStackMapsSize());
    }

    uintptr_t GetCodeBuffer() const
    {
        return reinterpret_cast<uintptr_t>(codeInfo_.GetCodeBuff());
    }

    uint32_t GetCodeSize() const
    {
        return static_cast<uint32_t>(codeInfo_.GetCodeSize());
    }

    void *GetFuncPtrFromCompiledModule(LLVMValueRef function)
    {
        return LLVMGetPointerToGlobal(engine_, function);
    }

    uint8_t *AllocaCodeSection(uintptr_t size, const char *sectionName)
    {
        return codeInfo_.AllocaCodeSection(size, sectionName);
    }

private:
    void UseRoundTripSectionMemoryManager();
    bool BuildMCJITEngine();
    void BuildAndRunPasses();
    void Initialize(LOptions option);

    LLVMMCJITCompilerOptions options_ {};
    LLVMModuleRef module_;
    LLVMExecutionEngineRef engine_ {nullptr};
    char *error_ {nullptr};
    struct CodeInfo codeInfo_ {};
};

class LLVMIRGeneratorImpl : public CodeGeneratorImpl {
public:
    explicit LLVMIRGeneratorImpl(LLVMModule *module, bool enableLog)
        : module_(module), enableLog_(enableLog) {}
    ~LLVMIRGeneratorImpl() = default;
    void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                             const CompilationConfig *cfg) override;
    void GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
        const JSMethod *method) override;

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

private:
    LLVMModule *module_;
    bool enableLog_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
