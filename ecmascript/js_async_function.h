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

#ifndef ECMASCRIPT_JS_ASYNC_FUNCTION_H
#define ECMASCRIPT_JS_ASYNC_FUNCTION_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "js_function.h"

namespace panda::ecmascript {
class JSAsyncAwaitStatusFunction : public JSFunction {
public:
    CAST_CHECK(JSAsyncAwaitStatusFunction, IsJSAsyncAwaitStatusFunction);

    static JSHandle<JSTaggedValue> AsyncFunctionAwaitFulfilled(JSThread *thread,
                                                               const JSHandle<JSAsyncAwaitStatusFunction> &func,
                                                               const JSHandle<JSTaggedValue> &value);

    static JSHandle<JSTaggedValue> AsyncFunctionAwaitRejected(JSThread *thread,
                                                              const JSHandle<JSAsyncAwaitStatusFunction> &func,
                                                              const JSHandle<JSTaggedValue> &reason);

    static constexpr size_t ASYNC_CONTEXT_OFFSET = JSFunction::SIZE;
    ACCESSORS(AsyncContext, ASYNC_CONTEXT_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSFunction, ASYNC_CONTEXT_OFFSET, SIZE)

    DECL_DUMP()
};

class JSAsyncFunction : public JSFunction {
public:
    static JSAsyncFunction *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAsyncFunction());
        return static_cast<JSAsyncFunction *>(object);
    }

    static void AsyncFunctionAwait(JSThread *thread, const JSHandle<JSAsyncFuncObject> &asyncFuncObj,
                                   const JSHandle<JSTaggedValue> &value);
    static constexpr size_t SIZE = JSFunction::SIZE;

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSFunction, SIZE, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_ASYNC_FUNCTION_H
