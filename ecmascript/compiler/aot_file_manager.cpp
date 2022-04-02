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
#include "compiler_macros.h"
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
    aotInfo_.SetHostCodeSectionAddr(codeBuff);
    // stackmaps ptr and size
    aotInfo_.SetStackMapAddr(reinterpret_cast<uintptr_t>(assembler_.GetStackMapsSection()));
    aotInfo_.SetStackMapSize(assembler_.GetStackMapsSize());
    aotInfo_.SetCodeSize(assembler_.GetCodeSize());
    aotInfo_.SetCodePtr(reinterpret_cast<uintptr_t>(assembler_.GetCodeBuffer()));
#ifndef NDEBUG
    assembler_.Disassemble(addr2name);
#endif
}

void AotFileManager::CollectAOTCodeInfo()
{
    auto codeBuff = reinterpret_cast<uint64_t>(assembler_.GetCodeBuffer());
    auto engine = assembler_.GetEngine();
    for (size_t i = 0; i < llvmModule_->GetFuncCount(); i++) {
        LLVMValueRef func = llvmModule_->GetFunction(i);
        uint64_t funcEntry = reinterpret_cast<uint64_t>(LLVMGetPointerToGlobal(engine, func));
        uint64_t length = 0;
        std::string tmp(LLVMGetValueName2(func, &length));
        if (length == 0) {
            continue;
        }
        std::cout << "CollectAOTCodeInfo " << tmp.c_str() << std::endl;
        aotInfo_.SetAOTFuncOffset(tmp, funcEntry - codeBuff);
    }
    aotInfo_.SetHostCodeSectionAddr(codeBuff);
    // stackmaps ptr and size
    aotInfo_.SetStackMapAddr(reinterpret_cast<uintptr_t>(assembler_.GetStackMapsSection()));
    aotInfo_.SetStackMapSize(assembler_.GetStackMapsSize());
    aotInfo_.SetCodeSize(assembler_.GetCodeSize());
    aotInfo_.SetCodePtr(reinterpret_cast<uintptr_t>(assembler_.GetCodeBuffer()));
}

void AotFileManager::SaveStubFile(const std::string &filename)
{
    RunLLVMAssembler();
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
