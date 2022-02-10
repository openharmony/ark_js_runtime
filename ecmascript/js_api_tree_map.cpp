/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/js_api_tree_map.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_tree-inl.h"

namespace panda::ecmascript {
void JSAPITreeMap::Set(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key,
                       const JSHandle<JSTaggedValue> &value)
{
    if (!TaggedTreeMap::IsKey(key.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JS");
    }
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject()));

    JSTaggedValue newMap = TaggedTreeMap::Set(thread, mapHandle, key, value);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    map->SetTreeMap(thread, newMap);
}

JSTaggedValue JSAPITreeMap::Get(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject()));
    return TaggedTreeMap::Get(thread, mapHandle, key);
}

int JSAPITreeMap::GetSize() const
{
    return TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSAPITreeMap::GetKey(int entry) const
{
    ASSERT_PRINT(entry < GetSize(), "entry must less than capacity");
    JSTaggedValue key = TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject())->GetKey(entry);
    return key.IsHole() ? JSTaggedValue::Undefined() : key;
}

JSTaggedValue JSAPITreeMap::GetValue(int entry) const
{
    ASSERT_PRINT(entry < GetSize(), "entry must less than capacity");
    JSTaggedValue value = TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject())->GetValue(entry);
    return value.IsHole() ? JSTaggedValue::Undefined() : value;
}

JSTaggedValue JSAPITreeMap::Delete(JSThread *thread, const JSHandle<JSAPITreeMap> &map,
                                   const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject()));
    int entry = TaggedTreeMap::FindEntry(thread, mapHandle, key);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (entry < 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<JSTaggedValue> value(thread, mapHandle->GetValue(entry));
    JSTaggedValue newMap = TaggedTreeMap::Delete(thread, mapHandle, entry);
    map->SetTreeMap(thread, newMap);
    return value.GetTaggedValue();
}

bool JSAPITreeMap::HasKey(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject()));
    return TaggedTreeMap::FindEntry(thread, mapHandle, key) >= 0;
}

bool JSAPITreeMap::HasValue(JSThread *thread, const JSHandle<JSTaggedValue> &value) const
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject()));
    return mapHandle->HasValue(thread, value.GetTaggedValue());
}

void JSAPITreeMap::Clear(const JSThread *thread, const JSHandle<JSAPITreeMap> &map)
{
    int cap = map->GetSize();
    JSTaggedValue internal = TaggedTreeMap::Create(thread, cap);
    map->SetTreeMap(thread, internal);
}

bool JSAPITreeMap::Replace(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap().GetTaggedObject()));
    int index = TaggedTreeMap::FindEntry(thread, mapHandle, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (index < 0) {
        return false;
    }
    mapHandle->SetValue(thread, index, value.GetTaggedValue());
    return true;
}
}  // namespace panda::ecmascript
