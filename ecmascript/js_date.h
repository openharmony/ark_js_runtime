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

#ifndef ECMASCRIPT_JSDATE_H
#define ECMASCRIPT_JSDATE_H

#include <array>

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
static constexpr int64_t DAYS_IN_YEAR = 365;
static constexpr std::array<int, 2> APPROXIMATION_NUMBER = {100000, 3652425};
static constexpr int64_t CHINA_BEFORE_1900_MS = -2177481943000;
static constexpr int64_t CHINA_1901_MS = -2177452800000;
static constexpr int CHINA_BEFORE_1901_ADDMS = 343000;
static constexpr int MS_PER_SECOND = 1000;
static constexpr int SEC_PER_MINUTE = 60;
static constexpr int MIN_PER_HOUR = 60;
static constexpr int MS_PER_HOUR = 3600 * 1000;
static constexpr int MS_PER_DAY = 86400000;
static constexpr int DAY_PER_WEEK = 7;
static constexpr int DATE_LENGTH = 9;
// the index in the Date Fields
static constexpr uint8_t YEAR = 0;
static constexpr uint8_t MONTH = 1;
static constexpr uint8_t DAYS = 2;
static constexpr uint8_t HOUR = 3;
static constexpr uint8_t MIN = 4;
static constexpr uint8_t SEC = 5;
static constexpr uint8_t MS = 6;
static constexpr uint8_t WEEKDAY = 7;
static constexpr uint8_t TIMEZONE = 8;
static constexpr int CHINA_BEFORE_1901_MIN = 485;
static constexpr int CHINA_AFTER_1901_MIN = 480;
static constexpr int CHINA_BEFORE_1901_MS = 343000;
static constexpr std::array<int, 3> LEAP_NUMBER = {4, 100, 400};
static constexpr std::array<int, 4> YEAR_NUMBER = {1970, 1969, 1901, 1601};

class DateUtils {
public:
    static void TransferTimeToDate(int64_t timeMs, std::array<int64_t, DATE_LENGTH> *date);
    static int64_t Mod(int64_t a, int b);
    static bool IsLeap(int64_t year);
    static int64_t GetDaysInYear(int64_t year);
    static int64_t GetDaysFromYear(int64_t year);
    // return the year, update days.
    static int64_t GetYearFromDays(int64_t *days);
    static int64_t FloorDiv(int64_t a, int64_t b);
};
class JSDate : public JSObject {
public:
    CAST_CHECK(JSDate, IsDate);

