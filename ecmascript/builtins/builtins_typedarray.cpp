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

#include "ecmascript/builtins/builtins_typedarray.h"
#include <cmath>
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
using TypedArrayHelper = base::TypedArrayHelper;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;

// 22.2.1
JSTaggedValue BuiltinsTypedArray::TypedArrayBaseConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, BaseConstructor);
    THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "TypedArray Constructor cannot be called.",
                                JSTaggedValue::Exception());
}

JSTaggedValue BuiltinsTypedArray::Int8ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledInt8ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Uint8ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledUint8ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Uint8ClampedArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv,
                                                   thread->GlobalConstants()->GetHandledUint8ClampedArrayString());
}

JSTaggedValue BuiltinsTypedArray::Int16ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledInt16ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Uint16ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledUint16ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Int32ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledInt32ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Uint32ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledUint32ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Float32ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledFloat32ArrayString());
}

JSTaggedValue BuiltinsTypedArray::Float64ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledFloat64ArrayString());
}

JSTaggedValue BuiltinsTypedArray::BigInt64ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledBigInt64ArrayString());
}

JSTaggedValue BuiltinsTypedArray::BigUint64ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::TypedArrayConstructor(argv, thread->GlobalConstants()->GetHandledBigUint64ArrayString());
}

// 22.2.2.1 %TypedArray%.from ( source [ , mapfn [ , thisArg ] ] )
JSTaggedValue BuiltinsTypedArray::From(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, From);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Let C be the this value.
    // 2. If IsConstructor(C) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the this value is not a Constructor.", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    // 3. If mapfn is undefined, let mapping be false.
    // 4. Else,
    //   a. If IsCallable(mapfn) is false, throw a TypeError exception.
    //   b. Let mapping be true.
    bool mapping = false;
    JSHandle<JSTaggedValue> mapfn = GetCallArg(argv, 1);
    if (!mapfn->IsUndefined()) {
        if (!mapfn->IsCallable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "the mapfn is not callable.", JSTaggedValue::Exception());
        }
        mapping = true;
    }
    // 5. Let usingIterator be ? GetMethod(source, @@iterator).
    JSHandle<JSTaggedValue> source = GetCallArg(argv, 0);
    if (!source->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the source is not an object.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> sourceObj(source);
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> usingIterator =
        JSObject::GetMethod(thread, JSHandle<JSTaggedValue>::Cast(sourceObj), iteratorSymbol);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. If usingIterator is not undefined, then
    //   a. Let values be ? IterableToList(source, usingIterator).
    //   b. Let len be the number of elements in values.
    //   c. Let targetObj be ? TypedArrayCreate(C, « len »).
    if (!usingIterator->IsUndefined()) {
        CVector<JSHandle<JSTaggedValue>> vec;
        JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, source, usingIterator);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> next(thread, JSTaggedValue::True());
        while (!next->IsFalse()) {
            next = JSIterator::IteratorStep(thread, iterator);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!next->IsFalse()) {
                JSHandle<JSTaggedValue> nextValue = JSIterator::IteratorValue(thread, next);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, nextValue.GetTaggedValue());
                vec.push_back(nextValue);
            }
        }
        uint32_t len = vec.size();
        JSTaggedType args[1] = {JSTaggedValue(len).GetRawData()};
        JSHandle<JSObject> targetObj = TypedArrayHelper::TypedArrayCreate(thread, thisHandle, 1, args);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        //   d. Let k be 0.
        //   e. Repeat, while k < len
        //     i. Let Pk be ! ToString(k).
        //     ii. Let kValue be the first element of values and remove that element from values.
        //     iii. If mapping is true, then
        //       1. Let mappedValue be ? Call(mapfn, thisArg, « kValue, k »).
        //     iv. Else, let mappedValue be kValue.
        //     v. Perform ? Set(targetObj, Pk, mappedValue, true).
        //     vi. Set k to k + 1.
        JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
        const size_t argsLength = 2;
        double k = 0;
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        while (k < len) {
            tKey.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kValue = vec[k];
            if (mapping) {
                EcmaRuntimeCallInfo info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
                info.SetCallArg(kValue.GetTaggedValue(), tKey.GetTaggedValue());
                JSTaggedValue callResult = JSFunction::Call(&info);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                mapValue.Update(callResult);
            } else {
                mapValue.Update(kValue.GetTaggedValue());
            }
            JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
            JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(targetObj), kKey, mapValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            k++;
        }
        //   f. Assert: values is now an empty List.
        //   g. Return targetObj.
        return targetObj.GetTaggedValue();
    }

    // 7. NOTE: source is not an Iterable so assume it is already an array-like object.
    // 8. Let arrayLike be ! ToObject(source).
    JSHandle<JSObject> arrayLikeObj = JSTaggedValue::ToObject(thread, source);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> arrayLike(arrayLikeObj);
    // 9. Let len be ? LengthOfArrayLike(arrayLike).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, arrayLike, lengthKey).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedNumber tLen = JSTaggedValue::ToLength(thread, lenResult);
    // 6. ReturnIfAbrupt(relativeTarget).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double len = tLen.GetNumber();

    // 10. Let targetObj be ? TypedArrayCreate(C, « len »).
    JSTaggedType args[1] = {JSTaggedValue(len).GetRawData()};
    JSHandle<JSObject> targetObj = TypedArrayHelper::TypedArrayCreate(thread, thisHandle, 1, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Let k be 0.
    // 12. Repeat, while k < len
    //   a. Let Pk be ! ToString(k).
    //   b. Let kValue be ? Get(arrayLike, Pk).
    //   c. If mapping is true, then
    //     i. Let mappedValue be ? Call(mapfn, thisArg, « kValue, k »).
    //   d. Else, let mappedValue be kValue.
    //   e. Perform ? Set(targetObj, Pk, mappedValue, true).
    //   f. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    const size_t argsLength = 2;
    double k = 0;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    while (k < len) {
        tKey.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, arrayLike, kKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> mapValue;
        if (mapping) {
            EcmaRuntimeCallInfo info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
            info.SetCallArg(kValue.GetTaggedValue()), tKey.GetTaggedValue();
            JSTaggedValue callResult = JSFunction::Call(&info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            mapValue = JSHandle<JSTaggedValue>(thread, callResult);
        } else {
            mapValue = kValue;
        }
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(targetObj), kKey, mapValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    // 13. Return targetObj.
    return targetObj.GetTaggedValue();
}

// 22.2.2.2 %TypedArray%.of ( ...items )
JSTaggedValue BuiltinsTypedArray::Of(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Of);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let len be the actual number of arguments passed to this function.
    uint32_t len = argv->GetArgsNumber();
    // 2. Let items be the List of arguments passed to this function.
    // 3. Let C be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 4. If IsConstructor(C) is false, throw a TypeError exception.
    if (!thisHandle->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the this value is not a Constructor.", JSTaggedValue::Exception());
    }
    // 5. Let newObj be TypedArrayCreate(C, « len »).
    JSTaggedType args[1] = {JSTaggedValue(len).GetRawData()};
    JSHandle<JSObject> newObj = TypedArrayHelper::TypedArrayCreate(thread, thisHandle, 1, args);
    // 6. ReturnIfAbrupt(newObj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let kValue be items[k].
    //   b. Let Pk be ! ToString(k).
    //   c. Perform ? Set(newObj, Pk, kValue, true).
    //   d. ReturnIfAbrupt(status).
    //   e. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    double k = 0;
    while (k < len) {
        tKey.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
        JSHandle<JSTaggedValue> kValue = GetCallArg(argv, k);
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newObj), kKey, kValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    // 9. Return newObj.
    return newObj.GetTaggedValue();
}

// 22.2.2.4
JSTaggedValue BuiltinsTypedArray::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    // 1. Return the this value.
    return GetThis(argv).GetTaggedValue();
}

