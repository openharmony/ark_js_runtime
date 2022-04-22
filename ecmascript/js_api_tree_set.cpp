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

#include "ecmascript/js_api_tree_set.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_tree.h"

namespace panda::ecmascript {
void JSAPITreeSet::Add(JSThread *thread, const JSHandle<JSAPITreeSet> &set, const JSHandle<JSTaggedValue> &value)
{
    if (!TaggedTreeSet::IsKey(value.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JS");
    }
    JSHandle<TaggedTreeSet> setHandle(thread, TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject()));

    JSTaggedValue newSet = TaggedTreeSet::Add(thread, setHandle, value);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    set->SetTreeSet(thread, newSet);
}

int JSAPITreeSet::GetSize() const
{
    return TaggedTreeSet::Cast(GetTreeSet().GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSAPITreeSet::GetKey(int entry) const
{
    ASSERT_PRINT(entry < GetSize(), "entry must less than capacity");
    JSTaggedValue key = TaggedTreeSet::Cast(GetTreeSet().GetTaggedObject())->GetKey(entry);
    return key.IsHole() ? JSTaggedValue::Undefined() : key;
}

bool JSAPITreeSet::Delete(JSThread *thread, const JSHandle<JSAPITreeSet> &set, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeSet> setHandle(thread, TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject()));

    int entry = TaggedTreeSet::FindEntry(thread, setHandle, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (entry < 0) {
        return false;
    }
    JSTaggedValue newSet = TaggedTreeSet::Delete(thread, setHandle, entry);
    set->SetTreeSet(thread, newSet);
    return true;
}

bool JSAPITreeSet::Has(JSThread *thread, const JSHandle<JSAPITreeSet> &set, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeSet> setHandle(thread, TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject()));
    return TaggedTreeSet::FindEntry(thread, setHandle, key) >= 0;
}

void JSAPITreeSet::Clear(const JSThread *thread, const JSHandle<JSAPITreeSet> &set)
{
    int cap = set->GetSize();
    JSTaggedValue internal = TaggedTreeSet::Create(thread, cap);
    set->SetTreeSet(thread, internal);
}

JSTaggedValue JSAPITreeSet::PopFirst(JSThread *thread, const JSHandle<JSAPITreeSet> &set)
{
    JSHandle<TaggedTreeSet> setHandle(thread, TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject()));
    int entry = setHandle->GetMinimum(setHandle->GetRootEntries());
    if (entry < 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<JSTaggedValue> value(thread, setHandle->GetKey(entry));
    JSTaggedValue newSet = TaggedTreeSet::Delete(thread, setHandle, entry);
    set->SetTreeSet(thread, newSet);
    return value.GetTaggedValue();
}

JSTaggedValue JSAPITreeSet::PopLast(JSThread *thread, const JSHandle<JSAPITreeSet> &set)
{
    JSHandle<TaggedTreeSet> setHandle(thread, TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject()));
    int entry = setHandle->GetMaximum(setHandle->GetRootEntries());
    if (entry < 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<JSTaggedValue> value(thread, setHandle->GetKey(entry));
    JSTaggedValue newSet = TaggedTreeSet::Delete(thread, setHandle, entry);
    set->SetTreeSet(thread, newSet);
    return value.GetTaggedValue();
}
}  // namespace panda::ecmascript
