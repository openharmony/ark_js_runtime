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

.global OptimizedCallRuntime
.type OptimizedCallRuntime, %function

.global OptimizedCallRuntimeWithArgv
.type OptimizedCallRuntimeWithArgv, %function

.global AsmIntCallRuntime
.type AsmIntCallRuntime, %function

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

.global PushCallIThisRangeAndDispatchNative
.type PushCallIThisRangeAndDispatchNative, %function

.global PushCallIRangeAndDispatchNative
.type PushCallIRangeAndDispatchNative, %function

.global PushCallArgs3AndDispatchNative
.type PushCallArgs3AndDispatchNative, %function

.global PushCallArgs2AndDispatchNative
.type PushCallArgs2AndDispatchNative, %function

.global PushCallArgs1AndDispatchNative
.type PushCallArgs1AndDispatchNative, %function

.global PushCallArgs0AndDispatchNative
.type PushCallArgs0AndDispatchNative, %function

.global ResumeRspAndDispatch
.type ResumeRspAndDispatch, %function

.global ResumeRspAndReturn
.type ResumeRspAndReturn, %function

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
    PUSH_GENERAL_REGS
    // construct the frame
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    pushq   $JS_ENTRY_FRAME_TYPE    // optimized entry frame type
    pushq   %rsi                    // prev managed fp

    // 16 bytes align check
    movl    %edx, %r14d
    testb   $1, %r14b
    jne     .LAlign16Bytes
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

.LCopyLoop:
    movq    -8(%r8, %rax, 8), %rcx
    pushq   %rcx

    addq    $-1, %rax
    jne     .LCopyLoop

    push    %r14
    movq    %rdi, %rax          // move glue to rax
    call    *%r9                // then call jsFunction
    leaq    (,%r14,8), %rcx   // NOTE: fixed for 3 extra arguments
    addq    %rcx, %rsp
    addq    $8, %rsp
    testb   $1, %r14b           // stack 16bytes align check
    jne     .LPopFrame
    addq    $8, %rsp

.LPopFrame:
    popq    %rsi           // pop prev-managed frame
    addq    $8, %rsp
    popq    %rbp

    POP_GENERAL_REGS
    movq    %rsi, ASM_GLUE_LEAVE_FRAME_OFFSET(%rdi)   // restore thread->leaveFrame_
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
    pushq   %rbp
    pushq   $OPTIMIZE_FRAME_TYPE
    //callee save
    pushq   %r14
    pushq   %rbx
    pushq   %rax
    
    // 16 bytes align check
    movl    %esi, %r14d
    testb   $1, %r14b
    jne      .LAlign16Bytes1
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

.LCopyLoop1:
    movq    -8(%r8, %rax, 8), %rbx
    pushq   %rbx

    addq    $-1, %rax
    jne     .LCopyLoop1

    pushq   %rdx                // actual argc
    movq    %rdi, %rax          // move glue to rax
    callq   *%rcx               // then call jsFunction
    leaq    (,%r14,8), %rcx
    addq    %rcx, %rsp
    addq    $8, %rsp

    testb   $1, %r14b           // stack 16bytes align check
    jne      .LPopFrame1
    addq    $8, %rsp

.LPopFrame1:
    addq    $8, %rsp
    popq    %rbx
    popq    %r14
    addq    $8, %rsp
    pop     %rbp
    ret

// uint64_t CallNativeTrampoline(uintptr_t glue, uintptr_t codeAddress, uint32_t argc, ...);
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

