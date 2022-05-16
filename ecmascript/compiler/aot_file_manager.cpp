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
#include "aot_file_manager.h"
#include "llvm_ir_builder.h"

namespace panda::ecmascript::kungfu {
void AotFileManager::CollectAOTCodeInfoOfStubs()
{
    auto codeBuff = reinterpret_cast<uintptr_t>(assembler_.GetCodeBuffer());
    auto engine = assembler_.GetEngine();
    std::map<uintptr_t, std::string> addr2name;
    auto callSigns = llvmModule_->GetCSigns();
    for (size_t i = 0; i < llvmModule_->GetFuncCount(); i++) {
        auto cs = callSigns[i];
        LLVMValueRef func = llvmModule_->GetFunction(i);
        ASSERT(func != nullptr);
        uintptr_t entry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
        aotInfo_.AddStubEntry(cs->GetTargetKind(), cs->GetID(), entry - codeBuff);
        ASSERT(!cs->GetName().empty());
        addr2name[entry] = cs->GetName();
    }

    uintptr_t codeBegin = asmModule_.GetCodeBufferOffset();
    auto asmCallSigns = asmModule_.GetCSigns();
    for (size_t i = 0; i < asmModule_.GetFunctionCount(); i++) {
        auto cs = asmCallSigns[i];
        auto entryOffset = asmModule_.GetFunction(cs->GetID());
        aotInfo_.AddStubEntry(cs->GetTargetKind(), cs->GetID(), entryOffset + codeBegin);
        ASSERT(!cs->GetName().empty());
        uintptr_t entry = codeBuff + entryOffset + codeBegin;
        addr2name[entry] = cs->GetName();
    }

    aotInfo_.SetHostCodeSectionAddr(codeBuff);
    // stackmaps ptr and size
    aotInfo_.SetStackMapAddr(reinterpret_cast<uintptr_t>(assembler_.GetStackMapsSection()));
    aotInfo_.SetStackMapSize(assembler_.GetStackMapsSize());
    aotInfo_.SetCodeSize(assembler_.GetCodeSize());
    aotInfo_.SetCodePtr(reinterpret_cast<uintptr_t>(assembler_.GetCodeBuffer()));

    const CompilerLog *log = GetLog();
    assembler_.Disassemble(addr2name, *log);
}

void AotFileManager::CollectAOTCodeInfo()
{
    auto codeBuff = reinterpret_cast<uint64_t>(assembler_.GetCodeBuffer());
    auto engine = assembler_.GetEngine();
    std::map<uintptr_t, std::string> addr2name;
    llvmModule_->IteratefuncIndexMap([&](size_t idx, LLVMValueRef func) {
        uint64_t funcEntry = reinterpret_cast<uint64_t>(LLVMGetPointerToGlobal(engine, func));
        uint64_t length = 0;
        std::string funcName(LLVMGetValueName2(func, &length));
        if (length == 0) {
            return;
        }
        COMPILER_LOG(INFO) << "CollectAOTCodeInfo " << funcName.c_str();
        aotInfo_.SetAOTFuncEntry(funcName, funcEntry - codeBuff, idx);
        addr2name[funcEntry] = funcName;
    });
    aotInfo_.SetHostCodeSectionAddr(codeBuff);
    // stackmaps ptr and size
    aotInfo_.SetStackMapAddr(reinterpret_cast<uintptr_t>(assembler_.GetStackMapsSection()));
    aotInfo_.SetStackMapSize(assembler_.GetStackMapsSize());
    aotInfo_.SetCodeSize(assembler_.GetCodeSize());
    aotInfo_.SetCodePtr(reinterpret_cast<uintptr_t>(assembler_.GetCodeBuffer()));

#ifndef NDEBUG
    const CompilerLog *log = GetLog();
    assembler_.Disassemble(addr2name, *log);
#endif
}

void AotFileManager::RunAsmAssembler()
{
    std::string triple(LLVMGetTarget(llvmModule_->GetModule()));
    NativeAreaAllocator allocator;
    Chunk chunk(&allocator);
    asmModule_.Run(triple, &chunk);

    auto buffer = asmModule_.GetBuffer();
    auto bufferSize = asmModule_.GetBufferSize();
    if (bufferSize == 0U) {
        return;
    }
    auto currentOffset = assembler_.GetCodeSize();
    auto codeBuffer = assembler_.AllocaCodeSection(bufferSize, "asm code");
    if (codeBuffer == nullptr) {
        LOG_ECMA(FATAL) << "AllocaCodeSection failed";
        return;
    }
    if (memcpy_s(codeBuffer, bufferSize, buffer, bufferSize) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        return;
    }
    asmModule_.SetCodeBufferOffset(currentOffset);
}

void AotFileManager::SaveStubFile(const std::string &filename)
{
    RunLLVMAssembler();
    RunAsmAssembler();
    CollectAOTCodeInfoOfStubs();
    aotInfo_.SerializeForStub(filename);
}

void AotFileManager::SaveAOTFile(const std::string &filename)
{
    RunLLVMAssembler();
    CollectAOTCodeInfo();
    aotInfo_.Serialize(filename);
}
}  // namespace panda::ecmascript::kungfu
