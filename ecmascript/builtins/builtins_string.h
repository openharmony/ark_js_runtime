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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_STRING_H
#define ECMASCRIPT_BUILTINS_BUILTINS_STRING_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::builtins {
constexpr int32_t ENCODE_MAX_UTF16 = 0X10FFFF;
constexpr uint16_t ENCODE_LEAD_LOW = 0xD800;
constexpr uint16_t ENCODE_TRAIL_LOW = 0xDC00;
constexpr uint32_t ENCODE_FIRST_FACTOR = 0x400;
constexpr uint32_t ENCODE_SECOND_FACTOR = 0x10000;
constexpr double DOUBLE_INT_MAX = static_cast<double>(INT_MAX);
constexpr double DOUBLE_INT_MIN = static_cast<double>(INT_MIN);

class BuiltinsString : public base::BuiltinsBase {
public:
    // 21.1.1.1
    static JSTaggedValue StringConstructor(EcmaRuntimeCallInfo *argv);
    // 21.1.2.1
    static JSTaggedValue FromCharCode(EcmaRuntimeCallInfo *argv);
    // 21.1.2.2
    static JSTaggedValue FromCodePoint(EcmaRuntimeCallInfo *argv);
    // 21.1.2.4
    static JSTaggedValue Raw(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetSubstitution(JSThread *thread, const JSHandle<EcmaString> &matched,
                                         const JSHandle<EcmaString> &srcString, int position,
                                         const JSHandle<TaggedArray> &captureList,
                                         const JSHandle<EcmaString> &replacement);
    // 21.1.3.1
    static JSTaggedValue CharAt(EcmaRuntimeCallInfo *argv);
    // 21.1.3.2
    static JSTaggedValue CharCodeAt(EcmaRuntimeCallInfo *argv);
    // 21.1.3.3
    static JSTaggedValue CodePointAt(EcmaRuntimeCallInfo *argv);
    // 21.1.3.4
    static JSTaggedValue Concat(EcmaRuntimeCallInfo *argv);
    // 21.1.3.5 String.prototype.constructor
    // 21.1.3.6
    static JSTaggedValue EndsWith(EcmaRuntimeCallInfo *argv);
    // 21.1.3.7
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
    // 21.1.3.8
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    // 21.1.3.9
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    // 21.1.3.10
    static JSTaggedValue LocaleCompare(EcmaRuntimeCallInfo *argv);
    // 21.1.3.11
    static JSTaggedValue Match(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue MatchAll(EcmaRuntimeCallInfo *argv);
    // 21.1.3.12
    static JSTaggedValue Normalize(EcmaRuntimeCallInfo *argv);
    
    static JSTaggedValue PadStart(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue PadEnd(EcmaRuntimeCallInfo *argv);
    // 21.1.3.13
    static JSTaggedValue Repeat(EcmaRuntimeCallInfo *argv);
    // 21.1.3.14
    static JSTaggedValue Replace(EcmaRuntimeCallInfo *argv);
    // 21.1.3.14.1 Runtime Semantics: GetSubstitution()
    // 21.1.3.15
    static JSTaggedValue Search(EcmaRuntimeCallInfo *argv);
    // 21.1.3.16
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 21.1.3.17
    static JSTaggedValue Split(EcmaRuntimeCallInfo *argv);
    // 21.1.3.17.1 Runtime Semantics: SplitMatch
    // 21.1.3.18
    static JSTaggedValue StartsWith(EcmaRuntimeCallInfo *argv);
    // 21.1.3.19
    static JSTaggedValue Substring(EcmaRuntimeCallInfo *argv);
    // 21.1.3.20
    static JSTaggedValue ToLocaleLowerCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.21
    static JSTaggedValue ToLocaleUpperCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.22
    static JSTaggedValue ToLowerCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.23
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 21.1.3.24
    static JSTaggedValue ToUpperCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.25
    static JSTaggedValue Trim(EcmaRuntimeCallInfo *argv);
    
    static JSTaggedValue TrimStart(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimEnd(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimLeft(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimRight(EcmaRuntimeCallInfo *argv);
    // 21.1.3.26
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);
    // 21.1.3.27
    static JSTaggedValue GetStringIterator(EcmaRuntimeCallInfo *argv);
    // 21.1.3
    static JSTaggedValue ThisStringValue(JSThread *thread, JSTaggedValue value);
    // 21.1.2.27
    static JSTaggedValue CreateIterator(EcmaRuntimeCallInfo *argv);
    // 10.1.2
    static uint16_t UTF16Decode(uint16_t lead, uint16_t trail);
    // annexB B.2.3.1
    static JSTaggedValue SubStr(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);

private:
    static int32_t ConvertDoubleToInt(double d);
    // 21.1.3.17.1
    static int32_t SplitMatch(const JSHandle<EcmaString> &str, int32_t q, const JSHandle<EcmaString> &reg);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_STRING_H