.global CallNativeTrampoline
.type CallNativeTrampoline, %function
CallNativeTrampoline:
    pushq   %rbp
    movq    %rsp, %rbp     // set frame pointer
    movq    %rbp, ASM_GLUE_LEAVE_FRAME_OFFSET(%rax)   // save to thread->leaveFrame_
    pushq   $LEAVE_FRAME_TYPE

    // callee save
    pushq    %r10
    pushq    %rbx

    movq    %rbp, %rdx
    addq    $16, %rdx        // 16 : for rbp & return address

    // load native pointer address
    movq    (%rdx), %r10
    subq    $8, %rsp         // align 16bytes
    subq    $ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE, %rsp

    // construct ecma_runtime_call_info    
    // get thread
    subq    $ASM_GLUE_TO_THREAD_OFFSET, %rax
    movq    %rax, (%rsp)                                         // thread_
    // get numArgs
    movq    $0, %rax
    movl    8(%rdx), %eax
    subl    $3, %eax
    movq    %rax, ECMA_RUNTIME_CALLINFO_NUMARGS_OFFSET(%rsp)     // numArgs
    // get gpr data
    movq    %rdx, %rbx
    addq    $16, %rbx
    movq    %rbx, ECMA_RUNTIME_CALLINFO_STACKARGS_OFFSET(%rsp)
    movq    $0,  ECMA_RUNTIME_CALLINFO_DATA_OFFSET(%rsp)
    movq    $0,  ECMA_RUNTIME_CALLINFO_PPREV_SP_OFFSET(%rsp)

    movq   %rsp, %rdi
    call   *%r10
    addq   $ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE, %rsp
    addq   $8, %rsp         // align 16bytes
    popq   %rbx
    popq   %r10
    addq   $8, %rsp
    popq   %rbp
    popq   %rdx        // load pc
    addq   $8, %rsp    // skip code address
    pushq   %rdx        // save pc
    ret 

// uint64_t OptimizedCallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
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

OptimizedCallRuntime:
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

AsmIntCallRuntime:
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

    popq    %r10
    movq    $0, ASM_GLUE_LEAVE_FRAME_OFFSET(%r10)   // save nullptr to thread->leaveFrame_
    popq    %rdx
    popq    %r10

    addq    $8, %rsp
    addq    $8, %rsp
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
//   |       argc               | OptimizedLeaveFrame
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
    mov     %rdx, (%rcx)
    mov     %rdx, %rax
    jmp     .LJSCallStart
JSCall:
    mov     %rax, %rdi
    mov     16(%rsp), %rax               // get jsfunc
.LJSCallStart:
    movabs  $TAGGED_MASK, %rdx           // IsTaggedInt
    and     %rax, %rdx
    cmp     $0x0, %rdx
    jne     .LNonCallable
    cmp     $0x0, %rax                   // IsHole
    je      .LNonCallable
    movabs  $TAGGED_SPECIAL_VALUE, %rdx
    and     %rax, %rdx                   // IsSpecial
    cmp     $0x0, %rdx
    jne     .LNonCallable

    mov     %rax, %r8               // save jsfunc
    mov     (%rax), %rax            // get jshclass
    movl    JS_HCLASS_BITFIELD_OFFSET(%rax), %eax
    btl     $JS_HCLASS_BITFIELD_CALLABLE_BIT, %eax  // IsCallable
    jnb     .LNonCallable

    shll    $24, %eax               // objecttype << 24
    leal    -50331649(%rax), %edx
    cmpl    $0x9FFFFFF, %edx
    jae     .LNotJSFunction         // objecttype in (0x04 ~ 0x0c)
    jmp     .LJSFunctionCall
.LNotJSFunction:
    cmpl    $0xd000000, %eax        // IsJsBoundFunction
    je      .LJSBoundFunction
    cmpl    $0x4f000000, %eax       // IsJsProxy
    je      .LJSProxy

.LNonCallable:
    pushq   %rbp
    movq    %rsp, %rbp                          // set frame pointer
    pushq   $OPTIMIZE_FRAME_TYPE                // set frame type
    mov     $MESSAGE_STRING_NON_CALLABLE_ID, %rax
    pushq   %rax                                //message id
    pushq   $1                                  //argc
    pushq   $RUNTIME_ID_THROW_TYPE_ERROR        //runtime id
    mov     %rcx, %rax        // glue
    call    OptimizedCallRuntime
    movabs  $TAGGED_VALUE_EXCEPTION, %rax       // reteurn exception
    addq    $32, %rsp
    pop     %rbp
    ret

