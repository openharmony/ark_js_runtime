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

#ifndef ECMASCRIPT_ECMA_RUNTIM_CALL_INFO_H
#define ECMASCRIPT_ECMA_RUNTIM_CALL_INFO_H

#include <algorithm>

#include "ecmascript/common.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "js_handle.h"

namespace panda::ecmascript {
class EcmaRuntimeCallInfo;
using EcmaEntrypoint = JSTaggedValue (*)(EcmaRuntimeCallInfo *);

class EcmaRuntimeCallInfo {
public:
    // For builtins interpreter call
    EcmaRuntimeCallInfo(JSThread *thread, uint32_t numArgs, JSTaggedValue *args)
        : thread_(thread), numArgs_(numArgs), gprArgs_(args, numArgs), stackArgs_(nullptr, static_cast<size_t>(0))
    {
        ASSERT(numArgs_ >= NUM_MANDATORY_JSFUNC_ARGS);
    }

    ~EcmaRuntimeCallInfo() = default;

    inline JSThread *GetThread() const
    {
        return thread_;
    }

    inline void SetNewTarget(JSTaggedValue tagged, [[maybe_unused]] int64_t tag = 0)
    {
        SetArg(NEW_TARGET_INDEX, tagged);
    }

    inline void SetFunction(JSTaggedValue tagged, [[maybe_unused]] int64_t tag = 0)
    {
        SetArg(FUNC_INDEX, tagged);
    }

    inline void SetThis(JSTaggedValue tagged, [[maybe_unused]] int64_t tag = 0)
    {
        SetArg(THIS_INDEX, tagged);
    }

    inline void SetCallArg(uint32_t idx, JSTaggedValue tagged, [[maybe_unused]] int64_t tag = 0)
    {
        ASSERT_PRINT(idx < GetArgsNumber(), "Can not set values out of index range");
        SetArg(idx + FIRST_ARGS_INDEX, tagged);
    }

    inline JSHandle<JSTaggedValue> GetArg(uint32_t idx) const
    {
        return JSHandle<JSTaggedValue>(GetArgAddress(idx));
    }

    inline JSHandle<JSTaggedValue> GetFunction() const
    {
        return GetArg(FUNC_INDEX);
    }

    inline JSHandle<JSTaggedValue> GetNewTarget() const
    {
        return GetArg(NEW_TARGET_INDEX);
    }

    inline JSHandle<JSTaggedValue> GetThis() const
    {
        return GetArg(THIS_INDEX);
    }

    inline JSHandle<JSTaggedValue> GetCallArg(uint32_t idx) const
    {
        return GetArg(idx + FIRST_ARGS_INDEX);
    }

    /*
     * The number of arguments pairs excluding the 'func', 'new.target' and 'this'. For instance:
     * for code fragment: " foo(v1); ", GetArgsNumber() returns 1
     */
    inline array_size_t GetArgsNumber() const
    {
        return numArgs_ - NUM_MANDATORY_JSFUNC_ARGS;
    }

    inline uintptr_t GetArgAddress(uint32_t idx) const
    {
        if (idx < gprArgs_.size()) {
            return reinterpret_cast<uintptr_t>(&gprArgs_[idx]);
        }
        return reinterpret_cast<uintptr_t>(&stackArgs_[idx - gprArgs_.size()]);
    }

private:
    DEFAULT_COPY_SEMANTIC(EcmaRuntimeCallInfo);
    DEFAULT_MOVE_SEMANTIC(EcmaRuntimeCallInfo);

    enum ArgsIndex : uint8_t { FUNC_INDEX = 0, NEW_TARGET_INDEX, THIS_INDEX, FIRST_ARGS_INDEX };

    inline void SetArg(uint32_t idx, JSTaggedValue tagged)
    {
        *reinterpret_cast<JSTaggedValue *>(GetArgAddress(idx)) = tagged;
    }

private:
    JSThread *thread_;
    uint32_t numArgs_;
    Span<JSTaggedValue> gprArgs_;
    Span<JSTaggedValue> stackArgs_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_ECMA_RUNTIM_CALL_INFO_H