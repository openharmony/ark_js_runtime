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

#include "ecmascript/weak_vector.h"

#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
JSHandle<WeakVector> WeakVector::Create(const JSThread *thread, uint32_t capacity)
{
    ASSERT(capacity < MAX_VECTOR_INDEX);

    uint32_t length = VectorToArrayIndex(capacity);
    JSHandle<WeakVector> vector = JSHandle<WeakVector>(thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length));

    vector->SetEnd(thread, 0);
    return vector;
}

bool WeakVector::Delete(const JSThread *thread, uint32_t index)
{
    uint32_t end = GetEnd();
    if (index < end) {
        Set(thread, index, JSTaggedValue::Hole());
        return true;
    }
    return false;
}

JSHandle<WeakVector> WeakVector::Grow(const JSThread *thread, const JSHandle<WeakVector> &old, uint32_t newCapacity)
{
    uint32_t oldCapacity = old->GetCapacity();
    ASSERT(newCapacity > oldCapacity);
    if (oldCapacity == MAX_VECTOR_INDEX) {
        return old;
    }

    if (newCapacity > MAX_VECTOR_INDEX) {
        newCapacity = MAX_VECTOR_INDEX;
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newVec = factory->CopyArray(JSHandle<TaggedArray>(old), VectorToArrayIndex(oldCapacity),
                                                      VectorToArrayIndex(newCapacity));

    return JSHandle<WeakVector>(newVec);
}

uint32_t WeakVector::PushBack(const JSThread *thread, JSTaggedValue value)
{
    uint32_t end = GetEnd();
    if (end == GetCapacity()) {
        return TaggedArray::MAX_ARRAY_INDEX;
    }

    Set(thread, end, value);
    SetEnd(thread, end + 1);
    return end;
}
}  // namespace panda::ecmascript
