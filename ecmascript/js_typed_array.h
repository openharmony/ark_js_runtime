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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_TYPED_ARRAY_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_TYPED_ARRAY_H

#include "ecmascript/tagged_array.h"
#include "js_object.h"

namespace panda::ecmascript {
class JSTypedArray : public JSObject {
public:
    static JSTypedArray *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSTypedArray());
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
    // 9.4.5.6 [[OwnPropertyKeys]] ( )
    static JSHandle<TaggedArray> OwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray);
    // 9.4.5.7 IntegerIndexedObjectCreate (prototype, internalSlotsList)
    // 9.4.5.8 IntegerIndexedElementGet ( O, index )
    static OperationResult IntegerIndexedElementGet(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                                    JSTaggedValue index);
    static OperationResult FastElementGet(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray, uint32_t index);
    // 9.4.5.9 IntegerIndexedElementSet ( O, index, value )
    static bool IntegerIndexedElementSet(JSThread *thread, const JSHandle<JSTaggedValue> &typedarray,
                                         JSTaggedValue index, const JSHandle<JSTaggedValue> &value);

    static constexpr size_t VIEWED_ARRAY_BUFFER_OFFSET = JSObject::SIZE;
    ACCESSORS(ViewedArrayBuffer, VIEWED_ARRAY_BUFFER_OFFSET, TYPED_ARRAY_NAME_OFFSET)
    ACCESSORS(TypedArrayName, TYPED_ARRAY_NAME_OFFSET, BYTE_LENGTH_OFFSET)
    ACCESSORS(ByteLength, BYTE_LENGTH_OFFSET, BYTE_OFFSET_OFFSET)
    ACCESSORS(ByteOffset, BYTE_OFFSET_OFFSET, ARRAY_LENGTH_OFFSET)
    ACCESSORS(ArrayLength, ARRAY_LENGTH_OFFSET, SIZE)

    static const uint32_t MAX_TYPED_ARRAY_INDEX = MAX_ELEMENT_INDEX;
    DECL_DUMP()

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, VIEWED_ARRAY_BUFFER_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_TYPED_ARRAY_H