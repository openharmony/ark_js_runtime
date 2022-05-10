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
#ifndef ECMASCRIPT_COMPILER_AARCH64_EXTEND_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_AARCH64_EXTEND_ASSEMBLER_H

#include "assembler_aarch64.h"
#include "ecmascript/compiler/assembler_moduler.h"

namespace panda::ecmascript::aarch64 {
using namespace panda::ecmascript::kungfu;
// ExtendAssembler implements frequently-used assembler macros.
class ExtendAssembler : public AssemblerArach64 {
public:
    explicit ExtendAssembler(Chunk *chunk, AssemblerModule *module)
        : AssemblerArach64(chunk), module_(module);
    {
    }
    void BindAssemblerStub(int id);
    void CalleeSave();
    void CalleeRestore();
    void CallAssemblerStub(int id);
    void SaveFpAndLr();
    void RestoreFpAndLr();
private:
    AssemblerModule *module_ {nullptr};
};
}  // panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H