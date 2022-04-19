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

#include "containers_treemap.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_tree_map.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_tree-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersTreeMap::TreeMapConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }

    // new TreeMap
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Set map’s internal slot with a new empty List.
    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(obj);
    JSTaggedValue internal = TaggedTreeMap::Create(thread);
    map->SetTreeMap(thread, internal);

    // If comparefn was supplied, let compare be comparefn; else let compare be hole.
    JSHandle<JSTaggedValue> compareFn(GetCallArg(argv, 0));
    if (compareFn->IsUndefined() || compareFn->IsNull()) {
        return map.GetTaggedValue();
    }
    if (!compareFn->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "comparefn is not Callable", JSTaggedValue::Exception());
    }

    TaggedTreeMap::Cast(internal.GetTaggedObject())->SetCompare(thread, compareFn.GetTaggedValue());
    return map.GetTaggedValue();
}

JSTaggedValue ContainersTreeMap::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // get and check this map
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    JSAPITreeMap::Set(thread, map, key, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return map.GetTaggedValue();
}

JSTaggedValue ContainersTreeMap::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    return JSAPITreeMap::Get(thread, map, key);
}

JSTaggedValue ContainersTreeMap::Remove(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Remove);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // get and check this map
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    return JSAPITreeMap::Delete(thread, map, key);
}

JSTaggedValue ContainersTreeMap::HasKey(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, HasKey);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);

    bool flag = JSAPITreeMap::HasKey(thread, JSHandle<JSAPITreeMap>::Cast(map), key);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return GetTaggedBoolean(flag);
}

JSTaggedValue ContainersTreeMap::HasValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, HasValue);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    bool flag = map->HasValue(thread, GetCallArg(argv, 0));
    return GetTaggedBoolean(flag);
}

JSTaggedValue ContainersTreeMap::GetFirstKey(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, GetFirstKey);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    return TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject())->GetFirstKey();
}

JSTaggedValue ContainersTreeMap::GetLastKey(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, GetLastKey);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    return TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject())->GetLastKey();
}

JSTaggedValue ContainersTreeMap::SetAll(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, SetAll);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Incorrect parameters, it should be TreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> dst = JSHandle<JSAPITreeMap>::Cast(self);
    JSHandle<TaggedTreeMap> dmap(thread, dst->GetTreeMap());
    JSHandle<TaggedTreeMap> smap(thread, JSHandle<JSAPITreeMap>::Cast(obj)->GetTreeMap());

    JSTaggedValue tmap = TaggedTreeMap::SetAll(thread, dmap, smap);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    dst->SetTreeMap(thread, tmap);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersTreeMap::Clear(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSAPITreeMap::Clear(thread, JSHandle<JSAPITreeMap>::Cast(self));
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersTreeMap::GetLowerKey(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, GetLowerKey);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);

    JSHandle<TaggedTreeMap> tmap(thread, map->GetTreeMap());
    return TaggedTreeMap::GetLowerKey(thread, tmap, key);
}

JSTaggedValue ContainersTreeMap::GetHigherKey(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, GetHigherKey);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);

    JSHandle<TaggedTreeMap> tmap(thread, map->GetTreeMap());
    return TaggedTreeMap::GetHigherKey(thread, tmap, key);
}

JSTaggedValue ContainersTreeMap::Replace(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Replace);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);

    bool success = JSAPITreeMap::Replace(thread, map, key, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return GetTaggedBoolean(success);
}

JSTaggedValue ContainersTreeMap::Keys(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSAPITreeMapIterator::CreateTreeMapIterator(thread, self, IterationKind::KEY);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersTreeMap::Values(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSAPITreeMapIterator::CreateTreeMapIterator(thread, self, IterationKind::VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersTreeMap::Entries(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPITreeMapIterator::CreateTreeMapIterator(thread, self, IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersTreeMap::ForEach(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check TreeMap object
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }

    // get and check callback function
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The first arg is not Callable", JSTaggedValue::Exception());
    }

    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);
    JSHandle<JSAPITreeMap> tmap = JSHandle<JSAPITreeMap>::Cast(self);
    JSMutableHandle<TaggedTreeMap> iteratedMap(thread, tmap->GetTreeMap());
    int elements = iteratedMap->NumberOfElements();
    JSMutableHandle<TaggedArray> entries(TaggedTreeMap::GetArrayFromMap(thread, iteratedMap));

    int index = 0;
    size_t length = entries->GetLength();
    const size_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    while (index < elements) {
        int entriesIndex = entries->Get(index).GetInt();
        key.Update(iteratedMap->GetKey(entriesIndex));
        value.Update(iteratedMap->GetValue(entriesIndex));

        // Let funcResult be Call(callbackfn, T, «e, e, S»).
        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
        info.SetCallArg(value.GetTaggedValue(), key.GetTaggedValue(), self.GetTaggedValue());
        JSTaggedValue ret = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);

        // check entries should be update, size will be update in tmap set or remove.
        if (tmap->GetSize() != static_cast<int>(length)) {
            iteratedMap.Update(tmap->GetTreeMap());
            entries.Update(TaggedTreeMap::GetArrayFromMap(thread, iteratedMap).GetTaggedValue());
            elements = iteratedMap->NumberOfElements();
            length = entries->GetLength();
        }
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersTreeMap::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, GetLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }
    int count = JSHandle<JSAPITreeMap>::Cast(self)->GetSize();
    return JSTaggedValue(count);
}

JSTaggedValue ContainersTreeMap::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeMap, IsEmpty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this map
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPITreeMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeMap", JSTaggedValue::Exception());
    }
    JSHandle<JSAPITreeMap> map = JSHandle<JSAPITreeMap>::Cast(self);
    return GetTaggedBoolean(map->GetSize() == 0);
}
}  // namespace panda::ecmascript::containers
