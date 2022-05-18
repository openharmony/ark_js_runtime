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
    str     x30, [sp, #-8]!          // returnaddress
    PUSH_CALLEE_SAVED_REGS
    str     x29, [sp, #-8]!          // callee c-fp
    mov     x29, sp
    mov     x19, #JS_ENTRY_FRAME_TYPE
    stp     x1, x19, [sp, #-16]!

    mov     x20, x0
    mov     w19, w2
    mov     x2, x1
    // expectedNumArgs <= actualNumArgs
    cmp     w19, w3
    b.ls    .LCopyArguments
    mov     w8, #JSUNDEFINED
    mov     w9, w19

.LCopyUndefined:
    sub     x9, x9, #1
    cmp     w9, w3
    str     x8, [sp, #-8]!
    b.hi    .LCopyUndefined
.LCopyArguments:
    // w8 = min(expectedNumArgs, actualNumArgs)
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
    str     x3, [sp, #-8]!
    blr     x5

    // pop argv
    add     sp, sp, w19, uxtw #3
    add     sp, sp, #8

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
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp                // set frame pointer = callsiteFp
    str     x29, [x0, #ASM_GLUE_LEAVE_FRAME_OFFSET]       // save to thread->currentFrame_
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
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp
    mov     x5, #OPTIMIZE_FRAME_TYPE
    str     x5, [sp, #-8]!
    // callee save
    str     x19, [sp, #-8]!

    mov     w5, w1
    // expectedNumArgs <= actualNumArgs
    cmp     w5, w2
    b.ls    .LCopyArguments1
    // undefined
    mov     x8, #JSUNDEFINED

.LCopyUndefined1:
    sub     x5, x5, #1
    cmp     w5, w2
    str     x8, [sp, #-8]!
    b.hi    .LCopyUndefined1
.LCopyArguments1:
    // w8 = min(expectedNumArgs, actualNumArgs)
    cmp     w1, w2
    csel    w5, w1, w2, lo
    cbz     w5, .InvokeCompiledJSFunction1
    mov     x19, x1                 // save expected numArgs
    sub     w5, w5, #1
    add     x5, x4, w5, uxtw #3
.LCopyArgLoop1:
    ldr     x10, [x4], #-8
    subs    w5, w5, #1
    str     x10, [sp, #-8]!

    b.ne    .LCopyArgLoop1

// Input:
// %x0 - glue
// argv push stack
.InvokeCompiledJSFunction1:
    str     x2, [sp, #-8]!
    blr     x3

    // pop argv
    add     sp, sp, w19, uxtw #3
    add     sp, sp, #8
    // callee restore
    ldr     x19, [sp], #8
    // deconstruct frame
    add     sp, sp, #8
    ldp     x29, x30, [sp], #16
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
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp                // set frame pointer = callsiteFp
    str     x29, [x0, #ASM_GLUE_CURRENT_FRAME_OFFSET]       // save to thread->currentFrame_

    // construct leave frame and callee save
    mov     x1, #LEAVE_FRAME_TYPE
    stp     x19, x1, [sp, #-16]!

    // load runtime trampoline address
    ldr     x19, [x29, #16]  // codeaddress

    // construct ecma_runtime_call_info
    sub     sp, sp, #ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE
    mov     x1, #ASM_GLUE_TO_THREAD_OFFSET
    sub     x0, x0, x1       // get thread
    str     x0, [sp]
    ldr     x0, [x29, #24]  // argc
    sub     x0, x0, #3
    str     x0, [sp, #ECMA_RUNTIME_CALLINFO_NUMARGS_OFFSET]     // numArgs
    add     x1, x29, #32    //argV
    str     x1, [sp, #ECMA_RUNTIME_CALLINFO_STACKARGS_OFFSET]     // data_
    mov     x0, 0
    str     x0, [sp, #ECMA_RUNTIME_CALLINFO_DATA_OFFSET]
    str     x0, [sp, #ECMA_RUNTIME_CALLINFO_PPREV_SP_OFFSET]

    mov     x0, sp
    blr     x19

    add     sp, sp, #ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE
    // descontruct leave frame and callee save register
    ldp     x19, x1, [sp], #16
    // restore register
    ldp     x29, x30, [sp], #16
    add     sp, sp, #8          // revert native pointer
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
    ldr     x1, [sp, #8]       // get jsfunc
    mov     x2, TAGGED_MASK
    cmp     x1, x2              // TaggedIsInt
    b.hs    .LNonCallable
    cbz     x1, .LNonCallable   // IsHole
    mov     x2, TAGGED_SPECIAL_VALUE
    and     x2, x1, x2
    cbnz    x2, .LNonCallable   // IsSpecial
    ldr     x2, [x1]            // get jshclass
    ldr     x2, [x2, #JS_HCLASS_BITFIELD_OFFSET]                  // get bitfield
    tbz     x2, #JS_HCLASS_BITFIELD_CALLABLE_BIT, .LNonCallable   // is callable
    and     w3, w2, #0xff
    sub     w3, w3, #4
    cmp     w3, #9
    b.hs    .LNotJSFunction
    // x0 -- glue
    // x1 -- jsfunc
    ldr     x2, [x1, #JS_FUNCTION_METHOD_OFFSET]    // get method
    ldr     x3, [sp, #8]                            // actual argc
    ldr     x4, [x2, #JS_METHOD_CALLFIELD_OFFSET]   // get call field of method
    tbnz    x4, #JS_METHOD_CALL_FIELD_NATIVE_BIT, .LCallNativeMethod
    tbnz    x4, #JS_METHOD_CALL_FIELD_AOT_BIT, .LCallOptimziedMethod
    brk     0
.LCallNativeMethod:
    ldr     x4, [x2, #JS_METHOD_NATIVE_POINTER_OFFSET]  // get native pointer
    str     x4, [sp, #-8]!                              // push native address
    b       CallBuiltinTrampoline

// Input:
// x0 -- glue
// x1 -- jsfunc
// x2 -- method
// x3 -- actual argc
// x4 -- callfield
// output:
// %x0 - glue
// %w1 - expectedNumArgs
// %w2 - actualNumArgs
// %x3 - codeAddr
.LCallOptimziedMethod:
    mov     x2, x3                                      // actual argc
    ldr     x3, [x1, #JS_FUNCTION_CODE_ENTRY_OFFSET]    // get codeaddress
    lsr     x4, x4, #JS_METHOD_NUM_ARGS_SHIFT
    and     w4, w4, #ASM_JS_METHOD_NUM_ARGS_MASK
    add     w1, w4, #ASM_NUM_MANDATORY_JSFUNC_ARGS      // add mandatory arguments
    cmp     w2, w1                                      // expectedNumArgs <= actualNumArgs
    add     x4, sp, #8                                  // argv
    b.hi    .LDirectCallCodeEntry
    b       OptimizedCallOptimized
.LDirectCallCodeEntry:
    br      x3

.LNotJSFunction:
    and     w5, w2, #0xff
    cmp     w5, #0x0d       // IsJsBoundFunction
    b.eq    .LJSBoundFunction
    cmp     w5, #0x4d       // IsJsProxy
    b.eq    .LJSProxy
    ret

// x0 -- glue
// x1 -- jsfunc
// x2 -- method
// x3 -- actual argc
// x4 -- callfield
.LJSBoundFunction:
    // construct frame
    stp     x29, x30, [sp, #-16]!           // save register for fp, rip
    mov     x29, sp
    mov     x5, #OPTIMIZE_FRAME_TYPE
    str     x5, [sp, #-8]!                  // set frame type

    add     x4, x29, #16                    // &argc -> x4
    // callee save
    str     x19, [sp, #-8]!

    // get bound arguments
    ldr     x2, [x1, #JS_FUNCTION_BOUND_ARG_OFFSET]
    // get bound length
    ldr     x2, [x2, #TAGGED_ARRAY_LENGTH_OFFSET]
    add     w19, w2, w3                    // boundNumArgs + actualArgs
    add     x4, x4, w3, uxtw #3            // argV[actualArgs - 1] -> x4
    sub     w3, w3, #ASM_NUM_MANDATORY_JSFUNC_ARGS
    cmp     w3, #0
    b.eq    .LCopyBoundArgument

.LCopyArgument2:
    ldr     x5, [x4], #-8
    str     x5, [sp, #-8]!
    add     w3, w3, #-1
    b.hi    .LCopyArgument2

.LCopyBoundArgument:
    // get bound arguments
    ldr     x4, [x1, #JS_FUNCTION_BOUND_ARG_OFFSET]
    add     x4, x4, #TAGGED_ARRAY_DATA_OFFSET
    cmp     w2, #0
    b.eq    .LPushCallTarget
    add     w2, w2, #-1
    add     x4, x4, w2, uxtw #3         // boundArg[length - 1]
.LCopyBoundArgumentLoop:
    ldr     x5, [x4], #-8
    str     x5, [sp, #-8]!
    add     w2, w2, #-1
    b.pl    .LCopyBoundArgumentLoop      // w2 >= 0

.LPushCallTarget:
    ldr     x5, [x1, #JS_FUNCTION_BOUND_THIS_OFFSET]     // thisObj
    mov     x6, #JSUNDEFINED                             // newTarget
    stp     x6, x5, [sp, #-16]!
    ldr     x5, [x1, #JS_FUNCTION_BOUND_TARGET_OFFSET]
    stp     x19, x5, [sp, #-16]!                           // calltarget actualArgc
    bl      JSCall                                         // recursive call jscall
    add     sp, sp, #8
    add     sp, sp, w19, uxtw #3
    ldr     x19, [sp, #8]!
    add     sp, sp, #8
    ldp     x29, x30, [sp, #16]!
    ret

.LJSProxy:
    brk     0
    ret
.LNonCallable:
    stp     x29, x30, [sp, #-16]!  // save register for fp, rip
    mov     x29, sp
    mov     x6, #OPTIMIZE_FRAME_TYPE
    mov     x5, #5
    and     x5, x5, #TAGGED_MASK
    stp     x5, x6, [sp, #-16]!
    mov     x5, #1
    mov     x6, #RUNTIME_ID_THROW_TYPE_ERROR
    stp     x5, x6, [sp, #-16]!
    bl      CallRuntime
    mov     x0, #TAGGED_VALUE_EXCEPTION
    add     sp, sp, #32
    ldp     x29, x30, [sp], #16
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

AsmInterpreterEntry:
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
