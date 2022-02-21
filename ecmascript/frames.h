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

//   Optimized Leave Frame(alias OptLeaveFrame) layout
//   +--------------------------+
//   |     patchID          |   ^
//   |- - - - - - - - -     |   |
//   |       callsiteFp     | Fixed
//   |- - - - - - - - -     | OptLeaveFrame
//   |       callsiteSp     |   |
//   |- - - - - - - - -     |   |
//   |       prevFp         |   |
//   |- - - - - - - - -     |   |
//   |       frameType      |   v
//   +--------------------------+

//   Optimized Frame(alias OptimizedFrame) layout
//   +--------------------------+
//   | calleesave registers |   ^
//   |- - - - - - - - -     |   |
//   |   returnaddress      | Fixed
//   |- - - - - - - - -     | OptimizedFrame
//   |       prevFp         |   |
//   |- - - - - - - - -     |   |
//   |     frameType        |   |
//   |- - - - - - - - -     |   |
//   |       callsiteSp     |   v
//   +--------------------------+

//   Optimized Entry Frame(alias OptimizedEntryFrame) layout
//   +--------------------------+
//   | calleesave registers |   ^
//   |- - - - - - - - -     |   |
//   |   returnaddress      | Fixed
//   |- - - - - - - - -     | OptimizedEntryFrame
//   |       prevFp         |   |
//   |- - - - - - - - -     |   |
//   |     frameType        |   |
//   |- - - - - - - - -     |   |
//   |  callsiteSp          |   v
//   |- - - - - - - - -     |   |
//   | prevManagedFrameFp   |   |
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
// (interpret frame)       (OptimizedEntryFrame)      (OptimizedFrame)     (OptLeaveFrame + Runtime Frame)
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
//   +--------------------------+---------------------------+
//   | calleesave registers |   ^                           ^
//   |- - - - - - - - -     |   |                           |
//   |   returnaddress      |   Fixed                       |
//   |- - - - - - - - -     | OptimizedEntryFrame           |
//   |       prevFp         |   |                           |
//   |- - - - - - - - -     |   |                           |
//   |     frameType        |   |                        bar's frame
//   |- - - - - - - - -     |   |                           |
//   |   callsiteSp         |   v                           |
//   |- - - - - - - - -     |   |                           |
//   |prevManagedFrameFp    |   |                           V
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   +--------------------------+---------------------------+
//   | calleesave registers |   ^                           ^
//   |- - - - - - - - -     |   |                           |
//   |   returnaddress      | Fixed                         |
//   |- - - - - - - - -     | OptimizedFrame                |
//   |       prevFp         |   |                           |
//   |- - - - - - - - -     |   |                       baz's frame Header
//   |     frameType        |   |                           |
//   |- - - - - - - - -     |   |                           |
//   |   callsitesp         |   v                           V
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |     patchID          |   ^                           ^
//   |- - - - - - - - -     |   |                           |
//   |     callsiteFp       | Fixed                         |
//   |- - - - - - - - -     | OptLeaveFrame                 |
//   |     callsitesp       |   |                         OptLeaveFrame
//   |- - - - - - - - -     |   |                           |
//   |       prevFp         |   |                           |
//   |- - - - - - - - -     |   |                           |
//   |       frameType      |   v                           |
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |                                                      |
//   |                 rtfunc's Frame                       |
//   |                                                      |
//   +------------------------------------------------------+
// ```
// Iterator:
// rtfunc get OptLeaveFrame by calling GetCurrentSPFrame.
// get baz's Frame by OptLeaveFrame.prev field.
// get bar's Frame by baz's frame fp field
// get foo's Frame by bar's Frame prev field

#ifndef ECMASCRIPT_FRAMES_H
#define ECMASCRIPT_FRAMES_H

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSThread;
enum class FrameType: uint64_t {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME = 1,
    INTERPRETER_FRAME = 2,
    OPTIMIZED_LEAVE_FRAME = 3,
};

class FrameConstants {
public:
#ifdef PANDA_TARGET_AMD64
    static constexpr int SP_DWARF_REG_NUM = 7;
    static constexpr int FP_DWARF_REG_NUM = 6;
    static constexpr int CALLSITE_SP_TO_FP_DELTA = -2;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = -3;
#else
#ifdef PANDA_TARGET_ARM64
    static constexpr int SP_DWARF_REG_NUM = 31;  /* x31 */
    static constexpr int FP_DWARF_REG_NUM = 29;  /* x29 */
    static constexpr int CALLSITE_SP_TO_FP_DELTA = -2;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = -3;
#else
#ifdef PANDA_TARGET_ARM32
    static constexpr int SP_DWARF_REG_NUM = 13;
    static constexpr int FP_DWARF_REG_NUM = 11;
    static constexpr int CALLSITE_SP_TO_FP_DELTA = 0;
    static constexpr int INTERPER_FRAME_FP_TO_FP_DELTA = 0;
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
    static constexpr int64_t GetCallsiteSpToFpDelta()
    {
        return MEMBER_OFFSET(OptimizedFrameBase, callsiteSp) - MEMBER_OFFSET(OptimizedFrameBase, prevFp);
    }
    uintptr_t callsiteSp;
    FrameType type;
    JSTaggedType *prevFp; // fp register
    static OptimizedFrameBase* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedFrameBase *>(reinterpret_cast<uintptr_t>(sp)
            - MEMBER_OFFSET(OptimizedFrameBase, prevFp));
    }
};

class OptimizedEntryFrame {
public:
    OptimizedEntryFrame() = default;
    ~OptimizedEntryFrame() = default;
    JSTaggedType *prevInterpretedFrameFp;
    OptimizedFrameBase base;
    static constexpr int64_t GetCallsiteSpToFpDelta()
    {
        return MEMBER_OFFSET(OptimizedEntryFrame, base.callsiteSp) - MEMBER_OFFSET(OptimizedEntryFrame, base.prevFp);
    }
    // INTERPER_FRAME_FP_TO_FP_DELTA
    static constexpr int64_t GetInterpreterFrameFpToFpDelta()
    {
        return MEMBER_OFFSET(OptimizedEntryFrame, prevInterpretedFrameFp) -
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
    static InterpretedFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrame *>(sp) - 1;
    }
    static constexpr uint32_t kSizeOn64Platform =
        2 * sizeof(int64_t) + 5 * sizeof(JSTaggedValue) + 2 * sizeof(uint64_t);
    static constexpr uint32_t kSizeOn32Platform =
        2 * sizeof(int32_t) + 5 * sizeof(JSTaggedValue) + 2 * sizeof(uint64_t);
};
static_assert(sizeof(InterpretedFrame) % sizeof(uint64_t) == 0u);

struct OptLeaveFrame {
    uintptr_t callsiteSp;
    uint64_t patchId;
    FrameType type;
    uintptr_t callsiteFp; //thread sp set here
    uintptr_t  returnAddr;
    uint64_t argRuntimeId;
    uint64_t argPatchId;
    uint64_t argc;
    static OptLeaveFrame* GetFrameFromSp(JSTaggedType *sp)
    {
        return reinterpret_cast<OptLeaveFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptLeaveFrame, callsiteFp));
    }
    static constexpr uint32_t kSizeOn64Platform = sizeof(FrameType) + 4 * sizeof(uint64_t);
    static constexpr uint32_t kSizeOn32Platform = sizeof(FrameType) + 3 * sizeof(int32_t) + sizeof(uint64_t);
    static constexpr uint32_t kPrevFpOffset = sizeof(FrameType);
};

#ifdef PANDA_TARGET_64
    static_assert(InterpretedFrame::kSizeOn64Platform == sizeof(InterpretedFrame));
    static_assert(OptimizedFrameBase::GetCallsiteSpToFpDelta() ==
        FrameConstants::CALLSITE_SP_TO_FP_DELTA * sizeof(uintptr_t));
    static_assert(OptimizedEntryFrame::GetCallsiteSpToFpDelta() ==
        FrameConstants::CALLSITE_SP_TO_FP_DELTA * sizeof(uintptr_t));
    static_assert(OptimizedEntryFrame::GetInterpreterFrameFpToFpDelta() ==
        FrameConstants::INTERPER_FRAME_FP_TO_FP_DELTA * sizeof(uintptr_t));
#endif
#ifdef PANDA_TARGET_32
    static_assert(InterpretedFrame::kSizeOn32Platform == sizeof(InterpretedFrame));
#endif
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H