// prototype
// 22.2.3.1 get %TypedArray%.prototype.buffer
JSTaggedValue BuiltinsTypedArray::GetBuffer(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, GetBuffer);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value does not have a [[ViewedArrayBuffer]] internal slot.",
                                    JSTaggedValue::Exception());
    }
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = JSHandle<JSTypedArray>::Cast(thisHandle)->GetViewedArrayBuffer();
    // 5. Return buffer.
    return buffer;
}

// 22.2.3.2
JSTaggedValue BuiltinsTypedArray::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, GetByteLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value does not have a [[ViewedArrayBuffer]] internal slot.",
                                    JSTaggedValue::Exception());
    }
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = JSHandle<JSTypedArray>::Cast(thisHandle)->GetViewedArrayBuffer();
    // 5. If IsDetachedBuffer(buffer) is true, return 0.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue(0);
    }
    // 6. Let size be the value of O’s [[ByteLength]] internal slot.
    // 7. Return size.
    return JSHandle<JSTypedArray>(thisHandle)->GetByteLength();
}

// 22.2.3.3
JSTaggedValue BuiltinsTypedArray::GetByteOffset(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, GetByteOffset);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value does not have a [[ViewedArrayBuffer]] internal slot.",
                                    JSTaggedValue::Exception());
    }
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = JSHandle<JSTypedArray>::Cast(thisHandle)->GetViewedArrayBuffer();
    // 5. If IsDetachedBuffer(buffer) is true, return 0.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue(0);
    }
    // 6. Let offset be the value of O’s [[ByteOffset]] internal slot.
    int32_t offset = TypedArrayHelper::GetByteOffset(thread, JSHandle<JSObject>(thisHandle));
    // 7. Return offset.
    return JSTaggedValue(offset);
}

// 22.2.3.5
JSTaggedValue BuiltinsTypedArray::CopyWithin(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, CopyWithin);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::CopyWithin(argv);
}

// 22.2.3.6
JSTaggedValue BuiltinsTypedArray::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    TypedArrayHelper::ValidateTypedArray(thread, thisHandle);
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    JSHandle<JSObject> self(thisHandle);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 4. Return CreateArrayIterator(O, "key+value").
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(self, IterationKind::KEY_AND_VALUE));
    return iter.GetTaggedValue();
}

// 22.2.3.7
JSTaggedValue BuiltinsTypedArray::Every(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Every);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int32_t len = TypedArrayHelper::GetArrayLength(thread, thisObjHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let testResult be ToBoolean(Call(callbackfn, T, «kValue, k, O»)).
    //     iv. ReturnIfAbrupt(testResult).
    //     v. If testResult is false, return false.
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    const size_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    int32_t k = 0;
    while (k < len) {
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, thisObjVal, k).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        info.SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(&info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        bool boolResult = callResult.ToBoolean();
        if (!boolResult) {
            return GetTaggedBoolean(false);
        }
        k++;
    }

    // 9. Return true.
    return GetTaggedBoolean(true);
}

// 22.2.3.8
JSTaggedValue BuiltinsTypedArray::Fill(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::Fill(argv);
}

