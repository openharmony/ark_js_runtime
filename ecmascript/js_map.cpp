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

#include "ecmascript/js_tagged_value.h"
#include "js_map.h"
#include "linked_hash_table.h"
#include "object_factory.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript {
void JSMap::Set(JSThread *thread, const JSHandle<JSMap> &map, const JSHandle<JSTaggedValue> &key,
                const JSHandle<JSTaggedValue> &value)
{
    if (!LinkedHashMap::IsKey(key.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JSSet");
    }
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));

    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Set(thread, mapHandle, key, value);
    map->SetLinkedMap(thread, newMap);
}

bool JSMap::Delete(const JSThread *thread, const JSHandle<JSMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    int entry = mapHandle->FindElement(key.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    mapHandle->RemoveEntry(thread, entry);

    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Shrink(thread, mapHandle);
    map->SetLinkedMap(thread, newMap);
    return true;
}

void JSMap::Clear(const JSThread *thread, const JSHandle<JSMap> &map)
{
    LinkedHashMap *linkedMap = LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject());
    linkedMap->Clear(thread);
}

bool JSMap::Has(JSTaggedValue key) const
{
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->Has(key);
}

JSTaggedValue JSMap::Get(JSTaggedValue key) const
{
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->Get(key);
}

int JSMap::GetSize() const
{
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSMap::GetKey(int entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(), "entry must be non-negative integer less than capacity");
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->GetKey(entry);
}

JSTaggedValue JSMap::GetValue(int entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(), "entry must be non-negative integer less than capacity");
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->GetValue(entry);
}
}  // namespace panda::ecmascript
