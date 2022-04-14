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
        x64::AssemblerX64 assembler(chunk);
        x64::AssemblerModuleX64 stubs;

        if (asmCallSigns_.empty()) {
            return;
        }
        auto cs = asmCallSigns_[0];
        std::cerr << "Stub Name: " << cs->GetName() << std::endl;
        offsetTable_.push_back(0U);
        stubs.Generate_AsmInterpCallRuntime(&assembler);
        assembler.Align16();

        buffer_ = assembler.GetBegin();
        bufferSize_ = assembler.GetCurrentPosition();
    }
}

void AssemblerModule::SetUpForAsmStubs()
{
    std::vector<CallSignature *> callSigns;
    RuntimeStubCSigns::GetCSigns(callSigns);

    for (size_t i = 0; i < callSigns.size(); i++) {
        CallSignature* cs = callSigns[i];
        ASSERT(!cs->GetName().empty());
        if (cs->IsAsmStub()) {
            asmCallSigns_.push_back(cs);
        }
    }
}
}  // namespace panda::ecmascript::kunfu
