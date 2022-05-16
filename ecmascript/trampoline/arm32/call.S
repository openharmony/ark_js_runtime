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

.global JSFunctionEntry
.type JSFunctionEntry, %function

.global CallRuntime
.type CallRuntime, %function

.global AsmInterpreterEntry
.type AsmInterpreterEntry, %function

.global JSCallDispatch
.type JSCallDispatch, %function

 .global OptimizedCallOptimized
.type OptimizedCallOptimized, %function

.global PushCallIThisRangeAndDispatch
.type PushCallIThisRangeAndDispatch, %function

.global PushCallIRangeAndDispatch
.type PushCallIRangeAndDispatch, %function

.global PushCallArgs3AndDispatch
.type PushCallArgs3AndDispatch, %function

.global PushCallArgs2AndDispatch
.type PushCallArgs2AndDispatch, %function

.global PushCallArgs1AndDispatch
.type PushCallArgs1AndDispatch, %function

.global PushCallArgs0AndDispatch
.type PushCallArgs0AndDispatch, %function

.global PushCallIThisRangeAndDispatchSlowPath
.type PushCallIThisRangeAndDispatchSlowPath, %function

.global PushCallIRangeAndDispatchSlowPath
.type PushCallIRangeAndDispatchSlowPath, %function

.global PushCallArgs3AndDispatchSlowPath
.type PushCallArgs3AndDispatchSlowPath, %function

.global PushCallArgs2AndDispatchSlowPath
.type PushCallArgs2AndDispatchSlowPath, %function

.global PushCallArgs1AndDispatchSlowPath
.type PushCallArgs1AndDispatchSlowPath, %function

.global PushCallArgs0AndDispatchSlowPath
.type PushCallArgs0AndDispatchSlowPath, %function

.global PushCallIRangeAndDispatchNative
.type PushCallIRangeAndDispatchNative, %function

.global PushCallArgsAndDispatchNative
.type PushCallArgsAndDispatchNative, %function

.global ResumeRspAndDispatch
.type ResumeRspAndDispatch, %function

.global ResumeRspAndReturn
.type ResumeRspAndReturn, %function

.global ResumeCaughtFrameAndDispatch
.type ResumeCaughtFrameAndDispatch, %function

.global CallRuntimeWithArgv
.type CallRuntimeWithArgv, %function

#define CALLEESAVE_REGS_SIZE   (32)
#define SLOT_SIZE              (4)
// 1: r11
#define ARGS_STACK_TOP_TO_FP_OFFSET (CALLEESAVE_REGS_SIZE + SLOT_SIZE)

// uint64_t JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
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
    add     r4, r11, #ARGS_STACK_TOP_TO_FP_OFFSET
    add     r4, r4,  #SLOT_SIZE                   // skip argv[]
    ldr     lr, [r4]   // load js function code address
    cmp     r8, r3
    movls   r3, r8
    cmp     r3, #0
    beq     .LInvokeJSFunction

    add     r4, r11, #ARGS_STACK_TOP_TO_FP_OFFSET // argv[]
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

// uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
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
//   |       argv[argc-1]       |
//   +--------------------------+
//   |       ..........         |
//   +--------------------------+
//   |       argv[1]            |
//   +--------------------------+
//   |       argv[0]            |
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

.global CallRuntime
.type CallRuntime, %function
CallRuntime:
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

//extern "C" JSTaggedType OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
//                                  uint32_t actualNumArgs, uintptr_t codeAddr, ...)
// Input:
// %r0 - glue
// %r1 - expectedNumArgs
// %r2 - actualNumArgs
// %r3 - codeAddr

.global OptimizedCallOptimized
.type OptimizedCallOptimized, %function
OptimizedCallOptimized:
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

// uint64_t CallNativeTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %r0 - glue
// %r1 - codeAddress
// %r2 - argc
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argv1
// sp + 16: argv0
// sp + 8:  actualArgc
// sp:      codeAddress
// construct Native Leave Frame:
//   +--------------------------+
//   |       argv0              | calltarget , newtARGET, this, ....
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|  Fixed OptimizedLeaveFrame
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

.global CallNativeTrampoline
.type CallNativeTrampoline, %function
CallNativeTrampoline:
    push    {r2}                                        // argc
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

    sub     sp, sp, #ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE
    mov     r4, #ASM_GLUE_TO_THREAD_OFFSET
    sub     r0, r0, r4   // get thread
    str     r0, [sp]
    sub     r0, r0, #3
    str     r2, [sp, #ECMA_RUNTIME_CALLINFO_NUMARGS_OFFSET]     // numArgs
    add     r0, r11,  #16   // argv[]
    str     r0, [sp, #ECMA_RUNTIME_CALLINFO_STACKARGS_OFFSET]    // stackArgs_
    mov     r0, #0
    str     r0, [sp, #ECMA_RUNTIME_CALLINFO_DATA_OFFSET]
    str     r0, [sp, #ECMA_RUNTIME_CALLINFO_PPREV_SP_OFFSET]
    mov     r0, sp
    blx     r1

    add     sp, sp, #ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE
    // descontruct leave frame and callee save register
    pop     {r4, r5}
    add     sp, sp, #8
    pop     {r11, lr}
    pop     {r2}
    bx      lr

// uint64_t JSCall(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new, JSTaggedType this, ...);
// webkit_jscc calling convention call js function()
// Input:
// %rax - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argc
// sp + 16: this
// sp + 8:  new
// sp:      jsfunc
//   +--------------------------+
//   |       ...                |
//   +--------------------------+
//   |       arg0               |
//   +--------------------------+
//   |       this               |
//   +--------------------------+
//   |       new                |
//   +--------------------------+ ---
//   |       jsfunction         |   ^
//   |--------------------------|  Fixed
//   |       argc               | OptimizedFrame
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |       frameType          |   v
//   +--------------------------+ ---
.global JSCall
.type JSCall, %function
JSCall:
    bx      lr

// uint64_t JSCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType callTarget, JSTaggedType argV[]);
// c++ calling convention call js function
// Input:
// %x0 - glue
// %x1 - argc
// %x2 - argV (calltarget, newtarget, thisObj, )
.global JSCallWithArgV
.type JSCallWithArgV, %function
JSCallWithArgV:
    bx      lr

PushCallIThisRangeAndDispatch:
    bx  lr

PushCallIRangeAndDispatch:
    bx  lr

PushCallArgs3AndDispatch:
    bx  lr

PushCallArgs2AndDispatch:
    bx  lr

PushCallArgs1AndDispatch:
    bx  lr

PushCallArgs0AndDispatch:
    bx  lr

PushCallIThisRangeAndDispatchSlowPath:
    bx  lr

PushCallIRangeAndDispatchSlowPath:
    bx  lr

PushCallArgs3AndDispatchSlowPath:
    bx  lr

PushCallArgs2AndDispatchSlowPath:
    bx  lr

PushCallArgs1AndDispatchSlowPath:
    bx  lr

PushCallArgs0AndDispatchSlowPath:
    bx  lr

PushCallIRangeAndDispatchNative:
    bx  lr

PushCallArgsAndDispatchNative:
    bx  lr

ResumeRspAndDispatch:
    bx  lr

ResumeRspAndReturn:
    bx  lr

ResumeCaughtFrameAndDispatch:
    bx  lr

AsmInterpreterEntry:
    bx  lr

JSCallDispatch:
    bx  lr

CallRuntimeWithArgv:
    bx  lr

.global JSCallEntry
.type JSCallEntry, %function
JSCallEntry:
    bx  lr
