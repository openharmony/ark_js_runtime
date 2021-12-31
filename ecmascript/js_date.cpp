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

#include "js_date.h"
#include <ctime>
#include <regex>
#include <sys/time.h>
#include "base/builtins_base.h"

namespace panda::ecmascript {
using NumberHelper = base::NumberHelper;
void DateUtils::TransferTimeToDate(int64_t timeMs, std::array<int64_t, DATE_LENGTH> *date)
{
    (*date)[HOUR] = Mod(timeMs, MS_PER_DAY);                                 // ms from hour, minutes, second, ms
    (*date)[DAYS] = (timeMs - (*date)[HOUR]) / MS_PER_DAY;                   // days from year, month, day
    (*date)[MS] = (*date)[HOUR] % MS_PER_SECOND;                             // ms
    (*date)[HOUR] = ((*date)[HOUR] - (*date)[MS]) / MS_PER_SECOND;           // s from hour, minutes, second
    (*date)[SEC] = (*date)[HOUR] % SEC_PER_MINUTE;                           // second
    (*date)[HOUR] = ((*date)[HOUR] - (*date)[SEC]) / SEC_PER_MINUTE;         // min from hour, minutes
    (*date)[MIN] = (*date)[HOUR] % SEC_PER_MINUTE;                           // min
    (*date)[HOUR] = ((*date)[HOUR] - (*date)[MIN]) / SEC_PER_MINUTE;         // hour
    (*date)[WEEKDAY] = Mod(((*date)[DAYS] + LEAP_NUMBER[0]), DAY_PER_WEEK);  // weekday
    (*date)[YEAR] = GetYearFromDays(&((*date)[DAYS]));                       // year
}
// static
bool DateUtils::IsLeap(int64_t year)
{
    return year % LEAP_NUMBER[0] == 0 && (year % LEAP_NUMBER[1] != 0 || year % LEAP_NUMBER[2] == 0);  // 2: means index
}

// static
int64_t DateUtils::GetDaysInYear(int64_t year)
{
    int64_t number;
    number = IsLeap(year) ? (DAYS_IN_YEAR + 1) : DAYS_IN_YEAR;
    return number;
}

// static
int64_t DateUtils::GetDaysFromYear(int64_t year)
{
    return DAYS_IN_YEAR * (year - YEAR_NUMBER[0]) + FloorDiv(year - YEAR_NUMBER[1], LEAP_NUMBER[0]) -
           FloorDiv(year - YEAR_NUMBER[2], LEAP_NUMBER[1]) +  // 2: year index
           FloorDiv(year - YEAR_NUMBER[3], LEAP_NUMBER[2]);   // 3, 2: year index
}

// static
int64_t DateUtils::FloorDiv(int64_t a, int64_t b)
{
    ASSERT(b != 0);
    int64_t m = a % b;
    int64_t res = m < 0 ? ((a - m - b) / b) : ((a - m) / b);
    return res;
}

// static
int64_t DateUtils::GetYearFromDays(int64_t *days)
{
    int64_t realDay;
    int64_t dayTemp = 0;
    int64_t d = *days;
    int64_t year = FloorDiv(d * APPROXIMATION_NUMBER[0], APPROXIMATION_NUMBER[1]) + YEAR_NUMBER[0];
    realDay = d - GetDaysFromYear(year);
    while (realDay != 0) {
        if (realDay < 0) {
            year--;
        } else {
            dayTemp = GetDaysInYear(year);
            if (realDay < dayTemp) {
                break;
            }
            year++;
        }
        realDay = d - GetDaysFromYear(year);
    }
    *days = realDay;
    return year;
}

// static
int64_t DateUtils::Mod(int64_t a, int b)
{
    ASSERT(b != 0);
    int64_t m = a % b;
    int64_t res = m < 0 ? (m + b) : m;
    return res;
}

// static
// 20.4.1.11
double JSDate::MakeTime(double hour, double min, double sec, double ms)
{
    if (std::isfinite(hour) && std::isfinite(min) && std::isfinite(sec) && std::isfinite(ms)) {
        double hourInteger = NumberHelper::TruncateDouble(hour);
        double minInteger = NumberHelper::TruncateDouble(min);
        double secInteger = NumberHelper::TruncateDouble(sec);
        double msInteger = NumberHelper::TruncateDouble(ms);
        return hourInteger * MS_PER_HOUR + minInteger * MS_PER_MINUTE + secInteger * MS_PER_SECOND + msInteger;
    }
    return base::NAN_VALUE;
}

// static
// 20.4.1.12
double JSDate::MakeDay(double year, double month, double date)
{
    if (std::isfinite(year) && std::isfinite(month) && std::isfinite(date)) {
        double yearInteger = NumberHelper::TruncateDouble(year);
        double monthInteger = NumberHelper::TruncateDouble(month);
        int64_t y = static_cast<int64_t>(yearInteger) + static_cast<int64_t>(monthInteger / MOUTH_PER_YEAR);
        int64_t m = static_cast<int64_t>(monthInteger) % MOUTH_PER_YEAR;
        if (m < 0) {
            m += MOUTH_PER_YEAR;
            y -= 1;
        }

        int64_t days = DateUtils::GetDaysFromYear(y);
        int index = DateUtils::IsLeap(year) ? 1 : 0;
        days += DAYS_FROM_MONTH[index][m];
        return static_cast<double>(days - 1) + NumberHelper::TruncateDouble(date);
    }
    return base::NAN_VALUE;
}

// static
// 20.4.1.13
double JSDate::MakeDate(double day, double time)
{
    if (std::isfinite(day) && std::isfinite(time)) {
        return time + day * MS_PER_DAY;
    }
    return base::NAN_VALUE;
}

// static
// 20.4.1.14
double JSDate::TimeClip(double time)
{
    if (-MAX_TIME_IN_MS <= time && time <= MAX_TIME_IN_MS) {
        return NumberHelper::TruncateDouble(time);
    }
    return base::NAN_VALUE;
}

// 20.4.1.8
double JSDate::LocalTime(double timeMs) const
{
    return timeMs + GetLocalOffsetFromOS(timeMs, true);
}

// 20.4.1.9
double JSDate::UTCTime(double timeMs) const
{
    return timeMs - GetLocalOffsetFromOS(timeMs, false);
}

// static
int JSDate::GetSignedNumFromString(const CString &str, int len, int *index)
{
    int res = 0;
    GetNumFromString(str, len, index, &res);
    if (str.at(0) == NEG) {
        return -res;
    }
    return res;
}

// static
bool JSDate::GetNumFromString(const CString &str, int len, int *index, int *num)
{
    int indexStr = *index;
    char oneByte = 0;
    while (indexStr < len) {
        oneByte = str.at(indexStr);
        if (oneByte >= '0' && oneByte <= '9') {
            break;
        }
        indexStr++;
    }
    if (indexStr >= len) {
        return false;
    }
    int value = 0;
    while (indexStr < len) {
        oneByte = str.at(indexStr);
        int val = oneByte - '0';
        if (val >= 0 && val <= NUM_NINE) {
            value = value * TEN + val;
            indexStr++;
        } else {
            break;
        }
    }
    *num = value;
    *index = indexStr;
    return true;
}

// 20.4.1.7
int64_t JSDate::GetLocalOffsetInMin(const JSThread *thread, int64_t timeMs, bool isLocal)
{
    if (!isLocal) {
        return 0;
    }
    double localOffset = this->GetLocalOffset().GetDouble();
    if (localOffset == MAX_DOUBLE) {
        localOffset = static_cast<double>(GetLocalOffsetFromOS(timeMs, isLocal));
        SetLocalOffset(thread, JSTaggedValue(localOffset));
    }
    return localOffset;
}

// static
JSTaggedValue JSDate::LocalParseStringToMs(const CString &str)
{
    int year = 0;
    int month = 0;
    int date = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    int index = 0;
    int len = str.length();
    bool isLocal = false;
    CString::size_type indexGmt;
    CString::size_type indexPlus = CString::npos;
    std::array<CString, MOUTH_PER_YEAR> monthName = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
    int localTime = 0;
    int localHours = 0;
    int localMinutes = 0;
    int64_t localMs = 0;
    CString::size_type localSpace;
    localSpace = str.find(' ', index);
    CString strMonth = str.substr(localSpace + 1, LENGTH_MONTH_NAME);
    for (int i = 0; i < MOUTH_PER_YEAR; i++) {
        if (strMonth == monthName[i]) {
            month = i;
            break;
        }
    }
    index += (LENGTH_MONTH_NAME + 1);
    GetNumFromString(str, len, &index, &date);
    GetNumFromString(str, len, &index, &year);
    indexGmt = str.find("GMT", index);
    if (indexGmt == CString::npos) {
        GetNumFromString(str, len, &index, &hours);
        GetNumFromString(str, len, &index, &minutes);
        GetNumFromString(str, len, &index, &seconds);
        isLocal = true;
        localMs -= (GetLocalOffsetFromOS(localMs, true) * MS_PER_MINUTE);
    } else {
        indexPlus = str.find(PLUS, indexGmt);
        int indexLocal = static_cast<int>(indexGmt);
        GetNumFromString(str, indexGmt, &index, &hours);
        GetNumFromString(str, indexGmt, &index, &minutes);
        GetNumFromString(str, indexGmt, &index, &seconds);
        GetNumFromString(str, len, &indexLocal, &localTime);
        localHours = localTime / HUNDRED;
        localMinutes = localTime % HUNDRED;
        localMs = static_cast<int64_t>(MakeTime(localHours, localMinutes, 0, 0));
        if (indexPlus != CString::npos) {
            localMs = -localMs;
        }
    }
    double day = MakeDay(year, month, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    double timeValue = TimeClip(MakeDate(day, time));
    if (std::isnan(timeValue)) {
        return JSTaggedValue(timeValue);
    }
    if (isLocal && timeValue < CHINA_1901_MS && (-localMs / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
        timeValue += static_cast<double>(localMs - CHINA_BEFORE_1901_MS);
    } else {
        timeValue += localMs;
    }
    return JSTaggedValue(timeValue);
}

// static
JSTaggedValue JSDate::UtcParseStringToMs(const CString &str)
{
    int year = 0;
    int month = 0;
    int date = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    int index = 0;
    int len = str.length();
    CString::size_type indexGmt;
    CString::size_type indexPlus = CString::npos;
    int localTime = 0;
    int localHours = 0;
    int localMinutes = 0;
    int64_t localMs = 0;
    bool isLocal = false;
    std::array<CString, MOUTH_PER_YEAR> monthName = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    GetNumFromString(str, len, &index, &date);
    CString strMonth = str.substr(index + 1, LENGTH_MONTH_NAME);
    for (int i = 0; i < MOUTH_PER_YEAR; i++) {
        if (strMonth == monthName[i]) {
            month = i;
            break;
        }
    }
    index += (LENGTH_MONTH_NAME + 1);
    GetNumFromString(str, len, &index, &year);
    indexGmt = str.find("GMT", index);
    if (indexGmt == CString::npos) {
        GetNumFromString(str, len, &index, &hours);
        GetNumFromString(str, len, &index, &minutes);
        GetNumFromString(str, len, &index, &seconds);
        isLocal = true;
        localMs -= (GetLocalOffsetFromOS(localMs, true) * MS_PER_MINUTE);
    } else {
        indexPlus = str.find(PLUS, indexGmt);
        int indexLocal = static_cast<int>(indexGmt);
        GetNumFromString(str, indexGmt, &index, &hours);
        GetNumFromString(str, indexGmt, &index, &minutes);
        GetNumFromString(str, indexGmt, &index, &seconds);
        GetNumFromString(str, len, &indexLocal, &localTime);
        localHours = localTime / HUNDRED;
        localMinutes = localTime % HUNDRED;
        localMs = static_cast<int64_t>(MakeTime(localHours, localMinutes, 0, 0));
        if (indexPlus != CString::npos) {
            localMs = -localMs;
        }
    }
    double day = MakeDay(year, month, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    double timeValue = TimeClip(MakeDate(day, time));
    if (std::isnan(timeValue)) {
        return JSTaggedValue(timeValue);
    }
    if (isLocal && timeValue < CHINA_1901_MS && (-localMs / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
        timeValue += static_cast<double>(localMs - CHINA_BEFORE_1901_MS);
    } else {
        timeValue += localMs;
    }
    return JSTaggedValue(timeValue);
}
// static
JSTaggedValue JSDate::IsoParseStringToMs(const CString &str)
{
    char flag = 0;
    int year;
    int month = 1;
    int date = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    int index = 0;
    int len = str.length();
    year = GetSignedNumFromString(str, len, &index);
    CString::size_type indexT = str.find(FLAG_TIME, index);
    CString::size_type indexZ = str.find(FLAG_UTC, index);
    CString::size_type indexEndFlag = 0;
    int64_t localMs = 0;
    if (indexZ != CString::npos) {
        indexEndFlag = indexZ;
    } else if (len >= MIN_LENGTH && str.at(len - INDEX_PLUS_NEG) == NEG) {
        indexEndFlag = len - INDEX_PLUS_NEG;
        flag = NEG;
    } else if (len >= MIN_LENGTH && str.at(len - INDEX_PLUS_NEG) == PLUS) {
        indexEndFlag = len - INDEX_PLUS_NEG;
        flag = PLUS;
    }
    if (indexT != CString::npos) {
        if ((indexT - index) == LENGTH_PER_TIME) {
            GetNumFromString(str, len, &index, &month);
        } else if ((indexT - index) == (LENGTH_PER_TIME + LENGTH_PER_TIME)) {
            GetNumFromString(str, len, &index, &month);
            GetNumFromString(str, len, &index, &date);
        }
        GetNumFromString(str, len, &index, &hours);
        GetNumFromString(str, len, &index, &minutes);
        if (indexEndFlag > 0) {
            if (indexEndFlag - index == LENGTH_PER_TIME) {
                GetNumFromString(str, len, &index, &seconds);
            } else if (indexEndFlag - index == (LENGTH_PER_TIME + LENGTH_PER_TIME + 1)) {
                GetNumFromString(str, len, &index, &seconds);
                GetNumFromString(str, len, &index, &ms);
            }
        } else {
            if (len - index == LENGTH_PER_TIME) {
                GetNumFromString(str, len, &index, &seconds);
            } else if (len - index == (LENGTH_PER_TIME + LENGTH_PER_TIME + 1)) {
                GetNumFromString(str, len, &index, &seconds);
                GetNumFromString(str, len, &index, &ms);
            }
        }
    } else {
        GetNumFromString(str, len, &index, &month);
        GetNumFromString(str, len, &index, &date);
    }
    if (indexEndFlag > 0) {
        int localHours = 0;
        int localMinutes = 0;
        if (indexZ == CString::npos) {
            GetNumFromString(str, len, &index, &localHours);
            GetNumFromString(str, len, &index, &localMinutes);
            if (flag == PLUS) {
                localMs = static_cast<int64_t>(-MakeTime(localHours, localMinutes, 0, 0));
            } else {
                localMs = static_cast<int64_t>(MakeTime(localHours, localMinutes, 0, 0));
            }
        }
    }
    if (indexEndFlag == 0 && indexT != CString::npos) {
        localMs -= (GetLocalOffsetFromOS(localMs, true) * MS_PER_MINUTE);
    }

    double day = MakeDay(year, month - 1, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    double timeValue = TimeClip(MakeDate(day, time));
    if (std::isnan(timeValue)) {
        return JSTaggedValue(timeValue);
    }
    if (flag == 0 && timeValue < CHINA_1901_MS && (-localMs / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
        timeValue += static_cast<double>(localMs - CHINA_BEFORE_1901_MS);
    } else {
        timeValue += localMs;
    }
    return JSTaggedValue(timeValue);
}

// 20.4.3.2 static
JSTaggedValue JSDate::Parse(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    const CString isoPriStr = "(^|(\\+|-)(\\d{2}))";
    const CString isoDateStr =
        "(((\\d{4})-(0?[1-9]|1[0-2])-(0?[1-9]|1[0-9]|2[0-9]|3[0-1]))"
        "|((\\d{4})-(0?[1-9]|1[0-2]))|(\\d{4}))";
    const CString isoTimeStr =
        "((T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])"
        "\\.(\\d{3}))|(T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9]))|"
        "(T([01][0-9]|2[0-3]):([0-5][0-9]))|(T([01][0-9]|2[0-3])))"
        "($|Z|((\\+|-)(([01][0-9]|2[0-3]):([0-5][0-9]))))";
    const CString isoRegStr = isoPriStr + isoDateStr + "($|Z|(" + isoTimeStr + "))";
    const CString utcDateStr =
        "^\\D*(0?[1-9]|1[0-9]|2[0-9]|3[0-1]) "
        "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) (\\d{4})";
    const CString timeStr =
        "(( ([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9]))|( ([01][0-9]|2[0-3]):([0-5][0-9])))"
        "($| *| GMT *| GMT((\\+|-)(\\d{4})) *)";
    const CString utcRegStr = utcDateStr + "($| *| GMT *| GMT((\\+|-)(\\d{4})) *|(" + timeStr + "))";
    const CString localDateStr =
        "^[a-zA-Z]* (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) "
        "(0?[1-9]|1[0-9]|2[0-9]|3[0-1]) (\\d{4})";
    const CString localRegStr = localDateStr + "($| *| GMT *| GMT((\\+|-)(\\d{4})) *|(" + timeStr + "))";

    std::regex isoReg(isoRegStr);
    std::regex utcReg(utcRegStr);
    std::regex localReg(localRegStr);
    JSHandle<JSTaggedValue> msg = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> str = JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread, msg));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    CString date = ConvertToString(EcmaString::Cast(str->GetTaggedObject()));
    if (std::regex_match(date, isoReg)) {
        return IsoParseStringToMs(date);
    }
    if (std::regex_match(date, utcReg)) {
        return UtcParseStringToMs(date);
    }
    if (std::regex_match(date, localReg)) {
        return LocalParseStringToMs(date);
    }
    return JSTaggedValue(base::NAN_VALUE);
}

// 20.4.3.1
JSTaggedValue JSDate::Now()
{
    // time from now is in ms.
    int64_t ans;
    struct timeval tv {
    };
    gettimeofday(&tv, nullptr);
    ans = static_cast<int64_t>(tv.tv_sec) * MS_PER_SECOND + (tv.tv_usec / MS_PER_SECOND);
    return JSTaggedValue(static_cast<double>(ans));
}

// 20.4.4.2 static
JSTaggedValue JSDate::UTC(EcmaRuntimeCallInfo *argv)
{
    double year;
    double month = 0;
    double date = 1;
    double hours = 0;
    double minutes = 0;
    double seconds = 0;
    double ms = 0;
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> yearArg = base::BuiltinsBase::GetCallArg(argv, 0);
    JSTaggedNumber yearValue = JSTaggedValue::ToNumber(thread, yearArg);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    if (yearValue.IsNumber()) {
        year = yearValue.GetNumber();
        if (std::isfinite(year) && !yearValue.IsInt()) {
            year = NumberHelper::TruncateDouble(year);
        }
        if (year >= 0 && year <= (HUNDRED - 1)) {
            year = year + NINETEEN_HUNDRED_YEAR;
        }
    } else {
        year = base::NAN_VALUE;
    }
    uint32_t index = 1;
    uint32_t numArgs = argv->GetArgsNumber();
    JSTaggedValue res;
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        month = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        date = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        hours = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        minutes = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        seconds = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        ms = res.GetNumber();
    }
    double day = MakeDay(year, month, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    return JSTaggedValue(TimeClip(MakeDate(day, time)));
}

int64_t JSDate::GetLocalOffsetFromOS(int64_t timeMs, bool isLocal)
{
    // Preserve the old behavior for non-ICU implementation by ignoring both timeMs and is_utc.
    if (!isLocal) {
        return 0;
    }
    timeMs /= JSDate::THOUSAND;
    time_t tv = std::time(reinterpret_cast<time_t *>(&timeMs));
    struct tm tm {
    };
    // localtime_r is only suitable for linux.
    struct tm *t = localtime_r(&tv, &tm);
    // tm_gmtoff includes any daylight savings offset.
    return t->tm_gmtoff / SEC_PER_MINUTE;
}

// 20.4.4.10
JSTaggedValue JSDate::GetTime() const
{
    return GetTimeValue();
}

// static
CString JSDate::StrToTargetLength(const CString &str, int length)
{
    int len;
    if (str[0] == NEG) {
        len = static_cast<int>(str.length() - 1);
    } else {
        len = static_cast<int>(str.length());
    }
    int dif = length - len;
    CString sub;
    for (int i = 0; i < dif; i++) {
        sub += '0';
    }
    if (str[0] == NEG) {
        sub = NEG + sub + str.substr(1, len);
    } else {
        sub = sub + str;
    }
    return sub;
}

bool JSDate::GetThisDateValues(std::array<int64_t, DATE_LENGTH> *date, bool isLocal) const
{
    double timeMs = this->GetTimeValue().GetDouble();
    if (std::isnan(timeMs)) {
        return false;
    }
    GetDateValues(timeMs, date, isLocal);
    return true;
}

// 20.4.4.35
JSTaggedValue JSDate::ToDateString(JSThread *thread) const
{
    std::array<CString, MOUTH_PER_YEAR> monthName = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    std::array<CString, DAY_PER_WEEK> weekdayName = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString year = StrToTargetLength(ToCString(fields[YEAR]), STR_LENGTH_YEAR);
    CString day = StrToTargetLength(ToCString(fields[DAYS]), STR_LENGTH_OTHERS);
    CString str = weekdayName[fields[WEEKDAY]] + SPACE + monthName[fields[MONTH]] + SPACE + day + SPACE + year;
    JSHandle<EcmaString> result = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str);
    return result.GetTaggedValue();
}

// static
CString JSDate::ToDateString(double timeMs)
{
    if (std::isnan(timeMs)) {
        return "Invalid Date";
    }
    std::array<CString, MOUTH_PER_YEAR> monthName = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    std::array<CString, DAY_PER_WEEK> weekdayName = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    std::array<int64_t, DATE_LENGTH> fields = {0};
    GetDateValues(timeMs, &fields, true);
    CString localTime;
    int localMin = 0;
    localMin = GetLocalOffsetFromOS(localMin, true);
    if (timeMs < CHINA_BEFORE_1900_MS && localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    CString year = ToCString(fields[YEAR]);
    year = StrToTargetLength(year, STR_LENGTH_YEAR);
    CString weekday = weekdayName[fields[WEEKDAY]];
    CString month = monthName[fields[MONTH]];
    CString day = StrToTargetLength(ToCString(fields[DAYS]), STR_LENGTH_OTHERS);
    CString hour = StrToTargetLength(ToCString(fields[HOUR]), STR_LENGTH_OTHERS);
    CString minute = StrToTargetLength(ToCString(fields[MIN]), STR_LENGTH_OTHERS);
    CString second = StrToTargetLength(ToCString(fields[SEC]), STR_LENGTH_OTHERS);
    CString str = weekday + SPACE + month + SPACE + day + SPACE + year + SPACE + hour + COLON + minute + COLON +
                  second + SPACE + "GMT" + localTime;
    return str;
}
// 20.4.4.36
JSTaggedValue JSDate::ToISOString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, false)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString year = ToCString(fields[YEAR]);
    if (year[0] == NEG) {
        year = StrToTargetLength(year, STR_LENGTH_YEAR + STR_LENGTH_OTHERS);
    } else if (year.length() > STR_LENGTH_YEAR) {
        year = PLUS + StrToTargetLength(year, STR_LENGTH_YEAR + STR_LENGTH_OTHERS);
    } else {
        year = StrToTargetLength(year, STR_LENGTH_YEAR);
    }
    CString month = StrToTargetLength(ToCString(fields[MONTH] + 1), STR_LENGTH_OTHERS);
    CString day = StrToTargetLength(ToCString(fields[DAYS]), STR_LENGTH_OTHERS);
    CString hour = StrToTargetLength(ToCString(fields[HOUR]), STR_LENGTH_OTHERS);
    CString minute = StrToTargetLength(ToCString(fields[MIN]), STR_LENGTH_OTHERS);
    CString second = StrToTargetLength(ToCString(fields[SEC]), STR_LENGTH_OTHERS);
    CString ms = StrToTargetLength(ToCString(fields[MS]), STR_LENGTH_OTHERS + 1);
    CString str =
        year + NEG + month + NEG + day + FLAG_TIME + hour + COLON + minute + COLON + second + POINT + ms + FLAG_UTC;
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str).GetTaggedValue();
}

CString JSDate::GetLocaleTimeStr(const std::array<int64_t, DATE_LENGTH> &fields) const
{
    CString hour;
    if (fields[HOUR] > MOUTH_PER_YEAR) {
        hour = ToCString(fields[HOUR] - MOUTH_PER_YEAR);
    } else {
        hour = ToCString(fields[HOUR]);
    }
    CString minute = ToCString(fields[MIN]);
    CString second = StrToTargetLength(ToCString(fields[SEC]), STR_LENGTH_OTHERS);
    CString str = hour + COLON + minute + COLON + second;
    if (fields[HOUR] >= MOUTH_PER_YEAR) {
        str = "下午" + str;
    } else {
        str = "上午" + str;
    }
    return str;
}

CString JSDate::GetLocaleDateStr(const std::array<int64_t, DATE_LENGTH> &fields) const
{
    CString year;
    if (fields[YEAR] < 0) {
        year = ToCString(-fields[YEAR] + 1);
    } else {
        year = ToCString(fields[YEAR]);
    }
    CString month = ToCString(fields[MONTH] + 1);
    CString day = ToCString(fields[DAYS]);
    CString str = year + VIRGULE + month + VIRGULE + day;
    return str;
}

// 20.4.4.38
JSTaggedValue JSDate::ToLocaleDateString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString str = GetLocaleDateStr(fields);
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str).GetTaggedValue();
}

