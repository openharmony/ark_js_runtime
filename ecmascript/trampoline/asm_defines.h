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

#ifndef ECMASCRIPT_ASM_DEFINES_H
#define ECMASCRIPT_ASM_DEFINES_H

#ifdef PANDA_TARGET_64
#define ASM_GLUE_CURRENT_FRAME_OFFSET            (16)
#define ASM_GLUE_LEAVE_FRAME_OFFSET              (24)
#define ASM_GLUE_BC_HANDLERS_OFFSET              (32)
#define ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET        (2080)
#define ASM_BASE_FUNCTION_METHOD_OFFSET          (32)
#define ASM_JS_METHOD_CALLFIELD_OFFSET           (72)
#define ASM_JS_METHOD_BYTECODEARRAY_OFFSET       (88)
#define ASM_JS_FUNCTION_LEXICAL_ENV_OFFSET       (48)
#define ASM_JS_FUNCTION_CONSTANT_POOL_OFFSET     (72)
#define ASM_JF_FUNCTION_PROFILE_TYPE_INFO_OFFSET (80)
#define ASM_JS_METHOD_HOTNESS_COUNTER_OFFSET     (12)
#define ASM_JS_METHOD_NATIVE_POINTER_OFFSET      (32)
#ifdef PANDA_TARGET_ARM64
#define ASM_GLUE_TO_THREAD_OFFSET                (136)
#else
#ifdef  PANDA_USE_MUSL
#define ASM_GLUE_TO_THREAD_OFFSET                (136)
#else
#define ASM_GLUE_TO_THREAD_OFFSET                (136)
#endif
#endif
// ecma_runtime_callinfo struct in stack
// -----------------------------
// | JSTaggedType *prevSp_     |
// | void *       data_        |
// | JSTaggedType *stackArgs_  |
// -----------------------------
// | size_t     numArgs        |
// | JSThread*  thread         |
// -----------------------------
#define ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE         (40)
#define ECMA_RUNTIME_CALLINFO_NUMARGS_OFFSET        (8)
#define ECMA_RUNTIME_CALLINFO_STACKARGS_OFFSET      (16)
#define ECMA_RUNTIME_CALLINFO_DATA_OFFSET           (24)
#define ECMA_RUNTIME_CALLINFO_PPREV_SP_OFFSET       (32)

#define JS_METHOD_CALLFIELD_OFFSET                  (0x48)
#define JS_METHOD_NATIVE_POINTER_OFFSET             (32)
#endif

#ifdef PANDA_TARGET_32
#define ASM_GLUE_CURRENT_FRAME_OFFSET            (16)
#define ASM_GLUE_LEAVE_FRAME_OFFSET              (24)
#define ASM_GLUE_BC_HANDLERS_OFFSET              (32)
#define ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET        (1056)
#define ASM_BASE_FUNCTION_METHOD_OFFSET          (32)
#define ASM_JS_METHOD_CALLFIELD_OFFSET           (48)
#define ASM_JS_METHOD_BYTECODEARRAY_OFFSET       (60)
#define ASM_JS_FUNCTION_LEXICAL_ENV_OFFSET       (48)
#define ASM_JS_FUNCTION_CONSTANT_POOL_OFFSET     (72)
#define ASM_JF_FUNCTION_PROFILE_TYPE_INFO_OFFSET (80)
#define ASM_JS_METHOD_HOTNESS_COUNTER_OFFSET     (12)
#define ASM_JS_METHOD_NATIVE_POINTER_OFFSET      (24)
#define ASM_GLUE_TO_THREAD_OFFSET                (80)
// ecma_runtime_callinfo struct in stack
// -----------------------------
// | JSTaggedType *prevSp_     |
// | void *       data_        |
// | JSTaggedType *stackArgs_  |
// -----------------------------
// | size_t     numArgs        |
// | JSThread*  thread         |
// -----------------------------
#define ASM_GLUE_ECMA_RUNTIME_CALLINFO_SIZE         (20)
#define ECMA_RUNTIME_CALLINFO_NUMARGS_OFFSET        (4)
#define ECMA_RUNTIME_CALLINFO_STACKARGS_OFFSET      (8)
#define ECMA_RUNTIME_CALLINFO_DATA_OFFSET           (12)
#define ECMA_RUNTIME_CALLINFO_PPREV_SP_OFFSET       (16)

#define JS_METHOD_NATIVE_POINTER_OFFSET    (24)
#define JS_METHOD_CALLFIELD_OFFSET         (40)
#endif

