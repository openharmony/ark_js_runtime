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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_GLOBAL_H
#define ECMASCRIPT_BUILTINS_BUILTINS_GLOBAL_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::builtins {
static constexpr uint8_t BIT_MASK = 0x0F;
static constexpr uint8_t BIT_MASK_FF = 0xFF;
static constexpr uint16_t BIT16_MASK = 0x3FF;
static constexpr uint8_t BIT_MASK_ONE = 0x80;
static constexpr uint8_t BIT_MASK_TWO = 0xC0;
using judgURIFunc = bool (*)(uint16_t);

class BuiltinsGlobal : public base::BuiltinsBase {
public:
    // 18.2.1
    static JSTaggedValue NotSupportEval(EcmaRuntimeCallInfo *msg);
    // 18.2.2
    static JSTaggedValue IsFinite(EcmaRuntimeCallInfo *msg);
    // 18.2.3
    static JSTaggedValue IsNaN(EcmaRuntimeCallInfo *msg);
    // 18.2.6
    static JSTaggedValue DecodeURI(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue EncodeURI(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue DecodeURIComponent(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue EncodeURIComponent(EcmaRuntimeCallInfo *msg);

    static JSTaggedValue PrintEntrypoint(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue CallJsBoundFunction(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue CallJsProxy(EcmaRuntimeCallInfo *msg);
#if ECMASCRIPT_ENABLE_RUNTIME_STAT
    static JSTaggedValue StartRuntimeStat(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue StopRuntimeStat(EcmaRuntimeCallInfo *msg);
#endif

private:
    static void PrintString(JSThread *thread, EcmaString *string);
    static void PrintValue(int64_t value, int64_t tag);
    static JSTaggedValue Encode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet);
    static JSTaggedValue Decode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet);
    static bool IsUnescapedURI(uint16_t ch);
    static bool IsInUnescapedURISet(uint16_t ch);
    static bool IsInReservedURISet(uint16_t ch);
    static bool IsReservedURI(uint16_t ch);
    static bool IsInMarkURISet(uint16_t ch);
    static bool IsHexDigits(uint16_t ch);
    static uint8_t GetValueFromTwoHex(uint16_t front, uint16_t behind);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ERROR_H
