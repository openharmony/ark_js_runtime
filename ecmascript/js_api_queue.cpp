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

#include "js_api_queue.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "interpreter/fast_runtime_stub-inl.h"

namespace panda::ecmascript {
void JSAPIQueue::Add(JSThread *thread, const JSHandle<JSAPIQueue> &queue, const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = queue->GetLength().GetArrayLength();
    JSHandle<TaggedArray> elements = GrowCapacity(thread, queue, length + 1);

    ASSERT(!elements->IsDictionaryMode());
    uint32_t tail = queue->GetTail();
    
    elements->Set(thread, tail, value);
    queue->SetLength(thread, JSTaggedValue(++length));

    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    queue->SetTail((tail + 1) % elementsSize);
}

JSHandle<TaggedArray> JSAPIQueue::GrowCapacity(const JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                                               uint32_t capacity)
{
    JSHandle<TaggedArray> newElements;
    uint32_t front = obj->GetFront();
    uint32_t tail = obj->GetTail();
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t oldLength = oldElements->GetLength();
    uint32_t newCapacity = 0;
    // Set the oldLength(DEFAULT_CAPACITY_LENGTH = 8) of elements when constructing
    ASSERT(oldLength != 0);
    if (oldLength == 0) {
        newCapacity = ComputeCapacity(capacity);
        newElements = thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldLength, newCapacity);
    } else if ((tail + 1) % oldLength == front) {
        newCapacity = ComputeCapacity(capacity);
        newElements = thread->GetEcmaVM()->GetFactory()->CopyQueue(oldElements, oldLength, newCapacity, front, tail);
        front = 0;
        tail = oldLength - 1;
    } else {
        return oldElements;
    }

    obj->SetElements(thread, newElements);
    obj->SetFront(front);
    obj->SetTail(tail);
    return newElements;
}

JSTaggedValue JSAPIQueue::GetFirst(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    if (queue->GetLength().GetArrayLength() < 1) {
        return JSTaggedValue::Undefined();
    }

    uint32_t index = queue->GetFront();

    JSHandle<TaggedArray> elements(thread, queue->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(index);
}

JSTaggedValue JSAPIQueue::Pop(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    uint32_t length = queue->GetLength().GetArrayLength();
    if (length < 1) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<TaggedArray> elements(thread, queue->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t front = queue->GetFront();

    JSTaggedValue value = elements->Get(front);
    queue->SetLength(thread, JSTaggedValue(length-1));

    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    queue->SetFront((front + 1) % elementsSize);

    return value;
}

JSTaggedValue JSAPIQueue::Get(JSThread *thread, const uint32_t index)
{
    if (index < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Get property index out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    return elements->Get(index);
}

JSTaggedValue JSAPIQueue::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    if (index < 0 || index >= GetLength().GetArrayLength()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Set property index out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIQueue::Has(JSTaggedValue value) const
{
    uint32_t begin = GetCurrentFront();
    uint32_t end = GetCurrentTail();
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();

    uint32_t index = begin;
    while (index != end) {
        if (JSTaggedValue::SameValue(elements->Get(index), value)) {
            return true;
        }
        // Set the capacity(DEFAULT_CAPACITY_LENGTH = 8) of elements when constructing
        ASSERT(capacity != 0);
        index = (index + 1) % capacity;
    }
    return false;
}

JSHandle<TaggedArray> JSAPIQueue::OwnKeys(JSThread *thread, const JSHandle<JSAPIQueue> &obj)
{
    uint32_t length = obj->GetLength().GetArrayLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys = factory->NewTaggedArray(length);

    for (uint32_t i = 0; i < length; i++) {
        keys->Set(thread, i, JSTaggedValue(i));
    }

    return keys;
}

bool JSAPIQueue::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                                const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    uint32_t index = 0;
    if (!UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }

    uint32_t length = obj->GetLength().GetArrayLength();
    if (index < 0 || index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(obj), key, desc);
}

uint32_t JSAPIQueue::GetArrayLength(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    uint32_t begin = queue->GetCurrentFront();
    uint32_t end = queue->GetCurrentTail();
    JSHandle<TaggedArray> elements(thread, queue->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    uint32_t length = (end - begin + elementsSize) % elementsSize;
    return length;
}

uint32_t JSAPIQueue::GetNextPosition(uint32_t current)
{
    uint32_t next = 0;
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    next = (current + 1) % elementsSize;
    return next;
}
} // namespace panda::ecmascript
