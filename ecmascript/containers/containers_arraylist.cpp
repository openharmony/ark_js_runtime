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

#include "containers_arraylist.h"
#include "ecmascript/base/array_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersArrayList::ArrayListConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    JSHandle<TaggedArray> newTaggedArray = factory->NewTaggedArray(JSAPIArrayList::DEFAULT_CAPACITY_LENGTH);
    obj->SetElements(thread, newTaggedArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    return GetTaggedBoolean(JSAPIArrayList::Add(thread, JSHandle<JSAPIArrayList>::Cast(self), value));
}

JSTaggedValue ContainersArrayList::Insert(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Insert);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not Integer", JSTaggedValue::Exception());
    }
    JSAPIArrayList::Insert(thread, JSHandle<JSAPIArrayList>::Cast(self), value, JSTaggedValue::ToUint32(thread, index));

    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersArrayList::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSAPIArrayList::Clear(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::Clone(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Clone);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIArrayList> newArrayList = JSAPIArrayList::Clone(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return newArrayList.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    bool isHas = JSHandle<JSAPIArrayList>::Cast(self)->Has(value.GetTaggedValue());

    return GetTaggedBoolean(isHas);
}

JSTaggedValue ContainersArrayList::GetCapacity(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);

    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetCapacity);
    JSThread *thread = argv->GetThread();
 
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    uint32_t capacity = JSAPIArrayList::GetCapacity(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return JSTaggedValue(capacity);
}

JSTaggedValue ContainersArrayList::IncreaseCapacityTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, IncreaseCapacityTo);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> newCapacity = GetCallArg(argv, 0);
    if (!newCapacity->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newCapacity is not Integer", JSTaggedValue::Exception());
    }

    JSAPIArrayList::IncreaseCapacityTo(thread, JSHandle<JSAPIArrayList>::Cast(self),
                                       JSTaggedValue::ToUint32(thread, newCapacity));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::TrimToCurrentLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, TrimToCurrentLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSAPIArrayList::TrimToCurrentLength(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    
    JSTaggedValue element = JSHandle<JSAPIArrayList>::Cast(self)->Get(thread, JSTaggedValue::ToUint32(thread, value));

    return element;
}

JSTaggedValue ContainersArrayList::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetIndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    return JSTaggedValue(JSAPIArrayList::GetIndexOf(thread, JSHandle<JSAPIArrayList>::Cast(self), value));
}

JSTaggedValue ContainersArrayList::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, IsEmpty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    return JSTaggedValue(JSAPIArrayList::IsEmpty(JSHandle<JSAPIArrayList>::Cast(self)));
}

JSTaggedValue ContainersArrayList::GetLastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetLastIndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    return JSTaggedValue(JSAPIArrayList::GetLastIndexOf(thread, JSHandle<JSAPIArrayList>::Cast(self), value));
}

JSTaggedValue ContainersArrayList::RemoveByIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, RemoveByIndex);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not Integer", JSTaggedValue::Exception());
    }

    JSAPIArrayList::RemoveByIndex(thread, JSHandle<JSAPIArrayList>::Cast(self), JSTaggedValue::ToUint32(thread, value));

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Remove);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    bool isRemove = JSAPIArrayList::Remove(thread, JSHandle<JSAPIArrayList>::Cast(self), value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return GetTaggedBoolean(isRemove);
}

JSTaggedValue ContainersArrayList::RemoveByRange(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, RemoveByRange);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 1);
    if (!startIndex->IsNumber() || !endIndex->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "startIndex or endIndex is not Integer", JSTaggedValue::Exception());
    }
    JSAPIArrayList::RemoveByRange(thread, JSHandle<JSAPIArrayList>::Cast(self), startIndex, endIndex);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::ReplaceAllElements(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, ReplaceAllElements);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    return JSAPIArrayList::ReplaceAllElements(thread, self, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersArrayList::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);
    JSHandle<JSAPIArrayList>::Cast(self)->Set(thread, JSTaggedValue::ToUint32(thread, index), value.GetTaggedValue());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersArrayList::SubArrayList(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, SubArrayList);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value1 = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value2 = GetCallArg(argv, 1);
    if (!value1->IsNumber() || !value2->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "startIndex or endIndex is not Integer", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIArrayList> newArrayList =
        JSAPIArrayList::SubArrayList(thread, JSHandle<JSAPIArrayList>::Cast(self), value1, value2);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArrayList.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Sort);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (callbackFnHandle->IsUndefined() || !callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }

    JSHandle<TaggedArray> elements(thread, JSHandle<JSAPIArrayList>::Cast(self)->GetElements());
    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    uint32_t length = JSHandle<JSAPIArrayList>::Cast(self)->GetLength().GetArrayLength();

    for (uint32_t i = 1; i < length; i++) {
        uint32_t beginIndex = 0;
        uint32_t endIndex = i;
        presentValue.Update(elements->Get(i));
        while (beginIndex < endIndex) {
            uint32_t middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(elements->Get(middleIndex));
            int32_t compareResult = base::ArrayHelper::SortCompare(thread, callbackFnHandle,
                                                                   middleValue, presentValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }

        if (endIndex >= 0 && endIndex < i) {
            for (uint32_t j = i; j > endIndex; j--) {
                previousValue.Update(elements->Get(j - 1));
                elements->Set(thread, j, previousValue.GetTaggedValue());
            }
            elements->Set(thread, endIndex, presentValue.GetTaggedValue());
        }
    }

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    
    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    return JSTaggedValue(JSHandle<JSAPIArrayList>::Cast(self)->GetSize());
}

JSTaggedValue ContainersArrayList::ConvertToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, ConvertToArray);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIArrayList> arrayList = JSHandle<JSAPIArrayList>::Cast(self);
    uint32_t length = arrayList->GetLength().GetArrayLength();
    JSHandle<JSArray> array = thread->GetEcmaVM()->GetFactory()->NewJSArray();
    array->SetArrayLength(thread, length);
    JSHandle<TaggedArray> arrayListElements(thread, arrayList->GetElements());

    uint32_t arrayListCapacity = arrayListElements->GetLength();

    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(arrayListElements, arrayListCapacity, arrayListCapacity);
    array->SetElements(thread, newElements);
    return array.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    return JSAPIArrayList::ForEach(thread, self, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersArrayList::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetIteratorObj);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIArrayList", JSTaggedValue::Exception());
    }

    JSTaggedValue values = JSAPIArrayList::GetIteratorObj(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return values;
}
}  // namespace panda::ecmascript::containers
