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

#ifndef PANDA_RUNTIME_ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_int8_array.h"
#include "ecmascript/js_uint8_array.h"
#include "ecmascript/js_uint8_clamped_array.h"
#include "ecmascript/js_int16_array.h"
#include "ecmascript/js_uint16_array.h"
#include "ecmascript/js_int32_array.h"
#include "ecmascript/js_uint32_array.h"
#include "ecmascript/js_float32_array.h"
#include "ecmascript/js_float64_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/base/error_type.h"

namespace panda::ecmascript::base {
DataViewType TypedArrayHelper::GetType(const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            return DataViewType::INT8;
        case JSType::JS_UINT8_ARRAY:
            return DataViewType::UINT8;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return DataViewType::UINT8_CLAMPED;
        case JSType::JS_INT16_ARRAY:
            return DataViewType::INT16;
        case JSType::JS_UINT16_ARRAY:
            return DataViewType::UINT16;
        case JSType::JS_INT32_ARRAY:
            return DataViewType::INT32;
        case JSType::JS_UINT32_ARRAY:
            return DataViewType::UINT32;
        case JSType::JS_FLOAT32_ARRAY:
            return DataViewType::FLOAT32;
        default:
            return DataViewType::FLOAT64;
    }
}

int32_t TypedArrayHelper::GetElementSize(const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return ElementSize::ONE;
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
            return ElementSize::TWO;
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
            return ElementSize::FOUR;
        default:
            return ElementSize::EIGHT;
    }
}

DataViewType TypedArrayHelper::GetTypeFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt8ArrayString())) {
        return DataViewType::INT8;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint8ArrayString())) {
        return DataViewType::UINT8;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint8ClampedArrayString())) {
        return DataViewType::UINT8_CLAMPED;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt16ArrayString())) {
        return DataViewType::INT16;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint16ArrayString())) {
        return DataViewType::UINT16;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt32ArrayString())) {
        return DataViewType::INT32;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint32ArrayString())) {
        return DataViewType::UINT32;
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledFloat32ArrayString())) {
        return DataViewType::FLOAT32;
    }
    return DataViewType::FLOAT64;
}

JSHandle<JSTaggedValue> TypedArrayHelper::GetConstructor(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSType type = obj->GetTaggedObject()->GetClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            return env->GetInt8ArrayFunction();
        case JSType::JS_UINT8_ARRAY:
            return env->GetUint8ArrayFunction();
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return env->GetUint8ClampedArrayFunction();
        case JSType::JS_INT16_ARRAY:
            return env->GetInt16ArrayFunction();
        case JSType::JS_UINT16_ARRAY:
            return env->GetUint16ArrayFunction();
        case JSType::JS_INT32_ARRAY:
            return env->GetInt32ArrayFunction();
        case JSType::JS_UINT32_ARRAY:
            return env->GetUint32ArrayFunction();
        case JSType::JS_FLOAT32_ARRAY:
            return env->GetFloat32ArrayFunction();
        default:
            return env->GetFloat64ArrayFunction();
    }
}

JSHandle<JSFunction> TypedArrayHelper::GetConstructorFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt8ArrayString())) {
        return JSHandle<JSFunction>(env->GetInt8ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint8ArrayString())) {
        return JSHandle<JSFunction>(env->GetUint8ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint8ClampedArrayString())) {
        return JSHandle<JSFunction>(env->GetUint8ClampedArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt16ArrayString())) {
        return JSHandle<JSFunction>(env->GetInt16ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint16ArrayString())) {
        return JSHandle<JSFunction>(env->GetUint16ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt32ArrayString())) {
        return JSHandle<JSFunction>(env->GetInt32ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint32ArrayString())) {
        return JSHandle<JSFunction>(env->GetUint32ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledFloat32ArrayString())) {
        return JSHandle<JSFunction>(env->GetFloat32ArrayFunction());
    }
    return JSHandle<JSFunction>(env->GetFloat64ArrayFunction());
}

