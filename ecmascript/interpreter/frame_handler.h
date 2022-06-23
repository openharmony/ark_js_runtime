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
namespace kungfu {
    class LLVMStackMapParser;
};

class FrameHandler {
public:
    explicit FrameHandler(const JSThread *thread);
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
        FrameIterator it(sp_);
        InterpretedFrame *state = it.GetFrame<InterpretedFrame>();
        return state->GetPc() == nullptr;
    }

    bool IsInterpretedFrame() const
    {
        FrameType type = GetFrameType();
        return (type >= FrameType::INTERPRETER_BEGIN) && (type <= FrameType::INTERPRETER_END);
    }

    bool IsInterpretedFrame(FrameType type) const
    {
        return (type >= FrameType::INTERPRETER_BEGIN) && (type <= FrameType::INTERPRETER_END);
    }

    bool IsAsmInterpretedFrame() const
    {
        FrameIterator it(sp_, thread_);
        FrameType type = it.GetFrameType();
        return (type == FrameType::ASM_INTERPRETER_FRAME) ||
            (type == FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    }

    bool IsAsmInterpretedFrame(FrameType type) const
    {
        return (type == FrameType::ASM_INTERPRETER_FRAME) ||
            (type == FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    }
    bool IsBuiltinFrame() const
    {
        FrameType type = GetFrameType();
        return (type >= FrameType::BUILTIN_BEGIN) && (type <= FrameType::BUILTIN_END);
    }
    bool IsBuiltinEntryFrame() const
    {
        return (GetFrameType() == FrameType::BUILTIN_ENTRY_FRAME);
    }

    bool IsInterpretedEntryFrame() const
    {
        if (thread_->IsAsmInterpreter()) {
            FrameType type = GetFrameType();
            return (type == FrameType::ASM_INTERPRETER_ENTRY_FRAME || type == FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
        }
        return (GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsInterpretedEntryFrame(FrameType type) const
    {
        if (thread_->IsAsmInterpreter()) {
            return (type == FrameType::ASM_INTERPRETER_ENTRY_FRAME || type == FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
        }
        return (type == FrameType::INTERPRETER_ENTRY_FRAME);
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

    JSTaggedType *GetFp() const
    {
        return fp_;
    }

    void PrevInterpretedFrame();
    JSTaggedType *GetPrevInterpretedFrame();

    // for InterpretedFrame.
    JSTaggedValue GetVRegValue(size_t index) const;
    void SetVRegValue(size_t index, JSTaggedValue value);

    JSTaggedValue GetEnv() const;
    JSTaggedValue GetAcc() const;
    uint32_t GetNumberArgs();
    uint32_t GetBytecodeOffset() const;
    JSMethod *GetMethod() const;
    JSMethod *CheckAndGetMethod() const;
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
    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1);
    void IterateFrameChain(JSTaggedType *start, const RootVisitor &v0, const RootRangeVisitor &v1) const;
    void IterateRsp(const RootVisitor &v0, const RootRangeVisitor &v1);
    void IterateSp(const RootVisitor &v0, const RootRangeVisitor &v1);

    // for collecting bc offset in aot
    void CollectBCOffsetInfo();
    std::string GetAotExceptionFuncName(JSTaggedType* fp) const;

private:
    FrameType GetFrameType() const
    {
        ASSERT(HasFrame());
        FrameType *typeAddr = reinterpret_cast<FrameType *>(reinterpret_cast<uintptr_t>(sp_) - sizeof(FrameType));
        return *typeAddr;
    }

    void AdvanceToInterpretedFrame();
    uintptr_t GetInterpretedFrameEnd(JSTaggedType *prevSp) const;
private:
    JSTaggedType *sp_ {nullptr};
    JSTaggedType *fp_ {nullptr};
    const JSThread *thread_ {nullptr};
    const kungfu::LLVMStackMapParser *stackmapParser_ {nullptr};
};

class StackAssertScope {
public:
    explicit StackAssertScope(JSThread *thread) : thread_(thread), oldSp_(thread->GetCurrentSPFrame()) {}

    ~StackAssertScope()
    {
        DASSERT_PRINT(oldSp_ == thread_->GetCurrentSPFrame(),
                      "StackAssertScope assert failed, sp did not restore as expeted");
    }

private:
    [[maybe_unused]] JSThread *thread_ {nullptr};
    const JSTaggedType *oldSp_ {nullptr};
};
} // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
