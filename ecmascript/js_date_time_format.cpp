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

#include "js_date_time_format.h"

#include "ecma_macros.h"
#include "global_env.h"
#include "js_array.h"
#include "js_date.h"
#include "js_intl.h"
#include "js_locale.h"
#include "js_object-inl.h"
#include "object_factory.h"

namespace panda::ecmascript {
struct CommonDateFormatPart {
    int32_t fField = 0;
    int32_t fBeginIndex = 0;   // NOLINT(misc-non-private-member-variables-in-classes)
    int32_t fEndIndex = 0;  // NOLINT(misc-non-private-member-variables-in-classes)
    int32_t index = 0;    // NOLINT(misc-non-private-member-variables-in-classes)
    bool isPreExist = false;

    CommonDateFormatPart() = default;
    CommonDateFormatPart(int32_t fField, int32_t fBeginIndex, int32_t fEndIndex, int32_t index, bool isPreExist)
        : fField(fField), fBeginIndex(fBeginIndex), fEndIndex(fEndIndex), index(index), isPreExist(isPreExist)
    {
    }

    ~CommonDateFormatPart() = default;

    DEFAULT_COPY_SEMANTIC(CommonDateFormatPart);
    DEFAULT_MOVE_SEMANTIC(CommonDateFormatPart);
};

namespace {
const std::vector<std::string> ICU_LONG_SHORT = {"long", "short"};
const std::vector<std::string> ICU_NARROW_LONG_SHORT = {"narrow", "long", "short"};
const std::vector<std::string> ICU2_DIGIT_NUMERIC = {"2-digit", "numeric"};
const std::vector<std::string> ICU_NARROW_LONG_SHORT2_DIGIT_NUMERIC = {"narrow", "long", "short", "2-digit", "numeric"};
const std::vector<IcuPatternEntry> ICU_WEEKDAY_PE = {
    {"EEEEE", "narrow"}, {"EEEE", "long"}, {"EEE", "short"},
    {"ccccc", "narrow"}, {"cccc", "long"}, {"ccc", "short"}
};
const std::vector<IcuPatternEntry> ICU_ERA_PE = {{"GGGGG", "narrow"}, {"GGGG", "long"}, {"GGG", "short"}};
const std::vector<IcuPatternEntry> ICU_YEAR_PE = {{"yy", "2-digit"}, {"y", "numeric"}};
const std::vector<IcuPatternEntry> ICU_MONTH_PE = {
    {"MMMMM", "narrow"}, {"MMMM", "long"}, {"MMM", "short"}, {"MM", "2-digit"}, {"M", "numeric"},
    {"LLLLL", "narrow"}, {"LLLL", "long"}, {"LLL", "short"}, {"LL", "2-digit"}, {"L", "numeric"}
};
const std::vector<IcuPatternEntry> ICU_DAY_PE = {{"dd", "2-digit"}, {"d", "numeric"}};
const std::vector<IcuPatternEntry> ICU_DAY_PERIOD_PE = {
    {"BBBBB", "narrow"}, {"bbbbb", "narrow"}, {"BBBB", "long"},
    {"bbbb", "long"}, {"B", "short"}, {"b", "short"}
};
const std::vector<IcuPatternEntry> ICU_HOUR_PE = {
    {"HH", "2-digit"}, {"H", "numeric"}, {"hh", "2-digit"}, {"h", "numeric"},
    {"kk", "2-digit"}, {"k", "numeric"}, {"KK", "2-digit"}, {"K", "numeric"}
};
const std::vector<IcuPatternEntry> ICU_MINUTE_PE = {{"mm", "2-digit"}, {"m", "numeric"}};
const std::vector<IcuPatternEntry> ICU_SECOND_PE = {{"ss", "2-digit"}, {"s", "numeric"}};
const std::vector<IcuPatternEntry> ICU_YIME_ZONE_NAME_PE = {{"zzzz", "long"}, {"z", "short"}};

const std::map<char16_t, HourCycleOption> HOUR_CYCLE_MAP = {
    {'K', HourCycleOption::H11},
    {'h', HourCycleOption::H12},
    {'H', HourCycleOption::H23},
    {'k', HourCycleOption::H24}
};
const std::map<std::string, HourCycleOption> TO_HOUR_CYCLE_MAP = {
    {"h11", HourCycleOption::H11},
    {"h12", HourCycleOption::H12},
    {"h23", HourCycleOption::H23},
    {"h24", HourCycleOption::H24}
};

// The value of the [[RelevantExtensionKeys]] internal slot is « "ca", "nu", "hc" ».
const std::set<std::string> RELEVANT_EXTENSION_KEYS = {"nu", "ca", "hc"};
}

icu::Locale *JSDateTimeFormat::GetIcuLocale() const
{
    ASSERT(GetLocaleIcu().IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetLocaleIcu().GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::Locale *>(result);
}

void JSDateTimeFormat::SetIcuLocale(JSThread *thread, const icu::Locale &icuLocale, const DeleteEntryPoint &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    icu::Locale *icuPointer = ecmaVm->GetRegionFactory()->New<icu::Locale>(icuLocale);
    ASSERT(icuPointer != nullptr);
    JSTaggedValue data = GetLocaleIcu();
    if (data.IsHeapObject() && data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(icuPointer);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuPointer);
    pointer->SetDeleter(callback);
    pointer->SetData(ecmaVm);
    SetLocaleIcu(thread, pointer.GetTaggedValue());
    ecmaVm->PushToArrayDataList(*pointer);
}

void JSDateTimeFormat::FreeIcuLocale(void *pointer, void *data)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuLocale = reinterpret_cast<icu::Locale *>(pointer);
    icuLocale->~Locale();
    if (data != nullptr) {
        reinterpret_cast<EcmaVM *>(data)->GetRegionFactory()->FreeBuffer(pointer);
    }
}

icu::SimpleDateFormat *JSDateTimeFormat::GetIcuSimpleDateFormat() const
{
    ASSERT(GetSimpleDateTimeFormatIcu().IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetSimpleDateTimeFormatIcu().GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::SimpleDateFormat *>(result);
}

void JSDateTimeFormat::SetIcuSimpleDateFormat(JSThread *thread, const icu::SimpleDateFormat &icuSimpleDateTimeFormat,
                                              const DeleteEntryPoint &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    icu::SimpleDateFormat *icuPointer = ecmaVm->GetRegionFactory()->New<icu::SimpleDateFormat>(icuSimpleDateTimeFormat);
    ASSERT(icuPointer != nullptr);
    JSTaggedValue data = GetSimpleDateTimeFormatIcu();
    if (data.IsHeapObject() && data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(icuPointer);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuPointer);
    pointer->SetDeleter(callback);
    pointer->SetData(ecmaVm);
    SetSimpleDateTimeFormatIcu(thread, pointer.GetTaggedValue());
    ecmaVm->PushToArrayDataList(*pointer);
}

void JSDateTimeFormat::FreeSimpleDateFormat(void *pointer, void *data)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuSimpleDateFormat = reinterpret_cast<icu::SimpleDateFormat *>(pointer);
    icuSimpleDateFormat->~SimpleDateFormat();
    if (data != nullptr) {
        reinterpret_cast<EcmaVM *>(data)->GetRegionFactory()->FreeBuffer(pointer);
    }
}

JSHandle<EcmaString> JSDateTimeFormat::ToValueString(JSThread *thread, const Value value)
{
    auto globalConst = thread->GlobalConstants();
    JSMutableHandle<EcmaString> result(thread, JSTaggedValue::Undefined());
    switch (value) {
        case Value::SHARED:
            result.Update(globalConst->GetHandledSharedString().GetTaggedValue());
            break;
        case Value::START_RANGE:
            result.Update(globalConst->GetHandledStartRangeString().GetTaggedValue());
            break;
        case Value::END_RANGE:
            result.Update(globalConst->GetHandledEndRangeString().GetTaggedValue());
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

// 13.1.1 InitializeDateTimeFormat (dateTimeFormat, locales, options)
// NOLINTNEXTLINE(readability-function-size)
JSHandle<JSDateTimeFormat> JSDateTimeFormat::InitializeDateTimeFormat(JSThread *thread,
                                                                      const JSHandle<JSDateTimeFormat> &dateTimeFormat,
                                                                      const JSHandle<JSTaggedValue> &locales,
                                                                      const JSHandle<JSTaggedValue> &options)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // 1. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);

    // 2. Let options be ? ToDateTimeOptions(options, "any", "date").
    JSHandle<JSObject> dateTimeOptions = ToDateTimeOptions(thread, options, RequiredOption::ANY, DefaultsOption::DATE);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);

    // 4. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, dateTimeOptions, globalConst->GetHandledLocaleMatcherString(),
        {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT}, {"lookup", "best fit"},
        LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);

