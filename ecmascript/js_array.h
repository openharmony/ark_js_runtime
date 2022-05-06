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

#ifndef ECMASCRIPT_JSARRAY_H
#define ECMASCRIPT_JSARRAY_H

#include <limits>
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
// ecma6 9.4.2 Array Exotic Object
class JSArray : public JSObject {
public:
    static constexpr int LENGTH_INLINE_PROPERTY_INDEX = 0;

    CAST_CHECK(JSArray, IsJSArray);

    static JSHandle<JSTaggedValue> ArrayCreate(JSThread *thread, JSTaggedNumber length);
    static JSHandle<JSTaggedValue> ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                               const JSHandle<JSTaggedValue> &newTarget);
    static JSTaggedValue ArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &originalArray,
                                            JSTaggedNumber length);
    static bool ArraySetLength(JSThread *thread, const JSHandle<JSObject> &array, const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, const JSHandle<JSTaggedValue> &key,
                                  const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, uint32_t index,
                                  const PropertyDescriptor &desc);

    static bool IsLengthString(JSThread *thread, const JSHandle<JSTaggedValue> &key);
    // ecma6 7.3 Operations on Objects
    static JSHandle<JSArray> CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements);
    // use first inlined property slot for array length
    inline size_t GetArrayLength() const
    {
        return GetLength().GetArrayLength();
    }

    inline void SetArrayLength(const JSThread *thread, uint32_t length)
    {
        SetLength(thread, JSTaggedValue(length));
    }

    static constexpr size_t LENGTH_OFFSET = JSObject::SIZE;
    ACCESSORS(Length, LENGTH_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LENGTH_OFFSET, SIZE)

    static const uint32_t MAX_ARRAY_INDEX = MAX_ELEMENT_INDEX;
    DECL_DUMP()

    static int32_t GetArrayLengthOffset()
    {
        return LENGTH_OFFSET;
    }

    static bool PropertyKeyToArrayIndex(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint32_t *output);

    static JSTaggedValue LengthGetter(JSThread *thread, const JSHandle<JSObject> &self);

    static bool LengthSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                             bool mayThrow = false);

    static JSHandle<JSTaggedValue> FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                          uint32_t index);

    static JSHandle<JSTaggedValue> FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                          const JSHandle<JSTaggedValue> &key);

    static bool FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                                       const JSHandle<JSTaggedValue> &value);

    static bool FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static void Sort(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &fn);
    static bool IncludeInSortedValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &value);
    static JSHandle<TaggedArray> ToTaggedArray(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

private:
    static void SetCapacity(JSThread *thread, const JSHandle<JSObject> &array, uint32_t oldLen, uint32_t newLen);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSARRAY_H
