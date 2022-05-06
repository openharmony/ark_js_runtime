
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

#include "ecmascript/interpreter/frame_handler.h"

#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"

#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
void FrameHandler::PrevFrame()
{
    auto type = GetFrameType();
    switch (type) {
        case FrameType::OPTIMIZED_FRAME: {
            OptimizedFrame *frame = OptimizedFrame::GetFrameFromSp(sp_);
            sp_ = reinterpret_cast<JSTaggedType *>(frame->prevFp);
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME: {
            OptimizedEntryFrame *frame = OptimizedEntryFrame::GetFrameFromSp(sp_);
            sp_ = reinterpret_cast<JSTaggedType *>(frame->preLeaveFrameFp);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
            AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
            InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
            sp_ = frame->base.prev;
            break;
        }
        case FrameType::LEAVE_FRAME:
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp_);
            sp_ = reinterpret_cast<JSTaggedType *>(frame->callsiteFp);
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME: {
            InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(sp_);
            sp_ = frame->base.prev;
            break;
        }
        default:
            UNREACHABLE();
    }
}

uintptr_t FrameHandler::GetPrevFrameCallSiteSp(const JSTaggedType *sp)
{
    if (sp == nullptr) {
        return 0U;
    }

    auto type = GetFrameType(sp);
    switch (type) {
        case FrameType::LEAVE_FRAME: {
            auto frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
            return frame->GetCallSiteSp();
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            auto frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp);
            return frame->GetCallSiteSp();
        }
        case FrameType::OPTIMIZED_FRAME: {
            auto frame = OptimizedFrame::GetFrameFromSp(sp);
            sp = frame->GetPrevFrameFp();
            type = GetFrameType(sp);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::INTERPRETER_ENTRY_FRAME:
        default:
            UNREACHABLE();
    }

    if (type == FrameType::OPTIMIZED_FRAME) {
        auto frame = OptimizedFrame::GetFrameFromSp(sp);
        return frame->GetCallSiteSp();
    }

    ASSERT((GetFrameType(sp) == FrameType::INTERPRETER_FRAME) ||
           (GetFrameType(sp) == FrameType::INTERPRETER_FAST_NEW_FRAME));
    auto frame = AsmInterpretedFrame::GetFrameFromSp(sp);
    return frame->GetCallSiteSp();
}

void FrameHandler::CurrentAsmInterpretedFrame()
{
    if (sp_ == nullptr) {
        return;
    }
    if (IsInterpretedFrame()) {
        return;
    }
    if (IsLeaveFrame()) {
        FrameHandler::PrevFrame();
        for (; HasFrame() && !IsInterpretedFrame(); FrameHandler::PrevFrame());
        return;
    }
    LOG_ECMA(FATAL) << "unsupported frame type of asm interpreter";
}

void FrameHandler::PrevInterpretedFrame()
{
    PrevFrame();
    for (; HasFrame() && !(IsInterpretedFrame() || IsInterpretedEntryFrame());
        FrameHandler::PrevFrame());
}

JSTaggedType* FrameHandler::GetPrevInterpretedFrame()
{
    PrevInterpretedFrame();
    return GetSp();
}

JSTaggedValue FrameHandler::GetVRegValue(size_t index) const
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return JSTaggedValue(sp_[index]);
}

void FrameHandler::SetVRegValue(size_t index, JSTaggedValue value)
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    sp_[index] = value.GetRawData();
}

JSTaggedValue FrameHandler::GetAcc() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->acc;
}

uint32_t FrameHandler::GetSize() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    JSTaggedType *prevSp = frame->base.prev;

    // The prev frame of InterpretedFrame may entry frame or interpreter frame.
    if (FrameHandler::GetFrameType(prevSp) == FrameType::INTERPRETER_ENTRY_FRAME) {
        return static_cast<uint32_t>((prevSp - sp_) - INTERPRETER_ENTRY_FRAME_STATE_SIZE);
    } else {
        return static_cast<uint32_t>((prevSp - sp_) - INTERPRETER_FRAME_STATE_SIZE);
    }
}

uint32_t FrameHandler::GetBytecodeOffset() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    JSMethod *method = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget();
    auto offset = frame->pc - method->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *FrameHandler::GetMethod() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget();
}

JSTaggedValue FrameHandler::GetFunction() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->function;
}

const uint8_t *FrameHandler::GetPc() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->pc;
}

ConstantPool *FrameHandler::GetConstpool() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
    JSTaggedValue function = frame->function;
    if (!function.IsJSFunction()) {
        return nullptr;
    }
    JSTaggedValue constpool = JSFunction::Cast(function.GetTaggedObject())->GetConstantPool();
    return ConstantPool::Cast(constpool.GetTaggedObject());
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
    return ConstantPool::Cast(frame->constpool.GetTaggedObject());
