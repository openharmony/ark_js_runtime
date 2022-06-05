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

#include "js_api_list.h"
#include "ecmascript/js_api_list_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_list.h"

namespace panda::ecmascript {
void JSAPIList::Add(JSThread *thread, const JSHandle<JSAPIList> &list, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    JSTaggedValue newList = TaggedSingleList::Add(thread, singleList, value);
    list->SetSingleList(thread, newList);
}

JSTaggedValue JSAPIList::GetFirst()
{
    JSTaggedValue res = TaggedSingleList::Cast(GetSingleList().GetTaggedObject())->GetFirst();
    if (res.IsHole()) {
        return JSTaggedValue::Undefined();
    }
    return res;
}

JSTaggedValue JSAPIList::GetLast()
{
    JSTaggedValue res = TaggedSingleList::Cast(GetSingleList().GetTaggedObject())->GetLast();
    if (res.IsHole()) {
        return JSTaggedValue::Undefined();
    }
    return res;
}

JSTaggedValue JSAPIList::Insert(JSThread *thread, const JSHandle<JSAPIList> &list, const JSHandle<JSTaggedValue> &value,
                                const int index)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    int nodeLength = singleList->Length();
    if (index < 0 || index > nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    JSTaggedValue newList = TaggedSingleList::Insert(thread, singleList, value, index);
    list->SetSingleList(thread, newList);
    return JSTaggedValue::True();
}

JSTaggedValue JSAPIList::Set(JSThread *thread, const JSHandle<JSAPIList> &list,
                             const int index, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    int nodeLength = singleList->Length();
    if (index < 0 || index >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    JSTaggedValue oldValue = singleList->Get(index);
    TaggedSingleList::Set(thread, singleList, index, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return oldValue;
}

bool JSAPIList::Has(const JSTaggedValue &element)
{
    TaggedSingleList *singleList = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    return singleList->Has(element);
}

bool JSAPIList::IsEmpty()
{
    return TaggedSingleList::Cast(GetSingleList().GetTaggedObject())->IsEmpty();
}

JSTaggedValue JSAPIList::Get(const int index)
{
    TaggedSingleList *singleList = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    int nodeLength = singleList->Length();
    if (index < 0 || index >= nodeLength) {
        return JSTaggedValue::Undefined();
    }
    return singleList->Get(index);
}

JSTaggedValue JSAPIList::GetIndexOf(const JSTaggedValue &element)
{
    TaggedSingleList *singleList = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    return JSTaggedValue(singleList->GetIndexOf(element));
}

JSTaggedValue JSAPIList::GetLastIndexOf(const JSTaggedValue &element)
{
    TaggedSingleList *singleList = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    return JSTaggedValue(singleList->GetLastIndexOf(element));
}

void JSAPIList::Clear(JSThread *thread)
{
    TaggedSingleList *singleList = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    singleList->Clear(thread);
}

JSTaggedValue JSAPIList::RemoveByIndex(JSThread *thread, const JSHandle<JSAPIList> &list, const int &index)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    int nodeLength = singleList->Length();
    if (index < 0 || index >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    return singleList->RemoveByIndex(thread, index);
}

JSTaggedValue JSAPIList::Remove(JSThread *thread, const JSTaggedValue &element)
{
    TaggedSingleList *singleList = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    return singleList->Remove(thread, element);
}

JSTaggedValue JSAPIList::ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                            const JSHandle<JSTaggedValue> &callbackFn,
                                            const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(thisHandle);
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    return TaggedSingleList::ReplaceAllElements(thread, thisHandle, callbackFn, thisArg, singleList);
}

JSTaggedValue JSAPIList::Sort(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                              const JSHandle<JSTaggedValue> &callbackFn)
{
    JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(thisHandle);
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    return TaggedSingleList::Sort(thread, callbackFn, singleList);
}

JSTaggedValue JSAPIList::Equal(JSThread *thread, const JSHandle<JSAPIList> &list)
{
    JSHandle<TaggedSingleList> compareList(thread, list->GetSingleList());
    return TaggedSingleList::Cast(GetSingleList().GetTaggedObject())->Equal(compareList);
}

JSTaggedValue JSAPIList::ConvertToArray(const JSThread *thread, const JSHandle<JSAPIList> &list)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    return TaggedSingleList::ConvertToArray(thread, singleList);
}

JSTaggedValue JSAPIList::GetSubList(JSThread *thread, const JSHandle<JSAPIList> &list,
                                    const int fromIndex, const int toIndex)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    int nodeLength = singleList->Length();
    if (fromIndex < 0 || toIndex < 0 || toIndex >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the fromIndex or the toIndex is out-of-bounds",
                                     JSTaggedValue::Exception());
    }
    if (fromIndex >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the toIndex cannot be less than or equal to fromIndex",
                                     JSTaggedValue::Exception());
    }
    int len = TaggedSingleList::ELEMENTS_START_INDEX + (toIndex - fromIndex) * TaggedSingleList::ENTRY_SIZE;
    JSHandle<JSAPIList> sublist = thread->GetEcmaVM()->GetFactory()->NewJSAPIList();
    JSHandle<TaggedSingleList> subSingleList(thread, TaggedSingleList::Create(thread, len));
    TaggedSingleList::GetSubList(thread, singleList, fromIndex, toIndex, subSingleList);
    sublist->SetSingleList(thread, subSingleList);
    return sublist.GetTaggedValue();
}

JSHandle<TaggedArray> JSAPIList::OwnKeys(JSThread *thread, const JSHandle<JSAPIList> &list)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    return TaggedSingleList::OwnKeys(thread, singleList);
}

bool JSAPIList::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIList> &list, const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    uint32_t length = static_cast<uint32_t>(singleList->Length());
    if (index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }
    list->Get(index);
    return true;
}

OperationResult JSAPIList::GetProperty(JSThread *thread, const JSHandle<JSAPIList> &list,
                                       const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList());
    int nodeLength = singleList->Length();
    int index = key->GetInt();
    if (index < 0 || index >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetProperty index out-of-bounds",
                                     OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    
    return OperationResult(thread, singleList->Get(index), PropertyMetaData(false));
}
}  // namespace panda::ecmascript