    // 6. Let calendar be ? GetOption(options, "calendar", "string", undefined, undefined).
    JSHandle<JSTaggedValue> calendar =
        JSLocale::GetOption(thread, dateTimeOptions, globalConst->GetHandledCalendarString(), OptionType::STRING,
                            globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
    dateTimeFormat->SetCalendar(thread, calendar);

    // 7. If calendar is not undefined, then
    //    a. If calendar does not match the Unicode Locale Identifier type nonterminal, throw a RangeError exception.
    std::string calendarStr;
    if (!calendar->IsUndefined()) {
        JSHandle<EcmaString> calendarEcmaStr = JSHandle<EcmaString>::Cast(calendar);
        calendarStr = JSLocale::ConvertToStdString(calendarEcmaStr);
        if (!JSLocale::IsNormativeCalendar(calendarStr)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid calendar", dateTimeFormat);
        }
    }

    // 9. Let numberingSystem be ? GetOption(options, "numberingSystem", "string", undefined, undefined).
    JSHandle<JSTaggedValue> numberingSystem =
        JSLocale::GetOption(thread, dateTimeOptions, globalConst->GetHandledNumberingSystemString(), OptionType::STRING,
                            globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
    dateTimeFormat->SetNumberingSystem(thread, numberingSystem);

    // 10. If numberingSystem is not undefined, then
    //     a. If numberingSystem does not match the Unicode Locale Identifier type nonterminal, throw a RangeError
    //        exception.
    std::string nsStr;
    if (!numberingSystem->IsUndefined()) {
        JSHandle<EcmaString> nsEcmaStr = JSHandle<EcmaString>::Cast(numberingSystem);
        nsStr = JSLocale::ConvertToStdString(nsEcmaStr);
        if (!JSLocale::IsWellNumberingSystem(nsStr)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid numberingSystem", dateTimeFormat);
        }
    }

    // 12. Let hour12 be ? GetOption(options, "hour12", "boolean", undefined, undefined).
    JSHandle<JSTaggedValue> hour12 =
        JSLocale::GetOption(thread, dateTimeOptions, globalConst->GetHandledHour12String(), OptionType::BOOLEAN,
                            globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);

    // 13. Let hourCycle be ? GetOption(options, "hourCycle", "string", « "h11", "h12", "h23", "h24" », undefined).
    auto hourCycle = JSLocale::GetOptionOfString<HourCycleOption>(
        thread, dateTimeOptions, globalConst->GetHandledHourCycleString(),
        {HourCycleOption::H11, HourCycleOption::H12, HourCycleOption::H23, HourCycleOption::H24},
        {"h11", "h12", "h23", "h24"}, HourCycleOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);

    // 14. If hour12 is not undefined, then
    //     a. Let hourCycle be null.
    if (!hour12->IsUndefined()) {
        hourCycle = HourCycleOption::UNDEFINED;
    }

    // 16. Let localeData be %DateTimeFormat%.[[LocaleData]].
    JSHandle<TaggedArray> availableLocales = (requestedLocales->GetLength() == 0) ? factory->EmptyArray() :
                                                                                  GainAvailableLocales(thread);

    // 17. Let r be ResolveLocale(%DateTimeFormat%.[[AvailableLocales]], requestedLocales, opt, %DateTimeFormat%
    //     .[[RelevantExtensionKeys]], localeData).
    ResolvedLocale resolvedLocale =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, RELEVANT_EXTENSION_KEYS);

    // 18. Set icuLocale to r.[[locale]].
    icu::Locale icuLocale = resolvedLocale.localeData;
    ASSERT_PRINT(!icuLocale.isBogus(), "icuLocale is bogus");
    UErrorCode status = U_ZERO_ERROR;

    // Set resolvedIcuLocaleCopy to a copy of icuLocale.
    // Set icuLocale.[[ca]] to calendar.
    // Set icuLocale.[[nu]] to numberingSystem.
    icu::Locale resolvedIcuLocaleCopy(icuLocale);
    if (!calendar->IsUndefined() && JSLocale::IsWellCalendar(icuLocale, calendarStr)) {
        icuLocale.setUnicodeKeywordValue("ca", calendarStr, status);
    }
    if (!numberingSystem->IsUndefined() && JSLocale::IsWellNumberingSystem(nsStr)) {
        icuLocale.setUnicodeKeywordValue("nu", nsStr, status);
    }

    // 24. Let timeZone be ? Get(options, "timeZone").
    OperationResult operationResult =
        JSObject::GetProperty(thread, dateTimeOptions, globalConst->GetHandledTimeZoneString());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
    dateTimeFormat->SetTimeZone(thread, operationResult.GetValue());

    // 25. If timeZone is not undefined, then
    //     a. Let timeZone be ? ToString(timeZone).
    //     b. If the result of IsValidTimeZoneName(timeZone) is false, then
    //        i. Throw a RangeError exception.
    std::unique_ptr<icu::TimeZone> icuTimeZone;
    if (!operationResult.GetValue()->IsUndefined()) {
        JSHandle<EcmaString> timezone = JSTaggedValue::ToString(thread, operationResult.GetValue());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
        icuTimeZone = ConstructTimeZone(JSLocale::ConvertToStdString(timezone));
        if (icuTimeZone == nullptr) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid timeZone", dateTimeFormat);
        }
    } else {
        // 26. Else,
        //     a. Let timeZone be DefaultTimeZone().
        icuTimeZone = std::unique_ptr<icu::TimeZone>(icu::TimeZone::createDefault());
    }

    // 36.a. Let hcDefault be dataLocaleData.[[hourCycle]].
    std::unique_ptr<icu::DateTimePatternGenerator> generator(
        icu::DateTimePatternGenerator::createInstance(icuLocale, status));
    ASSERT_PRINT(U_SUCCESS(status), "constructGenerator failed");
    HourCycleOption hcDefault = OptionToHourCycle(generator->getDefaultHourCycle(status));
    // b. Let hc be dateTimeFormat.[[HourCycle]].
    HourCycleOption hc = HourCycleOption::UNDEFINED;
    hc = (hourCycle == HourCycleOption::UNDEFINED) ? OptionToHourCycle(resolvedLocale.extensions.find("hc")->second) :
                                                   hourCycle;
    // c. If hc is null, then
    //    i. Set hc to hcDefault.
    if (hc == HourCycleOption::UNDEFINED) {
        hc = hcDefault;
    }
    // d. If hour12 is not undefined, then
    if (!hour12->IsUndefined()) {
        // i. If hour12 is true, then
        if (JSTaggedValue::SameValue(hour12.GetTaggedValue(), JSTaggedValue::True())) {
            // 1. If hcDefault is "h11" or "h23", then
            if (hcDefault == HourCycleOption::H11 || hcDefault == HourCycleOption::H23) {
                // a. Set hc to "h11".
                hc = HourCycleOption::H11;
            } else {
                // 2. Else,
                //    a. Set hc to "h12".
                hc = HourCycleOption::H12;
            }
        } else {
            // ii. Else,
            //     2. If hcDefault is "h11" or "h23", then
            if (hcDefault == HourCycleOption::H11 || hcDefault == HourCycleOption::H23) {
                // a. Set hc to "h23".
                hc = HourCycleOption::H23;
            } else {
                // 3. Else,
                //    a. Set hc to "h24".
                hc = HourCycleOption::H24;
            }
        }
    }

    // Set isHourDefined be false when dateTimeFormat.[[Hour]] is not undefined.
    bool isHourDefined = false;

    // 29. For each row of Table 6, except the header row, in table order, do
    //     a. Let prop be the name given in the Property column of the row.
    //     b. Let value be ? GetOption(options, prop, "string", « the strings given in the Values column of the
    //        row », undefined).
    //     c. Set opt.[[<prop>]] to value.
    std::string skeleton;
    std::vector<IcuPatternDesc> data = GetIcuPatternDesc(hc);
    for (const IcuPatternDesc &item : data) {
        // prop be [[TimeZoneName]]
        if (item.property == "timeZoneName") {
            int secondDigitsString = JSLocale::GetNumberOption(thread, dateTimeOptions,
                                                               globalConst->GetHandledFractionalSecondDigitsString(),
                                                               1, 3, 0);
            skeleton.append(secondDigitsString, 'S');
        }
        JSHandle<JSTaggedValue> property(thread, factory->NewFromStdString(item.property).GetTaggedValue());
        std::string value;
        bool isFind = JSLocale::GetOptionOfString(thread, dateTimeOptions, property, item.allowedValues, &value);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
        if (isFind) {
            skeleton += item.map.find(value)->second;
            // [[Hour]] is defined.
            isHourDefined = (item.property == "hour") ? true : isHourDefined;
        }
    }

