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

#include "ecmascript/interpreter/interpreter.h"

#include "ecmascript/frames.h"
#include "ecmascript/interpreter/frame_handler.h"
namespace panda::ecmascript {
// make EcmaRuntimeCallInfo in stack pointer as fallows:
//   +----------------------+   —
//   |      base.type       |   ^
//   |----------------------|   |
//   |      base.prev       | InterpretedEntryFrame
//   |----------------------|   |
//   |          pc          |   v
//   |----------------------|   —
//   |       numArgs        |   ^
//   |----------------------|   |
//   |        args...       |   |
//   |----------------------|   |
//   |        this          |   |
//   |----------------------| EcmaRuntimeCallInfo
//   |       newTarget      |   |
//   |----------------------|   |
//   |        func          |   v
//   +----------------------+   —
EcmaRuntimeCallInfo EcmaInterpreter::NewRuntimeCallInfo(
    JSThread *thread, JSHandle<JSTaggedValue> func, JSHandle<JSTaggedValue> thisObj, JSHandle<JSTaggedValue> newTarget,
    size_t numArgs)
{
    JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedType *newSp;
    JSTaggedType *prevSp = sp;
    if (thread->IsAsmInterpreter()) {
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
        newSp = FrameHandler::GetInterpretedEntryFrameStart(sp);
#else
        newSp = sp - AsmInterpretedFrame::NumOfMembers();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto leaveFrame = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
        if (leaveFrame != nullptr) {
            prevSp = leaveFrame;
        }
#endif
    } else {
        newSp = sp - InterpretedFrame::NumOfMembers();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    if (UNLIKELY(thread->DoStackOverflowCheck(newSp - numArgs - RESERVED_CALL_ARGCOUNT))) {
        EcmaRuntimeCallInfo ecmaRuntimeCallInfo(thread, INVALID_ARGS_NUMBER, nullptr);
        return ecmaRuntimeCallInfo;
    }

    thread->SetCurrentSPFrame(newSp);
    // create entry frame.
    InterpretedEntryFrame *entryState = InterpretedEntryFrame::GetFrameFromSp(newSp);
    entryState->base.type = FrameType::INTERPRETER_ENTRY_FRAME;
    entryState->base.prev = prevSp;
    entryState->pc = nullptr;

    newSp -= INTERPRETER_ENTRY_FRAME_STATE_SIZE;   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    *(--newSp) = JSTaggedValue(static_cast<int32_t>(numArgs)).GetRawData();
    newSp -= numArgs;   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    *(--newSp) = thisObj.GetTaggedType();
    *(--newSp) = newTarget.GetTaggedType();
    *(--newSp) = func.GetTaggedType();
    EcmaRuntimeCallInfo ecmaRuntimeCallInfo(thread, numArgs, newSp);
    return ecmaRuntimeCallInfo;
}
}  // namespace panda::ecmascript
