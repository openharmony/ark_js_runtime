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
#include <typeinfo>
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/llvm_stackmap_parser.h"

namespace panda::ecmascript {
#define FRAME_AND_TYPE_LIST(V)                                           \
    V(OptimizedFrame, OPTIMIZED_FRAME)                                   \
    V(OptimizedEntryFrame, OPTIMIZED_ENTRY_FRAME)                        \
    V(OptimizedJSFunctionFrame, OPTIMIZED_JS_FUNCTION_FRAME)             \
    V(OptimizedLeaveFrame, LEAVE_FRAME)                                  \
    V(OptimizedWithArgvLeaveFrame, LEAVE_FRAME_WITH_ARGV)                \
    V(InterpretedFrame, INTERPRETER_FRAME)                               \
    V(AsmInterpretedFrame, ASM_INTERPRETER_FRAME)                        \
    V(AsmInterpretedFrame, INTERPRETER_CONSTRUCTOR_FRAME)                \
    V(BuiltinFrame, BUILTIN_FRAME)                                       \
    V(BuiltinWithArgvFrame, BUILTIN_FRAME_WITH_ARGV)                     \
    V(BuiltinFrame, BUILTIN_ENTRY_FRAME)                                 \
    V(InterpretedFrame, INTERPRETER_FAST_NEW_FRAME)                      \
    V(InterpretedEntryFrame, INTERPRETER_ENTRY_FRAME)                    \
    V(AsmInterpretedEntryFrame, ASM_INTERPRETER_ENTRY_FRAME)             \
    V(OptimizedJSFunctionFrame, OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME) \
    V(AsmInterpretedBridgeFrame, ASM_INTERPRETER_BRIDGE_FRAME)

#define IMPLEMENT_GET_FRAME(Frame)                  \
    template<>                                      \
    Frame* FrameIterator::GetFrame()                \
    {                                               \
        return Frame::GetFrameFromSp(current_);     \
    }
    FRAME_LIST(IMPLEMENT_GET_FRAME)
#undef IMPLEMENT_GET_FRAME

void FrameIterator::Advance()
{
    ASSERT(!Done());
    FrameType t = GetFrameType();
    switch (t) {
#define CASE(FRAME, Type)                       \
        case FrameType::Type : {                \
            auto frame = GetFrame<FRAME>();     \
            current_ = frame->GetPrevFrameFp(); \
            break;                              \
        }
        FRAME_AND_TYPE_LIST(CASE)
#undef CASE
        default: {
            UNREACHABLE();
        }
    }
}
uintptr_t FrameIterator::GetPrevFrameCallSiteSp(uintptr_t curPc)
{
    if (Done()) {
        return 0;
    }
#define GET_CALLSITE_SP_LIST(V)                                \
    V(LEAVE_FRAME, OptimizedLeaveFrame)                        \
    V(LEAVE_FRAME_WITH_ARGV, OptimizedWithArgvLeaveFrame)      \
    V(BUILTIN_FRAME_WITH_ARGV, BuiltinWithArgvFrame)           \
    V(BUILTIN_FRAME, BuiltinFrame)                             \
    V(ASM_INTERPRETER_BRIDGE_FRAME, AsmInterpretedBridgeFrame)

    auto type = GetFrameType();
    switch (type) {
#define CASE(Type, Frame)                          \
        case FrameType::Type: {                    \
            auto frame = GetFrame<Frame>();        \
            return frame->GetCallSiteSp();         \
        }
        GET_CALLSITE_SP_LIST(CASE)
#undef CASE
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto callSiteSp = reinterpret_cast<uintptr_t>(current_) +
                thread_->GetEcmaVM()->GetStackMapParser()->GetFuncFpDelta(curPc);
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
}  // namespace panda::ecmascript
