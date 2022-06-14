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

namespace panda::ecmascript {
void FrameIterator::Advance()
{
    ASSERT(!Done());
    FrameType t = GetFrameType();
    switch (t) {
        case FrameType::OPTIMIZED_FRAME : {
            auto frame = GetFrame<OptimizedFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME : {
            auto frame = GetFrame<OptimizedEntryFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME : {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV : {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME : {
            auto frame = GetFrame<InterpretedFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME : {
            auto frame = GetFrame<AsmInterpretedFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME:
        case FrameType::BUILTIN_ENTRY_FRAME : {
            auto frame = GetFrame<BuiltinFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV : {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<InterpretedEntryFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<AsmInterpretedEntryFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            current_ = frame->GetPrevFrameFp();
            break;
        }
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
}  // namespace panda::ecmascript
