/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "js_map_iterator.h"
#include "builtins/builtins_errors.h"
#include "js_array.h"
#include "js_map.h"
#include "linked_hash_table-inl.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.Let O be the this value
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    // 3.If O does not have all of the internal slots of a Map Iterator Instance (23.1.5.3), throw a TypeError
    // exception.
    if (!input->IsJSMapIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a map iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSMapIterator> iter(input);
    iter->Update(thread);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    // 4.Let m be O.[[IteratedMap]].
    JSHandle<JSTaggedValue> iteratedMap(thread, iter->GetIteratedMap());

    // 5.Let index be O.[[MapNextIndex]].
    int index = static_cast<int>(iter->GetNextIndex());
    IterationKind itemKind = iter->GetIterationKind();
    // 7.If m is undefined, return CreateIterResultObject(undefined, true).
    if (iteratedMap->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    };
    JSHandle<LinkedHashMap> map(iteratedMap);
    int totalElements = map->NumberOfElements() + map->NumberOfDeletedElements();

    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    while (index < totalElements) {
        JSTaggedValue key = map->GetKey(index);
        if (!key.IsHole()) {
            iter->SetNextIndex(index + 1);
            keyHandle.Update(key);
            // If itemKind is key, let result be e.[[Key]]
            if (itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, keyHandle, false).GetTaggedValue();
            }
            JSHandle<JSTaggedValue> value(thread, map->GetValue(index));
            // Else if itemKind is value, let result be e.[[Value]].
            if (itemKind == IterationKind::VALUE) {
                return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
            }
            // Else
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<TaggedArray> array(factory->NewTaggedArray(2));  // 2 means the length of array
            array->Set(thread, 0, keyHandle);
            array->Set(thread, 1, value);
            JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        index++;
    }
    // 13.Set O.[[IteratedMap]] to undefined.
    iter->SetIteratedMap(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
}

void JSMapIterator::Update(const JSThread *thread)
{
    [[maybe_unused]] DisallowGarbageCollection noGc;
    JSTaggedValue iteratedMap = GetIteratedMap();
    if (iteratedMap.IsUndefined()) {
        return;
    }
    LinkedHashMap *map = LinkedHashMap::Cast(iteratedMap.GetTaggedObject());
    if (map->GetNextTable().IsHole()) {
        return;
    }
    int index = static_cast<int>(GetNextIndex());
    JSTaggedValue nextTable = map->GetNextTable();
    while (!nextTable.IsHole()) {
        index -= map->GetDeletedElementsAt(index);
        map = LinkedHashMap::Cast(nextTable.GetTaggedObject());
        nextTable = map->GetNextTable();
    }
    SetIteratedMap(thread, JSTaggedValue(map));
    SetNextIndex(index);
}

JSHandle<JSTaggedValue> JSMapIterator::CreateMapIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                         IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSMap()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", undefinedHandle);
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSMapIterator(JSHandle<JSMap>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript
