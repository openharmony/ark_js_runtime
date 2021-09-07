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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_DATE_H
#define ECMASCRIPT_BUILTINS_BUILTINS_DATE_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_date.h"

namespace panda::ecmascript::builtins {
class BuiltinsDate : public base::BuiltinsBase {
public:
    // 20.4.2 The Date Constructor
    static JSTaggedValue DateConstructor(EcmaRuntimeCallInfo *argv);

    // 20.4.3.1 Date.now ( )
    static JSTaggedValue Now(EcmaRuntimeCallInfo *argv);

    // 20.4.3.4 Date.UTC ( year [ , month [ , date [ , hours [ , minutes [ , seconds [ , ms ] ] ] ] ] ] )
    static JSTaggedValue UTC(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Parse(EcmaRuntimeCallInfo *argv);

    // 20.4.4.2 Date.prototype.getDate ( )
    GET_DATE_VALUE(GetDate, DAYS, true);

    // 20.4.4.3 Date.prototype.getDay ( )
    GET_DATE_VALUE(GetDay, WEEKDAY, true);

    // 20.4.4.4 Date.prototype.getFullYear ( )
    GET_DATE_VALUE(GetFullYear, YEAR, true);

    // 20.4.4.5 Date.prototype.getHours ( )
    GET_DATE_VALUE(GetHours, HOUR, true);

    // 20.4.4.6 Date.prototype.getMilliseconds ( )
    GET_DATE_VALUE(GetMilliseconds, MS, true);

    // 20.4.4.7 Date.prototype.getMinutes ( )
    GET_DATE_VALUE(GetMinutes, MIN, true);

    // 20.4.4.8 Date.prototype.getMonth ( )
    GET_DATE_VALUE(GetMonth, MONTH, true);

    // 20.4.4.9 Date.prototype.getSeconds ( )
    GET_DATE_VALUE(GetSeconds, SEC, true);

    // 20.4.4.10 Date.prototype.getTime ( )
    static JSTaggedValue GetTime(EcmaRuntimeCallInfo *argv);

    // 20.4.4.11 Date.prototype.getTimezoneOffset ( )
    GET_DATE_VALUE(GetTimezoneOffset, TIMEZONE, true);

    // 20.4.4.12 Date.prototype.getUTCDate ( )
    GET_DATE_VALUE(GetUTCDate, DAYS, false);

    // 20.4.4.13 Date.prototype.getUTCDay ( )
    GET_DATE_VALUE(GetUTCDay, WEEKDAY, false);

    // 20.4.4.14 Date.prototype.getUTCFullYear ( )
    GET_DATE_VALUE(GetUTCFullYear, YEAR, false);

    // 20.4.4.15 Date.prototype.getUTCHours ( )
    GET_DATE_VALUE(GetUTCHours, HOUR, false);

    // 20.4.4.16 Date.prototype.getUTCMilliseconds ( )
    GET_DATE_VALUE(GetUTCMilliseconds, MS, false);

    // 20.4.4.17 Date.prototype.getUTCMinutes ( )
    GET_DATE_VALUE(GetUTCMinutes, MIN, false);

    // 20.4.4.18 Date.prototype.getUTCMonth ( )
    GET_DATE_VALUE(GetUTCMonth, MONTH, false);

    // 20.4.4.19 Date.prototype.getUTCSeconds ( )
    GET_DATE_VALUE(GetUTCSeconds, SEC, false);

    // 20.3.4.20 Date.prototype.setDate ( date )
    SET_DATE_VALUE(SetDate, CODE_SET_DATE, true);

    // 20.3.4.21 Date.prototype.setFullYear ( year [ , month [ , date ] ] )
    SET_DATE_VALUE(SetFullYear, CODE_SET_FULL_YEAR, true);

    // 20.3.4.22 Date.prototype.setHours ( hour [ , min [ , sec [ , ms ] ] ] )
    SET_DATE_VALUE(SetHours, CODE_SET_HOURS, true);

    // 20.3.4.23 Date.prototype.setMilliseconds ( ms )
    SET_DATE_VALUE(SetMilliseconds, CODE_SET_MILLISECONDS, true);

    // 20.3.4.24 Date.prototype.setMinutes ( min [ , sec [ , ms ] ] )
    SET_DATE_VALUE(SetMinutes, CODE_SET_MINUTES, true);

    // 20.3.4.25 Date.prototype.setMonth ( month [ , date ] )
    SET_DATE_VALUE(SetMonth, CODE_SET_MONTH, true);

    // 20.3.4.26 Date.prototype.setSeconds ( sec [ , ms ] )
    SET_DATE_VALUE(SetSeconds, CODE_SET_SECONDS, true);

    // 20.3.4.27 Date.prototype.setTime ( time )
    static JSTaggedValue SetTime(EcmaRuntimeCallInfo *argv);

    // 20.3.4.28 Date.prototype.setUTCDate ( date )
    SET_DATE_VALUE(SetUTCDate, CODE_SET_DATE, false);

    // 20.3.4.29 Date.prototype.setUTCFullYear ( year [ , month [ , date ] ] )
    SET_DATE_VALUE(SetUTCFullYear, CODE_SET_FULL_YEAR, false);

    // 20.3.4.30 Date.prototype.setUTCHours ( hour [ , min [ , sec [ , ms ] ] ] )
    SET_DATE_VALUE(SetUTCHours, CODE_SET_HOURS, false);

    // 20.3.4.31 Date.prototype.setUTCMilliseconds ( ms )
    SET_DATE_VALUE(SetUTCMilliseconds, CODE_SET_MILLISECONDS, false);

    // 20.3.4.32 Date.prototype.setUTCMinutes ( min [ , sec [, ms ] ] )
    SET_DATE_VALUE(SetUTCMinutes, CODE_SET_MINUTES, false);

    // 20.3.4.33 Date.prototype.setUTCMonth ( month [ , date ] )
    SET_DATE_VALUE(SetUTCMonth, CODE_SET_MONTH, false);

    // 20.3.4.34 Date.prototype.setUTCSeconds ( sec [ , ms ] )
    SET_DATE_VALUE(SetUTCSeconds, CODE_SET_SECONDS, false);

    // 20.4.4.35 Date.prototype.toDateString ( )
    DATE_STRING(ToDateString);

    // 20.4.4.36 Date.prototype.toISOString ( )
    DATE_TO_STRING(ToISOString);

    // 20.4.4.37 Date.prototype.toJSON ( key )
    static JSTaggedValue ToJSON(EcmaRuntimeCallInfo *argv);

    // 20.4.4.38 Date.prototype.toLocaleDateString ( [ reserved1 [ , reserved2 ] ] )
    static JSTaggedValue ToLocaleDateString(EcmaRuntimeCallInfo *argv);

    // 20.4.4.39 Date.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] )
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);

    // 20.4.4.40 Date.prototype.toLocaleTimeString ( [ reserved1 [ , reserved2 ] ] )
    static JSTaggedValue ToLocaleTimeString(EcmaRuntimeCallInfo *argv);

    // 20.4.4.41 Date.prototype.toString ( )
    DATE_STRING(ToString);

    // 20.4.4.42 Date.prototype.toTimeString ( )
    DATE_STRING(ToTimeString);

    // 20.4.4.43 Date.prototype.toUTCString ( )
    DATE_STRING(ToUTCString);

    // 20.4.4.44 Date.prototype.valueOf ( )
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);

    // 20.4.4.45 Date.prototype [ @@toPrimitive ]
    static JSTaggedValue ToPrimitive(EcmaRuntimeCallInfo *argv);

private:
    // definition for set data code.
    static constexpr uint32_t CODE_SET_DATE = 0x32;
    static constexpr uint32_t CODE_SET_MILLISECONDS = 0x76;
    static constexpr uint32_t CODE_SET_SECONDS = 0x75;
    static constexpr uint32_t CODE_SET_MINUTES = 0x74;
    static constexpr uint32_t CODE_SET_HOURS = 0x73;
    static constexpr uint32_t CODE_SET_MONTH = 0x31;
    static constexpr uint32_t CODE_SET_FULL_YEAR = 0x30;
    static constexpr uint8_t CONSTRUCTOR_MAX_LENGTH = 7;
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_DATE_H