int32_t TypedArrayHelper::GetSizeFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName)
{
    int32_t elementSize;
    auto globalConst = thread->GlobalConstants();
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt8ArrayString()) ||
        JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint8ArrayString()) ||
        JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint8ClampedArrayString())) {
        elementSize = ElementSize::ONE;
    } else if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt16ArrayString()) ||
               JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint16ArrayString())) {
        elementSize = ElementSize::TWO;
    } else if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledInt32ArrayString()) ||
               JSTaggedValue::SameValue(typeName, globalConst->GetHandledUint32ArrayString()) ||
               JSTaggedValue::SameValue(typeName, globalConst->GetHandledFloat32ArrayString())) {
        elementSize = ElementSize::FOUR;
    } else {
        elementSize = ElementSize::EIGHT;
    }
    return elementSize;
}

void TypedArrayHelper::SetViewedArrayBuffer(JSThread *thread, const JSHandle<JSObject> &obj,
                                            JSTaggedValue viewedArrayBuffer)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            JSInt8Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_UINT8_ARRAY:
            JSUint8Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            JSUint8ClampedArray::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_INT16_ARRAY:
            JSInt16Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_UINT16_ARRAY:
            JSUint16Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_INT32_ARRAY:
            JSInt32Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_UINT32_ARRAY:
            JSUint32Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            JSFloat32Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            JSFloat64Array::Cast(*obj)->SetViewedArrayBuffer(thread, viewedArrayBuffer);
            break;
        default:
            break;
    }
}

void TypedArrayHelper::SetTypedArrayName(JSThread *thread, const JSHandle<JSObject> &obj,
                                         const JSHandle<JSTaggedValue> &typedArrayName)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            JSInt8Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_UINT8_ARRAY:
            JSUint8Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            JSUint8ClampedArray::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_INT16_ARRAY:
            JSInt16Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_UINT16_ARRAY:
            JSUint16Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_INT32_ARRAY:
            JSInt32Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_UINT32_ARRAY:
            JSUint32Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            JSFloat32Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            JSFloat64Array::Cast(*obj)->SetTypedArrayName(thread, typedArrayName);
            break;
        default:
            break;
    }
}

void TypedArrayHelper::SetByteLength(JSThread *thread, const JSHandle<JSObject> &obj, int32_t byteLength)
{
    auto byteLengthValue = JSTaggedValue(byteLength);
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            JSInt8Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_UINT8_ARRAY:
            JSUint8Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            JSUint8ClampedArray::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_INT16_ARRAY:
            JSInt16Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_UINT16_ARRAY:
            JSUint16Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_INT32_ARRAY:
            JSInt32Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_UINT32_ARRAY:
            JSUint32Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            JSFloat32Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            JSFloat64Array::Cast(*obj)->SetByteLength(thread, byteLengthValue);
            break;
        default:
            break;
    }
}

void TypedArrayHelper::SetByteOffset(JSThread *thread, const JSHandle<JSObject> &obj, int32_t byteOffset)
{
    auto byteOffsetValue = JSTaggedValue(byteOffset);
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            JSInt8Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_UINT8_ARRAY:
            JSUint8Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            JSUint8ClampedArray::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_INT16_ARRAY:
            JSInt16Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_UINT16_ARRAY:
            JSUint16Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_INT32_ARRAY:
            JSInt32Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_UINT32_ARRAY:
            JSUint32Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            JSFloat32Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            JSFloat64Array::Cast(*obj)->SetByteOffset(thread, byteOffsetValue);
            break;
        default:
            break;
    }
}

void TypedArrayHelper::SetArrayLength(JSThread *thread, const JSHandle<JSObject> &obj, int32_t arrayLength)
{
    auto arrayLengthValue = JSTaggedValue(arrayLength);
    JSType type = obj->GetJSHClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            JSInt8Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_UINT8_ARRAY:
            JSUint8Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            JSUint8ClampedArray::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_INT16_ARRAY:
            JSInt16Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_UINT16_ARRAY:
            JSUint16Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_INT32_ARRAY:
            JSInt32Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_UINT32_ARRAY:
            JSUint32Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_FLOAT32_ARRAY:
            JSFloat32Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        case JSType::JS_FLOAT64_ARRAY:
            JSFloat64Array::Cast(*obj)->SetArrayLength(thread, arrayLengthValue);
            break;
        default:
            break;
    }
}

