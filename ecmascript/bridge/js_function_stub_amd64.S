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

// uint64_t InvokeJSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
// Input:
// %rdi - glue
// %rsi - prevFp
// %rdx - expectedNumArgs
// %ecx - actualNumArgs
// %r8  - argV
// %r9  - codeAddr
.global InvokeJSFunctionEntry
.type InvokeJSFunctionEntry, %function
InvokeJSFunctionEntry: 
    // construct the frame
    pushq   %rbp
    movq    %rsp, %rbp // set frame pointer
    pushq   $1         // optimized entry frame type
    pushq   %rsi       // prev managed fp

    // callee save
    pushq   %rbx
    pushq   %rdi

    // expectedNumArgs > actualNumArgs
    movl     %edx, %ebx        // save expectedNumArgs
    cmpl     %ecx, %edx
    jbe     .LCopyArguments
    movl    %ecx, %eax
    movl    %ebx, %edx

.LCopyExtraAument:             // copy undefined value to stack
    pushq   $0xa

    addq    $-1, %rdx
    cmpq    %rax, %rdx
    ja      .LCopyExtraAument

.LCopyArguments:
    cmpl    %ecx, %ebx
    cmovbe  %ebx, %ecx
    // add  $3, %ecx        // todo :add 3 for calltarget newtarget this
    movl    %ecx, %eax

.LCopyLoop:  
    movq    -8(%r8, %rax, 8), %rcx
    pushq   %rcx

    addq    $-1, %rax
    jne     .LCopyLoop

    movq    %rdi, %rax     // move glue to rax
    call    *%r9            // then call jsFunction
    shll    $3, %ebx        // todo:: fixed for 3 extra arguments
    addq    %rbx, %rsp
    
    popq    %rdi           // callee restore
    popq    %rbx

    popq    %rsi           // pop rsi
    popq    %rbp           // skip frame type
    popq    %rbp

    // movq    %rsi, 2064(%rdi)   // restore thread->leaveFp
    ret

// uint64_t CallRuntimeTrampoline(uintptr_t glue, uint64_t runtime_id, uint64_t patch_id, uint32_t argc, ...);
// webkit_jscc calling convention
// %rax - glue
// rbp      - runtime_id
// rbp + 8  - patch_id
// rbp + 16 - argc
// rbp + 24 - arg0
.global CallRuntimeTrampoline
.type CallRuntimeTrampoline, %function
CallRuntimeTrampoline:
    pushq   %rbp
    movq    %rsp, %rbp     // set frame pointer
    // movq    %rbp, 2064(%rax)   // save to thread->leaveFp
    pushq    $3             // leave frame type
    subq    $24, %rsp       // increase the leave frame size
    
    // callee save
    pushq    %r10
    pushq    %r11
    pushq    %rdx

    // construct leave frame
    movq    %rsp, %r10
    addq    $24, %r10        // 24 : callee register size
    movq    %rbp, %rdx
    addq    $16, %rdx        // 16 : for rbp & return address

    movq    8(%rdx), %r11   // get patch pointer id
    movq    %r11, (%r10)    // save patch pointer id
    movq    %rdx, 8(%r10)   // save callsite sp
    movq    (%rbp), %r11
    movq    %r11, 16(%r10)  // save callsite fp 

    // load runtime trampoline address
    movq    (%rdx), %r10       // runtime id
    movq    2072(%rax, %r10, 8), %r11
    movq    %rax, %rdi       // glue
    movl    16(%rdx), %esi   // argc
    addq    $24, %rdx         // argV
    call    *%r11

    popq    %rdx
    popq    %r11
    popq    %r10

    addq    $32, %rsp
    popq    %rbp
    ret