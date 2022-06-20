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

#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
// 25.4.3.1 Promise ( executor )
JSTaggedValue BuiltinsPromise::PromiseConstructor([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. If NewTarget is undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "PromiseConstructor: NewTarget is undefined", JSTaggedValue::Exception());
    }
    // 2. If IsCallable(executor) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> executor = BuiltinsBase::GetCallArg(argv, 0);
    if (!executor->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "PromiseConstructor: executor is not callable", JSTaggedValue::Exception());
    }

    // 3. Let promise be OrdinaryCreateFromConstructor(NewTarget, "%PromisePrototype%",
    // «[[PromiseState]], [[PromiseResult]], [[PromiseFulfillReactions]], [[PromiseRejectReactions]]» ).
    // 4. ReturnIfAbrupt(promise).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSPromise> instancePromise =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Set promise's [[PromiseState]] internal slot to "pending".
    // 6. Set promise's [[PromiseFulfillReactions]] internal slot to a new empty List.
    // 7. Set promise's [[PromiseRejectReactions]] internal slot to a new empty List.
    // 8. Let resolvingFunctions be CreateResolvingFunctions(promise).
    JSHandle<ResolvingFunctionsRecord> resolvingFunction = JSPromise::CreateResolvingFunctions(thread, instancePromise);
    // 9. Let completion be Call(executor, undefined, «resolvingFunctions.[[Resolve]], resolvingFunctions.[[reject]])
    auto resolveFunc = resolvingFunction->GetResolveFunction();
    auto rejectFunc = resolvingFunction->GetRejectFunction();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    const size_t argsLength = 2; // 2: «resolvingFunctions.[[Resolve]], resolvingFunctions.[[Reject]]»
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, executor, undefined, undefined, argsLength);
    info.SetCallArg(resolveFunc, rejectFunc);
    JSTaggedValue taggedValue = JSFunction::Call(&info);
    JSHandle<JSTaggedValue> completionValue(thread, taggedValue);

    // 10. If completion is an abrupt completion, then
    // a. Let status be Call(resolvingFunctions.[[Reject]], undefined, «completion.[[value]]»).
    // b. ReturnIfAbrupt(status).
    if (thread->HasPendingException()) {
        completionValue = JSPromise::IfThrowGetThrowValue(thread);
        thread->ClearException();
        JSHandle<JSTaggedValue> reject(thread, resolvingFunction->GetRejectFunction());
        EcmaRuntimeCallInfo runtimeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);
        runtimeInfo.SetCallArg(completionValue.GetTaggedValue());
        JSFunction::Call(&runtimeInfo);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // 11. Return promise.
    return instancePromise.GetTaggedValue();
}

// 25.4.4.1 Promise.all ( iterable )
JSTaggedValue BuiltinsPromise::All(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, All);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. Let C be the this value.
    JSHandle<JSTaggedValue> ctor = GetThis(argv);
    // 2. If Type(C) is not Object, throw a TypeError exception.
    if (!ctor->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Promise ALL: this value is not object", JSTaggedValue::Exception());
    }
    // 3. Let S be Get(C, @@species).
    // 4. ReturnIfAbrupt(S).
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> sctor = JSObject::GetProperty(thread, ctor, speciesSymbol).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, sctor.GetTaggedValue());

    // 5. If S is neither undefined nor null, let C be S.
    if (!sctor->IsUndefined() && !sctor->IsNull()) {
        ctor = sctor;
    }
    // 6. Let promiseCapability be NewPromiseCapability(C).
    JSHandle<PromiseCapability> capa = JSPromise::NewPromiseCapability(thread, ctor);
    // 7. ReturnIfAbrupt(promiseCapability).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, capa.GetTaggedValue());
    // 8. Let iterator be GetIterator(iterable).
    JSHandle<JSTaggedValue> itor = JSIterator::GetIterator(thread, GetCallArg(argv, 0));
    // 9. IfAbruptRejectPromise(iterator, promiseCapability).
    if (thread->HasPendingException()) {
        itor = JSPromise::IfThrowGetThrowValue(thread);
    }
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, itor, capa);

    // 10. Let iteratorRecord be Record {[[iterator]]: iterator, [[done]]: false}.
    bool done = false;
    JSHandle<PromiseIteratorRecord> itRecord = factory->NewPromiseIteratorRecord(itor, done);
    // 11. Let result be PerformPromiseAll(iteratorRecord, C, promiseCapability).
    JSHandle<CompletionRecord> result = PerformPromiseAll(thread, itRecord, ctor, capa);
    // 12. If result is an abrupt completion,
    if (result->IsThrow()) {
        thread->ClearException();
        // a. If iteratorRecord.[[done]] is false, let result be IteratorClose(iterator, result).
        // b. IfAbruptRejectPromise(result, promiseCapability).
        if (!itRecord->GetDone()) {
            JSHandle<JSTaggedValue> closeVal =
                JSIterator::IteratorClose(thread, itor, JSHandle<JSTaggedValue>::Cast(result));
            if (closeVal.GetTaggedValue().IsRecord()) {
                result = JSHandle<CompletionRecord>::Cast(closeVal);
                RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, capa);
                return result->GetValue();
            }
        }
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, capa);
        return result->GetValue();
    }
    // 13. Return Completion(result).
    return result->GetValue();
}

// 25.4.4.3 Promise.race ( iterable )
JSTaggedValue BuiltinsPromise::Race(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Race);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    // 1. Let C be the this value.
    // 2. If Type(C) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    if (!thisValue->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Race: this value is not object", JSTaggedValue::Exception());
    }
    // 3. Let S be Get(C, @@species).
    // 4. ReturnIfAbrupt(S).
    // 5. If S is neither undefined nor null, let C be S.
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesConstructor = JSObject::GetProperty(thread, thisValue, speciesSymbol).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!(speciesConstructor->IsUndefined() || speciesConstructor->IsNull())) {
        thisValue = speciesConstructor;
    }

    // 6. Let promiseCapability be NewPromiseCapability(C).
    // 7. ReturnIfAbrupt(promiseCapability).
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, thisValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 8. Let iterator be GetIterator(iterable).
    // 9. IfAbruptRejectPromise(iterator, promiseCapability).
    JSHandle<JSTaggedValue> iterable = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, iterable);
    if (thread->HasPendingException()) {
        iterator = JSPromise::IfThrowGetThrowValue(thread);
    }
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, iterator, promiseCapability);

    // 10. Let iteratorRecord be Record {[[iterator]]: iterator, [[done]]: false}.
    bool done = false;
    JSHandle<PromiseIteratorRecord> iteratorRecord = factory->NewPromiseIteratorRecord(iterator, done);

    // 11. Let result be PerformPromiseRace(iteratorRecord, promiseCapability, C).
    // 12. If result is an abrupt completion, then
    //     a. If iteratorRecord.[[done]] is false, let result be IteratorClose(iterator,result).
    //     b. IfAbruptRejectPromise(result, promiseCapability).
    // 13. Return Completion(result).
    JSHandle<CompletionRecord> result = PerformPromiseRace(thread, iteratorRecord, promiseCapability, thisValue);
    if (result->IsThrow()) {
        thread->ClearException();
        if (!iteratorRecord->GetDone()) {
            JSHandle<JSTaggedValue> value =
                JSIterator::IteratorClose(thread, iterator, JSHandle<JSTaggedValue>::Cast(result));
            if (value.GetTaggedValue().IsCompletionRecord()) {
                result = JSHandle<CompletionRecord>(value);
                RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, promiseCapability);
                return result->GetValue();
            }
        }
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, promiseCapability);
        return result->GetValue();
    }
    return result->GetValue();
}

