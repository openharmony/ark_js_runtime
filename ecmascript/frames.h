/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//    in aot project, five Frames: Interpreter Frame、Runtime Frame、Optimized Entry Frame、Optimized Frame、
// Optimized Leave Frame.
//    Optimized Entry Frame: when Interpreter/RuntimeFrame call aot code, generate this kind of frame.
// following c-abi, we'll add the following field: frameType(distinguish frame)、sp(record callite sp register
// for llvm stackmap)、prev(record previous frame position).
//    Optimized Leave Frame: when aot code call Interpreter/RuntimeFrame, generate this kind of frame.
// gc related context (patchid、sp、fp) is saved to frame, prev field point to previous frame.
//    Optimized Frame: when aot code call aot code, generate this kind of frame.
// following c-abi, we'll add the following field: frameType(distinguish frame)、sp(record callite sp register
// for llvm stackmap).

// Frame Layout
// Interpreter Frame(alias   **iframe** ) Layout as follow:
// ```
//   +----------------------------------+-------------------+
//   |    argv[n-1]                     |                   ^
//   |----------------------------------|                   |
//   |    ......                        |                   |
//   |----------------------------------|                   |
//   |    thisArg [maybe not exist]     |                   |
//   |----------------------------------|                   |
//   |    newTarget [maybe not exist]   |                   |
//   |----------------------------------|                   |
//   |    callTarget [deleted]          |                   |
//   |----------------------------------|                   |
//   |    ......                        |                   |
//   |----------------------------------|                   |
//   |    Vregs [not exist in native]   |                   |
//   +----------------------------------+--------+      interpreter frame
//   |    base.frameType                |        ^          |
//   |----------------------------------|        |          |
//   |    base.prev(pre stack pointer)  |        |          |
//   |----------------------------------|        |          |
//   |    numActualArgs [deleted]       |        |          |
//   |----------------------------------|        |          |
//   |    env                           |        |          |
//   |----------------------------------|        |          |
//   |    acc                           |        |          |
//   |----------------------------------|InterpretedFrame   |
//   |    profileTypeInfo               |        |          |
//   |----------------------------------|        |          |
//   |    constantpool                  |        |          |
//   |----------------------------------|        |          |
//   |    method [changed to function]  |        |          |
//   |----------------------------------|        |          |
//   |    sp(current stack point)       |        |          |
//   |----------------------------------|        |          |
//   |    pc(bytecode addr)             |        v          v
//   +----------------------------------+--------+----------+

//   Optimized Leave Frame(alias OptimizedLeaveFrame) layout
//   +--------------------------+
//   |      argv[]              |
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|   |
//   |       patchID            | Fixed
//   |--------------------------| OptimizedLeaveFrame
//   |       RuntimeId          |   |
//   |--------------------------|   |
//   |       returnAddr         |   |
//   |--------------------------|   |
//   |       callsiteFp         |   |
//   |--------------------------|   |
//   |     frameType            |   v
//   +--------------------------+ ---
//   |  callee save registers   |
//   +--------------------------+

//   Optimized Frame(alias OptimizedFrame) layout
//   +--------------------------+
//   | calleesave registers |   ^
//   |----------------------|   |
//   |   returnaddress      | Fixed
//   |----------------------| OptimizedFrame
//   |       prevFp         |   |
//   |----------------------|   |
//   |     frameType        |   |
//   |----------------------|   |
//   |       callsiteSp     |   v
//   +--------------------------+

//   Optimized Entry Frame(alias OptimizedEntryFrame) layout
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

// ```
// address space grow from high address to low address.we add new field  **FrameType** ,
// the field's value is INTERPRETER_FRAME(represent interpreter frame).
// **currentsp**  is pointer to  callTarget field address, sp field 's value is  **currentsp** ,
// pre field pointer pre stack frame point. fill JSthread's sp field with iframe sp field
// by  calling JSThread->SetCurrentSPFrame and save pre Frame address to iframe pre field.

// For Example:
// ```
//                     call                   call
//     foo    -----------------> bar   ----------------------->baz ---------------------> rtfunc
// (interpret frame)       (OptimizedEntryFrame)      (OptimizedFrame)     (OptimizedLeaveFrame + Runtime Frame)
// ```