    // 13.1.3 BasicFormatMatcher (options, formats)
    [[maybe_unused]] auto formatMatcher = JSLocale::GetOptionOfString<FormatMatcherOption>(
        thread, dateTimeOptions, globalConst->GetHandledFormatMatcherString(),
        {FormatMatcherOption::BASIC, FormatMatcherOption::BEST_FIT}, {"basic", "best fit"},
        FormatMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);

    // Let dateStyle be ? GetOption(options, "string", «"full", "long", "medium", "short"», undefined).
    // Set dateTimeFormat.[[dateStyle]]
    auto dateStyle = JSLocale::GetOptionOfString<DateTimeStyleOption>(
        thread, dateTimeOptions, globalConst->GetHandledDateStyleString(),
        {DateTimeStyleOption::FULL, DateTimeStyleOption::LONG, DateTimeStyleOption::MEDIUM, DateTimeStyleOption::SHORT},
        {"full", "long", "medium", "short"}, DateTimeStyleOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
    dateTimeFormat->SetDateStyle(thread, JSTaggedValue(static_cast<int32_t>(dateStyle)));

    // Let timeStyle be ? GetOption(options, "string", «"full", "long", "medium", "short"», undefined).
    // Set dateTimeFormat.[[timeStyle]]
    auto timeStyle = JSLocale::GetOptionOfString<DateTimeStyleOption>(
        thread, dateTimeOptions, globalConst->GetHandledTimeStyleString(),
        {DateTimeStyleOption::FULL, DateTimeStyleOption::LONG, DateTimeStyleOption::MEDIUM, DateTimeStyleOption::SHORT},
        {"full", "long", "medium", "short"}, DateTimeStyleOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDateTimeFormat, thread);
    dateTimeFormat->SetTimeStyle(thread, JSTaggedValue(static_cast<int32_t>(timeStyle)));

    HourCycleOption dtfHourCycle = HourCycleOption::UNDEFINED;

    // If dateTimeFormat.[[Hour]] is defined, then
    if (isHourDefined) {
        // e. Set dateTimeFormat.[[HourCycle]] to hc.
        dtfHourCycle = hc;
    } else {
        // 37. Else,
        //     a. Set dateTimeFormat.[[HourCycle]] to undefined.
        dtfHourCycle = HourCycleOption::UNDEFINED;
    }

    // Set dateTimeFormat.[[hourCycle]].
    dateTimeFormat->SetHourCycle(thread, JSTaggedValue(static_cast<int32_t>(dtfHourCycle)));

    // Set dateTimeFormat.[[icuLocale]].
    dateTimeFormat->SetIcuLocale(thread, icuLocale, JSDateTimeFormat::FreeIcuLocale);

    // Creates a Calendar using the given timezone and given locale.
    // Set dateTimeFormat.[[icuSimpleDateFormat]].
    icu::UnicodeString dtfSkeleton(skeleton.c_str());
    status = U_ZERO_ERROR;
    icu::UnicodeString pattern = ChangeHourCyclePattern(
        generator.get()->getBestPattern(dtfSkeleton, UDATPG_MATCH_HOUR_FIELD_LENGTH, status), dtfHourCycle);
    ASSERT_PRINT((U_SUCCESS(status) != 0), "get best pattern failed");
    auto simpleDateFormatIcu(std::make_unique<icu::SimpleDateFormat>(pattern, icuLocale, status));
    if (U_FAILURE(status) != 0) {
        simpleDateFormatIcu = std::unique_ptr<icu::SimpleDateFormat>();
    }
    ASSERT_PRINT(simpleDateFormatIcu != nullptr, "invalid icuSimpleDateFormat");
    std::unique_ptr<icu::Calendar> calendarPtr = BuildCalendar(icuLocale, *icuTimeZone);
    ASSERT_PRINT(calendarPtr != nullptr, "invalid calendar");
    simpleDateFormatIcu->adoptCalendar(calendarPtr.release());
    dateTimeFormat->SetIcuSimpleDateFormat(thread, *simpleDateFormatIcu, JSDateTimeFormat::FreeSimpleDateFormat);

    // Set dateTimeFormat.[[iso8601]].
    bool iso8601 = strstr(icuLocale.getName(), "calendar=iso8601") != nullptr;
    dateTimeFormat->SetIso8601(thread, JSTaggedValue(iso8601));

    // Set dateTimeFormat.[[locale]].
    if (!hour12->IsUndefined() || hourCycle != HourCycleOption::UNDEFINED) {
        if ((resolvedLocale.extensions.find("hc") != resolvedLocale.extensions.end())
            && (dtfHourCycle != OptionToHourCycle((resolvedLocale.extensions.find("hc")->second)))) {
            resolvedIcuLocaleCopy.setUnicodeKeywordValue("hc", nullptr, status);
            ASSERT_PRINT(U_SUCCESS(status), "resolvedIcuLocaleCopy set hc failed");
        }
    }
    JSHandle<EcmaString> localeStr = JSLocale::ToLanguageTag(thread, resolvedIcuLocaleCopy);
    dateTimeFormat->SetLocale(thread, localeStr.GetTaggedValue());

    // Set dateTimeFormat.[[boundFormat]].
    dateTimeFormat->SetBoundFormat(thread, JSTaggedValue::Undefined());

    // 39. Return dateTimeFormat.
    return dateTimeFormat;
}

// 13.1.2 ToDateTimeOptions (options, required, defaults)
JSHandle<JSObject> JSDateTimeFormat::ToDateTimeOptions(JSThread *thread, const JSHandle<JSTaggedValue> &options,
                                                       const RequiredOption &required, const DefaultsOption &defaults)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If options is undefined, let options be null; otherwise let options be ? ToObject(options).
    JSHandle<JSObject> optionsResult(thread, JSTaggedValue::Null());
    if (!options->IsUndefined()) {
        optionsResult = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    }

    // 2. Let options be ObjectCreate(options).
    optionsResult = JSObject::ObjectCreate(thread, optionsResult);

    // 3. Let needDefaults be true.
    bool needDefaults = true;

