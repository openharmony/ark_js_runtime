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

.global CallRuntimeWithArgv
.type CallRuntimeWithArgv, %function

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
//                                uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
// Input:
// %rdi - glue
// %rsi - prevFp
// %rdx - expectedNumArgs
// %ecx - actualNumArgs
// %r8  - argV
// %r9  - codeAddr
JSFunctionEntry:
    ret


// uint64_t OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
//                                uint32_t actualNumArgs, uintptr_t codeAddr, uintptr_t argv);
// Input:
// %rdi - glue
// %rsi - expectedNumArgs
// %rdx - actualNumArgs
// %rcx - codeAddr
// %r8  - argv

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

OptimizedCallOptimized:
    ret

// uint64_t CallBuiltinTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// Input:
// %rax - glue
// stack layout:
// sp + N*8 argvN
// ........
// sp + 24: argv1
// sp + 16: argv0
// sp + 8:  actualArgc
// sp:      codeAddress
// construct Native Leave Frame:
//   +--------------------------+
//   |       argv0              | calltarget , newTarget, this, ....
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

// uint64_t CallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
// webkit_jscc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %rax - glue
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
//  sp - 8 : pc
//  sp - 16: rbp <---------current rbp & current sp
//  current sp - 8:  type

CallRuntime:
    pushq   %rbp
    movq    %rsp, %rbp     // set frame pointer
    movq    %rbp, ASM_GLUE_LEAVE_FRAME_OFFSET(%rax)   // save to thread->currentFrame_
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

// uint64_t JSCallWithArgV(uintptr_t glue, uint32_t argc, JSTaggedType calltarget, uintptr_t argv[]);
// c++ calling convention call js function
// Input:
// %rdi - glue
// %rsi - argc
// %rdx - calltarget
// %rcx - argV (calltarget, newtarget, thisObj, ...)
.global JSCallWithArgV
.type JSCallWithArgV, %function


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

JSCallWithArgV:
    ret
JSCall:
    ret

// ==================================================================================
// void PushCallArgsxAndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, ...);
// GHC calling convention
// Input1: for callarg0/1/2/3         Input2: for callrange
// %r13 - glue                        // %r13 - glue
// %rbp - function                    // %rbp - function
// %r12 - sp                          // %r12 - sp
// %rbx - method                      // %rbx - method
// %r14 - callField                   // %r14 - callField
// %rsi - arg0                        // %rsi - actualArgc
// %rdi - arg1                        // %rdi - argv
// %r8  - arg2
// ---------------------------------------------------------------------------------
// void PushCallIThisRangeAndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint32_t actualArgc, uintptr_t argv);
PushCallIThisRangeAndDispatch:
    movq    $JUMP_SIZE_PREF_IMM16_V8, %rdx
    movq    %rsp, %r10  // fp
    jmp     .CallIThisRangeEntry

// void PushCallIRangeAndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint32_t actualArgc, uintptr_t argv);
PushCallIRangeAndDispatch:
    movq    $JUMP_SIZE_PREF_IMM16_V8, %rdx
    movq    %rsp, %r10  // fp
    jmp     .CallIRangeEntry

// void PushCallArgs3AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0, uint64_t arg1, uint64_t arg2);
PushCallArgs3AndDispatch:
    movq    $JUMP_SIZE_PREF_V8_V8_V8_V8, %rdx
    movq    %rsp, %r10  // fp
    jmp     .Callargs3Entry

// void PushCallArgs2AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0, uint64_t arg1);
PushCallArgs2AndDispatch:
    movq    $JUMP_SIZE_PREF_V8_V8_V8, %rdx
    movq    %rsp, %r10  // fp
    jmp     .Callargs2Entry

// void PushCallArgs1AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0);
PushCallArgs1AndDispatch:
    movq    $JUMP_SIZE_PREF_V8_V8, %rdx
    movq    %rsp, %r10  // fp
    jmp     .Callarg1Entry

// void PushCallArgs0AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField);
PushCallArgs0AndDispatch:
    movq    $JUMP_SIZE_PREF_V8, %rdx
    movq    %rsp, %r10  // fp
    jmp     .PushCallThisUndefined   // Callargs0Entry

