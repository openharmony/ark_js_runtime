/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_displaynames.h"
#include "cstring.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "unicode/errorcode.h"
#include "unicode/locdspnm.h"
#include "unicode/locid.h"
#include "unicode/udisplaycontext.h"
#include "unicode/uloc.h"
#include "unicode/unistr.h"
#include "unicode/uscript.h"
#include "unicode/ustring.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "unicode/localebuilder.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript {
icu::LocaleDisplayNames *JSDisplayNames::GetIcuLocaleDisplayNames() const
{
    ASSERT(GetIcuLDN().IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetIcuLDN().GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::LocaleDisplayNames *>(result);
}

void JSDisplayNames::FreeIcuLocaleDisplayNames(void *pointer, [[maybe_unused]] void* hint)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuLocaleDisplayNames = reinterpret_cast<icu::LocaleDisplayNames *>(pointer);
    icuLocaleDisplayNames->~LocaleDisplayNames();
}


void JSDisplayNames::SetIcuLocaleDisplayNames(JSThread *thread, const JSHandle<JSDisplayNames> &displayNames,
                                              icu::LocaleDisplayNames* iculocaledisplaynames,
                                              const DeleteEntryPoint &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    
    ASSERT(iculocaledisplaynames != nullptr);
    JSTaggedValue data = displayNames->GetIcuLDN();
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(iculocaledisplaynames);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(iculocaledisplaynames);
    pointer->SetDeleter(callback);
    displayNames->SetIcuLDN(thread, pointer.GetTaggedValue());
}

JSHandle<TaggedArray> JSDisplayNames::GetAvailableLocales(JSThread *thread)
{
    const char *key = "calendar";
    const char *path = nullptr;
    JSHandle<TaggedArray> availableLocales = JSLocale::GetAvailableLocales(thread, key, path);
    return availableLocales;
}

namespace
{
    bool IsUnicodeScriptSubtag(const std::string& value)
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::LocaleBuilder builder;
        builder.setScript(value).build(status);
        return U_SUCCESS(status);
    }

    bool IsUnicodeRegionSubtag(const std::string& value)
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::LocaleBuilder builder;
        builder.setRegion(value).build(status);
        return U_SUCCESS(status);
    }
}

