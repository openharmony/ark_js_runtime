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

#include "ecmascript/js_weak_container.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/object_factory.h"
#include "libpandabase/utils/bit_utils.h"

namespace panda::ecmascript {
void JSWeakMap::Set(JSThread *thread, const JSHandle<JSWeakMap> &map, const JSHandle<JSTaggedValue> &key,
                    const JSHandle<JSTaggedValue> &value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (!LinkedHashMap::IsKey(JSTaggedValue(key.GetTaggedValue().CreateAndGetWeakRef()))) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JSMap");
    }
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));

    auto result = LinkedHashMap::SetWeakRef(thread, mapHandle, key, value);
    map->SetLinkedMap(thread, result);
}

bool JSWeakMap::Delete(JSThread *thread, const JSHandle<JSWeakMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    int entry = mapHandle->FindElement(key.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    mapHandle->RemoveEntry(thread, entry);

    auto result = LinkedHashMap::Shrink(thread, mapHandle);
    map->SetLinkedMap(thread, result);
    return true;
}

bool JSWeakMap::Has(JSTaggedValue key) const
{
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->Has(key);
}

JSTaggedValue JSWeakMap::Get(JSTaggedValue key) const
{
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->Get(key);
}

int JSWeakMap::GetSize() const
{
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->NumberOfElements();
}

void JSWeakSet::Add(JSThread *thread, const JSHandle<JSWeakSet> &weakSet, const JSHandle<JSTaggedValue> &value)
{
    if (!LinkedHashSet::IsKey(value.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JSWeakSet");
    }
    JSHandle<LinkedHashSet> weakSetHandle(thread, LinkedHashSet::Cast(weakSet->GetLinkedSet().GetTaggedObject()));

    auto result = LinkedHashSet::AddWeakRef(thread, weakSetHandle, value);
    weakSet->SetLinkedSet(thread, result);
}

bool JSWeakSet::Delete(JSThread *thread, const JSHandle<JSWeakSet> &weakSet, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<LinkedHashSet> weakSetHandle(thread, LinkedHashSet::Cast(weakSet->GetLinkedSet().GetTaggedObject()));
    int entry = weakSetHandle->FindElement(value.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    weakSetHandle->RemoveEntry(thread, entry);
    auto result = LinkedHashSet::Shrink(thread, weakSetHandle);
    weakSet->SetLinkedSet(thread, result);
    return true;
}

bool JSWeakSet::Has(JSTaggedValue value) const
{
    return LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject())->Has(value);
}

int JSWeakSet::GetSize() const
{
    return LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject())->NumberOfElements();
}
}  // namespace panda::ecmascript
