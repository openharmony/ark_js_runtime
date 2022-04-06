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

#include "ecmascript/builtins/builtins_promise_handler.h"

#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/assert_scope.h"

namespace panda::ecmascript::builtins {
// es6 25.4.1.3.2 Promise Resolve Functions
JSTaggedValue BuiltinsPromiseHandler::Resolve(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, Resolve);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();

    // 1. Assert: F has a [[Promise]] internal slot whose value is an Object.
    JSHandle<JSPromiseReactionsFunction> resolve = JSHandle<JSPromiseReactionsFunction>::Cast(GetConstructor(argv));
    ASSERT_PRINT(resolve->GetPromise().IsECMAObject(), "Resolve: promise must be js object");

    // 2. Let promise be the value of F's [[Promise]] internal slot.
    // 3. Let alreadyResolved be the value of F's [[AlreadyResolved]] internal slot.
    // 4. If alreadyResolved.[[value]] is true, return undefined.
    // 5. Set alreadyResolved.[[value]] to true.
    JSHandle<JSPromise> resolvePromise(thread, resolve->GetPromise());
    JSHandle<PromiseRecord> alreadyResolved(thread, resolve->GetAlreadyResolved());
    if (alreadyResolved->GetValue().IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    alreadyResolved->SetValue(thread, JSTaggedValue::True());

    // 6. If SameValue(resolution, promise) is true, then
    //     a. Let selfResolutionError be a newly created TypeError object.
    //     b. Return RejectPromise(promise, selfResolutionError).
    JSHandle<JSTaggedValue> resolution = BuiltinsBase::GetCallArg(argv, 0);
    if (JSTaggedValue::SameValue(resolution.GetTaggedValue(), resolvePromise.GetTaggedValue())) {
        JSHandle<JSObject> resolutionError =
            factory->GetJSError(ErrorType::TYPE_ERROR, "Resolve: The promise and resolution cannot be the same.");
        JSPromise::RejectPromise(thread, resolvePromise, JSHandle<JSTaggedValue>::Cast(resolutionError));
        return JSTaggedValue::Undefined();
    }
    // 7. If Type(resolution) is not Object, then
    //     a. Return FulfillPromise(promise, resolution).
    if (!resolution.GetTaggedValue().IsECMAObject()) {
        JSPromise::FulfillPromise(thread, resolvePromise, resolution);
        return JSTaggedValue::Undefined();
    }
    // 8. Let then be Get(resolution, "then").
    // 9. If then is an abrupt completion, then
    //     a. Return RejectPromise(promise, then.[[value]]).
    JSHandle<JSTaggedValue> thenKey(thread->GlobalConstants()->GetHandledPromiseThenString());
    JSHandle<JSTaggedValue> thenValue = JSObject::GetProperty(thread, resolution, thenKey).GetValue();
    if (thread->HasPendingException()) {
        if (!thenValue->IsJSError()) {
            thenValue = JSHandle<JSTaggedValue>(thread, thread->GetException());
        }
        thread->ClearException();
        return JSPromise::RejectPromise(thread, resolvePromise, thenValue);
    }
    // 10. Let thenAction be then.[[value]].
    // 11. If IsCallable(thenAction) is false, then
    //     a. Return FulfillPromise(promise, resolution).
    if (!thenValue->IsCallable()) {
        JSPromise::FulfillPromise(thread, resolvePromise, resolution);
        return JSTaggedValue::Undefined();
    }
    // 12. Perform EnqueueJob ("PromiseJobs", PromiseResolveThenableJob, «promise, resolution, thenAction»)
    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(3);  // 3: 3 means three args stored in array
    arguments->Set(thread, 0, resolvePromise);
    arguments->Set(thread, 1, resolution);
    arguments->Set(thread, 2, thenValue);  // 2: 2 means index of array is 2

    JSHandle<JSFunction> promiseResolveThenableJob(env->GetPromiseResolveThenableJob());
    JSHandle<job::MicroJobQueue> job = thread->GetEcmaVM()->GetMicroJobQueue();
    job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, promiseResolveThenableJob, arguments);

    // 13. Return undefined.
    return JSTaggedValue::Undefined();
}

// es6 25.4.1.3.1 Promise Reject Functions
JSTaggedValue BuiltinsPromiseHandler::Reject(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, Reject);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Assert: F has a [[Promise]] internal slot whose value is an Object.
    JSHandle<JSPromiseReactionsFunction> reject = JSHandle<JSPromiseReactionsFunction>::Cast(GetConstructor(argv));
    ASSERT_PRINT(reject->GetPromise().IsECMAObject(), "Reject: promise must be js object");

    // 2. Let promise be the value of F's [[Promise]] internal slot.
    // 3. Let alreadyResolved be the value of F's [[AlreadyResolved]] internal slot.
    // 4. If alreadyResolved.[[value]] is true, return undefined.
    // 5. Set alreadyResolved.[[value]] to true.
    JSHandle<JSPromise> rejectPromise(thread, reject->GetPromise());
    JSHandle<PromiseRecord> alreadyResolved(thread, reject->GetAlreadyResolved());
    if (alreadyResolved->GetValue().IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    alreadyResolved->SetValue(thread, JSTaggedValue::True());

    // 6. Return RejectPromise(promise, reason).
    JSHandle<JSTaggedValue> reason = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> result(thread, JSPromise::RejectPromise(thread, rejectPromise, reason));
    return result.GetTaggedValue();
}