// 22.2.3.9 %TypedArray%.prototype.filter ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsTypedArray::Filter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Filter);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    TypedArrayHelper::ValidateTypedArray(thread, thisHandle);
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSObject> thisObj(thisHandle);
    // 4. Let len be the value of O’s [[ArrayLength]] internal slot.
    int32_t len = TypedArrayHelper::GetArrayLength(thread, thisObj);
    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 10. Let kept be a new empty List.
    JSHandle<TaggedArray> kept(factory->NewTaggedArray(len));

    // 11. Let k be 0.
    // 12. Let captured be 0.
    // 13. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let selected be ToBoolean(Call(callbackfn, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(selected).
    //   f. If selected is true, then
    //     i. Append kValue to the end of kept.
    //     ii. Increase captured by 1.
    //   g. Increase k by 1.
    int32_t captured = 0;
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    for (int32_t k = 0; k < len; k++) {
        tKey.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, thisHandle, kKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle,
            undefined, 3); // 3: «kValue, k, O»
        info.SetCallArg(kValue.GetTaggedValue(), tKey.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(&info);
        bool testResult = callResult.ToBoolean();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (testResult) {
            kept->Set(thread, captured, kValue);
            captured++;
        }
    }
    // es11 9. Let A be ? TypedArraySpeciesCreate(O, « captured »).
    JSTaggedType args[1] = {JSTaggedValue(captured).GetRawData()};
    JSHandle<JSObject> newArrObj = TypedArrayHelper::TypedArraySpeciesCreate(thread, thisObj, 1, args);
    // 15. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 16. Let n be 0.
    // 17. For each element e of kept
    //   a. Let status be Set(A, ToString(n), e, true ).
    //   b. ReturnIfAbrupt(status).
    //   c. Increment n by 1.
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> ntKey(thread, JSTaggedValue::Undefined());
    for (int32_t n = 0; n < captured; n++) {
        valueHandle.Update(kept->Get(n));
        ntKey.Update(JSTaggedValue(n));
        JSHandle<JSTaggedValue> nKey(JSTaggedValue::ToString(thread, ntKey));
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(newArrObj), nKey, valueHandle, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 18. Return A.
    return newArrObj.GetTaggedValue();
}

// 22.2.3.10
JSTaggedValue BuiltinsTypedArray::Find(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::Find(argv);
}

// 22.2.3.11
JSTaggedValue BuiltinsTypedArray::FindIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::FindIndex(argv);
}

// 22.2.3.12
JSTaggedValue BuiltinsTypedArray::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int32_t len = TypedArrayHelper::GetArrayLength(thread, thisObjHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let funcResult be Call(callbackfn, T, «kValue, k, O»).
    //     iv. ReturnIfAbrupt(funcResult).
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    const size_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    int32_t k = 0;
    while (k < len) {
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, thisObjVal, k).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        info.SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        k++;
    }

    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

// 22.2.3.13
JSTaggedValue BuiltinsTypedArray::IndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::IndexOf(argv);
}

// 22.2.3.14
JSTaggedValue BuiltinsTypedArray::Join(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Join);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);

    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }

    uint32_t length =
        static_cast<uint32_t>(TypedArrayHelper::GetArrayLength(thread, JSHandle<JSObject>::Cast(thisHandle)));

    JSHandle<JSTaggedValue> sepHandle = GetCallArg(argv, 0);
    int sep = ',';
    uint32_t sepLength = 1;
    JSHandle<EcmaString> sepStringHandle;
    if (!sepHandle->IsUndefined()) {
        if (sepHandle->IsString()) {
            sepStringHandle = JSHandle<EcmaString>::Cast(sepHandle);
        } else {
            sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (sepStringHandle->IsUtf8() && sepStringHandle->GetLength() == 1) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            sep = sepStringHandle->GetDataUtf8()[0];
        } else if (sepStringHandle->GetLength() == 0) {
            sep = BuiltinsTypedArray::SeparatorFlag::MINUS_TWO;
            sepLength = 0;
        } else {
            sep = BuiltinsTypedArray::SeparatorFlag::MINUS_ONE;
            sepLength = sepStringHandle->GetLength();
        }
    }
    if (length == 0) {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        return globalConst->GetEmptyString();
    }
    size_t allocateLength = 0;
    bool isOneByte = (sep != BuiltinsTypedArray::SeparatorFlag::MINUS_ONE) || sepStringHandle->IsUtf8();
    CVector<JSHandle<EcmaString>> vec;
    JSMutableHandle<JSTaggedValue> elementHandle(thread, JSTaggedValue::Undefined());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    for (uint32_t k = 0; k < length; k++) {
        JSTaggedValue element = JSTypedArray::GetProperty(thread, thisHandle, k).GetValue().GetTaggedValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!element.IsUndefinedOrNull() && !element.IsHole()) {
            if (!element.IsString()) {
                elementHandle.Update(element);
                JSHandle<EcmaString> strElement = JSTaggedValue::ToString(thread, elementHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                element = strElement.GetTaggedValue();
            }
            auto nextStr = EcmaString::Cast(element.GetTaggedObject());
            JSHandle<EcmaString> nextStrHandle(thread, nextStr);
            vec.push_back(nextStrHandle);
            isOneByte = nextStr->IsUtf8() ? isOneByte : false;
            allocateLength += nextStr->GetLength();
        } else {
            vec.push_back(JSHandle<EcmaString>(globalConst->GetHandledEmptyString()));
        }
    }
    allocateLength += sepLength * (length - 1);
    auto newString = EcmaString::AllocStringObject(allocateLength, isOneByte, thread->GetEcmaVM());
    int current = 0;
    DISALLOW_GARBAGE_COLLECTION;
    for (uint32_t k = 0; k < length; k++) {
        if (k > 0) {
            if (sep >= 0) {
                newString->WriteData(static_cast<char>(sep), current);
            } else if (sep != BuiltinsTypedArray::SeparatorFlag::MINUS_TWO) {
                newString->WriteData(
                    *sepStringHandle, current, allocateLength - static_cast<size_t>(current), sepLength);
            }
            current += static_cast<int>(sepLength);
        }
        JSHandle<EcmaString> nextStr = vec[k];
        int nextLength = static_cast<int>(nextStr->GetLength());
        newString->WriteData(*nextStr, current, allocateLength - static_cast<size_t>(current), nextLength);
        current += nextLength;
    }
    ASSERT_PRINT(isOneByte == EcmaString::CanBeCompressed(newString), "isOneByte does not match the real value!");
    return JSTaggedValue(newString);
}

