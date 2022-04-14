
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
#include "ecmascript/mem/heap.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"

#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
void FrameHandler::PrevFrame()
{
    ASSERT(HasFrame());
    auto type = GetFrameType();
    switch (type) {
        case FrameType::OPTIMIZED_FRAME: {
            auto framehandle =
                reinterpret_cast<OptimizedFrameHandler *>(this);
            framehandle->PrevFrame();
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME: {
            auto framehandle =
                reinterpret_cast<OptimizedEntryFrameHandler *>(this);
            framehandle->PrevFrame();
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto framehandle =
                reinterpret_cast<InterpretedFrameHandler *>(this);
            framehandle->PrevFrame();
            break;
        }
        case FrameType::ASM_LEAVE_FRAME:
        case FrameType::OPTIMIZED_LEAVE_FRAME: {
            auto framehandle =
                reinterpret_cast<OptimizedLeaveFrameHandler *>(this);
            framehandle->PrevFrame();
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME: {
            auto framehandle = reinterpret_cast<InterpretedEntryFrameHandler *>(this);
            framehandle->PrevFrame();
            break;
        }
        default:
            UNREACHABLE();
    }
}

InterpretedFrameHandler::InterpretedFrameHandler(JSThread *thread)
    : FrameHandler(const_cast<JSTaggedType *>(thread->GetCurrentSPFrame()))
{
}

void InterpretedFrameHandler::PrevFrame()
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    sp_ = frame->base.prev;
}

void InterpretedFrameHandler::PrevInterpretedFrame()
{
    FrameHandler::PrevFrame();
    for (; HasFrame() && !(IsInterpretedFrame() || IsInterpretedEntryFrame()); FrameHandler::PrevFrame());
}

InterpretedFrameHandler InterpretedFrameHandler::GetPrevFrame() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return InterpretedFrameHandler(frame->base.prev);
}

JSTaggedValue InterpretedFrameHandler::GetVRegValue(size_t index) const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return JSTaggedValue(sp_[index]);
}

void InterpretedFrameHandler::SetVRegValue(size_t index, JSTaggedValue value)
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    sp_[index] = value.GetRawData();
}

JSTaggedValue InterpretedFrameHandler::GetAcc() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->acc;
}

uint32_t InterpretedFrameHandler::GetSize() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    JSTaggedType *prevSp = frame->base.prev;
    ASSERT(prevSp != nullptr);
    // The prev frame of InterpretedFrame may entry frame or interpreter frame.
    if (FrameHandler(prevSp).GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME) {
        return static_cast<uint32_t>((prevSp - sp_) - INTERPRETER_ENTRY_FRAME_STATE_SIZE);
    } else {
        return static_cast<uint32_t>((prevSp - sp_) - INTERPRETER_FRAME_STATE_SIZE);
    }
}

uint32_t InterpretedFrameHandler::GetBytecodeOffset() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    JSMethod *method = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget();
    auto offset = frame->pc - method->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *InterpretedFrameHandler::GetMethod() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget();
}

JSTaggedValue InterpretedFrameHandler::GetFunction() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->function;
}

const uint8_t *InterpretedFrameHandler::GetPc() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->pc;
}

JSTaggedType *InterpretedFrameHandler::GetSp() const
{
    ASSERT(HasFrame());
    return sp_;
}

ConstantPool *InterpretedFrameHandler::GetConstpool() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
    JSTaggedValue function = frame->function;
    if (function.IsJSFunction()) {
        JSTaggedValue constpool = JSFunction::Cast(function.GetTaggedObject())->GetConstantPool();
        return ConstantPool::Cast(constpool.GetTaggedObject());
    } else {
        return nullptr;
    }
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
    return ConstantPool::Cast(frame->constpool.GetTaggedObject());
#endif
}

JSTaggedValue InterpretedFrameHandler::GetEnv() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    return frame->env;
}

void InterpretedFrameHandler::SetEnv(JSTaggedValue env)
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp_);
#endif
    frame->env = env;
}

void InterpretedFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = sp_;
    if (current == nullptr) {
        return;
    }

#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
    AsmInterpretedFrame *frame = reinterpret_cast<AsmInterpretedFrame *>(current) - 1;
    if (frame->function != JSTaggedValue::Hole()) {
        uintptr_t start = ToUintPtr(current);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        AsmInterpretedFrame *prev_frame = reinterpret_cast<AsmInterpretedFrame *>(frame->base.prev) - 1;
        uintptr_t end = ToUintPtr(prev_frame);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
        if (frame->pc != nullptr) {
            // interpreter frame
            v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
            v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        }
    }