// 25.4.4.5 Promise.resolve ( x )
JSTaggedValue BuiltinsPromise::Resolve(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Resolve);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Let C be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. If Type(C) is not Object, throw a TypeError exception.
    if (!thisValue->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Resolve: this value is not object", JSTaggedValue::Exception());
    }
    // 3. If IsPromise(x) is true,
    //     a. Let xConstructor be Get(x, "constructor").
    //     b. ReturnIfAbrupt(xConstructor).
    //     c. If SameValue(xConstructor, C) is true, return x.
    JSHandle<JSTaggedValue> xValue = BuiltinsBase::GetCallArg(argv, 0);
    if (xValue->IsJSPromise()) {
        JSHandle<JSTaggedValue> ctorKey(globalConst->GetHandledConstructorString());
        JSHandle<JSTaggedValue> ctorValue = JSObject::GetProperty(thread, xValue, ctorKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (JSTaggedValue::SameValue(ctorValue.GetTaggedValue(), thisValue.GetTaggedValue())) {
            JSHandle<JSObject> value = JSHandle<JSObject>::Cast(xValue);
            return value.GetTaggedValue();
        }
    }
    // 4. Let promiseCapability be NewPromiseCapability(C).
    // 5. ReturnIfAbrupt(promiseCapability).
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, thisValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. Let resolveResult be Call(promiseCapability.[[Resolve]], undefined, «x»).
    // 7. ReturnIfAbrupt(resolveResult).
    JSHandle<JSTaggedValue> resolve(thread, promiseCapability->GetResolve());
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    info.SetCallArg(xValue.GetTaggedValue());
    JSFunction::Call(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 8. Return promiseCapability.[[Promise]].
    JSHandle<JSObject> promise(thread, promiseCapability->GetPromise());
    return promise.GetTaggedValue();
}

// 25.4.4.4 Promise.reject ( r )
JSTaggedValue BuiltinsPromise::Reject(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Reject);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // 1. Let C be the this value.
    // 2. If Type(C) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    if (!thisValue->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reject: this value is not object", JSTaggedValue::Exception());
    }

    // 3. Let promiseCapability be NewPromiseCapability(C).
    // 4. ReturnIfAbrupt(promiseCapability).
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, thisValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let rejectResult be Call(promiseCapability.[[Reject]], undefined, «r»).
    // 6. ReturnIfAbrupt(rejectResult).
    JSHandle<JSTaggedValue> reason = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> reject(thread, promiseCapability->GetReject());
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);
    info.SetCallArg(reason.GetTaggedValue());
    JSFunction::Call(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. Return promiseCapability.[[Promise]].
    JSHandle<JSObject> promise(thread, promiseCapability->GetPromise());
    return promise.GetTaggedValue();
}

// 25.4.4.6 get Promise [ @@species ]
JSTaggedValue BuiltinsPromise::GetSpecies([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    return JSTaggedValue(GetThis(argv).GetTaggedValue());
}

// 25.4.5.1 Promise.prototype.catch ( onRejected )
JSTaggedValue BuiltinsPromise::Catch(EcmaRuntimeCallInfo *argv)
{
    // 1. Let promise be the this value.
    // 2. Return Invoke(promise, "then", «undefined, onRejected»).
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Catch);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> promise = GetThis(argv);
    JSHandle<JSTaggedValue> thenKey = globalConst->GetHandledPromiseThenString();
    JSHandle<JSTaggedValue> reject = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: «undefined, onRejected»
    info.SetCallArg(undefined.GetTaggedValue(), reject.GetTaggedValue());
    return JSFunction::Invoke(&info, thenKey);
}

// 25.4.5.3 Promise.prototype.then ( onFulfilled , onRejected )
JSTaggedValue BuiltinsPromise::Then(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Then);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();

    // 1. Let promise be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. If IsPromise(promise) is false, throw a TypeError exception.
    if (!thisValue->IsJSPromise()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Then: thisValue is not promise!", JSTaggedValue::Exception());
    }
    // 3. Let C be SpeciesConstructor(promise, %Promise%).
    // 4. ReturnIfAbrupt(C).
    JSHandle<JSObject> promise = JSHandle<JSObject>::Cast(thisValue);
    JSHandle<JSTaggedValue> defaultFunc = JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction());
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, promise, defaultFunc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let resultCapability be NewPromiseCapability(C).
    // 6. ReturnIfAbrupt(resultCapability).
    JSHandle<PromiseCapability> resultCapability = JSPromise::NewPromiseCapability(thread, constructor);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> onFulfilled = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> onRejected = BuiltinsBase::GetCallArg(argv, 1);

    // 7. Return PerformPromiseThen(promise, onFulfilled, onRejected, resultCapability).
    return PerformPromiseThen(thread, JSHandle<JSPromise>::Cast(promise), onFulfilled, onRejected, resultCapability);
}