#endif
}

JSTaggedValue FrameHandler::GetEnv() const
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->env;
}

void FrameHandler::SetEnv(JSTaggedValue env)
{
    ASSERT(IsInterpretedFrame());
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    frame->env = env;
}

void FrameHandler::DumpStack(std::ostream &os) const
{
    size_t i = 0;
    FrameHandler frameHandler(thread_);
    for (; frameHandler.HasFrame(); frameHandler.PrevFrame()) {
        os << "[" << i++
        << "]:" << frameHandler.GetMethod()->ParseFunctionName()
        << "\n";
    }
}

void FrameHandler::DumpPC(std::ostream &os, const uint8_t *pc) const
{
    FrameHandler frameHandler(thread_);
    ASSERT(frameHandler.HasFrame());

    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
    int offset = pc - frameHandler.GetMethod()->GetBytecodeArray();
    os << "offset: " << offset << "\n";
}

ARK_INLINE void FrameHandler::InterpretedFrameIterate(const JSTaggedType *sp,
                                                      const RootVisitor &v0,
                                                      const RootRangeVisitor &v1) const
{
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp);
    uintptr_t start = ToUintPtr(sp);
    uintptr_t end = ToUintPtr(frame->GetCurrentFramePointer());
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    if (frame->pc != nullptr) {
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
    }
#else
    if (sp == nullptr) {
        return;
    }

#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp);
    if (frame->function == JSTaggedValue::Hole()) {
        return;
    }
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp);
    if (frame->sp == nullptr) {
        return;
    }
#endif

    uintptr_t start = ToUintPtr(sp);
    uintptr_t end = 0U;
    JSTaggedType *prevSp = frame->base.prev;
    FrameType type = FrameHandler::GetFrameType(prevSp);
    if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
        AsmInterpretedFrame *prevFrame = AsmInterpretedFrame::GetFrameFromSp(prevSp);
#else
        InterpretedFrame *prevFrame = InterpretedFrame::GetFrameFromSp(prevSp);
#endif
        end = ToUintPtr(prevFrame);
    } else if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
        end = ToUintPtr(GetInterpretedEntryFrameStart(prevSp));
    } else {
        LOG_ECMA(FATAL) << "frame type error!";
        UNREACHABLE();
    }

    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    if (frame->pc != nullptr) {
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
#else
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->constpool)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->profileTypeInfo)));
#endif
    }
#endif  // ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
}

ARK_INLINE JSTaggedType *FrameHandler::GetInterpretedEntryFrameStart(const JSTaggedType *sp)
{
    ASSERT(FrameHandler::GetFrameType(sp) == FrameType::INTERPRETER_ENTRY_FRAME);
    JSTaggedType *argcSp = const_cast<JSTaggedType *>(sp) - INTERPRETER_ENTRY_FRAME_STATE_SIZE - 1;
    uint32_t argc = argcSp[0];
    return argcSp - argc - RESERVED_CALL_ARGCOUNT;
}

