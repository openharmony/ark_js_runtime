/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #include "../asm_defines.h"

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t sp, uintptr_t pc, JSTaggedType constpool,
//                          JSTaggedType profileInfo, JSTaggedType acc, uint32_t hotnessCounter);
// Input:
// %rdi - glue
// %rsi - sp
// %rdx - pc
// %rcx - constpool
// %r8  - profileInfo
// %r9  - acc
// sp [0] hotnessCounter
.global JSCallEntry
.type JSCallEntry, %function
JSCallEntry:
    pushq   %rbp
    movq    %rsp, %rbp
    pushq   $JS_ENTRY_FRAME_TYPE
    pushq   %r15
    pushq   %r14
    pushq   %r13
    pushq   %r12
    pushq   %rbx
    movq    %rcx, %rbx  // constpool
    movq    %rdi, %r13  // glue
    movzbl  (%rdx), %eax   // opcode
    movl    16(%rbp), %edi // hotnessCounter
    movq    %rsi, %rbp  // sp
    movq    %rdx, %r12  // pc
    movq    %r8, %r14   // profileInfo
    movq    %r9, %rsi   // acc
    callq   *ASM_GLUE_BC_HANDLERS_OFFSET(%r13,%rax,8)
    popq    %rbx
    popq    %r12
    popq    %r13
    popq    %r14
    popq    %r15
    addq    $8, %rsp // skip type
    popq	%rbp
    retq