JSTaggedValue BuiltinsPromise::PerformPromiseThen(JSThread *thread, const JSHandle<JSPromise> &promise,
                                                  const JSHandle<JSTaggedValue> &onFulfilled,
                                                  const JSHandle<JSTaggedValue> &onRejected,
                                                  const JSHandle<PromiseCapability> &capability)
{
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<job::MicroJobQueue> job = ecmaVm->GetMicroJobQueue();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSMutableHandle<JSTaggedValue> fulfilled(thread, onFulfilled.GetTaggedValue());
    auto globalConst = thread->GlobalConstants();
    if (!fulfilled->IsCallable()) {
        fulfilled.Update(globalConst->GetIdentityString());
    }
    JSMutableHandle<JSTaggedValue> rejected(thread, onRejected.GetTaggedValue());
    if (!rejected->IsCallable()) {
        rejected.Update(globalConst->GetThrowerString());
    }
    JSHandle<PromiseReaction> fulfillReaction = factory->NewPromiseReaction();
    fulfillReaction->SetPromiseCapability(thread, capability.GetTaggedValue());
    fulfillReaction->SetHandler(thread, fulfilled.GetTaggedValue());

    JSHandle<PromiseReaction> rejectReaction = factory->NewPromiseReaction();
    rejectReaction->SetPromiseCapability(thread, capability.GetTaggedValue());
    rejectReaction->SetHandler(thread, rejected.GetTaggedValue());

    PromiseState state = promise->GetPromiseState();
    if (state == PromiseState::PENDING) {
        JSHandle<TaggedQueue> fulfillReactions(thread, promise->GetPromiseFulfillReactions());
        TaggedQueue *newQueue =
            TaggedQueue::Push(thread, fulfillReactions, JSHandle<JSTaggedValue>::Cast(fulfillReaction));
        promise->SetPromiseFulfillReactions(thread, JSTaggedValue(newQueue));

        JSHandle<TaggedQueue> rejectReactions(thread, promise->GetPromiseRejectReactions());
        newQueue = TaggedQueue::Push(thread, rejectReactions, JSHandle<JSTaggedValue>::Cast(rejectReaction));
        promise->SetPromiseRejectReactions(thread, JSTaggedValue(newQueue));
    } else if (state == PromiseState::FULFILLED) {
        JSHandle<TaggedArray> argv = factory->NewTaggedArray(2);  // 2: 2 means two args stored in array
        argv->Set(thread, 0, fulfillReaction.GetTaggedValue());
        argv->Set(thread, 1, promise->GetPromiseResult());

        JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
        job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, promiseReactionsJob, argv);
    } else if (state == PromiseState::REJECTED) {
        JSHandle<TaggedArray> argv = factory->NewTaggedArray(2);  // 2: 2 means two args stored in array
        argv->Set(thread, 0, rejectReaction.GetTaggedValue());
        argv->Set(thread, 1, promise->GetPromiseResult());
        // When a handler is added to a rejected promise for the first time, it is called with its operation
        // argument set to "handle".
        if (!promise->GetPromiseIsHandled()) {
            JSHandle<JSTaggedValue> reason(thread, JSTaggedValue::Null());
            thread->GetEcmaVM()->PromiseRejectionTracker(promise, reason, PromiseRejectionEvent::HANDLE);
        }
        JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
        job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, promiseReactionsJob, argv);
    }
    promise->SetPromiseIsHandled(true);
    return capability->GetPromise();
}

JSHandle<CompletionRecord> BuiltinsPromise::PerformPromiseAll(JSThread *thread,
                                                              const JSHandle<PromiseIteratorRecord> &itRecord,
                                                              const JSHandle<JSTaggedValue> &ctor,
                                                              const JSHandle<PromiseCapability> &capa)
{
    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Assert: constructor is a constructor function.
    ASSERT_PRINT(ctor->IsConstructor(), "PerformPromiseAll is not constructor");
    // 2. Assert: resultCapability is a PromiseCapability record. (not need)
    // 3. Let values be a new empty List.
    JSHandle<PromiseRecord> values = factory->NewPromiseRecord();
    JSHandle<TaggedArray> emptyArray = factory->EmptyArray();
    values->SetValue(thread, emptyArray);
    // 4. Let remainingElementsCount be a new Record { [[value]]: 1 }.
    JSHandle<PromiseRecord> remainCnt = factory->NewPromiseRecord();
    remainCnt->SetValue(thread, JSTaggedNumber(1));
    // 5. Let index be 0.
    uint32_t index = 0;
    // 6. Repeat
    JSHandle<JSTaggedValue> itor(thread, itRecord->GetIterator());
    JSMutableHandle<JSTaggedValue> next(thread, globalConst->GetUndefined());
    while (true) {
        // a. Let next be IteratorStep(iteratorRecord.[[iterator]]).
        next.Update(JSIterator::IteratorStep(thread, itor).GetTaggedValue());
        // b. If next is an abrupt completion, set iteratorRecord.[[done]] to true.
        if (thread->HasPendingException()) {
            itRecord->SetDone(true);
            next.Update(JSPromise::IfThrowGetThrowValue(thread).GetTaggedValue());
        }
        // c. ReturnIfAbrupt(next).
        RETURN_COMPLETION_IF_ABRUPT(thread, next);
        // d. If next is false,
        JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
        if (next->IsFalse()) {
            // i. Set iteratorRecord.[[done]] to true.
            itRecord->SetDone(true);
            // ii. Set remainingElementsCount.[[value]] to remainingElementsCount.[[value]] − 1.
            remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue()));
            // iii. If remainingElementsCount.[[value]] is 0,
            if (remainCnt->GetValue().IsZero()) {
                // 1. Let valuesArray be CreateArrayFromList(values).
                JSHandle<JSArray> jsArrayValues =
                    JSArray::CreateArrayFromList(thread, JSHandle<TaggedArray>(thread, values->GetValue()));
                // 2. Let resolveResult be Call(resultCapability.[[Resolve]], undefined, «valuesArray»).
                JSHandle<JSTaggedValue> resCapaFunc(thread, capa->GetResolve());
                EcmaRuntimeCallInfo info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, resCapaFunc, undefined, undefined, 1);
                info.SetCallArg(jsArrayValues.GetTaggedValue());
                JSTaggedValue resolveRes = JSFunction::Call(&info);
                // 3. ReturnIfAbrupt(resolveResult)
                JSHandle<JSTaggedValue> resolveAbrupt(thread, resolveRes);
                RETURN_COMPLETION_IF_ABRUPT(thread, resolveAbrupt);
            }
            // iv. Return resultCapability.[[Promise]].
            JSHandle<CompletionRecord> resRecord = factory->NewCompletionRecord(
                CompletionRecordType::NORMAL, JSHandle<JSTaggedValue>(thread, capa->GetPromise()));
            return resRecord;
        }
        // e. Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextVal = JSIterator::IteratorValue(thread, next);
        // f. If nextValue is an abrupt completion, set iteratorRecord.[[done]] to true.
        if (thread->HasPendingException()) {
            itRecord->SetDone(true);
            nextVal = JSHandle<JSTaggedValue>(thread, thread->GetException());
        }

        // g. ReturnIfAbrupt(nextValue).
        RETURN_COMPLETION_IF_ABRUPT(thread, nextVal);
        // h. Append undefined to values.
        JSHandle<TaggedArray> valuesArray =
            JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, values->GetValue()));
        valuesArray = TaggedArray::SetCapacity(thread, valuesArray, index + 1);
        valuesArray->Set(thread, index, JSTaggedValue::Undefined());
        values->SetValue(thread, valuesArray);
        // i. Let nextPromise be Invoke(constructor, "resolve", «‍nextValue»).
        JSHandle<JSTaggedValue> resolveKey = globalConst->GetHandledPromiseResolveString();
        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, ctor, undefined, 1);
        info.SetCallArg(nextVal.GetTaggedValue());
        JSTaggedValue taggedNextPromise = JSFunction::Invoke(&info, resolveKey);
        // j. ReturnIfAbrupt(nextPromise).
        JSHandle<JSTaggedValue> nextPromise(thread, taggedNextPromise);
        RETURN_COMPLETION_IF_ABRUPT(thread, nextPromise);
        // k. Let resolveElement be a new built-in function object as defined in Promise.all
        //    Resolve Element Functions.
        JSHandle<JSPromiseAllResolveElementFunction> resoleveElement = factory->NewJSPromiseAllResolveElementFunction();
        // l. Set the [[AlreadyCalled]] internal slot of resolveElement to a new Record {[[value]]: false }.
        JSHandle<PromiseRecord> falseRecord = factory->NewPromiseRecord();
        falseRecord->SetValue(thread, JSTaggedValue::False());
        resoleveElement->SetAlreadyCalled(thread, falseRecord);
        // m. Set the [[Index]] internal slot of resolveElement to index.
        resoleveElement->SetIndex(thread, JSTaggedValue(index));
        // n. Set the [[Values]] internal slot of resolveElement to values.
        resoleveElement->SetValues(thread, values);
        // o. Set the [[Capabilities]] internal slot of resolveElement to resultCapability.
        resoleveElement->SetCapabilities(thread, capa);
        // p. Set the [[RemainingElements]] internal slot of resolveElement to remainingElementsCount.
        resoleveElement->SetRemainingElements(thread, remainCnt);
        // q. Set remainingElementsCount.[[value]] to remainingElementsCount.[[value]] + 1.
        remainCnt->SetValue(thread, ++JSTaggedNumber(remainCnt->GetValue()));
        // r. Let result be Invoke(nextPromise, "then", «‍resolveElement, resultCapability.[[Reject]]»).
        JSHandle<JSTaggedValue> thenKey = globalConst->GetHandledPromiseThenString();
        EcmaRuntimeCallInfo runtimeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, nextPromise,
            undefined, 2); // 2: «‍resolveElement, resultCapability.[[Reject]]»
        runtimeInfo.SetCallArg(resoleveElement.GetTaggedValue(), capa->GetReject());
        JSTaggedValue taggedResult = JSFunction::Invoke(&runtimeInfo, thenKey);
        JSHandle<JSTaggedValue> result(thread, taggedResult);
        // s. ReturnIfAbrupt(result).
        RETURN_COMPLETION_IF_ABRUPT(thread, result);
        // t. Set index to index + 1.
        ++index;
    }
}

