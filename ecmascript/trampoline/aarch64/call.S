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

.global GeneratorReEnterAsmInterp
.type GeneratorReEnterAsmInterp, %function

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

.global ResumeUncaughtFrameAndReturn
.type ResumeUncaughtFrameAndReturn, %function

.global CallRuntimeWithArgv
.type CallRuntimeWithArgv, %function

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

#define SLOT_SIZE              (8)
#define CALLEESAVE_REGS_SIZE   (18 * SLOT_SIZE)
// 2: returnAddr and x29
#define ARGS_STACK_TOP_TO_FP_OFFSET  (CALLEESAVE_REGS_SIZE + 2 * SLOT_SIZE)

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

JSFunctionEntry:
    ret

// uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
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

// Output:
//  sp - 8 : x30
//  sp - 16: x29 <---------current x29 & current sp
//  current sp - 8:  type
//  current sp - 16: callee save x19

CallRuntime:
    ret

// extern "C" JSTaggedType OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
//                                  uint32_t actualNumArgs, uintptr_t codeAddr, uintptr_t argv)
// Input:
// %x0 - glue
// %w1 - expectedNumArgs
// %w2 - actualNumArgs
// %x3 - codeAddr
// %x4 - argv

// sp[0 * 8]  -  argc
// sp[1 * 8]  -  argv[0]
// sp[2 * 8]  -  argv[1]
// .....
// sp[(N -3) * 8] - argv[N - 1]
// Output:
// stack as followsn from high address to lowAdress
//  sp       -      argv[N - 1]
// sp[-8]    -      argv[N -2]
// ...........................
// sp[- 8(N - 1)] - arg[0]
// sp[- 8(N)]     - argc

OptimizedCallOptimized:
    ret

// uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %x0 - glue
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
//   |--------------------------|  Fixed
//   |       codeAddress        | OptimizedLeaveFrame
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

.global CallBuiltinTrampoline
.type CallBuiltinTrampoline, %function
CallBuiltinTrampoline:
    ret

// uint64_t JSCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType callTarget, JSTaggedType argV[]);
// c++ calling convention call js function
// Input:
// %x0 - glue
// %x1 - argc
// %x2 - argV (calltarget, newtarget, thisObj, )
.global JSCallWithArgV
.type JSCallWithArgV, %function


// uint64_t JSCall(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, JSTaggedType new, JSTaggedType this, ...);
// webkit_jscc calling convention call js function()
// Input:
// %x0 - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argc
// sp + 16: this
// sp + 8:  new
// sp:      jsfunc
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
//   |       RuntimeId          | OptimizedFrame
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
    ret

JSCallWithArgV:
    ret

PushCallIThisRangeAndDispatch:
    ret

PushCallIRangeAndDispatch:
    ret

PushCallArgs3AndDispatch:
    ret

PushCallArgs2AndDispatch:
    ret

PushCallArgs1AndDispatch:
    ret

PushCallArgs0AndDispatch:
    ret

PushCallIThisRangeAndDispatchSlowPath:
    ret

PushCallIRangeAndDispatchSlowPath:
    ret

PushCallArgs3AndDispatchSlowPath:
    ret

PushCallArgs2AndDispatchSlowPath:
    ret

PushCallArgs1AndDispatchSlowPath:
    ret

PushCallArgs0AndDispatchSlowPath:
    ret

PushCallIRangeAndDispatchNative:
    ret

PushCallArgsAndDispatchNative:
    ret

ResumeRspAndDispatch:
    ret

ResumeRspAndReturn:
    ldr    x30, [sp], #16 // 16 byte align
    ret

ResumeCaughtFrameAndDispatch:
    ret

ResumeUncaughtFrameAndReturn:
    ret

AsmInterpreterEntry:
    ret

GeneratorReEnterAsmInterp:
    ret

JSCallDispatch:
    ret

CallRuntimeWithArgv:
    ret

.type JSCallEntryInternal, %function
JSCallEntryInternal:
    str  x30, [sp, #-16]! // 16 byte align
    br   x8

.global JSCallEntry
.type JSCallEntry, %function
JSCallEntry:
    stp	x29, x30, [sp, #-160]!
    stp	d15, d14, [sp, #16]
 	stp	d13, d12, [sp, #32]
 	stp	d11, d10, [sp, #48]
 	stp	d9, d8, [sp, #64]
 	stp	x28, x27, [sp, #80]
 	stp	x26, x25, [sp, #96]
 	stp	x24, x23, [sp, #112]
 	stp	x22, x21, [sp, #128]
 	stp	x20, x19, [sp, #144]
 	ldrb	w8, [x2]
 	mov	x29, x1
 	mov	x19, x0
 	mov	x20, x2
 	add	x8, x0, x8, lsl #3
 	ldr	x8, [x8, #ASM_GLUE_BC_HANDLERS_OFFSET]
 	mov	x21, x3
 	mov	x22, x4
 	mov	x23, x5
 	mov	x24, x6
    bl JSCallEntryInternal
 	ldp	x20, x19, [sp, #144]
 	ldp	x22, x21, [sp, #128]
 	ldp	x24, x23, [sp, #112]
 	ldp	x26, x25, [sp, #96]
 	ldp	x28, x27, [sp, #80]
 	ldp	d9, d8, [sp, #64]
 	ldp	d11, d10, [sp, #48]
 	ldp	d13, d12, [sp, #32]
 	ldp	d15, d14, [sp, #16]
 	ldp	x29, x30, [sp], #160
 	ret
