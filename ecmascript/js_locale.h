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

#ifndef ECMASCRIPT_JSLOCALE_H
#define ECMASCRIPT_JSLOCALE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_object.h"
#include "ecmascript/mem/c_containers.h"
#include "ohos/init_data.h"
#include "unicode/basictz.h"
#include "unicode/brkiter.h"
#include "unicode/calendar.h"
#include "unicode/coll.h"
#include "unicode/datefmt.h"
#include "unicode/decimfmt.h"
#include "unicode/dtitvfmt.h"
#include "unicode/dtptngen.h"
#include "unicode/fieldpos.h"
#include "unicode/formattedvalue.h"
#include "unicode/gregocal.h"
#include "unicode/locid.h"
#include "unicode/normalizer2.h"
#include "unicode/numberformatter.h"
#include "unicode/numfmt.h"
#include "unicode/numsys.h"
#include "unicode/smpdtfmt.h"
#include "unicode/timezone.h"
#include "unicode/udat.h"
#include "unicode/unistr.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "unicode/uvernum.h"
#include "unicode/uversion.h"

namespace panda::ecmascript {
enum class OptionType : uint8_t { STRING = 0x01, BOOLEAN };
enum class LocaleMatcherOption : uint8_t { LOOKUP = 0x01, BEST_FIT, EXCEPTION };
enum class FormatMatcherOption : uint8_t { BASIC = 0x01, BEST_FIT, EXCEPTION };
enum class LocaleType : uint8_t {
    LITERAL = 0x01,
    NUMBER,
    PLUS_SIGN,
    MINUS_SIGN,
    PERCENT_SIGN,
    UNIT_PREFIX,
    UNIT_SUFFIX,
    CURRENCY_CODE,
    CURRENCY_PREFIX,
    CURRENCY_SUFFIX,
};
enum class RoundingType : uint8_t { FRACTIONDIGITS = 0x01, SIGNIFICANTDIGITS, COMPACTROUNDING, EXCEPTION };
enum class NotationOption : uint8_t { STANDARD = 0x01, SCIENTIFIC, ENGINEERING, COMPACT, EXCEPTION };

constexpr uint32_t MAX_DIGITS = 21;
constexpr uint32_t MAX_FRACTION_DIGITS = 20;
constexpr uint8_t INTL_INDEX_ZERO = 0;
constexpr uint8_t INTL_INDEX_ONE = 1;
constexpr uint8_t INTL_INDEX_TWO = 2;
constexpr uint8_t INTL_INDEX_THREE = 3;
constexpr uint8_t INTL_INDEX_FOUR = 4;
constexpr uint8_t INTL_INDEX_FIVE = 5;
constexpr uint8_t INTL_INDEX_EIGHT = 8;

class JSIntlIterator : public icu::Locale::Iterator {
public:
    JSIntlIterator(const JSHandle<TaggedArray> &data, array_size_t length) : length_(length), curIdx_(0)
    {
        for (array_size_t idx = 0; idx < length; idx++) {
            std::string str = base::StringHelper::ToStdString(EcmaString::Cast(data->Get(idx).GetTaggedObject()));
            data_.emplace_back(str);
        }
    }

    ~JSIntlIterator() override = default;
    DEFAULT_COPY_SEMANTIC(JSIntlIterator);
    DEFAULT_MOVE_SEMANTIC(JSIntlIterator);

    UBool hasNext() const override
    {
        return static_cast<UBool>(curIdx_ < length_);
    }

    const icu::Locale &next() override
    {
        ASSERT(curIdx_ < length_);
        UErrorCode status = U_ZERO_ERROR;
        locale_ = icu::Locale::forLanguageTag(data_[curIdx_].c_str(), status);
        ASSERT(U_SUCCESS(status));
        curIdx_++;
        return locale_;
    }

