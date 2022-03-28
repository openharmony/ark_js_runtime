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

#include "js_api_stack.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
bool JSAPIStack::Empty()
{
    if (this->GetTop() == -1) {
        return true;
    }
    return false;
}

JSTaggedValue JSAPIStack::Push(JSThread *thread, const JSHandle<JSAPIStack> &stack,
                               const JSHandle<JSTaggedValue> &value)
{
    int top = static_cast<int>(stack->GetTop());
    JSHandle<TaggedArray> elements = GrowCapacity(thread, stack, top + 1);

    ASSERT(!elements->IsDictionaryMode());
    elements->Set(thread, top + 1, value);
    stack->SetTop(++top);
    return value.GetTaggedValue();
}

JSTaggedValue JSAPIStack::Peek()
{
    int top = this->GetTop();
    if (top == -1) {
        return JSTaggedValue::Undefined();
    }

    TaggedArray *elements = TaggedArray::Cast(this->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(top);
}

JSTaggedValue JSAPIStack::Pop()
{
    int top = this->GetTop();
    if (top == -1) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(this->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    this->SetTop(--top);
    return elements->Get(top + 1);
}

int JSAPIStack::Search(const JSHandle<JSTaggedValue> &value)
{
    int top = this->GetTop();
    TaggedArray *elements = TaggedArray::Cast(this->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    for (int i = 0; i <= top; i++) {
        if (value.GetTaggedValue() == elements->Get(i)) {
            return i;
        }
    }
    return -1;
}

JSHandle<TaggedArray> JSAPIStack::GrowCapacity(const JSThread *thread, const JSHandle<JSAPIStack> &obj,
                                               uint32_t capacity)
{
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    uint32_t oldLength = oldElements->GetLength();
    if (capacity < oldLength) {
        return oldElements;
    }
    uint32_t newCapacity = ComputeCapacity(capacity);
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldLength, newCapacity);

    obj->SetElements(thread, newElements);
    return newElements;
}


JSTaggedValue JSAPIStack::Get(const uint32_t index)
{
    ASSERT(static_cast<int>(index) <= GetTop());
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    return elements->Get(index);
}

JSTaggedValue JSAPIStack::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIStack::Has(JSTaggedValue value) const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    int top = static_cast<int>(GetTop());
    if (top == -1) {
        return false;
    }
    
    for (int i = 0; i < top + 1; i++) {
        if (JSTaggedValue::SameValue(elements->Get(i), value)) {
            return true;
        }
    }
    return false;
}

JSHandle<TaggedArray> JSAPIStack::OwnKeys(JSThread *thread, const JSHandle<JSAPIStack> &obj)
{
    uint32_t top = obj->GetTop();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys = factory->NewTaggedArray(top);

    for (uint32_t i = 0; i < top + 1; i++) {
        keys->Set(thread, i, JSTaggedValue(i));
    }

    return keys;
}

bool JSAPIStack::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIStack> &obj,
                                const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }

    uint32_t length = static_cast<int>(obj->GetTop()) + 1;
    if (index + 1 > length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>::Cast(obj), key, desc);
}
}  // namespace panda::ecmascript
