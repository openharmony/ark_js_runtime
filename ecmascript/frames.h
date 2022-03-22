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

#ifndef ECMASCRIPT_FRAMES_H
#define ECMASCRIPT_FRAMES_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/trampoline/asm_defines.h"

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
//   +--------------------------+---------------------------+
//   |       argv[]             |                           ^
//   +--------------------------+----                       |
//   |       argc               |   ^                       |
//   |--------------------------|  Fixed                    |
//   |       RuntimeId          | OptimizedLeaveFrame       |
//   |--------------------------|   |                  OptimizedLeaveFrame
//   |       returnAddr         |   |                       |
//   |--------------------------|   |                       |
//   |       callsiteFp         |   |                       |
//   |--------------------------|   |                       |
//   |       frameType          |   V                       |
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

#include "ecmascript/base/aligned_struct.h"

namespace panda::ecmascript {
class JSThread;
enum class FrameType: uintptr_t {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME = 1,
    INTERPRETER_FRAME = 2,
    OPTIMIZED_LEAVE_FRAME = 3,
    INTERPRETER_FAST_NEW_FRAME = 4,
    ASM_LEAVE_FRAME = 5,
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
    static constexpr int CALLSITE_SP_TO_FP_DELTA_OFFSET = \
        CALLSITE_SP_TO_FP_DELTA * static_cast<int>(sizeof(uintptr_t));
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

class InterpretedFrameBase {
public:
    InterpretedFrameBase() = default;
    ~InterpretedFrameBase() = default;
    JSTaggedType  *prev; // for llvm :c-fp ; for interrupt: thread-fp for gc
    FrameType type;
    static constexpr size_t TYPE_OFFSET_32 = sizeof(uint32_t);
    static constexpr size_t TYPE_OFFSET_64 = sizeof(uint64_t);
    static constexpr size_t SizeArch32 = TYPE_OFFSET_32 + sizeof(FrameType);
    static constexpr size_t SizeArch64 = TYPE_OFFSET_64 + sizeof(FrameType);
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedFrameBase), InterpretedFrameBase::SizeArch32, InterpretedFrameBase::SizeArch64);

// align with 8
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                     base::AlignedPointer,
                                                     base::AlignedPointer,
                                                     base::AlignedSize,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     InterpretedFrameBase> {
    enum class Index : size_t {
        PcIndex = 0,
        SpIndex,
        CallSizeIndex,
        ConstPoolIndex,
        FunctionIndex,
        ProFileTypeInfoIndex,
        AccIndex,
        EnvIndex,
        BaseIndex,
        NumOfMembers
    };

    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrame *>(sp) - 1;
    }

    static uint32_t GetSpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::SpIndex)>(isArch32);
    }

    static uint32_t GetCallSizeOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CallSizeIndex)>(isArch32);
    }

    static uint32_t GetConstpoolOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::ConstPoolIndex)>(isArch32);
    }

    static uint32_t GetFunctionOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::FunctionIndex)>(isArch32);
    }

    static uint32_t GetProfileTypeInfoOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::ProFileTypeInfoIndex)>(isArch32);
    }

    static uint32_t GetAccOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::AccIndex)>(isArch32);
    }

    static uint32_t GetEnvOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::EnvIndex)>(isArch32);
    }
    
    static uint32_t GetBaseOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32);
    }

    static constexpr uint32_t GetSize(bool isArch32)
    {
        return isArch32 ? InterpretedFrame::SizeArch32 : InterpretedFrame::SizeArch64;
    }

    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) JSTaggedType *sp {nullptr};
    alignas(EAS) size_t callSize {0};
    alignas(EAS) JSTaggedValue constpool {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue profileTypeInfo {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) InterpretedFrameBase base;
};

STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedFrame), InterpretedFrame::SizeArch32, InterpretedFrame::SizeArch64);

static_assert(sizeof(InterpretedFrame) % sizeof(uint64_t) == 0u);

struct OptimizedLeaveFrame {
    FrameType type;
    uintptr_t callsiteFp; // thread sp set here
    uintptr_t returnAddr;
#ifndef PANDA_TARGET_32
    uint64_t argRuntimeId;
#endif
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
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedLeaveFrame, argc) + argc * sizeof(JSTaggedType);
#endif
    }
};

static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_FRAME) == OPTIMIZE_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_ENTRY_FRAME) == JS_ENTRY_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_LEAVE_FRAME) == LEAVE_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::ASM_LEAVE_FRAME) == ASM_LEAVE_FRAME_TYPE);
#ifdef PANDA_TARGET_64
    static_assert(OptimizedFrame::GetCallsiteSpToFpDelta() ==
        FrameConstants::CALLSITE_SP_TO_FP_DELTA * sizeof(uintptr_t));
    static_assert(OptimizedEntryFrame::GetInterpreterFrameFpToFpDelta() ==
        FrameConstants::INTERPER_FRAME_FP_TO_FP_DELTA * sizeof(uintptr_t));
#endif
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H
