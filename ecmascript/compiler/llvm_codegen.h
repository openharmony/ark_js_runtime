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

#include "code_generator.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/llvm_mcjit_engine.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/stub_module.h"
#include "llvm-c/Types.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Host.h"

namespace kungfu {
class LLVMCodeGeneratorImpl : public CodeGeneratorImpl {
public:
    explicit LLVMCodeGeneratorImpl(LLVMStubModule *module) : module_(module) {}
    ~LLVMCodeGeneratorImpl() = default;
    void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, int index) override;

private:
    LLVMStubModule *module_;
};

class LLVMModuleAssembler {
public:
    explicit LLVMModuleAssembler(LLVMStubModule *module, const char* triple)
        : stubmodule_(module), assembler_(module->GetModule(), triple) {}
    void AssembleModule();
    void AssembleStubModule(panda::ecmascript::StubModule *module);
    int GetCodeSize() const
    {
        return assembler_.GetCodeSize();
    }
    void CopyAssemblerToCode(panda::ecmascript::MachineCode *code)
    {
        code->SetData(reinterpret_cast<uint8_t *>(assembler_.GetCodeBuffer()), assembler_.GetCodeSize());
    }
private:
    LLVMStubModule *stubmodule_;
    LLVMAssembler assembler_;
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
