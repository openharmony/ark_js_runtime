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
// %r0 - glue
// %r1 - prevFp
// %r2 - expectedNumArgs
// %r3 - actualNumArgs
// %r5  - argV
// %r12  - codeAddr
.global InvokeJSFunctionEntry
.type InvokeJSFunctionEntry, %function
InvokeJSFunctionEntry:
    push    {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    add     r11, sp, #28
    sub     sp, sp, #4
    mov     r10, r2
    cmp     r2, r3
    bls     .LCopyArguments
    mov     r6, #0
    mov     r8, #10
    mov     r2, r10
    mov     r4, #0
.LCopyExtraAument:
    subs    r2, r2, #1
    mov     r7, r6
    mov     r9, r6
    push    {r6, r8}

    sbc     r4, r4, #0
    subs    r5, r3, r2
    rscs    r5, r4, #0
    blo     .LCopyExtraAument
.LCopyArguments:
    ldr     r12, [r11, #12]
    cmp     r10, r3
    add     lr, r1, #8
    add     r2, r1, #16
    movls   r3, r10
    add     r3, r3, #3          // add 3 for calltarget newtarget this LCopyArguments
    ldr     r4, [r11, #8]
    mov     r7, #0
    add     r5, r4, r3, lsl #3
.LCopyLoop:                                @ =>This Inner Loop Header: Depth=1
    ldr     r6, [r5, #4]
    ldr     r8, [r5]
    sub     r5, r5, #8
    subs    r3, r3, #1
    mov     r9, r6
    push    {r6, r8}

    bne     .LCopyLoop

    mov     r2, r12
    bl      InvokeCompiledJSFunction
    mov     r2, #24
    add     r2, r2, r10, lsl #3
    add     sp, sp, r2
    sub     sp, r11, #28
    pop     {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx      lr

// uint64_t InvokeCompiledJSFunction
// Input:
// %r0 - glue
// %r1 - prev fp
// %r2 - code address

.global InvokeCompiledJSFunction
.type InvokeCompiledJSFunction, %function
InvokeCompiledJSFunction:
    // construct the frame  
    push   {r11, lr}
    mov     lr, #1
    push    {lr, r1}            // js entry frame type, prev managerd fp
    add     r11, sp, #8
    mov     lr, r2
    blx     lr
    add     sp, sp, #8
    pop     {r11, lr}
    bx      lr


// uint64_t CallRuntimeTrampoline(uintptr_t glue, uint32_t runtime_id, uint32_t patch_id, uint32_t argc, ...);
// webkit_jscc calling convention

.global CallRuntimeTrampoline
.type CallRuntimeTrampoline, %function
CallRuntimeTrampoline:   // todo : need to rewrite
    push    {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    add     r11, sp, #28
    sub     sp, sp, #4
    bx lr
    add     sp, sp, #8
    pop     {r11, lr}
    bx      lr