// es6 25.4.1.5.1 GetCapabilitiesExecutor Functions
JSTaggedValue BuiltinsPromiseHandler::Executor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, Executor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Assert: F has a [[Capability]] internal slot whose value is a PromiseCapability Record.
    JSHandle<JSPromiseExecutorFunction> executor = JSHandle<JSPromiseExecutorFunction>::Cast(GetConstructor(argv));
    ASSERT_PRINT(executor->GetCapability().IsRecord(),
                 "Executor: F has a [[Capability]] internal slot whose value is a PromiseCapability Record.");

    // 2. Let promiseCapability be the value of F's [[Capability]] internal slot.
    // 3. If promiseCapability.[[Resolve]] is not undefined, throw a TypeError exception.
    JSHandle<PromiseCapability> promiseCapability(thread, executor->GetCapability());
    if (!promiseCapability->GetResolve().IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Executor: resolve should be undefine!", JSTaggedValue::Undefined());
    }
    // 4. If promiseCapability.[[Reject]] is not undefined, throw a TypeError exception.
    if (!promiseCapability->GetReject().IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Executor: reject should be undefine!", JSTaggedValue::Undefined());
    }
    // 5. Set promiseCapability.[[Resolve]] to resolve.
    // 6. Set promiseCapability.[[Reject]] to reject.
    JSHandle<JSTaggedValue> resolve = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> reject = GetCallArg(argv, 1);
    promiseCapability->SetResolve(thread, resolve);
    promiseCapability->SetReject(thread, reject);
    // 7. Return undefined.
    return JSTaggedValue::Undefined();
}

// es6 25.4.4.1.2 Promise.all Resolve Element Functions
JSTaggedValue BuiltinsPromiseHandler::ResolveElementFunction(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseHandler, ResolveElementFunction);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSPromiseAllResolveElementFunction> func =
        JSHandle<JSPromiseAllResolveElementFunction>::Cast(GetConstructor(argv));
    // 1. Let alreadyCalled be the value of F's [[AlreadyCalled]] internal slot.
    JSHandle<PromiseRecord> alreadyCalled =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, func->GetAlreadyCalled()));
    // 2. If alreadyCalled.[[value]] is true, return undefined.
    if (alreadyCalled->GetValue().IsTrue()) {
        return JSTaggedValue::Undefined();
    }
    // 3. Set alreadyCalled.[[value]] to true.
    alreadyCalled->SetValue(thread, JSTaggedValue::True());
    // 4. Let index be the value of F's [[Index]] internal slot.
    JSHandle<JSTaggedValue> index(thread, func->GetIndex());
    // 5. Let values be the value of F's [[Values]] internal slot.
    JSHandle<PromiseRecord> values = JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, func->GetValues()));
    // 6. Let promiseCapability be the value of F's [[Capabilities]] internal slot.
    JSHandle<PromiseCapability> capa =
        JSHandle<PromiseCapability>::Cast(JSHandle<JSTaggedValue>(thread, func->GetCapabilities()));
    // 7. Let remainingElementsCount be the value of F's [[RemainingElements]] internal slot.
    JSHandle<PromiseRecord> remainCnt =
        JSHandle<PromiseRecord>::Cast(JSHandle<JSTaggedValue>(thread, func->GetRemainingElements()));
    // 8. Set values[index] to x.
    JSHandle<TaggedArray> arrayValues =
        JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, values->GetValue()));
    arrayValues->Set(thread, JSTaggedValue::ToUint32(thread, index), GetCallArg(argv, 0).GetTaggedValue());
    // 9. Set remainingElementsCount.[[value]] to remainingElementsCount.[[value]] - 1.
    remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue()));
    // 10. If remainingElementsCount.[[value]] is 0,
    if (remainCnt->GetValue().IsZero()) {
        // a. Let valuesArray be CreateArrayFromList(values).
        JSHandle<JSArray> jsArrayValues = JSArray::CreateArrayFromList(thread, arrayValues);
        // b. Return Call(promiseCapability.[[Resolve]], undefined, «valuesArray»).
        JSHandle<JSTaggedValue> capaResolve(thread, capa->GetResolve());
        JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, capaResolve, undefined, undefined, 1);
        info.SetCallArg(jsArrayValues.GetTaggedValue());
        return JSFunction::Call(&info);
    }
    // 11. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsPromiseHandler::AsyncAwaitFulfilled(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAsyncAwaitStatusFunction> func(GetConstructor(argv));
    return JSAsyncAwaitStatusFunction::AsyncFunctionAwaitFulfilled(argv->GetThread(), func, value).GetTaggedValue();
}

JSTaggedValue BuiltinsPromiseHandler::AsyncAwaitRejected(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());

    JSHandle<JSTaggedValue> reason = GetCallArg(argv, 0);
    JSHandle<JSAsyncAwaitStatusFunction> func(GetConstructor(argv));
    return JSAsyncAwaitStatusFunction::AsyncFunctionAwaitRejected(argv->GetThread(), func, reason).GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