    // 4. If required is "date" or "any", then
    //    a. For each of the property names "weekday", "year", "month", "day", do
    //      i. Let prop be the property name.
    //      ii. Let value be ? Get(options, prop).
    //      iii. If value is not undefined, let needDefaults be false.
    auto globalConst = thread->GlobalConstants();
    if (required == RequiredOption::DATE || required == RequiredOption::ANY) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(CAPACITY_4);
        array->Set(thread, 0, globalConst->GetHandledWeekdayString());
        array->Set(thread, 1, globalConst->GetHandledYearString());
        array->Set(thread, 2, globalConst->GetHandledMonthString());  // 2 means the third slot
        array->Set(thread, 3, globalConst->GetHandledDayString());    // 3 means the fourth slot
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        array_size_t len = array->GetLength();
        for (array_size_t i = 0; i < len; i++) {
            key.Update(array->Get(thread, i));
            OperationResult operationResult = JSObject::GetProperty(thread, optionsResult, key);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
            if (!operationResult.GetValue()->IsUndefined()) {
                needDefaults = false;
            }
        }
    }

    // 5. If required is "time" or "any", then
    //    a. For each of the property names "dayPeriod", "hour", "minute", "second", "fractionalSecondDigits", do
    //      i. Let prop be the property name.
    //      ii. Let value be ? Get(options, prop).
    //      iii. If value is not undefined, let needDefaults be false.
    if (required == RequiredOption::TIME || required == RequiredOption::ANY) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(CAPACITY_5);
        array->Set(thread, 0, globalConst->GetHandledDayPeriodString());
        array->Set(thread, 1, globalConst->GetHandledHourString());
        array->Set(thread, 2, globalConst->GetHandledMinuteString());   // 2 means the second slot
        array->Set(thread, 3, globalConst->GetHandledSecondString());   // 3 means the third slot
        array->Set(thread, 4, globalConst->GetHandledFractionalSecondDigitsString());   // 4 means the fourth slot
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        array_size_t len = array->GetLength();
        for (array_size_t i = 0; i < len; i++) {
            key.Update(array->Get(thread, i));
            OperationResult operationResult = JSObject::GetProperty(thread, optionsResult, key);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
            if (!operationResult.GetValue()->IsUndefined()) {
                needDefaults = false;
            }
        }
    }

    // Let dateStyle/timeStyle be ? Get(options, "dateStyle"/"timeStyle").
    OperationResult dateStyleResult =
        JSObject::GetProperty(thread, optionsResult, globalConst->GetHandledDateStyleString());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    JSHandle<JSTaggedValue> dateStyle = dateStyleResult.GetValue();
    OperationResult timeStyleResult =
        JSObject::GetProperty(thread, optionsResult, globalConst->GetHandledTimeStyleString());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    JSHandle<JSTaggedValue> timeStyle = timeStyleResult.GetValue();

    // If dateStyle is not undefined or timeStyle is not undefined, let needDefaults be false.
    if (!dateStyle->IsUndefined() || !timeStyle->IsUndefined()) {
        needDefaults = false;
    }

    // If required is "date"/"time" and timeStyle is not undefined, throw a TypeError exception.
    if (required == RequiredOption::DATE && !timeStyle->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "timeStyle is not undefined", optionsResult);
    }
    if (required == RequiredOption::TIME && !dateStyle->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "dateStyle is not undefined", optionsResult);
    }

    // 6. If needDefaults is true and defaults is either "date" or "all", then
    //    a. For each of the property names "year", "month", "day", do
    //       i. Perform ? CreateDataPropertyOrThrow(options, prop, "numeric").
    if (needDefaults && (defaults == DefaultsOption::DATE || defaults == DefaultsOption::ALL)) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(CAPACITY_3);
        array->Set(thread, 0, globalConst->GetHandledYearString());
        array->Set(thread, 1, globalConst->GetHandledMonthString());
        array->Set(thread, 2, globalConst->GetHandledDayString());  // 2 means the third slot
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        array_size_t len = array->GetLength();
        for (array_size_t i = 0; i < len; i++) {
            key.Update(array->Get(thread, i));
            JSObject::CreateDataPropertyOrThrow(thread, optionsResult, key, globalConst->GetHandledNumericString());
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
        }
    }

    // 7. If needDefaults is true and defaults is either "time" or "all", then
    //    a. For each of the property names "hour", "minute", "second", do
    //       i. Perform ? CreateDataPropertyOrThrow(options, prop, "numeric").
    if (needDefaults && (defaults == DefaultsOption::TIME || defaults == DefaultsOption::ALL)) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(CAPACITY_3);
        array->Set(thread, 0, globalConst->GetHandledHourString());
        array->Set(thread, 1, globalConst->GetHandledMinuteString());
        array->Set(thread, 2, globalConst->GetHandledSecondString());  // 2 means the third slot
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        array_size_t len = array->GetLength();
        for (array_size_t i = 0; i < len; i++) {
            key.Update(array->Get(thread, i));
            JSObject::CreateDataPropertyOrThrow(thread, optionsResult, key, globalConst->GetHandledNumericString());
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
        }
    }

    // 8. Return options.
    return optionsResult;
}

// 13.1.7 FormatDateTime(dateTimeFormat, x)
JSHandle<EcmaString> JSDateTimeFormat::FormatDateTime(JSThread *thread,
                                                      const JSHandle<JSDateTimeFormat> &dateTimeFormat, double x)
{
    icu::SimpleDateFormat *simpleDateFormat = dateTimeFormat->GetIcuSimpleDateFormat();
    // 1. Let parts be ? PartitionDateTimePattern(dateTimeFormat, x).
    double xValue = JSDate::TimeClip(x);
    if (std::isnan(xValue)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid time value", thread->GetEcmaVM()->GetFactory()->GetEmptyString());
    }

    // 2. Let result be the empty String.
    icu::UnicodeString result;

    // 3. Set result to the string-concatenation of result and part.[[Value]].
    simpleDateFormat->format(xValue, result);

    // 4. Return result.
    return JSLocale::IcuToString(thread, result);
}

// 13.1.8 FormatDateTimeToParts (dateTimeFormat, x)
JSHandle<JSArray> JSDateTimeFormat::FormatDateTimeToParts(JSThread *thread,
                                                          const JSHandle<JSDateTimeFormat> &dateTimeFormat, double x)
{
    icu::SimpleDateFormat *simpleDateFormat = dateTimeFormat->GetIcuSimpleDateFormat();
    ASSERT(simpleDateFormat != nullptr);
    UErrorCode status = U_ZERO_ERROR;
    icu::FieldPositionIterator fieldPositionIter;
    icu::UnicodeString formattedParts;
    simpleDateFormat->format(x, formattedParts, &fieldPositionIter, status);
    if (U_FAILURE(status) != 0) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "format failed", thread->GetEcmaVM()->GetFactory()->NewJSArray());
    }

    // 2. Let result be ArrayCreate(0).
    JSHandle<JSArray> result(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    if (formattedParts.isBogus()) {
        return result;
    }

    // 3. Let n be 0.
    int32_t index = 0;
    int32_t preEdgePos = 0;
    std::vector<CommonDateFormatPart> parts;
    icu::FieldPosition fieldPosition;
    while (fieldPositionIter.next(fieldPosition)) {
        int32_t fField = fieldPosition.getField();
        int32_t fBeginIndex = fieldPosition.getBeginIndex();
        int32_t fEndIndex = fieldPosition.getEndIndex();
        if (preEdgePos < fBeginIndex) {
            parts.emplace_back(CommonDateFormatPart(fField, preEdgePos, fBeginIndex, index, true));
            ++index;
        }
        parts.emplace_back(CommonDateFormatPart(fField, fBeginIndex, fEndIndex, index, false));
        preEdgePos = fEndIndex;
        ++index;
    }
    int32_t length = formattedParts.length();
    if (preEdgePos < length) {
        parts.emplace_back(CommonDateFormatPart(-1, preEdgePos, length, index, true));
    }
    JSMutableHandle<EcmaString> substring(thread, JSTaggedValue::Undefined());

    // 4. For each part in parts, do
    for (auto part : parts) {
        substring.Update(JSLocale::IcuToString(thread, formattedParts, part.fBeginIndex,
                                               part.fEndIndex).GetTaggedValue());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
        // Let O be ObjectCreate(%ObjectPrototype%).
        // Perform ! CreateDataPropertyOrThrow(O, "type", part.[[Type]]).
        // Perform ! CreateDataPropertyOrThrow(O, "value", part.[[Value]]).
        // Perform ! CreateDataProperty(result, ! ToString(n), O).
        if (part.isPreExist) {
            JSLocale::PutElement(thread, part.index, result, ConvertFieldIdToDateType(thread, -1),
                                 JSHandle<JSTaggedValue>::Cast(substring));
        } else {
            JSLocale::PutElement(thread, part.index, result, ConvertFieldIdToDateType(thread, part.fField),
                                 JSHandle<JSTaggedValue>::Cast(substring));
        }
    }

    // 5. Return result.
    return result;
}

// 13.1.10 UnwrapDateTimeFormat(dtf)
JSHandle<JSTaggedValue> JSDateTimeFormat::UnwrapDateTimeFormat(JSThread *thread,
                                                               const JSHandle<JSTaggedValue> &dateTimeFormat)
{
    // 1. Assert: Type(dtf) is Object.
    ASSERT_PRINT(dateTimeFormat->IsJSObject(), "dateTimeFormat is not object");

    // 2. If dateTimeFormat does not have an [[InitializedDateTimeFormat]] internal slot
    //    and ? InstanceofOperator(dateTimeFormat, %DateTimeFormat%) is true, then
    //       a. Let dateTimeFormat be ? Get(dateTimeFormat, %Intl%.[[FallbackSymbol]]).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    bool isInstanceOf = JSFunction::InstanceOf(thread, dateTimeFormat, env->GetDateTimeFormatFunction());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, dateTimeFormat);
    if (!dateTimeFormat->IsJSDateTimeFormat() && isInstanceOf) {
        JSHandle<JSTaggedValue> key(thread, JSHandle<JSIntl>::Cast(env->GetIntlFunction())->GetFallbackSymbol());
        OperationResult operationResult = JSTaggedValue::GetProperty(thread, dateTimeFormat, key);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, dateTimeFormat);
        return operationResult.GetValue();
    }

    // 3. Perform ? RequireInternalSlot(dateTimeFormat, [[InitializedDateTimeFormat]]).
    if (!dateTimeFormat->IsJSDateTimeFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "is not JSDateTimeFormat",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }

    // 4. Return dateTimeFormat.
    return dateTimeFormat;
}

