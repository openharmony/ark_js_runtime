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

#include "ecmascript/builtins/builtins_atomics.h"

#include "ecmascript/base/atomic_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "utils/time.h"

namespace panda::ecmascript::builtins {
using NumberHelper = base::NumberHelper;
using AtomicHelper = base::AtomicHelper;
using BytesSize = base::BytesSize;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;

WaiterList *g_waitLists = Singleton<WaiterList>::GetInstance();
Mutex *g_mutex = Singleton<Mutex>::GetInstance();

// 25.4.2 Atomics.add ( typedArray, index, value )
JSTaggedValue BuiltinsAtomics::Sub(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Sub);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::SubFun());
}

JSTaggedValue BuiltinsAtomics::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::AddFun());
}

JSTaggedValue BuiltinsAtomics::And(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, And);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::AndFun());
}

JSTaggedValue BuiltinsAtomics::Or(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Or);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::OrFun());
}

JSTaggedValue BuiltinsAtomics::Xor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Xor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::XorFun());
}

JSTaggedValue BuiltinsAtomics::CompareExchange(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, CompareExchange);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::CompareExchangeFun());
}

JSTaggedValue BuiltinsAtomics::Exchange(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Exchange);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicReadModifyWrite(thread, typedArray, index, argv, AtomicHelper::ExchangeFun());
}

JSTaggedValue BuiltinsAtomics::Store(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Store);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    return AtomicHelper::AtomicStore(thread, typedArray, index, value);
}

JSTaggedValue BuiltinsAtomics::Load(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Load);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> typedArray = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    return AtomicHelper::AtomicLoad(thread, typedArray, index);
}

JSTaggedValue BuiltinsAtomics::IsLockFree(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, IsLockFree);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> sizeTag = GetCallArg(argv, 0);
    BytesSize size = BytesSize(JSTaggedValue::ToInt32(thread, sizeTag));
    bool result;
    switch (size) {
        case BytesSize::ONEBYTES:
        case BytesSize::TWOBYTES:
        case BytesSize::FOURBYTES:
        case BytesSize::EIGHTBYTES:
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return JSTaggedValue(result);
}

// 25.4.11 Atomics.wait ( typedArray, index, value, timeout )
JSTaggedValue BuiltinsAtomics::Wait(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Wait);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> array = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    JSHandle<JSTaggedValue> timeout = GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);

    // 1. Let buffer be ? ValidateIntegerTypedArray(typedArray, true).
    JSHandle<JSTaggedValue> arrayBuffer(thread, AtomicHelper::ValidateIntegerTypedArray(thread, array, true));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. If IsSharedArrayBuffer(buffer) is false, throw a TypeError exception.
    if (!arrayBuffer->IsSharedArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "buffer is not  sharedArrayBuffer.",
                                    JSTaggedValue::Exception());
    }

    // 3. Let indexedPosition be ? ValidateAtomicAccess(typedArray, index).
    uint32_t indexedPosition = AtomicHelper::ValidateAtomicAccess(thread, array, index);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. If typedArray.[[TypedArrayName]] is "BigInt64Array", let v be ? ToBigInt64(value).
    // 5. Otherwise, let v be ? ToInt32(value).
    int64_t v = 0;
    if (array->IsJSBigInt64Array()) {
        v = JSHandle<BigInt>::Cast(value)->ToInt64();
    } else {
        v = static_cast<int64_t>(JSTaggedValue::ToInt32(thread, value));
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. Let q be ? ToNumber(timeout).
    // 7. If q is NaN or +∞𝔽, let t be +∞; else if q is -∞𝔽, let t be 0; else let t be max(ℝ(q), 0).
    double t = 0;
    if (timeout->IsUndefined()) {
        t = base::POSITIVE_INFINITY;
    } else {
        JSTaggedNumber q = JSTaggedValue::ToNumber(thread, timeout);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        t = q.GetNumber();
        if (NumberHelper::IsNaN(q) || (!NumberHelper::IsFinite(q) && t > 0)) {
            t = base::POSITIVE_INFINITY;
        } else if (t < 0) {
            t = 0;
        }
    }

    // 8. Let B be AgentCanSuspend().
    // 9. If B is false, throw a TypeError exception.
    if (!thread->GetEcmaVM()->GetAllowAtomicWait()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "vm does not allow wait to block.",
                                    JSTaggedValue::Exception());
    }
    WaitResult res = WaitResult::OK;
    if (array->IsJSBigInt64Array()) {
        // AtomicHelper::Wait<int64_t>(thread, arrayBuffer, indexedPosition, v, t);
        res = DoWait<int64_t>(thread, arrayBuffer, indexedPosition, v, t);
    } else {
        // AtomicHelper::Wait<int32_t>(thread, arrayBuffer, indexedPosition, static_cast<int32_t>(v), t);
        res = DoWait<int32_t>(thread, arrayBuffer, indexedPosition, static_cast<int32_t>(v), t);
    }
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (res == WaitResult::OK) {
        return globalConst->GetOkString();
    } else if (res == WaitResult::NOT_EQ) {
        return globalConst->GetNotEqualString();
    }
    return globalConst->GetTimeoutString();
}