JSHandle<CompletionRecord> BuiltinsPromise::PerformPromiseRace(JSThread *thread,
                                                               const JSHandle<PromiseIteratorRecord> &iteratorRecord,
                                                               const JSHandle<PromiseCapability> &capability,
                                                               const JSHandle<JSTaggedValue> &constructor)
{
    // 1. Repeat
    //    a. Let next be IteratorStep(iteratorRecord.[[iterator]]).
    //    b. If next is an abrupt completion, set iteratorRecord.[[done]] to true.
    //    c. ReturnIfAbrupt(next).
    //    d. If next is false, then
    //       i. Set iteratorRecord.[[done]] to true.
    //       ii. Return promiseCapability.[[Promise]].
    //    e. Let nextValue be IteratorValue(next).
    //    f. If nextValue is an abrupt completion, set iteratorRecord.[[done]] to true.
    //    g. ReturnIfAbrupt(nextValue).
    //    h. Let nextPromise be Invoke(C, "resolve", «nextValue»).
    //    i. ReturnIfAbrupt(nextPromise).
    //    j. Let result be Invoke(nextPromise, "then", «promiseCapability.[[Resolve]], promiseCapability.[[Reject]]»).
    //    k. ReturnIfAbrupt(result).
    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> iterator(thread, iteratorRecord->GetIterator());
    JSMutableHandle<JSTaggedValue> next(thread, globalConst->GetUndefined());
    JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
    while (true) {
        next.Update(JSIterator::IteratorStep(thread, iterator).GetTaggedValue());
        if (thread->HasPendingException()) {
            iteratorRecord->SetDone(true);
            next.Update(JSPromise::IfThrowGetThrowValue(thread).GetTaggedValue());
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, next);
        if (next->IsFalse()) {
            iteratorRecord->SetDone(true);
            JSHandle<JSTaggedValue> promise(thread, capability->GetPromise());
            JSHandle<CompletionRecord> completionRecord =
                factory->NewCompletionRecord(CompletionRecordType::NORMAL, promise);
            return completionRecord;
        }
        JSHandle<JSTaggedValue> nextValue = JSIterator::IteratorValue(thread, next);
        if (thread->HasPendingException()) {
            iteratorRecord->SetDone(true);
            nextValue = JSPromise::IfThrowGetThrowValue(thread);
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, nextValue);
        JSHandle<JSTaggedValue> resolveStr = globalConst->GetHandledPromiseResolveString();

        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, constructor, undefined, 1);
        info.SetCallArg(nextValue.GetTaggedValue());
        JSTaggedValue result = JSFunction::Invoke(&info, resolveStr);
        JSHandle<JSTaggedValue> nextPromise(thread, result);
        if (thread->HasPendingException()) {
            nextPromise = JSPromise::IfThrowGetThrowValue(thread);
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, nextPromise);

        JSHandle<JSTaggedValue> thenStr = globalConst->GetHandledPromiseThenString();

        EcmaRuntimeCallInfo runtimeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, nextPromise, undefined, 2); // 2: two args
        runtimeInfo.SetCallArg(capability->GetResolve(), capability->GetReject());
        result = JSFunction::Invoke(&runtimeInfo, thenStr);
        JSHandle<JSTaggedValue> handleResult(thread, result);
        if (thread->HasPendingException()) {
            handleResult = JSPromise::IfThrowGetThrowValue(thread);
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, handleResult);
    }
}