JSTaggedValue TypedArrayHelper::GetViewedArrayBuffer(const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    JSTaggedValue buffer;
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            buffer = JSInt8Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_UINT8_ARRAY:
            buffer = JSUint8Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            buffer = JSUint8ClampedArray::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_INT16_ARRAY:
            buffer = JSInt16Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_UINT16_ARRAY:
            buffer = JSUint16Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_INT32_ARRAY:
            buffer = JSInt32Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_UINT32_ARRAY:
            buffer = JSUint32Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_FLOAT32_ARRAY:
            buffer = JSFloat32Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        case JSType::JS_FLOAT64_ARRAY:
            buffer = JSFloat64Array::Cast(*obj)->GetViewedArrayBuffer();
            break;
        default:
            break;
    }
    return buffer;
}

JSHandle<JSTaggedValue> TypedArrayHelper::GetTypedArrayName(JSThread *thread, const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    JSTaggedValue name;
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            name = JSInt8Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_UINT8_ARRAY:
            name = JSUint8Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            name = JSUint8ClampedArray::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_INT16_ARRAY:
            name = JSInt16Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_UINT16_ARRAY:
            name = JSUint16Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_INT32_ARRAY:
            name = JSInt32Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_UINT32_ARRAY:
            name = JSUint32Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_FLOAT32_ARRAY:
            name = JSFloat32Array::Cast(*obj)->GetTypedArrayName();
            break;
        case JSType::JS_FLOAT64_ARRAY:
            name = JSFloat64Array::Cast(*obj)->GetTypedArrayName();
            break;
        default:
            break;
    }
    return JSHandle<JSTaggedValue>(thread, name);
}

int32_t TypedArrayHelper::GetByteLength(JSThread *thread, const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    JSTaggedValue length;
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            length = JSInt8Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_UINT8_ARRAY:
            length = JSUint8Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            length = JSUint8ClampedArray::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_INT16_ARRAY:
            length = JSInt16Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_UINT16_ARRAY:
            length = JSUint16Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_INT32_ARRAY:
            length = JSInt32Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_UINT32_ARRAY:
            length = JSUint32Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_FLOAT32_ARRAY:
            length = JSFloat32Array::Cast(*obj)->GetByteLength();
            break;
        case JSType::JS_FLOAT64_ARRAY:
            length = JSFloat64Array::Cast(*obj)->GetByteLength();
            break;
        default:
            break;
    }
    return JSTaggedValue::ToLength(thread, JSHandle<JSTaggedValue>(thread, length)).ToInt32();
}

int32_t TypedArrayHelper::GetByteOffset(JSThread *thread, const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    JSTaggedValue length;
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            length = JSInt8Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_UINT8_ARRAY:
            length = JSUint8Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            length = JSUint8ClampedArray::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_INT16_ARRAY:
            length = JSInt16Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_UINT16_ARRAY:
            length = JSUint16Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_INT32_ARRAY:
            length = JSInt32Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_UINT32_ARRAY:
            length = JSUint32Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_FLOAT32_ARRAY:
            length = JSFloat32Array::Cast(*obj)->GetByteOffset();
            break;
        case JSType::JS_FLOAT64_ARRAY:
            length = JSFloat64Array::Cast(*obj)->GetByteOffset();
            break;
        default:
            break;
    }
    return JSTaggedValue::ToLength(thread, JSHandle<JSTaggedValue>(thread, length)).ToInt32();
}

int32_t TypedArrayHelper::GetArrayLength(JSThread *thread, const JSHandle<JSObject> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    JSTaggedValue length;
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            length = JSInt8Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_UINT8_ARRAY:
            length = JSUint8Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            length = JSUint8ClampedArray::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_INT16_ARRAY:
            length = JSInt16Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_UINT16_ARRAY:
            length = JSUint16Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_INT32_ARRAY:
            length = JSInt32Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_UINT32_ARRAY:
            length = JSUint32Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_FLOAT32_ARRAY:
            length = JSFloat32Array::Cast(*obj)->GetArrayLength();
            break;
        case JSType::JS_FLOAT64_ARRAY:
            length = JSFloat64Array::Cast(*obj)->GetArrayLength();
            break;
        default:
            break;
    }
    return JSTaggedValue::ToLength(thread, JSHandle<JSTaggedValue>(thread, length)).ToInt32();
}
}  // namespace panda::ecmascript::base

#endif  // PANDA_RUNTIME_ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
