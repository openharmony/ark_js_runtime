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

 #include "asm_defines.h"

 .global JSFunctionEntry
.type JSFunctionEntry, %function

.global RuntimeCallTrampolineAot
.type RuntimeCallTrampolineAot, %function

.global RuntimeCallTrampolineInterpreterAsm
.type RuntimeCallTrampolineInterpreterAsm, %function

 .global AotCallAotTrampoline
.type AotCallAotTrampoline, %function

.global HandleCommonCall
.type HandleCommonCall, %function

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

 /* JSFunctionEntry Callee Register restore Macro */
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

#define CALLEESAVE_REGS_SIZE   (48)
#define CALLER_SAVE_LR_SIZE    (8)
#define FP_SIZE                (8)
#define ARGS_STACK_TOP_TO_FP_OFFSET (CALLEESAVE_REGS_SIZE + CALLER_SAVE_LR_SIZE)

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, uintptr_t codeAddr, ...);
// Input:
// %rdi - glue
// %rsi - prevFp
// %rdx - expectedNumArgs
// %ecx - actualNumArgs
// %r8  - codeAddr
// %r9  - argv[0]
// sp[0 * 8]  -  argv[1]
// sp[1 * 8]  -  argv[2]
// sp[2 * 8]  -  argv[3]
// .....
// sp[(N -1) * 8] - argv[N - 1]
// Output:
// stack as followsn from high address to lowAdress
//  sp       -      argv[N - 1]
// sp[-8]    -      argv[N -2]
// ...........................
// sp[- 8(N - 1)] - arg[0]
// sp[-8(N)]   -    this
// sp[-8(N+1)] -    newTarget
// sp[-8(N + 2)] -  callTarget

JSFunctionEntry:
    PUSH_GENERAL_REGS
    // construct the frame
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    pushq   $JS_ENTRY_FRAME_TYPE    // optimized entry frame type
    pushq   %rsi                    // prev managed fp

    // 16 bytes align check
    movl    %edx, %r14d
    testb   $1, %r14b
    je      .LAlign16Bytes
    pushq   $0                  // push zero to align 16 bytes stack

.LAlign16Bytes:
    // expectedNumArgs > actualNumArgs
    movl     %edx, %ebx        // save expectedNumArgs
    cmpl     %ecx, %edx
    jbe     .LCopyArguments
    movl    %ecx, %eax
    movl    %ebx, %edx

.LCopyExtraAument:             // copy undefined value to stack
    pushq   $JSUNDEFINED

    addq    $-1, %rdx
    cmpq    %rax, %rdx
    ja      .LCopyExtraAument

.LCopyArguments:
    cmpl    %ecx, %ebx
    cmovbe  %ebx, %ecx
    movl    %ecx, %eax

    // argV[0] - r9
    addq $-1, %rax
    cmpq  $0, %rax
    jbe  .LCopyLoopEnd

    // PUSH_GENERAL_REGS and return address alignment
    movq %rbp, %r15
    addq $FP_SIZE, %r15
    addq $ARGS_STACK_TOP_TO_FP_OFFSET, %r15

.LCopyLoop:
    movq    -8(%r15, %rax, 8), %rcx
    pushq   %rcx

    addq    $-1, %rax
    jne     .LCopyLoop

.LCopyLoopEnd:
    // argV[0]
    pushq   %r9
    movq    %rdi, %rax          // move glue to rax
    call    *%r8                // then call jsFunction
    leaq    (,%r14,8), %rcx
    addq    %rcx, %rsp

    testb   $1, %r14b           // stack 16bytes align check
    je      .LPopFrame
    addq    $8, %rsp

.LPopFrame:
    popq    %rsi           // pop prev-managed frame
    addq    $8, %rsp
    popq    %rbp

    POP_GENERAL_REGS
    movq    %rsi, ASM_GLUE_CURRENT_FRAME_OFFSET(%rdi)   // restore thread->currentFrame_
    ret


// uint64_t AotCallAotTrampoline(uintptr_t glue, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, uintptr_t codeAddr, ...);
// Input:
// %rdi - glue
// %rsi - expectedNumArgs
// %rdx - actualNumArgs
// %ecx - codeAddr
// %r8  - argv[0]
// %r9  - argv[1]
// sp[0 * 8]  -  argv[2]
// sp[1 * 8]  -  argv[3]
// sp[2 * 8]  -  argv[4]
// .....
// sp[(N -3) * 8] - argv[N - 1]
// Output:
// stack as followsn from high address to lowAdress
//  sp       -      argv[N - 1]
// sp[-8]    -      argv[N -2]
// ...........................
// sp[- 8(N - 1)] - arg[0]

