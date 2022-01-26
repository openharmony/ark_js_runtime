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

#include "ecmascript/js_plural_rules.h"

#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_number_format.h"

namespace panda::ecmascript {
constexpr int32_t STRING_SEPARATOR_LENGTH = 4;

icu::number::LocalizedNumberFormatter *JSPluralRules::GetIcuNumberFormatter() const
{
    ASSERT(GetIcuNF().IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetIcuNF().GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::number::LocalizedNumberFormatter *>(result);
}

void JSPluralRules::FreeIcuNumberFormatter(void *pointer, [[maybe_unused]] void* hint)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuNumberFormatter = reinterpret_cast<icu::number::LocalizedNumberFormatter *>(pointer);
    icuNumberFormatter->~LocalizedNumberFormatter();
}

void JSPluralRules::SetIcuNumberFormatter(JSThread *thread, const JSHandle<JSPluralRules> &pluralRules,
    const icu::number::LocalizedNumberFormatter &icuNF, const DeleteEntryPoint &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    icu::number::LocalizedNumberFormatter *icuPointer =
        ecmaVm->GetRegionFactory()->New<icu::number::LocalizedNumberFormatter>(icuNF);
    ASSERT(icuPointer != nullptr);
    JSTaggedValue data = pluralRules->GetIcuNF();
    if (data.IsHeapObject() && data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(icuPointer);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuPointer);
    pointer->SetDeleter(callback);
    pluralRules->SetIcuNF(thread, pointer.GetTaggedValue());
    ecmaVm->PushToArrayDataList(*pointer);
}

icu::PluralRules *JSPluralRules::GetIcuPluralRules() const
{
    ASSERT(GetIcuPR().IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetIcuPR().GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::PluralRules *>(result);
}

void JSPluralRules::FreeIcuPluralRules(void *pointer, [[maybe_unused]] void* hint)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuPluralRules = reinterpret_cast<icu::PluralRules *>(pointer);
    icuPluralRules->~PluralRules();
}

void JSPluralRules::SetIcuPluralRules(JSThread *thread, const JSHandle<JSPluralRules> &pluralRules,
    const icu::PluralRules &icuPR, const DeleteEntryPoint &callback)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    icu::PluralRules *icuPointer = ecmaVm->GetRegionFactory()->New<icu::PluralRules>(icuPR);
    ASSERT(icuPointer != nullptr);
    JSTaggedValue data = pluralRules->GetIcuPR();
    if (data.IsHeapObject() && data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(icuPointer);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuPointer);
    pointer->SetDeleter(callback);
    pluralRules->SetIcuPR(thread, pointer.GetTaggedValue());
    ecmaVm->PushToArrayDataList(*pointer);
}

JSHandle<TaggedArray> JSPluralRules::BuildLocaleSet(JSThread *thread, const std::set<std::string> &icuAvailableLocales)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> locales = factory->NewTaggedArray(icuAvailableLocales.size());
    int32_t index = 0;

    for (const std::string &locale : icuAvailableLocales) {
        JSHandle<EcmaString> localeStr = factory->NewFromStdString(locale);
        locales->Set(thread, index++, localeStr);
    }
    return locales;
}

bool GetNextLocale(icu::StringEnumeration *locales, std::string &localeStr, int32_t *len)
{
    UErrorCode status = U_ZERO_ERROR;
    const char *locale = nullptr;
    locale = locales->next(len, status);
    if (!U_SUCCESS(status) || locale == nullptr) {
        localeStr = "";
        return false;
    }
    localeStr = std::string(locale);
    return true;
}

JSHandle<TaggedArray> JSPluralRules::GetAvailableLocales(JSThread *thread)
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::StringEnumeration> locales(icu::PluralRules::getAvailableLocales(status));
    ASSERT(U_SUCCESS(status));
    std::set<std::string> set;
    std::string localeStr;
    int32_t len = 0;
    while (GetNextLocale(locales.get(), localeStr, &len)) {
        if (len >= STRING_SEPARATOR_LENGTH) {
            std::replace(localeStr.begin(), localeStr.end(), '_', '-');
        }
        set.insert(localeStr);
    }
    return BuildLocaleSet(thread, set);
}