JSTaggedValue BuiltinsPromise::GetPromiseResolve(JSThread *thread, JSHandle<JSTaggedValue> promiseConstructor)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Let promiseResolve be ? Get(promiseConstructor, "resolve").
    JSHandle<JSTaggedValue> resolveKey = globalConst->GetHandledPromiseResolveString();
    JSHandle<JSTaggedValue> promiseResolve = JSObject::GetProperty(thread, promiseConstructor, resolveKey).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. If IsCallable(promiseResolve) is false, throw a TypeError exception.
    if (!promiseResolve->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "promiseResolve is not callable", JSTaggedValue::Exception());
    }
    // 3. Return promiseResolve.
    return promiseResolve.GetTaggedValue();
}

// 27.2.4.3 Promise.any ( iterable )
JSTaggedValue BuiltinsPromise::Any(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Any);
    JSThread *thread = argv->GetThread();
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let C be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. Let promiseCapability be ? NewPromiseCapability(C).
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, thisValue);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, promiseCapability.GetTaggedValue());
    // 3. Let promiseResolve be GetPromiseResolve(C).
    JSHandle<JSTaggedValue> promiseResolve(thread, BuiltinsPromise::GetPromiseResolve(thread, thisValue));
    // 4. IfAbruptRejectPromise(promiseResolve, promiseCapability).
    if (thread->HasPendingException()) {
        promiseResolve = JSPromise::IfThrowGetThrowValue(thread);
    }
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, promiseResolve, promiseCapability);
    // 5. Let iteratorRecord be GetIterator(iterable).
    JSHandle<JSTaggedValue> iterable = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, iterable);
    // 6. IfAbruptRejectPromise(iteratorRecord, promiseCapability).
    if (thread->HasPendingException()) {
        iterator = JSPromise::IfThrowGetThrowValue(thread);
    }
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, iterator, promiseCapability);
    // Let iteratorRecord be Record {[[iterator]]: iterator, [[done]]: false}.
    JSHandle<PromiseIteratorRecord> iteratorRecord = factory->NewPromiseIteratorRecord(iterator, false);
    // 7. Let result be PerformPromiseAny(iteratorRecord, C, promiseCapability, promiseResolve).
    JSHandle<CompletionRecord> result = PerformPromiseAny(thread, iteratorRecord, thisValue,
                                                          promiseCapability, promiseResolve);
    // 8. If result is an abrupt completion, then
    if (result->IsThrow()) {
        thread->ClearException();
        // a. If iteratorRecord.[[Done]] is false, set result to IteratorClose(iteratorRecord, result).
        // b. IfAbruptRejectPromise(result, promiseCapability).
        if (!iteratorRecord->GetDone()) {
            JSHandle<JSTaggedValue> resultHandle = JSHandle<JSTaggedValue>::Cast(result);
            JSHandle<JSTaggedValue> closeVal = JSIterator::IteratorClose(thread, iterator, resultHandle);
            if (closeVal.GetTaggedValue().IsCompletionRecord()) {
                result = JSHandle<CompletionRecord>(closeVal);
                RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, promiseCapability);
                return result->GetValue();
            }
        }
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, promiseCapability);
        return result->GetValue();
    }
    // 9. Return ? result.
    return result->GetValue();
}

