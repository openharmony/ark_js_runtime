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
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/assert_scope-inl.h"
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
    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(2);  // 2: 2 means two args stored in array
    auto result = resolvingFunction->GetResolveFunction();
    arguments->Set(thread, 0, result);
    result = resolvingFunction->GetRejectFunction();
    arguments->Set(thread, 1, result);

    JSHandle<JSTaggedValue> thisValue = globalConst->GetHandledUndefined();
    JSTaggedValue taggedValue = JSFunction::Call(thread, executor, thisValue, arguments);
    JSHandle<JSTaggedValue> completionValue(thread, taggedValue);

    // 10. If completion is an abrupt completion, then
    // a. Let status be Call(resolvingFunctions.[[Reject]], undefined, «completion.[[value]]»).
    // b. ReturnIfAbrupt(status).
    if (thread->HasPendingException()) {
        completionValue = JSPromise::IfThrowGetThrowValue(thread);
        thread->ClearException();
        array_size_t length = 1;
        JSHandle<TaggedArray> arrayCompletion = factory->NewTaggedArray(length);
        arrayCompletion->Set(thread, 0, completionValue);
        JSHandle<JSTaggedValue> reject(thread, resolvingFunction->GetRejectFunction());
        JSFunction::Call(thread, reject, thisValue, arrayCompletion);
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
    JSHandle<JSTaggedValue> done(thread, JSTaggedValue::False());
    JSHandle<PromiseIteratorRecord> itRecord = factory->NewPromiseIteratorRecord(itor, done);
    // 11. Let result be PerformPromiseAll(iteratorRecord, C, promiseCapability).
    JSHandle<CompletionRecord> result = PerformPromiseAll(thread, itRecord, ctor, capa);
    // 12. If result is an abrupt completion,
    if (result->IsThrow()) {
        thread->ClearException();
        // a. If iteratorRecord.[[done]] is false, let result be IteratorClose(iterator, result).
        // b. IfAbruptRejectPromise(result, promiseCapability).
        if (itRecord->GetDone().IsFalse()) {
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
    JSHandle<JSTaggedValue> done(thread, JSTaggedValue::False());
    JSHandle<PromiseIteratorRecord> iteratorRecord = factory->NewPromiseIteratorRecord(iterator, done);

    // 11. Let result be PerformPromiseRace(iteratorRecord, promiseCapability, C).
    // 12. If result is an abrupt completion, then
    //     a. If iteratorRecord.[[done]] is false, let result be IteratorClose(iterator,result).
    //     b. IfAbruptRejectPromise(result, promiseCapability).
    // 13. Return Completion(result).
    JSHandle<CompletionRecord> result = PerformPromiseRace(thread, iteratorRecord, promiseCapability, thisValue);
    if (result->IsThrow()) {
        thread->ClearException();
        if (iteratorRecord->GetDone().IsFalse()) {
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

    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
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
    array_size_t length = 1;
    JSHandle<TaggedArray> arrayCompletion = factory->NewTaggedArray(length);
    arrayCompletion->Set(thread, 0, xValue);
    JSHandle<JSTaggedValue> resolve(thread, promiseCapability->GetResolve());
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    JSFunction::Call(thread, resolve, undefined, arrayCompletion);
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
    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();

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
    array_size_t length = 1;
    JSHandle<TaggedArray> arrayCompletion = factory->NewTaggedArray(length);
    arrayCompletion->Set(thread, 0, reason);
    JSHandle<JSTaggedValue> reject(thread, promiseCapability->GetReject());
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    JSFunction::Call(thread, reject, undefined, arrayCompletion);
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
    auto ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> promise = GetThis(argv);
    JSHandle<JSTaggedValue> thenKey = globalConst->GetHandledPromiseThenString();
    JSHandle<JSTaggedValue> reject = GetCallArg(argv, 0);
    array_size_t length = 2;
    JSHandle<TaggedArray> arrayList = factory->NewTaggedArray(length);
    arrayList->Set(thread, 0, globalConst->GetHandledUndefined());
    arrayList->Set(thread, 1, reject);
    return JSFunction::Invoke(thread, promise, thenKey, arrayList);
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

    if (JSTaggedValue::SameValue(promise->GetPromiseState(),
                                 JSTaggedValue(static_cast<int32_t>(PromiseStatus::PENDING)))) {
        JSHandle<TaggedQueue> fulfillReactions(thread, promise->GetPromiseFulfillReactions());
        auto result =
            JSTaggedValue(TaggedQueue::Push(thread, fulfillReactions, JSHandle<JSTaggedValue>::Cast(fulfillReaction)));
        promise->SetPromiseFulfillReactions(thread, result);

        JSHandle<TaggedQueue> rejectReactions(thread, promise->GetPromiseRejectReactions());
        result =
            JSTaggedValue(TaggedQueue::Push(thread, rejectReactions, JSHandle<JSTaggedValue>::Cast(rejectReaction)));
        promise->SetPromiseRejectReactions(thread, result);
    } else if (JSTaggedValue::SameValue(promise->GetPromiseState(),
                                        JSTaggedValue(static_cast<int32_t>(PromiseStatus::FULFILLED)))) {
        JSHandle<TaggedArray> argv = factory->NewTaggedArray(2);  // 2: 2 means two args stored in array
        argv->Set(thread, 0, fulfillReaction.GetTaggedValue());
        argv->Set(thread, 1, promise->GetPromiseResult());

        JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
        job->EnqueueJob(thread, job::QueueType::QUEUE_PROMISE, promiseReactionsJob, argv);
    } else if (JSTaggedValue::SameValue(promise->GetPromiseState(),
                                        JSTaggedValue(static_cast<int32_t>(PromiseStatus::REJECTED)))) {
        JSHandle<TaggedArray> argv = factory->NewTaggedArray(2);  // 2: 2 means two args stored in array
        argv->Set(thread, 0, rejectReaction.GetTaggedValue());
        argv->Set(thread, 1, promise->GetPromiseResult());

        JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
        job->EnqueueJob(thread, job::QueueType::QUEUE_PROMISE, promiseReactionsJob, argv);
    }
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
    array_size_t index = 0;
    // 6. Repeat
    JSHandle<JSTaggedValue> itor(thread, itRecord->GetIterator());
    JSMutableHandle<JSTaggedValue> next(thread, globalConst->GetUndefined());
    while (true) {
        // a. Let next be IteratorStep(iteratorRecord.[[iterator]]).
        next.Update(JSIterator::IteratorStep(thread, itor).GetTaggedValue());
        // b. If next is an abrupt completion, set iteratorRecord.[[done]] to true.
        if (thread->HasPendingException()) {
            itRecord->SetDone(thread, JSTaggedValue::True());
            next.Update(JSPromise::IfThrowGetThrowValue(thread).GetTaggedValue());
        }
        // c. ReturnIfAbrupt(next).
        RETURN_COMPLETION_IF_ABRUPT(thread, next);
        // d. If next is false,
        if (next->IsFalse()) {
            // i. Set iteratorRecord.[[done]] to true.
            itRecord->SetDone(thread, JSTaggedValue::True());
            // ii. Set remainingElementsCount.[[value]] to remainingElementsCount.[[value]] − 1.
            remainCnt->SetValue(thread, --JSTaggedNumber(remainCnt->GetValue()));
            // iii. If remainingElementsCount.[[value]] is 0,
            if (remainCnt->GetValue().IsZero()) {
                // 1. Let valuesArray be CreateArrayFromList(values).
                JSHandle<JSArray> jsArrayValues =
                    JSArray::CreateArrayFromList(thread, JSHandle<TaggedArray>(thread, values->GetValue()));
                // 2. Let resolveResult be Call(resultCapability.[[Resolve]], undefined, «valuesArray»).
                JSHandle<JSTaggedValue> resCapaFunc(thread, capa->GetResolve());
                JSHandle<TaggedArray> argv = factory->NewTaggedArray(1);
                argv->Set(thread, 0, jsArrayValues.GetTaggedValue());
                JSTaggedValue resolveRes =
                    JSFunction::Call(thread, resCapaFunc, globalConst->GetHandledUndefined(), argv);
                // 3. ReturnIfAbrupt(resolveResult)
                JSHandle<JSTaggedValue> resolveAbrupt(thread, resolveRes);
                RETURN_COMPLETION_IF_ABRUPT(thread, resolveAbrupt);
            }
            // iv. Return resultCapability.[[Promise]].
            JSHandle<CompletionRecord> resRecord = factory->NewCompletionRecord(
                CompletionRecord::NORMAL,
                JSHandle<JSTaggedValue>(thread, capa->GetPromise()));
            return resRecord;
        }
        // e. Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextVal = JSIterator::IteratorValue(thread, next);
        // f. If nextValue is an abrupt completion, set iteratorRecord.[[done]] to true.
        if (thread->HasPendingException()) {
            itRecord->SetDone(thread, JSTaggedValue::True());
            if (thread->GetException().IsObjectWrapper()) {
                JSHandle<ObjectWrapper> wrapperVal(thread, thread->GetException());
                JSHandle<JSTaggedValue> throwVal(thread, wrapperVal->GetValue());
                nextVal = throwVal;
            } else {
                nextVal = JSHandle<JSTaggedValue>(thread, thread->GetException());
            }
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
        JSHandle<TaggedArray> nextValueArray = factory->NewTaggedArray(1);
        nextValueArray->Set(thread, 0, nextVal);
        JSTaggedValue taggedNextPromise = JSFunction::Invoke(thread, ctor, resolveKey, nextValueArray);
        // j. ReturnIfAbrupt(nextPromise).
        JSHandle<JSTaggedValue> nextPromise(thread, taggedNextPromise);
        RETURN_COMPLETION_IF_ABRUPT(thread, nextPromise);
        // k. Let resolveElement be a new built-in function object as defined in Promise.all
        //    Resolve Element Functions.
        JSHandle<JSPromiseAllResolveElementFunction> resoleveElement = factory->NewJSPromiseAllResolveElementFunction(
            reinterpret_cast<void *>(BuiltinsPromiseHandler::ResolveElementFunction));
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
        JSHandle<TaggedArray> arg = factory->NewTaggedArray(2);  // 2: 2 means two args stored in array
        arg->Set(thread, 0, resoleveElement);
        arg->Set(thread, 1, capa->GetReject());
        JSTaggedValue taggedResult = JSFunction::Invoke(thread, nextPromise, thenKey, arg);
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
    while (true) {
        next.Update(JSIterator::IteratorStep(thread, iterator).GetTaggedValue());
        if (thread->HasPendingException()) {
            iteratorRecord->SetDone(thread, JSTaggedValue::True());
            next.Update(JSPromise::IfThrowGetThrowValue(thread).GetTaggedValue());
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, next);
        if (next->IsFalse()) {
            iteratorRecord->SetDone(thread, JSTaggedValue::True());
            JSHandle<JSTaggedValue> promise(thread, capability->GetPromise());
            JSHandle<CompletionRecord> completionRecord =
                factory->NewCompletionRecord(CompletionRecord::NORMAL, promise);
            return completionRecord;
        }
        JSHandle<JSTaggedValue> nextValue = JSIterator::IteratorValue(thread, next);
        if (thread->HasPendingException()) {
            iteratorRecord->SetDone(thread, JSTaggedValue::True());
            nextValue = JSPromise::IfThrowGetThrowValue(thread);
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, nextValue);
        JSHandle<JSTaggedValue> resolveStr = globalConst->GetHandledPromiseResolveString();
        array_size_t length = 1;
        JSHandle<TaggedArray> array = factory->NewTaggedArray(length);
        array->Set(thread, 0, nextValue);
        JSTaggedValue result = JSFunction::Invoke(thread, constructor, resolveStr, array);
        JSHandle<JSTaggedValue> nextPromise(thread, result);
        if (thread->HasPendingException()) {
            nextPromise = JSPromise::IfThrowGetThrowValue(thread);
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, nextPromise);

        JSHandle<JSTaggedValue> thenStr = globalConst->GetHandledPromiseThenString();
        length = 2;  // 2: 2 means two args stored in array
        array = factory->NewTaggedArray(length);
        array->Set(thread, 0, capability->GetResolve());
        array->Set(thread, 1, capability->GetReject());
        result = JSFunction::Invoke(thread, nextPromise, thenStr, array);
        JSHandle<JSTaggedValue> handleResult(thread, result);
        if (thread->HasPendingException()) {
            handleResult = JSPromise::IfThrowGetThrowValue(thread);
        }
        RETURN_COMPLETION_IF_ABRUPT(thread, handleResult);
    }
}
}  // namespace panda::ecmascript::builtins