// InitializePluralRules ( pluralRules, locales, options )
JSHandle<JSPluralRules> JSPluralRules::InitializePluralRules(JSThread *thread,
                                                             const JSHandle<JSPluralRules> &pluralRules,
                                                             const JSHandle<JSTaggedValue> &locales,
                                                             const JSHandle<JSTaggedValue> &options)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    auto globalConst = thread->GlobalConstants();

    // 1. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPluralRules, thread);

    // 2&3. If options is undefined, then Let options be ObjectCreate(null). else Let options be ? ToObject(options).
    JSHandle<JSObject> prOptions;
    if (!options->IsUndefined()) {
        prOptions = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPluralRules, thread);
    } else {
        prOptions = factory->OrdinaryNewJSObjectCreate(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()));
    }

    // 5. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    LocaleMatcherOption matcher =
        JSLocale::GetOptionOfString(thread, prOptions, globalConst->GetHandledLocaleMatcherString(),
                                    {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT},
                                    {"lookup", "best fit"}, LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPluralRules, thread);

    // 7. Let t be ? GetOption(options, "type", "string", « "cardinal", "ordinal" », "cardinal").
    JSHandle<JSTaggedValue> property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledTypeString());
    TypeOption type =
        JSLocale::GetOptionOfString(thread, prOptions, property, { TypeOption::CARDINAL, TypeOption::ORDINAL },
                                    { "cardinal", "ordinal" }, TypeOption::CARDINAL);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPluralRules, thread);

    // set pluralRules.[[type]] to type
    pluralRules->SetType(type);

    // Let r be ResolveLocale(%PluralRules%.[[AvailableLocales]], requestedLocales, opt,
    // %PluralRules%.[[RelevantExtensionKeys]], localeData).
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = GetAvailableLocales(thread);
    }
    std::set<std::string> relevantExtensionKeys{""};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPluralRules, thread);
    icu::Locale icuLocale = r.localeData;

    // Get ICU numberFormatter with given locale
    icu::number::LocalizedNumberFormatter icuNumberFormatter =
        icu::number::NumberFormatter::withLocale(icuLocale).roundingMode(UNUM_ROUND_HALFUP);

    bool sucess = true;
    UErrorCode status = U_ZERO_ERROR;
    UPluralType icuType = UPLURAL_TYPE_CARDINAL;
    // Trans typeOption to ICU typeOption
    switch (type) {
        case TypeOption::ORDINAL:
            icuType = UPLURAL_TYPE_ORDINAL;
            break;
        case TypeOption::CARDINAL:
            icuType = UPLURAL_TYPE_CARDINAL;
            break;
        default:
            UNREACHABLE();
    }
    std::unique_ptr<icu::PluralRules> icuPluralRules(icu::PluralRules::forLocale(icuLocale, icuType, status));
    if (U_FAILURE(status)) {  // NOLINT(readability-implicit-bool-conversion)
        sucess = false;
    }

    // Trans typeOption to ICU typeOption
    if (!sucess || icuPluralRules == nullptr) {
        icu::Locale noExtensionLocale(icuLocale.getBaseName());
        status = U_ZERO_ERROR;
        icuType = UPLURAL_TYPE_CARDINAL;
        switch (type) {
            case TypeOption::ORDINAL:
                icuType = UPLURAL_TYPE_ORDINAL;
                break;
            case TypeOption::CARDINAL:
                icuType = UPLURAL_TYPE_CARDINAL;
                break;
            default:
                UNREACHABLE();
        }
        icuPluralRules.reset(icu::PluralRules::forLocale(icuLocale, icuType, status));
    }
    if (U_FAILURE(status) || icuPluralRules == nullptr) {  // NOLINT(readability-implicit-bool-conversion)
        THROW_RANGE_ERROR_AND_RETURN(thread, "cannot create icuPluralRules", pluralRules);
    }

    // 9. Perform ? SetNumberFormatDigitOptions(pluralRules, options, 0, 3, "standard").
    JSLocale::SetNumberFormatDigitOptions(thread, pluralRules, JSHandle<JSTaggedValue>::Cast(prOptions), MNFD_DEFAULT,
                                          MXFD_DEFAULT, NotationOption::STANDARD);
    icuNumberFormatter = JSNumberFormat::SetICUFormatterDigitOptions(icuNumberFormatter, pluralRules);

    // Set pluralRules.[[IcuPluralRules]] to icuPluralRules
    SetIcuPluralRules(thread, pluralRules, *icuPluralRules, JSPluralRules::FreeIcuPluralRules);

    // Set pluralRules.[[IcuNumberFormat]] to icuNumberFormatter
    SetIcuNumberFormatter(thread, pluralRules, icuNumberFormatter, JSPluralRules::FreeIcuNumberFormatter);

    // 12. Set pluralRules.[[Locale]] to the value of r.[[locale]].
    JSHandle<EcmaString> localeStr = JSLocale::ToLanguageTag(thread, icuLocale);
    pluralRules->SetLocale(thread, localeStr.GetTaggedValue());

    // 13. Return pluralRules.
    return pluralRules;
}