JSHandle<CompletionRecord> BuiltinsPromise::PerformPromiseAny(JSThread *thread,
                                                              const JSHandle<PromiseIteratorRecord> &iteratorRecord,
                                                              const JSHandle<JSTaggedValue> &constructor,
                                                              const JSHandle<PromiseCapability> &resultCapability,
                                                              const JSHandle<JSTaggedValue> &promiseResolve)
{
    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let errors be a new empty List.
    JSHandle<PromiseRecord> errors = factory->NewPromiseRecord();
    JSHandle<TaggedArray> emptyArray = factory->EmptyArray();
    errors->SetValue(thread, emptyArray);
    // 2. Let remainingElementsCount be the Record { [[Value]]: 1 }.
    JSHandle<PromiseRecord> remainCnt = factory->NewPromiseRecord();
    remainCnt->SetValue(thread, JSTaggedNumber(1));
    // 3. Let index be 0.
    uint32_t index = 0;
    // 4. Repeat,
    JSHandle<JSTaggedValue> iter(thread, iteratorRecord->GetIterator());
    JSMutableHandle<JSTaggedValue> next(thread, globalConst->GetUndefined());
    JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
    while (true) {
        // a. Let next be IteratorStep(iteratorRecord).
        next.Update(JSIterator::IteratorStep(thread, iter).GetTaggedValue());
        // b. If next is an abrupt completion, set iteratorRecord.[[Done]] to true.
        if (thread->HasPendingException()) {
            iteratorRecord->SetDone(true);
            next.Update(JSPromise::IfThrowGetThrowValue(thread).GetTaggedValue());
        }
        // c. ReturnIfAbrupt(next).
        RETURN_COMPLETION_IF_ABRUPT(thread, next);
        // d. If next is false, then
        if (next->IsFalse()) {
            // i. Set iteratorRecord.[[Done]] to true.
            iteratorRecord->SetDone(true);
            // ii. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
            remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue()));
            // iii. If remainingElementsCount.[[Value]] is 0, then
            if (remainCnt->GetValue().IsZero()) {
                // 1. Let error be a newly created AggregateError object.
                JSHandle<JSObject> error = factory->NewJSAggregateError();
                // 2. Perform ! DefinePropertyOrThrow(error, "errors", PropertyDescriptor { [[Configurable]]: true,
                //    [[Enumerable]]: false, [[Writable]]: true, [[Value]]: ! CreateArrayFromList(errors) }).
                JSHandle<JSTaggedValue> errorsKey(thread, globalConst->GetErrorsString());
                JSHandle<TaggedArray> errorsArray =
                    JSHandle<TaggedArray>::Cast(JSHandle<JSTaggedValue>(thread, errors->GetValue()));
                JSHandle<JSTaggedValue> errorsValue(JSArray::CreateArrayFromList(thread, errorsArray));
                PropertyDescriptor msgDesc(thread, errorsValue, true, false, true);
                JSHandle<JSTaggedValue> errorTagged = JSHandle<JSTaggedValue>::Cast(error);
                JSTaggedValue::DefinePropertyOrThrow(thread, errorTagged, errorsKey, msgDesc);
                // 3. Return ThrowCompletion(error).
                JSHandle<JSTaggedValue> errorCompletion(
                    factory->NewCompletionRecord(CompletionRecordType::THROW, errorTagged));
                JSHandle<CompletionRecord> errorResult = JSHandle<CompletionRecord>::Cast(errorCompletion);
                return errorResult;
            }
            // iv. Return resultCapability.[[Promise]].
            JSHandle<JSTaggedValue> resultCapabilityHandle(thread, resultCapability->GetPromise());
            JSHandle<CompletionRecord> resRecord = factory->NewCompletionRecord(
                CompletionRecordType::NORMAL, resultCapabilityHandle);
            return resRecord;
        }
        // e. Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextVal = JSIterator::IteratorValue(thread, next);
        // f. If nextValue is an abrupt completion, set iteratorRecord.[[Done]] to true.
        if (thread->HasPendingException()) {
            iteratorRecord->SetDone(true);
            nextVal = JSHandle<JSTaggedValue>(thread, thread->GetException());
        }
        // g. ReturnIfAbrupt(nextValue).
        RETURN_COMPLETION_IF_ABRUPT(thread, nextVal);
        // h. Append undefined to errors.
        JSHandle<JSTaggedValue> errorsHandle(thread, errors->GetValue());
        JSHandle<TaggedArray> errorsArray = JSHandle<TaggedArray>::Cast(errorsHandle);
        errorsArray = TaggedArray::SetCapacity(thread, errorsArray, index + 1);
        errorsArray->Set(thread, index, JSTaggedValue::Undefined());
        errors->SetValue(thread, errorsArray);
        // i. Let nextPromise be ? Call(promiseResolve, constructor, « nextValue »).
        EcmaRuntimeCallInfo taggedInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, promiseResolve, constructor, undefined, 1);
        taggedInfo.SetCallArg(nextVal.GetTaggedValue());
        JSTaggedValue taggedNextPromise = JSFunction::Call(&taggedInfo);
        JSHandle<JSTaggedValue> nextPromise(thread, taggedNextPromise);
        if (thread->HasPendingException()) {
            JSHandle<JSTaggedValue> promiseResult = JSPromise::IfThrowGetThrowValue(thread);
            JSHandle<CompletionRecord> completionRecord =
                factory->NewCompletionRecord(CompletionRecordType::THROW, promiseResult);
            return completionRecord;
        }
        // j. Let stepsRejected be the algorithm steps defined in Promise.any Reject Element Functions.
        // k. Let lengthRejected be the number of non-optional parameters of the function definition in
        //    Promise.any Reject Element Functions.
        // l. Let onRejected be CreateBuiltinFunction(stepsRejected, lengthRejected, "", « [[AlreadyCalled]],
        //    [[Index]], [[Errors]], [[Capability]], [[RemainingElements]] »).
        JSHandle<JSPromiseAnyRejectElementFunction> onRejected = factory->NewJSPromiseAnyRejectElementFunction();
        // m. Set onRejected.[[AlreadyCalled]] to false.
        onRejected->SetAlreadyCalled(thread, JSTaggedValue::False());
        // n. Set onRejected.[[Index]] to index.
        onRejected->SetIndex(index);
        // o. Set onRejected.[[Errors]] to errors.
        onRejected->SetErrors(thread, errors);
        // p. Set onRejected.[[Capability]] to resultCapability.
        onRejected->SetCapability(thread, resultCapability);
        // q. Set onRejected.[[RemainingElements]] to remainingElementsCount.
        onRejected->SetRemainingElements(thread, remainCnt);
        // r. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] + 1.
        remainCnt->SetValue(thread, ++JSTaggedNumber(remainCnt->GetValue()));
        // s. Perform ? Invoke(nextPromise, "then", « resultCapability.[[Resolve]], onRejected »).
        JSHandle<JSTaggedValue> thenKey = globalConst->GetHandledPromiseThenString();
        JSHandle<JSTaggedValue> resCapaFunc(thread, resultCapability->GetResolve());
        EcmaRuntimeCallInfo invokeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, nextPromise, undefined, 2); // 2: two args
        invokeInfo.SetCallArg(resCapaFunc.GetTaggedValue(), onRejected.GetTaggedValue());
        JSFunction::Invoke(&invokeInfo, thenKey);
        if (thread->HasPendingException()) {
            JSHandle<JSTaggedValue> taggedResult = JSPromise::IfThrowGetThrowValue(thread);
            JSHandle<CompletionRecord> completionRecord =
                factory->NewCompletionRecord(CompletionRecordType::THROW, taggedResult);
            return completionRecord;
        }
        // t. Set index to index + 1.
        ++index;
    }
}

// 25.6.4.2 Promise.allSettled ( iterable )
JSTaggedValue BuiltinsPromise::AllSettled(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, AllSettled);
    JSThread *thread = argv->GetThread();
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let C be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. Let promiseCapability be ? NewPromiseCapability(C).
    JSHandle<PromiseCapability> promiseCapability = JSPromise::NewPromiseCapability(thread, thisValue);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, promiseCapability.GetTaggedValue());
    // 3. Let promiseResolve be Completion(GetPromiseResolve(C)).
    JSHandle<JSTaggedValue> promiseResolve(thread, BuiltinsPromise::GetPromiseResolve(thread, thisValue));
    // 4. IfAbruptRejectPromise(promiseResolve, promiseCapability).
    if (thread->HasPendingException()) {
        promiseResolve = JSPromise::IfThrowGetThrowValue(thread);
    }
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, promiseResolve, promiseCapability);
    // 5. Let iteratorRecord be Completion(GetIterator(iterable)).
    JSHandle<JSTaggedValue> iterable = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, iterable);
    // 6. IfAbruptRejectPromise(iteratorRecord, promiseCapability).
    if (thread->HasPendingException()) {
        iterator = JSPromise::IfThrowGetThrowValue(thread);
    }
    RETURN_REJECT_PROMISE_IF_ABRUPT(thread, iterator, promiseCapability);
    // Let iteratorRecord be Record {[[iterator]]: iterator, [[done]]: false}.
    JSHandle<PromiseIteratorRecord> iteratorRecord = factory->NewPromiseIteratorRecord(iterator, false);
    // 7. Let result be PerformPromiseAllSettled(iteratorRecord, C, promiseCapability).
    JSHandle<CompletionRecord> result = PerformPromiseAllSettled(thread, iteratorRecord, thisValue,
                                                                 promiseCapability, promiseResolve);
    // 8. If result is an abrupt completion, then
    if (result->IsThrow()) {
        thread->ClearException();
        // a. If iteratorRecord.[[Done]] is false, set result to IteratorClose(iteratorRecord, result).
        if (!iteratorRecord->GetDone()) {
            JSHandle<JSTaggedValue> resultHandle = JSHandle<JSTaggedValue>::Cast(result);
            JSHandle<JSTaggedValue> closeVal = JSIterator::IteratorClose(thread, iterator, resultHandle);
            if (closeVal.GetTaggedValue().IsCompletionRecord()) {
                result = JSHandle<CompletionRecord>(closeVal);
                RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, promiseCapability);
                return result->GetValue();
            }
        }
        // b. IfAbruptRejectPromise(result, promiseCapability).
        RETURN_REJECT_PROMISE_IF_ABRUPT(thread, result, promiseCapability);
        return result->GetValue();
    }
    // 7.Return Completion(result).
    return result->GetValue();
}

