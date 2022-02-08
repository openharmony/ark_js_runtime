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

#include "js_relative_time_format.h"

#include "unicode/decimfmt.h"
#include "unicode/numfmt.h"
#include "unicode/unum.h"

namespace panda::ecmascript {
// 14.1.1 InitializeRelativeTimeFormat ( relativeTimeFormat, locales, options )
JSHandle<JSRelativeTimeFormat> JSRelativeTimeFormat::InitializeRelativeTimeFormat(
    JSThread *thread, const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat, const JSHandle<JSTaggedValue> &locales,
    const JSHandle<JSTaggedValue> &options)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1.Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);

    // 2&3. If options is undefined, then Let options be ObjectCreate(null). else Let options be ? ToObject(options).
    JSHandle<JSObject> rtfOptions;
    if (!options->IsUndefined()) {
        rtfOptions = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);
    } else {
        rtfOptions = factory->OrdinaryNewJSObjectCreate(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()));
    }

    // 5. Let matcher be ? GetOption(options, "localeMatcher", "string", «"lookup", "best fit"», "best fit").
    auto globalConst = thread->GlobalConstants();
    LocaleMatcherOption matcher =
        JSLocale::GetOptionOfString(thread, rtfOptions, globalConst->GetHandledLocaleMatcherString(),
                                    {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT},
                                    {"lookup", "best fit"}, LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);

    // 7. Let numberingSystem be ? GetOption(options, "numberingSystem", "string", undefined, undefined).
    JSHandle<JSTaggedValue> property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledNumberingSystemString());
    JSHandle<JSTaggedValue> undefinedValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> numberingSystemValue =
        JSLocale::GetOption(thread, rtfOptions, property, OptionType::STRING, undefinedValue, undefinedValue);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);

    // Check whether numberingSystem is well formed and set to %RelativeTimeFormat%.[[numberingSystem]]
    std::string numberingSystemStdStr;
    if (!numberingSystemValue->IsUndefined()) {
        JSHandle<EcmaString> numberingSystemString = JSHandle<EcmaString>::Cast(numberingSystemValue);
        if (numberingSystemString->IsUtf16()) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid numberingSystem", relativeTimeFormat);
        }
        numberingSystemStdStr = JSLocale::ConvertToStdString(numberingSystemString);
        if (!JSLocale::IsNormativeNumberingSystem(numberingSystemStdStr)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid numberingSystem", relativeTimeFormat);
        }
    } else {
        relativeTimeFormat->SetNumberingSystem(thread, globalConst->GetHandledLatnString());
    }

    // 10. Let localeData be %RelativeTimeFormat%.[[LocaleData]].
    // 11. Let r be ResolveLocale(%RelativeTimeFormat%.[[AvailableLocales]], requestedLocales, opt,
    // %RelativeTimeFormat%.[[RelevantExtensionKeys]], localeData).
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = JSLocale::GetAvailableLocales(thread, "calendar", nullptr);
    }
    std::set<std::string> relevantExtensionKeys{"nu"};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);
    icu::Locale icuLocale = r.localeData;

    // 12. Let locale be r.[[Locale]].
    JSHandle<EcmaString> localeStr = JSLocale::ToLanguageTag(thread, icuLocale);

    // 13. Set relativeTimeFormat.[[Locale]] to locale.
    relativeTimeFormat->SetLocale(thread, localeStr.GetTaggedValue());

    // 15. Set relativeTimeFormat.[[NumberingSystem]] to r.[[nu]].
    UErrorCode status = U_ZERO_ERROR;
    if (!numberingSystemStdStr.empty()) {
        if (JSLocale::IsWellNumberingSystem(numberingSystemStdStr)) {
            icuLocale.setUnicodeKeywordValue("nu", numberingSystemStdStr, status);
            ASSERT(U_SUCCESS(status));
        }
    }

    // 16. Let s be ? GetOption(options, "style", "string", «"long", "short", "narrow"», "long").
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledStyleString());
    RelativeStyleOption styleOption = JSLocale::GetOptionOfString(thread, rtfOptions, property,
        {RelativeStyleOption::LONG, RelativeStyleOption::SHORT, RelativeStyleOption::NARROW},
        {"long", "short", "narrow"}, RelativeStyleOption::LONG);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);

    // 17. Set relativeTimeFormat.[[Style]] to s.
    relativeTimeFormat->SetStyle(styleOption);

    // 18. Let numeric be ? GetOption(options, "numeric", "string", ?"always", "auto"?, "always").
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledNumericString());
    NumericOption numericOption =
        JSLocale::GetOptionOfString(thread, rtfOptions, property, {NumericOption::ALWAYS, NumericOption::AUTO},
                                    {"always", "auto"}, NumericOption::ALWAYS);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSRelativeTimeFormat, thread);

    // 19. Set relativeTimeFormat.[[Numeric]] to numeric.
    relativeTimeFormat->SetNumeric(numericOption);

    // 20. Let relativeTimeFormat.[[NumberFormat]] be ! Construct(%NumberFormat%, « locale »).
    icu::NumberFormat *icuNumberFormat = icu::NumberFormat::createInstance(icuLocale, UNUM_DECIMAL, status);
    if (U_FAILURE(status) != 0) {
        delete icuNumberFormat;
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu Number Format Error", relativeTimeFormat);
    }

    // Trans RelativeStyleOption to ICU Style
    UDateRelativeDateTimeFormatterStyle uStyle;
    switch (styleOption) {
        case RelativeStyleOption::LONG:
            uStyle = UDAT_STYLE_LONG;
            break;
        case RelativeStyleOption::SHORT:
            uStyle = UDAT_STYLE_SHORT;
            break;
        case RelativeStyleOption::NARROW:
            uStyle = UDAT_STYLE_NARROW;
            break;
        default:
            UNREACHABLE();
    }
    icu::RelativeDateTimeFormatter rtfFormatter(icuLocale, icuNumberFormat, uStyle, UDISPCTX_CAPITALIZATION_NONE,
                                                status);
    if (U_FAILURE(status) != 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu Formatter Error", relativeTimeFormat);
    }

    std::string numberingSystem = JSLocale::GetNumberingSystem(icuLocale);
    auto result = factory->NewFromStdString(numberingSystem);
    relativeTimeFormat->SetNumberingSystem(thread, result);

    // Set RelativeTimeFormat.[[IcuRelativeTimeFormatter]]
    factory->NewJSIntlIcuData(relativeTimeFormat, rtfFormatter, JSRelativeTimeFormat::FreeIcuRTFFormatter);

    // 22. Return relativeTimeFormat.
    return relativeTimeFormat;
}

