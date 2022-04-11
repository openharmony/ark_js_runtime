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

#include "ecmascript/builtins/builtins_arraybuffer.h"

#include <typeinfo>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "securec.h"

namespace panda::ecmascript::builtins {
// 24.1.2.1 ArrayBuffer(length)
JSTaggedValue BuiltinsArrayBuffer::ArrayBufferConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayBuffer, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newtarget is undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> lengthHandle = GetCallArg(argv, 0);
    JSTaggedNumber lenNum = JSTaggedValue::ToIndex(thread, lengthHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double length = lenNum.GetNumber();
    return AllocateArrayBuffer(thread, newTarget, length);
}

// 24.1.3.1 ArrayBuffer.isView(arg)
JSTaggedValue BuiltinsArrayBuffer::IsView(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    JSHandle<JSTaggedValue> arg = GetCallArg(argv, 0);
    // 1. If Type(arg) is not Object, return false.
    if (!arg->IsECMAObject()) {
        return BuiltinsArrayBuffer::GetTaggedBoolean(false);
    }
    // 2. If arg has a [[ViewedArrayBuffer]] internal slot, return true.
    if (arg->IsDataView() || arg->IsTypedArray()) {
        return BuiltinsArrayBuffer::GetTaggedBoolean(true);
    }
    // 3. Return false.
    return BuiltinsArrayBuffer::GetTaggedBoolean(false);
}

// 24.1.3.3 get ArrayBuffer [ @@species ]
JSTaggedValue BuiltinsArrayBuffer::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    return GetThis(argv).GetTaggedValue();
}

// 24.1.4.1 get ArrayBuffer.prototype.byteLength
JSTaggedValue BuiltinsArrayBuffer::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    JSThread *thread = argv->GetThread();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    // 4. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "IsDetachedBuffer", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> arrBuf(thisHandle);
    // 5. Let length be the value of O’s [[ArrayBufferByteLength]] internal slot.
    uint32_t length = arrBuf->GetArrayBufferByteLength();
    // 6. Return length.
    return JSTaggedValue(length);
}

// 24.1.4.3 ArrayBuffer.prototype.slice(start, end)
JSTaggedValue BuiltinsArrayBuffer::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayBuffer, Slice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> arrBuf(thisHandle);
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    // 4. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value IsDetachedBuffer", JSTaggedValue::Exception());
    }
    // 5. Let len be the value of O’s [[ArrayBufferByteLength]] internal slot.
    int32_t len = static_cast<int32_t>(arrBuf->GetArrayBufferByteLength());
    JSHandle<JSTaggedValue> startHandle = GetCallArg(argv, 0);
    // 6. Let relativeStart be ToInteger(start).
    JSTaggedNumber relativeStart = JSTaggedValue::ToInteger(thread, startHandle);
    // 7. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t start = base::NumberHelper::DoubleInRangeInt32(relativeStart.GetNumber());
    int32_t end;
    int32_t first;
    int32_t last;
    // 8. If relativeStart < 0, let first be max((len + relativeStart),0); else let first be min(relativeStart, len).
    if (start < 0) {
        first = std::max((len + start), 0);
    } else {
        first = std::min(start, len);
    }
    // 9. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    JSHandle<JSTaggedValue> endHandle = GetCallArg(argv, 1);
    if (endHandle->IsUndefined()) {
        end = len;
    } else {
        JSTaggedNumber relativeEnd = JSTaggedValue::ToInteger(thread, endHandle);
        // 10. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = base::NumberHelper::DoubleInRangeInt32(relativeEnd.GetNumber());
    }
    // 11. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    if (end < 0) {
        last = std::max((len + end), 0);
    } else {
        last = std::min(end, len);
    }
    // 12. Let newLen be max(final-first,0).
    uint32_t newLen = std::max((last - first), 0);
    // 13. Let ctor be SpeciesConstructor(O, %ArrayBuffer%).
    JSHandle<JSTaggedValue> defaultConstructor = env->GetArrayBufferFunction();
    JSHandle<JSObject> objHandle(thisHandle);
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
    // 14. ReturnIfAbrupt(ctor).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 15. Let new be Construct(ctor, «newLen»).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 1);
    info.SetCallArg(JSTaggedValue(newLen));
    JSTaggedValue taggedNewArrBuf = JSFunction::Construct(&info);
    JSHandle<JSTaggedValue> newArrBuf(thread, taggedNewArrBuf);
    // 16. ReturnIfAbrupt(new).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 17. If new does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!newArrBuf->IsArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have bufferdata internal slot", JSTaggedValue::Exception());
    }
    // 18. If IsDetachedBuffer(new) is true, throw a TypeError exception.
    if (IsDetachedBuffer(newArrBuf.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new arrayBuffer IsDetachedBuffer", JSTaggedValue::Exception());
    }
    // 19. If SameValue(new, O) is true, throw a TypeError exception.
    if (JSTaggedValue::SameValue(newArrBuf.GetTaggedValue(), thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "value of new arraybuffer and this is same", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> newJsArrBuf(newArrBuf);
    // 20. If the value of new’s [[ArrayBufferByteLength]] internal slot < newLen, throw a TypeError exception.
    uint32_t newArrBufLen = newJsArrBuf->GetArrayBufferByteLength();
    if (newArrBufLen < newLen) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new array buffer length smaller than newlen", JSTaggedValue::Exception());
    }
    // 21. NOTE: Side-effects of the above steps may have detached O.
    // 22. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value IsDetachedBuffer", JSTaggedValue::Exception());
    }
    if (newLen > 0) {
        // 23. Let fromBuf be the value of O’s [[ArrayBufferData]] internal slot.
        JSTaggedValue from = arrBuf->GetArrayBufferData();
        // 24. Let toBuf be the value of new’s [[ArrayBufferData]] internal slot.
        JSTaggedValue to = newJsArrBuf->GetArrayBufferData();
        // 25. Perform CopyDataBlockBytes(toBuf, fromBuf, first, newLen).
        JSArrayBuffer::CopyDataBlockBytes(to, from, first, newLen);
    }
    // Return new.
    return newArrBuf.GetTaggedValue();
}

