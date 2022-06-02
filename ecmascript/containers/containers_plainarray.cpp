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
 
#include "containers_plainarray.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_api_plain_array.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersPlainArray::PlainArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPIPlainArray> plainArray = JSHandle<JSAPIPlainArray>::Cast(obj);
    JSHandle<TaggedArray> keys =
        JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> values =
        JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
    plainArray->SetKeys(thread, keys);
    plainArray->SetValues(thread, values);
    return obj.GetTaggedValue();
}

JSTaggedValue ContainersPlainArray::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));
    if (!key->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    JSAPIPlainArray::Add(thread, JSHandle<JSAPIPlainArray>::Cast(self), key, value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersPlainArray::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    array->Clear(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersPlainArray::Clone(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Clone);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIPlainArray> newPlainArray =
        JSAPIPlainArray::Clone(thread, JSHandle<JSAPIPlainArray>::Cast(self));
    return newPlainArray.GetTaggedValue();
}

JSTaggedValue ContainersPlainArray::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsNumber()) {
        return JSTaggedValue::False();
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t key = value->GetNumber();
    bool result = array->Has(key);
    return JSTaggedValue(result);
}

JSTaggedValue ContainersPlainArray::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    if (!key->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the key is not integer", JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->Get(key.GetTaggedValue());

    return value;
}

JSTaggedValue ContainersPlainArray::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter =
        JSAPIPlainArray::GetIteratorObj(thread, JSHandle<JSAPIPlainArray>::Cast(self), IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersPlainArray::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, ForEach);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    return JSAPIPlainArray::ForEach(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersPlainArray::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, ToString);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSAPIPlainArray::ToString(thread, JSHandle<JSAPIPlainArray>::Cast(self));
    return value;
}

JSTaggedValue ContainersPlainArray::GetIndexOfKey(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetIndexOfKey);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsNumber()) {
        return JSTaggedValue(-1);
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t key = value->GetNumber();
    JSTaggedValue result = array->GetIndexOfKey(key);
    return result;
}

JSTaggedValue ContainersPlainArray::GetIndexOfValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetIndexOfValue);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSTaggedValue jsValue = array->GetIndexOfValue(value.GetTaggedValue());
    return jsValue;
}

JSTaggedValue ContainersPlainArray::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, IsEmpty);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    bool ret = array->IsEmpty();
    return JSTaggedValue(ret);
}

JSTaggedValue ContainersPlainArray::GetKeyAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetKeyAt);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsNumber()) {
        return JSTaggedValue(-1);
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t index = value->GetNumber();
    JSTaggedValue result = array->GetKeyAt(index);
    return result;
}

JSTaggedValue ContainersPlainArray::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Remove);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    if (!key->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the key is not integer", JSTaggedValue::Undefined());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->Remove(thread, key.GetTaggedValue());
    return value;
}

JSTaggedValue ContainersPlainArray::RemoveAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, RemoveAt);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Undefined());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->RemoveAt(thread, index.GetTaggedValue());
    return value;
}

JSTaggedValue ContainersPlainArray::RemoveRangeFrom(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, RemoveRangeFrom);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> valueIndex(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> valueSize(GetCallArg(argv, 1));
    if (!valueIndex->IsInteger() || !valueSize->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index or the value is not integer", JSTaggedValue::Undefined());
    }
    int32_t index = valueIndex->GetNumber();
    int32_t size = valueSize->GetNumber();
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->RemoveRangeFrom(thread, index, size);
    return value;
}

JSTaggedValue ContainersPlainArray::SetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, SetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));
    if (!index->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    array->SetValueAt(thread, index.GetTaggedValue(), value.GetTaggedValue());
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersPlainArray::GetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> idx(GetCallArg(argv, 0));
    if (!idx->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Undefined());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t index = idx->GetNumber();
    JSTaggedValue value = array->GetValueAt(index);
    return value;
}

JSTaggedValue ContainersPlainArray::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetSize);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIPlainArray", JSTaggedValue::Exception());
    }
    int32_t length = JSHandle<JSAPIPlainArray>::Cast(self)->GetSize();
    return JSTaggedValue(length);
}
} // namespace panda::ecmascript::containers
