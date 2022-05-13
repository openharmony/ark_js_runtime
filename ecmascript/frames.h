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
//   |    ......                        |                   |
//   |----------------------------------|                   |
//   |    Vregs [not exist in native]   |                   |
//   +----------------------------------+--------+      interpreter frame
//   |    base.frameType                |        ^          |
//   |----------------------------------|        |          |
//   |    base.prev(prev stack pointer) |        |          |
//   |----------------------------------|        |          |
//   |    pc(bytecode addr)             |        |          |
//   |----------------------------------|        |          |
//   |    sp(current stack pointer)     |        |          |
//   |----------------------------------|        |          |
//   |    env                           |        |          |
//   |----------------------------------|        |          |
//   |    acc                           |        |          |
//   |----------------------------------|InterpretedFrame   |
//   |    profileTypeInfo               |        |          |
//   |----------------------------------|        |          |
//   |    function                      |        |          |
//   |----------------------------------|        |          |
//   |    constpool                     |        v          v
//   +----------------------------------+--------+----------+

//   Optimized Leave Frame(alias OptimizedLeaveFrame) layout
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
//   |  callee save registers   |
//   +--------------------------+

//   Optimized Leave Frame with Argv(alias OptimizedWithArgvLeaveFrame) layout
//   +--------------------------+
//   |       argv[]             |
//   +--------------------------+ ---
//   |       argc               |   ^
//   |--------------------------|  Fixed
//   |       RuntimeId          | OptimizedWithArgvLeaveFrame
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
//   |       callSiteSp     |   v
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

//   Interpreted Entry Frame(alias InterpretedEntryFrame) layout
//   +--------------------------+
//   |      base.type       |   ^
//   |----------------------|   |
//   |      base.prev       | InterpretedEntryFrame
//   |----------------------|   |
//   |          pc          |   v
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
//   |    base.prev(prev stack pointer) |        |          |
//   |----------------------------------|        |          |
//   |    pc(bytecode addr)             |        |          |
//   +----------------------------------|        |          |
//   |    sp(current stack pointer)     |        |          |
//   |----------------------------------|        |          |
//   |    env                           |        |          |
//   |----------------------------------|        |          |
//   |    acc                           |        |          |
//   |----------------------------------|        |          |
//   |    profileTypeInfo               |InterpretedFrame   |
//   |----------------------------------|        |          |
//   |    function                      |        |          |
//   |----------------------------------|        |          |
//   |    constpool                     |        v          v
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
enum class FrameType: uintptr_t {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME = 1,
    LEAVE_FRAME = 2,
    LEAVE_FRAME_WITH_ARGV = 3,
    INTERPRETER_FRAME = 4,
    ASM_INTERPRETER_FRAME = 5,
    INTERPRETER_CONSTRUCTOR_FRAME = 6,
    BUILTIN_FRAME = 7,
    INTERPRETER_FAST_NEW_FRAME = 8,
    INTERPRETER_ENTRY_FRAME = 9,
    ASM_INTERPRETER_ENTRY_FRAME = 10,

    INTERPRETER_BEGIN = INTERPRETER_FRAME,
    INTERPRETER_END = INTERPRETER_FAST_NEW_FRAME,
};

