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

 #include "ecma_asm_defines.h"

 /* JSFunctionEntry Callee Register Save Macro */
 .macro PUSH_GENERAL_REGS
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq %rbx
    /* caller save */
    pushq %rdi
.endm

.macro POP_GENERAL_REGS
    /* caller restore */
    popq %rdi
    /* callee restore*/
    popq %rbx
    popq %r15
    popq %r14
    popq %r13
    popq %r12
.endm

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
// Input:
// %rdi - glue
// %rsi - prevFp
// %rdx - expectedNumArgs
// %ecx - actualNumArgs
// %r8  - argV
// %r9  - codeAddr
.global JSFunctionEntry
.type JSFunctionEntry, %function
JSFunctionEntry: 
    PUSH_GENERAL_REGS
    // construct the frame
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    pushq   $JS_ENTRY_FRAME_TYPE    // optimized entry frame type
    pushq   %rsi                    // prev managed fp

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
    
    popq    %rsi           // pop prev-managed frame
    addq    $8, %rsp       
    popq    %rbp

    POP_GENERAL_REGS
    movq    %rsi, ASM_GLUE_CURRENT_FRAME_OFFSET(%rdi)   // restore thread->currentFrame_
    ret



// uint64_t CallRuntimeTrampoline(uintptr_t glue, uint64_t runtime_id, uint64_t patch_id, uint64_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %x0 - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 32: argv1
// sp + 24: argv0
// sp + 16: argc
// sp + 8:  patch_id
// sp:      runtime_id
// construct Leave Frame:
//   +--------------------------+
//   |      argv[]              |
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|   |
//   |       patchID            | Fixed
//   |--------------------------| OptimizedLeaveFrame
//   |       RuntimeId          |   |
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |     frameType            |   v
//   +--------------------------+ ---

// Output:
//  sp - 8 : pc
//  sp - 16: rbp <---------current rbp & current sp
//  current sp - 8:  type

.global CallRuntimeTrampoline
.type CallRuntimeTrampoline, %function
CallRuntimeTrampoline:
    pushq   %rbp
    movq    %rsp, %rbp     // set frame pointer
    movq    %rbp, ASM_GLUE_CURRENT_FRAME_OFFSET(%rax)   // save to thread->currentFrame_
    pushq   $LEAVE_FRAME_TYPE
    
    // callee save
    pushq    %r10
    pushq    %rdx
    pushq    %rax

    // construct leave frame
    movq    %rbp, %rdx
    addq    $16, %rdx        // 16 : for rbp & return address

    // load runtime trampoline address
    movq    (%rdx), %r10       // runtime id
    movq    ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET(%rax, %r10, 8), %r10
    movq    %rax, %rdi       // glue
    movl    16(%rdx), %esi   // argc
    addq    $24, %rdx        // argV
    call    *%r10

    addq    $8, %rsp
    popq    %rdx
    popq    %r10

    addq    $8, %rsp
    popq    %rbp
    ret