/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "containers_queue.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_api_queue.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersQueue::QueueConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Queue, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> newTaggedArray = factory->NewTaggedArray(JSAPIQueue::DEFAULT_CAPACITY_LENGTH);
    obj->SetElements(thread, newTaggedArray);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersQueue::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Queue, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIQueue()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIQueue", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSAPIQueue::Add(thread, JSHandle<JSAPIQueue>::Cast(self), value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersQueue::GetFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Queue, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIQueue()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIQueue", JSTaggedValue::Exception());
    }

    JSTaggedValue value = JSAPIQueue::GetFirst(thread, JSHandle<JSAPIQueue>::Cast(self));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
    return value;
}

JSTaggedValue ContainersQueue::Pop(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Queue, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIQueue()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIQueue", JSTaggedValue::Exception());
    }

    JSTaggedValue value = JSAPIQueue::Pop(thread, JSHandle<JSAPIQueue>::Cast(self));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
    return value;
}

JSTaggedValue ContainersQueue::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv); // JSAPIQueue
    if (!thisHandle->IsJSAPIQueue()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIQueue", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIQueue> queue = JSHandle<JSAPIQueue>::Cast(thisHandle);
    // Let len be ToLength(Get(O, "length")).
    double len = JSAPIQueue::GetArrayLength(thread, queue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    // If IsCallable(callbackfn) is false, throw a TypeError exception.
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    InternalCallParams *arguments = thread->GetInternalCallParams();

    uint32_t index = queue->GetCurrentFront();
    uint32_t k = 0;
    while (k < len) {
        JSHandle<JSTaggedValue> kValue =
            JSHandle<JSTaggedValue>(thread, queue->Get(thread, index));
        index = queue->GetNextPosition(index);
        key.Update(JSTaggedValue(k));
        arguments->MakeArgv(kValue, key, thisHandle);
        JSTaggedValue funcResult =
            JSFunction::Call(thread, callbackFnHandle, thisArgHandle, 3, arguments->GetArgv()); // 3: three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        k++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersQueue::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Queue, GetIteratorObj);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIQueue()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIQueue", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIQueueIterator> iter(factory->NewJSAPIQueueIterator(JSHandle<JSAPIQueue>::Cast(self)));
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersQueue::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Queue, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIQueue()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIQueue", JSTaggedValue::Exception());
    }

    uint32_t length = JSHandle<JSAPIQueue>::Cast(self)->GetSize();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue(length);
}
} // namespace panda::ecmascript::containers