#else
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(current);
    if (frame->sp == nullptr) {
        return;
    }

    uintptr_t start = ToUintPtr(current);
    uintptr_t end = 0U;
    FrameType type = FrameHandler(frame->base.prev).GetFrameType();
    if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
        InterpretedFrame *prevFrame = InterpretedFrame::GetFrameFromSp(frame->base.prev);
        end = ToUintPtr(prevFrame);
    } else if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
        JSTaggedType *prevSp = frame->base.prev;
        int32_t argc = InterpretedEntryFrameHandler(prevSp).GetArgsNumber();
        end = ToUintPtr(prevSp - INTERPRETER_ENTRY_FRAME_STATE_SIZE - 1 - argc - RESERVED_CALL_ARGCOUNT);
    } else {
        LOG_ECMA(FATAL) << "frame type error!";
        UNREACHABLE();
    }
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    if (frame->pc != nullptr) {
        // interpreter frame
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->constpool)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->profileTypeInfo)));
    }
#endif
}

void InterpretedFrameHandler::DumpStack(std::ostream &os) const
{
    size_t i = 0;
    InterpretedFrameHandler frameHandler(sp_);
    for (; frameHandler.HasFrame(); frameHandler.PrevFrame()) {
        os << "[" << i++
           << "]:" << frameHandler.GetMethod()->ParseFunctionName()
           << "\n";
    }
}

void InterpretedFrameHandler::DumpPC(std::ostream &os, const uint8_t *pc) const
{
    InterpretedFrameHandler frameHandler(sp_);
    ASSERT(frameHandler.HasFrame());

    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
    int offset = pc - JSMethod::Cast(frameHandler.GetMethod())->GetBytecodeArray();
    os << "offset: " << offset << "\n";
}

// -----------------------------------------------InterpretedEntryFrameHandler------------------------------------------
InterpretedEntryFrameHandler::InterpretedEntryFrameHandler(JSThread *thread)
    : FrameHandler(const_cast<JSTaggedType *>(thread->GetCurrentSPFrame())) {}

void InterpretedEntryFrameHandler::PrevFrame()
{
    ASSERT(HasFrame());
    InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(sp_);
    sp_ = frame->base.prev;
}

int32_t InterpretedEntryFrameHandler::GetArgsNumber()
{
    JSTaggedType *argcSp = sp_ - INTERPRETER_ENTRY_FRAME_STATE_SIZE - 1;
    return argcSp[0];
}

void InterpretedEntryFrameHandler::Iterate([[maybe_unused]] const RootVisitor &v0, const RootRangeVisitor &v1)
{
    if (sp_ == nullptr) {
        return;
    }

    int32_t argc = GetArgsNumber();
    uintptr_t start = ToUintPtr(sp_ - INTERPRETER_ENTRY_FRAME_STATE_SIZE - 1 - argc - RESERVED_CALL_ARGCOUNT);
    uintptr_t end = ToUintPtr(sp_ - INTERPRETER_ENTRY_FRAME_STATE_SIZE);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

void OptimizedFrameHandler::PrevFrame()
{
    OptimizedFrame *frame = OptimizedFrame::GetFrameFromSp(sp_);
    sp_ = reinterpret_cast<JSTaggedType *>(frame->base.prevFp);
}

void OptimizedFrameHandler::Iterate(const RootVisitor &v0, [[maybe_unused]] const RootRangeVisitor &v1,
                                    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const
{
    if (sp_ != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::set<uintptr_t> slotAddrs;
        auto returnAddr = reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(sp_) + 1));
        bool enableCompilerLog = thread_->GetEcmaVM()->GetJSOptions().WasSetlogCompiledMethods();
        bool ret = kungfu::LLVMStackMapParser::GetInstance(enableCompilerLog).CollectStackMapSlots(
            returnAddr, reinterpret_cast<uintptr_t>(sp_), slotAddrs, derivedPointers, isVerifying);
        if (ret == false) {
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
            return;
        }
        for (const auto &slot : slotAddrs) {
            v0(Root::ROOT_FRAME, ObjectSlot(slot));
        }
    }
}