class FrameConstants {
public:
#ifdef PANDA_TARGET_AMD64
    static constexpr int SP_DWARF_REG_NUM = 7;
    static constexpr int FP_DWARF_REG_NUM = 6;
#else
#ifdef PANDA_TARGET_ARM64
    static constexpr int SP_DWARF_REG_NUM = 31;  /* x31 */
    static constexpr int FP_DWARF_REG_NUM = 29;  /* x29 */
#else
#ifdef PANDA_TARGET_ARM32
    static constexpr int SP_DWARF_REG_NUM = 13;
    static constexpr int FP_DWARF_REG_NUM = 11;
#else
    static constexpr int SP_DWARF_REG_NUM = 0;
    static constexpr int FP_DWARF_REG_NUM = 0;
#endif
#endif
#endif
    static constexpr int AARCH64_SLOT_SIZE = sizeof(uint64_t);
    static constexpr int AMD64_SLOT_SIZE = sizeof(uint64_t);
    static constexpr int ARM32_SLOT_SIZE = sizeof(uint32_t);
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                   base::AlignedPointer,
                                                   base::AlignedPointer,
                                                   base::AlignedPointer> {
public:
    enum class Index : size_t {
        CallSiteSpIndex = 0,
        TypeIndex,
        PrevFpIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static OptimizedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedFrame *>(reinterpret_cast<uintptr_t>(sp)
            - MEMBER_OFFSET(OptimizedFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    inline uintptr_t GetCallSiteSp() const
    {
        return callSiteSp;
    }
    static int32_t GetCallSiteFpToSpDelta(bool isArch32)
    {
        auto prevFpOffset = GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
        auto callSiteSpOffset = GetOffset<static_cast<size_t>(Index::CallSiteSpIndex)>(isArch32);
        return prevFpOffset - callSiteSpOffset;
    }

    alignas(EAS) uintptr_t callSiteSp {0};
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedFrame), OptimizedFrame::SizeArch32, OptimizedFrame::SizeArch64);

struct OptimizedEntryFrame {
public:
    OptimizedEntryFrame() = default;
    ~OptimizedEntryFrame() = default;
    JSTaggedType *preLeaveFrameFp;
    FrameType type;
    JSTaggedType *prevFp;

    inline JSTaggedType* GetPrevFrameFp()
    {
        return preLeaveFrameFp;
    }

    static OptimizedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedEntryFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedEntryFrame, prevFp));
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedFrameBase : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                         base::AlignedPointer,
                                                         base::AlignedSize> {
    enum class Index : size_t {
        PrevIndex = 0,
        TypeIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static size_t GetPrevOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::PrevIndex)>(isArch32);
    }

    static size_t GetTypeOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    alignas(EAS) JSTaggedType *prev {nullptr}; // for llvm :c-fp ; for interrupt: thread-fp for gc
    alignas(EAS) FrameType type {FrameType::OPTIMIZED_FRAME}; // 0
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedFrameBase),
                      InterpretedFrameBase::SizeArch32,
                      InterpretedFrameBase::SizeArch64);

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     base::AlignedPointer,
                                                     base::AlignedPointer,
                                                     InterpretedFrameBase> {
    enum class Index : size_t {
        ConstPoolIndex = 0,
        FunctionIndex,
        ProFileTypeInfoIndex,
        AccIndex,
        EnvIndex,
        SpIndex,
        PcIndex,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }
    static uint32_t NumOfMembers()
    {
        return sizeof(InterpretedFrame) / JSTaggedValue::TaggedTypeSize();
    }

    alignas(EAS) JSTaggedValue constpool {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue profileTypeInfo {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedType *sp {nullptr};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedFrame), InterpretedFrame::SizeArch32, InterpretedFrame::SizeArch64);

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct AsmInterpretedFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                        JSTaggedValue,
                                                        JSTaggedValue,
                                                        JSTaggedValue,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        InterpretedFrameBase> {
    enum class Index : size_t {
        FunctionIndex = 0,
        AccIndex,
        EnvIndex,
        CallSizeOrCallSiteSpIndex,
        FpIndex,
        PcIndex,
        BaseIndex,
        NumOfMembers
    };

    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetCurrentFramePointer()
    {
        return fp;
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static AsmInterpretedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmInterpretedFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    static size_t GetFpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::FpIndex)>(isArch32);
    }

    static size_t GetCallSizeOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CallSizeOrCallSiteSpIndex)>(isArch32);
    }

    static size_t GetFunctionOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::FunctionIndex)>(isArch32);
    }

    static size_t GetAccOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::AccIndex)>(isArch32);
    }

    static size_t GetEnvOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::EnvIndex)>(isArch32);
    }

    static size_t GetBaseOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32);
    }

    static size_t GetPcOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::PcIndex)>(isArch32);
    }

    static constexpr size_t GetSize(bool isArch32)
    {
        return isArch32 ? AsmInterpretedFrame::SizeArch32 : AsmInterpretedFrame::SizeArch64;
    }

    inline uintptr_t GetCallSiteSp() const
    {
        return callSizeOrCallSiteSp;
    }

    static int32_t GetCallSiteFpToSpDelta(bool isArch32)
    {
        auto fpOffset = GetSize(isArch32);
        auto callSiteSpOffset = GetOffset<static_cast<size_t>(Index::CallSizeOrCallSiteSpIndex)>(isArch32);
        return fpOffset - callSiteSpOffset;
    }

    static uint32_t NumOfMembers()
    {
        return sizeof(AsmInterpretedFrame) / JSTaggedValue::TaggedTypeSize();
    }

    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) uintptr_t callSizeOrCallSiteSp {0};
    alignas(EAS) JSTaggedType *fp {nullptr};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
    // vregs, not exist in native
    // args, may be truncated if not extra
    // thisObject, used in asm constructor frame
    // numArgs, used if extra or asm constructor frame
    enum ReverseIndex : int32_t { NUM_ARGS_REVERSE_INDEX = -1, THIS_OBJECT_REVERSE_INDEX = -2 };
};
STATIC_ASSERT_EQ_ARCH(sizeof(AsmInterpretedFrame), AsmInterpretedFrame::SizeArch32, AsmInterpretedFrame::SizeArch64);

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedEntryFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                          base::AlignedPointer,
                                                          InterpretedFrameBase> {
    enum class Index : size_t {
        PcIndex = 0,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedEntryFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct AsmInterpretedEntryFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                             base::AlignedPointer,
                                                             InterpretedFrameBase> {
    enum class Index : size_t {
        PcIndex = 0,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedEntryFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};

STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedEntryFrame),
                      InterpretedEntryFrame::SizeArch32,
                      InterpretedEntryFrame::SizeArch64);

struct OptimizedLeaveFrame {
    FrameType type;
    uintptr_t callsiteFp; // thread sp set here
    uintptr_t returnAddr;
#ifndef PANDA_TARGET_32
    uint64_t argRuntimeId;
#endif
    uint64_t argc;
    // argv[0]...argv[argc-1] dynamic according to agc
    static OptimizedLeaveFrame* GetFrameFromSp(const JSTaggedType *sp)
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
    inline JSTaggedType* GetPrevFrameFp()
    {
        return reinterpret_cast<JSTaggedType*>(callsiteFp);
    }
};

struct OptimizedWithArgvLeaveFrame {
    FrameType type;
    uintptr_t callsiteFp; // thread sp set here
    uintptr_t returnAddr;
#ifndef PANDA_TARGET_32
    uint64_t argRuntimeId;
#endif
    uint64_t argc;
    // uintptr_t argv[]
    static OptimizedWithArgvLeaveFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedWithArgvLeaveFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, callsiteFp));
    }
    uintptr_t GetCallSiteSp()
    {
#ifndef PANDA_TARGET_32
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, argRuntimeId);
#else
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, argc) + argc * sizeof(JSTaggedType);
#endif
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return reinterpret_cast<JSTaggedType*>(callsiteFp);
    }
};

struct BuiltinFrame {
    FrameType type;
    JSTaggedType *prevFp;
    uintptr_t returnAddr;
    uintptr_t argc;
    JSTaggedValue function;

    static BuiltinFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<BuiltinFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(BuiltinFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    uintptr_t GetCallSiteSp()
    {
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, argc);
    }
};

static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_FRAME) == OPTIMIZE_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::OPTIMIZED_ENTRY_FRAME) == JS_ENTRY_FRAME_TYPE);
static_assert(static_cast<uint64_t>(FrameType::LEAVE_FRAME) == LEAVE_FRAME_TYPE);
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H