//  %rdi -- glue
//  %r8  -- jsfunc
.LJSFunctionCall:
    mov     JS_FUNCTION_METHOD_OFFSET(%r8), %rsi    // get method
    movl    8(%rsp), %edx                            // Actual argc
    mov     JS_METHOD_CALLFIELD_OFFSET(%rsi), %rax   // get call field
    btq     $JS_METHOD_CALL_FIELD_NATIVE_BIT, %rax   // is native
    jb      .LCallNativeMethod
    btq     $JS_METHOD_CALL_FIELD_AOT_BIT, %rax      // is aot
    jb      .LCallOptimziedMethod
    // todo: need to fix it                   // is bytecode
    int3
    ret

.LCallOptimziedMethod:
    mov     JS_FUNCTION_CODE_ENTRY_OFFSET(%r8), %rcx  // get codeaddress
    shr     $JS_METHOD_NUM_ARGS_SHIFT, %rax
    andl    $ASM_JS_METHOD_NUM_ARGS_MASK, %eax
    addl    $ASM_NUM_MANDATORY_JSFUNC_ARGS, %eax      // add mandatory arguments
    mov     %eax, %esi      // expected numbers
    movq    %rsp,  %r8
    addq    $16,   %r8      // r8 argv
    cmpl    %esi, %edx       // expectedNumArgs <= actualNumArgs
    jg      .LDirectCallCodeEntry
    jmp     OptimizedCallOptimized
.LDirectCallCodeEntry:
    mov     %rdi, %rax
    jmp     *%rcx

.LCallNativeMethod:
    pop     %rax                                    // pc
    mov     JS_FUNCTION_METHOD_OFFSET(%r8), %rsi    // get method
    mov     JS_METHOD_NATIVE_POINTER_OFFSET(%rsi), %rsi  // get native pointer
    push    %rsi                                    // native code address
    push    %rax                                    // pc
    mov     %rdi,  %rax
    jmp     CallNativeTrampoline

//  input
//  %rdi -- glue
//  %r8  -- jsfunc
.LJSBoundFunction:
    // construct frame
    pushq   %rbp
    pushq   $OPTIMIZE_FRAME_TYPE                // set frame type
    
    // callee save
    pushq   %r10
    
    mov     %rsp, %rdx
    addq    $32,  %rdx
    mov     (%rdx), %rax        // get origin argc
    mov     %rax, %r10

    // get bound target
    mov     JS_FUNCTION_BOUND_ARG_OFFSET(%r8), %rcx
    // get bound length
    mov     TAGGED_ARRAY_LENGTH_OFFSET(%rcx), %rcx
    addq    %rcx, %r10

     // 16 bytes align check
    testb   $1, %r10b
    jne      .LAlign16Bytes2
    pushq   $0                  // push zero to align 16 bytes stack   

.LAlign16Bytes2:
    subq    $ASM_NUM_MANDATORY_JSFUNC_ARGS, %rax
    cmp     $0, %rax
    je      .LCopyBoundArgument
.LCopyArgument2:
    movq    24(%rdx, %rax, 8), %rcx
    pushq   %rcx

    addq    $-1, %rax
    jne     .LCopyArgument2

    // get bound target
    mov     JS_FUNCTION_BOUND_ARG_OFFSET(%r8), %rdx
    // get bound length
    mov     TAGGED_ARRAY_LENGTH_OFFSET(%rdx), %rax
    addq    $TAGGED_ARRAY_DATA_OFFSET, %rdx
.LCopyBoundArgument:
    cmp     $0, %rax
    je      .LPushCallTarget