// 25.4.12 Atomics.notify ( typedArray, index, count )
JSTaggedValue BuiltinsAtomics::Notify(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Atomics, Notify);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> array = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> count = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);

    // 1. Let buffer be ? ValidateIntegerTypedArray(typedArray, true).
    JSHandle<JSTaggedValue> arrayBuffer(thread, AtomicHelper::ValidateIntegerTypedArray(thread, array, true));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let indexedPosition be ? ValidateAtomicAccess(typedArray, index).
    uint32_t indexedPosition = AtomicHelper::ValidateAtomicAccess(thread, array, index);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If count is undefined, let c be +∞.
    // 4. Else,
    // a. Let intCount be ? ToIntegerOrInfinity(count).
    // b. Let c be max(intCount, 0).
    double c = 0;
    if (count->IsUndefined()) {
        c = base::POSITIVE_INFINITY;
    } else {
        JSTaggedNumber countTemp = JSTaggedValue::ToNumber(thread, count);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        c = base::NumberHelper::TruncateDouble(countTemp.GetNumber());
        c = c < 0 ? 0 : c;
    }
    // 6. If IsSharedArrayBuffer(buffer) is false, return +0𝔽.
    if (!arrayBuffer->IsSharedArrayBuffer()) {
        return JSTaggedValue(0);
    }
    return JSTaggedValue(Signal(arrayBuffer, indexedPosition, c));
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::AtomicReadModifyWrite(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                                     JSHandle<JSTaggedValue> &index, EcmaRuntimeCallInfo *argv,
                                                     const callbackfun &op)
{
    if (!typedArray->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    // 1. Let buffer be ? ValidateIntegerTypedArray(typedArray).
    JSTaggedValue bufferValue = base::AtomicHelper::ValidateIntegerTypedArray(thread, typedArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> buffer(thread, bufferValue);
    // 2. Let indexedPosition be ? ValidateAtomicAccess(typedArray, index).
    uint32_t indexedPosition = base::AtomicHelper::ValidateAtomicAccess(thread, typedArray, index);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Let arrayTypeName be typedArray.[[TypedArrayName]].
    JSHandle<JSTaggedValue> arrayTypeName(thread,
                                          JSTypedArray::Cast(typedArray->GetTaggedObject())->GetTypedArrayName());
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ArrayBuffer of this value is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    // 7. NOTE: The above check is not redundant with the check in ValidateIntegerTypedArray because the call to
    // ToBigInt or ToIntegerOrInfinity on the preceding lines can have arbitrary side effects, which could cause the
    // buffer to become detached.
    // 8. Let elementType be the Element Type value in Table 60 for arrayTypeName.
    DataViewType elementType = JSTypedArray::GetTypeFromName(thread, arrayTypeName);
    // 9. Return GetModifySetValueInBuffer(buffer, indexedPosition, elementType, v, op).
    return AtomicReadModifyWriteCase(thread, buffer.GetTaggedValue(), elementType, indexedPosition, argv, op);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::AtomicReadModifyWriteCase(JSThread *thread, JSTaggedValue arrBuf,
                                                         DataViewType type, uint32_t indexedPosition,
                                                         EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSArrayBuffer *jsArrayBuffer = JSArrayBuffer::Cast(arrBuf.GetTaggedObject());
    JSTaggedValue data = jsArrayBuffer->GetArrayBufferData();
    void *pointer = JSNativePointer::Cast(data.GetTaggedObject())->GetExternalPointer();
    auto *block = reinterpret_cast<uint8_t *>(pointer);
    uint32_t size = argv->GetArgsNumber();
    switch (type) {
        case DataViewType::UINT8:
            return HandleWithUint8(thread, size, block, indexedPosition, argv, op);
        case DataViewType::INT8:
            return HandleWithInt8(thread, size, block, indexedPosition, argv, op);
        case DataViewType::UINT16:
            return HandleWithUint16(thread, size, block, indexedPosition, argv, op);
        case DataViewType::INT16:
            return HandleWithInt16(thread, size, block, indexedPosition, argv, op);
        case DataViewType::UINT32:
            return HandleWithUint32(thread, size, block, indexedPosition, argv, op);
        case DataViewType::INT32:
            return HandleWithInt32(thread, size, block, indexedPosition, argv, op);
        case DataViewType::BIGINT64:
            return HandleWithBigInt64(thread, size, block, indexedPosition, argv, op);
        case DataViewType::BIGUINT64:
            return HandleWithBigUint64(thread, size, block, indexedPosition, argv, op);
        default:
            break;
    }
    UNREACHABLE();
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithUint8(JSThread *thread, uint32_t size, uint8_t *block,
                                               uint32_t indexedPosition,
                                               EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    uint8_t tag = JSTaggedValue::ToUint8(thread, value);
    if (size == 3) { // the number of parameters is 3
        auto result = op((block + indexedPosition), &tag);
        return BuiltinsBase::GetTaggedInt(result);
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    uint8_t newTag = JSTaggedValue::ToUint8(thread, newValue);
    uint8_t arg[ARGS_NUMBER] = {0};
    arg[0] = tag;
    arg[1] = newTag;
    auto result = op((block + indexedPosition), arg);
    return JSTaggedValue(result);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithInt8(JSThread *thread, uint32_t size, uint8_t *block,
                                              uint32_t indexedPosition,
                                              EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    int8_t tag = JSTaggedValue::ToInt8(thread, value);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<int8_t *>(block + indexedPosition), &tag);
        return BuiltinsBase::GetTaggedInt(result);
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    int8_t newTag = JSTaggedValue::ToInt8(thread, newValue);
    int8_t arg[ARGS_NUMBER] = {0};
    arg[0] = tag;
    arg[1] = newTag;
    auto result = op(reinterpret_cast<int8_t *>(block + indexedPosition), arg);
    return JSTaggedValue(result);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithUint16(JSThread *thread, uint32_t size, uint8_t *block,
                                                uint32_t indexedPosition,
                                                EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    uint16_t tag = JSTaggedValue::ToUint16(thread, value);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<uint16_t *>(block + indexedPosition), &tag);
        return BuiltinsBase::GetTaggedInt(result);
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    uint16_t newTag = JSTaggedValue::ToUint16(thread, newValue);
    uint16_t arg[ARGS_NUMBER] = {0};
    arg[0] = tag;
    arg[1] = newTag;
    auto result = op(reinterpret_cast<uint16_t *>(block + indexedPosition), arg);
    return JSTaggedValue(result);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithInt16(JSThread *thread, uint32_t size, uint8_t *block,
                                               uint32_t indexedPosition,
                                               EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    int16_t tag = JSTaggedValue::ToInt16(thread, value);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<int16_t *>(block + indexedPosition), &tag);
        return BuiltinsBase::GetTaggedInt(result);
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    int16_t newTag = JSTaggedValue::ToInt16(thread, newValue);
    int16_t arg[ARGS_NUMBER] = {0};
    arg[0] = tag;
    arg[1] = newTag;
    auto result = op(reinterpret_cast<int16_t *>(block + indexedPosition), arg);
    return JSTaggedValue(result);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithUint32(JSThread *thread, uint32_t size, uint8_t *block,
                                                uint32_t indexedPosition,
                                                EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    uint32_t tag = JSTaggedValue::ToUint32(thread, value);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<uint32_t *>(block + indexedPosition), &tag);
        return JSTaggedValue(result);
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    uint32_t newTag = JSTaggedValue::ToUint32(thread, newValue);
    uint32_t arg[ARGS_NUMBER] = {0};
    arg[0] = tag;
    arg[1] = newTag;
    auto result = op(reinterpret_cast<uint32_t *>(block + indexedPosition), arg);
    return JSTaggedValue(result);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithInt32(JSThread *thread, uint32_t size, uint8_t *block,
                                               uint32_t indexedPosition,
                                               EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    int32_t tag = JSTaggedValue::ToInt32(thread, value);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<int32_t *>(block + indexedPosition), &tag);
        return BuiltinsBase::GetTaggedInt(result);
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    int32_t newTag = JSTaggedValue::ToInt32(thread, newValue);
    int32_t arg[ARGS_NUMBER] = {0};
    arg[0] = tag;
    arg[1] = newTag;
    auto result = op(reinterpret_cast<int32_t *>(block + indexedPosition), arg);
    return JSTaggedValue(result);
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithBigInt64(JSThread *thread, uint32_t size, uint8_t *block,
                                                  uint32_t indexedPosition,
                                                  EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    int64_t val = 0;
    bool lossless = true;
    BigInt::BigIntToInt64(thread, value, &val, &lossless);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<int64_t *>(block + indexedPosition), &val);
        return BigInt::Int64ToBigInt(thread, result).GetTaggedValue();
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    int64_t newVal = 0;
    BigInt::BigIntToInt64(thread, newValue, &newVal, &lossless);
    int64_t arg[ARGS_NUMBER] = {0};
    arg[0] = val;
    arg[1] = newVal;
    auto result = op(reinterpret_cast<int64_t *>(block + indexedPosition), arg);
    return BigInt::Int64ToBigInt(thread, result).GetTaggedValue();
}

template<typename callbackfun>
JSTaggedValue BuiltinsAtomics::HandleWithBigUint64(JSThread *thread, uint32_t size, uint8_t *block,
                                                   uint32_t indexedPosition,
                                                   EcmaRuntimeCallInfo *argv, const callbackfun &op)
{
    JSHandle<JSTaggedValue> value = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    uint64_t val = 0;
    bool lossless = true;
    BigInt::BigIntToUint64(thread, value, &val, &lossless);
    if (size == 3) { // the number of parameters is 3
        auto result = op(reinterpret_cast<uint64_t *>(block + indexedPosition), &val);
        return BigInt::Uint64ToBigInt(thread, result).GetTaggedValue();
    }
    JSHandle<JSTaggedValue> newValue = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::FOURTH);
    uint64_t newVal = 0;
    BigInt::BigIntToUint64(thread, newValue, &newVal, &lossless);
    uint64_t arg[ARGS_NUMBER] = {0};
    arg[0] = val;
    arg[1] = newVal;
    auto result = op(reinterpret_cast<uint64_t *>(block + indexedPosition), arg);
    return BigInt::Uint64ToBigInt(thread, result).GetTaggedValue();
}

template <typename T>
WaitResult BuiltinsAtomics::DoWait(JSThread *thread, JSHandle<JSTaggedValue> &arrayBuffer,
                                   size_t index, T execpt, double timeout)
{
    MutexGuard lock_guard(g_mutex);
    JSHandle<JSNativePointer> np(thread, JSHandle<JSArrayBuffer>::Cast(arrayBuffer)->GetArrayBufferData());
    void *buffer = np->GetExternalPointer();
    ASSERT(buffer != nullptr);
    WaiterListNode *node = thread->GetEcmaVM()->GetWaiterListNode();
    node->date_ = buffer;
    node->index_ = index;
    node->waitPointer_ = reinterpret_cast<int8_t*>(buffer) + index;
    node->waiting_ = true;
    std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(node->waitPointer_);
    T value = atomicValue->load();
    if (value != execpt) {
        return WaitResult::NOT_EQ;
    }
    g_waitLists->AddNode(node);
    uint64_t currentTime = 0;
    uint64_t timeoutTime = 0;
    bool hasTimeout = timeout != base::POSITIVE_INFINITY;
    if (hasTimeout) {
        currentTime = time::GetCurrentTimeInMillis();
        timeoutTime = currentTime + static_cast<uint64_t>(timeout);
    }
    WaitResult res = WaitResult::OK;
    while (true) {
        if (!node->waiting_) {
            res = WaitResult::OK;
            break;
        }
        if (hasTimeout) {
            currentTime = time::GetCurrentTimeInMillis();
            if (currentTime >= timeoutTime) {
                res = WaitResult::TIME_OUT;
                break;
            }
            uint64_t untilTime = timeoutTime - currentTime;
            ASSERT(untilTime != 0);

            node->cond_.TimedWait(g_mutex, untilTime);
        } else {
            node->cond_.Wait(g_mutex);
        }
    }
    g_waitLists->DeleteNode(node);
    node->waiting_ = false;
    return res;
}

uint32_t BuiltinsAtomics::Signal(JSHandle<JSTaggedValue> &arrayBuffer, const size_t &index, double wakeCount)
{
    JSTaggedValue data = JSHandle<JSArrayBuffer>::Cast(arrayBuffer)->GetArrayBufferData();
    void *buffer = JSNativePointer::Cast(data.GetTaggedObject())->GetExternalPointer();
    ASSERT(buffer != nullptr);
    MutexGuard lock_guard(g_mutex);
    auto &locationListMap = g_waitLists->locationListMap_;
    auto iter = locationListMap.find(reinterpret_cast<int8_t *>(buffer) + index);
    if (iter == locationListMap.end()) {
        return 0;
    }
    WaiterListNode *node = iter->second.pHead;
    uint32_t wokenUpCount = 0;
    while (node != nullptr && wakeCount > 0.0) {
        if (!node->waiting_) {
            node = node->next_;
            continue;
        }
        if (buffer == node->date_) {
            ASSERT(index == node->index_);
            node->waiting_ = false;

            WaiterListNode *oldNode = node;
            node = node->next_;
            oldNode->cond_.Signal();
            if (wakeCount != base::POSITIVE_INFINITY) {
                wakeCount--;
            }
            wokenUpCount++;
            continue;
        }
        node = node->next_;
    }
    return wokenUpCount;
}
} // namespace panda::ecmascript::builtins
