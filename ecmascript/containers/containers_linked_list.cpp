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

#include "containers_linked_list.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_linked_list.h"
#include "ecmascript/js_api_linked_list_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_list.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersLinkedList::LinkedListConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPILinkedList> linkedList = JSHandle<JSAPILinkedList>::Cast(obj);
    JSTaggedValue doubleList = TaggedDoubleList::Create(thread);
    linkedList->SetDoubleList(thread, doubleList);
    return linkedList.GetTaggedValue();
}

JSTaggedValue ContainersLinkedList::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSAPILinkedList::Add(thread, jsAPILinkedList, value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLinkedList::AddFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, AddFirst);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSAPILinkedList::AddFirst(thread, jsAPILinkedList, value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLinkedList::GetFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, GetFirst);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    return jsAPILinkedList->GetFirst();
}

JSTaggedValue ContainersLinkedList::GetLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, GetLast);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    return jsAPILinkedList->GetLast();
}

JSTaggedValue ContainersLinkedList::Length(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Length);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    return JSTaggedValue(jsAPILinkedList->Length());
}

JSTaggedValue ContainersLinkedList::Insert(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Insert);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }

    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue result =
        JSAPILinkedList::Insert(thread, jsAPILinkedList, value, index->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersLinkedList::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    jsAPILinkedList->Clear(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLinkedList::Clone(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Clone);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSHandle<JSAPILinkedList> newLinkedList = JSAPILinkedList::Clone(thread, jsAPILinkedList);
    return newLinkedList.GetTaggedValue();
}

JSTaggedValue ContainersLinkedList::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return GetTaggedBoolean(jsAPILinkedList->Has(element.GetTaggedValue()));
}

JSTaggedValue ContainersLinkedList::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    return jsAPILinkedList->Get(index->GetInt());
}

JSTaggedValue ContainersLinkedList::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, GetIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return jsAPILinkedList->GetIndexOf(element.GetTaggedValue());
}

JSTaggedValue ContainersLinkedList::GetLastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, GetLastIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSHandle<JSTaggedValue> element(GetCallArg(argv, 0));
    return jsAPILinkedList->GetLastIndexOf(element.GetTaggedValue());
}

JSTaggedValue ContainersLinkedList::RemoveByIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, RemoveByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue nodeData =
        JSAPILinkedList::RemoveByIndex(thread, jsAPILinkedList, index->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return nodeData;
}

JSTaggedValue ContainersLinkedList::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    return jsAPILinkedList->Remove(thread, element.GetTaggedValue());
}

JSTaggedValue ContainersLinkedList::RemoveFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, RemoveFirst);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue lastValue = JSAPILinkedList::RemoveFirst(thread, jsAPILinkedList);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return lastValue;
}

JSTaggedValue ContainersLinkedList::RemoveFirstFound(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, RemoveFirstFound);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue result = JSAPILinkedList::RemoveFirstFound(thread, jsAPILinkedList, element.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersLinkedList::RemoveLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, RemoveLast);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue lastValue = JSAPILinkedList::RemoveLast(thread, jsAPILinkedList);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return lastValue;
}

JSTaggedValue ContainersLinkedList::RemoveLastFound(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, RemoveLastFound);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue result = JSAPILinkedList::RemoveLastFound(thread, jsAPILinkedList, element.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersLinkedList::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, Set);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 1);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    JSTaggedValue oldValue =
        JSAPILinkedList::Set(thread, jsAPILinkedList, index->GetInt(), element);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return oldValue;
}

JSTaggedValue ContainersLinkedList::ConvertToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, ConvertToArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(self);
    return JSAPILinkedList::ConvertToArray(thread, jsAPILinkedList);
}

JSTaggedValue ContainersLinkedList::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> callbackFnHandle(GetCallArg(argv, 0));
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The first arg is not Callable", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    JSHandle<JSAPILinkedList> linkedList = JSHandle<JSAPILinkedList>::Cast(thisHandle);
    JSHandle<TaggedDoubleList> doubleList(thread, linkedList->GetDoubleList());
    int length = linkedList->Length();

    int index = 0;
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    while (index < length) {
        JSTaggedValue value = doubleList->Get(index);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(value, JSTaggedValue(index), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLinkedList::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LinkedList, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSAPILinkedListIterator::CreateLinkedListIterator(thread, self);
    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript::containers