.LCopyBoundArgumentLoop:
    addq    $-1, %rax
    movq    (%rdx, %rax, 8), %rcx
    pushq   %rcx
    jne     .LCopyBoundArgumentLoop
.LPushCallTarget:
    mov     JS_FUNCTION_BOUND_THIS_OFFSET(%r8), %rax     // thisObj
    pushq   %rax
    pushq   $JSUNDEFINED                                         // newtarget
    mov     JS_FUNCTION_BOUND_TARGET_OFFSET(%r8), %rax   // calltarget
    pushq   %rax
    pushq   %r10                                         // push actual arguments
    mov     %rdi, %rax
    call    JSCall
    leaq    8(, %r10, 8), %rcx
    addq    %rcx, %rsp

    testb   $1, %r10b           // stack 16bytes align check
    jne     .LPopFrame2
    addq    $8, %rsp

.LPopFrame2:
    pop     %r10
    addq    $8, %rsp
    pop     %rbp
    ret
.LJSProxy:
    mov     %rsp, %rcx
    addq    $8,  %rcx
    mov     (%rcx), %rsi        // get origin argc
    mov     %r8, %rdx           // calltarget
    addq    $8,  %rcx           // argv

    movq    $JSPROXY_CALL_INTERNAL_INDEX, %r9
    movq    ASM_GLUE_COMSTUB_ENTRY_OFFSET(%rdi, %r9, 8), %r8
    jmp     *%r8
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
    jmp     .CallIThisRangeEntry

// void PushCallIRangeAndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint32_t actualArgc, uintptr_t argv);
PushCallIRangeAndDispatch:
    movq    $JUMP_SIZE_PREF_IMM16_V8, %rdx
    jmp     .CallIRangeEntry

// void PushCallArgs3AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0, uint64_t arg1, uint64_t arg2);
PushCallArgs3AndDispatch:
    movq    $JUMP_SIZE_PREF_V8_V8_V8_V8, %rdx
    jmp     .Callargs3Entry

// void PushCallArgs2AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0, uint64_t arg1);
PushCallArgs2AndDispatch:
    movq    $JUMP_SIZE_PREF_V8_V8_V8, %rdx
    jmp     .Callargs2Entry

// void PushCallArgs1AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField, uint64_t arg0);
PushCallArgs1AndDispatch:
    movq    $JUMP_SIZE_PREF_V8_V8, %rdx
    jmp     .Callarg1Entry

// void PushCallArgs0AndDispatch(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t callField);
PushCallArgs0AndDispatch:
    movq    $JUMP_SIZE_PREF_V8, %rdx
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
    pushq   $ASM_INTERPRETER_FRAME  // frame type
    pushq   %r12                    // prev_sp
    movq    ASM_JS_FUNCTION_LEXICAL_ENV_OFFSET(%rbp), %r11
    pushq   %r11                    // env
    pushq   $0                      // acc
    pushq   %rbp                    // function
    pushq   $0                      // reserved for jumpSizeAfterCall
    movq    ASM_JS_METHOD_BYTECODEARRAY_OFFSET(%rbx), %rcx
    pushq   %rcx                    // pc

.Align:
    testq   $15, %rsp  // low 4 bits must be 0b0000
    jnz      .PrepareDispatch
    pushq   $0

.PrepareDispatch:
    movq    ASM_JS_METHOD_HOTNESS_COUNTER_OFFSET(%rbx), %rdi      // hotness_counter:arg6 of dispatch   
    movq    $JSHOLE, %rsi                                         // acc: arg5 of dispatch
    movq    ASM_JF_FUNCTION_PROFILE_TYPE_INFO_OFFSET(%rbp), %r14  // profile_type_info:arg4 of dispatch
    movq    ASM_JS_FUNCTION_CONSTANT_POOL_OFFSET(%rbp), %rbx      // constant_pool: arg3 of dispatch
    movq    %r15, %r12                                            // sp: arg2 of dispatch
    movq    %rcx, %rbp                                            // pc: arg1 of dispatch
                                                                  // glue: arg0 of dispatch
    movq    %r15, ASM_GLUE_CURRENT_FRAME_OFFSET(%r13)  // save newSp to thread->current_sp

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
    pushq   %r12
    pushq   %r13
    pushq   %r15
