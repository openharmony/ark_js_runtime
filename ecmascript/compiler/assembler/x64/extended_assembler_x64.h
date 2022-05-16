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
#ifndef ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H
#define ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H

#include "assembler_x64.h"
#include "ecmascript/compiler/assembler_module.h"

namespace panda::ecmascript::x64 {
// ExtendedAssembler implements frequently-used assembler macros with some extended usages.
class ExtendedAssembler : public AssemblerX64 {
public:
    explicit ExtendedAssembler(Chunk *chunk, kungfu::AssemblerModule *module)
        : AssemblerX64(chunk), module_(module)
    {
    }
    void CallAssemblerStub(int id, bool isTail);
    void BindAssemblerStub(int id);
    void PushAlignBytes();
    void PopAlignBytes();
    void PushCppCalleeSaveRegisters();
    void PopCppCalleeSaveRegisters();
    void PushGhcCalleeSaveRegisters();
    void PopGhcCalleeSaveRegisters();
    void PushArgsWithArgv(Register argc, Register argv, Register operatorRegister);
private:
    kungfu::AssemblerModule *module_;
};
}  // panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H