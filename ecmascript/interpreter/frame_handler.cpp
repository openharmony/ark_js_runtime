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
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_thread.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"

namespace panda::ecmascript {
InterpretedFrameHandler::InterpretedFrameHandler(const JSThread *thread)
{
    sp_ = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
}

bool InterpretedFrameHandler::HasFrame() const
{
    // Breakframe also is a frame
    return sp_ != nullptr;
}

bool InterpretedFrameHandler::IsBreakFrame() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->sp == nullptr;
}

void InterpretedFrameHandler::PrevFrame()
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    sp_ = state->base.prev;
}

InterpretedFrameHandler InterpretedFrameHandler::GetPrevFrame() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
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
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->acc;
}

uint32_t InterpretedFrameHandler::GetSize() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    JSTaggedType *prevSp = state->base.prev;
    ASSERT(prevSp != nullptr);
    auto size = (prevSp - sp_) - FRAME_STATE_SIZE;
    return static_cast<uint32_t>(size);
}

uint32_t InterpretedFrameHandler::GetBytecodeOffset() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto offset = state->pc - JSMethod::Cast(state->method)->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *InterpretedFrameHandler::GetMethod() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->method;
}

JSTaggedValue InterpretedFrameHandler::GetFunction() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    uint32_t numVregs = state->method->GetNumVregs();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto func = JSTaggedValue(sp_[numVregs]);
    return func;
}

const uint8_t *InterpretedFrameHandler::GetPc() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->pc;
}

JSTaggedType *InterpretedFrameHandler::GetSp() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->sp;
}

ConstantPool *InterpretedFrameHandler::GetConstpool() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->constpool;
}

JSTaggedValue InterpretedFrameHandler::GetEnv() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->env;
}

void InterpretedFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = sp_;
    if (current != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        FrameState *state = reinterpret_cast<FrameState *>(current) - 1;

        if (state->sp != nullptr) {
            uintptr_t start = ToUintPtr(current);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            FrameState *prev_state = reinterpret_cast<FrameState *>(state->base.prev) - 1;
            uintptr_t end = ToUintPtr(prev_state);
            v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
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

void OptimizedFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    uintptr_t *current = fp_;
    if (current != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::set<uintptr_t> slotAddrs;
        auto returnAddr = reinterpret_cast<uintptr_t>(*(current + 1));
        LOG_ECMA(INFO) << __FUNCTION__ << " returnAddr :" << returnAddr << std::endl;
        bool ret = kungfu::LLVMStackMapParser::GetInstance().StackMapByFuncAddrFp(
            returnAddr,
            reinterpret_cast<uintptr_t>(fp_),
            slotAddrs);
        if (ret == false) {
            return;
        }
        for (auto &address: slotAddrs) {
            v0(Root::ROOT_FRAME, ObjectSlot(address));
        }
    }
}

void OptimizedEntryFrameHandler::Iterate([[maybe_unused]] const RootVisitor &v0,
    [[maybe_unused]] const RootRangeVisitor &v1) const
{
    // Entry Frame return address's callsite already visited by OptimizedFrameHandler
    // or HandleRuntimeTrampolines
}

void FrameIterator::HandleRuntimeTrampolines(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    if (thread_) {
        uintptr_t *fp = thread_->GetLastOptCallRuntimePc();
        if (fp == nullptr) {
            return;
        }
        std::set<uintptr_t> slotAddrs;
        auto returnAddr = *(fp + 1);
        LOG_ECMA(INFO) << __FUNCTION__ << " returnAddr :" << returnAddr << " fp: " << fp << std::endl;
        bool ret = kungfu::LLVMStackMapParser::GetInstance().StackMapByFuncAddrFp(
            reinterpret_cast<uintptr_t>(returnAddr),
            reinterpret_cast<uintptr_t>(fp),
            slotAddrs);
        if (ret == false) {
            LOG_ECMA(INFO) << " stackmap don't found returnAddr " << std::endl;
            return;
        }
        for (auto &address: slotAddrs) {
            LOG_ECMA(INFO) << "stackmap address : " << std::hex << address << std::endl;
            v0(Root::ROOT_FRAME, ObjectSlot(address));
        }
    }
}


void FrameIterator::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    JSTaggedType *current = fp_;
    // handle runtimeTrampolines Frame in order get stub returnAddress which used by
    // stackMap
    HandleRuntimeTrampolines(v0, v1);
    while (current) {
        FrameType type = Frame::GetFrameType(current);
        LOG_ECMA(INFO) << __FUNCTION__ << "type = " << static_cast<uint64_t>(type) << std::endl;
        if (type == FrameType::INTERPRETER_FRAME) {
            FrameState *state = reinterpret_cast<FrameState *>(current) - 1;
            InterpretedFrameHandler(current).Iterate(v0, v1);
            current = state->base.prev;
        } else if (type == FrameType::OPTIMIZED_FRAME) {
            OptimizedFrameStateBase *state = reinterpret_cast<OptimizedFrameStateBase *>(
                reinterpret_cast<intptr_t>(current) -
                MEMBER_OFFSET(OptimizedFrameStateBase, prev));
            OptimizedFrameHandler(reinterpret_cast<uintptr_t *>(current)).Iterate(v0, v1);
            current = reinterpret_cast<JSTaggedType *>(state->prev);
        } else {
            ASSERT(type == FrameType::OPTIMIZED_ENTRY_FRAME);
            OptimizedEntryFrameState *state = reinterpret_cast<OptimizedEntryFrameState *>(
                reinterpret_cast<intptr_t>(current) -
                MEMBER_OFFSET(OptimizedEntryFrameState, base.prev));
            OptimizedEntryFrameHandler(reinterpret_cast<uintptr_t *>(current)).Iterate(v0, v1);
            current = reinterpret_cast<JSTaggedType *>(state->threadFp);
        }
    }
}
}  // namespace panda::ecmascript
