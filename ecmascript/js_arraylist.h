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

#ifndef ECMASCRIPT_JSARRAYLIST_H
#define ECMASCRIPT_JSARRAYLIST_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSArrayList : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 10;
    static JSArrayList *Cast(ObjectHeader *object)
    {
        return static_cast<JSArrayList *>(object);
    }

    static void Add(JSThread *thread, const JSHandle<JSArrayList> &arrayList, const JSHandle<JSTaggedValue> &value);

    JSTaggedValue Get(JSThread *thread, const uint32_t index);

    JSTaggedValue Set(JSThread *thread, const uint32_t index, JSTaggedValue value);
    bool Has(JSTaggedValue value) const;

    static bool Delete(JSThread *thread, const JSHandle<JSArrayList> &obj, const JSHandle<JSTaggedValue> &key);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSArrayList> &obj);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSArrayList> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);

    inline int GetSize() const
    {
        return GetLength().GetArrayLength();
    }

    static constexpr size_t LENGTH_OFFSET = JSObject::SIZE;
    ACCESSORS(Length, LENGTH_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LENGTH_OFFSET, SIZE)
    DECL_DUMP()
private:
    inline static uint32_t ComputeCapacity(uint32_t oldCapacity)
    {
        uint32_t newCapacity = oldCapacity + (oldCapacity >> 1U);
        return newCapacity > DEFAULT_CAPACITY_LENGTH ? newCapacity : DEFAULT_CAPACITY_LENGTH;
    }
    static JSHandle<TaggedArray> GrowCapacity(const JSThread *thread, const JSHandle<JSArrayList> &obj,
                                              uint32_t capacity);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSARRAYLIST_H
