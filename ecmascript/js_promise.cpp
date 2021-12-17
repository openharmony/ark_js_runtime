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

#include "ecmascript/js_promise.h"
#include "ecmascript/base/error_type.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;

JSHandle<ResolvingFunctionsRecord> JSPromise::CreateResolvingFunctions(JSThread *thread,
                                                                       const JSHandle<JSPromise> &promise)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let alreadyResolved be a new Record { [[value]]: false }.
    JSHandle<PromiseRecord> record = factory->NewPromiseRecord();
    record->SetValue(thread, JSTaggedValue::False());

    // 2. Let resolve be a new built-in function object as defined in Promise Resolve Functions (25.4.1.3.2).
    JSHandle<JSPromiseReactionsFunction> resolve =
        factory->CreateJSPromiseReactionsFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::Resolve));
    // 3. Set the [[Promise]] internal slot of resolve to promise.
    resolve->SetPromise(thread, promise);
    // 4. Set the [[AlreadyResolved]] internal slot of resolve to alreadyResolved.
    resolve->SetAlreadyResolved(thread, record);
    // 5. Let reject be a new built-in function object as defined in Promise Reject Functions (25.4.1.3.1).
    JSHandle<JSPromiseReactionsFunction> reject =
        factory->CreateJSPromiseReactionsFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::Reject));
    // 6. Set the [[Promise]] internal slot of reject to promise.
    reject->SetPromise(thread, promise);
    // 7. Set the [[AlreadyResolved]] internal slot of reject to alreadyResolved.
    reject->SetAlreadyResolved(thread, record);
    // 8. Return a new Record { [[Resolve]]: resolve, [[Reject]]: reject }.
    JSHandle<ResolvingFunctionsRecord> reactions = factory->NewResolvingFunctionsRecord();
    reactions->SetResolveFunction(thread, resolve.GetTaggedValue());
    reactions->SetRejectFunction(thread, reject.GetTaggedValue());
    return reactions;
}

JSTaggedValue JSPromise::FulfillPromise(JSThread *thread, const JSHandle<JSPromise> &promise,
                                        const JSHandle<JSTaggedValue> &value)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Assert: the value of promise's [[PromiseState]] internal slot is "pending".
    JSHandle<JSTaggedValue> handleStatus(thread, promise->GetPromiseState());
    ASSERT_PRINT(JSTaggedValue::SameValue(handleStatus.GetTaggedValue(),
                                          JSTaggedValue(static_cast<int32_t>(PromiseStatus::PENDING))),
                 "FulfillPromise: state must be pending");
    // 2. Let reactions be the value of promise's [[PromiseFulfillReactions]] internal slot.
    JSHandle<TaggedQueue> reactions(thread, promise->GetPromiseFulfillReactions());
    // 3. Set the value of promise's [[PromiseResult]] internal slot to value.
    promise->SetPromiseResult(thread, value);
    // 4. Set the value of promise's [[PromiseFulfillReactions]] internal slot to undefined.
    promise->SetPromiseFulfillReactions(thread, globalConst->GetHandledUndefined());
    // 5. Set the value of promise's [[PromiseRejectReactions]] internal slot to undefined.
    promise->SetPromiseRejectReactions(thread, globalConst->GetHandledUndefined());
    // 6. Set the value of promise's [[PromiseState]] internal slot to "fulfilled".
    promise->SetPromiseState(thread, JSTaggedValue(static_cast<int32_t>(PromiseStatus::FULFILLED)));
    // 7. Return TriggerPromiseReactions(reactions, reason).
    return TriggerPromiseReactions(thread, reactions, value);
}

JSHandle<PromiseCapability> JSPromise::NewPromiseCapability(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If IsConstructor(C) is false, throw a TypeError exception.
    if (!obj->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "NewPromiseCapability: obj is not constructor!",
                                    factory->NewPromiseCapability());
    }
    // 2. NOTE C is assumed to be a constructor function that supports the parameter conventions of the Promise
    //    constructor (see 25.4.3.1).
    // 3. Let promiseCapability be a new PromiseCapability { [[Promise]]: undefined, [[Resolve]]: undefined,
    //    [[Reject]]: undefined }.
    JSHandle<PromiseCapability> promiseCapability = factory->NewPromiseCapability();
    // 4. Let executor be a new built-in function object as defined in GetCapabilitiesExecutor Functions
    //    (25.4.1.5.1).
    JSHandle<JSPromiseExecutorFunction> executor =
        factory->CreateJSPromiseExecutorFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::Executor));
    // 5. Set the [[Capability]] internal slot of executor to promiseCapability.
    executor->SetCapability(thread, promiseCapability.GetTaggedValue());
    // 6. Let promise be Construct(C, «executor»).
    // 7. ReturnIfAbrupt(promise).
    JSHandle<JSTaggedValue> newTarget(thread, JSTaggedValue::Undefined());
    InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(executor);
    JSTaggedValue result = JSFunction::Construct(thread, obj, 1, arguments->GetArgv(), newTarget);
    JSHandle<JSPromise> promise(thread, result);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, factory->NewPromiseCapability());
    // 8. If IsCallable(promiseCapability.[[Resolve]]) is false, throw a TypeError exception.
    if (!promiseCapability->GetResolve().IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "NewPromiseCapability: resolve is not a callable function!",
                                    factory->NewPromiseCapability());
    }
    // 9. If IsCallable(promiseCapability.[[Reject]]) is false, throw a TypeError exception.
    if (!promiseCapability->GetReject().IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "NewPromiseCapability: reject is not a callable function!",
                                    factory->NewPromiseCapability());
    }
    // 10. Set promiseCapability.[[Promise]] to promise.
    promiseCapability->SetPromise(thread, promise);
    // 11. Return promiseCapability.
    return promiseCapability;
}

