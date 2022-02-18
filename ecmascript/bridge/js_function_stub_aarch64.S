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
// %x0 - glue
// %x1 - prevFp
// %x2 - expectedNumArgs
// %x3 - actualNumArgs
// %x4 - argV
// %x5 - codeAddr
.global InvokeJSFunctionEntry
.type InvokeJSFunctionEntry, %function
InvokeJSFunctionEntry:
    stp     x29, x30, [sp, #-32]!  // save register for fp, rip
    str     x19, [sp, #16]         // callee save register
    // construct the frame
    mov     w19, #0x1              // optimized entry frame type
    stp     x19, x1, [sp, #-16]!

    mov     x29, sp
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

    add     w19, w19, #2 // 2: prevFp and frameType
    lsl     w8, w19, #3
    add     sp, sp, x8

    ldr     x19, [sp, #16]
    ldp     x29, x30, [sp], #32
    ldr     x1, [x19, #2056]    // restore thread->leaveFp
    ret

// uint64_t CallRuntimeTrampoline(uintptr_t glue, uint32_t runtime_id, uint32_t patch_id, uint32_t argc, ...);
// webkit_jscc calling convention
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


// Output:
//  sp - 8 : x30
//  sp - 16: x29 <---------current x29 & current sp
//  current sp - 8:  type             ^
//  current sp - 16: patchId          |
//  current sp - 24: callsiteSp      leave frame
//                                    V
//  current sp - 32: callee save x19

.global CallRuntimeTrampoline
.type CallRuntimeTrampoline, %function
CallRuntimeTrampoline:
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp                // set frame pointer = callsiteFp
    str     x29, [x0, #2056]       // save to thread->currentFrame_
    str     x19, [sp, #-32]        // callee save register

    // construct leave frame
    mov     x19, #0x3     //type
    str     x19, [sp, #-8]
    ldr     x19, [fp, #40]
    str     x19, [sp, #-16] // patchId
    str     x29, [sp, #-24] //callsiteSp ignored
    add     sp, sp, #-32

    // load runtime trampoline address
    ldr     x19, [x29, #16]  // runtime_id
    add     x19, x0, x19, lsl #3
    ldr     x19, [x19, #2072]
    ldr     x1, [x29, #32]  // argc
    add     x2, x29,  #40  // argv[]
    blr     x19

    // descontruct leave frame and callee save register
    ldr     x19, [sp]
    add   sp, sp, #32
    // restore register
    ldp     x29, x30, [sp], #16
    ret