// ==============================================================================
// void PushCallArgsxAndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, ...);
// GHC calling convention
// Input1: for callarg0/1/2/3         Input2: for callrange
// %r13 - glue                        // %r13 - glue
// %rbp - function                    // %rbp - function
// %r12 - sp                          // %r12 - sp
// %rbx - method                      // %rbx - method
// %r14 - callField                   // %r14 - callField
// %rsi - arg0                        // %rsi - actualArgc
// %rdi - arg1                        // %rdi - argv
// %r8  - arg2
// ---------------------------------------------------------------------------------
.macro DISPATCH_SLOW_PATH EXTRA_ENTRY, PUSH_ARGS_NO_EXTRA_ENTRY, PUSH_ARGS_ENTRY, UNDEFINED1, UNDEFINED2
    movq    %rsp, %r10  // fp
    movq    %r14, %r11
    shrq    $ASM_JS_METHOD_NUM_ARGS_START_BIT, %r11
    andq    $ASM_JS_METHOD_NUM_ARGS_MASK, %r11  // declaredNumArgs
    testq   $ASM_JS_METHOD_HAVE_EXTRA_MASK, %r14
    jnz     \EXTRA_ENTRY
    movq    %r11, %r15
    subq    %rax, %r15  // declaredNumArgs - actualNumArgs
    cmpq    $0, %r15
    jle     \PUSH_ARGS_NO_EXTRA_ENTRY

\UNDEFINED1:
    pushq   $JSUNDEFINED
    subq    $1, %r15
    cmpq    $0, %r15
    ja      \UNDEFINED1
    jmp     \PUSH_ARGS_NO_EXTRA_ENTRY

\EXTRA_ENTRY:
    pushq   %rax  // actualArgc
    movq    %r11, %r15
    subq    %rax, %r15  // declaredNumArgs - actualNumArgs
    cmpq    $0, %r15
    jle     \PUSH_ARGS_ENTRY

\UNDEFINED2:
    pushq   $JSUNDEFINED
    subq    $1, %r15
    cmpq    $0, %r15
    ja      \UNDEFINED2
    jmp     \PUSH_ARGS_ENTRY
.endm

// void PushCallIThisRangeAndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint32_t actualArgc, uintptr_t argv);
PushCallIThisRangeAndDispatchSlowPath:
    movq    $JUMP_SIZE_PREF_IMM16_V8, %rdx
    movq    %rsi, %rax
    DISPATCH_SLOW_PATH .PushCallIThisRangeWithExtra, .CallIThisRangeNoExtraEntry, .CallIThisRangeEntry, \
        .CallIThisRange, .CallIThisRangeNoExtra

// void PushCallIRangeAndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint32_t actualArgc, uintptr_t argv);
PushCallIRangeAndDispatchSlowPath:
    movq    $JUMP_SIZE_PREF_IMM16_V8, %rdx
    movq    %rsi, %rax
    DISPATCH_SLOW_PATH .PushCallIRangeWithExtra, .CallIRangeNoExtraEntry, .CallIRangeEntry, .CallIRange, \
        .CallIRangeNoExtra

// void PushCallArgs3AndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0, uint64_t arg1, uint64_t arg2);
PushCallArgs3AndDispatchSlowPath:
    movq    $JUMP_SIZE_PREF_V8_V8_V8_V8, %rdx
    movq    $3, %rax  // actualArgc
    DISPATCH_SLOW_PATH .PushCallArgs3WithExtra, .Callargs3NoExtraEntry, .Callargs3Entry, .Call3, .Call3NoExtra

// void PushCallArgs2AndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0, uint64_t arg1);
PushCallArgs2AndDispatchSlowPath:
    movq    $JUMP_SIZE_PREF_V8_V8_V8, %rdx
    movq    $2, %rax  // actualArgc
    DISPATCH_SLOW_PATH .PushCallArgs2WithExtra, .Callargs2NoExtraEntry, .Callargs2Entry, .Call2, .Call2NoExtra

// void PushCallArgs1AndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0);
PushCallArgs1AndDispatchSlowPath:
    movq    $JUMP_SIZE_PREF_V8_V8, %rdx
    movq    $1, %rax  // actualArgc
    DISPATCH_SLOW_PATH .PushCallArgs1WithExtra, .Callargs1NoExtraEntry, .Callarg1Entry, .Call1, .Call1NoExtra

// void PushCallArgs0AndDispatchSlowPath(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField);
PushCallArgs0AndDispatchSlowPath:
    movq    $JUMP_SIZE_PREF_V8, %rdx
    movq    $0, %rax  // actualArgc
    DISPATCH_SLOW_PATH .PushCallArgs0WithExtra, .Callargs0NoExtraEntry, .PushCallThisUndefined, .Call0, .Call0NoExtra

.CallIThisRangeNoExtraEntry:
    cmpq    %r11, %rax
    jae     .PushLoopPrepareNoExtra1
    movq    %rax, %r11

.PushLoopPrepareNoExtra1:
    cmpq    $0, %r11
    jbe     .PushCallThis   // skip push args

.PushLoopNoExtra:
    movq    -8(%rdi, %r11, 8), %r8
    pushq   %r8
    subq    $1, %r11
    cmpq    $0, %r11
    ja      .PushLoopNoExtra
    jmp     .PushCallThis

