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

#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/heap_roots.h"

namespace panda {
namespace ecmascript {
class JSThread;
class JSFunction;
class ConstantPool;

class InterpretedFrameHandler {
public:
    explicit InterpretedFrameHandler(JSTaggedType *sp) : sp_(sp) {}
    explicit InterpretedFrameHandler(const JSThread *thread);
    ~InterpretedFrameHandler() = default;
    DEFAULT_COPY_SEMANTIC(InterpretedFrameHandler);
    DEFAULT_MOVE_SEMANTIC(InterpretedFrameHandler);

    bool HasFrame() const;
    bool IsBreakFrame() const;
    void PrevFrame();
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

private:
    JSTaggedType *sp_{nullptr};
};

class OptimizedFrameHandler {
public:
    explicit OptimizedFrameHandler(uintptr_t *fp) : fp_(fp) {}
    explicit OptimizedFrameHandler(const JSThread *thread);
    ~OptimizedFrameHandler() = default;
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const;
private:
    uintptr_t *fp_ {nullptr};
};

class OptimizedEntryFrameHandler {
public:
    explicit OptimizedEntryFrameHandler(uintptr_t *fp) : fp_(fp) {}
    explicit OptimizedEntryFrameHandler(const JSThread *thread);
    ~OptimizedEntryFrameHandler() = default;
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const;
private:
    uintptr_t *fp_ {nullptr};
};

class FrameIterator {
public:
    explicit FrameIterator(JSTaggedType *fp) : fp_(fp) {}
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1) const;
private:
    JSTaggedType *fp_ {nullptr};
};
} // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