// 14.1.2  SingularRelativeTimeUnit ( unit )
bool SingularUnitToIcuUnit(JSThread *thread, const JSHandle<EcmaString> &unit, URelativeDateTimeUnit *unitEnum)
{
    // 1. Assert: Type(unit) is String.
    ASSERT(JSHandle<JSTaggedValue>::Cast(unit)->IsString());

    // 2. If unit is "seconds" or "second", return "second".
    // 3. If unit is "minutes" or "minute", return "minute".
    // 4. If unit is "hours" or "hour", return "hour".
    // 5. If unit is "days" or "day", return "day".
    // 6. If unit is "weeks" or "week", return "week".
    // 7. If unit is "months" or "month", return "month".
    // 8. If unit is "quarters" or "quarter", return "quarter".
    // 9. If unit is "years" or "year", return "year".
    auto globalConst = thread->GlobalConstants();
    JSHandle<EcmaString> second = JSHandle<EcmaString>::Cast(globalConst->GetHandledSecondString());
    JSHandle<EcmaString> minute = JSHandle<EcmaString>::Cast(globalConst->GetHandledMinuteString());
    JSHandle<EcmaString> hour = JSHandle<EcmaString>::Cast(globalConst->GetHandledHourString());
    JSHandle<EcmaString> day = JSHandle<EcmaString>::Cast(globalConst->GetHandledDayString());
    JSHandle<EcmaString> week = JSHandle<EcmaString>::Cast(globalConst->GetHandledWeekString());
    JSHandle<EcmaString> month = JSHandle<EcmaString>::Cast(globalConst->GetHandledMonthString());
    JSHandle<EcmaString> quarter = JSHandle<EcmaString>::Cast(globalConst->GetHandledQuarterString());
    JSHandle<EcmaString> year = JSHandle<EcmaString>::Cast(globalConst->GetHandledYearString());

    JSHandle<EcmaString> seconds = JSHandle<EcmaString>::Cast(globalConst->GetHandledSecondsString());
    JSHandle<EcmaString> minutes = JSHandle<EcmaString>::Cast(globalConst->GetHandledMinutesString());
    JSHandle<EcmaString> hours = JSHandle<EcmaString>::Cast(globalConst->GetHandledHoursString());
    JSHandle<EcmaString> days = JSHandle<EcmaString>::Cast(globalConst->GetHandledDaysString());
    JSHandle<EcmaString> weeks = JSHandle<EcmaString>::Cast(globalConst->GetHandledWeeksString());
    JSHandle<EcmaString> months = JSHandle<EcmaString>::Cast(globalConst->GetHandledMonthsString());
    JSHandle<EcmaString> quarters = JSHandle<EcmaString>::Cast(globalConst->GetHandledQuartersString());
    JSHandle<EcmaString> years = JSHandle<EcmaString>::Cast(globalConst->GetHandledYearsString());

    if (EcmaString::StringsAreEqual(*second, *unit) || EcmaString::StringsAreEqual(*seconds, *unit)) {
        *unitEnum = UDAT_REL_UNIT_SECOND;
    } else if (EcmaString::StringsAreEqual(*minute, *unit) || EcmaString::StringsAreEqual(*minutes, *unit)) {
        *unitEnum = UDAT_REL_UNIT_MINUTE;
    } else if (EcmaString::StringsAreEqual(*hour, *unit) || EcmaString::StringsAreEqual(*hours, *unit)) {
        *unitEnum = UDAT_REL_UNIT_HOUR;
    } else if (EcmaString::StringsAreEqual(*day, *unit) || EcmaString::StringsAreEqual(*days, *unit)) {
        *unitEnum = UDAT_REL_UNIT_DAY;
    } else if (EcmaString::StringsAreEqual(*week, *unit) || EcmaString::StringsAreEqual(*weeks, *unit)) {
        *unitEnum = UDAT_REL_UNIT_WEEK;
    } else if (EcmaString::StringsAreEqual(*month, *unit) || EcmaString::StringsAreEqual(*months, *unit)) {
        *unitEnum = UDAT_REL_UNIT_MONTH;
    } else if (EcmaString::StringsAreEqual(*quarter, *unit) || EcmaString::StringsAreEqual(*quarters, *unit)) {
        *unitEnum = UDAT_REL_UNIT_QUARTER;
    } else if (EcmaString::StringsAreEqual(*year, *unit) || EcmaString::StringsAreEqual(*years, *unit)) {
        *unitEnum = UDAT_REL_UNIT_YEAR;
    } else {
        return false;
    }
    // 11. else return unit.
    return true;
}

