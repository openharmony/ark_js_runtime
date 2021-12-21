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
#include "ecmascript/mem/heap_roots.h"
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
    bool HasFrame() const
    {
        // Breakframe also is a frame
        return sp_ != nullptr;
    }
    bool IsBreakFrame() const
    {
        ASSERT(HasFrame());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        FrameState *state = reinterpret_cast<FrameState *>(sp_) - 1;
        return state->sp == nullptr;
    }
    void PrevFrame();

    FrameType GetFrameType() const
    {
        ASSERT(HasFrame());
        FrameType type = *(reinterpret_cast<FrameType*>(
                        reinterpret_cast<uintptr_t>(sp_) + FrameConst::FRAME_TYPE_OFFSET));
        return type;
    }
private:
    friend class InterpretedFrameHandler;
    friend class OptimizedFrameHandler;
    friend class OptimizedEntryFrameHandler;
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

class OptimizedFrameHandler : public FrameHandler {
public:
    explicit OptimizedFrameHandler(uintptr_t *fp)
        : FrameHandler(reinterpret_cast<JSTaggedType *>(fp)), fp_(fp) {}
    explicit OptimizedFrameHandler(const JSThread *thread);
    ~OptimizedFrameHandler() = default;
    void PrevFrame();
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1, ChunkVector<DerivedData> *derivedPointers,
                 bool isVerifying) const;
private:
    uintptr_t *fp_ {nullptr};
};

class OptimizedEntryFrameHandler : public FrameHandler {
public:
    explicit OptimizedEntryFrameHandler(uintptr_t *fp)
        : FrameHandler(reinterpret_cast<JSTaggedType *>(fp)), fp_(fp) {}
    explicit OptimizedEntryFrameHandler(const JSThread *thread);
    ~OptimizedEntryFrameHandler() = default;
    void PrevFrame();
private:
    uintptr_t *fp_ {nullptr};
};

class FrameIterator {
public:
    explicit FrameIterator(JSTaggedType *fp, const JSThread *thread) : fp_(fp), thread_(thread) {}
    ~FrameIterator() = default;
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const;
    void HandleRuntimeTrampolines(const RootVisitor &v0, const RootRangeVisitor &v1,
                                  ChunkVector<DerivedData> *derivedPointers, bool isVerifying) const;
    void IterateStackMapAfterGC() const;
private:
    JSTaggedType *fp_ {nullptr};
    const JSThread *thread_ {nullptr};
};
} // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
