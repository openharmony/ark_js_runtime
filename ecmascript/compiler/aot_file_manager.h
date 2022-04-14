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
#ifndef ECMASCRIPT_KUNGFU_AOT_FILE_MANAGER_H
#define ECMASCRIPT_KUNGFU_AOT_FILE_MANAGER_H

#include "ecmascript/mem/machine_code.h"
#include "assembler_module.h"
#include "llvm_ir_builder.h"
#include "llvm_codegen.h"

namespace panda::ecmascript::kungfu {
class AotFileManager {
public:
    AotFileManager(LLVMModule *llvmModule, bool genFp = true) : llvmModule_(llvmModule),
        assembler_(llvmModule->GetModule(), genFp) {};
    ~AotFileManager() = default;
    // save function funcs for aot files containing stubs
    void SaveStubFile(const std::string &filename);

    // save function for aot files containing normal func translated from JS/TS
    void SaveAOTFile(const std::string &filename);

private:
    AotCodeInfo aotInfo_;
    LLVMModule *llvmModule_;
    LLVMAssembler assembler_;
    AssemblerModule asmModule_;

    void RunLLVMAssembler()
    {
        assembler_.Run();
    }
    void RunAsmAssembler();
    // collect aot component info
    void CollectAOTCodeInfoOfStubs();
    void CollectAOTCodeInfo();
};
}  // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_KUNGFU_AOT_FILE_MANAGER_H