// Unwrap RelativeTimeFormat
JSHandle<JSTaggedValue> JSRelativeTimeFormat::UnwrapRelativeTimeFormat(JSThread *thread,
                                                                       const JSHandle<JSTaggedValue> &rtf)
{
    ASSERT_PRINT(rtf->IsJSObject(), "rtf is not a JSObject");

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    bool isInstanceOf = JSFunction::InstanceOf(thread, rtf, env->GetRelativeTimeFormatFunction());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, rtf);
    if (!rtf->IsJSRelativeTimeFormat() && isInstanceOf) {
        JSHandle<JSTaggedValue> key(thread, JSHandle<JSIntl>::Cast(env->GetIntlFunction())->GetFallbackSymbol());
        OperationResult operationResult = JSTaggedValue::GetProperty(thread, rtf, key);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, rtf);
        return operationResult.GetValue();
    }

    // Perform ? RequireInternalSlot(relativeTimeFormat, [[InitializedRelativeTimeFormat]]).
    if (!rtf->IsJSRelativeTimeFormat()) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, rtf);
    }
    return rtf;
}

// CommonFormat
icu::FormattedRelativeDateTime GetIcuFormatted(JSThread *thread,
                                               const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat,
                                               double value, const JSHandle<EcmaString> &unit)
{
    icu::RelativeDateTimeFormatter *formatter = relativeTimeFormat->GetIcuRTFFormatter();
    ASSERT_PRINT(formatter != nullptr, "rtfFormatter is null");

    // If isFinite(value) is false, then throw a RangeError exception.
    if (!std::isfinite(value)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "", icu::FormattedRelativeDateTime());
    }

    // 10. If unit is not one of "second", "minute", "hour", "day", "week", "month", "quarter", or "year", throw a
    // RangeError exception.
    URelativeDateTimeUnit unitEnum;
    if (!SingularUnitToIcuUnit(thread, unit, &unitEnum)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "", icu::FormattedRelativeDateTime());
    }
    UErrorCode status = U_ZERO_ERROR;
    NumericOption numeric = relativeTimeFormat->GetNumeric();

    icu::FormattedRelativeDateTime formatted;
    switch (numeric) {
        case NumericOption::ALWAYS:
            formatted = formatter->formatNumericToValue(value, unitEnum, status);
            ASSERT_PRINT(U_SUCCESS(status), "icu format to value error");
            break;
        case NumericOption::AUTO:
            formatted = formatter->formatToValue(value, unitEnum, status);
            ASSERT_PRINT(U_SUCCESS(status), "icu format to value error");
            break;
        default:
            UNREACHABLE();
    }
    return formatted;
}

