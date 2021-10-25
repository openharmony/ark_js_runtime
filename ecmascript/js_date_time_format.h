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

#ifndef ECMASCRIPT_JS_DATE_TIME_FORMAT_H
#define ECMASCRIPT_JS_DATE_TIME_FORMAT_H

#include "js_locale.h"

namespace panda::ecmascript {
enum class CalendarOption : uint8_t { UNDEFINED = 0x01 };
enum class DateTimeStyleOption : uint8_t { FULL = 0x01, LONG, MEDIUM, SHORT, UNDEFINED, EXCEPTION };
enum class DefaultsOption : uint8_t { DATE = 0x01, TIME, ALL };
enum class HourCycleOption : uint8_t { H11 = 0x01, H12, H23, H24, UNDEFINED, EXCEPTION };
enum class RequiredOption : uint8_t { DATE = 0x01, TIME, ANY };
enum class Value : uint8_t { SHARED, START_RANGE, END_RANGE };

constexpr int CAPACITY_3 = 3;
constexpr int CAPACITY_4 = 4;
constexpr int CAPACITY_5 = 5;
constexpr int CAPACITY_8 = 8;
constexpr int STRING_LENGTH_2 = 2;
constexpr int STRING_LENGTH_3 = 3;
constexpr int STRING_LENGTH_7 = 7;
constexpr int STRING_LENGTH_8 = 8;
constexpr int STRING_LENGTH_9 = 9;
constexpr int STRING_LENGTH_10 = 10;

class IcuPatternDesc;

std::vector<IcuPatternDesc> BuildIcuPatternDescs();
std::vector<IcuPatternDesc> InitializePattern(const IcuPatternDesc &hourData);

using IcuPatternDescVect = std::vector<IcuPatternDesc>;
using IcuPatternEntry = std::pair<std::string, std::string>;

class IcuPatternDesc {
public:
    IcuPatternDesc(std::string property, const std::vector<IcuPatternEntry> &pairs,
                   std::vector<std::string> allowedValues) : property(std::move(property)), pairs(std::move(pairs)),
                                                             allowedValues(std::move(allowedValues))
    {
        for (const auto &pair : pairs) {
            map.insert(std::make_pair(pair.second, pair.first));
        }
    }

    virtual ~IcuPatternDesc() = default;

    std::string property;   // NOLINT(misc-non-private-member-variables-in-classes)
    std::vector<IcuPatternEntry> pairs; // NOLINT(misc-non-private-member-variables-in-classes)
    std::map<const std::string, const std::string> map; // NOLINT(misc-non-private-member-variables-in-classes)
    std::vector<std::string> allowedValues; // NOLINT(misc-non-private-member-variables-in-classes)

