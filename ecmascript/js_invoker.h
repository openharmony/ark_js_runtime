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

#ifndef ECMASCRIPT_INVOKE_H
#define ECMASCRIPT_INVOKE_H

#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
class JsInvoker {
public:
    JsInvoker(const JSThread *thread, JSTaggedValue func, JSTaggedValue obj)
        : JsInvoker(thread, func, obj, JSTaggedValue::Undefined())
    {
    }

    JsInvoker(const JSThread *thread, JSTaggedValue func, JSTaggedValue obj, JSTaggedValue newTarget)
    {
        AddArgument(JSHandle<JSTaggedValue>(thread, func));
        AddArgument(JSHandle<JSTaggedValue>(thread, newTarget));
        AddArgument(JSHandle<JSTaggedValue>(thread, obj));
    }

    ~JsInvoker() = default;
    NO_COPY_SEMANTIC(JsInvoker);
    NO_MOVE_SEMANTIC(JsInvoker);

    template<class T>
    void AddArgument(const JSHandle<T> &arg)
    {
        args_.emplace_back(JSHandle<JSTaggedValue>(arg));
    }

    template<class T>
    void AddArgument(JSHandle<T> &&arg)
    {
        args_.emplace_back(std::move(arg));
    }

    JSTaggedValue Invoke(JSThread *thread);

private:
    CVector<JSHandle<JSTaggedValue>> args_{};
};

JSTaggedValue InvokeJsFunction(JSThread *thread, const JSHandle<JSFunction> &func, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &newTgt, InternalCallParams *arguments);
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INVOKE_H