// Frame Layout as follow:
// ```
//   +----------------------------------+-------------------+
//   |    argv[n-1]                     |                   ^
//   |----------------------------------|                   |
//   |    ......                        |                   |
//   |----------------------------------|                   |
//   |    thisArg [maybe not exist]     |                   |
//   |----------------------------------|                   |
//   |    newTarget [maybe not exist]   |                   |
//   |----------------------------------|                   |
//   |    ......                        |                   |
//   |----------------------------------|                   |
//   |    Vregs                         |                   |
//   +----------------------------------+--------+     foo's frame
//   |    base.frameType                |        ^          |
//   |----------------------------------|        |          |
//   |    base.prev(pre stack pointer)  |        |          |
//   |----------------------------------|        |          |
//   |    env                           |        |          |
//   |----------------------------------|        |          |
//   |    acc                           |        |          |
//   |----------------------------------|        |          |
//   |    profileTypeInfo               |InterpretedFrame   |
//   |----------------------------------|        |          |
//   |    constantpool                  |        |          |
//   |----------------------------------|        |          |
//   |    function                      |        |          |
//   |----------------------------------|        |          |
//   |    sp(current stack point)       |        |          |
//   |----------------------------------|        |          |
//   |    pc(bytecode addr)             |        v          v
//   +----------------------------------+--------+----------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |   returnaddress      |   ^                           ^
//   |----------------------|   |                           |
//   |calleesave registers  | Fixed                         |
//   |----------------------| OptimizedEntryFrame       bar's frame
//   |      prevFp          |   |                           |
//   |----------------------|   |                           |
//   |      frameType       |   |                           |
//   |----------------------|   |                           |
//   |  prevLeaveFrameFp    |   v                           |
//   +--------------------------+                           V
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   +--------------------------+---------------------------+
//   | calleesave registers |   ^                           ^
//   |----------------------|   |                           |
//   |   returnaddress      | Fixed                         |
//   |----------------------| OptimizedFrame                |
//   |       prevFp         |   |                           |
//   |----------------------|   |                       baz's frame Header
//   |     frameType        |   |                           |
//   |----------------------|   |                           |
//   |   callsitesp         |   v                           V
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |      argv[]              |                           ^
//   +--------------------------+ ---                       |
//   |       argc               |   ^                       |
//   |--------------------------|   |                       |
//   |       patchID            | Fixed                     |
//   |--------------------------| OptimizedLeaveFrame             |
//   |       RuntimeId          |   |                       |
//   |--------------------------|   |                      OptimizedLeaveFrame
//   |       returnAddr         |   |                       |
//   |--------------------------|   |                       |
//   |       callsiteFp         |   |                       |
//   |--------------------------|   |                       |
//   |     frameType            |   v                       |
//   +--------------------------+ ----                      |
//   |  callee save registers   |                           V
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |                                                      |
//   |                 rtfunc's Frame                       |
//   |                                                      |
//   +------------------------------------------------------+
// ```
// Iterator:
// rtfunc get OptimizedLeaveFrame by calling GetCurrentSPFrame.
// get baz's Frame by OptimizedLeaveFrame.prev field.
// get bar's Frame by baz's frame fp field
// get foo's Frame by bar's Frame prev field

#ifndef ECMASCRIPT_FRAMES_H
#define ECMASCRIPT_FRAMES_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/trampoline/ecma_asm_defines.h"