.CallIRangeNoExtraEntry:
    cmpq    %r11, %rax
    jae     .PushLoopPrepareNoExtra2
    movq    %rax, %r11

.PushLoopPrepareNoExtra2:
    cmpq    $0, %r11
    jbe     .PushCallThisUndefined   // skip push args

.PushLoopNoExtra2:
    movq    -8(%rdi, %r11, 8), %r8
    pushq   %r8
    subq    $1, %r11
    cmpq    $0, %r11
    ja      .PushLoopNoExtra2
    jmp     .PushCallThisUndefined

.Callargs3NoExtraEntry:
    cmpq    $3, %r11
    jb      .Callargs2NoExtraEntry
    pushq   %r8  // arg2

.Callargs2NoExtraEntry:
    cmpq    $2, %r11
    jb      .Callargs1NoExtraEntry
    pushq   %rdi  // arg1

.Callargs1NoExtraEntry:
    cmpq    $1, %r11
    jb      .Callargs0NoExtraEntry
    pushq   %rsi  // arg0

.Callargs0NoExtraEntry:
    jmp     .PushCallThisUndefined

// ---------------------------------------------------------------------------------
.CallIThisRangeEntry:
    movq    %rsi, %r11
    cmpq    $0, %r11
    jbe     .PushCallThis   // actualArgc is 0, skip push args

.PushLoop1:
    movq    -8(%rdi, %r11, 8), %r8
    pushq   %r8
    subq    $1, %r11
    cmpq    $0, %r11
    ja      .PushLoop1

.PushCallThis:
    testq   $ASM_JS_METHOD_CALL_TYPE_MASK, %r14
    jz      .PushVregs
    testq   $ASM_JS_METHOD_HAVE_THIS_MASK, %r14
    jz      .PushNewTarget
    movq    -8(%rdi), %r11
    pushq   %r11     // this: this is before the argv list
    jmp     .PushNewTarget

.CallIRangeEntry:
    movq    %rsi, %r11
    cmpq    $0, %r11
    jbe     .PushCallThisUndefined   // actualArgc is 0, skip push args

.PushLoop2:
    movq    -8(%rdi, %r11, 8), %r8
    pushq   %r8
    subq    $1, %r11
    cmpq    $0, %r11
    ja      .PushLoop2
    jmp     .PushCallThisUndefined

.Callargs3Entry:
    pushq   %r8   // arg2

.Callargs2Entry:
    pushq   %rdi  // arg1

.Callarg1Entry:
    pushq   %rsi  // arg0

.PushCallThisUndefined:
    testq   $ASM_JS_METHOD_CALL_TYPE_MASK, %r14
    jz      .PushVregs
    testq   $ASM_JS_METHOD_HAVE_THIS_MASK, %r14
    jz      .PushNewTarget
    pushq   $JSUNDEFINED  // this

.PushNewTarget:
    testq   $ASM_JS_METHOD_HAVE_NEW_TARGET_MASK, %r14
    jz      .PushFunction
    pushq   $JSUNDEFINED  // new_target

.PushFunction:
    testq   $ASM_JS_METHOD_HAVE_FUNC_MASK, %r14
    jz      .PushVregs
    pushq   %rbp  // function

.PushVregs:
    movq    %r14, %r11
    shrq    $ASM_JS_METHOD_NUM_VREGS_START_BIT, %r11
    andq    $ASM_JS_METHOD_NUM_VREGS_MASK, %r11  // numVregs
    jz      .PushFrameState

.InitVregs:
    pushq   $JSUNDEFINED
    subq    $1, %r11
    jnz     .InitVregs

.PushFrameState:
    movq    %rsp, %r15  // newSp
    // skip stack overflow check now
    leaq    -ASM_INTERPRETED_FRAME_STATE_SIZE(%r12), %r11
    movq    %rdx, ASM_INTERPRETED_FRAME_CALL_SIZE_OFFSET(%r11) // lazy storage of jumpSizeAfterCall
    pushq   $ASM_INTERPRETER_FRAME_TYPE  // frame type
    pushq   %r12                    // prev_sp
    movq    ASM_JS_METHOD_BYTECODEARRAY_OFFSET(%rbx), %rcx
    pushq   %rcx                    // pc
    pushq   %r10                    // fp
    pushq   $0                      // reserved for jumpSizeAfterCall
    pushq   %r11                    // env
    pushq   $JSHOLE                 // acc
    pushq   %rbp                    // function

.Align:
    testq   $15, %rsp  // low 4 bits must be 0b0000
    jnz      .PrepareDispatch
    pushq   $0

