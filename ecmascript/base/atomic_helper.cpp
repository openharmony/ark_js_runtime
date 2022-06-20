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

#include "ecmascript/base/atomic_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"

namespace panda::ecmascript::base {
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;

JSTaggedValue AtomicHelper::ValidateIntegerTypedArray(JSThread *thread, JSHandle<JSTaggedValue> typedArray,
                                                      bool waitable)
{
    // 1. If waitable is not present, set waitable to false.
    // 2. Let buffer be ? ValidateTypedArray(typedArray).
    JSTaggedValue buffer = TypedArrayHelper::ValidateTypedArray(thread, typedArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> bufferHandle(thread, buffer);

    // 3. Let typeName be typedArray.[[TypedArrayName]].
    // 4. Let type be the Element Type value in Table 60 for typeName.
    JSHandle<JSTaggedValue> typeName(thread, JSTypedArray::Cast(typedArray->GetTaggedObject())->GetTypedArrayName());
    DataViewType type = JSTypedArray::GetTypeFromName(thread, typeName);

    // 5. If waitable is true, then
    // a. If typeName is not "Int32Array" or "BigInt64Array", throw a TypeError exception.
    // 6. Else,
    // a. If ! IsUnclampedIntegerElementType(type) is false and ! IsBigIntElementType(type) is false,
    // throw a  TypeError exception.
    if (waitable) {
        if (!(type == DataViewType::INT32 || type == DataViewType::BIGINT64)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The typeName is not Int32Array/BigInt64Array.",
                                        JSTaggedValue::Exception());
        }
    } else {
        if (!(BuiltinsArrayBuffer::IsUnclampedIntegerElementType(type) ||
              BuiltinsArrayBuffer::IsBigIntElementType(type))) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The typedArray type is not UnclampedInteger/BigInt.",
                                        JSTaggedValue::Exception());
        }
    }
    // 7. Return buffer.
    return bufferHandle.GetTaggedValue();
}

uint32_t AtomicHelper::ValidateAtomicAccess(JSThread *thread, const JSHandle<JSTaggedValue> typedArray,
                                            JSHandle<JSTaggedValue> requestIndex)
{
    // 1. Assert: typedArray is an Object that has a [[ViewedArrayBuffer]] internal slot.
    ASSERT(typedArray->IsECMAObject() && typedArray->IsTypedArray());
    // 2. Let length be typedArray.[[ArrayLength]].
    JSHandle<JSObject> typedArrayObj(typedArray);
    JSHandle<JSTypedArray> srcObj(typedArray);
    int32_t length = static_cast<int32_t>(srcObj->GetArrayLength());

    // 3. Let accessIndex be ? ToIndex(requestIndex).
    JSTaggedNumber accessIndex = JSTaggedValue::ToIndex(thread, requestIndex);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    int32_t index = base::NumberHelper::DoubleInRangeInt32(accessIndex.GetNumber());

    // 4. Assert: accessIndex ≥ 0.
    ASSERT(index >= 0);

    // 5. If accessIndex ≥ length, throw a RangeError exception.
    if (index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Index is overflow.", 0);
    }

    // 6. Let arrayTypeName be typedArray.[[TypedArrayName]].
    // 7. Let elementSize be the Element Size value specified in Table 60 for arrayTypeName.
    // 8. Let offset be typedArray.[[ByteOffset]].
    JSHandle<JSTaggedValue> arrayTypeName(thread, JSTypedArray::Cast(*typedArrayObj)->GetTypedArrayName());
    uint32_t elementSize = TypedArrayHelper::GetSizeFromName(thread, arrayTypeName);
    uint32_t offset = srcObj->GetByteOffset();
    // 9. Return (accessIndex × elementSize) + offset.
    uint32_t allOffset = static_cast<uint32_t>(index) * elementSize + offset;
    return allOffset;
}

JSTaggedValue AtomicHelper::AtomicStore(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                        JSHandle<JSTaggedValue> index, JSHandle<JSTaggedValue> &value)
{
    JSTaggedValue bufferValue = ValidateIntegerTypedArray(thread, typedArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> buffer(thread, bufferValue);
    uint32_t indexedPosition = ValidateAtomicAccess(thread, typedArray, index);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> arrayTypeName(thread,
                                          JSTypedArray::Cast(typedArray->GetTaggedObject())->GetTypedArrayName());
    DataViewType type = JSTypedArray::GetTypeFromName(thread, arrayTypeName);
    JSHandle<JSTaggedValue> bufferTag;
    if (type == DataViewType::BIGUINT64 || type == DataViewType::BIGINT64) {
        JSTaggedValue integerValue = JSTaggedValue::ToBigInt(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        bufferTag = JSHandle<JSTaggedValue>(thread, integerValue);
    } else {
        JSTaggedNumber integerValue = JSTaggedValue::ToInteger(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        bufferTag = JSHandle<JSTaggedValue>(thread, integerValue);
    }
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ArrayBuffer of this value is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    BuiltinsArrayBuffer::SetValueInBuffer(thread, buffer.GetTaggedValue(), indexedPosition, type, bufferTag, true);
    return bufferTag.GetTaggedValue();
}

JSTaggedValue AtomicHelper::AtomicLoad(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                       JSHandle<JSTaggedValue> index)
{
    JSTaggedValue bufferValue = ValidateIntegerTypedArray(thread, typedArray);
    JSHandle<JSTaggedValue> buffer(thread, bufferValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t indexedPosition = ValidateAtomicAccess(thread, typedArray, index);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ArrayBuffer of this value is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> arrayTypeName(thread,
                                          JSTypedArray::Cast(typedArray->GetTaggedObject())->GetTypedArrayName());
    DataViewType elementType = JSTypedArray::GetTypeFromName(thread, arrayTypeName);
    return BuiltinsArrayBuffer::GetValueFromBuffer(thread, buffer.GetTaggedValue(),
                                                   indexedPosition, elementType, true);
}
}  // panda::ecmascript::base