namespace panda::ecmascript {
class JSThread;
enum class FrameType: uint64_t  {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME = 1,
    INTERPRETER_FRAME = 2,
    OPTIMIZED_LEAVE_FRAME = 3,
    INTERPRETER_FAST_NEW_FRAME = 4,
};

class FrameConstants {
public:
#ifdef PANDA_TARGET_AMD64
    static constexpr int SP_DWARF_REG_NUM = 7;
    static constexpr int FP_DWARF_REG_NUM = 6;
    static constexpr int CALLSITE_SP_TO_FP_DELTA = -2;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = -2;
#else
#ifdef PANDA_TARGET_ARM64
    static constexpr int SP_DWARF_REG_NUM = 31;  /* x31 */
    static constexpr int FP_DWARF_REG_NUM = 29;  /* x29 */
    static constexpr int CALLSITE_SP_TO_FP_DELTA = -2;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = -2;
#else
#ifdef PANDA_TARGET_ARM32
    static constexpr int SP_DWARF_REG_NUM = 13;
    static constexpr int FP_DWARF_REG_NUM = 11;
    static constexpr int CALLSITE_SP_TO_FP_DELTA = -2;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = -2;
#else
    static constexpr int SP_DWARF_REG_NUM = 0;
    static constexpr int FP_DWARF_REG_NUM = 0;
    static constexpr int CALLSITE_SP_TO_FP_DELTA = 0;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = 0;
#endif
#endif
#endif
    static constexpr int AARCH64_SLOT_SIZE = sizeof(uint64_t);
    static constexpr int AMD64_SLOT_SIZE = sizeof(uint64_t);
    static constexpr int ARM32_SLOT_SIZE = sizeof(uint32_t);
};

class OptimizedFrameBase {
public:
    OptimizedFrameBase() = default;
    ~OptimizedFrameBase() = default;
    FrameType type;
    JSTaggedType *prevFp; // fp register
};

class OptimizedFrame {
public:
    uintptr_t callsiteSp;
    OptimizedFrameBase base;
public:
    OptimizedFrame() = default;
    ~OptimizedFrame() = default;
    static constexpr int64_t GetCallsiteSpToFpDelta()
    {
        return MEMBER_OFFSET(OptimizedFrame, callsiteSp) - MEMBER_OFFSET(OptimizedFrame, base.prevFp);
    }
    static OptimizedFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedFrame *>(reinterpret_cast<uintptr_t>(sp)
            - MEMBER_OFFSET(OptimizedFrame, base.prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prevFp;
    }
};

class OptimizedEntryFrame {
public:
    OptimizedEntryFrame() = default;
    ~OptimizedEntryFrame() = default;
    JSTaggedType *preLeaveFrameFp;
    OptimizedFrameBase base;

    inline JSTaggedType* GetPrevFrameFp()
    {
        return preLeaveFrameFp;
    }

    static constexpr int64_t GetInterpreterFrameFpToFpDelta()
    {
        return MEMBER_OFFSET(OptimizedEntryFrame, preLeaveFrameFp) -
            MEMBER_OFFSET(OptimizedEntryFrame, base.prevFp);
    }

    static OptimizedEntryFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedEntryFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedEntryFrame, base.prevFp));
    }
};

#define INTERPRETED_FRAME_OFFSET_LIST(V)                                                            \
    V(SP, PC, sizeof(uint32_t), sizeof(uint64_t))                                                   \
    V(CONSTPOOL, SP, sizeof(uint32_t), sizeof(uint64_t))                                            \
    V(FUNCTION, CONSTPOOL, JSTaggedValue::TaggedTypeSize(), JSTaggedValue::TaggedTypeSize())        \
    V(PROFILETYPEINFO, FUNCTION, JSTaggedValue::TaggedTypeSize(), JSTaggedValue::TaggedTypeSize())  \
    V(ACC, PROFILETYPEINFO, JSTaggedValue::TaggedTypeSize(), JSTaggedValue::TaggedTypeSize())       \
    V(ENV, ACC, JSTaggedValue::TaggedTypeSize(), JSTaggedValue::TaggedTypeSize())                   \
    V(BASE, ENV, JSTaggedValue::TaggedTypeSize(), JSTaggedValue::TaggedTypeSize())                  \

static constexpr uint32_t INTERPRETED_FRAME_PC_OFFSET_32 = 0U;
static constexpr uint32_t INTERPRETED_FRAME_PC_OFFSET_64 = 0U;
#define INTERPRETED_FRAME_OFFSET_MACRO(name, lastName, lastSize32, lastSize64) \
    static constexpr uint32_t INTERPRETED_FRAME_##name##_OFFSET_32 =           \
        INTERPRETED_FRAME_##lastName##_OFFSET_32 + (lastSize32);               \
    static constexpr uint32_t INTERPRETED_FRAME_##name##_OFFSET_64 =           \
        INTERPRETED_FRAME_##lastName##_OFFSET_64 + (lastSize64);
INTERPRETED_FRAME_OFFSET_LIST(INTERPRETED_FRAME_OFFSET_MACRO)
#undef INTERPRETED_FRAME_OFFSET_MACRO
static constexpr uint32_t SIZEOF_INTERPRETED_FRAME_32 = INTERPRETED_FRAME_ENV_OFFSET_32 +
    JSTaggedValue::TaggedTypeSize() + sizeof(uint32_t) + sizeof(uint64_t);