    static constexpr size_t TIME_VALUE_OFFSET = JSObject::SIZE;
    ACCESSORS(TimeValue, TIME_VALUE_OFFSET, LOCAL_TIME_OFFSET)
    ACCESSORS(LocalOffset, LOCAL_TIME_OFFSET, SIZE)  // localoffset in min

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, TIME_VALUE_OFFSET, SIZE)

    static double MakeDay(double year, double month, double date);
    static double MakeTime(double hour, double min, double sec, double ms);
    static double MakeDate(double day, double time);
    static double TimeClip(double time);
    static JSTaggedValue LocalParseStringToMs(const CString &str);
    static JSTaggedValue UtcParseStringToMs(const CString &str);
    static JSTaggedValue IsoParseStringToMs(const CString &str);
    static int GetSignedNumFromString(const CString &str, int len, int *index);
    static bool GetNumFromString(const CString &str, int len, int *index, int *num);

    // 20.4.1.7
    int64_t GetLocalOffsetInMin(const JSThread *thread, int64_t timeMs, bool isLocal);

    // 20.4.1.8
    double LocalTime(double timeMs) const;

    // 20.4.1.9
    double UTCTime(double timeMs) const;

    // 20.4.3.1
    static JSTaggedValue Now();

    // 20.4.3.2
    static JSTaggedValue Parse(EcmaRuntimeCallInfo *argv);

    // 20.4.3.4
    static JSTaggedValue UTC(EcmaRuntimeCallInfo *argv);

    // 20.4.4.10
    JSTaggedValue GetTime() const;

    // 20.4.4.19
    JSTaggedValue GetUTCSeconds();

    // 20.4.4.35
    JSTaggedValue ToDateString(JSThread *thread) const;
    static CString ToDateString(double timeMs);

    // 20.4.4.36
    JSTaggedValue ToISOString(JSThread *thread) const;

    // 20.4.4.38
    JSTaggedValue ToLocaleDateString(JSThread *thread) const;

    // 20.4.4.39
    JSTaggedValue ToLocaleString(JSThread *thread) const;

    // 20.4.4.40
    JSTaggedValue ToLocaleTimeString(JSThread *thread) const;

    // 20.4.4.41
    JSTaggedValue ToString(JSThread *thread) const;

    // 20.4.4.42
    JSTaggedValue ToTimeString(JSThread *thread) const;

    // 20.4.4.43
    JSTaggedValue ToUTCString(JSThread *thread) const;

    // 20.4.4.44
    JSTaggedValue ValueOf() const;

    JSTaggedValue SetDateValue(EcmaRuntimeCallInfo *argv, uint32_t code, bool isLocal) const;
    double GetDateValue(double timeMs, uint8_t code, bool isLocal) const;

    static constexpr double MAX_DOUBLE = std::numeric_limits<double>::max();
    static constexpr double MAX_INT = std::numeric_limits<int>::max();
    static constexpr uint16_t NINETEEN_HUNDRED_YEAR = 1900;
    static constexpr uint16_t HUNDRED = 100;
    static constexpr uint16_t THOUSAND = 1000;
    static double SetDateValues(const std::array<int64_t, DATE_LENGTH> *date, bool isLocal);
    static void GetDateValues(double timeMs, std::array<int64_t, DATE_LENGTH> *date, bool isLocal);
    static int64_t GetLocalOffsetFromOS(int64_t timeMs, bool isLocal);
    static CString StrToTargetLength(const CString &str, int length);
    DECL_DUMP()

private:
    bool GetThisDateValues(std::array<int64_t, DATE_LENGTH> *date, bool isLocal) const;
    CString GetLocaleTimeStr(const std::array<int64_t, DATE_LENGTH> &fields) const;
    CString GetLocaleDateStr(const std::array<int64_t, DATE_LENGTH> &fields) const;
    static int64_t MathMod(int64_t a, int b);

    static constexpr int MINUTE_PER_HOUR = 60;
    static constexpr int MOUTH_PER_YEAR = 12;
    static constexpr std::array<int, 12> MONTH_DAYS = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
    static constexpr int  DAYS_FROM_MONTH [2][13] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
    };
    static constexpr int STR_LENGTH_YEAR = 4;
    static constexpr int STR_LENGTH_OTHERS = 2;
    static constexpr int MONTH_PER_YEAR = 12;
    static constexpr int YEAR_DELTA = 399999;
    static constexpr int LINE_YEAR = 1970;
    static constexpr int CENTURY = 100;
    static constexpr char NEG = '-';
    static constexpr char PLUS = '+';
    static constexpr char SPACE = ' ';
    static constexpr char COLON = ':';
    static constexpr char POINT = '.';
    static constexpr int LENGTH_MONTH_NAME = 3;
    static constexpr int MS_PER_MINUTE = 60000;
    static constexpr int64_t MAX_TIME_IN_MS = static_cast<int64_t>(864000000) * 10000000;
    static constexpr int TEN = 10;
    static constexpr char FLAG_TIME = 'T';
    static constexpr char FLAG_UTC = 'Z';
    static constexpr char VIRGULE = '/';
    static constexpr char COMMA = ',';
    static constexpr int LENGTH_PER_TIME = 3;
    static constexpr int MIN_LENGTH = 10;
    static constexpr int INDEX_PLUS_NEG = 6;
    static constexpr int NUM_NINE = 9;
    static constexpr int ORIGIN_YEAR = 1901;

    static constexpr uint32_t CODE_FLAG = 0x0FULL;
    static constexpr size_t CODE_4_BIT = 4;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSDATE_H
