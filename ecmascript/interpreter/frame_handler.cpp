
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

#include "ecmascript/file_loader.h"
#include "ecmascript/llvm_stackmap_parser.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"

#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
FrameHandler::FrameHandler(const JSThread *thread)
    : sp_(const_cast<JSTaggedType *>(thread->GetCurrentFrame())), thread_(thread)
{
    stackmapParser_ = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser();
    AdvanceToInterpretedFrame();
}
ARK_INLINE void FrameHandler::AdvanceToInterpretedFrame()
{
    if (!thread_->IsAsmInterpreter()) {
        return;
    }
    FrameIterator it(sp_, thread_);
    for (; !it.Done(); it.Advance()) {
        FrameType t = it.GetFrameType();
        if (IsInterpretedFrame(t) || IsInterpretedEntryFrame(t)) {
            break;
        }
    }
    sp_ = it.GetSp();
}

ARK_INLINE void FrameHandler::PrevInterpretedFrame()
{
    if (!thread_->IsAsmInterpreter()) {
        FrameIterator it(sp_, thread_);
        it.Advance();
        sp_ = it.GetSp();
        return;
    }
    AdvanceToInterpretedFrame();
    FrameIterator it(sp_, thread_);
    FrameType t = it.GetFrameType();
    if (t == FrameType::ASM_INTERPRETER_FRAME) {
        auto frame = it.GetFrame<AsmInterpretedFrame>();
        if (thread_->IsAsmInterpreter()) {
            fp_ = frame->GetCurrentFramePointer();
        }
    }
    it.Advance();
    sp_ = it.GetSp();
    AdvanceToInterpretedFrame();
}

JSTaggedType* FrameHandler::GetPrevInterpretedFrame()
{
    PrevInterpretedFrame();
    return GetSp();
}

uint32_t FrameHandler::GetNumberArgs()
{
    if (thread_->IsAsmInterpreter()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return static_cast<uint32_t>(frame->GetCurrentFramePointer() - sp_);
    }
    ASSERT(IsInterpretedFrame());
    JSTaggedType *prevSp = nullptr;
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        prevSp = frame->GetPrevFrameFp();
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        prevSp = frame->GetPrevFrameFp();
    }
    auto prevSpEnd = reinterpret_cast<JSTaggedType*>(GetInterpretedFrameEnd(prevSp));
    return static_cast<uint32_t>(prevSpEnd - sp_);
}

JSTaggedValue FrameHandler::GetVRegValue(size_t index) const
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return JSTaggedValue(sp_[index]);
}

void FrameHandler::SetVRegValue(size_t index, JSTaggedValue value)
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    sp_[index] = value.GetRawData();
}

JSTaggedValue FrameHandler::GetAcc() const
{
    ASSERT(IsInterpretedFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->acc;
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->acc;
    }
}