bool JSPromise::IsPromise(const JSHandle<JSTaggedValue> &value)
{
    // 1. If Type(x) is not Object, return false.
    if (!value->IsECMAObject()) {
        return false;
    }
    // 2. If x does not have a [[PromiseState]] internal slot, return false.
    if (!value->IsJSPromise()) {
        return false;
    }
    // 3. Return true
    return true;
}

JSTaggedValue JSPromise::RejectPromise(JSThread *thread, const JSHandle<JSPromise> &promise,
                                       const JSHandle<JSTaggedValue> &reason)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Assert: the value of promise's [[PromiseState]] internal slot is "pending".
    JSHandle<JSTaggedValue> handleStatus(thread, promise->GetPromiseState());

    ASSERT_PRINT(JSTaggedValue::SameValue(handleStatus.GetTaggedValue(),
                                          JSTaggedValue(static_cast<int32_t>(PromiseStatus::PENDING))),
                 "RejectPromise: state must be pending");
    // 2. Let reactions be the value of promise's [[PromiseRejectReactions]] internal slot.
    JSHandle<TaggedQueue> reactions(thread, TaggedQueue::Cast(promise->GetPromiseRejectReactions().GetTaggedObject()));
    // 3. Set the value of promise's [[PromiseResult]] internal slot to reason.
    promise->SetPromiseResult(thread, reason);
    // 4. Set the value of promise's [[PromiseFulfillReactions]] internal slot to undefined.
    promise->SetPromiseFulfillReactions(thread, globalConst->GetHandledUndefined());
    // 5. Set the value of promise's [[PromiseRejectReactions]] internal slot to undefined.
    promise->SetPromiseRejectReactions(thread, globalConst->GetHandledUndefined());
    // 6. Set the value of promise's [[PromiseState]] internal slot to "rejected".
    promise->SetPromiseState(thread, JSTaggedValue(static_cast<int32_t>(PromiseStatus::REJECTED)));
    // 7. When a promise is rejected without any handlers, it is called with its operation argument set to "reject".
    if (!promise->GetPromiseIsHandled().ToBoolean()) {
        thread->GetEcmaVM()->PromiseRejectionTracker(promise, reason, PromiseRejectionEvent::REJECT);
    }
    // 8. Return TriggerPromiseReactions(reactions, reason).
    return TriggerPromiseReactions(thread, reactions, reason);
}

JSTaggedValue JSPromise::TriggerPromiseReactions(JSThread *thread, const JSHandle<TaggedQueue> &reactions,
                                                 const JSHandle<JSTaggedValue> &argument)
{
    // 1. Repeat for each reaction in reactions, in original insertion order
    // a. Perform EnqueueJob("PromiseJobs", PromiseReactionJob, «reaction, argument»).
    JSHandle<job::MicroJobQueue> job = thread->GetEcmaVM()->GetMicroJobQueue();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> promiseReactionsJob(globalEnv->GetPromiseReactionJob());
    JSMutableHandle<PromiseReaction> reaction(thread, JSTaggedValue::Undefined());
    while (!reactions->Empty()) {
        reaction.Update(reactions->Pop(thread));
        JSHandle<TaggedArray> arguments = factory->NewTaggedArray(2);  // 2 means the length of new array
        arguments->Set(thread, 0, reaction);
        arguments->Set(thread, 1, argument);
        job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, promiseReactionsJob, arguments);
    }
    // 2. Return undefined.
    return globalConst->GetUndefined();
}

JSHandle<JSTaggedValue> JSPromise::IfThrowGetThrowValue(JSThread *thread)
{
    return JSHandle<JSTaggedValue>(thread, thread->GetException());
}
}  // namespace panda::ecmascript
