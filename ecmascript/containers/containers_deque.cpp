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

#include "containers_deque.h"
#include "ecmascript/base/array_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_deque.h"
#include "ecmascript/js_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersDeque::DequeConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    obj->SetElements(thread, newElements);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersDeque::InsertFront(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, InsertFront);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSAPIDeque::InsertFront(thread, JSHandle<JSAPIDeque>::Cast(self), value);

    return JSTaggedValue::True();
}


JSTaggedValue ContainersDeque::InsertEnd(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, InsertEnd);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPIDeque::InsertEnd(thread, JSHandle<JSAPIDeque>::Cast(self), value);

    return JSTaggedValue::True();
}

JSTaggedValue ContainersDeque::GetFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetFirst);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue firstElement = deque->GetFront();
    return firstElement;
}

JSTaggedValue ContainersDeque::GetLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetLast);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue lastElement = deque->GetTail();
    return lastElement;
}

JSTaggedValue ContainersDeque::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    bool isHas = deque->Has(value.GetTaggedValue());
    return GetTaggedBoolean(isHas);
}

JSTaggedValue ContainersDeque::PopFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, PopFirst);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue firstElement = deque->PopFirst();
    return firstElement;
}

JSTaggedValue ContainersDeque::PopLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, PopLast);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue lastElement = deque->PopLast();
    return lastElement;
}

JSTaggedValue ContainersDeque::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(GetThis(argv));
    if (!thisHandle->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not IsJSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();

    JSHandle<TaggedArray> elements(thread, deque->GetElements());
    uint32_t capacity = elements->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t index = 0;
    while (first != last) {
        JSTaggedValue kValue = deque->Get(index);
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, 3); // 3:three args
        info.SetCallArg(kValue, JSTaggedValue(index), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        ASSERT(capacity != 0);
        first = (first + 1) % capacity;
        index = index + 1;
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersDeque::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetIteratorObj);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSTaggedValue values = JSAPIDeque::GetIteratorObj(thread, JSHandle<JSAPIDeque>::Cast(self));

    return values;
}

JSTaggedValue ContainersDeque::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetSize);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIDeque", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    uint32_t length = deque->GetSize();

    return JSTaggedValue(length);
}
} // namespace panda::ecmascript::containers
