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

#ifndef ECMASCRIPT_JS_API_VECTOR_H
#define ECMASCRIPT_JS_API_VECTOR_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPIVector : public JSObject {
public:
    static constexpr int32_t DEFAULT_CAPACITY_LENGTH = 10;
    static JSAPIVector *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIVector());
        return static_cast<JSAPIVector *>(object);
    }

    static bool Add(JSThread *thread, const JSHandle<JSAPIVector> &vector, const JSHandle<JSTaggedValue> &value);

    static void Insert(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                       const JSHandle<JSTaggedValue> &value, int32_t index);

    static void SetLength(JSThread *thread, const JSHandle<JSAPIVector> &vector, uint32_t newSize);
    
    uint32_t GetCapacity();

    static void IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPIVector> &vector, int32_t newCapacity);

    static int32_t GetIndexOf(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                        const JSHandle<JSTaggedValue> &obj);

    static int32_t GetIndexFrom(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                            const JSHandle<JSTaggedValue> &obj, int32_t index);
    
    bool IsEmpty() const;

    JSTaggedValue GetLastElement();

    static int32_t GetLastIndexOf(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                              const JSHandle<JSTaggedValue> &obj);

    static int32_t GetLastIndexFrom(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                const JSHandle<JSTaggedValue> &obj, int32_t index);

    static bool Remove(JSThread *thread, const JSHandle<JSAPIVector> &vector, const JSHandle<JSTaggedValue> &obj);

    static JSTaggedValue RemoveByIndex(JSThread *thread, const JSHandle<JSAPIVector> &vector, int32_t index);

    static JSTaggedValue RemoveByRange(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                       int32_t fromIndex, int32_t toIndex);

    static JSHandle<JSAPIVector> SubVector(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                        int32_t fromIndex, int32_t toIndex);

    static JSTaggedValue ToString(JSThread *thread, const JSHandle<JSAPIVector> &vector);

    static JSTaggedValue ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                 const JSHandle<JSTaggedValue> &callbackFn,
                                 const JSHandle<JSTaggedValue> &thisArg);

    static JSTaggedValue ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                            const JSHandle<JSTaggedValue> &callbackFn,
                                            const JSHandle<JSTaggedValue> &thisArg);

    static JSTaggedValue Get(JSThread *thread, const JSHandle<JSAPIVector> &vector, int32_t index);

    JSTaggedValue Set(JSThread *thread, int32_t index, const JSTaggedValue &value);

    bool Has(const JSTaggedValue &value) const;
    
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPIVector> &obj);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIVector> &obj, const JSHandle<JSTaggedValue> &key);

    static void TrimToCurrentLength(JSThread *thread, const JSHandle<JSAPIVector> &vector);

    static void Clear(const JSHandle<JSAPIVector> &vector);

    static JSHandle<JSAPIVector> Clone(JSThread *thread, const JSHandle<JSAPIVector> &vector);

    static JSTaggedValue GetFirstElement(const JSHandle<JSAPIVector> &vector);

    static JSTaggedValue GetIteratorObj(JSThread *thread, const JSHandle<JSAPIVector> &obj);

    inline int32_t GetSize() const
    {
        return GetLength();
    }

    static constexpr size_t ELEMENT_COUNT_OFFSET = JSObject::SIZE;
    ACCESSORS_PRIMITIVE_FIELD(Length, int32_t, ELEMENT_COUNT_OFFSET, ENDL_OFFSET)
    DEFINE_ALIGN_SIZE(ENDL_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ELEMENT_COUNT_OFFSET, ELEMENT_COUNT_OFFSET)
    DECL_DUMP()
private:
    static void GrowCapacity(JSThread *thread, const JSHandle<JSAPIVector> &vector, uint32_t minCapacity);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSAPIVECTOR_H