    DEFAULT_COPY_SEMANTIC(IcuPatternDesc);
    // NOLINT(performance-noexcept-move-constructor, hicpp-noexcept-move)
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(IcuPatternDesc);
};

class Pattern {
public:
    Pattern(const std::string &data1, const std::string &data2) : data(InitializePattern(
        IcuPatternDesc("hour", {{data1, "2-digit"}, {data2, "numeric"}}, {"2-digit", "numeric"}))) {}
    virtual ~Pattern() = default;
    std::vector<IcuPatternDesc> Get() const
    {
        return data;
    }

private:
    std::vector<IcuPatternDesc> data{};
    NO_COPY_SEMANTIC(Pattern);
    NO_MOVE_SEMANTIC(Pattern);
};

class JSDateTimeFormat : public JSObject {
public:
    static JSDateTimeFormat *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSDateTimeFormat());
        return reinterpret_cast<JSDateTimeFormat *>(object);
    }

    static constexpr size_t LOCALE_OFFSET = JSObject::SIZE;

    ACCESSORS(Locale, LOCALE_OFFSET, CALENDAR_OFFSET)
    ACCESSORS(Calendar, CALENDAR_OFFSET, NUMBER_STRING_SYSTEM_OFFSET)
    ACCESSORS(NumberingSystem, NUMBER_STRING_SYSTEM_OFFSET, TIME_ZONE_OFFSET)
    ACCESSORS(TimeZone, TIME_ZONE_OFFSET, HOUR_CYCLE_OFFSET)
    ACCESSORS(HourCycle, HOUR_CYCLE_OFFSET, LOCALE_ICU_OFFSET)
    ACCESSORS(LocaleIcu, LOCALE_ICU_OFFSET, SIMPLE_DATE_TIME_FORMAT_ICU_OFFSET)
    ACCESSORS(SimpleDateTimeFormatIcu, SIMPLE_DATE_TIME_FORMAT_ICU_OFFSET, ISO8601_OFFSET)
    ACCESSORS(Iso8601, ISO8601_OFFSET, DATE_STYLE_OFFSET)
    ACCESSORS(DateStyle, DATE_STYLE_OFFSET, TIME_STYLE_OFFSET)
    ACCESSORS(TimeStyle, TIME_STYLE_OFFSET, BOUND_FORMAT_OFFSET)
    ACCESSORS(BoundFormat, BOUND_FORMAT_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LOCALE_OFFSET, SIZE)
    DECL_DUMP()

    icu::Locale *GetIcuLocale() const;
    void SetIcuLocale(JSThread *thread, const icu::Locale &icuLocale, const DeleteEntryPoint &callback);
    static void FreeIcuLocale(void *pointer, void *data);

    icu::SimpleDateFormat *GetIcuSimpleDateFormat() const;
    void SetIcuSimpleDateFormat(JSThread *thread, const icu::SimpleDateFormat &icuSimpleDateTimeFormat,
                                const DeleteEntryPoint &callback);
    static void FreeSimpleDateFormat(void *pointer, void *data);

    // 13.1.1 InitializeDateTimeFormat (dateTimeFormat, locales, options)
    static JSHandle<JSDateTimeFormat> InitializeDateTimeFormat(JSThread *thread,
                                                               const JSHandle<JSDateTimeFormat> &dateTimeFormat,
                                                               const JSHandle<JSTaggedValue> &locales,
                                                               const JSHandle<JSTaggedValue> &options);

    // 13.1.2 ToDateTimeOptions (options, required, defaults)
    static JSHandle<JSObject> ToDateTimeOptions(JSThread *thread, const JSHandle<JSTaggedValue> &options,
                                                const RequiredOption &required, const DefaultsOption &defaults);

    // 13.1.7 FormatDateTime(dateTimeFormat, x)
    static JSHandle<EcmaString> FormatDateTime(JSThread *thread, const JSHandle<JSDateTimeFormat> &dateTimeFormat,
                                               double x);

    // 13.1.8 FormatDateTimeToParts (dateTimeFormat, x)
    static JSHandle<JSArray> FormatDateTimeToParts(JSThread *thread, const JSHandle<JSDateTimeFormat> &dateTimeFormat,
                                                   double x);

    // 13.1.10 UnwrapDateTimeFormat(dtf)
    static JSHandle<JSTaggedValue> UnwrapDateTimeFormat(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &dateTimeFormat);

    static JSHandle<TaggedArray> GainAvailableLocales(JSThread *thread);

    static void ResolvedOptions(JSThread *thread, const JSHandle<JSDateTimeFormat> &dateTimeFormat,
                                const JSHandle<JSObject> &options);

    static JSHandle<EcmaString> NormDateTimeRange(JSThread *thread, const JSHandle<JSDateTimeFormat> &dtf, double x,
                                                    double y);

    static JSHandle<JSArray> NormDateTimeRangeToParts(JSThread *thread, const JSHandle<JSDateTimeFormat> &dtf,
                                                        double x, double y);

private:
    static HourCycleOption OptionToHourCycle(const std::string &hc);

    static Value TrackValue(int32_t beginning, int32_t ending, std::array<int32_t, 2> begin,
                            std::array<int32_t, 2> end);

    static HourCycleOption OptionToHourCycle(UDateFormatHourCycle hc);

    static std::string ToHourCycleString(int32_t hc);

    static std::unique_ptr<icu::TimeZone> ConstructTimeZone(const std::string &timezone);

    static std::string ConstructFormattedTimeZoneID(const std::string &input);

    static std::string ToTitleCaseTimezonePosition(const std::string &input);

    static std::unique_ptr<icu::DateIntervalFormat> ConstructDateIntervalFormat(const JSHandle<JSDateTimeFormat> &dtf);

    static std::string ConstructGMTTimeZoneID(const std::string &input);

    static std::unique_ptr<icu::Calendar> BuildCalendar(const icu::Locale &locale, const icu::TimeZone &timeZone);

    static std::map<std::string, std::string> GetSpecialTimeZoneMap();

    static JSHandle<JSArray> ConstructFDateIntervalToJSArray(JSThread *thread,
                                                             const icu::FormattedDateInterval &formatted);

    static std::vector<IcuPatternDesc> GetIcuPatternDesc(const HourCycleOption &hourCycle);

    static std::unique_ptr<icu::SimpleDateFormat> CreateICUSimpleDateFormat(const icu::Locale &icuLocale,
                                                               const icu::UnicodeString &skeleton,
                                                               icu::DateTimePatternGenerator *generator,
                                                               HourCycleOption hc);

    static JSHandle<JSTaggedValue> ConvertFieldIdToDateType(JSThread *thread, int32_t fieldId);

    static icu::UnicodeString ChangeHourCyclePattern(const icu::UnicodeString &pattern, HourCycleOption hc);

    static std::string ToTitleCaseFunction(const std::string &input);

    static bool IsValidTimeZoneInput(const std::string &input);

    static JSHandle<EcmaString> ToValueString(JSThread *thread, Value value);

    static icu::FormattedDateInterval ConstructDTFRange(JSThread *thread, const JSHandle<JSDateTimeFormat> &dtf,
                                                        double x, double y);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_DATE_TIME_FORMAT_H