JSHandle<JSTaggedValue> ToHourCycleEcmaString(JSThread *thread, int32_t hc)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (hc) {
        case static_cast<int32_t>(HourCycleOption::H11):
            result.Update(globalConst->GetHandledH11String().GetTaggedValue());
            break;
        case static_cast<int32_t>(HourCycleOption::H12):
            result.Update(globalConst->GetHandledH12String().GetTaggedValue());
            break;
        case static_cast<int32_t>(HourCycleOption::H23):
            result.Update(globalConst->GetHandledH23String().GetTaggedValue());
            break;
        case static_cast<int32_t>(HourCycleOption::H24):
            result.Update(globalConst->GetHandledH24String().GetTaggedValue());
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> ToDateTimeStyleEcmaString(JSThread *thread, int32_t style)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (style) {
        case static_cast<int32_t>(DateTimeStyleOption::FULL):
            result.Update(globalConst->GetHandledFullString().GetTaggedValue());
            break;
        case static_cast<int32_t>(DateTimeStyleOption::LONG):
            result.Update(globalConst->GetHandledLongString().GetTaggedValue());
            break;
        case static_cast<int32_t>(DateTimeStyleOption::MEDIUM):
            result.Update(globalConst->GetHandledMediumString().GetTaggedValue());
            break;
        case static_cast<int32_t>(DateTimeStyleOption::SHORT):
            result.Update(globalConst->GetHandledShortString().GetTaggedValue());
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

// 13.4.5  Intl.DateTimeFormat.prototype.resolvedOptions ()
void JSDateTimeFormat::ResolvedOptions(JSThread *thread, const JSHandle<JSDateTimeFormat> &dateTimeFormat,
                                       const JSHandle<JSObject> &options)
{   //  Table 8: Resolved Options of DateTimeFormat Instances
    //    Internal Slot	        Property
    //    [[Locale]]	        "locale"
    //    [[Calendar]]	        "calendar"
    //    [[NumberingSystem]]	"numberingSystem"
    //    [[TimeZone]]	        "timeZone"
    //    [[HourCycle]]	        "hourCycle"
    //                          "hour12"
    //    [[Weekday]]	        "weekday"
    //    [[Era]]	            "era"
    //    [[Year]]	            "year"
    //    [[Month]]         	"month"
    //    [[Day]]	            "day"
    //    [[Hour]]	            "hour"
    //    [[Minute]]	        "minute"
    //    [[Second]]        	"second"
    //    [[TimeZoneName]]	    "timeZoneName"
    auto globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // 5. For each row of Table 8, except the header row, in table order, do
    //    Let p be the Property value of the current row.
    // [[Locale]]
    JSHandle<JSTaggedValue> locale(thread, dateTimeFormat->GetLocale());
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleString();
    JSObject::CreateDataPropertyOrThrow(thread, options, property, locale);
    // [[Calendar]]
    JSMutableHandle<JSTaggedValue> calendarValue(thread, dateTimeFormat->GetCalendar());
    icu::SimpleDateFormat *icuSimpleDateFormat = dateTimeFormat->GetIcuSimpleDateFormat();
    const icu::Calendar *calendar = icuSimpleDateFormat->getCalendar();
    std::string icuCalendar = calendar->getType();
    if (icuCalendar == "gregorian") {
        if (dateTimeFormat->GetIso8601() == JSTaggedValue::True()) {
            calendarValue.Update(globalConst->GetHandledIso8601String().GetTaggedValue());
        } else {
            calendarValue.Update(globalConst->GetHandledGregoryString().GetTaggedValue());
        }
    } else if (icuCalendar == "ethiopic-amete-alem") {
        calendarValue.Update(globalConst->GetHandledEthioaaString().GetTaggedValue());
    }
    property = globalConst->GetHandledCalendarString();
    JSObject::CreateDataPropertyOrThrow(thread, options, property, calendarValue);
    // [[NumberingSystem]]
    JSHandle<JSTaggedValue> numberingSystem(thread, dateTimeFormat->GetNumberingSystem());
    if (numberingSystem->IsUndefined()) {
        numberingSystem = globalConst->GetHandledLatnString();
    }
    property = globalConst->GetHandledNumberingSystemString();
    JSObject::CreateDataPropertyOrThrow(thread, options, property, numberingSystem);
    // [[TimeZone]]
    JSMutableHandle<JSTaggedValue> timezoneValue(thread, dateTimeFormat->GetTimeZone());
    const icu::TimeZone &icuTZ = calendar->getTimeZone();
    icu::UnicodeString timezone;
    icuTZ.getID(timezone);
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString canonicalTimezone;
    icu::TimeZone::getCanonicalID(timezone, canonicalTimezone, status);
    if (U_SUCCESS(status) != 0) {
        if ((canonicalTimezone == UNICODE_STRING_SIMPLE("Etc/UTC")) != 0 ||
            (canonicalTimezone == UNICODE_STRING_SIMPLE("Etc/GMT")) != 0) {
            timezoneValue.Update(globalConst->GetUTCString());
        } else {
            timezoneValue.Update(JSLocale::IcuToString(thread, canonicalTimezone).GetTaggedValue());
        }
    }
    property = globalConst->GetHandledTimeZoneString();
    JSObject::CreateDataPropertyOrThrow(thread, options, property, timezoneValue);
    // [[HourCycle]]
    // For web compatibility reasons, if the property "hourCycle" is set, the "hour12" property should be set to true
    // when "hourCycle" is "h11" or "h12", or to false when "hourCycle" is "h23" or "h24".
    // i. Let hc be dtf.[[HourCycle]].
    JSHandle<JSTaggedValue> hcValue;
    HourCycleOption hc = static_cast<HourCycleOption>(dateTimeFormat->GetHourCycle().GetInt());
    if (hc != HourCycleOption::UNDEFINED) {
        property = globalConst->GetHandledHourCycleString();
        hcValue = ToHourCycleEcmaString(thread, dateTimeFormat->GetHourCycle().GetInt());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, hcValue);
        if (hc == HourCycleOption::H11 || hc == HourCycleOption::H12) {
            JSHandle<JSTaggedValue> trueValue(thread, JSTaggedValue::True());
            hcValue = trueValue;
        } else if (hc == HourCycleOption::H23 || hc == HourCycleOption::H24) {
            JSHandle<JSTaggedValue> falseValue(thread, JSTaggedValue::False());
            hcValue = falseValue;
        }
        property = globalConst->GetHandledHour12String();
        JSObject::CreateDataPropertyOrThrow(thread, options, property, hcValue);
    }
    // [[DateStyle]], [[TimeStyle]].
    icu::UnicodeString patternUnicode;
    icuSimpleDateFormat->toPattern(patternUnicode);
    std::string pattern;
    patternUnicode.toUTF8String(pattern);
    if (dateTimeFormat->GetDateStyle() == JSTaggedValue(static_cast<int32_t>(DateTimeStyleOption::UNDEFINED)) &&
        dateTimeFormat->GetTimeStyle() == JSTaggedValue(static_cast<int32_t>(DateTimeStyleOption::UNDEFINED))) {
        for (const auto &item : BuildIcuPatternDescs()) {
            // fractionalSecondsDigits need to be added before timeZoneName.
            if (item.property == "timeZoneName") {
                int tmpResult = count(pattern.begin(), pattern.end(), 'S');
                int fsd = (tmpResult >= STRING_LENGTH_3) ? STRING_LENGTH_3 : tmpResult;
                if (fsd > 0) {
                    JSHandle<JSTaggedValue> fsdValue(thread, JSTaggedValue(fsd));
                    property = globalConst->GetHandledFractionalSecondDigitsString();
                    JSObject::CreateDataPropertyOrThrow(thread, options, property, fsdValue);
                }
            }
            property = JSHandle<JSTaggedValue>::Cast(factory->NewFromStdString(item.property));
            for (const auto &pair : item.pairs) {
                if (pattern.find(pair.first) != std::string::npos) {
                    hcValue = JSHandle<JSTaggedValue>::Cast(factory->NewFromStdString(pair.second));
                    JSObject::CreateDataPropertyOrThrow(thread, options, property, hcValue);
                    break;
                }
            }
        }
    }
    if (dateTimeFormat->GetDateStyle() != JSTaggedValue(static_cast<int32_t>(DateTimeStyleOption::UNDEFINED))) {
        property = globalConst->GetHandledDateStyleString();
        hcValue = ToDateTimeStyleEcmaString(thread, dateTimeFormat->GetDateStyle().GetInt());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, hcValue);
    }
    if (dateTimeFormat->GetTimeStyle() != JSTaggedValue(static_cast<int32_t>(DateTimeStyleOption::UNDEFINED))) {
        property = globalConst->GetHandledTimeStyleString();
        hcValue = ToDateTimeStyleEcmaString(thread, dateTimeFormat->GetTimeStyle().GetInt());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, hcValue);
    }
}

