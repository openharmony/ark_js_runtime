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

#include "ecmascript/builtins/builtins_sharedarraybuffer.h"

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
// 25.2.2.1 SharedArrayBuffer ( [ length ] )
JSTaggedValue BuiltinsSharedArrayBuffer::SharedArrayBufferConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArrayBuffer, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newtarget is undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> lengthHandle = GetCallArg(argv, 0);
    // 2. Let byteLength be ? ToIndex(length).
    JSTaggedNumber lenNum = JSTaggedValue::ToIndex(thread, lengthHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double byteLength = lenNum.GetNumber();
    // 3. Return ? AllocateSharedArrayBuffer(NewTarget, byteLength).
    return AllocateSharedArrayBuffer(thread, newTarget, byteLength);
}

// 25.2.1.2 IsSharedArrayBuffer ( obj )
JSTaggedValue BuiltinsSharedArrayBuffer::IsSharedArrayBuffer(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(thread, SharedArrayBuffer, IsSharedArrayBuffer);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    JSHandle<JSTaggedValue> arg = GetCallArg(argv, 0);
    // 1. If Type(arg) is not Object,and it not has an [[ArrayBufferData]] internal slot return false.
    if (!arg->IsECMAObject()) {
        return BuiltinsSharedArrayBuffer::GetTaggedBoolean(false);
    }
    if (!arg->IsSharedArrayBuffer()) {
        return BuiltinsSharedArrayBuffer::GetTaggedBoolean(false);
    }
    // 2. Let bufferData be obj.[[ArrayBufferData]].
    JSHandle<JSArrayBuffer> buffer(arg);
    JSTaggedValue bufferdata = buffer->GetArrayBufferData();
    // 3. If bufferData is null, return false.
    if (bufferdata == JSTaggedValue::Null()) {
        return BuiltinsSharedArrayBuffer::GetTaggedBoolean(false);
    }
    // 4. If this ArrayBuffer is not shared, return false.
    if (buffer->GetShared() == false) {
        return BuiltinsSharedArrayBuffer::GetTaggedBoolean(false);
    }
    return BuiltinsSharedArrayBuffer::GetTaggedBoolean(true);
}

bool BuiltinsSharedArrayBuffer::IsShared(JSTaggedValue arrayBuffer)
{
    if (!arrayBuffer.IsSharedArrayBuffer()) {
        return false;
    }
    JSArrayBuffer *buffer = JSArrayBuffer::Cast(arrayBuffer.GetTaggedObject());
    JSTaggedValue dataSlot = buffer->GetArrayBufferData();
    // 2. If arrayBuffer’s [[ArrayBufferData]] internal slot is null, return false.
    if (dataSlot.IsNull()) {
        return false;
    }
    // 3. If this ArrayBuffer is not shared, return false.
    return buffer->GetShared();
}

// 25.2.1.1 AllocateSharedArrayBuffer ( constructor, byteLength )
JSTaggedValue BuiltinsSharedArrayBuffer::AllocateSharedArrayBuffer(
    JSThread *thread, const JSHandle<JSTaggedValue> &newTarget, double byteLength)
{
    BUILTINS_API_TRACE(thread, SharedArrayBuffer, AllocateSharedArrayBuffer);
    /**
     * 1. Let obj be ? OrdinaryCreateFromConstructor(constructor, "%SharedArrayBuffer.prototype%",
     * «[[ArrayBufferData]], [[ArrayBufferByteLength]] »).
     * */
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> shaArrBufFunc = env->GetSharedArrayBufferFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(shaArrBufFunc), newTarget);
    // 2. ReturnIfAbrupt
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Assert: byteLength is a positive integer.
    ASSERT(JSTaggedValue(byteLength).IsInteger());
    ASSERT(byteLength >= 0);
    // 4. Let block be CreateSharedByteDataBlock(byteLength).
    if (byteLength > INT_MAX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Out of range", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> sharedArrayBuffer(obj);
    // 6. Set obj’s [[ArrayBufferData]] internal slot to block.
    factory->NewJSSharedArrayBufferData(sharedArrayBuffer, byteLength);
    // 7. Set obj’s [[ArrayBufferByteLength]] internal slot to byteLength.
    sharedArrayBuffer->SetArrayBufferByteLength(static_cast<uint32_t>(byteLength));
    // 8. Return obj.
    return sharedArrayBuffer.GetTaggedValue();
}