    inline const std::string &operator[](size_t index) const noexcept
    {
        ASSERT(index < length_);
        return data_[index];
    }

private:
    std::vector<std::string> data_{};
    array_size_t length_{0};
    array_size_t curIdx_{0};
    icu::Locale locale_{};
};

struct ResolvedLocale {
    std::string locale;
    icu::Locale localeData;
    std::map<std::string, std::string> extensions;
};

struct MatcherResult {
    std::string locale;
    std::string extension;
};

struct OptionData {
    std::string name;
    std::string key;
    std::vector<std::string> possibleValues;
    bool isBoolValue = false;
};

struct TagElements {
    JSHandle<JSTaggedValue> language;
    JSHandle<JSTaggedValue> script;
    JSHandle<JSTaggedValue> region;
};

class JSLocale : public JSObject {
public:
    struct ParsedLocale {
        std::string base;
        std::string extension;
    };

    static JSLocale *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSLocale());
        return static_cast<JSLocale *>(object);
    }

    static constexpr size_t ICU_FIELD_OFFSET = JSObject::SIZE;
    // icu::Locale internal slot.
    ACCESSORS(IcuField, ICU_FIELD_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ICU_FIELD_OFFSET, SIZE)
    DECL_DUMP()

    icu::Locale *GetIcuLocale() const
    {
        ASSERT(GetIcuField().IsJSNativePointer());
        auto result = JSNativePointer::Cast(GetIcuField().GetTaggedObject())->GetExternalPointer();
        return reinterpret_cast<icu::Locale *>(result);
    }

    static void FreeIcuLocale(void *pointer, void *data)
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

    static std::string ConvertToStdString(const JSHandle<EcmaString> &ecmaStr);

    // 6.2.2 IsStructurallyValidLanguageTag ( locale )
    static bool IsStructurallyValidLanguageTag(const JSHandle<EcmaString> &tag);

    static bool DealwithLanguageTag(const std::vector<std::string> &containers, size_t &address);

    // 6.2.3 CanonicalizeUnicodeLocaleId ( locale )
    static JSHandle<EcmaString> CanonicalizeUnicodeLocaleId(JSThread *thread, const JSHandle<EcmaString> &locale);

    // 6.2.4 DefaultLocale ()
    static JSHandle<EcmaString> DefaultLocale(JSThread *thread);

    // 6.4.1 IsValidTimeZoneName ( timeZone )
    static bool IsValidTimeZoneName(const icu::TimeZone &tz);

    // 9.2.1 CanonicalizeLocaleList ( locales )
    static JSHandle<TaggedArray> CanonicalizeLocaleList(JSThread *thread, const JSHandle<JSTaggedValue> &locales);

    template<typename T>
    static JSHandle<TaggedArray> CanonicalizeHelper(JSThread *thread, const JSHandle<JSTaggedValue> &locales,
                                                       JSHandle<T> &obj, JSHandle<TaggedArray> &seen);

    // 9.2.2 BestAvailableLocale ( availableLocales, locale )
    static std::string BestAvailableLocale(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                           const std::string &locale);

    // 9.2.3 LookupMatcher ( availableLocales, requestedLocales )
    static JSHandle<EcmaString> LookupMatcher(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                              const JSHandle<TaggedArray> &requestedLocales);

    // 9.2.4 BestFitMatcher ( availableLocales, requestedLocales )
    static JSHandle<EcmaString> BestFitMatcher(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                               const JSHandle<TaggedArray> &requestedLocales);

    // 9.2.5 UnicodeExtensionValue ( extension, key )
    static std::string UnicodeExtensionValue(const std::string extension, const std::string key);

    // 9.2.7 ResolveLocale ( availableLocales, requestedLocales, options, relevantExtensionKeys, localeData )
    static ResolvedLocale ResolveLocale(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                        const JSHandle<TaggedArray> &requestedLocales, LocaleMatcherOption matcher,
                                        const std::set<std::string> &relevantExtensionKeys);

    // 9.2.8 LookupSupportedLocales ( availableLocales, requestedLocales )
    static JSHandle<TaggedArray> LookupSupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                                        const JSHandle<TaggedArray> &requestedLocales);

    // 9.2.9 BestFitSupportedLocales ( availableLocales, requestedLocales )
    static JSHandle<TaggedArray> BestFitSupportedLocales(JSThread *thread,
                                                         const JSHandle<TaggedArray> &availableLocales,
                                                         const JSHandle<TaggedArray> &requestedLocales);

    // 9.2.10 SupportedLocales ( availableLocales, requestedLocales, options )
    static JSHandle<JSArray> SupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                              const JSHandle<TaggedArray> &requestedLocales,
                                              const JSHandle<JSTaggedValue> &options);

    // 9.2.11 GetOption ( options, property, type, values, fallback )
    template<typename T>
    static T GetOptionOfString(JSThread *thread, const JSHandle<JSObject> &options,
                             const JSHandle<JSTaggedValue> &property, const std::vector<T> &enumValues,
                             const std::vector<std::string> &strValues, T fallback)
    {
        // 1. Let value be ? Get(options, property).
        OperationResult operationResult = JSObject::GetProperty(thread, options, property);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, T::EXCEPTION);
        JSHandle<JSTaggedValue> value = operationResult.GetValue();

        if (value->IsUndefined()) {
            return fallback;
        }

        // 2. If value is not undefined, then
        // d. If values is not undefined, then
        //   i. If values does not contain an element equal to value, throw a RangeError exception.
        JSHandle<EcmaString> valueEStr = JSTaggedValue::ToString(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, T::EXCEPTION);
        std::string valueStr = ConvertToStdString(valueEStr);
        int existIdx = -1;
        if (!enumValues.empty()) {
            int strValuesSize = strValues.size();
            for (int i = 0; i < strValuesSize; i++) {
                if (strValues[i] == valueStr) {
                    existIdx = i;
                }
            }
            if (existIdx == -1) {
                THROW_RANGE_ERROR_AND_RETURN(thread, "getStringOption failed", T::EXCEPTION);
            }
        }
        if (existIdx == -1) {
            UNREACHABLE();
        }
        // e.Return value.
        return enumValues[existIdx];
    }

    static bool GetOptionOfBool(JSThread *thread, const JSHandle<JSObject> &options,
                              const JSHandle<JSTaggedValue> &property, bool fallback, bool *res);

    static JSHandle<JSTaggedValue> GetOption(JSThread *thread, const JSHandle<JSObject> &options,
                                             const JSHandle<JSTaggedValue> &property, OptionType type,
                                             const JSHandle<JSTaggedValue> &values,
                                             const JSHandle<JSTaggedValue> &fallback);

    static bool GetOptionOfString(JSThread *thread, const JSHandle<JSObject> &options,
                                const JSHandle<JSTaggedValue> &property, const std::vector<std::string> &values,
                                std::string *optionValue);

    // 9.2.12 DefaultNumberOption ( value, minimum, maximum, fallback )
    static int DefaultNumberOption(JSThread *thread, const JSHandle<JSTaggedValue> &value, int minimum, int maximum,
                                   int fallback);

    // 9.2.13 GetNumberOption ( options, property, minimum, maximum, fallback )
    static int GetNumberOption(JSThread *thread, const JSHandle<JSObject> &options,
                               const JSHandle<JSTaggedValue> &property, int minimum, int maximum, int fallback);

    static bool IsLanguageSubtag(const std::string &value)
    {
        return IsAlpha(value, INTL_INDEX_TWO, INTL_INDEX_THREE) || IsAlpha(value, INTL_INDEX_FIVE, INTL_INDEX_EIGHT);
    }

    static bool IsScriptSubtag(const std::string &value)
    {
        return IsAlpha(value, INTL_INDEX_FOUR, INTL_INDEX_FOUR);
    }

    static bool IsRegionSubtag(const std::string &value)
    {
        return IsAlpha(value, INTL_INDEX_TWO, INTL_INDEX_TWO) || IsDigit(value, INTL_INDEX_THREE, INTL_INDEX_THREE);
    }

    static bool IsVariantSubtag(const std::string &value)
    {
        return IsThirdDigitAlphanum(value) || IsAlphanum(value, INTL_INDEX_FIVE, INTL_INDEX_EIGHT);
    }

    static bool IsThirdDigitAlphanum(const std::string &value)
    {
        return InRange(value[0], '0', '9') && value.length() == 4 &&
               IsAlphanum(value.substr(INTL_INDEX_ONE), INTL_INDEX_THREE, INTL_INDEX_THREE);
    }

    static bool IsExtensionSingleton(const std::string &value)
    {
        return IsAlphanum(value, INTL_INDEX_ONE, INTL_INDEX_ONE);
    }

    static bool IsNormativeCalendar(const std::string &value)
    {
        return IsWellAlphaNumList(value);
    }

    static bool IsNormativeNumberingSystem(const std::string &value)
    {
        return IsWellAlphaNumList(value);
    }

    static bool IsWellNumberingSystem(const std::string &value)
    {
        std::set<std::string> irregularList = {"native", "traditio", "finance"};
        if (irregularList.find(value) != irregularList.end()) {
            return false;
        }
        UErrorCode status = U_ZERO_ERROR;
        icu::NumberingSystem *numberingSystem = icu::NumberingSystem::createInstanceByName(value.c_str(), status);
        bool result = U_SUCCESS(status) != 0 && numberingSystem != nullptr;
        delete numberingSystem;
        numberingSystem = NULL;
        return result;
    }

    static bool IsWellCollation(const icu::Locale &locale, const std::string &value)
    {
        std::set<std::string>  irregularList = {"standard", "search"};
        if (irregularList.find(value) !=  irregularList.end()) {
            return false;
        }
        return IsWellExtension<icu::Collator>(locale, "collation", value);
    }

    static bool IsWellCalendar(const icu::Locale &locale, const std::string &value)
    {
        return IsWellExtension<icu::Calendar>(locale, "calendar", value);
    }

    template<typename T>
    static bool IsWellExtension(const icu::Locale &locale, const char *key, const std::string &value)
    {
        UErrorCode status = U_ZERO_ERROR;
        const char *outdatedType = uloc_toLegacyType(key, value.c_str());
        if (outdatedType == nullptr) {
            return false;
        }
        icu::StringEnumeration *sequence = T::getKeywordValuesForLocale(key, icu::Locale(locale.getBaseName()),
                                                                        false, status);
        if (U_FAILURE(status)) {
            delete sequence;
            sequence = NULL;
            return false;
        }
        int32_t size;
        const char *element = sequence->next(&size, status);
        while (U_SUCCESS(status) && element != nullptr) {
            if (strcmp(outdatedType, element) == 0) {
                delete sequence;
                sequence = NULL;
                return true;
            }
            element = sequence->next(&size, status);
        }
        delete sequence;
        sequence = NULL;
        return false;
    }

    static inline constexpr int AsciiAlphaToLower(uint32_t c)
    {
        constexpr uint32_t FLAG = 0x20;
        return static_cast<int>(c | FLAG);
    }

    static bool IsAsciiAlpha(char ch)
    {
        return InRange(ch, 'A', 'Z') || InRange(ch, 'a', 'z');
    }

    static char LocaleIndependentAsciiToUpper(char ch)
    {
        return (InRange(ch, 'a', 'z')) ? static_cast<char>((ch - 'a' + 'A')) : ch;
    }

    static char LocaleIndependentAsciiToLower(char ch)
    {
        return (InRange(ch, 'A', 'Z')) ? static_cast<char>((ch - 'A' + 'a')) : ch;
    }

    template<typename T, typename U>
    static bool InRange(T value, U start, U end)
    {
        ASSERT(start <= end);
        ASSERT(sizeof(T) >= sizeof(U));
        return (value >= static_cast<T>(start)) && (value <= static_cast<T>(end));
    }

    static bool IsWellAlphaNumList(const std::string &value)
    {
        if (value.length() < 3) {
            return false;
        }
        char lastChar  = value[value.length() - 1];
        if (lastChar == '-') {
            return false;
        }
        std::vector<std::string> items;
        std::istringstream input(value);
        std::string temp;
        while (getline(input, temp, '-')) {
            items.push_back(temp);
        }
        for (auto &item : items) {
            if (!IsAlphanum(item, INTL_INDEX_THREE, INTL_INDEX_EIGHT)) {
                return false;
            }
        }
        return true;
    }

    static bool ValidateOtherTags(const icu::Locale &locale, const char *packageName, const char *key, bool &res)
    {
        const char *localeCountry = locale.getCountry();
        const char *localeScript = locale.getScript();
        if (localeCountry[0] != '\0' && localeScript[0] != '\0') {
            std::string removeCountry;
            removeCountry = locale.getLanguage();
            removeCountry.append("-");
            removeCountry.append(localeScript);
            return CheckLocales(removeCountry.c_str(), key, packageName, res);
        }
        if (localeCountry[0] != '\0' || localeScript[0] != '\0') {
            std::string language = locale.getLanguage();
            return CheckLocales(language.c_str(), key, packageName, res);
        }
        return res;
    }

    static bool CheckLocales(const icu::Locale &locale, const char *key, const char *packageName, bool &res)
    {
        res = false;
        UErrorCode status = U_ZERO_ERROR;
        const char *formalLocale = locale.getName();
        UResourceBundle *localeRes = ures_open(packageName, formalLocale, &status);
        if (localeRes != nullptr && status == U_ZERO_ERROR) {
            bool flag = (key == nullptr) ? true : false;
            if (flag) {
                res = true;
            } else {
                UResourceBundle *keyRes = ures_getByKey(localeRes, key, nullptr, &status);
                if (keyRes != nullptr && status == U_ZERO_ERROR) {
                    res = true;
                }
                ures_close(keyRes);
            }
        }
        ures_close(localeRes);
        if (res) {
            return res;
        } else {
            ValidateOtherTags(locale, packageName, key, res);
        }
        return res;
    }

    static JSHandle<EcmaString> IcuToString(JSThread *thread, const icu::UnicodeString &string);

    static JSHandle<EcmaString> IcuToString(JSThread *thread, const icu::UnicodeString &string, int32_t begin,
                                            int32_t end);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread, const char *key, const char *path);

    static JSHandle<JSObject> PutElement(JSThread *thread, int index, const JSHandle<JSArray> &array,
                                         const JSHandle<JSTaggedValue> &fieldTypeString,
                                         const JSHandle<JSTaggedValue> &value);

    static JSHandle<EcmaString> ToLanguageTag(JSThread *thread, const icu::Locale &locale);

    static std::string GetNumberingSystem(const icu::Locale &icuLocale);

    static bool IsWellFormedCurrencyCode(const std::string &currency);

    static JSHandle<JSTaggedValue> GetNumberFieldType(JSThread *thread, JSTaggedValue x, int32_t fieldId);

    static bool ApplyOptionsToTag(JSThread *thread, const JSHandle<EcmaString> &tag, const JSHandle<JSObject> &options,
                                  TagElements &tagElements);

    static JSHandle<JSLocale> InitializeLocale(JSThread *thread, const JSHandle<JSLocale> &locale,
                                               const JSHandle<EcmaString> &localeString,
                                               const JSHandle<JSObject> &options);

    static JSHandle<EcmaString> NormalizeKeywordValue(JSThread *thread, const JSHandle<JSLocale> &locale,
                                                    const std::string &key);

    static void HandleLocaleExtension(size_t &start, size_t &extensionEnd, std::string result, size_t len);

    static ParsedLocale HandleLocale(const JSHandle<EcmaString> &locale);

    static JSHandle<EcmaString> ToString(JSThread *thread, const JSHandle<JSLocale> &locale);

    // 12.1.1 SetNumberFormatDigitOptions ( intlObj, options, mnfdDefault, mxfdDefault, notation )
    template<typename T>
    static void SetNumberFormatDigitOptions(JSThread *thread, const JSHandle<T> &intlObj,
                                            const JSHandle<JSTaggedValue> &options, int mnfdDefault, int mxfdDefault,
                                            NotationOption notation)
    {
        // 1. Assert: Type(intlObj) is Object.
        // 2. Assert: Type(options) is Object.
        // 3. Assert: Type(mnfdDefault) is Number.
        // 4. Assert: Type(mxfdDefault) is Number.
        ASSERT(options->IsHeapObject());
        auto globalConst = thread->GlobalConstants();
        // Set intlObj.[[MinimumFractionDigits]] to 0.
        intlObj->SetMinimumFractionDigits(thread, JSTaggedValue(0));
        // Set intlObj.[[MaximumFractionDigits]] to 0.
        intlObj->SetMaximumFractionDigits(thread, JSTaggedValue(0));
        // Set intlObj.[[MinimumSignificantDigits]] to 0.
        intlObj->SetMinimumSignificantDigits(thread, JSTaggedValue(0));
        // Set intlObj.[[MaximumSignificantDigits]] to 0.
        intlObj->SetMaximumSignificantDigits(thread, JSTaggedValue(0));

        // 5. Let mnid be ? GetNumberOption(options, "minimumIntegerDigits,", 1, 21, 1).
        JSHandle<JSTaggedValue> mnidKey = globalConst->GetHandledMinimumIntegerDigitsString();
        int mnid = GetNumberOption(thread, JSHandle<JSObject>::Cast(options), mnidKey, 1, MAX_DIGITS, 1);
        // 6. Let mnfd be ? Get(options, "minimumFractionDigits").
        JSHandle<JSTaggedValue> mnfdKey = globalConst->GetHandledMinimumFractionDigitsString();
        JSHandle<JSTaggedValue> mnfd = JSTaggedValue::GetProperty(thread, options, mnfdKey).GetValue();
        // 7. Let mxfd be ? Get(options, "maximumFractionDigits").
        JSHandle<JSTaggedValue> mxfdKey = globalConst->GetHandledMaximumFractionDigitsString();
        JSHandle<JSTaggedValue> mxfd = JSTaggedValue::GetProperty(thread, options, mxfdKey).GetValue();
        // 8. Let mnsd be ? Get(options, "minimumSignificantDigits").
        JSHandle<JSTaggedValue> mnsdKey = globalConst->GetHandledMinimumSignificantDigitsString();
        JSHandle<JSTaggedValue> mnsd = JSTaggedValue::GetProperty(thread, options, mnsdKey).GetValue();
        // 9. Let mxsd be ? Get(options, "maximumSignificantDigits").
        JSHandle<JSTaggedValue> mxsdKey = globalConst->GetHandledMaximumSignificantDigitsString();
        JSHandle<JSTaggedValue> mxsd = JSTaggedValue::GetProperty(thread, options, mxsdKey).GetValue();

        // 10. Set intlObj.[[MinimumIntegerDigits]] to mnid.
        intlObj->SetMinimumIntegerDigits(thread, JSTaggedValue(mnid));
        // 11. If mnsd is not undefined or mxsd is not undefined, then
        if (!mnsd->IsUndefined() || !mxsd->IsUndefined()) {
            // a. Set intlObj.[[RoundingType]] to significantDigits.
            intlObj->SetRoundingType(thread, JSTaggedValue(static_cast<uint32_t>(RoundingType::SIGNIFICANTDIGITS)));
            // b. Let mnsd be ? DefaultNumberOption(mnsd, 1, 21, 1).
            mnsd = JSHandle<JSTaggedValue>(
                thread, JSTaggedValue(JSLocale::DefaultNumberOption(thread, mnsd, 1, MAX_DIGITS, 1)));
            // c. Let mxsd be ? DefaultNumberOption(mxsd, mnsd, 21, 21).
            mxsd = JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(JSLocale::DefaultNumberOption(thread, mxsd, mnsd->GetInt(), MAX_DIGITS, MAX_DIGITS)));
            // d. Set intlObj.[[MinimumSignificantDigits]] to mnsd.
            intlObj->SetMinimumSignificantDigits(thread, mnsd);
            // e. Set intlObj.[[MaximumSignificantDigits]] to mxsd.
            intlObj->SetMaximumSignificantDigits(thread, mxsd);
        } else {
            if (!mnfd->IsUndefined() || !mxfd->IsUndefined()) {
                // 12. Else if mnfd is not undefined or mxfd is not undefined, then
                // a. Set intlObj.[[RoundingType]] to fractionDigits.
                intlObj->SetRoundingType(thread, JSTaggedValue(static_cast<uint32_t>(RoundingType::FRACTIONDIGITS)));
                if (!mxfd->IsUndefined()) {
                    JSTaggedValue mxfdValue =
                        JSTaggedValue(JSLocale::DefaultNumberOption(thread, mxfd, 0, MAX_FRACTION_DIGITS, mxfdDefault));
                    mxfd = JSHandle<JSTaggedValue>(thread, mxfdValue);
                    mnfdDefault = std::min(mnfdDefault, mxfd->GetInt());
                }
                // b. Let mnfd be ? DefaultNumberOption(mnfd, 0, 20, mnfdDefault).
                mnfd = JSHandle<JSTaggedValue>(
                    thread, JSTaggedValue(DefaultNumberOption(thread, mnfd, 0, MAX_FRACTION_DIGITS, mnfdDefault)));
                // c. Let mxfdActualDefault be max( mnfd, mxfdDefault ).
                int mxfdActualDefault = std::max(mnfd->GetInt(), mxfdDefault);
                // d. Let mxfd be ? DefaultNumberOption(mxfd, mnfd, 20, mxfdActualDefault).
                mxfd = JSHandle<JSTaggedValue>(
                    thread, JSTaggedValue(JSLocale::DefaultNumberOption(thread, mxfd, mnfd->GetInt(),
                                                                        MAX_FRACTION_DIGITS, mxfdActualDefault)));
                // e. Set intlObj.[[MinimumFractionDigits]] to mnfd.
                intlObj->SetMinimumFractionDigits(thread, mnfd);
                // f. Set intlObj.[[MaximumFractionDigits]] to mxfd.
                intlObj->SetMaximumFractionDigits(thread, mxfd);
            } else if (notation == NotationOption::COMPACT) {
                // 13. Else if notation is "compact", then
                // a. Set intlObj.[[RoundingType]] to compactRounding.
                intlObj->SetRoundingType(thread, JSTaggedValue(static_cast<int>(RoundingType::COMPACTROUNDING)));
            } else {
                // 14. else,
                // a.Set intlObj.[[RoundingType]] to fractionDigits.
                intlObj->SetRoundingType(thread, JSTaggedValue(static_cast<int>(RoundingType::FRACTIONDIGITS)));
                // b.Set intlObj.[[MinimumFractionDigits]] to mnfdDefault.
                intlObj->SetMinimumFractionDigits(thread, JSTaggedValue(mnfdDefault));
                // c.Set intlObj.[[MaximumFractionDigits]] to mxfdDefault.
                intlObj->SetMaximumFractionDigits(thread, JSTaggedValue(mxfdDefault));
            }
        }
    }

    static JSHandle<TaggedArray> ConstructLocaleList(JSThread *thread,
                                                     const std::vector<std::string> &icuAvailableLocales);

    static bool CheckLocales(const icu::Locale &locale, const char *path, const char *key);

    static bool IsPrivateSubTag(std::string result, size_t len)
    {
        if ((len > INTL_INDEX_ONE) && (result[INTL_INDEX_ONE] == '-')) {
            ASSERT(result[INTL_INDEX_ZERO] == 'x' || result[INTL_INDEX_ZERO] == 'i');
            return true;
        }
        return false;
    }

private:
    static icu::Locale BuildICULocale(const std::string &bcp47Locale);

    static bool IsCheckRange(const std::string &str, size_t min, size_t max, bool(rangeCheckFunc)(char))
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char i : str) {
            if (!rangeCheckFunc(i)) {
                return false;
            }
        }
        return true;
    }

    static bool IsAlpha(const std::string &str, size_t min, size_t max)
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char c : str) {
            if (!IsAsciiAlpha(c)) {
                return false;
            }
        }
        return true;
    }

    static bool IsDigit(const std::string &str, size_t min, size_t max)
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char i : str) {
            if (!InRange(i, '0', '9')) {
                return false;
            }
        }
        return true;
    }

    static bool IsAlphanum(const std::string &str, size_t min, size_t max)
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char i : str) {
            if (!IsAsciiAlpha(i) && !InRange(i, '0', '9')) {
                return false;
            }
        }
        return true;
    }

    static bool IsAToZ(char ch)
    {
        int lowerCh = JSLocale::AsciiAlphaToLower(ch);
        return JSLocale::InRange(lowerCh, 'a', 'z');
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSLOCALE_H