// 22.2.3.15
JSTaggedValue BuiltinsTypedArray::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    TypedArrayHelper::ValidateTypedArray(thread, thisHandle);
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    JSHandle<JSObject> self(thisHandle);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 4. Return CreateArrayIterator(O, "key").
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(self, IterationKind::KEY));
    return iter.GetTaggedValue();
}

// 22.2.3.16
JSTaggedValue BuiltinsTypedArray::LastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::LastIndexOf(argv);
}

// 22.2.3.17
JSTaggedValue BuiltinsTypedArray::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, GetLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[TypedArrayName]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value does not have a [[TypedArrayName]] internal slot.",
                                    JSTaggedValue::Exception());
    }
    // 4. Assert: O has [[ViewedArrayBuffer]] and [[ArrayLength]] internal slots.
    // 5. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = JSHandle<JSTypedArray>::Cast(thisHandle)->GetViewedArrayBuffer();
    // 6. If IsDetachedBuffer(buffer) is true, return 0.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue(0);
    }
    // 7. Let length be the value of O’s [[ArrayLength]] internal slot.
    int32_t length = TypedArrayHelper::GetArrayLength(thread, JSHandle<JSObject>(thisHandle));
    // 8. Return length.
    return JSTaggedValue(length);
}

// 22.2.3.18 %TypedArray%.prototype.map ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsTypedArray::Map(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Map);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    TypedArrayHelper::ValidateTypedArray(thread, thisHandle);
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSObject> thisObj(thisHandle);
    // 4. Let len be the value of O’s [[ArrayLength]] internal slot.
    int32_t len = TypedArrayHelper::GetArrayLength(thread, thisObj);
    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackfnHandle = GetCallArg(argv, 0);
    if (!callbackfnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    // es11 5. Let A be ? TypedArraySpeciesCreate(O, « len »).
    JSTaggedType args[1] = {JSTaggedValue(len).GetRawData()};
    JSHandle<JSObject> newArrObj = TypedArrayHelper::TypedArraySpeciesCreate(thread, thisObj, 1, args); // 1: one arg.
    // 11. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12. Let k be 0.
    // 13. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let mappedValue be Call(callbackfn, T, «kValue, k, O»).
    //   e. ReturnIfAbrupt(mappedValue).
    //   f. Let status be Set(A, Pk, mappedValue, true ).
    //   g. ReturnIfAbrupt(status).
    //   h. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
    const size_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    for (int32_t k = 0; k < len; k++) {
        key.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, thisHandle, key).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackfnHandle, thisArgHandle, undefined, argsLength);
        info.SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(&info);
        mapValue.Update(callResult);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(newArrObj), key, mapValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // 14. Return A.
    return newArrObj.GetTaggedValue();
}

// 22.2.3.19
JSTaggedValue BuiltinsTypedArray::Reduce(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::Reduce(argv);
}

// 22.2.3.20
JSTaggedValue BuiltinsTypedArray::ReduceRight(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::ReduceRight(argv);
}

// 22.2.3.21
JSTaggedValue BuiltinsTypedArray::Reverse(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::Reverse(argv);
}

