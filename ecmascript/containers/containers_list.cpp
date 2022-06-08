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

#include "containers_list.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_list.h"
#include "ecmascript/js_api_list_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_list.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersList::ListConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(obj);

    JSTaggedValue singleList = TaggedSingleList::Create(thread);
    list->SetSingleList(thread, singleList);

    return list.GetTaggedValue();
}

JSTaggedValue ContainersList::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    JSAPIList::Add(thread, jSAPIList, value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersList::Insert(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Insert);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    JSAPIList::Insert(thread, jSAPIList, value, index->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersList::GetFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetFirst);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    return jSAPIList->GetFirst();
}

JSTaggedValue ContainersList::GetLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetLast);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    return jSAPIList->GetLast();
}

JSTaggedValue ContainersList::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> element= GetCallArg(argv, 0);
    return GetTaggedBoolean(jSAPIList->Has(element.GetTaggedValue()));
}

JSTaggedValue ContainersList::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, IsEmpty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    return GetTaggedBoolean(jSAPIList->IsEmpty());
}

JSTaggedValue ContainersList::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }
    return jsAPIList->Get(index->GetInt());
}

JSTaggedValue ContainersList::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return jsAPIList->GetIndexOf(element.GetTaggedValue());
}

JSTaggedValue ContainersList::GetLastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetLastIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return jsAPIList->GetLastIndexOf(element.GetTaggedValue());
}

JSTaggedValue ContainersList::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Set);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 1);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSTaggedValue oldValue = JSAPIList::Set(thread, jsAPIList, index->GetInt(), element);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return oldValue;
}

JSTaggedValue ContainersList::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check List object
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }

    // get and check callback function
    JSHandle<JSTaggedValue> callbackFnHandle(GetCallArg(argv, 0));
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The first arg is not Callable", JSTaggedValue::Exception());
    }

    // If thisArgHandle was supplied, let T be thisArgHandle; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(thisHandle);
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    int length = list->Length();

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    int index = 0;
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    while (index < length) {
        JSTaggedValue value = singleList->Get(index);
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        info.SetCallArg(value, JSTaggedValue(index), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersList::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    jsAPIList->Clear(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersList::RemoveByIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, RemoveByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSTaggedValue result = JSAPIList::RemoveByIndex(thread, jsAPIList, index->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersList::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    return jsAPIList->Remove(thread, element.GetTaggedValue());
}

JSTaggedValue ContainersList::ReplaceAllElements(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, ReplaceAllElements);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    return JSAPIList::ReplaceAllElements(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersList::Equal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Equal);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsJSAPIList()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSAPIList> handleObj = JSHandle<JSAPIList>::Cast(obj);
    return jsAPIList->Equal(thread, handleObj);
}

JSTaggedValue ContainersList::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Sort);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not IsJSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }
    return JSAPIList::Sort(thread, self, callbackFnHandle);
}

JSTaggedValue ContainersList::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSAPIListIterator::CreateListIterator(thread, self);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersList::ConvertToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, ConvertToArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    return JSAPIList::ConvertToArray(thread, jsAPIList);
}

JSTaggedValue ContainersList::GetSubList(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetSubList);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> fromIndex= GetCallArg(argv, 0);

    if (!fromIndex->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> toIndex = GetCallArg(argv, 1);

    if (!toIndex->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSTaggedValue newList = JSAPIList::GetSubList(thread, jsAPIList, fromIndex->GetInt(), toIndex->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newList;
}

JSTaggedValue ContainersList::Length(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Length);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    return JSTaggedValue(jsAPIList->Length());
}
}  // namespace panda::ecmascript::containers
