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

#include "containers_lightweightmap.h"
#include "ecmascript/base/array_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_lightweightmap.h"
#include "ecmascript/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersLightWeightMap::LightWeightMapConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPILightWeightMap> lwMap = JSHandle<JSAPILightWeightMap>::Cast(obj);
    JSHandle<TaggedArray> hashArray = factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> valueArray = factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH);
    lwMap->SetHashes(thread, hashArray.GetTaggedValue());
    lwMap->SetKeys(thread, keyArray.GetTaggedValue());
    lwMap->SetValues(thread, valueArray.GetTaggedValue());

    return lwMap.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::Length(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Length);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    return JSTaggedValue(JSHandle<JSAPILightWeightMap>::Cast(self)->GetLength());
}

JSTaggedValue ContainersLightWeightMap::HasAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, HasAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> lightWeightMap(GetCallArg(argv, 0));
    if (!lightWeightMap->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "argv is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::HasAll(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                       JSHandle<JSAPILightWeightMap>::Cast(lightWeightMap));
}

JSTaggedValue ContainersLightWeightMap::HasKey(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, HasKey);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    return JSAPILightWeightMap::HasKey(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
}

JSTaggedValue ContainersLightWeightMap::HasValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, HasValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    return JSAPILightWeightMap::HasValue(thread, JSHandle<JSAPILightWeightMap>::Cast(self), value);
}

JSTaggedValue ContainersLightWeightMap::IncreaseCapacityTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, IncreaseCapacityTo);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));

    if (!index->IsInt()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the size is not integer", JSTaggedValue::Exception());
    }
    JSAPILightWeightMap::IncreaseCapacityTo(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                            index.GetTaggedValue().GetInt());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLightWeightMap::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Entries);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightMapIterator::CreateLightWeightMapIterator(thread, self, IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    return JSAPILightWeightMap::Get(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
}

JSTaggedValue ContainersLightWeightMap::GetIndexOfKey(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetIndexOfKey);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    int32_t index = JSAPILightWeightMap::GetIndexOfKey(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
    return JSTaggedValue(index);
}

JSTaggedValue ContainersLightWeightMap::GetIndexOfValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetIndexOfValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));

    int32_t index = JSAPILightWeightMap::GetIndexOfValue(thread, JSHandle<JSAPILightWeightMap>::Cast(self), value);
    return JSTaggedValue(index);
}

JSTaggedValue ContainersLightWeightMap::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, IsEmpty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }
    return JSHandle<JSAPILightWeightMap>::Cast(self)->IsEmpty();
}

JSTaggedValue ContainersLightWeightMap::GetKeyAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetKeyAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));

    if (!index->IsInt()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::GetKeyAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                         index->GetInt());
}

JSTaggedValue ContainersLightWeightMap::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Keys);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightMapIterator::CreateLightWeightMapIterator(thread, self, IterationKind::KEY);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::SetAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, SetAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> lightWeightMap(GetCallArg(argv, 0));

    if (!lightWeightMap->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "argv is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSAPILightWeightMap::SetAll(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                JSHandle<JSAPILightWeightMap>::Cast(lightWeightMap));
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLightWeightMap::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Set);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));

    JSAPILightWeightMap::Set(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key, value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLightWeightMap::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    return JSAPILightWeightMap::Remove(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
}

JSTaggedValue ContainersLightWeightMap::RemoveAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, RemoveAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    if (!index->IsInt()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not integer", JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::RemoveAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                         index->GetInt());
}

JSTaggedValue ContainersLightWeightMap::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSAPILightWeightMap::Clear(thread, JSHandle<JSAPILightWeightMap>::Cast(self));
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLightWeightMap::SetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, SetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));
    if (!index->IsInt()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "index is not Integer", JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::SetValueAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                           index->GetInt(), value);
}

JSTaggedValue ContainersLightWeightMap::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check lightweightmap object
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }
    // get and check callback function
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The first arg is not Callable", JSTaggedValue::Exception());
    }
    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);
    JSHandle<JSAPILightWeightMap> tmap = JSHandle<JSAPILightWeightMap>::Cast(self);
    JSMutableHandle<TaggedArray> keys(thread, tmap->GetKeys());
    JSMutableHandle<TaggedArray> values(thread, tmap->GetValues());
    int elements = tmap->GetSize();

    int index = 0;
    uint32_t length = keys->GetLength();
    const size_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    while (index < elements) {
        // ignore the hash value is required to determine the true index
        // Let funcResult be Call(callbackfn, T, «e, e, S»).
        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
        info.SetCallArg(values->Get(index), keys->Get(index), self.GetTaggedValue());
        JSTaggedValue ret = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);

        // check entries should be update, size will be update in tmap set or remove.
        if (tmap->GetLength() != length) {
            keys.Update(tmap->GetKeys());
            values.Update(tmap->GetValues());
            elements = tmap->GetSize();
            length = keys->GetLength();
        }
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLightWeightMap::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, ToString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::ToString(thread, JSHandle<JSAPILightWeightMap>::Cast(self));
}

JSTaggedValue ContainersLightWeightMap::GetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    if (!index->IsInt()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the index is not integer", JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::GetValueAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                           index->GetInt());
}

JSTaggedValue ContainersLightWeightMap::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Keys);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightMapIterator::CreateLightWeightMapIterator(thread, self, IterationKind::VALUE);
    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript::containers
