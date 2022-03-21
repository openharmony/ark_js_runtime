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

#define CALLEESAVE_REGS_SIZE   (32)
#define SLOT_SIZE              (4)
// 1: r11
#define ARGS_STACK_TOP_TO_FP_OFFSET (CALLEESAVE_REGS_SIZE + SLOT_SIZE)

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, uintptr_t codeAddr, ...);
// Input:
// %r0 - glue
// %r1 - prevFp
// %r2 - expectedNumArgs
// %r3 - actualNumArgs
JSFunctionEntry:
    push    {r4, r5, r6, r7, r8, r9, r10, lr}   // callee save
    push    {r11}                               // rbp
    mov     r11, sp                             // set frame pointer
    // construct frame
    mov     r6, #JS_ENTRY_FRAME_TYPE
    mov     r8, #0
    push    {r6}                                // frame type
    push    {r1}                                // prev managed frame
    push    {r1}                                // prev managed frame
    push    {r0}

    mov     r8, r2                             // save expected 
    cmp     r2, r3
    bls     .LCopyArguments
    mov     r6, #JSUNDEFINED
    mov     r10, #0
    mov     r2, r8
    mov     r4, #0
.LCopyExtraAument:
    subs    r2, r2, #1
    mov     r7, r6
    mov     r9, r6
    push    {r6, r10}

    sbc     r4, r4, #0
    subs    r5, r3, r2
    rscs    r5, r4, #0
    blo     .LCopyExtraAument
.LCopyArguments:
    ldr     lr, [r11, #ARGS_STACK_TOP_TO_FP_OFFSET]   // load js function code address
    cmp     r8, r3
    movls   r3, r8
    cmp     r3, #0
    beq     .LInvokeJSFunction

    add     r4, r11, #ARGS_STACK_TOP_TO_FP_OFFSET
    add     r4, r4,  #SLOT_SIZE                   // skip codeAddr,load argv[0] addr
    add     r5, r4, r3, lsl #3
.LCopyLoop:
    sub     r5, r5, #8
    subs    r3, r3, #1
    ldr     r10, [r5, #4]
    ldr     r6, [r5]
    push    {r6, r10}

    bne     .LCopyLoop
.LInvokeJSFunction:
    mov     r10, #OPTIMIZE_FRAME_TYPE  // set frame type
    mov     r9, #OPTIMIZE_FRAME_TYPE  // set frame type
    blx     lr
    mov     r2, #0                    // need fix for calltarget thisTarget NewTarget
    add     r2, r2, r8, lsl #3
    add     sp, sp, r2

    pop     {r4}
    pop     {r1}
    str     r1, [r4, #ASM_GLUE_CURRENT_FRAME_OFFSET]
    add     sp, sp, #8
    pop     {r11}
    pop     {r4, r5, r6, r7, r8, r9, r10, lr}
    bx      lr

// uint64_t RuntimeCallTrampoline(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %r0 - glue
// %r2, %r3 - runtime_id 
// stack layout:
// sp + N*8 argvN
// ........
// sp + 16: argv1
// sp + 8:  argv0
// sp:      argc
// construct Leave Frame:
//   +--------------------------+
//   |       argv[]             |
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|  Fixed OptimizedLeaveFrame
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   |
//   |--------------------------|   |
//   |       padding            |   v
//   +--------------------------+ ---

.global RuntimeCallTrampolineAot
.type RuntimeCallTrampolineAot, %function
RuntimeCallTrampolineAot:
    push    {r11, lr}                                   // save register for fp, rip
    mov     r11, sp                                     // set frame pointer = callsiteFp
    str     r11, [r0, #ASM_GLUE_CURRENT_FRAME_OFFSET]   // save to thread->currentFrame_
    add     sp, sp, #-8                                 // increase frame size
    push    {r4, r5}                                    // callee save

    // construct leave frame
    mov     r4, #0
    mov     r5, #LEAVE_FRAME_TYPE
    str     r5, [r11, #-4]                             // save frame type
    str     r4, [r11, #-8]                             // padding


    // load runtime trampoline address
    add     r2, r0, r2, lsl #2
    ldr     r4, [r2, #ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET]
    ldr     r1, [r11, #8]   // argc
    add     r2, r11,  #16   // argv[]
    blx     r4

    // descontruct leave frame and callee save register
    pop     {r4, r5}
    add     sp, sp, #8
    pop     {r11, lr}
    bx      lr

.global RuntimeCallTrampolineInterpreterAsm
.type RuntimeCallTrampolineInterpreterAsm, %function
RuntimeCallTrampolineInterpreterAsm:
    add     sp, sp, #-8
    str     lr, [sp, #4]
    str     sp, [r0, #ASM_GLUE_LEAVE_FRAME_OFFSET]   // save to thread->currentFrame_
    add     sp, sp, #-8                                 // increase frame size
    push    {r4, r5}                                    // callee save

    // construct leave frame
    mov     r4, #0
    mov     r5, #ASM_LEAVE_FRAME_TYPE
    str     r4, [sp, #12]
    str     r5, [sp, #8]                              // save frame type


    // load runtime trampoline address
    add     r2, r0, r2, lsl #2
    ldr     r4, [r2, #ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET]
    ldr     r1, [sp, #24]  // argc
    add     r2, sp,  #32   // argv[]
    blx      r4

    // descontruct leave frame and callee save register
    pop     {r4, r5}
    add     sp, sp, #12
    ldr     r1, [sp, #0]
    bx      lr


//extern "C" JSTaggedType AotCallAotTrampoline(uintptr_t glue, uint32_t expectedNumArgs,
//                                  uint32_t actualNumArgs, uintptr_t codeAddr, ...)
// Input:
// %r0 - glue
// %r1 - expectedNumArgs
// %r2 - actualNumArgs
// %r3 - codeAddr

.global AotCallAotTrampoline
.type AotCallAotTrampoline, %function
AotCallAotTrampoline:
    push    {r4, r5, r6, r7, r8, r9, r10, lr}   // callee save

    mov     r8, r1                             // save expected
    cmp     r1, r2
    bls     .LCopyArguments1
    mov     r6, #JSUNDEFINED
    mov     r10, #0
    mov     r1, r8
    mov     r4, #0
.LCopyExtraAument1:
    subs    r1, r1, #1
    mov     r7, r6
    mov     r9, r6
    push    {r6, r10}

    sbc     r4, r4, #0
    subs    r5, r2, r1
    rscs    r5, r4, #0
    blo     .LCopyExtraAument1
.LCopyArguments1:
    cmp     r8, r2
    movls   r2, r8
    cmp     r2, #0
    beq     .LInvokeJSFunction1

    add    r4, sp, #CALLEESAVE_REGS_SIZE
    // 4 * (expectedNumArgs - actualNumArgs)
    sub    r6, r8, r2
    add    r4, r4, r6, lsl #3 // load argv[0] addr
    add    r5, r4, r2, lsl #3
.LCopyLoop1:
    sub     r5, r5, #8
    subs    r2, r2, #1
    ldr     r10, [r5, #4]
    ldr     r6, [r5]
    push    {r6, r10}

    bne     .LCopyLoop1
.LInvokeJSFunction1:
    blx     r3
    mov     r1, #0
    add     r1, r1, r8, lsl #3
    add     sp, sp, r1

    pop     {r4, r5, r6, r7, r8, r9, r10, lr}
    bx      lr