// Use dateInterval(x, y) construct datetimeformatrange
icu::FormattedDateInterval JSDateTimeFormat::ConstructDTFRange(JSThread *thread, const JSHandle<JSDateTimeFormat> &dtf,
                                                               double x, double y)
{
    std::unique_ptr<icu::DateIntervalFormat> dateIntervalFormat(ConstructDateIntervalFormat(dtf));
    if (dateIntervalFormat == nullptr) {
        icu::FormattedDateInterval emptyValue;
        THROW_TYPE_ERROR_AND_RETURN(thread, "create dateIntervalFormat failed", emptyValue);
    }
    UErrorCode status = U_ZERO_ERROR;
    icu::DateInterval dateInterval(x, y);
    icu::FormattedDateInterval formatted = dateIntervalFormat->formatToValue(dateInterval, status);
    return formatted;
}

JSHandle<EcmaString> JSDateTimeFormat::NormDateTimeRange(JSThread *thread, const JSHandle<JSDateTimeFormat> &dtf,
                                                         double x, double y)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> result = factory->GetEmptyString();
    // 1. Let x be TimeClip(x).
    x = JSDate::TimeClip(x);
    // 2. If x is NaN, throw a RangeError exception.
    if (std::isnan(x)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "x is NaN", result);
    }
    // 3. Let y be TimeClip(y).
    y = JSDate::TimeClip(y);
    // 4. If y is NaN, throw a RangeError exception.
    if (std::isnan(y)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "y is NaN", result);
    }

    icu::FormattedDateInterval formatted = ConstructDTFRange(thread, dtf, x, y);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);

    // Formatted to string.
    bool outputRange = false;
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString formatResult = formatted.toString(status);
    if (U_FAILURE(status) != 0) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "format to string failed",
                                    thread->GetEcmaVM()->GetFactory()->GetEmptyString());
    }
    icu::ConstrainedFieldPosition cfpos;
    while (formatted.nextPosition(cfpos, status) != 0) {
        if (cfpos.getCategory() == UFIELD_CATEGORY_DATE_INTERVAL_SPAN) {
            outputRange = true;
            break;
        }
    }
    result = JSLocale::IcuToString(thread, formatResult);
    if (!outputRange) {
        return FormatDateTime(thread, dtf, x);
    }
    return result;
}

JSHandle<JSArray> JSDateTimeFormat::NormDateTimeRangeToParts(JSThread *thread, const JSHandle<JSDateTimeFormat> &dtf,
                                                             double x, double y)
{
    JSHandle<JSArray> result(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    // 1. Let x be TimeClip(x).
    x = JSDate::TimeClip(x);
    // 2. If x is NaN, throw a RangeError exception.
    if (std::isnan(x)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "x is invalid time value", result);
    }
    // 3. Let y be TimeClip(y).
    y = JSDate::TimeClip(y);
    // 4. If y is NaN, throw a RangeError exception.
    if (std::isnan(y)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "y is invalid time value", result);
    }

    icu::FormattedDateInterval formatted = ConstructDTFRange(thread, dtf, x, y);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    return ConstructFDateIntervalToJSArray(thread, formatted);
}

JSHandle<TaggedArray> JSDateTimeFormat::GainAvailableLocales(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateTimeFormatLocales = env->GetDateTimeFormatLocales();
    const char *key = "calendar";
    const char *path = nullptr;
    if (dateTimeFormatLocales->IsUndefined()) {
        JSHandle<TaggedArray> availableLocales = JSLocale::GetAvailableLocales(thread, key, path);
        env->SetDateTimeFormatLocales(thread, availableLocales);
        return availableLocales;
    }
    return JSHandle<TaggedArray>::Cast(dateTimeFormatLocales);
}

JSHandle<JSArray> JSDateTimeFormat::ConstructFDateIntervalToJSArray(JSThread *thread,
                                                                    const icu::FormattedDateInterval &formatted)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString formattedValue = formatted.toTempString(status);
    // Let result be ArrayCreate(0).
    JSHandle<JSArray> array(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    // Let index be 0.
    int index = 0;
    int32_t preEndPos = 0;
    std::array<int32_t, 2> begin {};
    std::array<int32_t, 2> end {};
    begin[0] = begin[1] = end[0] = end[1] = 0;
    std::vector<CommonDateFormatPart> parts;

    /**
     * From ICU header file document @unumberformatter.h
     * Sets a constraint on the field category.
     *
     * When this instance of ConstrainedFieldPosition is passed to FormattedValue#nextPosition,
     * positions are skipped unless they have the given category.
     *
     * Any previously set constraints are cleared.
     *
     * For example, to loop over only the number-related fields:
     *
     *     ConstrainedFieldPosition cfpo;
     *     cfpo.constrainCategory(UFIELDCATEGORY_NUMBER_FORMAT);
     *     while (fmtval.nextPosition(cfpo, status)) {
     *         // handle the number-related field position
     *     }
     */
    JSMutableHandle<EcmaString> substring(thread, JSTaggedValue::Undefined());
    icu::ConstrainedFieldPosition cfpos;
    while (formatted.nextPosition(cfpos, status)) {
        int32_t fCategory = cfpos.getCategory();
        int32_t fField = cfpos.getField();
        int32_t fStart = cfpos.getStart();
        int32_t fLimit = cfpos.getLimit();

        // 2 means the number of elements in category
        if (fCategory == UFIELD_CATEGORY_DATE_INTERVAL_SPAN && (fField == 0 || fField == 1)) {
            begin[fField] = fStart;
            end[fField] = fLimit;
        }
        if (fCategory == UFIELD_CATEGORY_DATE) {
            if (preEndPos < fStart) {
                parts.emplace_back(CommonDateFormatPart(fField, preEndPos, fStart, index, true));
                preEndPos = fStart;  // NOLINT(clang-analyzer-deadcode.DeadStores)
                index++;
            }
            parts.emplace_back(CommonDateFormatPart(fField, fStart, fLimit, index, false));
            preEndPos = fLimit;
            ++index;
        }
    }
    if (U_FAILURE(status) != 0) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "format date interval error", array);
    }
    int32_t length = formattedValue.length();
    if (length > preEndPos) {
        parts.emplace_back(CommonDateFormatPart(-1, preEndPos, length, index, true));
    }
    for (auto part : parts) {
        substring.Update(JSLocale::IcuToString(thread, formattedValue, part.fBeginIndex,
                                               part.fEndIndex).GetTaggedValue());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
        JSHandle<JSObject> element;
        if (part.isPreExist) {
            element = JSLocale::PutElement(thread, part.index, array, ConvertFieldIdToDateType(thread, -1),
                                           JSHandle<JSTaggedValue>::Cast(substring));
        } else {
            element = JSLocale::PutElement(thread, part.index, array, ConvertFieldIdToDateType(thread, part.fField),
                                           JSHandle<JSTaggedValue>::Cast(substring));
        }
        JSHandle<JSTaggedValue> value = JSHandle<JSTaggedValue>::Cast(
            ToValueString(thread, TrackValue(part.fBeginIndex, part.fEndIndex, begin, end)));
        JSObject::SetProperty(thread, element, thread->GlobalConstants()->GetHandledSourceString(), value, true);
    }
    return array;
}

Value JSDateTimeFormat::TrackValue(int32_t beginning, int32_t ending,
                                   std::array<int32_t, 2> begin, std::array<int32_t, 2> end)
{
    Value value = Value::SHARED;
    if ((begin[0] <= beginning) && (beginning <= end[0]) && (begin[0] <= ending) && (ending <= end[0])) {
        value = Value::START_RANGE;
    } else if ((begin[1] <= beginning) && (beginning <= end[1]) && (begin[1] <= ending) && (ending <= end[1])) {
        value = Value::END_RANGE;
    }
    return value;
}