// 20.4.4.39
JSTaggedValue JSDate::ToLocaleString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString strDate = GetLocaleDateStr(fields);
    CString strTime = GetLocaleTimeStr(fields);
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(strDate + SPACE + strTime).GetTaggedValue();
}

// 20.4.4.40
JSTaggedValue JSDate::ToLocaleTimeString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString str = GetLocaleTimeStr(fields);
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str).GetTaggedValue();
}

// 20.4.4.41
JSTaggedValue JSDate::ToString(JSThread *thread) const
{
    std::array<CString, DAY_PER_WEEK> weekdayName = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    std::array<CString, MOUTH_PER_YEAR> monthName = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    int localMin = 0;
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString localTime;
    localMin = GetLocalOffsetFromOS(localMin, true);
    if (static_cast<int64_t>(this->GetTimeValue().GetDouble()) < CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    CString year = ToCString(fields[YEAR]);
    year = StrToTargetLength(year, STR_LENGTH_YEAR);
    CString weekday = weekdayName[fields[WEEKDAY]];
    CString month = monthName[fields[MONTH]];
    CString day = StrToTargetLength(ToCString(fields[DAYS]), STR_LENGTH_OTHERS);
    CString hour = StrToTargetLength(ToCString(fields[HOUR]), STR_LENGTH_OTHERS);
    CString minute = StrToTargetLength(ToCString(fields[MIN]), STR_LENGTH_OTHERS);
    CString second = StrToTargetLength(ToCString(fields[SEC]), STR_LENGTH_OTHERS);
    CString str = weekday + SPACE + month + SPACE + day + SPACE + year + SPACE + hour + COLON + minute + COLON +
                  second + SPACE + "GMT" + localTime;
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str).GetTaggedValue();
}

// 20.4.4.42
JSTaggedValue JSDate::ToTimeString(JSThread *thread) const
{
    int localMin = 0;
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString localTime;
    localMin = GetLocalOffsetFromOS(localMin, true);
    if (static_cast<int64_t>(this->GetTimeValue().GetDouble()) < CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    CString hour = StrToTargetLength(ToCString(fields[HOUR]), STR_LENGTH_OTHERS);
    CString minute = StrToTargetLength(ToCString(fields[MIN]), STR_LENGTH_OTHERS);
    CString second = StrToTargetLength(ToCString(fields[SEC]), STR_LENGTH_OTHERS);
    CString str = hour + COLON + minute + COLON + second + SPACE + "GMT" + localTime;
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str).GetTaggedValue();
}

// 20.4.4.43
JSTaggedValue JSDate::ToUTCString(JSThread *thread) const
{
    std::array<CString, DAY_PER_WEEK> weekdayName = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    std::array<CString, MOUTH_PER_YEAR> monthName = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(&fields, false)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString year = ToCString(fields[YEAR]);
    year = StrToTargetLength(year, STR_LENGTH_YEAR);
    CString weekday = weekdayName[fields[WEEKDAY]];
    CString month = monthName[fields[MONTH]];
    CString day = StrToTargetLength(ToCString(fields[DAYS]), STR_LENGTH_OTHERS);
    CString hour = StrToTargetLength(ToCString(fields[HOUR]), STR_LENGTH_OTHERS);
    CString minute = StrToTargetLength(ToCString(fields[MIN]), STR_LENGTH_OTHERS);
    CString second = StrToTargetLength(ToCString(fields[SEC]), STR_LENGTH_OTHERS);
    CString ms = StrToTargetLength(ToCString(fields[MS]), STR_LENGTH_OTHERS);
    CString str = weekday + COMMA + SPACE + day + SPACE + month + SPACE + year + SPACE + hour + COLON + minute + COLON +
                  second + SPACE + "GMT";
    return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(str).GetTaggedValue();
}

// 20.4.4.44
JSTaggedValue JSDate::ValueOf() const
{
    return this->GetTimeValue();
}

// static
void JSDate::GetDateValues(double timeMs, std::array<int64_t, DATE_LENGTH> *date, bool isLocal)
{
    int64_t tz = 0;
    int64_t timeMsInt;
    int month = 0;
    timeMsInt = static_cast<int64_t>(timeMs);
    if (isLocal) {  // timezone offset
        tz = GetLocalOffsetFromOS(timeMsInt, isLocal);
        if (timeMsInt < CHINA_BEFORE_1900_MS && tz == CHINA_AFTER_1901_MIN) {
            timeMsInt += CHINA_BEFORE_1901_ADDMS;
            timeMsInt += tz * MS_PER_SECOND * SEC_PER_MINUTE;
            tz = CHINA_BEFORE_1901_MIN;
        } else {
            timeMsInt += tz * MS_PER_SECOND * SEC_PER_MINUTE;
        }
    }

    DateUtils::TransferTimeToDate(timeMsInt, date);

    int index = DateUtils::IsLeap((*date)[YEAR]) ? 1 : 0;
    int left = 0;
    int right = MONTH_PER_YEAR;
    while (left <= right) {
        int middle = (left + right) / 2; // 2 : half
        if (DAYS_FROM_MONTH[index][middle] <= (*date)[DAYS] && DAYS_FROM_MONTH[index][middle + 1] > (*date)[DAYS]) {
            month = middle;
            (*date)[DAYS] -= DAYS_FROM_MONTH[index][month];
            break;
        } else if ((*date)[DAYS] > DAYS_FROM_MONTH[index][middle]) { // NOLINT(readability-else-after-return)
            left = middle + 1;
        } else if ((*date)[DAYS] < DAYS_FROM_MONTH[index][middle]) {
            right = middle - 1;
        }
    }

    (*date)[MONTH] = month;
    (*date)[DAYS] = (*date)[DAYS] + 1;
    (*date)[TIMEZONE] = -tz;
}