JSHandle<EcmaString> FormatNumericToString(JSThread *thread, const icu::number::LocalizedNumberFormatter *icuFormatter,
                                           const icu::PluralRules *icuPluralRules, double n)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::number::FormattedNumber formatted = icuFormatter->formatDouble(n, status);
    if (U_FAILURE(status)) {  // NOLINT(readability-implicit-bool-conversion)
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid resolve number", JSHandle<EcmaString>::Cast(exception));
    }

    icu::UnicodeString uString = icuPluralRules->select(formatted, status);
    if (U_FAILURE(status)) {  // NOLINT(readability-implicit-bool-conversion)
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid resolve number", JSHandle<EcmaString>::Cast(exception));
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> result =
        factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(uString.getBuffer()), uString.length());
    return result;
}
JSHandle<EcmaString> JSPluralRules::ResolvePlural(JSThread *thread, const JSHandle<JSPluralRules> &pluralRules,
                                                  double n)
{
    icu::PluralRules *icuPluralRules = pluralRules->GetIcuPluralRules();
    icu::number::LocalizedNumberFormatter *icuFormatter = pluralRules->GetIcuNumberFormatter();
    if (icuPluralRules == nullptr || icuFormatter == nullptr) {
        return JSHandle<EcmaString>(thread, JSTaggedValue::Undefined());
    }

    JSHandle<EcmaString> result = FormatNumericToString(thread, icuFormatter, icuPluralRules, n);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    return result;
}

void JSPluralRules::ResolvedOptions(JSThread *thread, const JSHandle<JSPluralRules> &pluralRules,
                                    const JSHandle<JSObject> &options)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    auto globalConst = thread->GlobalConstants();

    // [[Locale]]
    JSHandle<JSTaggedValue> property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledLocaleString());
    JSHandle<EcmaString> locale(thread, pluralRules->GetLocale());
    PropertyDescriptor localeDesc(thread, JSHandle<JSTaggedValue>::Cast(locale), true, true, true);
    JSObject::DefineOwnProperty(thread, options, property, localeDesc);

    // [[type]]
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledTypeString());
    JSHandle<JSTaggedValue> typeValue;
    if (pluralRules->GetType() == TypeOption::CARDINAL) {
        typeValue = globalConst->GetHandledCardinalString();
    } else {
        typeValue = globalConst->GetHandledOrdinalString();
    }
    PropertyDescriptor typeDesc(thread, typeValue, true, true, true);
    JSObject::DefineOwnProperty(thread, options, property, typeDesc);

    // [[MinimumIntegerDigits]]
    property = JSHandle<JSTaggedValue>::Cast(globalConst->GetHandledMinimumIntegerDigitsString());
    JSHandle<JSTaggedValue> minimumIntegerDigits(thread, pluralRules->GetMinimumIntegerDigits());
    JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumIntegerDigits);

    RoundingType roundingType = pluralRules->GetRoundingType();
    if (roundingType == RoundingType::SIGNIFICANTDIGITS) {
        // [[MinimumSignificantDigits]]
        property = globalConst->GetHandledMinimumSignificantDigitsString();
        JSHandle<JSTaggedValue> minimumSignificantDigits(thread, pluralRules->GetMinimumSignificantDigits());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumSignificantDigits);
        // [[MaximumSignificantDigits]]
        property = globalConst->GetHandledMaximumSignificantDigitsString();
        JSHandle<JSTaggedValue> maximumSignificantDigits(thread, pluralRules->GetMaximumSignificantDigits());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, maximumSignificantDigits);
    } else {
        // [[MinimumFractionDigits]]
        property = globalConst->GetHandledMinimumFractionDigitsString();
        JSHandle<JSTaggedValue> minimumFractionDigits(thread, pluralRules->GetMinimumFractionDigits());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumFractionDigits);
        // [[MaximumFractionDigits]]
        property = globalConst->GetHandledMaximumFractionDigitsString();
        JSHandle<JSTaggedValue> maximumFractionDigits(thread, pluralRules->GetMaximumFractionDigits());
        JSObject::CreateDataPropertyOrThrow(thread, options, property, maximumFractionDigits);
    }

    // 5. Let pluralCategories be a List of Strings representing the possible results of PluralRuleSelect
    // for the selected locale pr.[[Locale]]. This List consists of unique String values,
    // from the the list "zero", "one", "two", "few", "many" and "other",
    // that are relevant for the locale whose localization is specified in LDML Language Plural Rules.
    UErrorCode status = U_ZERO_ERROR;
    icu::PluralRules *icuPluralRules = pluralRules->GetIcuPluralRules();
    ASSERT(icuPluralRules != nullptr);
    std::unique_ptr<icu::StringEnumeration> categories(icuPluralRules->getKeywords(status));
    int32_t count = categories->count(status);
    ASSERT(U_SUCCESS(status));
    JSHandle<TaggedArray> pluralCategories = factory->NewTaggedArray(count);
    for (int32_t i = 0; i < count; i++) {
        const icu::UnicodeString *category = categories->snext(status);
        ASSERT(U_SUCCESS(status));
        JSHandle<EcmaString> value = JSLocale::IcuToString(thread, *category);
        pluralCategories->Set(thread, i, value);
    }

    // 6. Perform ! CreateDataProperty(options, "pluralCategories", CreateArrayFromList(pluralCategories)).
    property = globalConst->GetHandledPluralCategoriesString();
    JSHandle<JSArray> jsPluralCategories = JSArray::CreateArrayFromList(thread, pluralCategories);
    JSObject::CreateDataPropertyOrThrow(thread, options, property, JSHandle<JSTaggedValue>::Cast(jsPluralCategories));
}
}  // namespace panda::ecmascript