std::vector<IcuPatternDesc> BuildIcuPatternDescs()
{
    std::vector<IcuPatternDesc> items = {
        IcuPatternDesc("weekday", ICU_WEEKDAY_PE, ICU_NARROW_LONG_SHORT),
        IcuPatternDesc("era", ICU_ERA_PE, ICU_NARROW_LONG_SHORT),
        IcuPatternDesc("year", ICU_YEAR_PE, ICU2_DIGIT_NUMERIC),
        IcuPatternDesc("month", ICU_MONTH_PE, ICU_NARROW_LONG_SHORT2_DIGIT_NUMERIC),
        IcuPatternDesc("day", ICU_DAY_PE, ICU2_DIGIT_NUMERIC),
        IcuPatternDesc("dayPeriod", ICU_DAY_PERIOD_PE, ICU_NARROW_LONG_SHORT),
        IcuPatternDesc("hour", ICU_HOUR_PE, ICU2_DIGIT_NUMERIC),
        IcuPatternDesc("minute", ICU_MINUTE_PE, ICU2_DIGIT_NUMERIC),
        IcuPatternDesc("second", ICU_SECOND_PE, ICU2_DIGIT_NUMERIC),
        IcuPatternDesc("timeZoneName", ICU_YIME_ZONE_NAME_PE, ICU_LONG_SHORT)
    };
    return items;
}

std::vector<IcuPatternDesc> InitializePattern(const IcuPatternDesc &hourData)
{
    std::vector<IcuPatternDesc> result;
    std::vector<IcuPatternDesc> items = BuildIcuPatternDescs();
    std::vector<IcuPatternDesc>::iterator item = items.begin();
    while (item != items.end()) {
        if (item->property != "hour") {
            result.emplace_back(IcuPatternDesc(item->property, item->pairs, item->allowedValues));
        } else {
            result.emplace_back(hourData);
        }
        item++;
    }
    return result;
}

std::vector<IcuPatternDesc> JSDateTimeFormat::GetIcuPatternDesc(const HourCycleOption &hourCycle)
{
    if (hourCycle == HourCycleOption::H11) {
        Pattern h11("KK", "K");
        return h11.Get();
    } else if (hourCycle == HourCycleOption::H12) {
        Pattern h12("hh", "h");
        return h12.Get();
    } else if (hourCycle == HourCycleOption::H23) {
        Pattern h23("HH", "H");
        return h23.Get();
    } else if (hourCycle == HourCycleOption::H24) {
        Pattern h24("kk", "k");
        return h24.Get();
    } else if (hourCycle == HourCycleOption::UNDEFINED) {
        Pattern pattern("jj", "j");
        return pattern.Get();
    }
    UNREACHABLE();
}

icu::UnicodeString JSDateTimeFormat::ChangeHourCyclePattern(const icu::UnicodeString &pattern, HourCycleOption hc)
{
    if (hc == HourCycleOption::UNDEFINED || hc == HourCycleOption::EXCEPTION) {
        return pattern;
    }
    icu::UnicodeString result;
    char16_t key = u'\0';
    auto mapIter = std::find_if(HOUR_CYCLE_MAP.begin(), HOUR_CYCLE_MAP.end(),
        [hc](const std::map<char16_t, HourCycleOption>::value_type item) {
                                    return item.second == hc;
    });
    if (mapIter != HOUR_CYCLE_MAP.end()) {
        key = mapIter->first;
    }
    bool needChange = true;
    char16_t last = u'\0';
    for (int32_t i = 0; i < pattern.length(); i++) {
        char16_t ch = pattern.charAt(i);
        if (ch == '\'') {
            needChange = !needChange;
            result.append(ch);
        } else if (HOUR_CYCLE_MAP.find(ch) != HOUR_CYCLE_MAP.end()) {
            result = (needChange && last == u'd') ? result.append(' ') : result;
            result.append(needChange ? key : ch);
        } else {
            result.append(ch);
        }
        last = ch;
    }
    return result;
}

std::unique_ptr<icu::SimpleDateFormat> JSDateTimeFormat::CreateICUSimpleDateFormat(const icu::Locale &icuLocale,
                                                                                   const icu::UnicodeString &skeleton,
                                                                                   icu::DateTimePatternGenerator *gn,
                                                                                   HourCycleOption hc)
{
    // See https://github.com/tc39/ecma402/issues/225
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString pattern = ChangeHourCyclePattern(
        gn->getBestPattern(skeleton, UDATPG_MATCH_HOUR_FIELD_LENGTH, status), hc);
    ASSERT_PRINT((U_SUCCESS(status) != 0), "get best pattern failed");

    status = U_ZERO_ERROR;
    auto dateFormat(std::make_unique<icu::SimpleDateFormat>(pattern, icuLocale, status));
    if (U_FAILURE(status) != 0) {
        return std::unique_ptr<icu::SimpleDateFormat>();
    }
    ASSERT_PRINT(dateFormat != nullptr, "dateFormat failed");
    return dateFormat;
}

std::unique_ptr<icu::Calendar> JSDateTimeFormat::BuildCalendar(const icu::Locale &locale, const icu::TimeZone &timeZone)
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::Calendar> calendar(icu::Calendar::createInstance(timeZone, locale, status));
    ASSERT_PRINT(U_SUCCESS(status), "buildCalendar failed");
    ASSERT_PRINT(calendar.get() != nullptr, "calender is nullptr");

    /**
     * Return the class ID for this class.
     *
     * This is useful only for comparing to a return value from getDynamicClassID(). For example:
     *
     *     Base* polymorphic_pointer = createPolymorphicObject();
     *     if (polymorphic_pointer->getDynamicClassID() ==
     *     Derived::getStaticClassID()) ...
     */
    if (calendar->getDynamicClassID() == icu::GregorianCalendar::getStaticClassID()) {
        auto gregorianCalendar = static_cast<icu::GregorianCalendar *>(calendar.get());
        // ECMAScript start time, value = -(2**53)
        const double beginTime = -9007199254740992;
        gregorianCalendar->setGregorianChange(beginTime, status);
        ASSERT(U_SUCCESS(status));
    }
    return calendar;
}

std::unique_ptr<icu::TimeZone> JSDateTimeFormat::ConstructTimeZone(const std::string &timezone)
{
    if (timezone.empty()) {
        return std::unique_ptr<icu::TimeZone>();
    }
    std::string canonicalized = ConstructFormattedTimeZoneID(timezone);

    std::unique_ptr<icu::TimeZone> tz(icu::TimeZone::createTimeZone(canonicalized.c_str()));
    if (!JSLocale::IsValidTimeZoneName(*tz)) {
        return std::unique_ptr<icu::TimeZone>();
    }
    return tz;
}

std::map<std::string, std::string> JSDateTimeFormat::GetSpecialTimeZoneMap()
{
    std::vector<std::string> specicalTimeZones = {
        "America/Argentina/ComodRivadavia"
        "America/Knox_IN"
        "Antarctica/McMurdo"
        "Australia/ACT"
        "Australia/LHI"
        "Australia/NSW"
        "Antarctica/DumontDUrville"
        "Brazil/DeNoronha"
        "CET"
        "CST6CDT"
        "Chile/EasterIsland"
        "EET"
        "EST"
        "EST5EDT"
        "GB"
        "GB-Eire"
        "HST"
        "MET"
        "MST"
        "MST7MDT"
        "Mexico/BajaNorte"
        "Mexico/BajaSur"
        "NZ"
        "NZ-CHAT"
        "PRC"
        "PST8PDT"
        "ROC"
        "ROK"
        "UCT"
        "W-SU"
        "WET"};
    std::map<std::string, std::string> map;
    for (const auto &item : specicalTimeZones) {
        std::string upper(item);
        transform(upper.begin(), upper.end(), upper.begin(), toupper);
        map.insert({upper, item});
    }
    return map;
}

std::string JSDateTimeFormat::ConstructFormattedTimeZoneID(const std::string &input)
{
    std::string result = input;
    transform(result.begin(), result.end(), result.begin(), toupper);
    static const std::vector<std::string> tzStyleEntry = {
        "GMT", "ETC/UTC", "ETC/UCT", "GMT0", "ETC/GMT", "GMT+0", "GMT-0"
    };
    if (result.find("SYSTEMV/") == 0) {
        result.replace(0, STRING_LENGTH_8, "SystemV/");
    } else if (result.find("US/") == 0) {
        result = (result.length() == STRING_LENGTH_3) ? result : "US/" + ToTitleCaseTimezonePosition(input.substr(3));
    } else if (result.find("ETC/GMT") == 0 && result.length() > STRING_LENGTH_7) {
        result = ConstructGMTTimeZoneID(input);
    } else if (count(tzStyleEntry.begin(), tzStyleEntry.end(), result)) {
        result = "UTC";
    }
    return result;
}

