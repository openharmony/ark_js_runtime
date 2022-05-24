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
