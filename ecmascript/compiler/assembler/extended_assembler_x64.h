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

namespace panda::ecmascript::x64 {
// ExtendedAssembler implements frequently-used assembler macros with some extended usages.
class ExtendedAssemblerX64 : public AssemblerX64 {
public:
    explicit ExtendedAssemblerX64(Chunk *chunk)
        : AssemblerX64(chunk)
    {
    }

    void PushAlignBytes();
    void PushCppCalleeSaveRegisters();
    void PopCppCalleeSaveRegisters();
};
}  // panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H