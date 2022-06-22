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
 
#include "containers_lightweightset.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_api_lightweightset.h"
#include "ecmascript/js_api_lightweightset_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersLightWeightSet::LightWeightSetConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPILightWeightSet> lightweightSet = JSHandle<JSAPILightWeightSet>::Cast(obj);
    
    JSHandle<TaggedArray> hashes =
        JSAPILightWeightSet::CreateSlot(thread, JSAPILightWeightSet::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> values =
        JSAPILightWeightSet::CreateSlot(thread, JSAPILightWeightSet::DEFAULT_CAPACITY_LENGTH);
    lightweightSet->SetHashes(thread, hashes);
    lightweightSet->SetValues(thread, values);
    return obj.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightSet::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    bool flag = JSAPILightWeightSet::Add(thread, JSHandle<JSAPILightWeightSet>::Cast(self), value);
    return JSTaggedValue(flag);
}

JSTaggedValue ContainersLightWeightSet::AddAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, AddAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    return JSTaggedValue(JSAPILightWeightSet::AddAll(thread, JSHandle<JSAPILightWeightSet>::Cast(self), value));
}

JSTaggedValue ContainersLightWeightSet::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, IsEmpty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return JSTaggedValue(set->IsEmpty());
}

JSTaggedValue ContainersLightWeightSet::GetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, GetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Undefined());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Undefined());
    }
    int32_t index = value->GetInt();
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return set->GetValueAt(index);
}

JSTaggedValue ContainersLightWeightSet::HasAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, HasAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return JSTaggedValue(set->HasAll(value));
}

JSTaggedValue ContainersLightWeightSet::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return JSTaggedValue(set->Has(value));
}

JSTaggedValue ContainersLightWeightSet::HasHash(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, HasHash);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return JSTaggedValue(set->HasHash(value));
}

JSTaggedValue ContainersLightWeightSet::Equal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Equal);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    return JSTaggedValue(JSAPILightWeightSet::Equal(thread, JSHandle<JSAPILightWeightSet>::Cast(self), value));
}

JSTaggedValue ContainersLightWeightSet::IncreaseCapacityTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, IncreaseCapacityTo);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    int32_t minCapacity = value->GetInt();
    JSAPILightWeightSet::IncreaseCapacityTo(thread, JSHandle<JSAPILightWeightSet>::Cast(self), minCapacity);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLightWeightSet::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightSet::GetIteratorObj(thread, JSHandle<JSAPILightWeightSet>::Cast(self), IterationKind::VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightSet::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Values);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightSet::GetIteratorObj(thread, JSHandle<JSAPILightWeightSet>::Cast(self), IterationKind::VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightSet::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Entries);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightSet::GetIteratorObj(thread, JSHandle<JSAPILightWeightSet>::Cast(self),
                                            IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightSet::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    return JSAPILightWeightSet::ForEach(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersLightWeightSet::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, GetIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    int32_t result = set->GetIndexOf(value);
    return JSTaggedValue(result);
}

JSTaggedValue ContainersLightWeightSet::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return set->Remove(thread, key);
}

JSTaggedValue ContainersLightWeightSet::RemoveAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, RemoveAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsInteger()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }
    int32_t index = value->GetInt();
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    return JSTaggedValue(set->RemoveAt(thread, index));
}

JSTaggedValue ContainersLightWeightSet::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSAPILightWeightSet *set = JSAPILightWeightSet::Cast(self->GetTaggedObject());
    set->Clear(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLightWeightSet::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, ToString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSAPILightWeightSet::ToString(thread, JSHandle<JSAPILightWeightSet>::Cast(self));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return value;
}

JSTaggedValue ContainersLightWeightSet::ToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, ToArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILightWeightSet> lightweightset = JSHandle<JSAPILightWeightSet>::Cast(self);
    uint32_t length = lightweightset->GetLength();
    JSHandle<JSArray> array = thread->GetEcmaVM()->GetFactory()->NewJSArray();
    array->SetArrayLength(thread, length);
    JSHandle<TaggedArray> valueArray(thread, lightweightset->GetValues());
    uint32_t capacity = valueArray->GetLength();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(valueArray, capacity, capacity);
    array->SetElements(thread, newElements);
    return array.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightSet::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightSet, GetSize);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightSet", JSTaggedValue::Exception());
    }
    return JSTaggedValue(JSHandle<JSAPILightWeightSet>::Cast(self)->GetSize());
}
} // namespace panda::ecmascript::containers