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

#ifndef ECMASCRIPT_JSAPIQUEUE_H
#define ECMASCRIPT_JSAPIQUEUE_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPIQueue : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 8;
    static JSAPIQueue *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIQueue());
        return static_cast<JSAPIQueue *>(object);
    }

    static void Add(JSThread *thread, const JSHandle<JSAPIQueue> &queue, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue GetFirst(JSThread *thread, const JSHandle<JSAPIQueue> &queue);
    static JSTaggedValue Pop(JSThread *thread, const JSHandle<JSAPIQueue> &queue);
    static void ForEach(JSThread *thread, const JSHandle<JSAPIQueue> &queue, const JSHandle<JSTaggedValue> &value);
    JSTaggedValue Get(JSThread *thread, const uint32_t index);

    JSTaggedValue Set(JSThread *thread, const uint32_t index, JSTaggedValue value);
    bool Has(JSTaggedValue value) const;

    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPIQueue> &obj);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIQueue> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);

    inline uint32_t GetSize() const
    {
        return GetLength().GetArrayLength();
    }

    inline uint32_t GetCurrentFront() const
    {
        return GetFront();
    }

    inline uint32_t GetCurrentTail() const
    {
        return GetTail();
    }

    static constexpr size_t LENGTH_OFFSET = JSObject::SIZE;
    ACCESSORS(Length, LENGTH_OFFSET, FRONT_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Front, uint32_t, FRONT_OFFSET, TAIL_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Tail, uint32_t, TAIL_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LENGTH_OFFSET, FRONT_OFFSET)
    DECL_DUMP()

    static uint32_t GetArrayLength(JSThread *thread, const JSHandle<JSAPIQueue> &queue);

    uint32_t GetNextPosition(uint32_t currentPosition);

private:
    inline static uint32_t ComputeCapacity(uint32_t oldCapacity)
    {
        uint32_t newCapacity = oldCapacity + (oldCapacity >> 1U);
        return newCapacity > DEFAULT_CAPACITY_LENGTH ? newCapacity : DEFAULT_CAPACITY_LENGTH;
    }
    static JSHandle<TaggedArray> GrowCapacity(const JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                                              uint32_t capacity);
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_JSAPIQUEUE_H
