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

#ifndef ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
#define ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H

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
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/base/error_type.h"

namespace panda::ecmascript::base {
DataViewType TypedArrayHelper::GetType(const JSHandle<JSTypedArray> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    return GetType(type);
}

DataViewType TypedArrayHelper::GetType(JSType type)
{
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
        case JSType::JS_FLOAT64_ARRAY:
            return DataViewType::FLOAT64;
        case JSType::JS_BIGINT64_ARRAY:
            return DataViewType::BIGINT64;
        default:
            return DataViewType::BIGUINT64;
    }
}

uint32_t TypedArrayHelper::GetElementSize(const JSHandle<JSTypedArray> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    return GetElementSize(type);
}

uint32_t TypedArrayHelper::GetElementSize(JSType type)
{
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
        case JSType::JS_FLOAT64_ARRAY:
            return env->GetFloat64ArrayFunction();
        case JSType::JS_BIGINT64_ARRAY:
            return env->GetBigInt64ArrayFunction();
        default:
            return env->GetBigUint64ArrayFunction();
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
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledFloat64ArrayString())) {
        return JSHandle<JSFunction>(env->GetFloat64ArrayFunction());
    }
    if (JSTaggedValue::SameValue(typeName, globalConst->GetHandledBigInt64ArrayString())) {
        return JSHandle<JSFunction>(env->GetBigInt64ArrayFunction());
    }
    return JSHandle<JSFunction>(env->GetBigUint64ArrayFunction());
}

uint32_t TypedArrayHelper::GetSizeFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName)
{
    uint32_t elementSize;
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
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
