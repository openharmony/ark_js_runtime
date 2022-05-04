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

#ifndef ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
#define ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H

#include "ecmascript/frames.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/visitor.h"

namespace panda {
namespace ecmascript {
class JSThread;
class ConstantPool;

class FrameHandler {
public:
    explicit FrameHandler(const JSThread *thread)
        : sp_(const_cast<JSTaggedType *>(thread->GetCurrentSPFrame())), thread_(thread) {}
    ~FrameHandler() = default;

    DEFAULT_COPY_SEMANTIC(FrameHandler);
    DEFAULT_MOVE_SEMANTIC(FrameHandler);

    bool HasFrame() const
    {
        return sp_ != nullptr;
    }

    inline static FrameType GetFrameType(const JSTaggedType *sp)
    {
        ASSERT(sp != nullptr);
        FrameType *typeAddr = reinterpret_cast<FrameType *>(reinterpret_cast<uintptr_t>(sp) - sizeof(FrameType));
        return *typeAddr;
    }

    inline static bool IsEntryFrame(const uint8_t *pc)
    {
        return pc == nullptr;
    }

    bool IsEntryFrame() const
    {
        ASSERT(HasFrame());
        // The structure of InterpretedFrame, AsmInterpretedFrame, InterpretedEntryFrame is the same, order is pc, base.
        InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
        return state->pc == nullptr;
    }

    bool IsInterpretedFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::INTERPRETER_FRAME) || (type == FrameType::INTERPRETER_FAST_NEW_FRAME);
    }

    bool IsInterpretedEntryFrame() const
    {
        return (GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsLeaveFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::LEAVE_FRAME) || (type == FrameType::LEAVE_FRAME_WITH_ARGV);
    }

    JSTaggedType *GetSp() const
    {
        return sp_;
    }

    void PrevInterpretedFrame();
    JSTaggedType *GetPrevInterpretedFrame();

    // for llvm.
    static uintptr_t GetPrevFrameCallSiteSp(const JSTaggedType *sp);

    // for InterpretedFrame.
    JSTaggedValue GetVRegValue(size_t index) const;
    void SetVRegValue(size_t index, JSTaggedValue value);

    JSTaggedValue GetEnv() const;
    void SetEnv(JSTaggedValue env);

    JSTaggedValue GetAcc() const;
    uint32_t GetSize() const;
    uint32_t GetBytecodeOffset() const;
    JSMethod *GetMethod() const;
    JSTaggedValue GetFunction() const;
    const uint8_t *GetPc() const;
    ConstantPool *GetConstpool() const;

    void DumpStack(std::ostream &os) const;
    void DumpStack() const
    {
        DumpStack(std::cout);
    }

    void DumpPC(std::ostream &os, const uint8_t *pc) const;
    void DumpPC(const uint8_t *pc) const
    {
        DumpPC(std::cout, pc);
    }

    // for InterpretedEntryFrame.
    static JSTaggedType* GetInterpretedEntryFrameStart(const JSTaggedType *sp);

    // for Frame GC.
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const;

private:
    FrameType GetFrameType() const
    {
        ASSERT(HasFrame());
        FrameType *typeAddr = reinterpret_cast<FrameType *>(reinterpret_cast<uintptr_t>(sp_) - sizeof(FrameType));
        return *typeAddr;
    }

    void PrevFrame();

    // for Frame GC.
    void InterpretedFrameIterate(const JSTaggedType *sp, const RootVisitor &v0, const RootRangeVisitor &v1) const;
    void InterpretedEntryFrameIterate(const JSTaggedType *sp, const RootVisitor &v0, const RootRangeVisitor &v1) const;
    void OptimizedFrameIterate(
        const JSTaggedType *sp, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
    void OptimizedEntryFrameIterate(
        const JSTaggedType *sp, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
    void OptimizedLeaveFrameIterate(
        const JSTaggedType *sp, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;
    void OptimizedWithArgvLeaveFrameIterate(
        const JSTaggedType *sp, const RootVisitor &v0, const RootRangeVisitor &v1,
        ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers, bool isVerifying) const;

private:
    JSTaggedType *sp_ {nullptr};
    const JSThread *thread_ {nullptr};
};
} // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
