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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_TYPEDARRAY_H
#define ECMASCRIPT_BUILTINS_BUILTINS_TYPEDARRAY_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsTypedArray : public base::BuiltinsBase {
public:
    enum SeparatorFlag : int { MINUS_ONE = -1, MINUS_TWO = -2 };
    // 22.2.1
    static JSTaggedValue TypedArrayBaseConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Int8ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint8ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint8ClampedArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Int16ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint16ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Int32ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint32ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Float32ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Float64ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue BigInt64ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue BigUint64ArrayConstructor(EcmaRuntimeCallInfo *argv);

    // 22.2.1.2.1
    static JSTaggedValue AllocateTypedArray(EcmaRuntimeCallInfo *argv);

    // 22.2.2.1
    static JSTaggedValue From(EcmaRuntimeCallInfo *argv);
    // 22.2.2.2
    static JSTaggedValue Of(EcmaRuntimeCallInfo *argv);
    // 22.2.2.4
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);

    // prototype
    // 22.2.3.1
    static JSTaggedValue GetBuffer(EcmaRuntimeCallInfo *argv);
    // 22.2.3.2
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    // 22.2.3.3
    static JSTaggedValue GetByteOffset(EcmaRuntimeCallInfo *argv);
    // 22.2.3.5
    static JSTaggedValue CopyWithin(EcmaRuntimeCallInfo *argv);
    // 22.2.3.6
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    // 22.2.3.7
    static JSTaggedValue Every(EcmaRuntimeCallInfo *argv);
    // 22.2.3.8
    static JSTaggedValue Fill(EcmaRuntimeCallInfo *argv);
    // 22.2.3.9
    static JSTaggedValue Filter(EcmaRuntimeCallInfo *argv);
    // 22.2.3.10
    static JSTaggedValue Find(EcmaRuntimeCallInfo *argv);
    // 22.2.3.11
    static JSTaggedValue FindIndex(EcmaRuntimeCallInfo *argv);
    // 22.2.3.12
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    // 22.2.3.13
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    // 22.2.3.14
    static JSTaggedValue Join(EcmaRuntimeCallInfo *argv);
    // 22.2.3.15
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    // 22.2.3.16
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    // 22.2.3.17
    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);
    // 22.2.3.18
    static JSTaggedValue Map(EcmaRuntimeCallInfo *argv);
    // 22.2.3.19
    static JSTaggedValue Reduce(EcmaRuntimeCallInfo *argv);
    // 22.2.3.20
    static JSTaggedValue ReduceRight(EcmaRuntimeCallInfo *argv);
    // 22.2.3.21
    static JSTaggedValue Reverse(EcmaRuntimeCallInfo *argv);
    // 22.2.3.22
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    // 22.2.3.23
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 22.2.3.24
    static JSTaggedValue Some(EcmaRuntimeCallInfo *argv);
    // 22.2.3.25
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    // 22.2.3.26
    static JSTaggedValue Subarray(EcmaRuntimeCallInfo *argv);
    // 22.2.3.27
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 22.2.3.28
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 22.2.3.29
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    // 22.2.3.31
    static JSTaggedValue ToStringTag(EcmaRuntimeCallInfo *argv);
    // es12 23.2.3.13
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_TYPEDARRAY_H