uint32_t FrameHandler::GetBytecodeOffset() const
{
    ASSERT(IsInterpretedFrame());
    JSMethod *method = GetMethod();
    auto offset = GetPc() - method->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *FrameHandler::GetMethod() const
{
    ASSERT(IsInterpretedFrame());
    auto function = GetFunction();
    return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget();
}

JSMethod *FrameHandler::CheckAndGetMethod() const
{
    ASSERT(IsInterpretedFrame());
    auto function = GetFunction();
    if (function.IsJSFunctionBase() || function.IsJSProxy()) {
        return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget();
    }
    return nullptr;
}

JSTaggedValue FrameHandler::GetFunction() const
{
    ASSERT(IsInterpretedFrame());
    if (thread_->IsAsmInterpreter()) {
        FrameType type = GetFrameType();
        switch (type) {
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
                auto frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
                return frame->function;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV: {
                auto *frame = BuiltinWithArgvFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME: {
                auto *frame = BuiltinFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME:
            case FrameType::INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::LEAVE_FRAME:
            case FrameType::LEAVE_FRAME_WITH_ARGV:
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    } else {
        FrameIterator it(sp_, thread_);
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->function;
    }
}

const uint8_t *FrameHandler::GetPc() const
{
    ASSERT(IsInterpretedFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->GetPc();
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->GetPc();
    }
}

ConstantPool *FrameHandler::GetConstpool() const
{
    ASSERT(IsInterpretedFrame());
    auto function = GetFunction();
    JSTaggedValue constpool = JSFunction::Cast(function.GetTaggedObject())->GetConstantPool();
    return ConstantPool::Cast(constpool.GetTaggedObject());
}

JSTaggedValue FrameHandler::GetEnv() const
{
    ASSERT(IsInterpretedFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->GetEnv();
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->env;
    }
}

void FrameHandler::DumpStack(std::ostream &os) const
{
    size_t i = 0;
    FrameHandler frameHandler(thread_);
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        os << "[" << i++
        << "]:" << frameHandler.GetMethod()->ParseFunctionName()
        << "\n";
    }
}

void FrameHandler::DumpPC(std::ostream &os, const uint8_t *pc) const
{
    FrameHandler frameHandler(thread_);
    ASSERT(frameHandler.HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
    int offset = pc - frameHandler.GetMethod()->GetBytecodeArray();
    os << "offset: " << offset << "\n";
}

ARK_INLINE uintptr_t FrameHandler::GetInterpretedFrameEnd(JSTaggedType *prevSp) const
{
    uintptr_t end = 0U;
    FrameIterator it(prevSp, thread_);
    FrameType type = it.GetFrameType();
    switch (type) {
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto frame = it.GetFrame<AsmInterpretedFrame>();
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto frame = it.GetFrame<InterpretedFrame>();
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME:
            end = ToUintPtr(GetInterpretedEntryFrameStart(prevSp));
            break;
        case FrameType::BUILTIN_FRAME_WITH_ARGV:
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::BUILTIN_FRAME:
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::LEAVE_FRAME:
        case FrameType::LEAVE_FRAME_WITH_ARGV:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
        default: {
            LOG_ECMA(FATAL) << "frame type error!";
            UNREACHABLE();
        }
    }
    return end;
}

ARK_INLINE JSTaggedType *FrameHandler::GetInterpretedEntryFrameStart(const JSTaggedType *sp)
{
    ASSERT(FrameHandler::GetFrameType(sp) == FrameType::INTERPRETER_ENTRY_FRAME);
    JSTaggedType *argcSp = const_cast<JSTaggedType *>(sp) - INTERPRETER_ENTRY_FRAME_STATE_SIZE - 1;
    uint32_t argc = argcSp[0];
    return argcSp - argc - RESERVED_CALL_ARGCOUNT;
}

void FrameHandler::IterateRsp(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    IterateFrameChain(current, v0, v1);
}

void FrameHandler::IterateSp(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    for (FrameIterator it(current, thread_); !it.Done(); it.Advance()) {
        // only interpreter entry frame is on thread sp when rsp is enable
        ASSERT(it.GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
        auto frame = it.GetFrame<InterpretedEntryFrame>();
        frame->GCIterate(it, v0, v1);
    }
}

void FrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    if (thread_->IsAsmInterpreter()) {
        IterateSp(v0, v1);
        IterateRsp(v0, v1);
        return;
    }
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    FrameType frameType = FrameHandler::GetFrameType(current);
    if (frameType != FrameType::INTERPRETER_ENTRY_FRAME) {
        auto leaveFrame = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
        if (leaveFrame != nullptr) {
            current = leaveFrame;
        }
    }
    IterateFrameChain(current, v0, v1);
}

void FrameHandler::IterateFrameChain(JSTaggedType *start, const RootVisitor &v0, const RootRangeVisitor &v1) const
{
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers = thread_->GetEcmaVM()->GetHeap()->GetDerivedPointers();
    bool isVerifying = false;

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    isVerifying = thread_->GetEcmaVM()->GetHeap()->IsVerifying();
#endif
    JSTaggedType *current = start;
    for (FrameIterator it(current, thread_); !it.Done(); it.Advance()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_FRAME: {
                auto frame = it.GetFrame<OptimizedFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
                auto frame = it.GetFrame<AsmInterpretedFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME: {
                auto frame = it.GetFrame<InterpretedFrame>();
                frame->GCIterate(it, v0, v1);
                break;
            }
            case FrameType::LEAVE_FRAME: {
                auto frame = it.GetFrame<OptimizedLeaveFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::LEAVE_FRAME_WITH_ARGV: {
                auto frame = it.GetFrame<OptimizedWithArgvLeaveFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV: {
                auto frame = it.GetFrame<BuiltinWithArgvFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME: {
                auto frame = it.GetFrame<BuiltinFrame>();
                frame->GCIterate(it, v0, v1, derivedPointers, isVerifying);
                break;
            }
            case FrameType::INTERPRETER_ENTRY_FRAME: {
                auto frame = it.GetFrame<InterpretedEntryFrame>();
                frame->GCIterate(it, v0, v1);
                break;
            }
            case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_BRIDGE_FRAME: {
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    }
}

std::string FrameHandler::GetAotExceptionFuncName(JSTaggedType* argv) const
{
    JSTaggedValue func = JSTaggedValue(*(argv)); // 3: skip returnaddr and argc
    JSMethod *method = JSFunction::Cast(func.GetTaggedObject())->GetMethod();
    return method->GetMethodName();
}

void FrameHandler::CollectBCOffsetInfo()
{
    thread_->GetEcmaVM()->ClearExceptionBCList();
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    FrameIterator it(current, thread_);
    ASSERT(it.GetFrameType() == FrameType::LEAVE_FRAME);
    auto leaveFrame = it.GetFrame<OptimizedLeaveFrame>();
    auto returnAddr = leaveFrame->GetReturnAddr();
    // skip native function, need to fixit later.
    it.Advance();

    for (; !it.Done(); it.Advance()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                auto constInfo = stackmapParser_->GetConstInfo(returnAddr);
                if (!constInfo.empty()) {
                    auto name = GetAotExceptionFuncName(frame->GetArgv(it));
                    thread_->GetEcmaVM()->StoreBCOffsetInfo(name, constInfo[0]);
                }
                returnAddr = frame->GetReturnAddr();
                break;
            }
            case FrameType::LEAVE_FRAME: {
                auto frame = it.GetFrame<OptimizedLeaveFrame>();
                returnAddr = frame->GetReturnAddr();
                break;
            }
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME:
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::LEAVE_FRAME_WITH_ARGV:
            case FrameType::BUILTIN_FRAME_WITH_ARGV:
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME:
            case FrameType::INTERPRETER_ENTRY_FRAME: {
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    }
}
}  // namespace panda::ecmascript