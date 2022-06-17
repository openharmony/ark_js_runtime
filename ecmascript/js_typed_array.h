/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_TYPED_ARRAY_H
#define ECMASCRIPT_JS_TYPED_ARRAY_H

#include "ecmascript/tagged_array.h"
#include "ecmascript/js_dataview.h"
#include "js_object.h"

namespace panda::ecmascript {
enum class ContentType : uint8_t { None = 1, Number, BigInt };
class JSTypedArray : public JSObject {
public:
    static JSTypedArray *Cast(TaggedObject *object)
    {
    #if ECMASCRIPT_ENABLE_CAST_CHECK
        if (!(JSTaggedValue(object).IsTypedArray() || JSTaggedValue(object).IsJSTypedArray())) {
            UNREACHABLE();
        }
    #else
        ASSERT(JSTaggedValue(object).IsTypedArray() || JSTaggedValue(object).IsJSTypedArray());
    #endif
        return static_cast<JSTypedArray *>(object);
    }

    static JSHandle<JSTaggedValue> ToPropKey(JSThread *thread, const JSHandle<JSTaggedValue> &key);

    // 9.4.5 Integer-Indexed Exotic Objects
    // 9.4.5.1 [[GetOwnProperty]] ( P )
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                               const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);
    // 9.4.5.2 [[HasProperty]] ( P )
    static bool HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                            const JSHandle<JSTaggedValue> &key);
    // 9.4.5.3 [[DefineOwnProperty]] ( P, Desc )
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                  const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc);
    // 9.4.5.4 [[Get]] ( P, Receiver )
    static inline OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                              const JSHandle<JSTaggedValue> &key)
    {
        return GetProperty(thread, typedarray, key, typedarray);
    }
    static inline OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                              uint32_t index)
    {
        return FastElementGet(thread, typedarray, index);
    }
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);
    // 9.4.5.5 [[Set]] ( P, V, Receiver )
    static inline bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                   const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                   bool mayThrow = false)
    {
        return SetProperty(thread, typedarray, key, value, typedarray, mayThrow);
    }
    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                            const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                            const JSHandle<JSTaggedValue> &receiver, bool mayThrow = false);
    // s12 10.4.5.6 [[Delete]] ( P )
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                               const JSHandle<JSTaggedValue> &key);
    // 9.4.5.6 [[OwnPropertyKeys]] ( )
    static JSHandle<TaggedArray> OwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray);
    // 9.4.5.7 IntegerIndexedObjectCreate (prototype, internalSlotsList)
    // 9.4.5.8 IntegerIndexedElementGet ( O, index )
    static OperationResult IntegerIndexedElementGet(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                                    JSTaggedValue index);
    static OperationResult FastElementGet(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray, uint32_t index);
    static bool FastCopyElementToArray(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                       JSHandle<TaggedArray> &array);
    // 9.4.5.9 IntegerIndexedElementSet ( O, index, value )
    static bool IntegerIndexedElementSet(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                         JSTaggedValue index, const JSHandle<JSTaggedValue> &value);
    // s12 10.4.5.9 IsValidIntegerIndex ( O, index )
    static bool IsValidIntegerIndex(const JSHandle<JSTaggedValue> &typedArray, JSTaggedValue index);
    static JSTaggedValue FastGetPropertyByIndex(JSThread *thread, const JSTaggedValue typedarray, uint32_t index,
                                                JSType jsType);
    static JSTaggedValue FastSetPropertyByIndex(JSThread *thread, const JSTaggedValue typedarray, uint32_t index,
                                                JSTaggedValue value, JSType jsType);
    // only use in TypeArray fast set property
    static JSTaggedNumber NonEcmaObjectToNumber(JSThread *thread, const JSTaggedValue tagged);
    static constexpr size_t VIEWED_ARRAY_BUFFER_OFFSET = JSObject::SIZE;
    static DataViewType GetTypeFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName);
    ACCESSORS(ViewedArrayBuffer, VIEWED_ARRAY_BUFFER_OFFSET, TYPED_ARRAY_NAME_OFFSET)
    ACCESSORS(TypedArrayName, TYPED_ARRAY_NAME_OFFSET, BYTE_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ByteLength, uint32_t, BYTE_LENGTH_OFFSET, BYTE_OFFSET_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ByteOffset, uint32_t, BYTE_OFFSET_OFFSET, ARRAY_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ArrayLength, uint32_t, ARRAY_LENGTH_OFFSET, CONTENT_TYPE_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ContentType, ContentType, CONTENT_TYPE_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static const uint32_t MAX_TYPED_ARRAY_INDEX = MAX_ELEMENT_INDEX;
    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, VIEWED_ARRAY_BUFFER_OFFSET, BYTE_LENGTH_OFFSET)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_TYPED_ARRAY_H