JSHandle<CompletionRecord> BuiltinsPromise::PerformPromiseAllSettled(JSThread *thread,
                                                                     const JSHandle<PromiseIteratorRecord> &iterRecord,
                                                                     const JSHandle<JSTaggedValue> &constructor,
                                                                     const JSHandle<PromiseCapability> &resultCapa,
                                                                     const JSHandle<JSTaggedValue> &promiseResolve)
{
    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let values be a new empty List.
    JSHandle<PromiseRecord> values = factory->NewPromiseRecord();
    JSHandle<TaggedArray> emptyArray = factory->EmptyArray();
    values->SetValue(thread, emptyArray);
    // 2. Let remainingElementsCount be the Record { [[Value]]: 1 }.
    JSHandle<PromiseRecord> remainCnt = factory->NewPromiseRecord();
    remainCnt->SetValue(thread, JSTaggedNumber(1));
    // 3. Let index be 0.
    uint32_t index = 0;
    // 4. Repeat,
    JSHandle<JSTaggedValue> iter(thread, iterRecord->GetIterator());
    JSMutableHandle<JSTaggedValue> next(thread, globalConst->GetUndefined());
    JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
    while (true) {
        // a. Let next be IteratorStep(iteratorRecord).
        next.Update(JSIterator::IteratorStep(thread, iter).GetTaggedValue());
        // b. If next is an abrupt completion, set iteratorRecord.[[Done]] to true.
        if (thread->HasPendingException()) {
            iterRecord->SetDone(true);
            next.Update(JSPromise::IfThrowGetThrowValue(thread).GetTaggedValue());
        }
        // c. ReturnIfAbrupt(next).
        RETURN_COMPLETION_IF_ABRUPT(thread, next);
        // d. If next is false, then
        if (next->IsFalse()) {
            // i. Set iteratorRecord.[[Done]] to true.
            iterRecord->SetDone(true);
            // ii. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
            remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue()));
            // iii. If remainingElementsCount.[[Value]] is 0, then
            if (remainCnt->GetValue().IsZero()) {
                // 1. Let valuesArray be ! CreateArrayFromList(values).
                JSHandle<TaggedArray> taggedValues(thread, values->GetValue());
                JSHandle<JSArray> jsArrayValues = JSArray::CreateArrayFromList(thread, taggedValues);
                // 2. Perform ? Call(resultCapability.[[Resolve]], undefined, « valuesArray »).
                JSHandle<JSTaggedValue> resCapaFunc(thread, resultCapa->GetResolve());
                EcmaRuntimeCallInfo info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, resCapaFunc, undefined, undefined, 1);
                info.SetCallArg(jsArrayValues.GetTaggedValue());
                JSFunction::Call(&info);
                if (thread->HasPendingException()) {
                    JSHandle<JSTaggedValue> throwValue = JSPromise::IfThrowGetThrowValue(thread);
                    JSHandle<CompletionRecord> completionRecord =
                        factory->NewCompletionRecord(CompletionRecordType::THROW, throwValue);
                    return completionRecord;
                }
            }
            // iv. Return resultCapability.[[Promise]].
            JSHandle<JSTaggedValue> resultCapabilityHandle(thread, resultCapa->GetPromise());
            JSHandle<CompletionRecord> resRecord = factory->NewCompletionRecord(
                CompletionRecordType::NORMAL, resultCapabilityHandle);
            return resRecord;
        }
        // e. Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextVal = JSIterator::IteratorValue(thread, next);
        // f. If nextValue is an abrupt completion, set iteratorRecord.[[Done]] to true.
        if (thread->HasPendingException()) {
            iterRecord->SetDone(true);
            nextVal = JSHandle<JSTaggedValue>(thread, thread->GetException());
        }
        // g. ReturnIfAbrupt(nextValue).
        RETURN_COMPLETION_IF_ABRUPT(thread, nextVal);
        // h. Append undefined to values.
        JSHandle<JSTaggedValue> valuesHandle(thread, values->GetValue());
        JSHandle<TaggedArray> valuesArray = JSHandle<TaggedArray>::Cast(valuesHandle);
        valuesArray = TaggedArray::SetCapacity(thread, valuesArray, index + 1);
        valuesArray->Set(thread, index, JSTaggedValue::Undefined());
        values->SetValue(thread, valuesArray);
        // i. Let nextPromise be ? Call(promiseResolve, constructor, « nextValue »).
        EcmaRuntimeCallInfo taggedInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, promiseResolve, constructor, undefined, 1);
        taggedInfo.SetCallArg(nextVal.GetTaggedValue());
        JSTaggedValue taggedNextPromise = JSFunction::Call(&taggedInfo);
        JSHandle<JSTaggedValue> nextPromise(thread, taggedNextPromise);
        if (thread->HasPendingException()) {
            JSHandle<JSTaggedValue> promiseResult = JSPromise::IfThrowGetThrowValue(thread);
            JSHandle<CompletionRecord> completionRecord =
                factory->NewCompletionRecord(CompletionRecordType::THROW, promiseResult);
            return completionRecord;
        }
        // j. Let stepsFulfilled be the algorithm steps defined in Promise.allSettled Resolve Element Functions.
        // k. Let lengthFulfilled be the number of non-optional parameters of the function definition in
        //    Promise.allSettled Resolve Element Functions.
        // l. Let onFulfilled be CreateBuiltinFunction(stepsFulfilled, lengthFulfilled, "",
        //    « [[AlreadyCalled]], [[Index]], [[Values]], [[Capability]], [[RemainingElements]] »).
        JSHandle<JSPromiseAllSettledElementFunction> onFulfilled =
            factory->NewJSPromiseAllSettledResolveElementFunction();
        // m. Let alreadyCalled be the Record { [[Value]]: false }.
        JSHandle<PromiseRecord> alreadyCalled = factory->NewPromiseRecord();
        alreadyCalled->SetValue(thread, JSTaggedValue::False());
        // n. Set onFulfilled.[[AlreadyCalled]] to alreadyCalled.
        onFulfilled->SetAlreadyCalled(thread, alreadyCalled);
        // o. Set onFulfilled.[[Index]] to index.
        onFulfilled->SetIndex(index);
        // p. Set onFulfilled.[[Values]] to values.
        onFulfilled->SetValues(thread, values);
        // q. Set onFulfilled.[[Capability]] to resultCapability.
        onFulfilled->SetCapability(thread, resultCapa);
        // r. Set onFulfilled.[[RemainingElements]] to remainingElementsCount.
        onFulfilled->SetRemainingElements(thread, remainCnt);
        // s. Let stepsRejected be the algorithm steps defined in Promise.allSettled Reject Element Functions.
        // t. Let lengthRejected be the number of non-optional parameters of the function definition in
        //    Promise.allSettled Reject Element Functions.
        // u. Let onRejected be CreateBuiltinFunction(stepsRejected, lengthRejected, "",
        //    « [[AlreadyCalled]], [[Index]], [[Values]], [[Capability]], [[RemainingElements]] »).
        JSHandle<JSPromiseAllSettledElementFunction> onRejected =
            factory->NewJSPromiseAllSettledRejectElementFunction();
        // v. Set onRejected.[[AlreadyCalled]] to alreadyCalled.
        onRejected->SetAlreadyCalled(thread, alreadyCalled);
        // w. Set onRejected.[[Index]] to index.
        onRejected->SetIndex(index);
        // x. Set onRejected.[[Values]] to values.
        onRejected->SetValues(thread, values);
        // y. Set onRejected.[[Capability]] to resultCapability.
        onRejected->SetCapability(thread, resultCapa);
        // z. Set onRejected.[[RemainingElements]] to remainingElementsCount.
        onRejected->SetRemainingElements(thread, remainCnt);
        // aa. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] + 1.
        remainCnt->SetValue(thread, ++JSTaggedNumber(remainCnt->GetValue()));
        // ab. Perform ? Invoke(nextPromise, "then", « onFulfilled, onRejected »).
        JSHandle<JSTaggedValue> thenKey = globalConst->GetHandledPromiseThenString();
        EcmaRuntimeCallInfo invokeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, nextPromise, undefined, 2); // 2: two args
        invokeInfo.SetCallArg(onFulfilled.GetTaggedValue(), onRejected.GetTaggedValue());
        JSFunction::Invoke(&invokeInfo, thenKey);
        if (thread->HasPendingException()) {
            JSHandle<JSTaggedValue> taggedResult = JSPromise::IfThrowGetThrowValue(thread);
            JSHandle<CompletionRecord> completionRecord =
                factory->NewCompletionRecord(CompletionRecordType::THROW, taggedResult);
            return completionRecord;
        }
        // ac. Set index to index + 1.
        ++index;
    }
}

