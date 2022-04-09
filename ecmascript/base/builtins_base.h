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

#ifndef ECMASCRIPT_BASE_BUILTINS_BASE_H
#define ECMASCRIPT_BASE_BUILTINS_BASE_H

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class JSArray;
namespace base {
class BuiltinsBase {
public:
    enum ArgsPosition : uint32_t { FIRST = 0, SECOND, THIRD, FOURTH, FIFTH };
    static JSHandle<TaggedArray> GetArgsArray(EcmaRuntimeCallInfo *msg);
    static inline JSHandle<JSTaggedValue> GetConstructor(EcmaRuntimeCallInfo *msg)
    {
        return msg->GetFunction();
    }

    static inline JSHandle<JSTaggedValue> GetThis(EcmaRuntimeCallInfo *msg)
    {
        return msg->GetThis();
    }

    static inline JSHandle<JSTaggedValue> GetNewTarget(EcmaRuntimeCallInfo *msg)
    {
        return msg->GetNewTarget();
    }

    static inline JSHandle<JSTaggedValue> GetCallArg(EcmaRuntimeCallInfo *msg, uint32_t position)
    {
        if (position >= msg->GetArgsNumber()) {
            JSThread *thread = msg->GetThread();
            return thread->GlobalConstants()->GetHandledUndefined();
        }
        return msg->GetCallArg(position);
    }

    static inline JSTaggedValue GetTaggedInt(int32_t value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedDouble(double value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedBoolean(bool value)
    {
        return JSTaggedValue(value);
    }

    static inline JSTaggedValue GetTaggedString(JSThread *thread, const char *str)
    {
        return thread->GetEcmaVM()->GetFactory()->NewFromASCII(str).GetTaggedValue();
    }
};
}  // namespace base
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_BASE_BUILTINS_BASE_H
