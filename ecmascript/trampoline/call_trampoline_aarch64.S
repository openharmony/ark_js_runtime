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

 /* JSFunctionEntry Callee Register Save Macro */
.macro PUSH_CALLEE_SAVED_REGS
    stp x27, x28, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x19, x20, [sp, #-16]!
    stp d14, d15, [sp, #-16]!
    stp d12, d13, [sp, #-16]!
    stp d10, d11, [sp, #-16]!
    stp d8, d9, [sp, #-16]!
.endm

 /* JSFunctionEntry Callee Register restore Macro */
.macro POP_CALLEE_SAVED_REGS
    /* callee save register */
    ldp d8, d9,   [sp], #16
    ldp d10, d11, [sp], #16
    ldp d12, d13, [sp], #16
    ldp d14, d15, [sp], #16
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
.endm

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
// Input:
// %x0 - glue
// %x1 - prevFp
// %x2 - expectedNumArgs
// %x3 - actualNumArgs
// %x4 - argV
// %x5 - codeAddr
// construct Entry Frame
//   +--------------------------+
//   |   returnaddress      |   ^
//   |----------------------|   |
//   |calleesave registers  | Fixed
//   |----------------------| OptimizedEntryFrame
//   |      prevFp          |   |
//   |----------------------|   |
//   |      frameType       |   |
//   |----------------------|   |
//   |  prevLeaveFrameFp    |   v
//   +--------------------------+

.global JSFunctionEntry
.type JSFunctionEntry, %function
JSFunctionEntry:
    str     x30, [sp, #-8]!          // returnaddress
    PUSH_CALLEE_SAVED_REGS
    str     x29, [sp, #-8]!          // callee c-fp
    mov     x29, sp
    mov     x19, #JS_ENTRY_FRAME_TYPE
    stp     x1, x19, [sp, #-16]!

    mov     x20, x0
    mov     w19, w2
    mov     x2, x1
    cmp     w19, w3
    b.ls    .LCopyArguments
    mov     w8, #10
    mov     w9, w19

.LCopyUndefined:
    sub     x9, x9, #1
    cmp     w9, w3
    str     x8, [sp, #-8]!
    b.hi    .LCopyUndefined
.LCopyArguments:
    cmp     w19, w3
    csel    w8, w19, w3, lo
    cbz     w8, .InvokeCompiledJSFunction
    sub     w9, w8, #1              // =1
    add     x9, x4, w9, uxtw #3
.LCopyArgLoop:
    ldr     x10, [x9], #-8
    subs    w8, w8, #1              // =1
    str     x10, [sp, #-8]!

    b.ne    .LCopyArgLoop

// Input:
// %x0 - glue
// argv push stack
.InvokeCompiledJSFunction:
    mov     x2, x5
    blr     x2

    // pop argv
    lsl     w8, w19, #3
    add     sp, sp, x8
    // pop prevLeaveFrameFp to restore thread->currentFrame_
    ldr    x19, [sp]
    add    sp, sp, #0x8
    str    x19, [x20, #ASM_GLUE_CURRENT_FRAME_OFFSET]
    // pop entry frame type and c-fp
    add    sp, sp, #0x8
    ldr    x29, [sp]
    add    sp, sp, #0x8

    // restore callee save registers
    POP_CALLEE_SAVED_REGS
    // restore return address
    ldr     x30, [sp]
    add     sp, sp, #0x8
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
//  sp - 8 : x30
//  sp - 16: x29 <---------current x29 & current sp
//  current sp - 8:  type
//  current sp - 16: callee save x19

.global RuntimeCallTrampolineAot
.type RuntimeCallTrampolineAot, %function
RuntimeCallTrampolineAot:
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp                // set frame pointer = callsiteFp
    str     x29, [x0, #ASM_GLUE_CURRENT_FRAME_OFFSET]       // save to thread->currentFrame_
    str     x19, [sp, #-16]        // callee save register

    // construct leave frame
    mov     x19, #LEAVE_FRAME_TYPE
    str     x19, [sp, #-8]
    add     sp, sp, #-16

    // load runtime trampoline address
    ldr     x19, [x29, #16]  // runtime_id
    add     x19, x0, x19, lsl #3
    ldr     x19, [x19, #ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET]
    ldr     x1, [x29, #24]  // argc
    add     x2, x29,  #32   // argv[]
    blr     x19

    // descontruct leave frame and callee save register
    ldr     x19, [sp]
    add   sp, sp, #16
    // restore register
    ldp     x29, x30, [sp], #16
    ret

.global RuntimeCallTrampolineInterpreterAsm
.type RuntimeCallTrampolineInterpreterAsm, %function
RuntimeCallTrampolineInterpreterAsm:
    stp     #0, x30, [sp, #-16]!  // save register for fp, rip
    str     sp, [x0, #ASM_GLUE_LEAVE_FRAME_OFFSET]       // save to thread->currentFrame_
    str     x19, [sp, #-16]        // callee save register

    // construct leave frame
    mov     x19, #ASM_LEAVE_FRAME_TYPE
    str     x19, [sp, #-8]
    add     sp, sp, #-16

    // load runtime trampoline address
    ldr     x19, [sp, #32]  // runtime_id
    add     x19, x0, x19, lsl #3
    ldr     x19, [x19, #ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET]
    ldr     x1, [sp, #40]  // argc
    add     x2, sp,  #48   // argv[]
    blr     x19

    // descontruct leave frame and callee save register
    ldr     x19, [sp]
    add   sp, sp, #24
    // restore register
    ldr     x30, [sp], #8
    ret