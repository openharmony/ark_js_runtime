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

#ifndef ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_H
#define ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_dataview.h"
#include <limits>
#include <string>

namespace panda::ecmascript::base {
enum ElementSize : uint8_t { ONE = 1, TWO = 2, FOUR = 4, EIGHT = 8 };
class TypedArrayHelper {
public:
    static JSTaggedValue TypedArrayConstructor(EcmaRuntimeCallInfo *argv,
                                               const JSHandle<JSTaggedValue> &constructorName);
    static JSHandle<JSObject> AllocateTypedArray(ObjectFactory *factory, EcmaVM *ecmaVm,
                                                 const JSHandle<JSTaggedValue> &constructorName,
                                                 const JSHandle<JSTaggedValue> &newTarget);
    static JSHandle<JSObject> AllocateTypedArray(ObjectFactory *factory, EcmaVM *ecmaVm,
                                                 const JSHandle<JSTaggedValue> &constructorName,
                                                 const JSHandle<JSTaggedValue> &newTarget, int32_t length);
    static JSHandle<JSObject> TypedArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &obj,
                                                      uint32_t argc, JSTaggedType argv[]);
    static JSHandle<JSObject> TypedArrayCreate(JSThread *thread, const JSHandle<JSTaggedValue> &constructor,
                                               uint32_t argc, const JSTaggedType argv[]);
    static JSTaggedValue ValidateTypedArray(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    inline static DataViewType GetType(const JSHandle<JSObject> &obj);
    inline static int32_t GetElementSize(const JSHandle<JSObject> &obj);
    inline static DataViewType GetTypeFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName);
    inline static JSHandle<JSTaggedValue> GetConstructor(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    inline static JSHandle<JSFunction> GetConstructorFromName(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &typeName);
    inline static int32_t GetSizeFromName(JSThread *thread, const JSHandle<JSTaggedValue> &typeName);
    inline static int32_t GetByteOffset(JSThread *thread, const JSHandle<JSObject> &obj);
    inline static int32_t GetArrayLength(JSThread *thread, const JSHandle<JSObject> &obj);
    static int32_t SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                               const JSHandle<JSTaggedValue> &buffer, const JSHandle<JSTaggedValue> &firstValue,
                               const JSHandle<JSTaggedValue> &secondValue);

private:
    static JSTaggedValue CreateFromOrdinaryObject(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj);
    static JSTaggedValue CreateFromTypedArray(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                              const JSHandle<JSTaggedValue> &constructorName);
    static JSTaggedValue CreateFromArrayBuffer(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                               const JSHandle<JSTaggedValue> &constructorName);
    static JSHandle<JSObject> AllocateTypedArrayBuffer(JSThread *thread, EcmaVM *ecmaVm, const JSHandle<JSObject> &obj,
                                                       double length);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_H