// InitializeDisplayNames ( displayNames, locales, options )
JSHandle<JSDisplayNames> JSDisplayNames::InitializeDisplayNames(JSThread *thread,
                                                                const JSHandle<JSDisplayNames> &displayNames,
                                                                const JSHandle<JSTaggedValue> &locales,
                                                                const JSHandle<JSTaggedValue> &options)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    auto globalConst = thread->GlobalConstants();
    // 3. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 4. If options is undefined, throw a TypeError exception.
    if (options->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "options is undefined", displayNames);
    }

    // 5. Let options be ? GetOptionsObject(options).
    JSHandle<JSObject> optionsObject = JSTaggedValue::ToObject(thread, options);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // Note: No need to create a record. It's not observable.
    // 6. Let opt be a new Record.
    // 7. Let localeData be %DisplayNames%.[[LocaleData]].
    // 8. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleMatcherString();
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, optionsObject, property, {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT},
        {"lookup", "best fit"}, LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 10. Let r be ResolveLocale(%DisplayNames%.[[AvailableLocales]], requestedLocales, opt,
    // %DisplayNames%.[[RelevantExtensionKeys]]).
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = JSDisplayNames::GetAvailableLocales(thread);
    }
    std::set<std::string> relevantExtensionKeys{""};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);
    icu::Locale icuLocale = r.localeData;

    // 11. Let style be ? GetOption(options, "style", "string", « "narrow", "short", "long" », "long").
    property = globalConst->GetHandledStyleString();
    auto StyOpt = JSLocale::GetOptionOfString<StyOption>(thread, optionsObject, property,
                                                         {StyOption::NARROW, StyOption::SHORT, StyOption::LONG},
                                                         {"narrow", "short", "long"}, StyOption::LONG);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 12. Set DisplayNames.[[Style]] to style.
    displayNames->SetStyle(StyOpt);

    // 13. Let type be ? GetOption(options, "type", "string", « "language", "region", "script", "currency" »,
    // "undefined").
    property = globalConst->GetHandledTypeString();
    auto type = JSLocale::GetOptionOfString<TypednsOption>(thread, optionsObject, property,
                                                           {TypednsOption::LANGUAGE, TypednsOption::REGION,
                                                           TypednsOption::SCRIPT, TypednsOption::CURRENCY},
                                                           {"language", "region", "script", "currency"},
                                                           TypednsOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 14. If type is undefined, throw a TypeError exception.
    if (type == TypednsOption::UNDEFINED) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "type is undefined", displayNames);
    }

    // 15. Set displayNames.[[Type]] to type.
    displayNames->SetType(type);

    // 16. Let fallback be ? GetOption(options, "fallback", "string", « "code", "none" », "code").
    property = globalConst->GetHandledFallbackString();
    auto fallback = JSLocale::GetOptionOfString<FallbackOption>(thread, optionsObject, property,
                                                                {FallbackOption::CODE, FallbackOption::NONE},
                                                                {"code", "none"}, FallbackOption::CODE);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 17. Set displayNames.[[Fallback]] to fallback.
    displayNames->SetFallback(fallback);

    // 18. Set displayNames.[[Locale]] to the value of r.[[Locale]].
    JSHandle<EcmaString> localeStr = JSLocale::ToLanguageTag(thread, icuLocale);
    displayNames->SetLocale(thread, localeStr.GetTaggedValue());
    // 19. Let dataLocale be r.[[dataLocale]].
    // 20. Let dataLocaleData be localeData.[[<dataLocale>]].
    // 21. Let types be dataLocaleData.[[types]].
    // 22. Assert: types is a Record (see 12.3.3).
    // 23. Let typeFields be types.[[<type>]].
    // 24. Assert: typeFields is a Record (see 12.3.3).
    // 25. Let styleFields be typeFields.[[<style>]].
    // 26. Assert: styleFields is a Record (see 12.3.3).
    // 27. Set displayNames.[[Fields]] to styleFields.
    // 28. Return displayNames.

    // Trans StyOption to ICU Style
    UDisplayContext uStyle;
    switch (StyOpt) {
        case StyOption::LONG:
            uStyle = UDISPCTX_LENGTH_FULL;
            break;
        case StyOption::SHORT:
            uStyle = UDISPCTX_LENGTH_SHORT;
            break;
        case StyOption::NARROW:
            uStyle = UDISPCTX_LENGTH_SHORT;
            break;
        default:
            UNREACHABLE();
    }
    UDisplayContext display_context[] = {uStyle};
    icu::LocaleDisplayNames *icudisplaynames(icu::LocaleDisplayNames::createInstance(icuLocale, display_context, 1));
    SetIcuLocaleDisplayNames(thread, displayNames, icudisplaynames, JSDisplayNames::FreeIcuLocaleDisplayNames);
    return displayNames;
}

// CanonicalCodeForDisplayNames ( type, code )
JSHandle<EcmaString> JSDisplayNames::CanonicalCodeForDisplayNames(JSThread *thread,
                                                                  const JSHandle<JSDisplayNames> &displayNames,
                                                                  const TypednsOption &typeOpt,
                                                                  const JSHandle<EcmaString> &code)
{
    if (typeOpt == TypednsOption::LANGUAGE) {
        // a. If code does not match the unicode_language_id production, throw a RangeError exception.
        UErrorCode status = U_ZERO_ERROR;
        std::string codeSt = JSLocale::ConvertToStdString(code);
        icu::Locale loc = icu::Locale(icu::Locale::forLanguageTag(codeSt, status).getBaseName());
        std::string checked = loc.toLanguageTag<std::string>(status);
        if (checked.size() == 0) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "not match the language id", code);
        }
        if (U_FAILURE(status)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "not match the unicode_language_id", code);
        }
        // b. If IsStructurallyValidLanguageTag(code) is false, throw a RangeError exception.
        // c. Set code to CanonicalizeUnicodeLocaleId(code).
        // d. Return code.
        if (!JSLocale::IsStructurallyValidLanguageTag(code)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "not a structurally valid", code);
        }
        JSHandle<EcmaString> codeStr = JSLocale::CanonicalizeUnicodeLocaleId(thread, code);
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames();
        icu::UnicodeString result;
        std::string codeString = JSLocale::ConvertToStdString(codeStr);
        icuLocaldisplaynames->languageDisplayName(codeString.c_str(), result);
        JSHandle<EcmaString> codeResult = JSLocale::IcuToString(thread, result);
        return codeResult;
    } else if (typeOpt == TypednsOption::REGION) {
        // a. If code does not match the unicode_region_subtag production, throw a RangeError exception.
        std::string regionCode = JSLocale::ConvertToStdString(code);
        if (!IsUnicodeRegionSubtag(regionCode)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid region", code);
        }
        // b. Let code be the result of mapping code to upper case as described in 6.1.
        // c. Return code.
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames();
        icu::UnicodeString result;
        icuLocaldisplaynames->regionDisplayName(regionCode.c_str(), result);
        JSHandle<EcmaString> codeResult = JSLocale::IcuToString(thread, result);
        return codeResult;
    } else if (typeOpt == TypednsOption::SCRIPT) {
        std::string scriptCode = JSLocale::ConvertToStdString(code);
        if (!IsUnicodeScriptSubtag(scriptCode)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid script", code);
        }
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames();
        icu::UnicodeString result;
        icuLocaldisplaynames->scriptDisplayName(scriptCode.c_str(), result);
        JSHandle<EcmaString> codeResult = JSLocale::IcuToString(thread, result);
        return codeResult;
    }
    // 4. 4. Assert: type is "currency".
    // 5. If ! IsWellFormedCurrencyCode(code) is false, throw a RangeError exception.
    ASSERT(typeOpt == TypednsOption::CURRENCY);
    std::string cCode = JSLocale::ConvertToStdString(code);
    if (!JSLocale::IsWellFormedCurrencyCode(cCode)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "not a wellformed currency code", code);
    }
    icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames();
    icu::UnicodeString result;
    icuLocaldisplaynames->keyValueDisplayName("currency", cCode.c_str(), result);
    JSHandle<EcmaString> codeResult = JSLocale::IcuToString(thread, result);
    return codeResult;
}

