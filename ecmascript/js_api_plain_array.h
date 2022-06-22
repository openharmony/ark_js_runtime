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

#ifndef ECMASCRIPT_JS_API_PLAIN_ARRAY_H
#define ECMASCRIPT_JS_API_PLAIN_ARRAY_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPIPlainArray : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 8;
    static JSAPIPlainArray *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIPlainArray());
        return static_cast<JSAPIPlainArray *>(object);
    }
    static void Add(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj, JSHandle<JSTaggedValue> key,
                    JSHandle<JSTaggedValue> value);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                               const JSHandle<JSTaggedValue> &key);
    static JSHandle<TaggedArray> CreateSlot(const JSThread *thread, const uint32_t capacity);
    static JSHandle<JSAPIPlainArray> Clone(JSThread *thread, const JSHandle<JSAPIPlainArray> &plainArray);
    static JSHandle<JSTaggedValue> GetIteratorObj(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                                                  IterationKind kind);
    static JSTaggedValue ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                 const JSHandle<JSTaggedValue> &callbackFn, const JSHandle<JSTaggedValue> &thisArg);
    static JSTaggedValue ToString(JSThread *thread, const JSHandle<JSAPIPlainArray> &plainarray);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                                const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue Set(JSThread *thread, const JSHandle<JSAPIPlainArray> &obj,
                             const uint32_t index, JSTaggedValue value);
    JSTaggedValue RemoveAt(JSThread *thread, JSTaggedValue index);
    JSTaggedValue GetIndexOfKey(int32_t key);
    JSTaggedValue GetIndexOfValue(JSTaggedValue value);
    JSTaggedValue GetKeyAt(int32_t index);
    JSTaggedValue GetValueAt(int32_t index);
    JSTaggedValue Get(const JSTaggedValue key);
    JSTaggedValue Remove(JSThread *thread, JSTaggedValue key);
    JSTaggedValue RemoveRangeFrom(JSThread *thread, int32_t index, int32_t batchSize);
    bool SetValueAt(JSThread *thread, JSTaggedValue index, JSTaggedValue value);
    bool Has(const int32_t key);
    bool IsEmpty();
    bool AdjustForward(JSThread *thread, int32_t index, int32_t forwardSize);
    int32_t BinarySearch(TaggedArray *array, int32_t fromIndex, int32_t toIndex, int32_t key);
    void Clear(JSThread *thread);
    void AdjustArray(JSThread *thread, TaggedArray *srcArray, int32_t fromIndex, int32_t toIndex,
                     bool direction);
    inline int GetSize() const
    {
        return GetLength();
    }
    static constexpr size_t KEYS_OFFSET = JSObject::SIZE;
    ACCESSORS(Keys, KEYS_OFFSET, VALUES_OFFSET);
    ACCESSORS(Values, VALUES_OFFSET, LENGTH_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Length, int32_t, LENGTH_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, KEYS_OFFSET, LENGTH_OFFSET)
    DECL_DUMP()
private:
    inline static uint32_t ComputeCapacity(uint32_t oldCapacity)
    {
        uint32_t newCapacity = oldCapacity + (oldCapacity >> 1U);
        return newCapacity > DEFAULT_CAPACITY_LENGTH ? newCapacity : DEFAULT_CAPACITY_LENGTH;
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_PLAIN_ARRAY_H
