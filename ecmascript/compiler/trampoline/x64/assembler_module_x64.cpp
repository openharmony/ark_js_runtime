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
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/frames.h"
#include "assembler_module_x64.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

void AssemblerModuleX64::Generate_AsmInterpCallRuntime(AssemblerX64 *assembler)
{
    __ Pushq(0);
    __ Movq(rsp, Operand(rax, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Pushq(static_cast<int32_t>(FrameType::ASM_LEAVE_FRAME));

    __ Pushq(r10);
    __ Pushq(rdx);
    __ Pushq(rax);

    __ Movq(rsp, rdx);
    // 48: rbp & return address
    __ Addq(48, rdx);
    __ Movq(Operand(rdx, 0), r10);
    __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
    __ Movq(rax, rdi);
    // 8: argc
    __ Movq(Operand(rdx, 8), rsi);
    // 16: argv
    __ Addq(16, rdx);
    __ Callq(r10);

    __ Popq(r10);
    __ Movq(0, Operand(r10, JSThread::GlueData::GetLeaveFrameOffset(false)));
    __ Popq(rdx);
    __ Popq(r10);
    // 16: skip rbp & frame type
    __ Addq(16, rsp);
    __ Ret();
}

void AssemblerModuleX64::Generate_OptimizedCallRuntime(AssemblerX64 *assembler)
{
    __ Pushq(rbp);
    __ Movq(rsp, rbp);
    __ Movq(rsp, Operand(rax, JSThread::GlueData::GetCurrentFrameOffset(false)));
    __ Pushq(static_cast<int32_t>(FrameType::ASM_LEAVE_FRAME));

    __ Pushq(r10);
    __ Pushq(rdx);
    __ Pushq(rax);

    __ Movq(rbp, rdx);
    // 16: rbp & return address
    __ Addq(16, rdx);

    __ Movq(Operand(rdx, 0), r10);
    __ Movq(Operand(rax, r10, Times8, JSThread::GlueData::GetRTStubEntriesOffset(false)), r10);
    __ Movq(rax, rdi);
    // 8: argc
    __ Movq(Operand(rdx, 8), rsi);
    // 16: argv
    __ Addq(16, rdx);
    __ Callq(r10);


    // 8: skip rax
    __ Addq(8, rsp);
    __ Popq(rdx);
    __ Popq(r10);

    // 8: skip frame type
    __ Addq(8, rsp);
    __ Popq(rbp);
    __ Ret();
}

#undef __
}  // namespace panda::ecmascript::x64