JSHandle<JSTaggedValue> StyOptionToEcmaString(JSThread *thread, StyOption style)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (style) {
        case StyOption::LONG:
            result.Update(globalConst->GetHandledLongString().GetTaggedValue());
            break;
        case StyOption::SHORT:
            result.Update(globalConst->GetHandledShortString().GetTaggedValue());
            break;
        case StyOption::NARROW:
            result.Update(globalConst->GetHandledNarrowString().GetTaggedValue());
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> TypeOptionToEcmaString(JSThread *thread, TypednsOption type)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (type) {
        case TypednsOption::LANGUAGE:
            result.Update(globalConst->GetHandledLanguageString().GetTaggedValue());
            break;
        case TypednsOption::CALENDAR:
            result.Update(globalConst->GetHandledCalendarString().GetTaggedValue());
            break;
        case TypednsOption::CURRENCY:
            result.Update(globalConst->GetHandledCurrencyString().GetTaggedValue());
            break;
        case TypednsOption::DATETIMEFIELD:
            result.Update(globalConst->GetHandledDateTimeFieldString().GetTaggedValue());
            break;
        case TypednsOption::REGION:
            result.Update(globalConst->GetHandledRegionString().GetTaggedValue());
            break;
        case TypednsOption::SCRIPT:
            result.Update(globalConst->GetHandledScriptString().GetTaggedValue());
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> FallbackOptionToEcmaString(JSThread *thread, FallbackOption fallback)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (fallback) {
        case FallbackOption::CODE:
            result.Update(globalConst->GetHandledCodeString().GetTaggedValue());
            break;
        case FallbackOption::NONE:
            result.Update(globalConst->GetHandledNoneString().GetTaggedValue());
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

void JSDisplayNames::ResolvedOptions(JSThread *thread, const JSHandle<JSDisplayNames> &displayNames,
                                     const JSHandle<JSObject> &options)
{
    auto globalConst = thread->GlobalConstants();

    // [[Locale]]
    JSHandle<JSTaggedValue> propertyKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> locale(thread, displayNames->GetLocale());
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, locale);

    // [[Style]]
    StyOption style = displayNames->GetStyle();
    propertyKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleString = StyOptionToEcmaString(thread, style);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, styleString);

    // [[type]]
    TypednsOption type = displayNames->GetType();
    propertyKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> typeString = TypeOptionToEcmaString(thread, type);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, typeString);

    // [[fallback]]
    FallbackOption fallback = displayNames->GetFallback();
    propertyKey = globalConst->GetHandledFallbackString();
    JSHandle<JSTaggedValue> fallbackString = FallbackOptionToEcmaString(thread, fallback);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, fallbackString);
}
}  // namespace panda::ecmascript