.endm

.macro  POP_GHC_CALLEE_SAVED_REGISTERS
    popq    %r15
    popq    %r13
    popq    %r12
.endm

// void PushCallIThisRangeAndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t actualArgc, uintptr_t argv);
PushCallIThisRangeAndDispatchNative:
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    PUSH_GHC_CALLEE_SAVED_REGISTERS
    testb   $1, %r8b
    jnz     .AlignedCallIThisRangeNative
    pushq   $0
.AlignedCallIThisRangeNative:
    jmp     .CallIThisRangeNativeEntry

// void PushCallIRangeAndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t actualArgc, uintptr_t argv);
PushCallIRangeAndDispatchNative:
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    PUSH_GHC_CALLEE_SAVED_REGISTERS
    testb   $1, %r8b
    jnz     .AlignedCallIRangeNative
    pushq   $0
.AlignedCallIRangeNative:
    jmp     .CallIRangeNativeEntry

// void PushCallArgs3AndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t actualArgc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
PushCallArgs3AndDispatchNative:
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    PUSH_GHC_CALLEE_SAVED_REGISTERS
    // aligned alreay
    jmp     .Callargs3NativeEntry

// void PushCallArgs2AndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t actualArgc, uint64_t arg0, uint64_t arg1);
PushCallArgs2AndDispatchNative:
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    PUSH_GHC_CALLEE_SAVED_REGISTERS
    // aligned alreay
    jmp     .Callargs2NativeEntry

// void PushCallArgs1AndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t actualArgc, uint64_t arg0);
PushCallArgs1AndDispatchNative:
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    PUSH_GHC_CALLEE_SAVED_REGISTERS
    // aligned alreay
    jmp     .Callarg1NativeEntry

// void PushCallArgs0AndDispatchNative(uintptr_t glue, uint64_t function,
//     uintptr_t sp, uintptr_t method, uint64_t actualArgc);
PushCallArgs0AndDispatchNative:
    pushq   %rbp
    movq    %rsp, %rbp              // set frame pointer
    PUSH_GHC_CALLEE_SAVED_REGISTERS
    pushq   $0  // 16 bytes align
    jmp     .PushCallThisUndefinedNative
// ----------------------------------------------------------------------------
.CallIThisRangeNativeEntry:
    movq    %r8, %r11
    cmpq    $0, %r11
    jbe     .PushCallThisNative   // actualArgc is 0, skip push args

.PushLoopNative1:
    movq    -8(%rdx, %r11, 8), %r10
    pushq   %r10
    subq    $1, %r11
    cmpq    $0, %r11
    ja      .PushLoopNative1

.PushCallThisNative:
    movq    -8(%r9), %r11
    pushq   %r11     // this: this is before the argv list
    jmp     .PushLeftFrameNative

.CallIRangeNativeEntry:
    movq    %r8, %r11
    cmpq    $0, %r11
    jbe     .PushCallThisUndefinedNative   // actualArgc is 0, skip push args

.PushLoopNative2:
    movq    -8(%rdx, %r11, 8), %r10
    pushq   %r10
    subq    $1, %r11
    cmpq    $0, %r11
    ja      .PushLoopNative2
    jmp     .PushCallThisUndefinedNative

.Callargs3NativeEntry:
    movq    24(%rbp), %r10
    pushq   %r10   // arg2

.Callargs2NativeEntry:
    movq    16(%rbp), %r10
    pushq   %r10  // arg1

.Callarg1NativeEntry:
    pushq   %r9   // arg0

.PushCallThisUndefinedNative:
    pushq   $JSUNDEFINED  // this

