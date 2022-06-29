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
// pre field pointer pre stack frame point. fill JSThread's sp field with iframe sp field
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
#include "ecmascript/llvm_stackmap_type.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/visitor.h"
namespace panda::ecmascript {
class JSThread;
class EcmaVM;
class FrameIterator;
namespace kungfu {
    class LLVMStackMapParser;
};
using DerivedDataKey = std::pair<uintptr_t, uintptr_t>;
enum class FrameType: uintptr_t {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME = 1,
    OPTIMIZED_JS_FUNCTION_FRAME = 2,
    LEAVE_FRAME = 3,
    LEAVE_FRAME_WITH_ARGV = 4,
    INTERPRETER_FRAME = 5,
    ASM_INTERPRETER_FRAME = 6,
    INTERPRETER_CONSTRUCTOR_FRAME = 7,
    BUILTIN_FRAME = 8,
    BUILTIN_FRAME_WITH_ARGV = 9,
    BUILTIN_ENTRY_FRAME = 10,
    INTERPRETER_FAST_NEW_FRAME = 11,
    INTERPRETER_ENTRY_FRAME = 12,
    ASM_INTERPRETER_ENTRY_FRAME = 13,
    ASM_INTERPRETER_BRIDGE_FRAME = 14,
    OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME = 15,