// 24.1.1.1 AllocateArrayBuffer(constructor, byteLength)
JSTaggedValue BuiltinsArrayBuffer::AllocateArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget,
                                                       double byteLength)
{
    BUILTINS_API_TRACE(thread, ArrayBuffer, AllocateArrayBuffer);
    /**
     * 1. Let obj be OrdinaryCreateFromConstructor(constructor, "%ArrayBufferPrototype%",
     * «[[ArrayBufferData]], [[ArrayBufferByteLength]]» ).
     * */
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> arrBufFunc = env->GetArrayBufferFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrBufFunc), newTarget);
    // 2. ReturnIfAbrupt
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Assert: byteLength is a positive integer.
    ASSERT(JSTaggedValue(byteLength).IsInteger());
    ASSERT(byteLength >= 0);
    // 4. Let block be CreateByteDataBlock(byteLength).
    if (byteLength > INT_MAX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Out of range", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> arrayBuffer(obj);
    // 6. Set obj’s [[ArrayBufferData]] internal slot to block.
    factory->NewJSArrayBufferData(arrayBuffer, byteLength);
    // 7. Set obj’s [[ArrayBufferByteLength]] internal slot to byteLength.
    arrayBuffer->SetArrayBufferByteLength(static_cast<uint32_t>(byteLength));
    // 8. Return obj.
    return arrayBuffer.GetTaggedValue();
}

// 24.1.1.2 IsDetachedBuffer()
bool BuiltinsArrayBuffer::IsDetachedBuffer(JSTaggedValue arrayBuffer)
{
    // 1. Assert: Type(arrayBuffer) is Object and it has an [[ArrayBufferData]] internal slot.
    ASSERT(arrayBuffer.IsArrayBuffer());
    JSArrayBuffer *buffer = JSArrayBuffer::Cast(arrayBuffer.GetTaggedObject());
    JSTaggedValue dataSlot = buffer->GetArrayBufferData();
    // 2. If arrayBuffer’s [[ArrayBufferData]] internal slot is null, return true.
    // 3. Return false.
    return dataSlot == JSTaggedValue::Null();
}