std::string JSDateTimeFormat::ToTitleCaseFunction(const std::string &input)
{
    std::string result(input);
    transform(result.begin(), result.end(), result.begin(), tolower);
    result[0] = toupper(result[0]);
    return result;
}

bool JSDateTimeFormat::IsValidTimeZoneInput(const std::string &input)
{
    std::regex r("[a-zA-Z_-/]*");
    bool isValid = regex_match(input, r);
    return isValid;
}

std::string JSDateTimeFormat::ToTitleCaseTimezonePosition(const std::string &input)
{
    if (!IsValidTimeZoneInput(input)) {
        return std::string();
    }
    std::vector<std::string> titleEntry;
    std::vector<std::string> charEntry;
    int32_t leftPosition = 0;
    int32_t titleLength = 0;
    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == '_' || input[i] == '-' || input[i] == '/') {
            std::string s(1, input[i]);
            charEntry.emplace_back(s);
            titleLength = i - leftPosition;
            titleEntry.emplace_back(input.substr(leftPosition, titleLength));
            leftPosition = i + 1;
        } else {
            continue;
        }
    }
    std::string result;
    for (size_t i = 0; i < titleEntry.size()-1; i++) {
        std::string titleValue = ToTitleCaseFunction(titleEntry[i]);
        if (titleValue == "Of" || titleValue == "Es" || titleValue == "Au") {
            titleValue[0] = tolower(titleValue[0]);
        }
        result = result + titleValue + charEntry[i];
    }
    result = result + ToTitleCaseFunction(titleEntry[titleEntry.size()-1]);
    return result;
}

std::string JSDateTimeFormat::ConstructGMTTimeZoneID(const std::string &input)
{
    if (input.length() < STRING_LENGTH_8 || input.length() > STRING_LENGTH_10) {
        return "";
    }
    std::string ret = "Etc/GMT";
    if (regex_match(input.substr(7), std::regex("[+-][1][0-4]")) || (regex_match(input.substr(7),
        std::regex("[+-][0-9]")) || input.substr(7) == "0")) {
        return ret + input.substr(7);
    }
    return "";
}

std::string JSDateTimeFormat::ToHourCycleString(int32_t hc)
{
    auto mapIter = std::find_if(TO_HOUR_CYCLE_MAP.begin(), TO_HOUR_CYCLE_MAP.end(),
        [hc](const std::map<std::string, HourCycleOption>::value_type item) {
        return static_cast<int32_t>(item.second) == hc;
    });
    if (mapIter != TO_HOUR_CYCLE_MAP.end()) {
        return mapIter->first;
    }
    return "";
}

HourCycleOption JSDateTimeFormat::OptionToHourCycle(const std::string &hc)
{
    auto iter = TO_HOUR_CYCLE_MAP.find(hc);
    if (iter != TO_HOUR_CYCLE_MAP.end()) {
        return iter->second;
    }
    return HourCycleOption::UNDEFINED;
}

HourCycleOption JSDateTimeFormat::OptionToHourCycle(UDateFormatHourCycle hc)
{
    HourCycleOption hcOption = HourCycleOption::UNDEFINED;
    switch (hc) {
        case UDAT_HOUR_CYCLE_11:
            hcOption = HourCycleOption::H11;
            break;
        case UDAT_HOUR_CYCLE_12:
            hcOption = HourCycleOption::H12;
            break;
        case UDAT_HOUR_CYCLE_23:
            hcOption = HourCycleOption::H23;
            break;
        case UDAT_HOUR_CYCLE_24:
            hcOption = HourCycleOption::H24;
            break;
        default:
            UNREACHABLE();
    }
    return hcOption;
}

JSHandle<JSTaggedValue> JSDateTimeFormat::ConvertFieldIdToDateType(JSThread *thread, int32_t fieldId)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    if (fieldId == -1) {
        result.Update(globalConst->GetHandledLiteralString().GetTaggedValue());
    } else if (fieldId == UDAT_YEAR_FIELD || fieldId == UDAT_EXTENDED_YEAR_FIELD) {
        result.Update(globalConst->GetHandledYearString().GetTaggedValue());
    } else if (fieldId == UDAT_YEAR_NAME_FIELD) {
        result.Update(globalConst->GetHandledYearNameString().GetTaggedValue());
    } else if (fieldId == UDAT_MONTH_FIELD || fieldId == UDAT_STANDALONE_MONTH_FIELD) {
        result.Update(globalConst->GetHandledMonthString().GetTaggedValue());
    } else if (fieldId == UDAT_DATE_FIELD) {
        result.Update(globalConst->GetHandledDayString().GetTaggedValue());
    } else if (fieldId == UDAT_HOUR_OF_DAY1_FIELD ||
               fieldId == UDAT_HOUR_OF_DAY0_FIELD || fieldId == UDAT_HOUR1_FIELD || fieldId == UDAT_HOUR0_FIELD) {
        result.Update(globalConst->GetHandledHourString().GetTaggedValue());
    } else if (fieldId == UDAT_MINUTE_FIELD) {
        result.Update(globalConst->GetHandledMinuteString().GetTaggedValue());
    } else if (fieldId == UDAT_SECOND_FIELD) {
        result.Update(globalConst->GetHandledSecondString().GetTaggedValue());
    } else if (fieldId == UDAT_DAY_OF_WEEK_FIELD || fieldId == UDAT_DOW_LOCAL_FIELD ||
               fieldId == UDAT_STANDALONE_DAY_FIELD) {
        result.Update(globalConst->GetHandledWeekdayString().GetTaggedValue());
    } else if (fieldId == UDAT_AM_PM_FIELD || fieldId == UDAT_AM_PM_MIDNIGHT_NOON_FIELD ||
               fieldId == UDAT_FLEXIBLE_DAY_PERIOD_FIELD) {
        result.Update(globalConst->GetHandledDayPeriodString().GetTaggedValue());
    } else if (fieldId == UDAT_TIMEZONE_FIELD || fieldId == UDAT_TIMEZONE_RFC_FIELD ||
               fieldId == UDAT_TIMEZONE_GENERIC_FIELD || fieldId == UDAT_TIMEZONE_SPECIAL_FIELD ||
               fieldId == UDAT_TIMEZONE_LOCALIZED_GMT_OFFSET_FIELD || fieldId == UDAT_TIMEZONE_ISO_FIELD ||
               fieldId == UDAT_TIMEZONE_ISO_LOCAL_FIELD) {
        result.Update(globalConst->GetHandledTimeZoneNameString().GetTaggedValue());
    } else if (fieldId == UDAT_ERA_FIELD) {
        result.Update(globalConst->GetHandledEraString().GetTaggedValue());
    } else if (fieldId == UDAT_FRACTIONAL_SECOND_FIELD) {
        result.Update(globalConst->GetHandledFractionalSecondString().GetTaggedValue());
    } else if (fieldId == UDAT_RELATED_YEAR_FIELD) {
        result.Update(globalConst->GetHandledRelatedYearString().GetTaggedValue());
    } else if (fieldId == UDAT_QUARTER_FIELD || fieldId == UDAT_STANDALONE_QUARTER_FIELD) {
        UNREACHABLE();
    }
    return result;
}

std::unique_ptr<icu::DateIntervalFormat> JSDateTimeFormat::ConstructDateIntervalFormat(
    const JSHandle<JSDateTimeFormat> &dtf)
{
    icu::SimpleDateFormat *icuSimpleDateFormat = dtf->GetIcuSimpleDateFormat();
    icu::Locale locale = *(dtf->GetIcuLocale());
    std::string hcString = ToHourCycleString(dtf->GetHourCycle().GetInt());
    UErrorCode status = U_ZERO_ERROR;
    // Sets the Unicode value for a Unicode keyword.
    if (!hcString.empty()) {
        locale.setUnicodeKeywordValue("hc", hcString, status);
    }
    icu::UnicodeString pattern;
    // Return a pattern string describing this date format.
    pattern = icuSimpleDateFormat->toPattern(pattern);
    // Utility to return a unique skeleton from a given pattern.
    icu::UnicodeString skeleton = icu::DateTimePatternGenerator::staticGetSkeleton(pattern, status);
    // Construct a DateIntervalFormat from skeleton and a given locale.
    std::unique_ptr<icu::DateIntervalFormat> dateIntervalFormat(
        icu::DateIntervalFormat::createInstance(skeleton, locale, status));
    if (U_FAILURE(status)) {
        return nullptr;
    }
    dateIntervalFormat->setTimeZone(icuSimpleDateFormat->getTimeZone());
    return dateIntervalFormat;
}
}  // namespace panda::ecmascript