// 27.2.5.3 Promise.prototype.finally ( onFinally )
JSTaggedValue BuiltinsPromise::Finally(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Promise, Finally);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let promise be the this value.
    // 2. If Type(promise) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> promise = GetThis(argv);
    if (!promise->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reject: this value is not object", JSTaggedValue::Exception());
    }
    // 3. Let C be SpeciesConstructor(promise, %Promise%).
    // 4. Assert: IsConstructor(C) is true.
    JSHandle<JSTaggedValue> onFinally = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSObject> ctor = JSHandle<JSObject>::Cast(promise);
    JSHandle<JSTaggedValue> promiseFunc = JSHandle<JSTaggedValue>::Cast(env->GetPromiseFunction());
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, ctor, promiseFunc);
    ASSERT_PRINT(constructor->IsConstructor(), "constructor is not constructor");
    JSHandle<JSTaggedValue> thenFinally;
    JSHandle<JSTaggedValue> catchFinally;
    // 5. If IsCallable(onFinally) is false, then
    if (!onFinally->IsCallable()) {
        // a. Let thenFinally be onFinally.
        // b. Let catchFinally be onFinally.
        thenFinally = onFinally;
        catchFinally = onFinally;
    // 6. Else,
    } else {
        // a. Let stepsThenFinally be the algorithm steps defined in Then Finally Functions.
        // b. Let thenFinally be CreateBuiltinFunction(stepsThenFinally, « [[Constructor]], [[OnFinally]] »).
        JSHandle<JSPromiseFinallyFunction> thenFinallyFun =
            factory->NewJSPromiseThenFinallyFunction();
        // c. Set thenFinally.[[Constructor]] to C.
        // d. Set thenFinally.[[OnFinally]] to onFinally.
        thenFinallyFun->SetConstructor(thread, constructor);
        thenFinallyFun->SetOnFinally(thread, onFinally);
        thenFinally = JSHandle<JSTaggedValue>(thenFinallyFun);
        // e. Let stepsCatchFinally be the algorithm steps defined in Catch Finally Functions.
        // f. Let catchFinally be CreateBuiltinFunction(stepsCatchFinally, « [[Constructor]], [[OnFinally]] »).
        JSHandle<JSPromiseFinallyFunction> catchFinallyFun =
            factory->NewJSPromiseCatchFinallyFunction();
        // g. Set catchFinally.[[Constructor]] to C.
        // h. Set catchFinally.[[OnFinally]] to onFinally.
        catchFinallyFun->SetConstructor(thread, constructor);
        catchFinallyFun->SetOnFinally(thread, onFinally);
        catchFinally = JSHandle<JSTaggedValue>(catchFinallyFun);
    }
    // 7. return invoke(promise, "then", <<thenFinally, catchFinally>>)
    JSHandle<JSTaggedValue> thenKey(globalConst->GetHandledPromiseThenString());
    JSHandle<JSTaggedValue> undefined(globalConst->GetHandledUndefined());
    EcmaRuntimeCallInfo invokeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    invokeInfo.SetCallArg(thenFinally.GetTaggedValue(), catchFinally.GetTaggedValue());
    return JSFunction::Invoke(&invokeInfo, thenKey);
}
}  // namespace panda::ecmascript::builtins
