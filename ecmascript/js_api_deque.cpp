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

#include "js_api_deque.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
void JSAPIDeque::InsertFront(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> elements(thread, deque->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();
    ASSERT(capacity != 0);
    if ((first + capacity - 1) % capacity == last) {
        elements = GrowCapacity(thread, deque, capacity, first, last);
        ASSERT(!elements->IsDictionaryMode());
        deque->SetLast(capacity - 1);
        first = 0;
    }
    capacity = elements->GetLength();
    ASSERT(capacity != 0);
    first = (first + capacity - 1) % capacity;
    elements->Set(thread, first, value);
    deque->SetFirst(first);
}

void JSAPIDeque::InsertEnd(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> elements(thread, deque->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();
    ASSERT(capacity != 0);
    if (first == (last + 1) % capacity) {
        elements = GrowCapacity(thread, deque, capacity, first, last);
        ASSERT(!elements->IsDictionaryMode());
        deque->SetFirst(0);
        last = capacity - 1;
    }
    elements->Set(thread, last, value);
    capacity = elements->GetLength();
    ASSERT(capacity != 0);
    last = (last + 1) % capacity;
    deque->SetLast(last);
}

JSTaggedValue JSAPIDeque::GetFront()
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(GetFirst());
}

JSTaggedValue JSAPIDeque::GetTail()
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    uint32_t last = GetLast();
    ASSERT(capacity != 0);
    return elements->Get((last + capacity - 1) % capacity);
}

JSHandle<TaggedArray> JSAPIDeque::GrowCapacity(JSThread *thread, const JSHandle<JSAPIDeque> &deque,
                                               uint32_t oldCapacity, uint32_t first, uint32_t last)
{
    JSHandle<TaggedArray> oldElements(thread, deque->GetElements());
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t size = deque->GetSize();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyDeque(oldElements, newCapacity, size, first, last);
    deque->SetElements(thread, newElements);
    return newElements;
}

JSTaggedValue JSAPIDeque::PopFirst()
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    uint32_t first = GetFirst();
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    JSTaggedValue firstElement = elements->Get(first);
    ASSERT(capacity != 0);
    first = (first + 1) % capacity;
    SetFirst(first);
    return firstElement;
}

JSTaggedValue JSAPIDeque::PopLast()
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    uint32_t last = GetLast();
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    ASSERT(capacity != 0);
    last = (last + capacity - 1) % capacity;
    JSTaggedValue lastElement = elements->Get(last);
    SetLast(last);
    return lastElement;
}

bool JSAPIDeque::IsEmpty()
{
    uint32_t first = GetFirst();
    uint32_t last = GetLast();
    return first == last;
}

uint32_t JSAPIDeque::GetSize() const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t first = GetFirst();
    uint32_t last = GetLast();
    ASSERT(capacity != 0);
    return (last - first + capacity) % capacity;
}

JSTaggedValue JSAPIDeque::Get(const uint32_t index)
{
    ASSERT(index < GetSize());
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t first = GetFirst();
    ASSERT(capacity != 0);
    uint32_t curIndex = (first + index) % capacity;
    return elements->Get(curIndex);
}

JSTaggedValue JSAPIDeque::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    ASSERT(index < GetSize());
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t first = GetFirst();
    ASSERT(capacity != 0);
    uint32_t curIndex = (first + index) % capacity;
    elements->Set(thread, curIndex, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIDeque::Has(JSTaggedValue value) const
{
    uint32_t first = GetFirst();
    uint32_t last = GetLast();
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t index = first;
    while (index != last) {
        if (JSTaggedValue::SameValue(elements->Get(index), value)) {
            return true;
        }
        ASSERT(capacity != 0);
        index = (index + 1) % capacity;
    }
    return false;
}

JSHandle<TaggedArray> JSAPIDeque::OwnKeys(JSThread *thread, const JSHandle<JSAPIDeque> &deque)
{
    uint32_t length = deque->GetSize();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys = factory->NewTaggedArray(length);
    
    JSHandle<TaggedArray> oldElements(thread, deque->GetElements());
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t oldCapacity = oldElements->GetLength();
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t firstIndex = deque->GetFirst();
    uint32_t lastIndex = deque->GetLast();
    uint32_t size = deque->GetSize();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyDeque(oldElements, newCapacity, size, firstIndex, lastIndex);
    deque->SetFirst(0);
    deque->SetLast(size - 1);
    deque->SetElements(thread, newElements);

    for (uint32_t i = 0; i < size; i++) {
        keys->Set(thread, i, JSTaggedValue(i));
    }

    return keys;
}

bool JSAPIDeque::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIDeque> &deque,
                                const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }

    uint32_t length = deque->GetSize();
    if (index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }

    deque->Get(index);
    return true;
}

JSTaggedValue JSAPIDeque::GetIteratorObj(JSThread *thread, const JSHandle<JSAPIDeque> &deque)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIDequeIterator> iter(factory->NewJSAPIDequeIterator(deque));

    return iter.GetTaggedValue();
}

OperationResult JSAPIDeque::GetProperty(JSThread *thread, const JSHandle<JSAPIDeque> &obj,
                                        const JSHandle<JSTaggedValue> &key)
{
    int length = static_cast<int>(obj->GetSize());
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetProperty index out-of-bounds",
                                     OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }

    return OperationResult(thread, obj->Get(index), PropertyMetaData(false));
}
} // namespace panda::ecmascript