// 25.2.3.2 get SharedArrayBuffer [ @@species ]
JSTaggedValue BuiltinsSharedArrayBuffer::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArrayBuffer, Species);
    // 1. Return the this value.
    return GetThis(argv).GetTaggedValue();
}

// 25.2.4.1 get SharedArrayBuffer.prototype.byteLength
JSTaggedValue BuiltinsSharedArrayBuffer::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArrayBuffer, GetByteLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> shaArrBuf(thisHandle);
    // 5. Let length be the value of O’s [[SharedArrayBufferByteLength]] internal slot.
    uint32_t length = shaArrBuf->GetArrayBufferByteLength();
    // 6. Return length.
    return JSTaggedValue(length);
}

// 25.2.4.3 SharedArrayBuffer.prototype.slice ( start, end )
JSTaggedValue BuiltinsSharedArrayBuffer::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArrayBuffer, Slice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> shaArrBuf(thisHandle);
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    // 4. If IsSharedArrayBuffer(O) is false, throw a TypeError exception.
    if (!IsShared(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value not IsSharedArrayBuffer", JSTaggedValue::Exception());
    }
    // 5. Let len be the value of O’s [[ArrayBufferByteLength]] internal slot.
    int32_t len = static_cast<int32_t>(shaArrBuf->GetArrayBufferByteLength());
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
    last = end < 0 ? std::max((len + end), 0) : std::min(end, len);
    // 12. Let newLen be max(final-first,0).
    uint32_t newLen = std::max((last - first), 0);
    // 13. Let ctor be SpeciesConstructor(O, %SharedArrayBuffer%).
    JSHandle<JSTaggedValue> defaultConstructor = env->GetSharedArrayBufferFunction();
    JSHandle<JSObject> objHandle(thisHandle);
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
    // 14. ReturnIfAbrupt(ctor).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 15. Let new be Construct(ctor, «newLen»).
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 1);
    info.SetCallArg(JSTaggedValue(newLen));
    JSTaggedValue taggedNewArrBuf = JSFunction::Construct(&info);
    JSHandle<JSTaggedValue> newArrBuf(thread, taggedNewArrBuf);
    // 16. ReturnIfAbrupt(new).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 17. If new does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!newArrBuf->IsSharedArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have bufferdata internal slot", JSTaggedValue::Exception());
    }
    // 18. If IsSharedArrayBuffer(new) is false, throw a TypeError exception.
    if (!IsShared(newArrBuf.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new arrayBuffer not IsSharedArrayBuffer", JSTaggedValue::Exception());
    }
    // 19. If SameValue(new, O) is true, throw a TypeError exception.
    if (JSTaggedValue::SameValue(newArrBuf.GetTaggedValue(), thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "value of new arraybuffer and this is same", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> newJsShaArrBuf(newArrBuf);
    // 20. If the value of new’s [[ArrayBufferByteLength]] internal slot < newLen, throw a TypeError exception.
    uint32_t newArrBufLen = newJsShaArrBuf->GetArrayBufferByteLength();
    if (newArrBufLen < newLen) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new array buffer length smaller than newlen", JSTaggedValue::Exception());
    }
    if (newLen > 0) {
        // 23. Let fromBuf be the value of O’s [[ArrayBufferData]] internal slot.
        JSTaggedValue from = shaArrBuf->GetArrayBufferData();
        // 24. Let toBuf be the value of new’s [[ArrayBufferData]] internal slot.
        JSTaggedValue to = newJsShaArrBuf->GetArrayBufferData();
        // 25. Perform CopyDataBlockBytes(toBuf, fromBuf, first, newLen).
        JSArrayBuffer::CopyDataBlockBytes(to, from, first, newLen);
    }
    // Return new.
    return newArrBuf.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins