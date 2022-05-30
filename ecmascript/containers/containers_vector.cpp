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

#include "containers_vector.h"

#include "ecmascript/base/array_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_api_vector.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersVector::VectorConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSAPIVector> obj =
        JSHandle<JSAPIVector>(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    JSHandle<TaggedArray> newTaggedArray = factory->NewTaggedArray(JSAPIVector::DEFAULT_CAPACITY_LENGTH);
    obj->SetElements(thread, newTaggedArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersVector::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPIVector::Add(thread, JSHandle<JSAPIVector>::Cast(self), value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue::True();
}

JSTaggedValue ContainersVector::Insert(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Insert);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
 
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in index needs to be number", JSTaggedValue::Exception());
    }
    int32_t indexInt = JSTaggedValue::ToInt32(thread, index);
    JSAPIVector::Insert(thread, JSHandle<JSAPIVector>::Cast(self), value, indexInt);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersVector::SetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, SetLength);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> newSize = GetCallArg(argv, 0);
    if (!newSize->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in parameter needs to be number", JSTaggedValue::Exception());
    }
    if (newSize->GetNumber() < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "An incorrect size was set",  JSTaggedValue::Exception());
    }
    JSAPIVector::SetLength(thread, JSHandle<JSAPIVector>::Cast(self), JSTaggedValue::ToUint32(thread, newSize));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersVector::GetCapacity(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetCapacity);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    uint32_t capacity = JSHandle<JSAPIVector>::Cast(self)->GetCapacity();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue(capacity);
}

JSTaggedValue ContainersVector::IncreaseCapacityTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, IncreaseCapacityTo);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> newCapacity = GetCallArg(argv, 0);
    if (!newCapacity->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in parameter needs to be number", JSTaggedValue::Exception());
    }
    JSAPIVector::IncreaseCapacityTo(thread,
                                    JSHandle<JSAPIVector>::Cast(self),
                                    JSTaggedValue::ToInt32(thread, newCapacity));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersVector::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in index needs to be number", JSTaggedValue::Exception());
    }
    int32_t indexInt = JSTaggedValue::ToInt32(thread, index);
    JSTaggedValue value = JSAPIVector::Get(thread, JSHandle<JSAPIVector>::Cast(self), indexInt);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    
    return value;
}

JSTaggedValue ContainersVector::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    int index = JSAPIVector::GetIndexOf(thread, JSHandle<JSAPIVector>::Cast(self), element);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue(index);
}

JSTaggedValue ContainersVector::GetIndexFrom(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetIndexFrom);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in index needs to be number", JSTaggedValue::Exception());
    }
    int32_t indexInt = JSTaggedValue::ToInt32(thread, index);
    int indexOut = JSAPIVector::GetIndexFrom(thread, JSHandle<JSAPIVector>::Cast(self),
                                             element, indexInt);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue(indexOut);
}

JSTaggedValue ContainersVector::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, IsEmpty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    bool ret = JSHandle<JSAPIVector>::Cast(self)->IsEmpty();

    return GetTaggedBoolean(ret);
}

JSTaggedValue ContainersVector::GetLastElement(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetLastElement);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSHandle<JSAPIVector>::Cast(self)->GetLastElement();

    return value;
}

JSTaggedValue ContainersVector::GetLastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetLastIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    int index = JSAPIVector::GetLastIndexOf(thread, JSHandle<JSAPIVector>::Cast(self), element);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue(index);
}

JSTaggedValue ContainersVector::GetLastIndexFrom(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetLastIndexFrom);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in index needs to be number", JSTaggedValue::Exception());
    }
    int32_t indexInt = JSTaggedValue::ToInt32(thread, index);
    int indexOut = JSAPIVector::GetLastIndexFrom(thread, JSHandle<JSAPIVector>::Cast(self),
                                                 element, indexInt);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(indexOut);
}

JSTaggedValue ContainersVector::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    bool ret = JSAPIVector::Remove(thread, JSHandle<JSAPIVector>::Cast(self), element);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return GetTaggedBoolean(ret);
}

JSTaggedValue ContainersVector::RemoveByIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, RemoveByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in index needs to be number", JSTaggedValue::Exception());
    }
    JSTaggedValue value =
        JSAPIVector::RemoveByIndex(thread, JSHandle<JSAPIVector>::Cast(self), JSTaggedValue::ToInt32(thread, index));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return value;
}

JSTaggedValue ContainersVector::RemoveByRange(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, RemoveByRange);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> fromIndex = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> toIndex = GetCallArg(argv, 1);
    if (!fromIndex->IsNumber() || !toIndex->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in parameter needs to be number", JSTaggedValue::Exception());
    }

    JSAPIVector::RemoveByRange(thread, JSHandle<JSAPIVector>::Cast(self),
                               JSTaggedValue::ToInt32(thread, fromIndex),
                               JSTaggedValue::ToInt32(thread, toIndex));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersVector::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Set);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 1);
    if (!index->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in index needs to be number", JSTaggedValue::Exception());
    }

    int32_t indexInt = JSTaggedValue::ToInt32(thread, index);
    int32_t len = static_cast<int>(JSHandle<JSAPIVector>::Cast(self)->GetSize());
    if (indexInt < 0 || indexInt >= len) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSHandle<JSAPIVector>::Cast(self)->Set(thread,
                                                                 indexInt,
                                                                 element.GetTaggedValue());
    return value;
}