.PushLeftFrameNative:
    pushq   $JSUNDEFINED  // new_target
    pushq   %rsi          // function

.PushFrameStateNative:
    movq    %rsp, %r15  // newSp
    // skip stack overflow check now
    pushq   $ASM_INTERPRETER_FRAME  // frame type
    pushq   %rdx                    // prev_sp
    movq    ASM_JS_FUNCTION_LEXICAL_ENV_OFFSET(%rsi), %r11
    pushq   %r11                    // env
    pushq   $0                      // acc
    pushq   %rsi                    // function
    pushq   $0                      // jumpSizeAfterCall
    pushq   $0                      // pc
    movq    %r15, ASM_GLUE_CURRENT_FRAME_OFFSET(%rdi)  // save newSp to thread->current_sp

.CallNative:
    // load native pointer address
    movq    ASM_JS_METHOD_NATIVE_POINTER_OFFSET(%rcx), %r10
    subq    $8, %rsp                                               // align 16bytes
    subq    $ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE, %rsp

    // construct ecma_runtime_call_info
    movq    %rdi, %rax
    subq    $ASM_GLUE_TO_THREAD_OFFSET, %rax
    movq    %rax, (%rsp)                                           // thread_
    movq    %r8, ECMA_RUNTIME_CALLINFO_NUMARGS_OFFSET(%rsp)        // numArgs
    movq    %r15, ECMA_RUNTIME_CALLINFO_STACKARGS_OFFSET(%rsp)     // stackArgs_
    movq    $0, ECMA_RUNTIME_CALLINFO_DATA_OFFSET(%rsp)            // data_
    movq    $0, ECMA_RUNTIME_CALLINFO_PPREV_SP_OFFSET(%rsp)        // prevsp_

    leaq    (, %r8, 8), %r12  // store argc * 8 into callee-saved register
    movq    %rsp, %rdi  // &ecma_runtime_call_info
    call    *%r10
    // ingore exception check now
    // optimize these three add operations
    addq    $ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE, %rsp
    addq    $8, %rsp                                                // align 16bytes
    addq    $ASM_INTERPRETED_FRAME_STATE_SIZE, %rsp
    addq    %r12, %rsp  // add NUM_MANDATORY_JSFUNC_ARGS + argc * 8

    testq   $1, %rsp
    jz      .NO_NEED_TO_POP_ALIGNED
    subq    $8, %rsp

.NO_NEED_TO_POP_ALIGNED:
    POP_GHC_CALLEE_SAVED_REGISTERS
    popq    %rbp
    ret

// ResumeRspAndDispatch(uintptr_t glue, uintptr_t pc, uintptr_t sp, uintptr_t constantPool,
//     uint64_t profileTypeInfo, uint64_t acc, uint32_t hotnessCounter, size_t jumpSize)
// GHC calling convention
// %r13 - glue
// %rbp - pc
// %r12 - sp
// %rbx - constantPool
// %r14 - profileTypeInfo
// %rsi - acc
// %rdi - hotnessCounter
// %r8  - jumpSizeAfterCall
ResumeRspAndDispatch:
    movq    %r12, %rsp    // resume rsp
    addq    %r8, %rbp     // newPc
    movzbq  (%rbp), %rax  // opcode
    movq    ASM_GLUE_BC_HANDLERS_OFFSET(%r13, %rax, 8), %r11
    jmp     *%r11

// ResumeRspAndReturn(uintptr_t glue, uintptr_t sp)
// GHC calling convention
// %r13 - glue
// %rbp - sp
ResumeRspAndReturn:
    movq    %rbp, %rsp
    retq


// uint64_t OptimizedCallRuntimeWithArgv(uintptr_t glue, uint64_t runtime_id, uint64_t argc, uintptr_t argv);
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

OptimizedCallRuntimeWithArgv:
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
    pushq   $ASM_OPTIMIZED_WITH_ARGV_LEAVE_FRAME

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