static constexpr uint32_t SIZEOF_INTERPRETED_FRAME_64 = INTERPRETED_FRAME_ENV_OFFSET_64 +
    JSTaggedValue::TaggedTypeSize() + sizeof(uint64_t) + sizeof(uint64_t);

class InterpretedFrameBase {
public:
    InterpretedFrameBase() = default;
    ~InterpretedFrameBase() = default;
    JSTaggedType  *prev; // for llvm :c-fp ; for interrupt: thread-fp for gc
    FrameType type;
};

// align with 8
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedFrame {
    const uint8_t *pc;
    JSTaggedType *sp;
    // aligned with 8 bits
    alignas(sizeof(uint64_t)) JSTaggedValue constpool;
    JSTaggedValue function;
    JSTaggedValue profileTypeInfo;
    JSTaggedValue acc;
    JSTaggedValue env;
    InterpretedFrameBase base;
    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrame *>(sp) - 1;
    }

    static constexpr uint32_t GetSpOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_SP_OFFSET_32;
        }
        return INTERPRETED_FRAME_SP_OFFSET_64;
    }

    static constexpr uint32_t GetConstpoolOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_CONSTPOOL_OFFSET_32;
        }
        return INTERPRETED_FRAME_CONSTPOOL_OFFSET_64;
    }

    static constexpr uint32_t GetFunctionOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_FUNCTION_OFFSET_32;
        }
        return INTERPRETED_FRAME_FUNCTION_OFFSET_64;
    }

    static constexpr uint32_t GetProfileTypeInfoOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_PROFILETYPEINFO_OFFSET_32;
        }
        return INTERPRETED_FRAME_PROFILETYPEINFO_OFFSET_64;
    }

    static constexpr uint32_t GetAccOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_ACC_OFFSET_32;
        }
        return INTERPRETED_FRAME_ACC_OFFSET_64;
    }

    static constexpr uint32_t GetEnvOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_ENV_OFFSET_32;
        }
        return INTERPRETED_FRAME_ENV_OFFSET_64;
    }
    
    static constexpr uint32_t GetBaseOffset(bool isArch32)
    {
        if (isArch32) {
            return INTERPRETED_FRAME_BASE_OFFSET_32;
        }
        return INTERPRETED_FRAME_BASE_OFFSET_64;
    }

    static constexpr uint32_t GetSize(bool isArch32)
    {
        if (isArch32) {
            return kSizeOn32Platform;
        }
        return kSizeOn64Platform;
    }

    static constexpr uint32_t kSizeOn64Platform =
        2 * sizeof(int64_t) + 5 * sizeof(JSTaggedValue) + 2 * sizeof(uint64_t);
    static constexpr uint32_t kSizeOn32Platform =
        2 * sizeof(int32_t) + 5 * sizeof(JSTaggedValue) + 2 * sizeof(uint64_t);
};
static_assert(sizeof(InterpretedFrame) % sizeof(uint64_t) == 0u);

struct OptimizedLeaveFrame {
    FrameType type;
    uintptr_t callsiteFp; // thread sp set here
    uintptr_t returnAddr;
#ifndef PANDA_TARGET_32
    uint64_t argRuntimeId;
#endif
    uint64_t argPatchId;
    uint64_t argc;
    // argv[] is dynamic
    static OptimizedLeaveFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedLeaveFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedLeaveFrame, callsiteFp));
    }
    uintptr_t GetCallSiteSp()
    {
#ifndef PANDA_TARGET_32
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedLeaveFrame, argRuntimeId);
#else
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedLeaveFrame, argPatchId);
#endif
    }
};

static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_FRAME) == OPTIMIZE_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_ENTRY_FRAME) == JS_ENTRY_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_LEAVE_FRAME) == LEAVE_FRAME_TYPE);
#ifdef PANDA_TARGET_64
    static_assert(InterpretedFrame::kSizeOn64Platform == sizeof(InterpretedFrame));
    static_assert(OptimizedFrame::GetCallsiteSpToFpDelta() ==
        FrameConstants::CALLSITE_SP_TO_FP_DELTA * sizeof(uintptr_t));
    static_assert(OptimizedEntryFrame::GetInterpreterFrameFpToFpDelta() ==
        FrameConstants::INTERPER_FRAME_FP_TO_FP_DELTA * sizeof(uintptr_t));
#endif
#ifdef PANDA_TARGET_32
    static_assert(InterpretedFrame::kSizeOn32Platform == sizeof(InterpretedFrame));
#endif
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H