// 22.2.3.22 %TypedArray%.prototype.set ( overloaded [ , offset ])
JSTaggedValue BuiltinsTypedArray::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Assert: array is any ECMAScript language value other than an Object with a [[TypedArrayName]] internal slot.
    // If it is such an Object, the definition in 22.2.3.22.2 applies.
    // 2. Let target be the this value.
    JSHandle<JSTaggedValue> target = GetThis(argv);
    // 3. If Type(target) is not Object, throw a TypeError exception.
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> targetObj(target);
    // 4. If target does not have a [[TypedArrayName]] internal slot, throw a TypeError exception.
    if (!target->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value does not have a [[TypedArrayName]] internal slot.",
                                    JSTaggedValue::Exception());
    }

    // 5. Assert: target has a [[ViewedArrayBuffer]] internal slot.
    // 6. Let targetOffset be ToInteger (offset).
    JSTaggedNumber tTargetOffset = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 1));
    // 7. ReturnIfAbrupt(targetOffset).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double targetOffset = tTargetOffset.GetNumber();
    // 8. If targetOffset < 0, throw a RangeError exception.
    if (targetOffset < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The targetOffset of This value is less than 0.",
                                     JSTaggedValue::Exception());
    }
    // 9. Let targetBuffer be the value of target’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSTaggedValue> targetBuffer(thread, JSTypedArray::Cast(*targetObj)->GetViewedArrayBuffer());
    // 10. If IsDetachedBuffer(targetBuffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(targetBuffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The targetBuffer of This value is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    // 11. Let targetLength be the value of target’s [[ArrayLength]] internal slot.
    // 12. Let targetName be the String value of target’s [[TypedArrayName]] internal slot.
    // 13. Let targetElementSize be the Number value of the Element Size value specified in Table 49 for targetName.
    // 14. Let targetType be the String value of the Element Type value in Table 49 for targetName.
    // 15. Let targetByteOffset be the value of target’s [[ByteOffset]] internal slot.
    int32_t targetLength = TypedArrayHelper::GetArrayLength(thread, targetObj);
    JSHandle<JSTaggedValue> targetName(thread, JSTypedArray::Cast(*targetObj)->GetTypedArrayName());
    int32_t targetElementSize = TypedArrayHelper::GetSizeFromName(thread, targetName);
    DataViewType targetType = TypedArrayHelper::GetTypeFromName(thread, targetName);
    int32_t targetByteOffset = TypedArrayHelper::GetByteOffset(thread, targetObj);

    JSHandle<JSTaggedValue> argArray = GetCallArg(argv, 0);

    // 22.2.3.22.1 %TypedArray%.prototype.set (array [ , offset ] )
    if (!argArray->IsTypedArray()) {
        // 16. Let src be ToObject(array).
        JSHandle<JSObject> src = JSTaggedValue::ToObject(thread, argArray);
        // 17. ReturnIfAbrupt(src).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 18. Let srcLength be ToLength(Get(src, "length")).
        JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
        JSHandle<JSTaggedValue> lenResult =
            JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(src), lengthKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedNumber tSrcLen = JSTaggedValue::ToLength(thread, lenResult);
        // 19. ReturnIfAbrupt(srcLength).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double srcLen = tSrcLen.GetNumber();
        // 20. If srcLength + targetOffset > targetLength, throw a RangeError exception.
        if (srcLen + targetOffset > targetLength) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The sum of srcLength and targetOffset is greater than targetLength.",
                                         JSTaggedValue::Exception());
        }
        // 21. Let targetByteIndex be targetOffset × targetElementSize + targetByteOffset.
        int32_t targetByteIndex = targetOffset * targetElementSize + targetByteOffset;
        // 22. Let k be 0.
        // 23. Let limit be targetByteIndex + targetElementSize × srcLength.
        int32_t k = 0;
        int32_t limit = targetByteIndex + targetElementSize * srcLen;
        // 24. Repeat, while targetByteIndex < limit
        //   a. Let Pk be ToString(k).
        //   b. If target.[[ContentType]] is BigInt, set value to ? ToBigInt(value).
        //   c. Otherwise, set value to ? ToNumber(value).
        //   d. If IsDetachedBuffer(targetBuffer) is true, throw a TypeError exception.
        //   e. Perform SetValueInBuffer(targetBuffer, targetByteIndex, targetType, kNumber).
        //   f. Set k to k + 1.
        //   g. Set targetByteIndex to targetByteIndex + targetElementSize.
        JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> kNumberHandle(thread, JSTaggedValue::Undefined());
        while (targetByteIndex < limit) {
            tKey.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
            JSHandle<JSTaggedValue> kValue =
                JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(src), kKey).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (BuiltinsArrayBuffer::IsDetachedBuffer(targetBuffer.GetTaggedValue())) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "The targetBuffer of This value is detached buffer.",
                                            JSTaggedValue::Exception());
            }
            ContentType contentType = JSHandle<JSTypedArray>::Cast(target)->GetContentType();
            if (contentType == ContentType::BigInt) {
                kNumberHandle.Update(JSTaggedValue::ToBigInt(thread, kValue));
            } else {
                kNumberHandle.Update(JSTaggedValue::ToNumber(thread, kValue));
            }
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(), targetByteIndex,
                                                  targetType, kNumberHandle, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            k++;
            targetByteIndex = targetByteIndex + targetElementSize;
        }
        // 25. Return undefined.
        return JSTaggedValue::Undefined();
    }

    // 22.2.3.22.2 %TypedArray%.prototype.set(typedArray [, offset ] )
    JSHandle<JSObject> typedArray(argArray);
    // 12. Let srcBuffer be the value of typedArray’s [[ViewedArrayBuffer]] internal slot.
    // 13. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
    JSTaggedValue srcBuffer = JSTypedArray::Cast(*typedArray)->GetViewedArrayBuffer();
    JSHandle<JSTaggedValue> srcBufferHandle = JSHandle<JSTaggedValue>(thread, srcBuffer);
    if (BuiltinsArrayBuffer::IsDetachedBuffer(srcBuffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ArrayBuffer of typedArray is detached buffer.",
                                    JSTaggedValue::Exception());
    }

    ContentType objContentType = JSHandle<JSTypedArray>::Cast(target)->GetContentType();
    ContentType argArrayContentType = JSHandle<JSTypedArray>::Cast(argArray)->GetContentType();
    if (argArrayContentType != objContentType) {
        THROW_TYPE_ERROR_AND_RETURN(thread,
                                    "argArrayContentType is not equal objContentType.",
                                    JSTaggedValue::Exception());
    }
    // 18. Let srcName be the String value of typedArray’s [[TypedArrayName]] internal slot.
    // 19. Let srcType be the String value of the Element Type value in Table 49 for srcName .
    // 20. Let srcElementSize be the Number value of the Element Size value specified in Table 49 for srcName.
    // 21. Let srcLength be the value of typedArray’s [[ArrayLength]] internal slot.
    // 22. Let srcByteOffset be the value of typedArray’s [[ByteOffset]] internal slot.
    JSHandle<JSTaggedValue> srcName(thread, JSTypedArray::Cast(*typedArray)->GetTypedArrayName());
    DataViewType srcType = TypedArrayHelper::GetTypeFromName(thread, srcName);
    int32_t srcElementSize = TypedArrayHelper::GetSizeFromName(thread, srcName);
    int32_t srcLength = TypedArrayHelper::GetArrayLength(thread, typedArray);
    int32_t srcByteOffset = TypedArrayHelper::GetByteOffset(thread, typedArray);
    // 23. If srcLength + targetOffset > targetLength, throw a RangeError exception.
    if (srcLength + targetOffset > targetLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The sum of srcLength and targetOffset is greater than targetLength.",
                                     JSTaggedValue::Exception());
    }
    // 24. If SameValue(srcBuffer, targetBuffer) is true, then
    //   a. Let srcBuffer be CloneArrayBuffer(targetBuffer, srcByteOffset, %ArrayBuffer%).
    //   b. NOTE: %ArrayBuffer% is used to clone targetBuffer because is it known to not have any observable
    //      side-effects.
    //   c. ReturnIfAbrupt(srcBuffer).
    //   d. Let srcByteIndex be 0.
    // 25. Else, let srcByteIndex be srcByteOffset.
    int32_t srcByteIndex;
    if (JSTaggedValue::SameValue(srcBufferHandle.GetTaggedValue(), targetBuffer.GetTaggedValue())) {
        srcBuffer =
            BuiltinsArrayBuffer::CloneArrayBuffer(thread, targetBuffer, srcByteOffset, env->GetArrayBufferFunction());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        srcBufferHandle = JSHandle<JSTaggedValue>(thread, srcBuffer);
        srcByteIndex = 0;
    } else {
        srcByteIndex = srcByteOffset;
    }
    // 26. Let targetByteIndex be targetOffset × targetElementSize + targetByteOffset.
    int32_t targetByteIndex = targetOffset * targetElementSize + targetByteOffset;
    // 27. Let limit be targetByteIndex + targetElementSize × srcLength.
    int32_t limit = targetByteIndex + targetElementSize * srcLength;
    // 28. If SameValue(srcType, targetType) is false, then
    //   a. Repeat, while targetByteIndex < limit
    //     i. Let value be GetValueFromBuffer(srcBuffer, srcByteIndex, srcType).
    //     ii. Perform SetValueInBuffer (targetBuffer, targetByteIndex, targetType, value).
    //     iii. Set srcByteIndex to srcByteIndex + srcElementSize.
    //     iv. Set targetByteIndex to targetByteIndex + targetElementSize.
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    if (srcType != targetType) {
        while (targetByteIndex < limit) {
            JSTaggedValue taggedData =
                BuiltinsArrayBuffer::GetValueFromBuffer(thread, srcBufferHandle.GetTaggedValue(),
                                                        srcByteIndex, srcType, true);
            value.Update(taggedData);
            BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(), targetByteIndex,
                                                  targetType, value, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            srcByteIndex = srcByteIndex + srcElementSize;
            targetByteIndex = targetByteIndex + targetElementSize;
        }
    } else {
        // 29. Else,
        //   a. NOTE: If srcType and targetType are the same the transfer must be performed in a manner that preserves
        //   the bit-level encoding of the source data.
        //   b. Repeat, while targetByteIndex < limit
        //     i. Let value be GetValueFromBuffer(srcBuffer, srcByteIndex, "Uint8").
        //     ii. Perform SetValueInBuffer (targetBuffer, targetByteIndex, "Uint8", value).
        //     iii. Set srcByteIndex to srcByteIndex + 1.
        //     iv. Set targetByteIndex to targetByteIndex + 1.
        while (targetByteIndex < limit) {
            JSTaggedValue taggedData =
                BuiltinsArrayBuffer::GetValueFromBuffer(thread, srcBufferHandle.GetTaggedValue(), srcByteIndex,
                                                        DataViewType::UINT8, true);
            value.Update(taggedData);
            BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(), targetByteIndex,
                                                  DataViewType::UINT8, value, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            srcByteIndex = srcByteIndex + 1;
            targetByteIndex = targetByteIndex + 1;
        }
    }
    // 30. Return undefined.
    return JSTaggedValue::Undefined();
}  // namespace panda::ecmascript::builtins

