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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ARRAY_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ARRAY_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
static constexpr uint8_t INDEX_TWO = 2;
static constexpr uint8_t INDEX_THREE = 3;
class BuiltinsArray : public base::BuiltinsBase {
public:
    // 22.1.1
    static JSTaggedValue ArrayConstructor(EcmaRuntimeCallInfo *argv);

    // 22.1.2.1
    static JSTaggedValue From(EcmaRuntimeCallInfo *argv);
    // 22.1.2.2
    static JSTaggedValue IsArray(EcmaRuntimeCallInfo *argv);
    // 22.1.2.3
    static JSTaggedValue Of(EcmaRuntimeCallInfo *argv);
    // 22.1.2.5
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);

    // prototype
    // 22.1.3.1
    static JSTaggedValue Concat(EcmaRuntimeCallInfo *argv);
    // 22.1.3.3
    static JSTaggedValue CopyWithin(EcmaRuntimeCallInfo *argv);
    // 22.1.3.4
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    // 22.1.3.5
    static JSTaggedValue Every(EcmaRuntimeCallInfo *argv);
    // 22.1.3.6
    static JSTaggedValue Fill(EcmaRuntimeCallInfo *argv);
    // 22.1.3.7
    static JSTaggedValue Filter(EcmaRuntimeCallInfo *argv);
    // 22.1.3.8
    static JSTaggedValue Find(EcmaRuntimeCallInfo *argv);
    // 22.1.3.9
    static JSTaggedValue FindIndex(EcmaRuntimeCallInfo *argv);
    // 22.1.3.10
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    // 22.1.3.11
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    // 22.1.3.12
    static JSTaggedValue Join(EcmaRuntimeCallInfo *argv);
    // 22.1.3.13
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    // 22.1.3.14
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    // 22.1.3.15
    static JSTaggedValue Map(EcmaRuntimeCallInfo *argv);
    // 22.1.3.16
    static JSTaggedValue Pop(EcmaRuntimeCallInfo *argv);
    // 22.1.3.17
    static JSTaggedValue Push(EcmaRuntimeCallInfo *argv);
    // 22.1.3.18
    static JSTaggedValue Reduce(EcmaRuntimeCallInfo *argv);
    // 22.1.3.19
    static JSTaggedValue ReduceRight(EcmaRuntimeCallInfo *argv);
    // 22.1.3.20
    static JSTaggedValue Reverse(EcmaRuntimeCallInfo *argv);
    // 22.1.3.21
    static JSTaggedValue Shift(EcmaRuntimeCallInfo *argv);
    // 22.1.3.22
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 22.1.3.23
    static JSTaggedValue Some(EcmaRuntimeCallInfo *argv);
    // 22.1.3.24
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    // 22.1.3.25
    static JSTaggedValue Splice(EcmaRuntimeCallInfo *argv);
    // 22.1.3.26
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 22.1.3.27
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 22.1.3.28
    static JSTaggedValue Unshift(EcmaRuntimeCallInfo *argv);
    // 22.1.3.29
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    // 22.1.3.31
    static JSTaggedValue Unscopables(EcmaRuntimeCallInfo *argv);
    // es12 23.1.3.13
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
    // es12 23.1.3.10
    static JSTaggedValue Flat(EcmaRuntimeCallInfo *argv);
    // es12 23.1.3.11
    static JSTaggedValue FlatMap(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ARRAY_H