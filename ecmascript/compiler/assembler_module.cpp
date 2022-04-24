/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "assembler/assembler_x64.h"
#include "call_signature.h"
#include "rt_call_signature.h"
#include "trampoline/x64/assembler_module_x64.h"
#include "assembler_module.h"

namespace panda::ecmascript::kungfu {
void AssemblerModule::Run(const std::string &triple, Chunk* chunk)
{
    SetUpForAsmStubs();
    if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
        GenerateStubsX64(chunk);
    }
}

void AssemblerModule::GenerateStubsX64(Chunk* chunk)
{
    x64::AssemblerX64 assembler(chunk);
    COMPILER_LOG(INFO) << "compiling asm stubs";
    for (size_t i = 0; i < asmCallSigns_.size(); i++) {
        auto cs = asmCallSigns_[i];
        ASSERT(cs->HasConstructor());
        COMPILER_LOG(INFO) << "Stub Name: " << cs->GetName();
        size_t offset = assembler.GetCurrentPosition();
        offsetTable_.push_back(offset);
        AssemblerStub *stub = static_cast<AssemblerStub*>(
            cs->GetConstructor()(nullptr));
        stub->Generate(&assembler);
        delete stub;
    }
    buffer_ = assembler.GetBegin();
    bufferSize_ = assembler.GetCurrentPosition();
}

void AssemblerModule::SetUpForAsmStubs()
{
    std::vector<CallSignature *> callSigns;
    RuntimeStubCSigns::GetCSigns(callSigns);

#define INIT_SIGNATURES(name)                                    \
    callSigns[RuntimeStubCSigns::ID_##name]->SetConstructor(     \
        []([[maybe_unused]]void* arg) {                          \
            return static_cast<void*>(                           \
                new name##Stub());                               \
    });
    RUNTIME_ASM_STUB_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES

    for (size_t i = 0; i < callSigns.size(); i++) {
        CallSignature* cs = callSigns[i];
        ASSERT(!cs->GetName().empty());
        if (cs->IsAsmStub()) {
            asmCallSigns_.push_back(cs);
        }
    }
    callSigns.clear();
}

void CallRuntimeStub::Generate(Assembler *assembler)
{
    x64::AssemblerX64 *assemblerX64 = static_cast<x64::AssemblerX64*>(assembler);
    x64::AssemblerModuleX64::Generate_CallRuntime(assemblerX64);
    assemblerX64->Align16();
}
}  // namespace panda::ecmascript::kunfu
