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

#ifndef ECMASCRIPT_JSAPIDEQUE_H
#define ECMASCRIPT_JSAPIDEQUE_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPIDeque : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 8;
    static JSAPIDeque *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIDeque());
        return static_cast<JSAPIDeque *>(object);
    }

    static void InsertFront(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &value);
    
    static void InsertEnd(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &value);

    static JSTaggedValue ForEach(JSThread *thread, const JSHandle<JSAPIDeque> &deque);

    static JSTaggedValue GetIteratorObj(JSThread *thread, const JSHandle<JSAPIDeque> &deque);
    
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPIDeque> &deque);

    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);

    JSTaggedValue GetFront();

    JSTaggedValue GetTail();

    JSTaggedValue PopFirst();

    JSTaggedValue PopLast();

    JSTaggedValue Get(const uint32_t index);

    JSTaggedValue Set(JSThread *thread, const uint32_t index, JSTaggedValue value);
        
    uint32_t GetSize() const;

    bool Has(JSTaggedValue value) const;

    static constexpr size_t FIRST_OFFSET = JSObject::SIZE;
    ACCESSORS_PRIMITIVE_FIELD(First, uint32_t, FIRST_OFFSET, LAST_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Last, uint32_t, LAST_OFFSET, ENDL_OFFSET)
    DEFINE_ALIGN_SIZE(ENDL_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ENDL_OFFSET, ENDL_OFFSET)
    DECL_DUMP()
private:
    inline static uint32_t ComputeCapacity(uint32_t oldCapacity)
    {
        uint32_t newCapacity = oldCapacity << 1U;
        return newCapacity > DEFAULT_CAPACITY_LENGTH ? newCapacity : DEFAULT_CAPACITY_LENGTH;
    }
    static JSHandle<TaggedArray> GrowCapacity(JSThread *thread, const JSHandle<JSAPIDeque> &deque,
                                              uint32_t oldCapacity, uint32_t first, uint32_t last);
    bool IsEmpty();
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_JSAPIDEQUE_H
