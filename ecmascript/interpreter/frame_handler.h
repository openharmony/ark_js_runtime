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

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/frames.h"

namespace panda {
namespace ecmascript {
class JSThread;
class JSFunction;
class ConstantPool;

class FrameHandler {
public:
    explicit FrameHandler(JSTaggedType *sp) : sp_(sp) {}
    ~FrameHandler() = default;
    DEFAULT_COPY_SEMANTIC(FrameHandler);
    DEFAULT_MOVE_SEMANTIC(FrameHandler);
    static constexpr uintptr_t kFrameTypeOffset = -sizeof(FrameType);

    bool HasFrame() const
    {
        // Breakframe also is a frame
        return sp_ != nullptr;
    }

    bool IsEntryFrame() const
    {
        ASSERT(HasFrame());
#if ECMASCRIPT_COMPILE_ASM_INTERPRETER
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        AsmInterpretedFrame *state = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return state->function == JSTaggedValue::Hole();
#else
        // interpreter entry frame or interpreter frame which sp is nullptr.
        FrameType type = GetFrameType();
        if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
            return true;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        InterpretedFrame *state = InterpretedFrame::GetFrameFromSp(sp_);
        return state->sp == nullptr;
#endif
    }

    void PrevFrame();

    FrameType GetFrameType() const
    {
        ASSERT(HasFrame());
        FrameType* typeAddr = reinterpret_cast<FrameType*>(reinterpret_cast<uintptr_t>(sp_) + kFrameTypeOffset);
        return *typeAddr;
    }

    bool IsInterpretedFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::INTERPRETER_FRAME) || (type == FrameType::INTERPRETER_FAST_NEW_FRAME);
    }

    bool IsInterpretedEntryFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsOptimizedLeaveFrame() const
    {
        return GetFrameType() == FrameType::OPTIMIZED_LEAVE_FRAME || GetFrameType() == FrameType::ASM_LEAVE_FRAME;
    }

    OptimizedLeaveFrame* GetOptimizedLeaveFrame() const
    {
        assert(IsOptimizedLeaveFrame());
        return OptimizedLeaveFrame::GetFrameFromSp(sp_);
    }

protected:
    JSTaggedType *sp_ {nullptr};
};

class InterpretedFrameHandler : public FrameHandler {
public:
    explicit InterpretedFrameHandler(JSThread *thread);
    explicit InterpretedFrameHandler(JSTaggedType *sp) : FrameHandler(sp) {}
    DEFAULT_COPY_SEMANTIC(InterpretedFrameHandler);
    DEFAULT_MOVE_SEMANTIC(InterpretedFrameHandler);

    void PrevFrame();
    void PrevInterpretedFrame();
    InterpretedFrameHandler GetPrevFrame() const;

    JSTaggedValue GetVRegValue(size_t index) const;
    void SetVRegValue(size_t index, JSTaggedValue value);

    JSTaggedValue GetAcc() const;
    uint32_t GetSize() const;
    uint32_t GetBytecodeOffset() const;
    JSMethod *GetMethod() const;
    JSTaggedValue GetFunction() const;
    const uint8_t *GetPc() const;
    JSTaggedType *GetSp() const;
    ConstantPool *GetConstpool() const;
    JSTaggedValue GetEnv() const;

    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1);

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
};

class InterpretedEntryFrameHandler : public FrameHandler {
public:
    explicit InterpretedEntryFrameHandler(JSThread *thread);
    explicit InterpretedEntryFrameHandler(JSTaggedType *sp) : FrameHandler(sp) {}
    DEFAULT_COPY_SEMANTIC(InterpretedEntryFrameHandler);
    DEFAULT_MOVE_SEMANTIC(InterpretedEntryFrameHandler);

    void PrevFrame();
    int32_t GetArgsNumber();
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1);
};

class OptimizedFrameHandler : public FrameHandler {
public:
    explicit OptimizedFrameHandler(uintptr_t *sp)
        : FrameHandler(reinterpret_cast<JSTaggedType *>(sp)) {}
    explicit OptimizedFrameHandler(const JSThread *thread);
    ~OptimizedFrameHandler() = default;
    void PrevFrame();
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
                ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                 bool isVerifying) const;
};

class OptimizedEntryFrameHandler : public FrameHandler {
public:
    explicit OptimizedEntryFrameHandler(uintptr_t *sp)
        : FrameHandler(reinterpret_cast<JSTaggedType *>(sp)) {}
    explicit OptimizedEntryFrameHandler(const JSThread *thread);
    ~OptimizedEntryFrameHandler() = default;
    void PrevFrame();
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
                ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                bool isVerifying) const;
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1);
};

class OptimizedLeaveFrameHandler : public FrameHandler {
public:
    explicit OptimizedLeaveFrameHandler(uintptr_t *sp)
        : FrameHandler(reinterpret_cast<JSTaggedType *>(sp)) {}
    explicit OptimizedLeaveFrameHandler(const JSThread *thread);
    ~OptimizedLeaveFrameHandler() = default;
    void PrevFrame();
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1,
                ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                bool isVerifying) const;
};

class FrameIterator {
public:
    explicit FrameIterator(JSTaggedType *sp, const JSThread *thread) : sp_(sp), thread_(thread) {}
    ~FrameIterator() = default;
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const;
    void IterateStackMapAfterGC() const;
private:
    JSTaggedType *sp_ {nullptr};
    const JSThread *thread_ {nullptr};
};
} // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