// 14.1.2 SingularRelativeTimeUnit ( unit )
JSHandle<EcmaString> SingularUnitString(JSThread *thread, const JSHandle<EcmaString> &unit)
{
    auto globalConst = thread->GlobalConstants();
    JSHandle<EcmaString> second = JSHandle<EcmaString>::Cast(globalConst->GetHandledSecondString());
    JSHandle<EcmaString> minute = JSHandle<EcmaString>::Cast(globalConst->GetHandledMinuteString());
    JSHandle<EcmaString> hour = JSHandle<EcmaString>::Cast(globalConst->GetHandledHourString());
    JSHandle<EcmaString> day = JSHandle<EcmaString>::Cast(globalConst->GetHandledDayString());
    JSHandle<EcmaString> week = JSHandle<EcmaString>::Cast(globalConst->GetHandledWeekString());
    JSHandle<EcmaString> month = JSHandle<EcmaString>::Cast(globalConst->GetHandledMonthString());
    JSHandle<EcmaString> quarter = JSHandle<EcmaString>::Cast(globalConst->GetHandledQuarterString());
    JSHandle<EcmaString> year = JSHandle<EcmaString>::Cast(globalConst->GetHandledYearString());
    JSHandle<EcmaString> seconds = JSHandle<EcmaString>::Cast(globalConst->GetHandledSecondsString());
    JSHandle<EcmaString> minutes = JSHandle<EcmaString>::Cast(globalConst->GetHandledMinutesString());
    JSHandle<EcmaString> hours = JSHandle<EcmaString>::Cast(globalConst->GetHandledHoursString());
    JSHandle<EcmaString> days = JSHandle<EcmaString>::Cast(globalConst->GetHandledDaysString());
    JSHandle<EcmaString> weeks = JSHandle<EcmaString>::Cast(globalConst->GetHandledWeeksString());
    JSHandle<EcmaString> months = JSHandle<EcmaString>::Cast(globalConst->GetHandledMonthsString());
    JSHandle<EcmaString> quarters = JSHandle<EcmaString>::Cast(globalConst->GetHandledQuartersString());
    JSHandle<EcmaString> years = JSHandle<EcmaString>::Cast(globalConst->GetHandledYearsString());

    // 2. If unit is "seconds" or "second", return "second".
    if (EcmaString::StringsAreEqual(*second, *unit) || EcmaString::StringsAreEqual(*seconds, *unit)) {
        return second;
    }
    // 3. If unit is "minutes" or "minute", return "minute".
    if (EcmaString::StringsAreEqual(*minute, *unit) || EcmaString::StringsAreEqual(*minutes, *unit)) {
        return minute;
    }
    // 4. If unit is "hours" or "hour", return "hour".
    if (EcmaString::StringsAreEqual(*hour, *unit) || EcmaString::StringsAreEqual(*hours, *unit)) {
        return hour;
    }
    // 5. If unit is "days" or "day", return "day".
    if (EcmaString::StringsAreEqual(*day, *unit) || EcmaString::StringsAreEqual(*days, *unit)) {
        return day;
    }
    // 6. If unit is "weeks" or "week", return "week".
    if (EcmaString::StringsAreEqual(*week, *unit) || EcmaString::StringsAreEqual(*weeks, *unit)) {
        return week;
    }
    // 7. If unit is "months" or "month", return "month".
    if (EcmaString::StringsAreEqual(*month, *unit) || EcmaString::StringsAreEqual(*months, *unit)) {
        return month;
    }
    // 8. If unit is "quarters" or "quarter", return "quarter".
    if (EcmaString::StringsAreEqual(*quarter, *unit) || EcmaString::StringsAreEqual(*quarters, *unit)) {
        return quarter;
    }
    // 9. If unit is "years" or "year", return "year".
    if (EcmaString::StringsAreEqual(*year, *unit) || EcmaString::StringsAreEqual(*years, *unit)) {
        return year;
    }

    JSHandle<JSTaggedValue> undefinedValue(thread, JSTaggedValue::Undefined());
    return JSHandle<EcmaString>::Cast(undefinedValue);
}

