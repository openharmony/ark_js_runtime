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

#include "ecmascript/js_async_function.h"

#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/generator_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromise = builtins::BuiltinsPromise;

void JSAsyncFunction::AsyncFunctionAwait(JSThread *thread, const JSHandle<JSAsyncFuncObject> &asyncFuncObj,
                                         const JSHandle<JSTaggedValue> &value)
{
    // 1.Let asyncContext be the running execution context.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> asyncCtxt(thread, asyncFuncObj->GetGeneratorContext());

    // 2.Let promiseCapability be ! NewPromiseCapability(%Promise%).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<PromiseCapability> pcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));

    // 3.Let resolveResult be ! Call(promiseCapability.[[Resolve]], undefined, « value »).
    JSHandle<JSTaggedValue> resolve(thread, pcap->GetResolve());
    JSHandle<JSTaggedValue> thisArg = globalConst->GetHandledUndefined();
    InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(value);
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(thread, resolve, thisArg, 1, arguments->GetArgv());

    // 4.Let onFulfilled be a new built-in function object as defined in AsyncFunction Awaited Fulfilled.
    JSHandle<JSAsyncAwaitStatusFunction> fulFunc =
        factory->NewJSAsyncAwaitStatusFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::AsyncAwaitFulfilled));

    // 5.Let onRejected be a new built-in function object as defined in AsyncFunction Awaited Rejected.
    JSHandle<JSAsyncAwaitStatusFunction> rejFunc =
        factory->NewJSAsyncAwaitStatusFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::AsyncAwaitRejected));

    // 6.Set onFulfilled.[[AsyncContext]] to asyncContext.
    // 7.Set onRejected.[[AsyncContext]] to asyncContext.
    fulFunc->SetAsyncContext(thread, asyncCtxt);
    rejFunc->SetAsyncContext(thread, asyncCtxt);

    // 8.Let throwawayCapability be ! NewPromiseCapability(%Promise%).
    // 9.Set throwawayCapability.[[Promise]].[[PromiseIsHandled]] to true.
    JSHandle<PromiseCapability> tcap =
        JSPromise::NewPromiseCapability(thread, JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction()));
    JSHandle<JSPromise>(thread, tcap->GetPromise())->SetPromiseIsHandled(thread, JSTaggedValue::Undefined());

    // 10.Perform ! PerformPromiseThen(promiseCapability.[[Promise]], onFulfilled, onRejected, throwawayCapability).
    JSHandle<JSPromise> promise(thread, pcap->GetPromise());
    [[maybe_unused]] JSTaggedValue pres = BuiltinsPromise::PerformPromiseThen(
        thread, promise, JSHandle<JSTaggedValue>::Cast(fulFunc), JSHandle<JSTaggedValue>::Cast(rejFunc), tcap);

    // 11.Remove asyncContext from the execution context stack and restore the execution context that
    //    is at the top of the execution context stack as the running execution context.
    // 12.Set the code evaluation state of asyncContext such that when evaluation is resumed with a Completion
    //    resumptionValue the following steps will be performed:
    //   a.Return resumptionValue.
    // 13.Return.
}

JSHandle<JSTaggedValue> JSAsyncAwaitStatusFunction::AsyncFunctionAwaitFulfilled(
    JSThread *thread, const JSHandle<JSAsyncAwaitStatusFunction> &func, const JSHandle<JSTaggedValue> &value)
{
    // 1.Let asyncContext be F.[[AsyncContext]].
    JSHandle<GeneratorContext> asyncCtxt(thread, func->GetAsyncContext());

    // 2.Let prevContext be the running execution context.
    C2IBridge c2i;
    GeneratorHelper::ChangeGenContext(thread, asyncCtxt, &c2i);
    // 3.Suspend prevContext.
    // 4.Push asyncContext onto the execution context stack; asyncContext is now the running execution context.
    // 5.Resume the suspended evaluation of asyncContext using NormalCompletion(value) as the result of the
    //   operation that suspended it. Let result be the value returned by the resumed computation.
    JSHandle<JSObject> result = GeneratorHelper::Next(thread, asyncCtxt, value.GetTaggedValue());
    GeneratorHelper::ResumeContext(thread);
    // 6.Assert: When we reach this step, asyncContext has already been removed from the execution context stack
    //   and prevContext is the currently running execution context.

    // 7.Return Completion(result).
    return JSHandle<JSTaggedValue>::Cast(result);
}

JSHandle<JSTaggedValue> JSAsyncAwaitStatusFunction::AsyncFunctionAwaitRejected(
    JSThread *thread, const JSHandle<JSAsyncAwaitStatusFunction> &func, const JSHandle<JSTaggedValue> &reason)
{
    // 1.Let asyncContext be F.[[AsyncContext]].
    JSHandle<GeneratorContext> asyncCtxt(thread, func->GetAsyncContext());

    // 2.Let prevContext be the running execution context.
    C2IBridge c2i;
    GeneratorHelper::ChangeGenContext(thread, asyncCtxt, &c2i);
    // 3.Suspend prevContext.
    // 4.Push asyncContext onto the execution context stack; asyncContext is now the running execution context.
    // 5.Resume the suspended evaluation of asyncContext using Completion{[[Type]]: throw,
    //   [[Value]]: reason, [[Target]]: empty} as the result of the operation that suspended it.
    //   Let result be the value returned by the resumed computation.
    JSHandle<JSObject> result = GeneratorHelper::Throw(thread, asyncCtxt, reason.GetTaggedValue());
    GeneratorHelper::ResumeContext(thread);
    // 6.Assert: When we reach this step, asyncContext has already been removed from the execution context stack
    //   and prevContext is the currently running execution context.

    // 7.Return Completion(result).
    return JSHandle<JSTaggedValue>::Cast(result);
}
}  // namespace panda::ecmascript
