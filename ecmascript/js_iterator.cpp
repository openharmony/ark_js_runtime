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

#include "js_iterator.h"
#include "ecma_macros.h"
#include "ecma_vm.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/internal_call_params.h"
#include "global_env.h"
#include "js_invoker.h"
#include "js_symbol.h"
#include "object_factory.h"

namespace panda::ecmascript {
JSTaggedValue JSIterator::IteratorCloseAndReturn(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                                 const JSHandle<JSTaggedValue> &status)
{
    ASSERT(thread->HasPendingException());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue exception = thread->GetException();
    JSHandle<JSTaggedValue> record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(CompletionRecordType::THROW,
        JSHandle<JSTaggedValue>(thread, exception)));
    JSHandle<JSTaggedValue> result = JSIterator::IteratorClose(thread, iter, record);
    if (result->IsCompletionRecord()) {
        return CompletionRecord::Cast(result->GetTaggedObject())->GetValue();
    }
    return result.GetTaggedValue();
}

JSHandle<JSTaggedValue> JSIterator::GetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    // 1.ReturnIfAbrupt(obj).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, obj);
    // 2.If method was not passed, then
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> func = JSObject::GetMethod(thread, obj, iteratorSymbol);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, obj);

    return GetIterator(thread, obj, func);
}

JSHandle<JSTaggedValue> JSIterator::GetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &method)
{
    // 1.ReturnIfAbrupt(obj).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, obj);
    // 3.Let iterator be Call(method,obj).
    JSTaggedValue ret = JSFunction::Call(thread, method, obj, 0, nullptr);
    JSHandle<JSTaggedValue> iter(thread, ret);
    // 4.ReturnIfAbrupt(iterator).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, iter);
    // 5.If Type(iterator) is not Object, throw a TypeError exception
    if (!iter->IsECMAObject()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "", undefinedHandle);
    }
    return iter;
}  // namespace panda::ecmascript
// 7.4.2
JSHandle<JSObject> JSIterator::IteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    // 1.If value was not passed, then Let result be Invoke(iterator, "next", «‍ »).
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledNextString());
    JSHandle<JSTaggedValue> next(JSObject::GetMethod(thread, iter, key));
    ASSERT(next->IsCallable());
    JSTaggedValue ret = JSFunction::Call(thread, next, iter, 0, nullptr);
    JSHandle<JSObject> result(thread, ret);
    // 3.ReturnIfAbrupt(result)
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result);
    // 4.If Type(result) is not Object, throw a TypeError exception.
    if (!result->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "", result);
    }
    return result;
}

