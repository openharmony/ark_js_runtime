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
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
struct EcmaRuntimeCallInfo;
using EcmaEntrypoint = JSTaggedValue (*)(EcmaRuntimeCallInfo *);

struct EcmaRuntimeCallInfo : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer> {
    enum class Index : size_t {
        ThreadIndex = 0,
        NumArgsIndex,
        StackArgsIndex,
        NumOfMembers
    };

public:
    inline JSThread *GetThread() const
    {
        return thread_;
    }

    inline void SetNewTarget(const JSTaggedValue tagged)
    {
        SetArg(NEW_TARGET_INDEX, tagged);
    }

    inline void SetFunction(const JSTaggedValue tagged)
    {
        SetArg(FUNC_INDEX, tagged);
    }

    inline void SetThis(const JSTaggedValue tagged)
    {
        SetArg(THIS_INDEX, tagged);
    }

    inline void SetCallArg(int32_t idx, const JSTaggedValue tagged)
    {
        ASSERT_PRINT(idx < GetArgsNumber(), "Can not set values out of index range");
        SetArg(idx + FIRST_ARGS_INDEX, tagged);
    }

    inline void SetCallArg(const JSTaggedValue arg)
    {
        ASSERT_PRINT(GetArgsNumber() == 1, "args number is not 1");
        SetArg(FIRST_ARGS_INDEX, arg);
    }

    inline void SetCallArg(const JSTaggedValue arg0, const JSTaggedValue arg1)
    {
        ASSERT_PRINT(GetArgsNumber() == 2, "args number is not 2");  // 2: args number
        SetArg(FIRST_ARGS_INDEX, arg0);
        SetArg(FIRST_ARGS_INDEX + 1, arg1);
    }

    inline void SetCallArg(const JSTaggedValue arg0, const JSTaggedValue arg1, const JSTaggedValue arg2)
    {
        ASSERT_PRINT(GetArgsNumber() == 3, "args number is not 3");  // 3: args number
        SetArg(FIRST_ARGS_INDEX, arg0);
        SetArg(FIRST_ARGS_INDEX + 1, arg1);
        SetArg(FIRST_ARGS_INDEX + 2, arg2);  // 2: second index
    }

    inline void SetCallArg(const JSTaggedValue arg0, const JSTaggedValue arg1, const JSTaggedValue arg2,
                           const JSTaggedValue arg3)
    {
        ASSERT_PRINT(GetArgsNumber() == 4, "args number is not 4");  // 4: args number
        SetArg(FIRST_ARGS_INDEX, arg0);
        SetArg(FIRST_ARGS_INDEX + 1, arg1);
        SetArg(FIRST_ARGS_INDEX + 2, arg2);  // 2: second index
        SetArg(FIRST_ARGS_INDEX + 3, arg3);  // 3: third index
    }

    inline void SetCallArg(const JSTaggedValue arg0, const JSTaggedValue arg1, const JSTaggedValue arg2,
                           const JSTaggedValue arg3, const JSTaggedValue arg4)
    {
        ASSERT_PRINT(GetArgsNumber() == 5, "args number is not 5");  // 5: args number
        SetArg(FIRST_ARGS_INDEX, arg0);
        SetArg(FIRST_ARGS_INDEX + 1, arg1);
        SetArg(FIRST_ARGS_INDEX + 2, arg2);  // 2: second index
        SetArg(FIRST_ARGS_INDEX + 3, arg3);  // 3: third index
        SetArg(FIRST_ARGS_INDEX + 4, arg4);  // 4: fourth index
    }

    inline void SetCallArg(int32_t argc, const JSTaggedType argv[])
    {
        for (int32_t i = 0; i < argc; i++) {
            SetCallArg(i, JSTaggedValue(argv[i]));
        }
    }

    inline void SetCallArg(int32_t argsLength, const JSHandle<TaggedArray> args)
    {
        for (int32_t i = 0; i < argsLength; i++) {
            SetCallArg(i, args->Get(thread_, i));
        }
    }

    inline void SetCallArg(int32_t argsLength, const TaggedArray* args)
    {
        for (int32_t i = 0; i < argsLength; i++) {
            SetCallArg(i, args->Get(thread_, i));
        }
    }

    inline void SetCallArg(int32_t argsLength, int32_t startIndex, const EcmaRuntimeCallInfo* argv, int32_t offset)
    {
        for (int32_t i = startIndex; i < argsLength; i++) {
            SetCallArg(i, argv->GetCallArgValue(i - startIndex + offset));
        }
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

    inline JSHandle<JSTaggedValue> GetCallArg(int32_t idx) const
    {
        return GetArg(idx + FIRST_ARGS_INDEX);
    }

    inline JSTaggedValue GetFunctionValue() const
    {
        JSHandle<JSTaggedValue> func = GetFunction();
        return func.GetTaggedValue();
    }

    inline JSTaggedValue GetNewTargetValue() const
    {
        JSHandle<JSTaggedValue> newTarget = GetNewTarget();
        return newTarget.GetTaggedValue();
    }

    inline JSTaggedValue GetThisValue() const
    {
        JSHandle<JSTaggedValue> thisObj = GetThis();
        return thisObj.GetTaggedValue();
    }

    inline JSTaggedValue GetCallArgValue(int32_t idx) const
    {
        JSHandle<JSTaggedValue> arg = GetCallArg(idx);
        return arg.GetTaggedValue();
    }

    /*
     * The number of arguments pairs excluding the 'func', 'new.target' and 'this'. For instance:
     * for code fragment: " foo(v1); ", GetArgsNumber() returns 1
     */
    inline int32_t GetArgsNumber() const
    {
        return numArgs_ - NUM_MANDATORY_JSFUNC_ARGS;
    }

    inline JSTaggedType *GetArgs()
    {
        return stackArgs_;
    }

private:
    enum ArgsIndex : uint8_t { FUNC_INDEX = 0, NEW_TARGET_INDEX, THIS_INDEX, FIRST_ARGS_INDEX };

    inline uintptr_t GetArgAddress(int32_t idx) const
    {
        if (idx < numArgs_) {
            return reinterpret_cast<uintptr_t>(&stackArgs_[idx]);
        }
        return 0U;
    }

    inline void SetArg(int32_t idx, const JSTaggedValue tagged)
    {
        uintptr_t addr = GetArgAddress(idx);
        if (addr != 0U) {
            *reinterpret_cast<JSTaggedValue *>(addr) = tagged;
        }
    }

    inline JSHandle<JSTaggedValue> GetArg(int32_t idx) const
    {
        return JSHandle<JSTaggedValue>(GetArgAddress(idx));
    }

private:
    alignas(EAS) JSThread *thread_ {nullptr};
    alignas(EAS) int32_t numArgs_ {0};  // include func, newTarget, this, equal to stackArgs size.
    __extension__ JSTaggedType stackArgs_[0];  // NOLINT(modernize-avoid-c-arrays)
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_ECMA_RUNTIM_CALL_INFO_H