// 22.2.3.23 %TypedArray%.prototype.slice ( start, end )
JSTaggedValue BuiltinsTypedArray::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Slice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    TypedArrayHelper::ValidateTypedArray(thread, thisHandle);
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSObject> thisObj(thisHandle);
    // 4. Let len be the value of O’s [[ArrayLength]] internal slot.
    int32_t len = TypedArrayHelper::GetArrayLength(thread, thisObj);

    double k;
    // 5. Let relativeStart be ToInteger(start).
    JSTaggedNumber tRelativeStart = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // 6. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double relativeStart = tRelativeStart.GetNumber();
    // 7. If relativeStart < 0, let k be max((len + relativeStart),0); else let k be min(relativeStart, len).
    if (relativeStart < 0) {
        k = relativeStart + len > 0 ? relativeStart + len : 0;
    } else {
        k = relativeStart < len ? relativeStart : len;
    }
    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    double relativeEnd = len;
    JSHandle<JSTaggedValue> end = GetCallArg(argv, 1);
    if (!end->IsUndefined()) {
        JSTaggedNumber tRelativeEnd = JSTaggedValue::ToInteger(thread, end);
        // 9. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        relativeEnd = tRelativeEnd.GetNumber();
    }

    // 10. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    double final = 0;
    if (relativeEnd < 0) {
        final = relativeEnd + len > 0 ? relativeEnd + len : 0;
    } else {
        final = relativeEnd < len ? relativeEnd : len;
    }
    // 11. Let count be max(final – k, 0).
    double count = (final - k) > 0 ? (final - k) : 0;
    // es11 9. Let A be ? TypedArraySpeciesCreate(O, « count »).
    JSTaggedType args[1] = {JSTaggedValue(count).GetRawData()};
    JSHandle<JSObject> newArrObj = TypedArrayHelper::TypedArraySpeciesCreate(thread, thisObj, 1, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 17. Let srcName be the String value of O’s [[TypedArrayName]] internal slot.
    // 18. Let srcType be the String value of the Element Type value in Table 49 for srcName.
    JSHandle<JSTaggedValue> srcName(thread, JSTypedArray::Cast(*thisObj)->GetTypedArrayName());
    DataViewType srcType = TypedArrayHelper::GetTypeFromName(thread, srcName);
    // 19. Let targetName be the String value of A’s [[TypedArrayName]] internal slot.
    // 20. Let targetType be the String value of the Element Type value in Table 49 for targetName.
    JSHandle<JSTaggedValue> targetName(thread, JSTypedArray::Cast(*newArrObj)->GetTypedArrayName());
    DataViewType targetType = TypedArrayHelper::GetTypeFromName(thread, targetName);
    // 21. If SameValue(srcType, targetType) is false, then
    //   a. Let n be 0.
    //   b. Repeat, while k < final
    //     i. Let Pk be ToString(k).
    //     ii. Let kValue be Get(O, Pk).
    //     iii. ReturnIfAbrupt(kValue).
    //     iv. Let status be Set(A, ToString(n), kValue, true ).
    //     v. ReturnIfAbrupt(status).
    //     vi. Increase k by 1.
    //     vii. Increase n by 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> ntKey(thread, JSTaggedValue::Undefined());
    if (srcType != targetType) {
        double n = 0;
        while (k < final) {
            tKey.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
            kValue.Update(JSTaggedValue::GetProperty(thread, thisHandle, kKey).GetValue().GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            ntKey.Update(JSTaggedValue(n));
            JSHandle<JSTaggedValue> nKey(JSTaggedValue::ToString(thread, ntKey));
            JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(newArrObj), nKey, kValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            n++;
            k++;
        }
    } else if (count > 0) {
        // 22. Else if count > 0,
        //   a. Let srcBuffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
        //   b. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
        JSHandle<JSTaggedValue> srcBuffer(thread, JSTypedArray::Cast(*thisObj)->GetViewedArrayBuffer());
        if (BuiltinsArrayBuffer::IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The ArrayBuffer of this value is detached buffer.",
                                        JSTaggedValue::Exception());
        }
        //   c. Let targetBuffer be the value of A’s [[ViewedArrayBuffer]] internal slot.
        JSHandle<JSTaggedValue> targetBuffer(thread, JSTypedArray::Cast(*newArrObj)->GetViewedArrayBuffer());
        //   d. Let elementSize be the Number value of the Element Size value specified in Table 49 for srcType.
        int32_t elementSize = TypedArrayHelper::GetSizeFromName(thread, srcName);
        //   e. NOTE: If srcType and targetType are the same the transfer must be performed in a manner that
        //   preserves the bit-level encoding of the source data. f. Let srcByteOffset be the value of O’s
        //   [[ByteOffset]] internal slot.
        int32_t srcByteOffset = TypedArrayHelper::GetByteOffset(thread, thisObj);
        //   g. Let targetByteIndex be 0.
        //   h. Let srcByteIndex be (k × elementSize) + srcByteOffset.

        int32_t srcByteIndex = k * elementSize + srcByteOffset;
        //   i. Repeat, while targetByteIndex < count × elementSize
        //     i. Let value be GetValueFromBuffer(srcBuffer, srcByteIndex, "Uint8").
        //     ii. Perform SetValueInBuffer (targetBuffer, targetByteIndex, "Uint8", value).
        //     iii. Increase srcByteIndex by 1.
        //     iv. Increase targetByteIndex by 1.
        JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
        for (int32_t targetByteIndex = 0; targetByteIndex < count * elementSize; srcByteIndex++, targetByteIndex++) {
            JSTaggedValue taggedData = BuiltinsArrayBuffer::GetValueFromBuffer(thread, srcBuffer.GetTaggedValue(),
                                                                               srcByteIndex, DataViewType::UINT8,
                                                                               true);
            value.Update(taggedData);
            BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(), targetByteIndex,
                                                  DataViewType::UINT8, value, true);
        }
    }
    // 23. Return A.
    return newArrObj.GetTaggedValue();
}