JSHandle<JSObject> JSIterator::IteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                            const JSHandle<JSTaggedValue> &value)
{
    // 2.Let result be Invoke(iterator, "next", «‍value»).
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledNextString());
    JSHandle<JSTaggedValue> next(JSObject::GetMethod(thread, iter, key));
    ASSERT(next->IsCallable());
    InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(value);
    JSTaggedValue ret = JSFunction::Call(thread, next, iter, 1, arguments->GetArgv());

    JSHandle<JSObject> result(thread, ret);
    // 3.ReturnIfAbrupt(result)
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result);
    // 4.If Type(result) is not Object, throw a TypeError exception.
    if (!result->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "", result);
    }
    return result;
}
// 7.4.3
bool JSIterator::IteratorComplete(JSThread *thread, const JSHandle<JSTaggedValue> &iterResult)
{
    ASSERT_PRINT(iterResult->IsECMAObject(), "iterResult must be JSObject");
    // Return ToBoolean(Get(iterResult, "done")).
    JSHandle<JSTaggedValue> doneStr = thread->GlobalConstants()->GetHandledDoneString();
    JSHandle<JSTaggedValue> done = JSTaggedValue::GetProperty(thread, iterResult, doneStr).GetValue();
    return done->ToBoolean();
}
// 7.4.4
JSHandle<JSTaggedValue> JSIterator::IteratorValue(JSThread *thread, const JSHandle<JSTaggedValue> &iterResult)
{
    ASSERT_PRINT(iterResult->IsECMAObject(), "iterResult must be JSObject");
    // Return Get(iterResult, "value").
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, iterResult, valueStr).GetValue();
    return value;
}
// 7.4.5
JSHandle<JSTaggedValue> JSIterator::IteratorStep(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    // 1.Let result be IteratorNext(iterator).
    JSHandle<JSTaggedValue> result(IteratorNext(thread, iter));
    // 2.ReturnIfAbrupt(result).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result);
    // 3.Let done be IteratorComplete(result).
    bool done = IteratorComplete(thread, result);
    // 4.ReturnIfAbrupt(done).
    JSHandle<JSTaggedValue> doneHandle(thread, JSTaggedValue(done));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, doneHandle);
    // 5.If done is true, return false.
    if (done) {
        JSHandle<JSTaggedValue> falseHandle(thread, JSTaggedValue::False());
        return falseHandle;
    }
    return result;
}
// 7.4.6
JSHandle<JSTaggedValue> JSIterator::IteratorClose(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                                  const JSHandle<JSTaggedValue> &completion)
{
    // 1.Assert: Type(iterator) is Object.
    ASSERT_PRINT(iter->IsECMAObject(), "iter must be JSObject");
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> exceptionOnThread;
    if (thread->HasPendingException()) {
        exceptionOnThread = JSHandle<JSTaggedValue>(thread, thread->GetException());
        thread->ClearException();
    }
    JSHandle<JSTaggedValue> returnStr(globalConst->GetHandledReturnString());
    // 3.Let return be GetMethod(iterator, "return").
    JSHandle<JSTaggedValue> returnFunc(JSObject::GetMethod(thread, iter, returnStr));
    // 4.ReturnIfAbrupt(return).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, returnFunc);
    // 5.If return is undefined, return Completion(completion).
    if (returnFunc->IsUndefined()) {
        if (!exceptionOnThread.IsEmpty()) {
            thread->SetException(exceptionOnThread.GetTaggedValue());
        }
        return completion;
    }
    // 6.Let innerResult be Call(return, iterator, «‍ »).
    JSTaggedValue ret = JSFunction::Call(thread, returnFunc, iter, 0, nullptr);
    if (!exceptionOnThread.IsEmpty()) {
        thread->SetException(exceptionOnThread.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> innerResult(thread, ret);
    JSHandle<CompletionRecord> completionRecord(thread, globalConst->GetUndefined());
    if (completion->IsCompletionRecord()) {
        completionRecord = JSHandle<CompletionRecord>::Cast(completion);
    }
    // 7.If completion.[[type]] is throw, return Completion(completion).
    if (!completionRecord.GetTaggedValue().IsUndefined() && completionRecord->IsThrow()) {
        if (!exceptionOnThread.IsEmpty()) {
            thread->SetException(exceptionOnThread.GetTaggedValue());
        }
        return completion;
    }
    // 8.If innerResult.[[type]] is throw, return Completion(innerResult).
    if (thread->HasPendingException()) {
        return innerResult;
    }
    // 9.If Type(innerResult.[[value]]) is not Object, throw a TypeError exception.
    if (!innerResult->IsECMAObject()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "", undefinedHandle);
    }
    if (!exceptionOnThread.IsEmpty()) {
        thread->SetException(exceptionOnThread.GetTaggedValue());
    }
    return completion;
}
// 7.4.7
JSHandle<JSObject> JSIterator::CreateIterResultObject(JSThread *thread, const JSHandle<JSTaggedValue> &value, bool done)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> constructor(env->GetObjectFunction());
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> doneStr = globalConst->GetHandledDoneString();
    JSHandle<JSTaggedValue> doneValue(thread, JSTaggedValue(done));
    // 2. Let obj be OrdinaryObjectCreate(%Object.prototype%).
    JSHandle<JSObject> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    // 3. Perform ! CreateDataPropertyOrThrow(obj, "value", value).
    // 4. Perform ! CreateDataPropertyOrThrow(obj, "done", done).
    JSObject::CreateDataPropertyOrThrow(thread, obj, valueStr, value);
    JSObject::CreateDataPropertyOrThrow(thread, obj, doneStr, doneValue);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    // 5. Return obj.
    return obj;
}
}  // namespace panda::ecmascript