ARK_INLINE void FrameHandler::InterpretedEntryFrameIterate(const JSTaggedType *sp,
                                                           [[maybe_unused]] const RootVisitor &v0,
                                                           const RootRangeVisitor &v1) const
{
    if (sp == nullptr) {
        return;
    }

    uintptr_t start = ToUintPtr(GetInterpretedEntryFrameStart(sp));
    uintptr_t end = ToUintPtr(sp - INTERPRETER_ENTRY_FRAME_STATE_SIZE);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void FrameHandler::OptimizedFrameIterate(const JSTaggedType *sp,
                                                    const RootVisitor &v0,
                                                    [[maybe_unused]] const RootRangeVisitor &v1,
                                                    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                    bool isVerifying) const
{
    if (sp == nullptr) {
        return;
    }

    std::set<uintptr_t> slotAddrs;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto returnAddr =
        reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(const_cast<JSTaggedType *>(sp)) + 1));
    bool enableCompilerLog = thread_->GetEcmaVM()->GetJSOptions().WasSetlogCompiledMethods();
    bool ret = kungfu::LLVMStackMapParser::GetInstance(enableCompilerLog).CollectStackMapSlots(
        returnAddr, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::OptimizedEntryFrameIterate(const JSTaggedType *sp,
                                                         const RootVisitor &v0,
                                                         [[maybe_unused]] const RootRangeVisitor &v1,
                                                         ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                         bool isVerifying) const
{
    if (sp == nullptr) {
        return;
    }

    std::set<uintptr_t> slotAddrs;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto returnAddr = reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(const_cast<JSTaggedType *>(sp)) + 1));
    bool enableCompilerLog = thread_->GetEcmaVM()->GetJSOptions().WasSetlogCompiledMethods();
    bool ret = kungfu::LLVMStackMapParser::GetInstance(enableCompilerLog).CollectStackMapSlots(
        returnAddr, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::OptimizedLeaveFrameIterate(const JSTaggedType *sp,
                                                         const RootVisitor &v0,
                                                         [[maybe_unused]] const RootRangeVisitor &v1,
                                                         ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                         bool isVerifying) const
{
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        uintptr_t start = ToUintPtr(&frame->argc + 1); // argv
        uintptr_t end = ToUintPtr(&frame->argc + 1 + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }

    std::set<uintptr_t> slotAddrs;
    bool ret = kungfu::LLVMStackMapParser::GetInstance().CollectStackMapSlots(
        frame->returnAddr, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying);
    if (!ret) {
        return;
    }

    for (auto slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::OptimizedWithArgvLeaveFrameIterate(const JSTaggedType *sp,
                                                                 const RootVisitor &v0,
                                                                 [[maybe_unused]] const RootRangeVisitor &v1,
                                                                 ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                                 bool isVerifying) const
{
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        uintptr_t* argvPtr = reinterpret_cast<uintptr_t *>(&frame->argc + 1);
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(*argvPtr);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }

    std::set<uintptr_t> slotAddrs;
    bool ret = kungfu::LLVMStackMapParser::GetInstance().CollectStackMapSlots(
        frame->returnAddr, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying);
    if (!ret) {
        return;
    }

    for (auto slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
void FrameHandler::IterateRsp(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    IterateFrameChain(current, v0, v1);
}

void FrameHandler::IterateSp(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    while (current) {
        // only interpreter entry frame is on thread sp when rsp is enable
        ASSERT(FrameHandler::GetFrameType(current) == FrameType::INTERPRETER_ENTRY_FRAME);
        InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(current);
        InterpretedEntryFrameIterate(current, v0, v1);
        current = frame->GetPrevFrameFp();
    }
}

void FrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    IterateSp(v0, v1);
    IterateRsp(v0, v1);
}
#else
void FrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    FrameType frameType = FrameHandler(thread_).GetFrameType();
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    if (frameType != FrameType::INTERPRETER_ENTRY_FRAME) {
        auto leaveFrame = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
        if (leaveFrame != nullptr) {
            current = leaveFrame;
        }
    }
    IterateFrameChain(current, v0, v1);
}
#endif  // ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK

void FrameHandler::IterateFrameChain(JSTaggedType *start, const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers = thread_->GetEcmaVM()->GetHeap()->GetDerivedPointers();
    bool isVerifying = false;

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    isVerifying = thread_->GetEcmaVM()->GetHeap()->IsVerifying();
#endif
    JSTaggedType *current = start;
    while (current) {
        FrameType type = FrameHandler::GetFrameType(current);
        if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
#ifdef ECMASCRIPT_COMPILE_ASM_INTERPRETER
            AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(current);
#else
            InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(current);
#endif
            InterpretedFrameIterate(current, v0, v1);
            current = frame->GetPrevFrameFp();
        } else if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
            InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(current);
            InterpretedEntryFrameIterate(current, v0, v1);
            current = frame->GetPrevFrameFp();
        } else if (type == FrameType::OPTIMIZED_FRAME) {
            OptimizedFrame *frame = OptimizedFrame::GetFrameFromSp(current);
            OptimizedFrameIterate(current, v0, v1, derivedPointers, isVerifying);
            current = frame->GetPrevFrameFp();
        } else if (type == FrameType::OPTIMIZED_ENTRY_FRAME) {
            OptimizedEntryFrame *frame = OptimizedEntryFrame::GetFrameFromSp(current);
            current = frame->GetPrevFrameFp();
            // NOTE: due to "AotInfo" iteration, current frame might not be interpreted frame
        } else if (type == FrameType::LEAVE_FRAME_WITH_ARGV) {
            OptimizedWithArgvLeaveFrame *frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(current);
            OptimizedWithArgvLeaveFrameIterate(current, v0, v1, derivedPointers, isVerifying);
            current = reinterpret_cast<JSTaggedType *>(frame->callsiteFp);
        } else {
            ASSERT(type == FrameType::LEAVE_FRAME);
            OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(current);
            OptimizedLeaveFrameIterate(current, v0, v1, derivedPointers, isVerifying);
            // arm32, arm64 and x86_64 support stub and aot, when aot/stub call runtime, generate Optimized
            // Leave Frame.
            current = reinterpret_cast<JSTaggedType *>(frame->callsiteFp);
            ASSERT(GetFrameType(current) == FrameType::OPTIMIZED_ENTRY_FRAME ||
                   GetFrameType(current) == FrameType::OPTIMIZED_FRAME ||
                   GetFrameType(current) == FrameType::INTERPRETER_FRAME ||
                   GetFrameType(current) == FrameType::INTERPRETER_ENTRY_FRAME);
        }
    }
}
}  // namespace panda::ecmascript