// 22.2.3.24
JSTaggedValue BuiltinsTypedArray::Some(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::Some(argv);
}

// 22.2.3.25
JSTaggedValue BuiltinsTypedArray::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Sort);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let obj be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    JSHandle<JSTaggedValue> buffer;
    buffer = JSHandle<JSTaggedValue>(thread, TypedArrayHelper::ValidateTypedArray(thread, thisHandle));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double len = TypedArrayHelper::GetArrayLength(thread, thisObjHandle);

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);

    uint32_t i = 0;
    while (i < len - 1) {
        uint32_t j = len - 1;
        while (j > i) {
            JSHandle<JSTaggedValue> xValue = JSTaggedValue::GetProperty(thread, thisObjVal, j - 1).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<JSTaggedValue> yValue = JSTaggedValue::GetProperty(thread, thisObjVal, j).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            int32_t compareResult;
            compareResult = TypedArrayHelper::SortCompare(thread, callbackFnHandle, buffer, xValue, yValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (compareResult > 0) {
                JSTaggedValue::SetProperty(thread, thisObjVal, j - 1, yValue, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSTaggedValue::SetProperty(thread, thisObjVal, j, xValue, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            j--;
        }
        i++;
    }
    return JSTaggedValue::ToObject(thread, thisHandle).GetTaggedValue();
}

// 22.2.3.26 %TypedArray%.prototype.subarray( [ begin [ , end ] ] )
JSTaggedValue BuiltinsTypedArray::Subarray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Subarray);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> thisObj(thisHandle);
    // 3. If O does not have a [[TypedArrayName]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value does not have a [[TypedArrayName]] internal slot.",
                                    JSTaggedValue::Exception());
    }
    // 4. Assert: O has a [[ViewedArrayBuffer]] internal slot.
    // 6. Let srcLength be the value of O’s [[ArrayLength]] internal slot.
    int32_t srcLength = TypedArrayHelper::GetArrayLength(thread, thisObj);
    // 7. Let relativeBegin be ToInteger(begin).
    JSTaggedNumber tRelativeBegin = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // 8. ReturnIfAbrupt(relativeBegin).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double relativeBegin = tRelativeBegin.GetNumber();

    double beginIndex;
    // 9. If relativeBegin < 0, let beginIndex be max((srcLength + relativeBegin), 0); else let beginIndex be
    // min(relativeBegin, srcLength).
    if (relativeBegin < 0) {
        beginIndex = relativeBegin + srcLength > 0 ? relativeBegin + srcLength : 0;
    } else {
        beginIndex = relativeBegin < srcLength ? relativeBegin : srcLength;
    }

    // 10. If end is undefined, let relativeEnd be srcLength; else, let relativeEnd be ToInteger(end).
    double relativeEnd = srcLength;
    JSHandle<JSTaggedValue> end = GetCallArg(argv, 1);
    if (!end->IsUndefined()) {
        JSTaggedNumber tRelativeEnd = JSTaggedValue::ToInteger(thread, end);
        // 11. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        relativeEnd = tRelativeEnd.GetNumber();
    }
    // 12. If relativeEnd < 0, let endIndex be max((srcLength + relativeEnd), 0); else let endIndex be
    // min(relativeEnd, srcLength).
    double endIndex;
    if (relativeEnd < 0) {
        endIndex = relativeEnd + srcLength > 0 ? relativeEnd + srcLength : 0;
    } else {
        endIndex = relativeEnd < srcLength ? relativeEnd : srcLength;
    }
    // 13. Let newLength be max(endIndex – beginIndex, 0).
    double newLength = (endIndex - beginIndex) > 0 ? (endIndex - beginIndex) : 0;
    // 14. Let constructorName be the String value of O’s [[TypedArrayName]] internal slot.
    // 15. Let elementSize be the Number value of the Element Size value specified in Table 49 for constructorName.
    // 16. Let srcByteOffset be the value of O’s [[ByteOffset]] internal slot.
    // 17. Let beginByteOffset be srcByteOffset + beginIndex × elementSize.
    JSHandle<JSTaggedValue> constructorName(thread, JSTypedArray::Cast(*thisObj)->GetTypedArrayName());
    int32_t elementSize = TypedArrayHelper::GetSizeFromName(thread, constructorName);
    int32_t srcByteOffset = TypedArrayHelper::GetByteOffset(thread, thisObj);
    int32_t beginByteOffset = srcByteOffset + beginIndex * elementSize;
    // 21. Let argumentsList be «buffer, beginByteOffset, newLength».
    // 5. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = JSTypedArray::Cast(*thisObj)->GetViewedArrayBuffer();
    // 22. Return Construct(constructor, argumentsList).
    const size_t argsLength = 3;
    JSTaggedType args[argsLength] = {
        buffer.GetRawData(), JSTaggedValue(beginByteOffset).GetRawData(), JSTaggedValue(newLength).GetRawData()
    };
    JSHandle<JSObject> newArr = TypedArrayHelper::TypedArraySpeciesCreate(thread, thisObj, argsLength, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArr.GetTaggedValue();
}