.PrepareDispatch:
    movq    ASM_JS_METHOD_HOTNESS_COUNTER_OFFSET(%rbx), %rdi      // hotness_counter:arg6 of dispatch
    movq    $JSHOLE, %rsi                                         // acc: arg5 of dispatch
    movq    ASM_JF_FUNCTION_PROFILE_TYPE_INFO_OFFSET(%rbp), %r14  // profile_type_info:arg4 of dispatch
    movq    ASM_JS_FUNCTION_CONSTANT_POOL_OFFSET(%rbp), %rbx      // constant_pool: arg3 of dispatch
    movq    %r15, %rbp                                            // sp: arg1 of dispatch
    movq    %rcx, %r12                                            // pc: arg2 of dispatch
                                                                  // glue: arg0 of dispatch

.Dispatch:
    movzbq  (%rcx), %rax
    movq    ASM_GLUE_BC_HANDLERS_OFFSET(%r13, %rax, 8), %r11
    jmp     *%r11

// ================================================================================
// void PushCallArgsxAndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, ...);
// c++ calling convention: callee-save is now GHC calling convention
// Input1: for callarg0/1/2/3         Input2: for callrange
// %rdi - glue                        // %rdi - glue
// %rsi - function                    // %rsi - function
// %rdx - sp                          // %rdx - sp
// %rcx - method                      // %rcx - method
// %r8  - actualArgc                  // %r8  - actualArgc
// %r9  - arg0                        // %r9  - argv
// sp+8 - arg2
// sp   - arg1
// ---------------------------------------------------------------------------------
.macro PUSH_GHC_CALLEE_SAVED_REGISTERS
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r15
.endm

.macro  POP_GHC_CALLEE_SAVED_REGISTERS
    popq    %r15
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
.endm

PushCallIRangeAndDispatchNative:
    ret

PushCallArgsAndDispatchNative:
    ret

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t sp, uintptr_t pc, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// %r13 - glue
// %rbp - sp
// %r12 - pc
// %rbx - constantPool
// %r14 - profileTypeInfo
// %rsi - acc
// %rdi - hotnessCounter
// %r8  - jumpSizeAfterCall
ResumeRspAndDispatch:
    movq    %rbp, %r11
    subq    $ASM_INTERPRETED_FRAME_STATE_SIZE, %r11
    movq    ASM_INTERPRETED_FRAME_FP_OFFSET(%r11), %r10
    movq    %r10, %rsp                                    // resume rsp as the value before call or execute
    movq    ASM_INTERPRETED_FRAME_PREV_SP_OFFSET(%r11), %rbp

    addq    %r8, %r12     // newPc
    movzbq  (%r12), %rax  // opcode
    movq    ASM_GLUE_BC_HANDLERS_OFFSET(%r13, %rax, 8), %r11
    jmp     *%r11

// ResumeRspAndReturn(uintptr_t glue, uintptr_t sp)
// GHC calling convention
// %r13 - glue
// %rbp - sp
ResumeRspAndReturn:
    retq

ResumeCaughtFrameAndDispatch:
    retq

ResumeUncaughtFrameAndReturn:
    retq

// uint64_t CallRuntimeWithArgv(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t argv);
// cc calling convention call runtime_id's runtion function(c-abi)
// JSTaggedType (*)(uintptr_t argGlue, uint64_t argc, JSTaggedType argv[])
// Input:
// %rdi - glue
// %rsi - runtime_id
// %edx - argc
// %rcx - argv
// stack layout:
//   +--------------------------+
//   |       return addr        |
//   +--------------------------+

// %r8  - argV
// %r9  - codeAddr

// Output:
// construct Leave Frame:
//   +--------------------------+
//   |       returnAddr         |
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

CallRuntimeWithArgv:
    movq %rsp, %r8
    movq (%rsp), %r9
    pushq  %rcx  // argv[]
    pushq  %rdx  // argc
    pushq  %rsi  // RuntimeId
    pushq  %r9   // returnAddr

    // construct leave frame
    pushq   %rbp
    movq    %rsp, %rbp     // set frame pointer
    movq    %rbp, ASM_GLUE_LEAVE_FRAME_OFFSET(%rdi)   // save to thread->leaveFrame_
    pushq   $ASM_LEAVE_FRAME_WITH_ARGV

    movq    ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET(%rdi, %rsi, 8), %r9
    movq    %rdx, %rsi // argc
    movq    %rcx, %rdx // argv
    pushq   %r8
    call    *%r9
    popq    %r8
    addq    $8, %rsp // skip type
    popq    %rbp
    movq    %r8, %rsp
    ret

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

AsmInterpreterEntry:
    ret

GeneratorReEnterAsmInterp:
    ret

JSCallDispatch:
    ret