// 14.1.5 FormatRelativeTime ( relativeTimeFormat, value, unit )
JSHandle<EcmaString> JSRelativeTimeFormat::Format(JSThread *thread, double value, const JSHandle<EcmaString> &unit,
                                                  const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    icu::FormattedRelativeDateTime formatted = GetIcuFormatted(thread, relativeTimeFormat, value, unit);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString uString = formatted.toString(status);
    if (U_FAILURE(status) != 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu formatted toString error", factory->GetEmptyString());
    }
    JSHandle<EcmaString> string =
        factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(uString.getBuffer()), uString.length());
    return string;
}

void FormatToArray(JSThread *thread, const JSHandle<JSArray> &array,
                   const icu::FormattedRelativeDateTime &formatted, double value,
                   const JSHandle<EcmaString> &unit)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString formattedText = formatted.toString(status);
    if (U_FAILURE(status)) {
        THROW_TYPE_ERROR(thread, "formattedRelativeDateTime toString failed");
    }

    icu::ConstrainedFieldPosition cfpo;
    // Set constrainCategory to UFIELD_CATEGORY_NUMBER which is specified for UNumberFormatFields
    cfpo.constrainCategory(UFIELD_CATEGORY_NUMBER);
    int32_t index = 0;
    int32_t previousLimit = 0;
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> taggedValue(thread, JSTaggedValue(value));
    JSMutableHandle<JSTaggedValue> typeString(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> unitString = globalConst->GetHandledUnitString();
    std::vector<std::pair<int32_t, int32_t>> separatorFields;
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
    while ((formatted.nextPosition(cfpo, status) != 0)) {
        int32_t fieldId = cfpo.getField();
        int32_t start = cfpo.getStart();
        int32_t limit = cfpo.getLimit();
        // Special case when fieldId is UNUM_GROUPING_SEPARATOR_FIELD
        if (static_cast<UNumberFormatFields>(fieldId) == UNUM_GROUPING_SEPARATOR_FIELD) {
            separatorFields.push_back(std::pair<int32_t, int32_t>(start, limit));
            continue;
        }
        // If start greater than previousLimit, means a literal type exists before number fields
        // so add a literal type with value of formattedText.sub(0, start)
        if (start > previousLimit) {
            typeString.Update(globalConst->GetLiteralString());
            JSHandle<EcmaString> substring = JSLocale::IcuToString(thread, formattedText, previousLimit, start);
            JSLocale::PutElement(thread, index++, array, typeString, JSHandle<JSTaggedValue>::Cast(substring));
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        // Add part when type is unit
        // Iterate former grouping separator vector and add unit element to array
        for (auto it = separatorFields.begin(); it != separatorFields.end(); it++) {
            if (it->first > start) {
                // Add Integer type element
                JSHandle<EcmaString> resString = JSLocale::IcuToString(thread, formattedText, start, it->first);
                typeString.Update(
                    JSLocale::GetNumberFieldType(thread, taggedValue.GetTaggedValue(), fieldId).GetTaggedValue());
                JSHandle<JSObject> record =
                    JSLocale::PutElement(thread, index++, array, typeString, JSHandle<JSTaggedValue>::Cast(resString));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                JSObject::CreateDataPropertyOrThrow(thread, record, unitString, JSHandle<JSTaggedValue>::Cast(unit));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                // Add Group type element
                resString = JSLocale::IcuToString(thread, formattedText, it->first, it->second);
                typeString.Update(JSLocale::GetNumberFieldType(thread, taggedValue.GetTaggedValue(),
                                                               UNUM_GROUPING_SEPARATOR_FIELD).GetTaggedValue());
                record =
                    JSLocale::PutElement(thread, index++, array, typeString, JSHandle<JSTaggedValue>::Cast(resString));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                JSObject::CreateDataPropertyOrThrow(thread, record, unitString, JSHandle<JSTaggedValue>::Cast(unit));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                start = it->second;
            }
        }
        // Add current field unit
        JSHandle<EcmaString> subString = JSLocale::IcuToString(thread, formattedText, start, limit);
        typeString.Update(JSLocale::GetNumberFieldType(thread, taggedValue.GetTaggedValue(), fieldId).GetTaggedValue());
        JSHandle<JSObject> record =
            JSLocale::PutElement(thread, index++, array, typeString, JSHandle<JSTaggedValue>::Cast(subString));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        JSObject::CreateDataPropertyOrThrow(thread, record, unitString, JSHandle<JSTaggedValue>::Cast(unit));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        previousLimit = limit;
    }
    // If iterated length is smaller than formattedText.length, means a literal type exists after number fields
    // so add a literal type with value of formattedText.sub(previousLimit, formattedText.length)
    if (formattedText.length() > previousLimit) {
        typeString.Update(globalConst->GetLiteralString());
        JSHandle<EcmaString> substring =
            JSLocale::IcuToString(thread, formattedText, previousLimit, formattedText.length());
        JSLocale::PutElement(thread, index, array, typeString, JSHandle<JSTaggedValue>::Cast(substring));
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
}

// 14.1.6 FormatRelativeTimeToParts ( relativeTimeFormat, value, unit )
JSHandle<JSArray> JSRelativeTimeFormat::FormatToParts(JSThread *thread, double value, const JSHandle<EcmaString> &unit,
                                                      const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat)
{
    icu::FormattedRelativeDateTime formatted = GetIcuFormatted(thread, relativeTimeFormat, value, unit);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    JSHandle<EcmaString> singularUnit = SingularUnitString(thread, unit);
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    FormatToArray(thread, array, formatted, value, singularUnit);
    return array;
}

void JSRelativeTimeFormat::ResolvedOptions(JSThread *thread, const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat,
                                           const JSHandle<JSObject> &options)
{
    if (relativeTimeFormat->GetIcuRTFFormatter() != nullptr) {
        [[maybe_unused]] icu::RelativeDateTimeFormatter *formatter = relativeTimeFormat->GetIcuRTFFormatter();
    } else {
        THROW_RANGE_ERROR(thread, "rtf is not initialized");
    }

    auto globalConst = thread->GlobalConstants();
    // [[locale]]
    JSHandle<JSTaggedValue> property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledLocaleString());
    JSHandle<EcmaString> locale(thread, relativeTimeFormat->GetLocale());
    PropertyDescriptor localeDesc(thread, JSHandle<JSTaggedValue>::Cast(locale), true, true, true);
    JSObject::DefineOwnProperty(thread, options, property, localeDesc);

    // [[Style]]
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledStyleString());
    RelativeStyleOption style = relativeTimeFormat->GetStyle();
    JSHandle<JSTaggedValue> styleValue;
    if (style == RelativeStyleOption::LONG) {
        styleValue = globalConst->GetHandledLongString();
    } else if (style == RelativeStyleOption::SHORT) {
        styleValue = globalConst->GetHandledShortString();
    } else if (style == RelativeStyleOption::NARROW) {
        styleValue = globalConst->GetHandledNarrowString();
    }
    PropertyDescriptor styleDesc(thread, styleValue, true, true, true);
    JSObject::DefineOwnProperty(thread, options, property, styleDesc);

    // [[Numeric]]
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledNumericString());
    NumericOption numeric = relativeTimeFormat->GetNumeric();
    JSHandle<JSTaggedValue> numericValue;
    if (numeric == NumericOption::ALWAYS) {
        numericValue = globalConst->GetHandledAlwaysString();
    } else if (numeric == NumericOption::AUTO) {
        numericValue = globalConst->GetHandledAutoString();
    } else {
        THROW_RANGE_ERROR(thread, "numeric is exception");
    }
    PropertyDescriptor numericDesc(thread, numericValue, true, true, true);
    JSObject::DefineOwnProperty(thread, options, property, numericDesc);

    // [[NumberingSystem]]
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledNumberingSystemString());
    JSHandle<JSTaggedValue> numberingSystem(thread, relativeTimeFormat->GetNumberingSystem());
    PropertyDescriptor numberingSystemDesc(thread, numberingSystem, true, true, true);
    JSObject::DefineOwnProperty(thread, options, property, numberingSystemDesc);
}
}  // namespace panda::ecmascript