#define OPTIMIZE_FRAME_TYPE                   (0)
#define JS_ENTRY_FRAME_TYPE                   (1)
#define LEAVE_FRAME_TYPE                      (3)
#define ASM_OPTIMIZED_WITH_ARGV_LEAVE_FRAME   (7)
#define ASM_LEAVE_FRAME_TYPE                  (5)
#define ASM_INTERPRETER_FRAME                 (3)
#define JSUNDEFINED                           (0xa)
#define JSHOLE                                (0x0)
#define ASM_JS_METHOD_NUM_VREGS_START_BIT     (4)
#define ASM_JS_METHOD_NUM_VREGS_BIT_SIZE      (28)
#define ASM_JS_METHOD_NUM_VREGS_MASK          (0xfffffff)  // ((0x1ULL << ASM_JS_METHOD_NUM_VREGS_BIT_SIZE) - 1)
#define ASM_JS_METHOD_NUM_ARGS_START_BIT      (32)
#define ASM_JS_METHOD_NUM_ARGS_BIT_SIZE       (28)
#define ASM_JS_METHOD_NUM_ARGS_MASK           (0xfffffff)  // ((0x1ULL << ASM_JS_METHOD_NUM_ARGS_BIT_SIZE) - 1)
#define ASM_JS_METHOD_HAVE_THIS_MASK          (0x1)
#define ASM_JS_METHOD_HAVE_NEW_TARGET_MASK    (0x2)
#define ASM_JS_METHOD_HAVE_EXTRA_MASK         (0x4)
#define ASM_JS_METHOD_HAVE_FUNC_MASK          (0x8)
#define ASM_JS_METHOD_IS_NATIVE_MASK          (0x1000000000000000)
#define ASM_JS_METHOD_CALL_TYPE_MASK          (0xf)

#define ASM_INTERPRETED_FRAME_STATE_SIZE (56)
#define ASM_INTERPRETED_FRAME_CALL_SIZE_OFFSET (8)
#define ASM_INTERPRETED_FRAME_FUNCTION_OFFSET  (16)
#define ASM_INTERPRETED_FRAME_ACC_OFFSET       (24)
#define ASM_INTERPRETED_FRAME_ENV_OFFSET       (32)

#define JUMP_SIZE_PREF_V8                      (3)
#define JUMP_SIZE_PREF_V8_V8                   (4)
#define JUMP_SIZE_PREF_V8_V8_V8                (5)
#define JUMP_SIZE_PREF_V8_V8_V8_V8             (6)
#define JUMP_SIZE_PREF_IMM16_V8                (5)

// js function offset
#define JS_HCLASS_BITFIELD_OFFSET           (56)
#define JS_FUNCTION_METHOD_OFFSET           (32)
#define JS_FUNCTION_CODE_ENTRY_OFFSET       (40)
#define JS_FUNCTION_BOUND_TARGET_OFFSET     (48)
#define JS_FUNCTION_BOUND_THIS_OFFSET       (56)
#define JS_FUNCTION_BOUND_ARG_OFFSET        (64)

// Tagged Array offset
#define TAGGED_ARRAY_LENGTH_OFFSET          (8)
#define TAGGED_ARRAY_DATA_OFFSET            (16)

// TAG_MASK
#define TAGGED_MASK                     (0xffff000000000000)
#define TAGGED_SPECIAL_VALUE            (0x02)
#define TAGGED_VALUE_EXCEPTION          (0x12)

// JS_METHOD CALL FIELD DEFINES
#define JS_METHOD_CALL_FIELD_NATIVE_BIT     (60)
#define JS_METHOD_CALL_FIELD_AOT_BIT        (61)
#define JS_METHOD_NUM_ARGS_SHIFT            (32)

// JS_HCLASS BIT FIELD DEFINES
#define JS_HCLASS_BITFIELD_CALLABLE_BIT     (8)

// MESSAGE_STRING_ID
#define MESSAGE_STRING_NON_CALLABLE_ID      (0xffff000000000005)
#define RUNTIME_ID_THROW_TYPE_ERROR         (0x16)

// com stub
#ifdef PANDA_TARGET_64
#define ASM_GLUE_COMSTUB_ENTRY_OFFSET     (0xdf0)
#else
#define ASM_GLUE_COMSTUB_ENTRY_OFFSET     (0x708)
#endif
#define JSPROXY_CALL_INTERNAL_INDEX       (0x15)

#define ASM_NUM_MANDATORY_JSFUNC_ARGS       (3)
#endif  // ECMASCRIPT_ASM_DEFINES_H