AotCallAotTrampoline:
    PUSH_GENERAL_REGS

    // 16 bytes align check
    movl    %esi, %r14d
    testb   $1, %r14b
    je      .LAlign16Bytes1
    pushq   $0                  // push zero to align 16 bytes stack

.LAlign16Bytes1:
    // expectedNumArgs > actualNumArgs
    movl     %esi, %ebx        // save expectedNumArgs
    cmpl     %edx, %esi
    jbe     .LCopyArguments1
    movl    %edx, %eax
    movl    %ebx, %esi

.LCopyExtraAument1:             // copy undefined value to stack
    pushq   $JSUNDEFINED

    addq    $-1, %rsi
    cmpq    %rax, %rsi
    ja      .LCopyExtraAument1

.LCopyArguments1:
    cmpl    %edx, %ebx
    cmovbe  %ebx, %edx
    movl    %edx, %eax

    // r12: extraArgument alignment
    movq %r14, %r12
    subq %rdx, %r12

    testb   $1, %r14b           // stack 16bytes align check
    je      .align16
    addq $1, %r12
.align16:
    cmpq  $0, %rax
    jbe  .ArgNumsEqualZero
    cmpq  $1, %rax
    jbe  .ArgNumsEqualOne
    cmpq  $2, %rax
    jbe  .ArgNumsEqualTwo

    // r12: extragArgument size
    leaq    (,%r12,8), %r12
    movq %rsp, %r15
    addq $ARGS_STACK_TOP_TO_FP_OFFSET, %r15
    addq %r12, %r15

    // argV[0] - r8 ;argv[1]-r9
    addq $-2, %rax
.ArgNumsMoreTwo:
    movq    -8(%r15, %rax, 8), %r12
    pushq   %r12
    addq    $-1, %rax
    jne     .ArgNumsMoreTwo
.ArgNumsEqualTwo:
    // argv[1] r9
    push %r9
.ArgNumsEqualOne:
    // argv[0] r8
    pushq   %r8
.ArgNumsEqualZero:
    movq    %rdi, %rax          // move glue to rax
    movq    %rcx, %r8
    callq   *%r8                // then call jsFunction
    leaq    (,%r14,8), %rcx
    addq    %rcx, %rsp

    testb   $1, %r14b           // stack 16bytes align check
    je      .LPopFrame1
    addq    $8, %rsp

.LPopFrame1:
    POP_GENERAL_REGS
    ret

// uint64_t RuntimeCallTrampoline(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %x0 - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argv1
// sp + 16: argv0
// sp + 8:  argc
// sp:      runtime_id
// construct Leave Frame:
//   +--------------------------+
//   |       argv[]             |
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|  Fixed
//   |       RuntimeId          | OptimizedLeaveFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---

// Output:
//  sp - 8 : pc
//  sp - 16: rbp <---------current rbp & current sp
//  current sp - 8:  type

RuntimeCallTrampolineAot:
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
    movl    8(%rdx), %esi    // argc
    addq    $16, %rdx        // argV
    call    *%r10

    addq    $8, %rsp
    popq    %rdx
    popq    %r10

    addq    $8, %rsp
    popq    %rbp
    ret

RuntimeCallTrampolineInterpreterAsm:
    pushq   $0
    movq    %rsp, ASM_GLUE_LEAVE_FRAME_OFFSET(%rax)   // save to thread->leaveFrame_
    pushq   $ASM_LEAVE_FRAME_TYPE
    
    // callee save
    pushq    %r10
    pushq    %rdx
    pushq    %rax

    // construct leave frame
    movq    %rsp, %rdx
    addq    $48, %rdx        // 16 : for rbp & return address

    // load runtime trampoline address
    movq    (%rdx), %r10       // runtime id
    movq    ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET(%rax, %r10, 8), %r10
    movq    %rax, %rdi       // glue
    movl    8(%rdx), %esi   // argc
    addq    $16, %rdx        // argV
    call    *%r10

    addq    $8, %rsp
    popq    %rdx
    popq    %r10

    addq    $8, %rsp
    addq    $8, %rsp
    ret

// void HandleCommonCall(uintptr_t glue, uint64_t callType, uintptr_t sp, uint64_t funcReg, uint64_t actualArgc, ...);
HandleCommonCall:
    ret