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

#include "js_api_linked_list.h"
#include "ecmascript/js_api_linked_list_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "tagged_list.h"

namespace panda::ecmascript {
JSTaggedValue JSAPILinkedList::Insert(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                      const JSHandle<JSTaggedValue> &value, const int index)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (index < 0 || index > nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds",
                                     JSTaggedValue::Exception());
    }
    JSTaggedValue newList = TaggedDoubleList::Insert(thread, doubleList, value, index);
    list->SetDoubleList(thread, newList);
    return JSTaggedValue::True();
}

void JSAPILinkedList::Clear(JSThread *thread)
{
    TaggedDoubleList *doubleList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    doubleList->Clear(thread);
}

JSHandle<JSAPILinkedList> JSAPILinkedList::Clone(JSThread *thread, const JSHandle<JSAPILinkedList> &list)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int capacity = doubleList->NumberOfNodes();
    JSHandle<JSAPILinkedList> newLinkedList = thread->GetEcmaVM()->GetFactory()->NewJSAPILinkedList();
    JSTaggedValue newTaggedList = TaggedDoubleList::Create(thread, capacity);
    JSHandle<TaggedDoubleList> newDoubleList(thread, newTaggedList);
    doubleList->CopyArray(thread, newDoubleList);
    newLinkedList->SetDoubleList(thread, newDoubleList.GetTaggedValue());
    return newLinkedList;
}

JSTaggedValue JSAPILinkedList::RemoveFirst(JSThread *thread, const JSHandle<JSAPILinkedList> &list)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (nodeLength < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "there is no element", JSTaggedValue::Exception());
    }
    return doubleList->RemoveFirst(thread);
}

JSTaggedValue JSAPILinkedList::RemoveLast(JSThread *thread, const JSHandle<JSAPILinkedList> &list)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (nodeLength < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "there is no element", JSTaggedValue::Exception());
    }
    return doubleList->RemoveLast(thread);
}

JSTaggedValue JSAPILinkedList::RemoveByIndex(JSThread *thread, JSHandle<JSAPILinkedList> &list, const int index)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (index < 0 || index >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    return doubleList->RemoveByIndex(thread, index);
}

JSTaggedValue JSAPILinkedList::Remove(JSThread *thread, const JSTaggedValue &element)
{
    TaggedDoubleList *doubleList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    int nodeLength = doubleList->Length();
    if (nodeLength < 0) {
        return JSTaggedValue::False();
    }
    return doubleList->Remove(thread, element);
}

JSTaggedValue JSAPILinkedList::RemoveFirstFound(JSThread *thread, JSHandle<JSAPILinkedList> &list,
                                                const JSTaggedValue &element)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (nodeLength < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "there is no element", JSTaggedValue::Exception());
    }
    return TaggedDoubleList::RemoveFirstFound(thread, doubleList, element);
}

JSTaggedValue JSAPILinkedList::RemoveLastFound(JSThread *thread, JSHandle<JSAPILinkedList> &list,
                                               const JSTaggedValue &element)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (nodeLength < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "there is no element", JSTaggedValue::Exception());
    }
    return TaggedDoubleList::RemoveLastFound(thread, doubleList, element);
}

void JSAPILinkedList::Add(JSThread *thread, const JSHandle<JSAPILinkedList> &list, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    JSTaggedValue newLinkedList = TaggedDoubleList::Add(thread, doubleList, value);
    list->SetDoubleList(thread, newLinkedList);
}

void JSAPILinkedList::AddFirst(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                               const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    JSTaggedValue newLinkedList = TaggedDoubleList::AddFirst(thread, doubleList, value);
    list->SetDoubleList(thread, newLinkedList);
}

JSTaggedValue JSAPILinkedList::GetFirst()
{
    JSTaggedValue res = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject())->GetFirst();
    if (res.IsHole()) {
        return JSTaggedValue::Undefined();
    }
    return res;
}

JSTaggedValue JSAPILinkedList::GetLast()
{
    JSTaggedValue res = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject())->GetLast();
    if (res.IsHole()) {
        return JSTaggedValue::Undefined();
    }
    return res;
}

JSTaggedValue JSAPILinkedList::Set(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                   const int index, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    if (index < 0 || index >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    JSTaggedValue oldValue = doubleList->Get(index);
    TaggedDoubleList::Set(thread, doubleList, index, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return oldValue;
}

bool JSAPILinkedList::Has(const JSTaggedValue &element)
{
    TaggedDoubleList *doubleList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    return doubleList->Has(element);
}

JSTaggedValue JSAPILinkedList::Get(const int index)
{
    TaggedDoubleList *doubleList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    int nodeLength = doubleList->Length();
    if (index < 0 || index >= nodeLength) {
        return JSTaggedValue::Undefined();
    }
    return doubleList->Get(index);
}

JSTaggedValue JSAPILinkedList::GetIndexOf(const JSTaggedValue &element)
{
    TaggedDoubleList *doubleList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    return JSTaggedValue(doubleList->GetIndexOf(element));
}

JSTaggedValue JSAPILinkedList::GetLastIndexOf(const JSTaggedValue &element)
{
    TaggedDoubleList *doubleList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    return JSTaggedValue(doubleList->GetLastIndexOf(element));
}

JSTaggedValue JSAPILinkedList::ConvertToArray(const JSThread *thread, const JSHandle<JSAPILinkedList> &list)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    return TaggedDoubleList::ConvertToArray(thread, doubleList);
}

JSHandle<TaggedArray> JSAPILinkedList::OwnKeys(JSThread *thread, const JSHandle<JSAPILinkedList> &list)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList().GetTaggedObject());
    return TaggedDoubleList::OwnKeys(thread, doubleList);
}

bool JSAPILinkedList::GetOwnProperty(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                     const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    uint32_t length = static_cast<uint32_t>(doubleList->Length());
    if (index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index out-of-bounds", false);
    }

    list->Get(index);
    return true;
}

OperationResult JSAPILinkedList::GetProperty(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                             const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedDoubleList> doubleList(thread, list->GetDoubleList());
    int nodeLength = doubleList->Length();
    int index = key->GetInt();
    if (index < 0 || index >= nodeLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index out-of-bounds",
                                     OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    
    return OperationResult(thread, doubleList->Get(index), PropertyMetaData(false));
}
}  // namespace panda::ecmascript