JSTaggedValue ContainersVector::SubVector(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, SubVector);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> fromIndex = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> toIndex = GetCallArg(argv, 1);
    if (!fromIndex->IsNumber() || !toIndex->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The passed in parameter needs to be number", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIVector> subVector = JSAPIVector::SubVector(thread, JSHandle<JSAPIVector>::Cast(self),
                                                             JSTaggedValue::ToInt32(thread, fromIndex),
                                                             JSTaggedValue::ToInt32(thread, toIndex));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return subVector.GetTaggedValue();
}

JSTaggedValue ContainersVector::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, ToString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSTaggedValue value = JSAPIVector::ToString(thread, JSHandle<JSAPIVector>::Cast(self));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return value;
}

JSTaggedValue ContainersVector::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetSize);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    int32_t length = JSHandle<JSAPIVector>::Cast(self)->GetSize();
    return JSTaggedValue(length);
}

JSTaggedValue ContainersVector::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv); // JSAPIVector

    // If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    return JSAPIVector::ForEach(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersVector::ReplaceAllElements(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, ReplaceAllElements);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv); // JSAPIVector

    // If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    return JSAPIVector::ReplaceAllElements(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersVector::TrimToCurrentLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, TrimToCurrentLength);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSAPIVector::TrimToCurrentLength(thread, JSHandle<JSAPIVector>::Cast(self));
    return JSTaggedValue::True();
}

JSTaggedValue ContainersVector::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSAPIVector::Clear(JSHandle<JSAPIVector>::Cast(self));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersVector::Clone(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Clone);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIVector> newVector = JSAPIVector::Clone(thread, JSHandle<JSAPIVector>::Cast(self));
    return newVector.GetTaggedValue();
}

JSTaggedValue ContainersVector::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    bool isHas = JSHandle<JSAPIVector>::Cast(self)->Has(value.GetTaggedValue());
    return GetTaggedBoolean(isHas);
}

JSTaggedValue ContainersVector::CopyToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, CopyToArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> array(GetCallArg(argv, 0));
    if (!array->IsJSArray()) {
        return JSTaggedValue::False();
    }

    JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(self);
    JSHandle<TaggedArray> vectorElements(thread, vector->GetElements());
    JSHandle<TaggedArray> arrayElements = JSArray::ToTaggedArray(thread, array);
    JSHandle<TaggedArray> resultArray = TaggedArray::Append(thread, arrayElements, vectorElements);

    JSHandle<JSArray> jsArray = JSHandle<JSArray>::Cast(array);
    uint32_t sumLength = static_cast<uint32_t>(vector->GetSize()) + jsArray->GetArrayLength();
    jsArray->SetArrayLength(thread, sumLength);
    jsArray->SetElements(thread, resultArray);
    
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersVector::ConvertToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, ConvertToArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(self);
    int32_t length = vector->GetSize();
    JSHandle<JSArray> array = thread->GetEcmaVM()->GetFactory()->NewJSArray();
    array->SetArrayLength(thread, length);
    JSHandle<TaggedArray> vectorElements(thread, vector->GetElements());
    uint32_t vectorCapacity = vectorElements->GetLength();

    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(vectorElements, vectorCapacity, vectorCapacity);

    array->SetElements(thread, newElements);
    return array.GetTaggedValue();
}

JSTaggedValue ContainersVector::GetFirstElement(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetFirstElement);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSTaggedValue firstElement = JSAPIVector::GetFirstElement(JSHandle<JSAPIVector>::Cast(self));
    return firstElement;
}

JSTaggedValue ContainersVector::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, Sort);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);

    JSHandle<TaggedArray> elements(thread, JSHandle<JSAPIVector>::Cast(self)->GetElements());
    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    uint32_t length = static_cast<uint32_t>(JSHandle<JSAPIVector>::Cast(self)->GetSize());

    for (uint32_t i = 1; i < length; i++) {
        uint32_t beginIndex = 0;
        uint32_t endIndex = i;
        presentValue.Update(elements->Get(i));
        while (beginIndex < endIndex) {
            uint32_t middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(elements->Get(middleIndex));
            int32_t compareResult = base::ArrayHelper::SortCompare(thread, callbackFnHandle, middleValue, presentValue);
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

JSTaggedValue ContainersVector::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Vector, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIVector()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIVector", JSTaggedValue::Exception());
    }

    JSTaggedValue values = JSAPIVector::GetIteratorObj(thread, JSHandle<JSAPIVector>::Cast(self));
    return values;
}
} // namespace panda::ecmascript::containers
