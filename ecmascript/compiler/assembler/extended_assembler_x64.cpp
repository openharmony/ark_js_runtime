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

#include "extended_assembler_x64.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript::x64 {
void ExtendedAssemblerX64::PushAlignBytes()
{
    Pushq(0);
}

// c++ calling convention
void ExtendedAssemblerX64::PushCppCalleeSaveRegisters()
{
    Pushq(r12);
    Pushq(r13);
    Pushq(r14);
    Pushq(r15);
    Pushq(rbx);
}

void ExtendedAssemblerX64::PopCppCalleeSaveRegisters()
{
    Popq(rbx);
    Popq(r15);
    Popq(r14);
    Popq(r13);
    Popq(r12);
}
}  // panda::ecmascript::x64