// 22.2.3.27
JSTaggedValue BuiltinsTypedArray::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::ToLocaleString(argv);
}

// 22.2.3.28
JSTaggedValue BuiltinsTypedArray::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::ToString(argv);
}

// 22.2.3.29
JSTaggedValue BuiltinsTypedArray::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    TypedArrayHelper::ValidateTypedArray(thread, thisHandle);
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    JSHandle<JSObject> self(thisHandle);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 4. Return CreateArrayIterator(O, "value").
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(self, IterationKind::VALUE));
    return iter.GetTaggedValue();
}

// 22.2.3.31
JSTaggedValue BuiltinsTypedArray::ToStringTag(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TypedArray, ToStringTag);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, return undefined.
    if (!thisHandle->IsECMAObject()) {
        return JSTaggedValue::Undefined();
    }
    // 3. If O does not have a [[TypedArrayName]] internal slot, return undefined.
    if (!thisHandle->IsTypedArray()) {
        return JSTaggedValue::Undefined();
    }
    // 4. Let name be the value of O’s [[TypedArrayName]] internal slot.
    JSTaggedValue name = JSHandle<JSTypedArray>::Cast(thisHandle)->GetTypedArrayName();
    // 5. Assert: name is a String value.
    ASSERT(name.IsString());
    // 6. Return name.
    return name;
}

// es12 23.2.3.13
JSTaggedValue BuiltinsTypedArray::Includes(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    if (!GetThis(argv)->IsTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "This is not a TypedArray.", JSTaggedValue::Exception());
    }
    return BuiltinsArray::Includes(argv);
}
}  // namespace panda::ecmascript::builtins
