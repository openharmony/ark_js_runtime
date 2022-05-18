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
#include "assembler_module.h"
#include "ecmascript/compiler/assembler/x64/assembler_x64.h"
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/trampoline/x64/assembler_stubs_x64.h"
#include "ecmascript/compiler/trampoline/aarch64/assembler_stubs.h"

namespace panda::ecmascript::kungfu {
void AssemblerModule::Run(const std::string &triple, Chunk* chunk)
{
    SetUpForAsmStubs();
    if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
        GenerateStubsX64(chunk);
    } else if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
        GenerateStubsAarch64(chunk);
    } else {
        UNREACHABLE();
    }
}

void AssemblerModule::GenerateStubsX64(Chunk* chunk)
{
    x64::ExtendedAssembler assembler(chunk, this);
    COMPILER_LOG(INFO) << "compiling asm stubs";
    for (size_t i = 0; i < asmCallSigns_.size(); i++) {
        auto cs = asmCallSigns_[i];
        ASSERT(cs->HasConstructor());
        COMPILER_LOG(INFO) << "Stub Name: " << cs->GetName();
        AssemblerStub *stub = static_cast<AssemblerStub*>(
            cs->GetConstructor()(nullptr));
        stub->GenerateX64(&assembler);
        delete stub;
    }
    buffer_ = assembler.GetBegin();
    bufferSize_ = assembler.GetCurrentPosition();
}

void AssemblerModule::GenerateStubsAarch64(Chunk* chunk)
{
    aarch64::ExtendedAssembler assembler(chunk, this);
    COMPILER_LOG(INFO) << "compiling asm stubs";
    for (size_t i = 0; i < asmCallSigns_.size(); i++) {
        auto cs = asmCallSigns_[i];
        ASSERT(cs->HasConstructor());
        COMPILER_LOG(INFO) << "Stub Name: " << cs->GetName();
        AssemblerStub *stub = static_cast<AssemblerStub*>(
            cs->GetConstructor()(nullptr));
        stub->GenerateAarch64(&assembler);
        delete stub;
    }
    buffer_ = assembler.GetBegin();
    bufferSize_ = assembler.GetCurrentPosition();
}

void AssemblerModule::SetUpForAsmStubs()
{
    RuntimeStubCSigns::GetASMCSigns(asmCallSigns_);
    for (auto cs : asmCallSigns_) {
        symbolTable_[cs->GetID()] = new Label();
    }
}

#define DECLARE_ASM_STUB_X64_GENERATE(name)                                                       \
void name##Stub::GenerateX64(Assembler *assembler)                                                \
{                                                                                                 \
    x64::ExtendedAssembler *assemblerX64 = static_cast<x64::ExtendedAssembler*>(assembler);       \
    x64::AssemblerStubsX64::name(assemblerX64);                                                   \
    assemblerX64->Align16();                                                                      \
}

#define DECLARE_ASM_STUB_AARCH64_GENERATE(name)                                                   \
void name##Stub::GenerateAarch64(Assembler *assembler)                                            \
{                                                                                                 \
    aarch64::ExtendedAssembler *assemblerAarch64 = static_cast<aarch64::ExtendedAssembler*>(assembler); \
    aarch64::AssemblerStubs::name(assemblerAarch64);                                                    \
}
RUNTIME_ASM_STUB_LIST(DECLARE_ASM_STUB_X64_GENERATE)
RUNTIME_ASM_STUB_LIST(DECLARE_ASM_STUB_AARCH64_GENERATE)
#undef DECLARE_ASM_STUB_GENERATE
}  // namespace panda::ecmascript::kunfu
