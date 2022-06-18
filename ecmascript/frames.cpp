/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#include "ecmascript/frames.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/file_loader.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/llvm_stackmap_parser.h"
#include "ecmascript/interpreter/frame_handler.h"

namespace panda::ecmascript {
JSTaggedType *OptimizedLeaveFrame::GetJsFuncFrameArgv(JSThread *thread) const
{
    auto current = GetPrevFrameFp();
    int delta = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser()->GetFuncFpDelta(returnAddr);
    uintptr_t *preFrameSp = reinterpret_cast<uintptr_t *>(const_cast<JSTaggedType *>(current))
        + delta / sizeof(uintptr_t);
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(preFrameSp + sizeof(uint64_t) / sizeof(uintptr_t));
    return argv;
}

void FrameIterator::Advance()
{
    ASSERT(!Done());
    FrameType t = GetFrameType();
    switch (t) {
        case FrameType::OPTIMIZED_FRAME : {
            auto frame = GetFrame<OptimizedFrame>();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(optimizedReturnAddr_);
            optimizedReturnAddr_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME : {
            auto frame = GetFrame<OptimizedEntryFrame>();
            optimizedReturnAddr_ = 0;
            optimizedCallSiteSp_ = 0;
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
            optimizedReturnAddr_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(optimizedReturnAddr_);
            optimizedReturnAddr_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
            optimizedReturnAddr_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV : {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
            optimizedReturnAddr_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME : {
            auto frame = GetFrame<InterpretedFrame>();
            optimizedReturnAddr_ = 0;
            optimizedCallSiteSp_ = 0;
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME : {
            auto frame = GetFrame<AsmInterpretedFrame>();
            optimizedReturnAddr_ = 0;
            optimizedCallSiteSp_ = 0;
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME:
        case FrameType::BUILTIN_ENTRY_FRAME : {
            auto frame = GetFrame<BuiltinFrame>();
            optimizedReturnAddr_ = frame->GetReturnAddr();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV : {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            optimizedReturnAddr_ = frame->GetReturnAddr();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<InterpretedEntryFrame>();
            optimizedReturnAddr_ = 0;
            optimizedCallSiteSp_ = 0;
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<AsmInterpretedEntryFrame>();
            optimizedReturnAddr_ = 0;
            optimizedCallSiteSp_ = 0;
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(optimizedReturnAddr_);
            optimizedReturnAddr_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        default: {
            UNREACHABLE();
        }
    }
}
uintptr_t FrameIterator::GetPrevFrameCallSiteSp(uintptr_t curPc) const
{
    if (Done()) {
        return 0;
    }
    auto type = GetFrameType();
    switch (type) {
        case FrameType::LEAVE_FRAME: {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV: {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_FRAME: {
            auto frame = GetFrame<BuiltinFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME: {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            ASSERT(thread_ != nullptr);
            auto callSiteSp = reinterpret_cast<uintptr_t>(current_) +
                thread_->GetEcmaVM()->GetFileLoader()->GetStackMapParser()->GetFuncFpDelta(curPc);
            return callSiteSp;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME : {
            auto callSiteSp = reinterpret_cast<uintptr_t>(current_) + sizeof(uintptr_t);
            return callSiteSp;
        }
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME: {
            return 0;
        }
        default: {
            UNREACHABLE();
        }
    }
}

ARK_INLINE void OptimizedFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &v0,
    [[maybe_unused]] const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    bool isVerifying) const
{
    std::set<uintptr_t> slotAddrs;
    const JSThread *thread = it.GetThread();
    ASSERT(thread != nullptr);
    bool ret = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser()->CollectGCSlots(
        it.GetOptimizedReturnAddr(), reinterpret_cast<uintptr_t>(it.GetSp()), slotAddrs, derivedPointers,
        isVerifying, it.GetCallSiteSp());
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void OptimizedJSFunctionFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &v0,
    const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    bool isVerifying) const
{
    OptimizedJSFunctionFrame *frame = OptimizedJSFunctionFrame::GetFrameFromSp(it.GetSp());

    const JSThread *thread = it.GetThread();
    ASSERT(thread != nullptr);
    int delta = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser()->GetFuncFpDelta(it.GetOptimizedReturnAddr());
    uintptr_t *preFrameSp = frame->ComputePrevFrameSp(it.GetSp(), delta);

    auto argc = *(reinterpret_cast<uint64_t *>(preFrameSp));
    JSTaggedType *argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(preFrameSp));
    if (argc > 0) {
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }

    std::set<uintptr_t> slotAddrs;
    bool ret = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser()->CollectGCSlots(
        it.GetOptimizedReturnAddr(), reinterpret_cast<uintptr_t>(it.GetSp()),
        slotAddrs, derivedPointers, isVerifying, it.GetCallSiteSp());
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void AsmInterpretedFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &v0,
    const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    bool isVerifying) const
{
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(it.GetSp());
    uintptr_t start = ToUintPtr(it.GetSp());
    uintptr_t end = ToUintPtr(frame->GetCurrentFramePointer());
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    if (frame->pc != nullptr) {
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
    }

    std::set<uintptr_t> slotAddrs;
    const JSThread *thread = it.GetThread();
    bool ret = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser()->CollectGCSlots(
        it.GetOptimizedReturnAddr(), reinterpret_cast<uintptr_t>(it.GetSp()), slotAddrs, derivedPointers, isVerifying,
        it.GetCallSiteSp());
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
        return;
    }
    for (auto slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void InterpretedFrame::GCIterate(const FrameIterator &it,
                                            const RootVisitor &v0,
                                            const RootRangeVisitor &v1) const
{
    auto sp = it.GetSp();
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp);
    if (frame->function == JSTaggedValue::Hole()) {
        return;
    }

    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    uintptr_t start = ToUintPtr(sp);
    uintptr_t end = 0U;
    const JSThread *thread = it.GetThread();
    FrameIterator prevIt(prevSp, thread);
    FrameType type = prevIt.GetFrameType();
    if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
        auto prevFrame = InterpretedFrame::GetFrameFromSp(prevSp);
        end = ToUintPtr(prevFrame);
    } else if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
        end = ToUintPtr(FrameHandler::GetInterpretedEntryFrameStart(prevSp));
    } else {
        LOG_ECMA(FATAL) << "frame type error!";
    }
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));

    // pc == nullptr, init InterpretedFrame & native InterpretedFrame.
    if (frame->pc != nullptr) {
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->constpool)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->profileTypeInfo)));
    }
}

ARK_INLINE void OptimizedLeaveFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &v0,
    const RootRangeVisitor &v1,
    [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    [[maybe_unused]] bool isVerifying) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(&frame->argc + 1);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void OptimizedWithArgvLeaveFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &v0,
    const RootRangeVisitor &v1,
    [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    [[maybe_unused]] bool isVerifying) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedWithArgvLeaveFrame *frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        uintptr_t* argvPtr = reinterpret_cast<uintptr_t *>(&frame->argc + 1);
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(*argvPtr);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void BuiltinWithArgvFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &v0,
    const RootRangeVisitor &v1,
    [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    [[maybe_unused]] bool isVerifying) const
{
    const JSTaggedType *sp = it.GetSp();
    auto frame = BuiltinWithArgvFrame::GetFrameFromSp(sp);
    auto argc = frame->GetNumArgs() + BuiltinFrame::RESERVED_CALL_ARGCOUNT;
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void BuiltinFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &v0,
    const RootRangeVisitor &v1,
    [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    [[maybe_unused]] bool isVerifying) const
{
    const JSTaggedType *sp = it.GetSp();
    auto frame = BuiltinFrame::GetFrameFromSp(sp);
    // no need to visit stack map for entry frame
    if (frame->type == FrameType::BUILTIN_ENTRY_FRAME) {
        // only visit function
        v0(Root::ROOT_FRAME, ObjectSlot(frame->GetStackArgsAddress()));
        return;
    }
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    auto argc = frame->GetNumArgs() + BuiltinFrame::RESERVED_CALL_ARGCOUNT;
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void InterpretedEntryFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &v0,
    const RootRangeVisitor &v1) const
{
    const JSTaggedType* sp = it.GetSp();
    uintptr_t start = ToUintPtr(FrameHandler::GetInterpretedEntryFrameStart(sp));
    uintptr_t end = ToUintPtr(sp - INTERPRETER_ENTRY_FRAME_STATE_SIZE);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}
}  // namespace panda::ecmascript
