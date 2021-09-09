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

#include "llvm_codegen.h"
#include "ecmascript/object_factory.h"
#include "stub_descriptor.h"

using namespace panda::ecmascript;
namespace kungfu {
void LLVMCodeGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, int index)
{
    auto function = module_->GetStubFunction(index);

    LLVMIRBuilder builder(&graph, circuit, module_, function);
    builder.Build();
}

void LLVMModuleAssembler::AssembleModule()
{
    compiler_.Run();
}

void LLVMModuleAssembler::CopyAssembleCodeToModule(StubModule *module)
{
    auto codeBuff = reinterpret_cast<Address>(compiler_.GetCodeBuffer());
    auto engine = compiler_.GetEngine();
    for (int i = 0; i < FAST_STUB_MAXCOUNT; i++) {
        auto stubfunction = stubmodule_->GetStubFunction(i);
        if (stubfunction != nullptr) {
            Address stubEntry = reinterpret_cast<Address>(LLVMGetPointerToGlobal(engine, stubfunction));
            module->SetStubEntry(i, stubEntry - codeBuff);
        }
    }

    auto codeSize = compiler_.GetCodeSize();

    MachineCode *code = reinterpret_cast<MachineCode *>(new char(sizeof(MachineCode) + codeSize));
    code->SetInstructionSizeInBytes(nullptr, JSTaggedValue(codeSize), SKIP_BARRIER);
    code->SetData(reinterpret_cast<uint8_t *>(codeBuff), codeSize);
    module->SetCode(code);
}
}  // namespace kungfu