    INTERPRETER_BEGIN = INTERPRETER_FRAME,
    INTERPRETER_END = INTERPRETER_FAST_NEW_FRAME,
    BUILTIN_BEGIN = BUILTIN_FRAME,
    BUILTIN_END = BUILTIN_ENTRY_FRAME,
};

enum class ReservedSlots: int {
    OPTIMIZED_RESERVED_SLOT = 1,
    OPTIMIZED_JS_FUNCTION_RESERVED_SLOT = 1,
    OPTIMIZED_ENTRY_RESERVED_SLOT = 2,
};

enum class JSCallMode : uintptr_t {
    CALL_ARG0 = 0,
    CALL_ARG1,
    CALL_ARG2,
    CALL_ARG3,
    CALL_WITH_ARGV,
    CALL_THIS_WITH_ARGV,
    CALL_CONSTRUCTOR_WITH_ARGV,
    CALL_SUPER_CALL_WITH_ARGV,
    CALL_GETTER,
    CALL_SETTER,
    CALL_ENTRY,
    CALL_GENERATOR,
    CALL_FROM_AOT,
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                   base::AlignedPointer,
                                                   base::AlignedPointer,
                                                   base::AlignedPointer> {
public:
    void GCIterate(const FrameIterator &it,
        const RootVisitor &v0,
        [[maybe_unused]] const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
        bool isVerifying) const;
private:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
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
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    [[maybe_unused]] alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedFrame), OptimizedFrame::SizeArch32, OptimizedFrame::SizeArch64);

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedJSFunctionArgConfigFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                   base::AlignedPointer,
                                                   base::AlignedPointer> {
private:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static OptimizedJSFunctionArgConfigFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedJSFunctionArgConfigFrame *>(reinterpret_cast<uintptr_t>(sp)
            - MEMBER_OFFSET(OptimizedJSFunctionArgConfigFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    [[maybe_unused]] alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedJSFunctionArgConfigFrame),
    OptimizedJSFunctionArgConfigFrame::SizeArch32, OptimizedJSFunctionArgConfigFrame::SizeArch64);

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedJSFunctionFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                   base::AlignedPointer,
                                                   base::AlignedPointer,
                                                   base::AlignedPointer> {
public:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    uintptr_t* ComputePrevFrameSp(const JSTaggedType *sp, int delta) const
    {
        uintptr_t *preFrameSp = reinterpret_cast<uintptr_t *>(const_cast<JSTaggedType *>(sp))
            + delta / sizeof(uintptr_t);
        return preFrameSp;
    }
    JSTaggedType* GetArgv(uintptr_t *preFrameSp) const
    {
        return reinterpret_cast<JSTaggedType *>(preFrameSp + sizeof(uint64_t) / sizeof(uintptr_t));
    }

    JSTaggedType* GetArgv(const FrameIterator &it) const;

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    uintptr_t* GetPreFrameSp(const FrameIterator &it);
    void GCIterate(
        const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
    std::optional<kungfu::DeoptBundleVec> GetDeoptBundleInfo(const FrameIterator &it) const;
    // dynamic callee saveregisters for x86-64
    [[maybe_unused]] alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    // dynamic callee saveregisters for arm64
    // argc
    // argv[0]
    // argv[1]
    // ... argv[n - 1]
    friend class FrameIterator;
private:
    static OptimizedJSFunctionFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedJSFunctionFrame *>(reinterpret_cast<uintptr_t>(sp)
            - MEMBER_OFFSET(OptimizedJSFunctionFrame, prevFp));
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedJSFunctionFrame), OptimizedJSFunctionFrame::SizeArch32,
    OptimizedJSFunctionFrame::SizeArch64);

struct OptimizedEntryFrame {
public:
    OptimizedEntryFrame() = default;
    ~OptimizedEntryFrame() = default;
    JSTaggedType *preLeaveFrameFp;
    [[maybe_unused]] FrameType type;
    JSTaggedType *prevFp;

    inline JSTaggedType* GetPrevFrameFp()
    {
        return preLeaveFrameFp;
    }
    friend class FrameIterator;
private:
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

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prev;
    }

    static InterpretedFrameBase* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrameBase *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

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
                                                     InterpretedFrameBase> {
public:
    enum class Index : size_t {
        ConstPoolIndex = 0,
        FunctionIndex,
        ProFileTypeInfoIndex,
        AccIndex,
        EnvIndex,
        PcIndex,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp() const
    {
        return base.prev;
    }

    inline const uint8_t *GetPc() const
    {
        return pc;
    }

    inline JSTaggedValue GetEnv() const
    {
        return env;
    }

    static uint32_t NumOfMembers()
    {
        return sizeof(InterpretedFrame) / JSTaggedValue::TaggedTypeSize();
    }
    void GCIterate(const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1) const;

    alignas(EAS) JSTaggedValue constpool {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue profileTypeInfo {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
    friend class FrameIterator;
private:
    static InterpretedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }
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
        CallSizeIndex,
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
        return GetOffset<static_cast<size_t>(Index::CallSizeIndex)>(isArch32);
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

    static uint32_t NumOfMembers()
    {
        return sizeof(AsmInterpretedFrame) / JSTaggedValue::TaggedTypeSize();
    }
    void GCIterate(const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;

    JSTaggedValue GetEnv() const
    {
        return env;
    }

    const uint8_t *GetPc()
    {
        return pc;
    }

    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) uintptr_t callSize {0};
    alignas(EAS) JSTaggedType *fp {nullptr};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
    // vregs, not exist in native
    // args, may be truncated if not extra
    // thisObject, used in asm constructor frame
    // numArgs, used if extra or asm constructor frame
    enum ReverseIndex : int32_t { THIS_OBJECT_REVERSE_INDEX = -2 };
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
    void GCIterate(const FrameIterator &it, const RootVisitor &v0,
        const RootRangeVisitor &v1) const;
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};

STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedEntryFrame),
                      InterpretedEntryFrame::SizeArch32,
                      InterpretedEntryFrame::SizeArch64);

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

    static AsmInterpretedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmInterpretedEntryFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};

struct AsmInterpretedBridgeFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                              AsmInterpretedEntryFrame,
                                                              base::AlignedPointer> {
    enum class Index : size_t {
        EntryIndex = 0,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static AsmInterpretedBridgeFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmInterpretedBridgeFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(AsmInterpretedBridgeFrame, returnAddr));
    }
    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + sizeof(AsmInterpretedBridgeFrame);
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return entry.base.prev;
    }

    alignas(EAS) AsmInterpretedEntryFrame entry;
    alignas(EAS) uintptr_t returnAddr;
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
};

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
    uintptr_t GetCallSiteSp() const
    {
#ifndef PANDA_TARGET_32
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedLeaveFrame, argRuntimeId);
#else
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedLeaveFrame, argc) + argc * sizeof(JSTaggedType);
#endif
    }
    inline JSTaggedType* GetPrevFrameFp() const
    {
        return reinterpret_cast<JSTaggedType*>(callsiteFp);
    }

    JSTaggedType *GetJsFuncFrameArgv(JSThread *thread) const;

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    void GCIterate(
        const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
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
    uintptr_t GetCallSiteSp() const
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
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    void GCIterate(
        const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
};

struct BuiltinFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                 base::AlignedSize,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer> {
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NativeCodeIndex,
        NumArgsIndex,
        StackArgsIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static constexpr uint32_t RESERVED_CALL_ARGCOUNT = 3;

    static BuiltinFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<BuiltinFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(BuiltinFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + MEMBER_OFFSET(BuiltinFrame, nativeCode);
    }
    static size_t GetPreFpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }
    static size_t GetNumArgsToFpDelta(bool isArch32)
    {
        auto offset = GetOffset<static_cast<size_t>(Index::NumArgsIndex)>(isArch32);
        return offset - GetPreFpOffset(isArch32);
    }

    static size_t GetNativeCodeToFpDelta(bool isArch32)
    {
        auto offset = GetOffset<static_cast<size_t>(Index::NativeCodeIndex)>(isArch32);
        return offset - GetPreFpOffset(isArch32);
    }
    static size_t GetStackArgsToFpDelta(bool isArch32)
    {
        auto offset = GetOffset<static_cast<size_t>(Index::StackArgsIndex)>(isArch32);
        return offset - GetPreFpOffset(isArch32);
    }
    uintptr_t GetStackArgsAddress()
    {
        return reinterpret_cast<uintptr_t>(&stackArgs);
    }
    JSTaggedValue GetFunction()
    {
        auto functionAddress = reinterpret_cast<JSTaggedType *>(GetStackArgsAddress());
        return JSTaggedValue(*functionAddress);
    }
    size_t GetNumArgs()
    {
        return numArgs & 0xFFFFFFFF;
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    void GCIterate(
        const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
    alignas(EAS) FrameType type;
    alignas(EAS) JSTaggedType *prevFp;
    alignas(EAS) uintptr_t returnAddr;
    alignas(EAS) uintptr_t nativeCode;
    alignas(EAS) uintptr_t numArgs;
    alignas(EAS) uintptr_t stackArgs;
};

struct BuiltinWithArgvFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                         base::AlignedSize,
                                                         base::AlignedPointer,
                                                         base::AlignedPointer> {
    enum class Index : int {
        StackArgsTopIndex = -1,
        NumArgsIndex = -1,
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static BuiltinWithArgvFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<BuiltinWithArgvFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(BuiltinFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + sizeof(BuiltinWithArgvFrame);
    }
    uintptr_t GetStackArgsAddress()
    {
        auto topAddress = ToUintPtr(this) +
            (static_cast<int>(Index::StackArgsTopIndex) * sizeof(uintptr_t));
        auto numberArgs = GetNumArgs() + BuiltinFrame::RESERVED_CALL_ARGCOUNT;
        return topAddress - numberArgs * sizeof(uintptr_t);
    }
    JSTaggedValue GetFunction()
    {
        auto functionAddress = reinterpret_cast<JSTaggedType *>(GetStackArgsAddress());
        return JSTaggedValue(*functionAddress);
    }
    size_t GetNumArgs()
    {
        auto argcAddress = reinterpret_cast<size_t *>(
            ToUintPtr(this) + (static_cast<int>(Index::NumArgsIndex) * sizeof(uintptr_t)));
        return *argcAddress;
    }
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    void GCIterate(
        const FrameIterator &it, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
    // argv(... this, new.target, function)
    // numargs
    alignas(EAS) FrameType type;
    alignas(EAS) JSTaggedType *prevFp;
    alignas(EAS) uintptr_t returnAddr;
};

class FrameIterator {
public:
    explicit FrameIterator(JSTaggedType *sp, const JSThread *thread = nullptr);
    FrameType GetFrameType() const
    {
        ASSERT(current_ != nullptr);
        FrameType *typeAddr = reinterpret_cast<FrameType *>(
            reinterpret_cast<uintptr_t>(current_) - sizeof(FrameType));
        return *typeAddr;
    }

    template<class T>
    T* GetFrame()
    {
        return T::GetFrameFromSp(current_);
    }

    template<class T>
    const T* GetFrame() const
    {
        return T::GetFrameFromSp(current_);
    }

    bool Done() const
    {
        return current_ == nullptr;
    }
    JSTaggedType *GetSp() const
    {
        return current_;
    }
    JSTaggedType *GetSp()
    {
        return current_;
    }
    const kungfu::LLVMStackMapParser* GetStackMapParser() const
    {
        return stackmapParser_;
    }
    void Advance();
    uintptr_t GetPrevFrameCallSiteSp(uintptr_t curPc = 0) const;
    uintptr_t GetCallSiteSp() const
    {
        return optimizedCallSiteSp_;
    }
    uintptr_t GetOptimizedReturnAddr() const
    {
        return optimizedReturnAddr_;
    }
    const JSThread *GetThread() const
    {
        return thread_;
    }
    bool CollectGCSlots(std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data,
                        bool isVerifying) const;
private:
    JSTaggedType *current_ {nullptr};
    const JSThread *thread_ {nullptr};
    const kungfu::LLVMStackMapParser *stackmapParser_ {nullptr};
    uintptr_t optimizedCallSiteSp_ {0};
    uintptr_t optimizedReturnAddr_ {0};
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H