double JSDate::GetDateValue(double timeMs, uint8_t code, bool isLocal) const
{
    if (std::isnan(timeMs)) {
        return base::NAN_VALUE;
    }
    std::array<int64_t, DATE_LENGTH> date = {0};
    GetDateValues(timeMs, &date, isLocal);
    return static_cast<double>(date[code]);
}

JSTaggedValue JSDate::SetDateValue(EcmaRuntimeCallInfo *argv, uint32_t code, bool isLocal) const
{
    // get date values.
    std::array<int64_t, DATE_LENGTH> date = {0};
    double timeMs = this->GetTimeValue().GetDouble();

    // get values from argv.
    uint32_t argc = argv->GetArgsNumber();
    if (argc == 0) {
        return JSTaggedValue(base::NAN_VALUE);
    }

    uint32_t firstValue = code & CODE_FLAG;
    uint32_t endValue = (code >> CODE_4_BIT) & CODE_FLAG;
    uint32_t count = endValue - firstValue;

    if (argc < count) {
        count = argc;
    }

    if (std::isnan(timeMs) && firstValue == 0) {
        timeMs = 0.0;
        GetDateValues(timeMs, &date, false);
    } else {
        GetDateValues(timeMs, &date, isLocal);
    }

    for (uint32_t i = 0; i < count; i++) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, i);
        JSThread *thread = argv->GetThread();
        JSTaggedNumber res = JSTaggedValue::ToNumber(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double temp = res.GetNumber();
        if (std::isnan(temp)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        date[firstValue + i] = NumberHelper::TruncateDouble(temp);
    }
    // set date values.
    return JSTaggedValue(SetDateValues(&date, isLocal));
}

// static
double JSDate::SetDateValues(const std::array<int64_t, DATE_LENGTH> *date, bool isLocal)
{
    int64_t month = DateUtils::Mod((*date)[MONTH], MONTH_PER_YEAR);
    int64_t year = (*date)[YEAR] + ((*date)[MONTH] - month) / MONTH_PER_YEAR;
    int64_t days = DateUtils::GetDaysFromYear(year);
    int index = DateUtils::IsLeap(year) ? 1 : 0;
    days += DAYS_FROM_MONTH[index][month];

    days += (*date)[DAYS] - 1;
    int64_t millisecond =
        (((*date)[HOUR] * MIN_PER_HOUR + (*date)[MIN]) * SEC_PER_MINUTE + (*date)[SEC]) * MS_PER_SECOND + (*date)[MS];
    int64_t result = days * MS_PER_DAY + millisecond;
    if (isLocal) {
        int64_t offset = GetLocalOffsetFromOS(result, isLocal) * SEC_PER_MINUTE * MS_PER_SECOND;
        if (result < CHINA_1901_MS && (offset / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
            offset += CHINA_BEFORE_1901_ADDMS;
        }
        result -= offset;
    }
    return TimeClip(result);
}
}  // namespace panda::ecmascript