void OptimizedEntryFrameHandler::Iterate(const RootVisitor &v0, [[maybe_unused]] const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const
{
    if (sp_ != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::set<uintptr_t> slotAddrs;
        auto returnAddr = reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(sp_) + 1));
        bool enableCompilerLog = thread_->GetEcmaVM()->GetJSOptions().WasSetlogCompiledMethods();
        bool ret = kungfu::LLVMStackMapParser::GetInstance(enableCompilerLog).CollectStackMapSlots(
            returnAddr, reinterpret_cast<uintptr_t>(sp_), slotAddrs, derivedPointers, isVerifying);
        if (ret == false) {
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
            return;
        }
        for (const auto &slot : slotAddrs) {
            v0(Root::ROOT_FRAME, ObjectSlot(slot));
        }
    }
}

void OptimizedEntryFrameHandler::PrevFrame()
{
    OptimizedEntryFrame *frame = OptimizedEntryFrame::GetFrameFromSp(sp_);
    sp_ = reinterpret_cast<JSTaggedType *>(frame->preLeaveFrameFp);
}

void OptimizedLeaveFrameHandler::PrevFrame()
{
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp_);
    sp_ = reinterpret_cast<JSTaggedType *>(frame->callsiteFp);
}

void OptimizedLeaveFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const
{
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp_);
    if (frame->argc > 0) {
        uintptr_t start = ToUintPtr(&frame->argc + 1); // argv
        uintptr_t end = ToUintPtr(&frame->argc + 1 + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
    std::set<uintptr_t> slotAddrs;
    bool enableCompilerLog = thread_->GetEcmaVM()->GetJSOptions().WasSetlogCompiledMethods();
    bool ret = kungfu::LLVMStackMapParser::GetInstance(enableCompilerLog).CollectStackMapSlots(
        frame->returnAddr, reinterpret_cast<uintptr_t>(sp_), slotAddrs, derivedPointers, isVerifying);
    if (ret == false) {
        return;
    }
    for (auto slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

void FrameIterator::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers = thread_->GetEcmaVM()->GetHeap()->GetDerivedPointers();
    bool isVerifying = false;

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    isVerifying = thread_->GetEcmaVM()->GetHeap()->GetIsVerifying();
#endif

    // asm interpreter leaveframe
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    if (current == nullptr) {
        // c++ interpreter frame
        current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    }
    while (current) {
        FrameType type = FrameHandler(current).GetFrameType();
        if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
            AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(current);
#else
            InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(current);
#endif
            InterpretedFrameHandler(current).Iterate(v0, v1);
            current = frame->GetPrevFrameFp();
        } else if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
            InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(current);
            InterpretedEntryFrameHandler(current).Iterate(v0, v1);
            current = frame->GetPrevFrameFp();
        } else if (type == FrameType::OPTIMIZED_FRAME) {
            OptimizedFrame *frame = OptimizedFrame::GetFrameFromSp(current);
            OptimizedFrameHandler(thread_, reinterpret_cast<uintptr_t *>(current)).Iterate(v0, v1, derivedPointers,
                isVerifying);
            current = frame->GetPrevFrameFp();
        } else if (type == FrameType::OPTIMIZED_ENTRY_FRAME) {
            OptimizedEntryFrame *frame = OptimizedEntryFrame::GetFrameFromSp(current);
            current = frame->GetPrevFrameFp();
            // NOTE: due to "AotInfo" iteration, current frame might not be interpreted frame
        } else {
            ASSERT(type == FrameType::OPTIMIZED_LEAVE_FRAME || type == FrameType::ASM_LEAVE_FRAME);
            OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(current);
            OptimizedLeaveFrameHandler(thread_, reinterpret_cast<uintptr_t *>(current)).Iterate(v0,
                v1, derivedPointers, isVerifying);
            //  arm32, arm64 and x86_64 support stub and aot, when aot/stub call runtime, generate Optimized
            // Leave Frame.
            current = reinterpret_cast<JSTaggedType *>(frame->callsiteFp);
            ASSERT(FrameHandler(current).GetFrameType() == FrameType::OPTIMIZED_ENTRY_FRAME ||
            FrameHandler(current).GetFrameType() == FrameType::OPTIMIZED_FRAME ||
            FrameHandler(current).GetFrameType() == FrameType::INTERPRETER_FRAME ||
            FrameHandler(current).GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
        }
    }
}
}  // namespace panda::ecmascript
