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

#include <cstdint>

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_thread.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"

namespace panda::ecmascript {
EcmaFrameHandler::EcmaFrameHandler(const JSThread *thread)
{
    sp_ = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
}

bool EcmaFrameHandler::HasFrame() const
{
    // Breakframe also is a frame
    return sp_ != nullptr;
}

bool EcmaFrameHandler::IsBreakFrame() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->sp == nullptr;
}

void EcmaFrameHandler::PrevFrame()
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    sp_ = state->base.prev;
}

EcmaFrameHandler EcmaFrameHandler::GetPrevFrame() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return EcmaFrameHandler(state->base.prev);
}

JSTaggedValue EcmaFrameHandler::GetVRegValue(size_t index) const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return JSTaggedValue(sp_[index]);
}

void EcmaFrameHandler::SetVRegValue(size_t index, JSTaggedValue value)
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    sp_[index] = value.GetRawData();
}

JSTaggedValue EcmaFrameHandler::GetAcc() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->acc;
}

uint32_t EcmaFrameHandler::GetSize() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    JSTaggedType *prevSp = state->base.prev;
    ASSERT(prevSp != nullptr);
    auto size = (prevSp - sp_) - FRAME_STATE_SIZE;
    return static_cast<uint32_t>(size);
}

uint32_t EcmaFrameHandler::GetBytecodeOffset() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto offset = state->pc - JSMethod::Cast(state->method)->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *EcmaFrameHandler::GetMethod() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->method;
}

JSTaggedValue EcmaFrameHandler::GetFunction() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    uint32_t numVregs = state->method->GetNumVregs();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto func = JSTaggedValue(sp_[numVregs]);
    return func;
}

const uint8_t *EcmaFrameHandler::GetPc() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->pc;
}

JSTaggedType *EcmaFrameHandler::GetSp() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->sp;
}

ConstantPool *EcmaFrameHandler::GetConstpool() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->constpool;
}

JSTaggedValue EcmaFrameHandler::GetEnv() const
{
    ASSERT(HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
    return state->env;
}

void EcmaFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
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

void EcmaFrameHandler::DumpStack(std::ostream &os) const
{
    size_t i = 0;
    EcmaFrameHandler frameHandler(sp_);
    for (; frameHandler.HasFrame(); frameHandler.PrevFrame()) {
        os << "[" << i++
           << "]:" << frameHandler.GetMethod()->ParseFunctionName()
           << "\n";
    }
}

void EcmaFrameHandler::DumpPC(std::ostream &os, const uint8_t *pc) const
{
    EcmaFrameHandler frameHandler(sp_);
    ASSERT(frameHandler.HasFrame());

    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
    int offset = pc - JSMethod::Cast(frameHandler.GetMethod())->GetBytecodeArray();
    os << "offset: " << offset << "\n";
}

void OptimizedFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    JSTaggedType *current = fp_;
    if (current != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::vector<uintptr_t> slotAddrs;
        auto returnAddr = reinterpret_cast<uintptr_t>(*(current + 1));
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

void OptimizedEntryFrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    JSTaggedType *current = fp_;
    if (current != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::vector<uintptr_t> slotAddrs;
        auto returnAddr = *(current + 1);
        bool ret = kungfu::LLVMStackMapParser::GetInstance().StackMapByFuncAddrFp(
            reinterpret_cast<uintptr_t>(returnAddr),
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

void FrameIterator::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    JSTaggedType *current = fp_;
    while (current) {
        FrameType type = *(reinterpret_cast<FrameType*>(
                        reinterpret_cast<long long>(current) + FrameConst::kFrameType));
        if (type == FrameType::INTERPRETER_FRAME) {
            FrameState *state = reinterpret_cast<FrameState *>(current) - 1;
            EcmaFrameHandler(current).Iterate(v0, v1);
            current = state->base.prev;
        } else if (type == FrameType::OPTIMIZED_FRAME) {
            OptFrameStateBase *state = reinterpret_cast<OptFrameStateBase *>(
                reinterpret_cast<long long>(current) -
                MEMBER_OFFSET(OptFrameStateBase, prev));
            OptimizedFrameHandler(current).Iterate(v0, v1);
            current = reinterpret_cast<JSTaggedType *>(state->prev);
        } else {
            LLVMOptimizedEntryFrameState *state = reinterpret_cast<LLVMOptimizedEntryFrameState *>(
                reinterpret_cast<long long>(current) -
                MEMBER_OFFSET(LLVMOptimizedEntryFrameState, base.prev));
            OptimizedEntryFrameHandler(current).Iterate(v0, v1);
            current = reinterpret_cast<JSTaggedType *>(state->threadFp);
        }
    }
}
}  // namespace panda::ecmascript
