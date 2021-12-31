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

#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/mem/heap.h"
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
        case FrameType::INTERPRETER_FRAME: {
            auto framehandle =
                reinterpret_cast<InterpretedFrameHandler *>(this);
            framehandle->PrevFrame();
            break;
        }
        case FrameType::OPTIMIZED_LEAVE_FRAME: {
            auto framehandle =
                reinterpret_cast<OptimizedLeaveFrameHandler *>(this);
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
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    sp_ = state->base.prev;
}

void InterpretedFrameHandler::PrevInterpretedFrame()
{
    FrameHandler::PrevFrame();
    for (; HasFrame() && GetFrameType() != FrameType::INTERPRETER_FRAME; FrameHandler::PrevFrame());
}

InterpretedFrameHandler InterpretedFrameHandler::GetPrevFrame() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return InterpretedFrameHandler(state->base.prev);
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
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return state->acc;
}

uint32_t InterpretedFrameHandler::GetSize() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    JSTaggedType *prevSp = state->base.prev;
    ASSERT(prevSp != nullptr);
    auto size = (prevSp - sp_) - FRAME_STATE_SIZE;
    return static_cast<uint32_t>(size);
}

uint32_t InterpretedFrameHandler::GetBytecodeOffset() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    JSMethod *method = ECMAObject::Cast(state->function.GetTaggedObject())->GetCallTarget();
    auto offset = state->pc - method->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *InterpretedFrameHandler::GetMethod() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return ECMAObject::Cast(state->function.GetTaggedObject())->GetCallTarget();
}

JSTaggedValue InterpretedFrameHandler::GetFunction() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return state->function;
}

const uint8_t *InterpretedFrameHandler::GetPc() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return state->pc;
}

JSTaggedType *InterpretedFrameHandler::GetSp() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return state->sp;
}

ConstantPool *InterpretedFrameHandler::GetConstpool() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return ConstantPool::Cast(state->constpool.GetTaggedObject());
}

JSTaggedValue InterpretedFrameHandler::GetEnv() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
    return state->env;
}

void InterpretedFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = sp_;
    if (current != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(current) - 1;

        if (state->sp != nullptr) {
            uintptr_t start = ToUintPtr(current);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            InterpretedFrame *prev_state = reinterpret_cast<InterpretedFrame *>(state->base.prev) - 1;
            uintptr_t end = ToUintPtr(prev_state);
            v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
            v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&state->function)));
            if (state->pc != nullptr) {
                // interpreter frame
                v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&state->acc)));
                v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&state->constpool)));
                v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&state->env)));
                v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&state->profileTypeInfo)));
            }
        }
    }
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

void OptimizedFrameHandler::PrevFrame()
{
    OptimizedFrame *state = OptimizedFrame::GetFrameFromSp(sp_);
    sp_ = reinterpret_cast<JSTaggedType *>(state->prev);
}

void OptimizedFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
                                    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const
{
    if (sp_ != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::set<uintptr_t> slotAddrs;
        auto returnAddr = reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(sp_) + 1));
        bool ret = ::kungfu::LLVMStackMapParser::GetInstance().GetStackMapIterm(
            returnAddr, reinterpret_cast<uintptr_t>(sp_), v0, v1, derivedPointers, isVerifying);
        if (ret == false) {
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
            return;
        }
    }
}

void OptimizedEntryFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
                                    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const
{
    if (sp_ != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::set<uintptr_t> slotAddrs;
        auto returnAddr = reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(sp_) + 1));
        bool ret = ::kungfu::LLVMStackMapParser::GetInstance().GetStackMapIterm(
            returnAddr, reinterpret_cast<uintptr_t>(sp_), v0, v1, derivedPointers, isVerifying);
        if (ret == false) {
#ifndef NDEBUG
            LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
            return;
        }
    }
}

void OptimizedEntryFrameHandler::PrevFrame()
{
    OptimizedEntryFrame *state = OptimizedEntryFrame::GetFrameFromSp(sp_);
    sp_ = reinterpret_cast<JSTaggedType *>(state->threadFp);
}

void OptimizedLeaveFrameHandler::PrevFrame()
{
    OptLeaveFrame *state = reinterpret_cast<OptLeaveFrame *>(
        reinterpret_cast<uintptr_t>(sp_) - MEMBER_OFFSET(OptLeaveFrame, prev));
    sp_ = reinterpret_cast<JSTaggedType *>(state->prev);
}

void OptimizedLeaveFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
                                    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const
{
    OptLeaveFrame *state = reinterpret_cast<OptLeaveFrame *>(
        reinterpret_cast<intptr_t>(sp_) - MEMBER_OFFSET(OptLeaveFrame, prev));
    bool ret = ::kungfu::LLVMStackMapParser::GetInstance().GetStackMapIterm(
        state, v0, v1, derivedPointers, isVerifying);
    if (ret == false) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found patchPointId " << state->patchId;
#endif
        return;
    }
}

void FrameIterator::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers = thread_->GetEcmaVM()->GetHeap()->GetDerivedPointers();
    bool isVerifying = false;

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    isVerifying = thread_->GetEcmaVM()->GetHeap()->GetIsVerifying();
#endif

    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    while (current) {
        FrameType type = FrameHandler(current).GetFrameType();
        if (type == FrameType::INTERPRETER_FRAME) {
            InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(current);
            InterpretedFrameHandler(current).Iterate(v0, v1);
            current = state->base.prev;
        } else if (type == FrameType::OPTIMIZED_FRAME) {
            OptimizedFrame *state = OptimizedFrame::GetFrameFromSp(current);
            OptimizedFrameHandler(reinterpret_cast<uintptr_t *>(current)).Iterate(v0, v1, derivedPointers, isVerifying);
            current = reinterpret_cast<JSTaggedType *>(state->prev);
        } else if (type == FrameType::OPTIMIZED_ENTRY_FRAME) {
            OptimizedEntryFrame *state = OptimizedEntryFrame::GetFrameFromSp(current);
                current = reinterpret_cast<JSTaggedType *>(state->threadFp);
        } else {
            ASSERT(type == FrameType::OPTIMIZED_LEAVE_FRAME);
            OptLeaveFrame *state = OptLeaveFrame::GetFrameFromSp(current);
            OptimizedLeaveFrameHandler(reinterpret_cast<uintptr_t *>(current)).Iterate(v0,
                v1, derivedPointers, isVerifying);
            current = reinterpret_cast<JSTaggedType *>(state->prev);
        }
    }
}
}  // namespace panda::ecmascript
