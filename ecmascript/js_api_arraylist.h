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

#ifndef ECMASCRIPT_JS_API_ARRAYLIST_H
#define ECMASCRIPT_JS_API_ARRAYLIST_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
/**
 * Provide the object of non ECMA standard jsapi container.
 * JSAPIArrayList provides dynamically modified array.
 * */
class JSAPIArrayList : public JSObject {
public:
    static constexpr uint32_t DEFAULT_CAPACITY_LENGTH = 10;
    static JSAPIArrayList *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIArrayList());
        return static_cast<JSAPIArrayList *>(object);
    }

    static bool Add(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList, const JSHandle<JSTaggedValue> &value);
    static void Insert(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                       const JSHandle<JSTaggedValue> &value, const int &index);
    static void Clear(JSThread *thread, const JSHandle<JSAPIArrayList> &obj);
    static JSHandle<JSAPIArrayList> Clone(JSThread *thread, const JSHandle<JSAPIArrayList> &obj);
    static uint32_t GetCapacity(JSThread *thread, const JSHandle<JSAPIArrayList> &obj);
    static void IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                   int capacity);
    static void TrimToCurrentLength(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList);
    static bool IsEmpty(const JSHandle<JSAPIArrayList> &arrayList);
    static int GetIndexOf(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                          const JSHandle<JSTaggedValue> &value);
    static int GetLastIndexOf(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                              const JSHandle<JSTaggedValue> &value);
    static bool RemoveByIndex(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList, int value);
    static bool Remove(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                       const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue RemoveByRange(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                       const JSHandle<JSTaggedValue> &value1, const JSHandle<JSTaggedValue> &value2);
    static JSTaggedValue ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                            const JSHandle<JSTaggedValue> &callbackFn,
                                            const JSHandle<JSTaggedValue> &thisArg);
    static JSHandle<JSAPIArrayList> SubArrayList(JSThread *thread, const JSHandle<JSAPIArrayList> &arrayList,
                                              const JSHandle<JSTaggedValue> &value1,
                                              const JSHandle<JSTaggedValue> &value2);
    static JSTaggedValue ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                 const JSHandle<JSTaggedValue> &callbackFn,
                                 const JSHandle<JSTaggedValue> &thisArg);

    static JSTaggedValue GetIteratorObj(JSThread *thread, const JSHandle<JSAPIArrayList> &obj);

    JSTaggedValue Set(JSThread *thread, const uint32_t index, JSTaggedValue value);
    JSTaggedValue Get(JSThread *thread, const uint32_t index);

    bool Has(const JSTaggedValue value) const;
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPIArrayList> &obj);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIArrayList> &obj,
                               const JSHandle<JSTaggedValue> &key);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSAPIArrayList> &obj,
                                       const JSHandle<JSTaggedValue> &key);
    inline int GetSize() const
    {
        return GetLength().GetInt();
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
    static JSHandle<TaggedArray> GrowCapacity(const JSThread *thread, const JSHandle<JSAPIArrayList> &obj,
                                              uint32_t capacity);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_ARRAYLIST_H