// 24.1.1.4
JSTaggedValue BuiltinsArrayBuffer::CloneArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &srcBuffer,
                                                    uint32_t srcByteOffset, JSHandle<JSTaggedValue> constructor)
{
    BUILTINS_API_TRACE(thread, ArrayBuffer, CloneArrayBuffer);
    // 1. Assert: Type(srcBuffer) is Object and it has an [[ArrayBufferData]] internal slot.
    ASSERT(srcBuffer->IsArrayBuffer());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 2. If cloneConstructor is not present
    if (constructor->IsUndefined()) {
        // a. Let cloneConstructor be SpeciesConstructor(srcBuffer, %ArrayBuffer%).
        JSHandle<JSTaggedValue> defaultConstructor = env->GetArrayBufferFunction();
        JSHandle<JSObject> objHandle(srcBuffer);
        constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
        // b. ReturnIfAbrupt(cloneConstructor).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
        if (IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
        } else {
            ASSERT(constructor->IsConstructor());
        }
    }
    // 4. Let srcLength be the value of srcBuffer’s [[ArrayBufferByteLength]] internal slot.
    JSHandle<JSArrayBuffer> arrBuf(srcBuffer);
    uint32_t srcLen = arrBuf->GetArrayBufferByteLength();
    // 5. Assert: srcByteOffset ≤ srcLength.
    ASSERT(srcByteOffset <= srcLen);
    // 6. Let cloneLength be srcLength – srcByteOffset.
    int32_t cloneLen = static_cast<int32_t>(srcLen - srcByteOffset);
    // 8. Let targetBuffer be AllocateArrayBuffer(cloneConstructor, cloneLength).
    JSTaggedValue taggedBuf = AllocateArrayBuffer(thread, constructor, cloneLen);
    // 9. ReturnIfAbrupt(targetBuffer).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 10. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
    if (IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 11. Let targetBlock be the value of targetBuffer’s [[ArrayBufferData]] internal slot.
    JSHandle<JSArrayBuffer> newArrBuf(thread, taggedBuf);
    // Perform CopyDataBlockBytes(targetBlock, 0, srcBlock, srcByteOffset, cloneLength).
    // 7. Let srcBlock be the value of srcBuffer’s [[ArrayBufferData]] internal slot.
    JSTaggedValue srcBlock = arrBuf->GetArrayBufferData();
    JSTaggedValue targetBlock = newArrBuf->GetArrayBufferData();
    if (cloneLen > 0) {
        JSArrayBuffer::CopyDataBlockBytes(targetBlock, srcBlock, srcByteOffset, cloneLen);
    }
    return taggedBuf;
}

// 24.1.1.5
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBuffer(JSThread *thread, JSTaggedValue arrBuf, uint32_t byteIndex,
                                                      DataViewType type, bool littleEndian)
{
    JSArrayBuffer *jsArrayBuffer = JSArrayBuffer::Cast(arrBuf.GetTaggedObject());
    JSTaggedValue data = jsArrayBuffer->GetArrayBufferData();
    void *pointer = JSNativePointer::Cast(data.GetTaggedObject())->GetExternalPointer();
    auto *block = reinterpret_cast<uint8_t *>(pointer);
    switch (type) {
        case DataViewType::UINT8:
        case DataViewType::UINT8_CLAMPED: {
            uint8_t res = block[byteIndex];  // NOLINT
            return GetTaggedInt(res);
        }
        case DataViewType::INT8: {
            uint8_t res = block[byteIndex];  // NOLINT
            auto int8Res = static_cast<int8_t>(res);
            return GetTaggedInt(int8Res);
        }
        case DataViewType::UINT16:
            return GetValueFromBufferForInteger<uint16_t, NumberSize::UINT16>(block, byteIndex, littleEndian);
        case DataViewType::INT16:
            return GetValueFromBufferForInteger<int16_t, NumberSize::INT16>(block, byteIndex, littleEndian);
        case DataViewType::UINT32:
            return GetValueFromBufferForInteger<uint32_t, NumberSize::UINT32>(block, byteIndex, littleEndian);
        case DataViewType::INT32:
            return GetValueFromBufferForInteger<int32_t, NumberSize::INT32>(block, byteIndex, littleEndian);
        case DataViewType::FLOAT32:
            return GetValueFromBufferForFloat<float, UnionType32, NumberSize::FLOAT32>(block, byteIndex, littleEndian);
        case DataViewType::FLOAT64:
            return GetValueFromBufferForFloat<double, UnionType64, NumberSize::FLOAT64>(block, byteIndex, littleEndian);
        case DataViewType::BIGINT64:
            return GetValueFromBufferForBigInt(thread, block, byteIndex);
        case DataViewType::BIGUINT64:
            return GetValueFromBufferForBigUint(thread, block, byteIndex);
        default:
            break;
    }

    UNREACHABLE();
}

// 24.1.1.6
JSTaggedValue BuiltinsArrayBuffer::SetValueInBuffer(JSThread *thread, JSTaggedValue arrBuf, uint32_t byteIndex,
                                                    DataViewType type, const JSHandle<JSTaggedValue> &value,
                                                    bool littleEndian)
{
    JSArrayBuffer *jsArrayBuffer = JSArrayBuffer::Cast(arrBuf.GetTaggedObject());
    JSTaggedValue data = jsArrayBuffer->GetArrayBufferData();
    void *pointer = JSNativePointer::Cast(data.GetTaggedObject())->GetExternalPointer();
    auto *block = reinterpret_cast<uint8_t *>(pointer);
    if (IsBigIntElementType(type)) {
        switch (type) {
            case DataViewType::BIGINT64:
                SetValueInBufferForBigInt(thread, value, block, byteIndex, false);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                break;
            case DataViewType::BIGUINT64:
                SetValueInBufferForBigInt(thread, value, block, byteIndex, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                break;
            default:
                UNREACHABLE();
        }
        return JSTaggedValue::Undefined();
    }
    double val = value.GetTaggedValue().GetNumber();
    switch (type) {
        case DataViewType::UINT8:
            SetValueInBufferForByte<uint8_t>(val, block, byteIndex);
            break;
        case DataViewType::UINT8_CLAMPED:
            SetValueInBufferForUint8Clamped(val, block, byteIndex);
            break;
        case DataViewType::INT8:
            SetValueInBufferForByte<int8_t>(val, block, byteIndex);
            break;
        case DataViewType::UINT16:
            SetValueInBufferForInteger<uint16_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::INT16:
            SetValueInBufferForInteger<int16_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::UINT32:
            SetValueInBufferForInteger<uint32_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::INT32:
            SetValueInBufferForInteger<int32_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::FLOAT32:
            SetValueInBufferForFloat<float>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::FLOAT64:
            SetValueInBufferForFloat<double>(val, block, byteIndex, littleEndian);
            break;
        default:
            UNREACHABLE();
    }
    return JSTaggedValue::Undefined();
}

// es12 25.1.2.7 IsBigIntElementType ( type )
bool BuiltinsArrayBuffer::IsBigIntElementType(DataViewType type)
{
    if (type == DataViewType::BIGINT64 || type == DataViewType::BIGUINT64) {
        return true;
    }
    return false;
}

template<typename T>
void BuiltinsArrayBuffer::SetTypeData(uint8_t *block, T value, uint32_t index)
{
    uint32_t sizeCount = 0;
    uint8_t *res = nullptr;
    if constexpr (std::is_same_v<T, uint32_t*>) {
        sizeCount = sizeof(uint32_t) * 2; // 2:Array occupies 8 bytes
        res = reinterpret_cast<uint8_t *>(value);
    } else {
        sizeCount = sizeof(T);
        res = reinterpret_cast<uint8_t *>(&value);
    }
    for (uint32_t i = 0; i < sizeCount; i++) {
        *(block + index + i) = *(res + i);  // NOLINT
    }
}

template <typename T>
T BuiltinsArrayBuffer::LittleEndianToBigEndian(T liValue)
{
    uint8_t sizeCount = sizeof(T);
    T biValue;
    switch (sizeCount) {
        case NumberSize::UINT16:
            biValue = ((liValue & 0x00FF) << BITS_EIGHT)     // NOLINT
                      | ((liValue & 0xFF00) >> BITS_EIGHT);  // NOLINT
            break;
        case NumberSize::UINT32:
            biValue = ((liValue & 0x000000FF) << BITS_TWENTY_FOUR)     // NOLINT
                      | ((liValue & 0x0000FF00) << BITS_EIGHT)         // NOLINT
                      | ((liValue & 0x00FF0000) >> BITS_EIGHT)         // NOLINT
                      | ((liValue & 0xFF000000) >> BITS_TWENTY_FOUR);  // NOLINT
            break;
        default:
            UNREACHABLE();
            break;
    }
    return biValue;
}

uint64_t BuiltinsArrayBuffer::LittleEndianToBigEndianUint64(uint64_t liValue)
{
    return ((liValue & 0x00000000000000FF) << BITS_FIFTY_SIX)      // NOLINT
           | ((liValue & 0x000000000000FF00) << BITS_FORTY)        // NOLINT
           | ((liValue & 0x0000000000FF0000) << BITS_TWENTY_FOUR)  // NOLINT
           | ((liValue & 0x00000000FF000000) << BITS_EIGHT)        // NOLINT
           | ((liValue & 0x000000FF00000000) >> BITS_EIGHT)        // NOLINT
           | ((liValue & 0x0000FF0000000000) >> BITS_TWENTY_FOUR)  // NOLINT
           | ((liValue & 0x00FF000000000000) >> BITS_FORTY)        // NOLINT
           | ((liValue & 0xFF00000000000000) >> BITS_FIFTY_SIX);   // NOLINT
}

template<typename T, BuiltinsArrayBuffer::NumberSize size>
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForInteger(uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    static_assert(std::is_integral_v<T>, "T must be integral");
    static_assert(sizeof(T) == size, "Invalid number size");
    static_assert(sizeof(T) >= sizeof(uint16_t), "T must have a size more than uint8");

    ASSERT(size >= NumberSize::UINT16 || size <= NumberSize::FLOAT64);
    T res = *reinterpret_cast<T *>(block + byteIndex);
    if (!littleEndian) {
        res = LittleEndianToBigEndian(res);
    }

    // uint32_t maybe overflow with TaggedInt
    // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
    if constexpr (std::is_same_v<T, uint32_t>) {
        // NOLINTNEXTLINE(clang-diagnostic-sign-compare)
        if (res > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
            return GetTaggedDouble(static_cast<double>(res));
        }
    }
    return GetTaggedInt(res);
}

template<typename T, typename UnionType, BuiltinsArrayBuffer::NumberSize size>
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForFloat(uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>, "T must be correct type");
    static_assert(sizeof(T) == size, "Invalid number size");

    UnionType unionValue = {0};
    // NOLINTNEXTLINE(readability-braces-around-statements)
    if constexpr (std::is_same_v<T, float>) {
        unionValue.uValue = *reinterpret_cast<uint32_t *>(block + byteIndex);
        if (std::isnan(unionValue.value)) {
            return GetTaggedDouble(unionValue.value);
        }
        if (!littleEndian) {
            uint32_t res = LittleEndianToBigEndian(unionValue.uValue);
            return GetTaggedDouble(bit_cast<T>(res));
        }
    } else if constexpr (std::is_same_v<T, double>) {  // NOLINTNEXTLINE(readability-braces-around-statements)
        unionValue.uValue = *reinterpret_cast<uint64_t *>(block + byteIndex);
        if (std::isnan(unionValue.value) && !JSTaggedValue::IsImpureNaN(unionValue.value)) {
            return GetTaggedDouble(unionValue.value);
        }
        if (!littleEndian) {
            uint64_t res = LittleEndianToBigEndianUint64(unionValue.uValue);
            return GetTaggedDouble(bit_cast<T>(res));
        }
    }

    return GetTaggedDouble(unionValue.value);
}

JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForBigInt(JSThread *thread, uint8_t *block, uint32_t byteIndex)
{
    uint32_t *pTmp = reinterpret_cast<uint32_t *>(block + byteIndex);
    JSHandle<BigInt> valBigInt = BigInt::CreateBigint(thread, 2); // 2:Create a bigint of 2 elements
    BigInt::SetDigit(thread, valBigInt, 0, *pTmp);
    uint32_t tmp = *(pTmp + 1);
    if (tmp >> BITS_THIRTY_ONE) {
        valBigInt->SetSign(true);
    }
    if ((*pTmp == 0) && (tmp >> BITS_THIRTY_ONE)) {
        BigInt::SetDigit(thread, valBigInt, 1, tmp);
    } else {
        BigInt::SetDigit(thread, valBigInt, 1, tmp & ~(1 << BITS_THIRTY_ONE));
    }
    BigIntHelper::RightTruncate(thread, valBigInt);
    return valBigInt.GetTaggedValue();
}

JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForBigUint(JSThread *thread, uint8_t *block, uint32_t byteIndex)
{
    uint32_t *pTmp = reinterpret_cast<uint32_t *>(block + byteIndex);
    JSHandle<BigInt> valBigInt = BigInt::CreateBigint(thread, 2); // 2:2 elements
    BigInt::SetDigit(thread, valBigInt, 0, *pTmp);
    BigInt::SetDigit(thread, valBigInt, 1, *(pTmp + 1));
    BigIntHelper::RightTruncate(thread, valBigInt);
    return valBigInt.GetTaggedValue();
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForByte(double val, uint8_t *block, uint32_t byteIndex)
{
    static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>, "T must be int8/uint8");
    T res;
    if (std::isnan(val) || std::isinf(val)) {
        res = 0;
        SetTypeData(block, res, byteIndex);
        return;
    }
    auto int64Val = static_cast<int64_t>(val);
    auto *resArr = reinterpret_cast<T *>(&int64Val);
    res = *resArr;
    SetTypeData(block, res, byteIndex);
}

void BuiltinsArrayBuffer::SetValueInBufferForUint8Clamped(double val, uint8_t *block, uint32_t byteIndex)
{
    uint8_t res;
    if (std::isnan(val) || val <= 0) {
        res = 0;
        SetTypeData(block, res, byteIndex);
        return;
    }
    val = val >= UINT8_MAX ? UINT8_MAX : val;
    constexpr double HALF = 0.5;
    val = val == HALF ? 0 : std::round(val);
    res = static_cast<uint64_t>(val);
    SetTypeData(block, res, byteIndex);
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForInteger(double val, uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    static_assert(std::is_integral_v<T>, "T must be integral");
    static_assert(sizeof(T) >= sizeof(uint16_t), "T must have a size more than uint8");
    T res;
    if (std::isnan(val) || std::isinf(val)) {
        res = 0;
        SetTypeData(block, res, byteIndex);
        return;
    }
    auto int64Val = static_cast<int64_t>(val);
    // NOLINTNEXTLINE(readability-braces-around-statements)
    if constexpr (std::is_same_v<T, uint16_t>) {
        auto *pTmp = reinterpret_cast<int16_t *>(&int64Val);
        int16_t tmp = *pTmp;
        res = static_cast<T>(tmp);
    } else {  // NOLINTNEXTLINE(readability-braces-around-statements)
        auto *pTmp = reinterpret_cast<T *>(&int64Val);
        res = *pTmp;
    }

    if (!littleEndian) {
        res = LittleEndianToBigEndian<T>(res);
    }
    SetTypeData(block, res, byteIndex);
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForFloat(double val, uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>, "T must be float type");
    auto data = static_cast<T>(val);
    if (std::isnan(val)) {
        SetTypeData(block, data, byteIndex);
        return;
    }
    if (!littleEndian) {
        if constexpr (std::is_same_v<T, float>) {
            uint32_t res = bit_cast<uint32_t>(data);
            data = bit_cast<T>(LittleEndianToBigEndian(res));
        } else if constexpr (std::is_same_v<T, double>) {
            uint64_t res = bit_cast<uint64_t>(data);
            data = bit_cast<T>(LittleEndianToBigEndianUint64(res));
        }
    }
    SetTypeData(block, data, byteIndex);
}

JSTaggedValue BuiltinsArrayBuffer::SetValueInBufferForBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &val,
                                                             uint8_t *block, uint32_t byteIndex, bool isUint)
{
    JSHandle<BigInt> valBigintHandle;
    if (isUint) {
        valBigintHandle = JSHandle<BigInt>(thread, val->ToBigUint64(thread, val));
    } else {
        valBigintHandle = JSHandle<BigInt>(thread, val->ToBigInt64(thread, val));
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t valLength = valBigintHandle->GetLength();
    uint32_t varBuffer[2] = {0}; // 2:2 elements with 8 bits
    for (uint32_t i = 0; i < valLength; i++) {
        varBuffer[i] = valBigintHandle->GetDigit(i);
    }
    if (!isUint) {
        bool sign = valBigintHandle->GetSign();
        varBuffer[1] = sign ? (varBuffer[1] |= 1 << BITS_THIRTY_ONE) : varBuffer[1];
    }
    SetTypeData(block, varBuffer, byteIndex);
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::builtins
