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

#ifndef ECMASCRIPT_TAGGED_QUEUE_INL_H
#define ECMASCRIPT_TAGGED_QUEUE_INL_H

#include "ecmascript/tagged_queue.h"

namespace panda::ecmascript {
inline TaggedQueue *TaggedQueue::Create(JSThread *thread, array_size_t capacity, JSTaggedValue initVal)
{
    array_size_t length = QueueToArrayIndex(capacity);

    auto queue = TaggedQueue::Cast(*thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length, initVal));
    queue->SetStart(thread, JSTaggedValue(0));  // equal to 0 when add 1.
    queue->SetEnd(thread, JSTaggedValue(0));
    queue->SetCapacity(thread, JSTaggedValue(capacity));
    return queue;
}

inline JSTaggedValue TaggedQueue::Pop(JSThread *thread)
{
    if (Empty()) {
        return JSTaggedValue::Hole();
    }

    array_size_t start = GetStart().GetArrayLength();
    JSTaggedValue value = Get(start);

    array_size_t capacity = GetCapacity().GetArrayLength();
    ASSERT(capacity != 0);
    SetStart(thread, JSTaggedValue((start + 1) % capacity));
    return value;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_QUEUE_INL_H