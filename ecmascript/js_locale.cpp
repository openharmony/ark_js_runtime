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

#include "base/string_helper.h"
#include "ecma_macros.h"
#include "ecma_vm.h"
#include "global_env.h"
#include "js_locale.h"
#include "object_factory.h"

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

#include "unicode/localematcher.h"

namespace panda::ecmascript {
const std::string LATN_STRING = "latn";
// 6.2.2 IsStructurallyValidLanguageTag( locale )
bool JSLocale::IsStructurallyValidLanguageTag(const JSHandle<EcmaString> &tag)
{
    std::string tagCollection = ConvertToStdString(tag);
    std::vector<std::string> containers;
    std::string substring;
    std::set<std::string> uniqueSubtags;
    size_t address = 1;
    for (auto it = tagCollection.begin(); it != tagCollection.end(); it++) {
        if (*it != '-' && it != tagCollection.end() - 1) {
            substring += *it;
        } else {
            if (it == tagCollection.end() - 1) {
                substring += *it;
            }
            containers.push_back(substring);
            if (IsVariantSubtag(substring)) {
                std::transform(substring.begin(), substring.end(), substring.begin(), AsciiAlphaToLower);
                if (!uniqueSubtags.insert(substring).second) {
                    return false;
                }
            }
            substring.clear();
        }
    }
    bool result = DealwithLanguageTag(containers, address);
    return result;
}

std::string JSLocale::ConvertToStdString(const JSHandle<EcmaString> &ecmaStr)
{
    return std::string(ConvertToString(*ecmaStr, StringConvertedUsage::LOGICOPERATION));
}

// 6.2.3 CanonicalizeUnicodeLocaleId( locale )
JSHandle<EcmaString> JSLocale::CanonicalizeUnicodeLocaleId(JSThread *thread, const JSHandle<EcmaString> &locale)
{
    [[maybe_unused]] ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!IsStructurallyValidLanguageTag(locale)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }

    if (locale->GetLength() == 0 || locale->IsUtf16()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }

    std::string localeCStr = ConvertToStdString(locale);
    std::transform(localeCStr.begin(), localeCStr.end(), localeCStr.begin(), AsciiAlphaToLower);
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale formalLocale = icu::Locale::forLanguageTag(localeCStr.c_str(), status);
    if ((U_FAILURE(status) != 0) || (formalLocale.isBogus() != 0)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }

    // Resets the LocaleBuilder to match the locale.
    // Returns an instance of Locale created from the fields set on this builder.
    formalLocale = icu::LocaleBuilder().setLocale(formalLocale).build(status);
    // Canonicalize the locale ID of this object according to CLDR.
    formalLocale.canonicalize(status);
    if ((U_FAILURE(status) != 0) || (formalLocale.isBogus() != 0)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }
    JSHandle<EcmaString> languageTag = ToLanguageTag(thread, formalLocale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    return languageTag;
}

// 6.2.4 DefaultLocale ()
JSHandle<EcmaString> JSLocale::DefaultLocale(JSThread *thread)
{
    icu::Locale defaultLocale;
    auto globalConst = thread->GlobalConstants();
    if (strcmp(defaultLocale.getName(), "en_US_POSIX") == 0 || strcmp(defaultLocale.getName(), "c") == 0) {
        return JSHandle<EcmaString>::Cast(globalConst->GetHandledEnUsString());
    }
    if (defaultLocale.isBogus() != 0) {
        return JSHandle<EcmaString>::Cast(globalConst->GetHandledUndString());
    }
    return ToLanguageTag(thread, defaultLocale);
}

// 6.4.1 IsValidTimeZoneName ( timeZone )
bool JSLocale::IsValidTimeZoneName(const icu::TimeZone &tz)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString id;
    tz.getID(id);
    icu::UnicodeString canonical;
    icu::TimeZone::getCanonicalID(id, canonical, status);
    UBool canonicalFlag = (canonical != icu::UnicodeString("Etc/Unknown", -1, US_INV));
    return (U_SUCCESS(status) != 0) && (canonicalFlag != 0);
}

void JSLocale::HandleLocaleExtension(size_t &start, size_t &extensionEnd,
                                     const std::string result, size_t len)
{
    bool flag = false;
    while (start < len - INTL_INDEX_TWO) {
        if (result[start] != '-') {
            start++;
            continue;
        }
        if (result[start + INTL_INDEX_TWO] == '-') {
            extensionEnd = start;
            flag = true;
            break;
        }
        if (!flag) {
            start++;
        }
        start += INTL_INDEX_TWO;
    }
}

JSLocale::ParsedLocale JSLocale::HandleLocale(const JSHandle<EcmaString> &localeString)
{
    std::string result = ConvertToStdString(localeString);
    size_t len = result.size();
    ParsedLocale parsedResult;

    // a. The single-character subtag ’x’ as the primary subtag indicates
    //    that the language tag consists solely of subtags whose meaning is
    //    defined by private agreement.
    // b. Extensions cannot be used in tags that are entirely private use.
    if (IsPrivateSubTag(result, len)) {
        parsedResult.base = result;
        return parsedResult;
    }
    // If cannot find "-u-", return the whole string as base.
    size_t foundExtension = result.find("-u-");
    if (foundExtension == std::string::npos) {
        parsedResult.base = result;
        return parsedResult;
    }
    // Let privateIndex be Call(%StringProto_indexOf%, foundLocale, « "-x-" »).
    size_t privateIndex = result.find("-x-");
    if (privateIndex != std::string::npos && privateIndex < foundExtension) {
        parsedResult.base = result;
        return parsedResult;
    }
    const std::string basis = result.substr(INTL_INDEX_ZERO, foundExtension);
    size_t extensionEnd = len;
    ASSERT(len > INTL_INDEX_TWO);
    size_t start = foundExtension + INTL_INDEX_ONE;
    HandleLocaleExtension(start, extensionEnd, result, len);
    const std::string end = result.substr(extensionEnd);
    parsedResult.base = basis + end;
    parsedResult.extension = result.substr(foundExtension, extensionEnd - foundExtension);
    return parsedResult;
}

// 9.2.1 CanonicalizeLocaleList ( locales )
JSHandle<TaggedArray> JSLocale::CanonicalizeLocaleList(JSThread *thread, const JSHandle<JSTaggedValue> &locales)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If locales is undefined, then
    //    a. Return a new empty List.
    if (locales->IsUndefined()) {
        return factory->EmptyArray();
    }
    // 2. Let seen be a new empty List.
    JSHandle<TaggedArray> localeSeen = factory->NewTaggedArray(1);
    // 3. If Type(locales) is String or Type(locales) is Object and locales has an [[InitializedLocale]] internal slot,
    //    then
    //    a. Let O be CreateArrayFromList(« locales »).
    // 4. Else,
    //    a.Let O be ? ToObject(locales).
    if (locales->IsString()) {
        JSHandle<EcmaString> tag = JSHandle<EcmaString>::Cast(locales);
        JSHandle<TaggedArray> temp = factory->NewTaggedArray(1);
        temp->Set(thread, 0, tag.GetTaggedValue());
        JSHandle<JSArray> obj = JSArray::CreateArrayFromList(thread, temp);
        JSHandle<TaggedArray> finalSeen = CanonicalizeHelper<JSArray>(thread, obj, localeSeen);
        return finalSeen;
    } else if (locales->IsJSLocale()) {
        JSHandle<EcmaString> tag = JSLocale::ToString(thread, JSHandle<JSLocale>::Cast(locales));
        JSHandle<TaggedArray> temp = factory->NewTaggedArray(1);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
        temp->Set(thread, 0, tag.GetTaggedValue());
        JSHandle<JSArray> obj = JSArray::CreateArrayFromList(thread, temp);
        JSHandle<TaggedArray> finalSeen = CanonicalizeHelper<JSArray>(thread, obj, localeSeen);
        return finalSeen;
    } else {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, locales);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
        JSHandle<TaggedArray> finalSeen = CanonicalizeHelper<JSObject>(thread, obj, localeSeen);
        return finalSeen;
    }
    return localeSeen;
}

template<typename T>
JSHandle<TaggedArray> JSLocale::CanonicalizeHelper(JSThread *thread, JSHandle<T> &obj, JSHandle<TaggedArray> &seen)
{
    OperationResult operationResult = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj),
                                                                 thread->GlobalConstants()->GetHandledLengthString());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
    JSTaggedNumber len = JSTaggedValue::ToLength(thread, operationResult.GetValue());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. Let seen be a new empty List.
    uint32_t requestedLocalesLen = len.ToUint32();
    seen = factory->NewTaggedArray(requestedLocalesLen);
    // 6. Let k be 0.
    // 7. Repeat, while k < len
    JSMutableHandle<JSTaggedValue> pk(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> tag(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    JSHandle<JSTaggedValue> objTagged = JSHandle<JSTaggedValue>::Cast(obj);
    for (uint32_t k = 0; k < requestedLocalesLen; k++) {
        // a. Let Pk be ToString(k).
        JSHandle<JSTaggedValue> kHandle(thread, JSTaggedValue(k));
        JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, kHandle);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
        pk.Update(str.GetTaggedValue());
        // b. Let kPresent be ? HasProperty(O, Pk).
        bool kPresent = JSTaggedValue::HasProperty(thread, objTagged, pk);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

        // c. If kPresent is true, then
        if (kPresent) {
            // i. Let kValue be ? Get(O, Pk).
            OperationResult result = JSTaggedValue::GetProperty(thread, objTagged, pk);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
            JSHandle<JSTaggedValue> kValue = result.GetValue();
            // ii. If Type(kValue) is not String or Object, throw a TypeError exception.
            if (!kValue->IsString() && !kValue->IsJSObject()) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "kValue is not String or Object.", factory->EmptyArray());
            }
            // iii. If Type(kValue) is Object and kValue has an [[InitializedLocale]] internal slot, then
            //        1. Let tag be kValue.[[Locale]].
            // iv.  Else,
            //        1. Let tag be ? ToString(kValue).
            if (kValue->IsJSLocale()) {
                JSHandle<EcmaString> kValueStr = JSLocale::ToString(thread, JSHandle<JSLocale>::Cast(kValue));
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                tag.Update(kValueStr.GetTaggedValue());
            } else {
                JSHandle<EcmaString> kValueString = JSTaggedValue::ToString(thread, kValue);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                JSHandle<EcmaString> canonicalStr = CanonicalizeUnicodeLocaleId(thread, kValueString);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                tag.Update(canonicalStr.GetTaggedValue());
            }
            // vii. If canonicalizedTag is not an element of seen, append canonicalizedTag as the last element of seen.
            bool isExist = false;
            uint32_t seenLen = seen->GetLength();
            for (uint32_t i = 0; i < seenLen; i++) {
                if (JSTaggedValue::SameValue(seen->Get(thread, i), tag.GetTaggedValue())) {
                    isExist = true;
                }
            }
            if (!isExist) {
                seen->Set(thread, index++, JSHandle<JSTaggedValue>::Cast(tag));
            }
        }
        // d. Increase k by 1.
    }
    // set capacity
    seen = TaggedArray::SetCapacity(thread, seen, index);
    // 8. Return seen.
    return seen;
}

// 9.2.2 BestAvailableLocale ( availableLocales, locale )
std::string JSLocale::BestAvailableLocale(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                          const std::string &locale)
{
    // 1. Let candidate be locale.
    std::string localeCandidate = locale;
    std::string undefined = std::string();
    // 2. Repeat,
    uint32_t length = availableLocales->GetLength();
    JSMutableHandle<EcmaString> item(thread, JSTaggedValue::Undefined());
    while (true) {
        // a. If availableLocales contains an element equal to candidate, return candidate.
        for (uint32_t i = 0; i < length; ++i) {
            item.Update(availableLocales->Get(thread, i));
            std::string itemStr = ConvertToStdString(item);
            if (itemStr == localeCandidate) {
                return localeCandidate;
            }
        }
        // b. Let pos be the character index of the last occurrence of "-" (U+002D) within candidate.
        //    If that character does not occur, return undefined.
        size_t pos = localeCandidate.rfind('-');
        if (pos == std::string::npos) {
            return undefined;
        }
        // c. If pos ≥ 2 and the character "-" occurs at index pos-2 of candidate, decrease pos by 2.
        if (pos >= INTL_INDEX_TWO && localeCandidate[pos - INTL_INDEX_TWO] == '-') {
            pos -= INTL_INDEX_TWO;
        }
        // d. Let candidate be the substring of candidate from position 0, inclusive, to position pos, exclusive.
        localeCandidate = localeCandidate.substr(0, pos);
    }
}

// 9.2.3 LookupMatcher ( availableLocales, requestedLocales )
JSHandle<EcmaString> JSLocale::LookupMatcher(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                             const JSHandle<TaggedArray> &requestedLocales)
{
    MatcherResult result = {std::string(), std::string()};
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let result be a new Record.
    // 2. For each element locale of requestedLocales in List order, do
    uint32_t length = requestedLocales->GetLength();
    JSMutableHandle<EcmaString> locale(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; ++i) {
        locale.Update(requestedLocales->Get(thread, i));
        // 2. a. Let noExtensionsLocale be the String value that is locale
        //       with all Unicode locale extension sequences removed.
        ParsedLocale parsedResult = HandleLocale(locale);
        // 2. b. Let availableLocale be BestAvailableLocale(availableLocales, noExtensionsLocale).
        std::string availableLocale = BestAvailableLocale(thread, availableLocales, parsedResult.base);
        // 2. c. If availableLocale is not undefined, append locale to the end of subset.
        if (!availableLocale.empty()) {
            result = {std::string(), std::string()};
            // 2. c. i. Set result.[[locale]] to availableLocale.
            result.locale = availableLocale;
            // 2. c. ii. If locale and noExtensionsLocale are not the same String value, then
            // 2. c. ii. 1. Let extension be the String value consisting of  the first substring of locale that is a
            //              Unicode locale extension sequence.
            if (!parsedResult.extension.empty()) {
                result.extension = parsedResult.extension;
            }
            // 2. c. ii. 2. Set result.[[extension]] to extension.
            std::string res = result.locale + result.extension;
            // 2. c. iii. Return result.
            return factory->NewFromStdString(res);
        }
    }

    // 3. Let defLocale be DefaultLocale();
    // 4. Set result.[[locale]] to defLocale.
    // 5. Return result.
    std::string defLocale = ConvertToStdString(DefaultLocale(thread));
    result.locale = defLocale;
    return factory->NewFromStdString(result.locale);
}

icu::LocaleMatcher BuildLocaleMatcher(JSThread *thread, uint32_t *availableLength, UErrorCode *status,
                                      const JSHandle<TaggedArray> &availableLocales)
{
    std::string locale = JSLocale::ConvertToStdString(JSLocale::DefaultLocale(thread));
    icu::Locale defaultLocale = icu::Locale::forLanguageTag(locale, *status);
    ASSERT_PRINT(U_SUCCESS(*status), "icu::Locale::forLanguageTag failed");
    icu::LocaleMatcher::Builder builder;
    builder.setDefaultLocale(&defaultLocale);
    uint32_t length = availableLocales->GetLength();

    JSMutableHandle<EcmaString> item(thread, JSTaggedValue::Undefined());
    for (*availableLength = 0; *availableLength < length; ++(*availableLength)) {
        item.Update(availableLocales->Get(thread, *availableLength));
        std::string itemStr = JSLocale::ConvertToStdString(item);
        icu::Locale localeForLanguageTag = icu::Locale::forLanguageTag(itemStr, *status);
        if (U_SUCCESS(*status) != 0) {
            builder.addSupportedLocale(localeForLanguageTag);
        } else {
            break;
        }
    }
    *status = U_ZERO_ERROR;
    return builder.build(*status);
}

// 9.2.4 BestFitMatcher ( availableLocales, requestedLocales )
JSHandle<EcmaString> JSLocale::BestFitMatcher(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                              const JSHandle<TaggedArray> &requestedLocales)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    UErrorCode status = U_ZERO_ERROR;
    uint32_t availableLength = availableLocales->GetLength();
    icu::LocaleMatcher matcher = BuildLocaleMatcher(thread, &availableLength, &status, availableLocales);
    ASSERT(U_SUCCESS(status));

    uint32_t requestedLocalesLength = requestedLocales->GetLength();
    JSIntlIterator iter(requestedLocales, requestedLocalesLength);
    auto bestFit = matcher.getBestMatch(iter, status)->toLanguageTag<std::string>(status);

    if (U_FAILURE(status) != 0) {
        return DefaultLocale(thread);
    }

    for (uint32_t i = 0; i < requestedLocalesLength; ++i) {
        if (iter[i] == bestFit) {
            return JSHandle<EcmaString>(thread, requestedLocales->Get(thread, i));
        }
    }
    return factory->NewFromStdString(bestFit);
}

// 9.2.8 LookupSupportedLocales ( availableLocales, requestedLocales )
JSHandle<TaggedArray> JSLocale::LookupSupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                                       const JSHandle<TaggedArray> &requestedLocales)
{
    uint32_t index = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t length = requestedLocales->GetLength();
    // 1. Let subset be a new empty List.
    JSHandle<TaggedArray> subset = factory->NewTaggedArray(length);
    JSMutableHandle<EcmaString> item(thread, JSTaggedValue::Undefined());
    // 2. For each element locale of requestedLocales in List order, do
    //    a. Let noExtensionsLocale be the String value that is locale with all Unicode locale extension sequences
    //       removed.
    //    b. Let availableLocale be BestAvailableLocale(availableLocales, noExtensionsLocale).
    //    c. If availableLocale is not undefined, append locale to the end of subset.
    for (uint32_t i = 0; i < length; ++i) {
        item.Update(requestedLocales->Get(thread, i));
        ParsedLocale foundationResult = HandleLocale(item);
        std::string availableLocale = BestAvailableLocale(thread, availableLocales, foundationResult.base);
        if (!availableLocale.empty()) {
            subset->Set(thread, index++, item.GetTaggedValue());
        }
    }
    // 3. Return subset.
    return TaggedArray::SetCapacity(thread, subset, index);
}

// 9.2.9 BestFitSupportedLocales ( availableLocales, requestedLocales )
JSHandle<TaggedArray> JSLocale::BestFitSupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                                        const JSHandle<TaggedArray> &requestedLocales)
{
    UErrorCode status = U_ZERO_ERROR;
    uint32_t requestLength = requestedLocales->GetLength();
    uint32_t availableLength = availableLocales->GetLength();
    icu::LocaleMatcher matcher = BuildLocaleMatcher(thread, &availableLength, &status, availableLocales);
    ASSERT(U_SUCCESS(status));

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> defaultLocale = DefaultLocale(thread);
    JSHandle<TaggedArray> result = factory->NewTaggedArray(requestLength);

    uint32_t index = 0;
    JSMutableHandle<EcmaString> locale(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < requestLength; ++i) {
        locale.Update(requestedLocales->Get(thread, i));
        if (EcmaString::StringsAreEqual(*locale, *defaultLocale)) {
            result->Set(thread, index++, locale.GetTaggedValue());
        } else {
            status = U_ZERO_ERROR;
            std::string localeStr = ConvertToStdString(locale);
            icu::Locale desired = icu::Locale::forLanguageTag(localeStr, status);
            auto bestFit = matcher.getBestMatch(desired, status)->toLanguageTag<std::string>(status);
            if ((U_SUCCESS(status) != 0) &&
                EcmaString::StringsAreEqual(*locale, *(factory->NewFromStdString(bestFit)))) {
                result->Set(thread, index++, locale.GetTaggedValue());
            }
        }
    }
    result = TaggedArray::SetCapacity(thread, result, index);
    return result;
}

JSHandle<EcmaString> JSLocale::ToLanguageTag(JSThread *thread, const icu::Locale &locale)
{
    UErrorCode status = U_ZERO_ERROR;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto result = locale.toLanguageTag<std::string>(status);
    bool flag = (U_FAILURE(status) == 0) ? true : false;
    if (!flag) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }
    size_t findBeginning = result.find("-u-");
    std::string finalRes;
    std::string tempRes;
    if (findBeginning == std::string::npos) {
        return factory->NewFromStdString(result);
    }
    size_t specialBeginning  = findBeginning + INTL_INDEX_THREE;
    size_t specialCount = 0;
    while (result[specialBeginning] != '-') {
        specialCount++;
        specialBeginning++;
    }
    if (findBeginning != std::string::npos) {
        // It begin with "-u-xx" or with more elements.
        tempRes = result.substr(0, findBeginning + INTL_INDEX_THREE + specialCount);
        if (result.size() <= findBeginning + INTL_INDEX_THREE + specialCount) {
            return factory->NewFromStdString(result);
        }
        std::string leftStr = result.substr(findBeginning + INTL_INDEX_THREE + specialCount + INTL_INDEX_ONE);
        std::istringstream temp(leftStr);
        std::string buffer;
        std::vector<std::string> resContainer;
        while (getline(temp, buffer, '-')) {
            if (buffer != "true" && buffer != "yes") {
                resContainer.push_back(buffer);
            }
        }
        for (auto it = resContainer.begin(); it != resContainer.end(); it++) {
            std::string tag = "-";
            tag += *it;
            finalRes += tag;
        }
    }
    if (!finalRes.empty()) {
        tempRes += finalRes;
    }
    result = tempRes;
    return factory->NewFromStdString(result);
}

// 9.2.10 SupportedLocales ( availableLocales, requestedLocales, options )
JSHandle<JSArray> JSLocale::SupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                             const JSHandle<TaggedArray> &requestedLocales,
                                             const JSHandle<JSTaggedValue> &options)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. If options is not undefined, then
    //    a. Let options be ? ToObject(options).
    //    b. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    // 2. Else, let matcher be "best fit".
    LocaleMatcherOption matcher = LocaleMatcherOption::BEST_FIT;
    if (!options->IsUndefined()) {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);

        matcher = GetOptionOfString<LocaleMatcherOption>(thread, obj, globalConst->GetHandledLocaleMatcherString(),
                                                         {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT},
                                                         {"lookup", "best fit"}, LocaleMatcherOption::BEST_FIT);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    }

    // 3. If matcher is "best fit", then
    //    a. Let supportedLocales be BestFitSupportedLocales(availableLocales, requestedLocales).
    // 4. Else,
    //    a. Let supportedLocales be LookupSupportedLocales(availableLocales, requestedLocales).
    JSMutableHandle<TaggedArray> supportedLocales(thread, JSTaggedValue::Undefined());
    bool isBestfitSupport = false;
    if (matcher == LocaleMatcherOption::BEST_FIT && isBestfitSupport) {
        supportedLocales.Update(BestFitSupportedLocales(thread, availableLocales, requestedLocales).GetTaggedValue());
    } else {
        supportedLocales.Update(LookupSupportedLocales(thread, availableLocales, requestedLocales).GetTaggedValue());
    }

    JSHandle<JSArray> subset = JSArray::CreateArrayFromList(thread, supportedLocales);
    // 5. Return CreateArrayFromList(supportedLocales).
    return subset;
}

// 9.2.11 GetOption ( options, property, type, values, fallback )
JSHandle<JSTaggedValue> JSLocale::GetOption(JSThread *thread, const JSHandle<JSObject> &options,
                                            const JSHandle<JSTaggedValue> &property, OptionType type,
                                            const JSHandle<JSTaggedValue> &values,
                                            const JSHandle<JSTaggedValue> &fallback)
{
    // 1. Let value be ? Get(options, property).
    JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, options, property).GetValue();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // 2. If value is not undefined, then
    if (!value->IsUndefined()) {
        // a. Assert: type is "boolean" or "string".
        ASSERT_PRINT(type == OptionType::BOOLEAN || type == OptionType::STRING, "type is not boolean or string");

        // b. If type is "boolean", then
        //    i. Let value be ToBoolean(value).
        if (type == OptionType::BOOLEAN) {
            value = JSHandle<JSTaggedValue>(thread, JSTaggedValue(value->ToBoolean()));
        }
        // c. If type is "string", then
        //    i. Let value be ? ToString(value).
        if (type == OptionType::STRING) {
            JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, value);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            value = JSHandle<JSTaggedValue>(thread, str.GetTaggedValue());
        }

        // d. If values is not undefined, then
        //    i. If values does not contain an element equal to value, throw a RangeError exception.
        if (!values->IsUndefined()) {
            bool isExist = false;
            JSHandle<TaggedArray> valuesArray = JSHandle<TaggedArray>::Cast(values);
            uint32_t length = valuesArray->GetLength();
            for (uint32_t i = 0; i < length; i++) {
                if (JSTaggedValue::SameValue(valuesArray->Get(thread, i), value.GetTaggedValue())) {
                    isExist = true;
                }
            }
            if (!isExist) {
                JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
                THROW_RANGE_ERROR_AND_RETURN(thread, "values does not contain an element equal to value", exception);
            }
        }
        // e. Return value.
        return value;
    }
    // 3. Else, return fallback.
    return fallback;
}

bool JSLocale::GetOptionOfString(JSThread *thread, const JSHandle<JSObject> &options,
                                 const JSHandle<JSTaggedValue> &property, const std::vector<std::string> &values,
                                 std::string *optionValue)
{
    // 1. Let value be ? Get(options, property).
    OperationResult operationResult = JSObject::GetProperty(thread, options, property);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<JSTaggedValue> value = operationResult.GetValue();
    // 2. If value is not undefined, then
    if (value->IsUndefined()) {
        return false;
    }
    //    c. If type is "string" "string", then
    //       i. Let value be ? ToString(value).
    JSHandle<EcmaString> valueEStr = JSTaggedValue::ToString(thread, value);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (valueEStr->IsUtf16()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Value out of range for locale options property", false);
    }
    *optionValue = JSLocale::ConvertToStdString(valueEStr);
    if (values.empty()) {
        return true;
    }
    // d. If values is not undefined, then
    //    i. If values does not contain an element equal to value, throw a RangeError exception.
    for (const auto &item : values) {
        if (item == *optionValue) {
            return true;
        }
    }
    THROW_RANGE_ERROR_AND_RETURN(thread, "Value out of range for locale options property", false);
}

// 9.2.12 DefaultNumberOption ( value, minimum, maximum, fallback )
int JSLocale::DefaultNumberOption(JSThread *thread, const JSHandle<JSTaggedValue> &value, int minimum, int maximum,
                                  int fallback)
{
    // 1. If value is not undefined, then
    if (!value->IsUndefined()) {
        // a. Let value be ? ToNumber(value).
        JSTaggedNumber number = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fallback);
        // b. If value is NaN or less than minimum or greater than maximum, throw a RangeError exception.
        double num = JSTaggedValue(number).GetNumber();
        if (std::isnan(num) || num < minimum || num > maximum) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "", fallback);
        }
        // c. Return floor(value).
        return std::floor(num);
    }
    // 2. Else, return fallback.
    return fallback;
}

// 9.2.13 GetNumberOption ( options, property, minimum, maximum, fallback )
int JSLocale::GetNumberOption(JSThread *thread, const JSHandle<JSObject> &options,
                              const JSHandle<JSTaggedValue> &property, int min, int max, int fallback)
{
    // 1. Let value be ? Get(options, property).
    JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, options, property).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fallback);

    // 2. Return ? DefaultNumberOption(value, minimum, maximum, fallback).
    int result = DefaultNumberOption(thread, value, min, max, fallback);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fallback);
    return result;
}

// 9.2.5 UnicodeExtensionValue ( extension, key )
std::string JSLocale::UnicodeExtensionValue(const std::string extension, const std::string key)
{
    // 1. Assert: The number of elements in key is 2.
    // 2. Let size be the number of elements in extension.
    ASSERT(key.size() == INTL_INDEX_TWO);
    size_t size = extension.size();
    // 3. Let searchValue be the concatenation of "-" , key, and "-".
    std::string searchValue = "-" + key + "-";
    // 4. Let pos be Call(%StringProto_indexOf%, extension, « searchValue »).
    size_t pos = extension.find(searchValue);
    // 5. If pos ≠ -1, then
    if (pos != std::string::npos) {
        // a. Let start be pos + 4.
        size_t start = pos + INTL_INDEX_FOUR;
        // b. Let end be start.
        size_t end = start;
        // c. Let k be start.
        size_t k = start;
        // d. Let done be false.
        bool done = false;
        // e. Repeat, while done is false
        while (!done) {
            // i. Let e be Call(%StringProto_indexOf%, extension, « "-", k »).
            size_t e = extension.find("-", k);
            size_t len;
            // ii. If e = -1, let len be size - k; else let len be e - k.
            if  (e == std::string::npos) {
                len = size - k;
            } else {
                len = e - k;
            }
            // iii. If len = 2, then
            //     1. Let done be true.
            if (len == INTL_INDEX_TWO) {
                done = true;
            // iv. Else if e = -1, then
            //    1. Let end be size.
            //    2. Let done be true.
            } else if (e == std::string::npos) {
                end = size;
                done = true;
            // v. Else,
            //   1. Let end be e.
            //   2. Let k be e + 1.
            } else {
                end = e;
                k = e + INTL_INDEX_ONE;
            }
        }
        // f. Return the String value equal to the substring of extension consisting of the code units at indices.
        // start (inclusive) through end (exclusive).
        std::string result = extension.substr(start, end - start);
        return result;
    }
    // 6. Let searchValue be the concatenation of "-" and key.
    searchValue = "-" + key;
    // 7. Let pos be Call(%StringProto_indexOf%, extension, « searchValue »).
    pos = extension.find(searchValue);
    // 8. If pos ≠ -1 and pos + 3 = size, then
    //    a. Return the empty String.
    if (pos != std::string::npos && pos + INTL_INDEX_THREE == size) {
        return "";
    }
    // 9. Return undefined.
    return "undefined";
}

ResolvedLocale JSLocale::ResolveLocale(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                       const JSHandle<TaggedArray> &requestedLocales, LocaleMatcherOption matcher,
                                       const std::set<std::string> &relevantExtensionKeys)
{
    bool isBestfitSupport = false;
    std::map<std::string, std::set<std::string>> localeMap = {
        {"hc", {"h11", "h12", "h23", "h24"}},
        {"lb", {"strict", "normal", "loose"}},
        {"kn", {"true", "false"}},
        {"kf", {"upper", "lower", "false"}}
    };

    // 1. Let matcher be options.[[localeMatcher]].
    // 2. If matcher is "lookup" "lookup", then
    //    a. Let r be LookupMatcher(availableLocales, requestedLocales).
    // 3. Else,
    //    a. Let r be BestFitMatcher(availableLocales, requestedLocales).
    JSMutableHandle<EcmaString> locale(thread, JSTaggedValue::Undefined());
    if (availableLocales->GetLength() == 0 && requestedLocales->GetLength() == 0) {
        locale.Update(DefaultLocale(thread).GetTaggedValue());
    } else {
        if (matcher == LocaleMatcherOption::BEST_FIT && isBestfitSupport) {
            locale.Update(BestFitMatcher(thread, availableLocales, requestedLocales).GetTaggedValue());
        } else {
            locale.Update(LookupMatcher(thread, availableLocales, requestedLocales).GetTaggedValue());
        }
    }

    // 4. Let foundLocale be r.[[locale]].
    // 5. Let result be a new Record.
    // 6. Set result.[[dataLocale]] to foundLocale.
    // 7. Let supportedExtension be "-u".
    std::string foundLocale = ConvertToStdString(locale);
    icu::Locale foundLocaleData = BuildICULocale(foundLocale);
    ResolvedLocale result;
    result.localeData = foundLocaleData;
    JSHandle<EcmaString> tag = ToLanguageTag(thread, foundLocaleData);
    result.locale = ConvertToStdString(tag);
    std::string supportedExtension = "-u";
    icu::LocaleBuilder localeBuilder;
    localeBuilder.setLocale(foundLocaleData).clearExtensions();
    // 8. For each element key of relevantExtensionKeys in List order, do
    for (auto &key : relevantExtensionKeys) {
        auto doubleMatch = foundLocale.find(key);
        if (doubleMatch == std::string::npos) {
            continue;
        }
        UErrorCode status = U_ZERO_ERROR;
        std::set<std::string> keyLocaleData;
        std::unique_ptr<icu::StringEnumeration> wellFormKey(foundLocaleData.createKeywords(status));
        if (U_FAILURE(status) != 0) {
            return result;
        }
        if (!wellFormKey) {
            return result;
        }
        std::string value;

        // c. Let keyLocaleData be foundLocaleData.[[<key>]].
        // e. Let value be keyLocaleData[0].
        if ((key != "ca") && (key != "co") && (key != "nu")) {
            keyLocaleData = localeMap[key];
            value = *keyLocaleData.begin();
        }

        // g. Let supportedExtensionAddition be "".
        // h. If r has an [[extension]] field, then
        std::string  supportedExtensionAddition;
        size_t found = foundLocale.find("-u-");
        if (found != std::string::npos) {
            std::string extension = foundLocale.substr(found + INTL_INDEX_ONE);

            // i. Let requestedValue be UnicodeExtensionValue(r.[[extension]], key).
            std::string requestedValue = UnicodeExtensionValue(extension, key);
            if (key == "kn" && requestedValue.empty()) {
                requestedValue = "true";
            }

            // ii. If requestedValue is not undefined, then
            if (requestedValue != "undefined") {
                // 1. If requestedValue is not the empty String, then
                if (!requestedValue.empty()) {
                    // a. If keyLocaleData contains requestedValue, then
                    //    i. Let value be requestedValue.
                    //    ii. Let supportedExtensionAddition be the concatenation of "-", key, "-", and value.
                    if (key == "ca" || key == "co") {
                        if (key == "co") {
                            bool isValidValue = IsWellCollation(foundLocaleData, requestedValue);
                            if (!isValidValue) {
                                continue;
                            }
                            value = requestedValue;
                            supportedExtensionAddition = "-" + key + "-" + value;
                            localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                        } else {
                            bool isValidValue = IsWellCalendar(foundLocaleData, requestedValue);
                            if (!isValidValue) {
                                continue;
                            }
                            value = requestedValue;
                            supportedExtensionAddition = "-" + key + "-" + value;
                            localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                        }
                    } else if (key == "nu") {
                        bool isValidValue = IsWellNumberingSystem(requestedValue);
                        if (!isValidValue) {
                            continue;
                        }
                        value = requestedValue;
                        supportedExtensionAddition = "-" + key + "-" + value;
                        localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                    } else if (keyLocaleData.find(requestedValue) != keyLocaleData.end()) {
                        value = requestedValue;
                        supportedExtensionAddition = "-" + key + "-" + value;
                        localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                    }
                }
            }
        }
        result.extensions.insert(std::pair<std::string, std::string>(key, value));
        supportedExtension +=  supportedExtensionAddition;
    }
    size_t found = foundLocale.find("-u-");
    if (found != std::string::npos) {
        foundLocale = foundLocale.substr(0, found);
    }

    // 9. If the number of elements in supportedExtension is greater than 2, then
    if (supportedExtension.size() > 2) {
        // a. Let privateIndex be Call(%StringProto_indexOf%, foundLocale, « "-x-" »).
        size_t privateIndex = foundLocale.find("-x-");
        // b. If privateIndex = -1, then
        //    i. Let foundLocale be the concatenation of foundLocale and supportedExtension.
        if (privateIndex == std::string::npos) {
            foundLocale = foundLocale + supportedExtension;
        } else {
            std::string preExtension = foundLocale.substr(0, privateIndex);
            std::string postExtension = foundLocale.substr(privateIndex);
            foundLocale = preExtension + supportedExtension + postExtension;
        }

        tag = ToLanguageTag(thread, foundLocaleData);
        if (!IsStructurallyValidLanguageTag(tag)) {
            result.extensions.erase(result.extensions.begin(), result.extensions.end());
            result.locale = foundLocale;
        }
        tag = CanonicalizeUnicodeLocaleId(thread, tag);
        foundLocale = ConvertToStdString(tag);
    }

    // 10. Set result.[[locale]] to foundLocale.
    result.locale = foundLocale;
    UErrorCode status = U_ZERO_ERROR;
    foundLocaleData = localeBuilder.build(status);
    result.localeData = foundLocaleData;

    // 11. Return result.
    return result;
}

icu::Locale JSLocale::BuildICULocale(const std::string &bcp47Locale)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale icuLocale = icu::Locale::forLanguageTag(bcp47Locale, status);
    ASSERT_PRINT(U_SUCCESS(status), "forLanguageTag failed");
    ASSERT_PRINT(!icuLocale.isBogus(), "icuLocale is bogus");
    return icuLocale;
}

JSHandle<TaggedArray> JSLocale::ConstructLocaleList(JSThread *thread,
                                                    const std::vector<std::string> &icuAvailableLocales)
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

JSHandle<EcmaString> JSLocale::IcuToString(JSThread *thread, const icu::UnicodeString &string)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(string.getBuffer()), string.length());
}

JSHandle<EcmaString> JSLocale::IcuToString(JSThread *thread, const icu::UnicodeString &string, int32_t begin,
                                           int32_t end)
{
    return IcuToString(thread, string.tempSubStringBetween(begin, end));
}

std::string JSLocale::GetNumberingSystem(const icu::Locale &icuLocale)
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::NumberingSystem> numberingSystem(icu::NumberingSystem::createInstance(icuLocale, status));
    if (U_SUCCESS(status) != 0) {
        return numberingSystem->getName();
    }
    return LATN_STRING;
}

bool JSLocale::IsWellFormedCurrencyCode(const std::string &currency)
{
    if (currency.length() != INTL_INDEX_THREE) {
        return false;
    }
    return (IsAToZ(currency[INTL_INDEX_ZERO]) && IsAToZ(currency[INTL_INDEX_ONE]) && IsAToZ(currency[INTL_INDEX_TWO]));
}

JSHandle<JSObject> JSLocale::PutElement(JSThread *thread, int index, const JSHandle<JSArray> &array,
                                        const JSHandle<JSTaggedValue> &fieldTypeString,
                                        const JSHandle<JSTaggedValue> &value)
{
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // Let record be ! ObjectCreate(%ObjectPrototype%).
    JSHandle<JSObject> record = factory->NewEmptyJSObject();

    auto globalConst = thread->GlobalConstants();
    // obj.type = field_type_string
    JSObject::CreateDataPropertyOrThrow(thread, record, globalConst->GetHandledTypeString(), fieldTypeString);
    // obj.value = value
    JSObject::CreateDataPropertyOrThrow(thread, record, globalConst->GetHandledValueString(), value);

    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(array), index,
                               JSHandle<JSTaggedValue>::Cast(record), true);
    return record;
}

// 9.2.11 GetOption ( options, property, type, values, fallback )
bool JSLocale::GetOptionOfBool(JSThread *thread, const JSHandle<JSObject> &options,
                               const JSHandle<JSTaggedValue> &property, bool fallback, bool *res)
{
    // 1. Let value be ? Get(options, property).
    OperationResult operationResult = JSObject::GetProperty(thread, options, property);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<JSTaggedValue> value = operationResult.GetValue();
    *res = fallback;
    // 2. If value is not undefined, then
    if (!value->IsUndefined()) {
        // b. Let value be ToBoolean(value).
        *res = value->ToBoolean();
        return true;
    }
    // 3. not found
    return false;
}

JSHandle<JSTaggedValue> JSLocale::GetNumberFieldType(JSThread *thread, JSTaggedValue x, int32_t fieldId)
{
    ASSERT(x.IsNumber());
    double number = 0;
    auto globalConst = thread->GlobalConstants();
    if (static_cast<UNumberFormatFields>(fieldId) == UNUM_INTEGER_FIELD) {
        number = x.GetNumber();
        if (std::isfinite(number)) {
            return globalConst->GetHandledIntegerString();
        }
        if (std::isnan(number)) {
            return globalConst->GetHandledNanString();
        }
        return globalConst->GetHandledInfinityString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_FRACTION_FIELD) {
        return globalConst->GetHandledFractionString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_DECIMAL_SEPARATOR_FIELD) {
        return globalConst->GetHandledDecimalString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_GROUPING_SEPARATOR_FIELD) {
        return globalConst->GetHandledGroupString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_CURRENCY_FIELD) {
        return globalConst->GetHandledCurrencyString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_PERCENT_FIELD) {
        return globalConst->GetHandledPercentSignString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_SIGN_FIELD) {
        number = x.GetNumber();
        return std::signbit(number) ? globalConst->GetHandledMinusSignString()
                                    : globalConst->GetHandledPlusSignString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_EXPONENT_SYMBOL_FIELD) {
        return globalConst->GetHandledExponentSeparatorString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_EXPONENT_SIGN_FIELD) {
        return globalConst->GetHandledExponentMinusSignString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_EXPONENT_FIELD) {
        return globalConst->GetHandledExponentIntegerString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_COMPACT_FIELD) {
        return globalConst->GetHandledCompactString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_MEASURE_UNIT_FIELD) {
        return globalConst->GetHandledUnitString();
    } else {
        UNREACHABLE();
    }
}

// 10.1.1 ApplyOptionsToTag( tag, options )
bool JSLocale::ApplyOptionsToTag(JSThread *thread, const JSHandle<EcmaString> &tag, const JSHandle<JSObject> &options,
                                 TagElements &tagElements)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    if (*tag == *(factory->GetEmptyString())) {
        return false;
    }
    // 2. If IsStructurallyValidLanguageTag(tag) is false, throw a RangeError exception.
    if (!IsStructurallyValidLanguageTag(tag)) {
        return false;
    }

    tagElements.language =
        GetOption(thread, options, globalConst->GetHandledLanguageString(), OptionType::STRING,
                  globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 4. If language is not undefined, then
    //    a. If language does not match the unicode_language_subtag production, throw a RangeError exception.
    if (!tagElements.language->IsUndefined()) {
        std::string languageStr = ConvertToStdString(JSHandle<EcmaString>::Cast(tagElements.language));
        if (languageStr[INTL_INDEX_ZERO] == '\0' ||
            IsAlpha(languageStr, INTL_INDEX_FOUR, INTL_INDEX_FOUR)) {
            return false;
        }
    }

    // 5. Let script be ? GetOption(options, "script", "string", undefined, undefined).
    tagElements.script =
        GetOption(thread, options, globalConst->GetHandledScriptString(), OptionType::STRING,
                  globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 6. If script is not undefined, then
    //    a. If script does not match the unicode_script_subtag production, throw a RangeError exception.
    if (!tagElements.script->IsUndefined()) {
        std::string scriptStr = JSLocale::ConvertToStdString((JSHandle<EcmaString>::Cast(tagElements.script)));
        if (scriptStr[INTL_INDEX_ZERO] == '\0') {
            return false;
        }
    }

    // 7. Let region be ? GetOption(options, "region", "string", undefined, undefined).
    // 8. If region is not undefined, then
    //    a. If region does not match the unicode_region_subtag production, throw a RangeError exception.
    tagElements.region =
        GetOption(thread, options, globalConst->GetHandledRegionString(), OptionType::STRING,
                  globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    if (!tagElements.region->IsUndefined()) {
        std::string regionStr = ConvertToStdString(JSHandle<EcmaString>::Cast(tagElements.region));
        if (regionStr[INTL_INDEX_ZERO] == '\0') {
            return false;
        }
    }
    return true;
}

bool BuildOptionsTags(const JSHandle<EcmaString> &tag, icu::LocaleBuilder *builder, JSHandle<JSTaggedValue> language,
                      JSHandle<JSTaggedValue> script, JSHandle<JSTaggedValue> region)
{
    std::string tagStr = JSLocale::ConvertToStdString(tag);
    int32_t len = static_cast<int32_t>(tagStr.length());
    ASSERT(len > 0);
    builder->setLanguageTag({ tagStr.c_str(), len });
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale locale = builder->build(status);
    locale.canonicalize(status);
    if (U_FAILURE(status) != 0) {
        return false;
    }
    builder->setLocale(locale);

    if (!language->IsUndefined()) {
        std::string languageStr = JSLocale::ConvertToStdString(JSHandle<EcmaString>::Cast(language));
        builder->setLanguage(languageStr);
        builder->build(status);
        if ((U_FAILURE(status) != 0)) {
            return false;
        }
    }

    if (!script->IsUndefined()) {
        std::string scriptStr = JSLocale::ConvertToStdString((JSHandle<EcmaString>::Cast(script)));
        builder->setScript(scriptStr);
        builder->build(status);
        if ((U_FAILURE(status) != 0)) {
            return false;
        }
    }

    if (!region->IsUndefined()) {
        std::string regionStr = JSLocale::ConvertToStdString(JSHandle<EcmaString>::Cast(region));
        builder->setRegion(regionStr);
        builder->build(status);
        if ((U_FAILURE(status) != 0)) {
            return false;
        }
    }
    return true;
}

bool InsertOptions(JSThread *thread, const JSHandle<JSObject> &options, icu::LocaleBuilder *builder)
{
    const std::vector<std::string> hourCycleValues = {"h11", "h12", "h23", "h24"};
    const std::vector<std::string> caseFirstValues = {"upper", "lower", "false"};
    const std::vector<std::string> emptyValues = {};
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    std::string strResult;
    bool findca =
        JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledCalendarString(), emptyValues, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findca) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("ca"), strResult.c_str())) {
            return false;
        }
        builder->setUnicodeLocaleKeyword("ca", strResult.c_str());
    }

    bool findco =
        JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledCollationString(), emptyValues, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findco) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("co"), strResult.c_str())) {
            return false;
        }
        builder->setUnicodeLocaleKeyword("co", strResult.c_str());
    }

    bool findhc = JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledHourCycleString(),
                                              hourCycleValues, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findhc) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("hc"), strResult.c_str())) {
            return false;
        }
        builder->setUnicodeLocaleKeyword("hc", strResult.c_str());
    }

    bool findkf = JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledCaseFirstString(),
                                              caseFirstValues, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findkf) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("kf"), strResult.c_str())) {
            return false;
        }
        builder->setUnicodeLocaleKeyword("kf", strResult.c_str());
    }

    bool boolResult = false;
    bool findkn =
        JSLocale::GetOptionOfBool(thread, options, globalConst->GetHandledNumericString(), false, &boolResult);
    if (findkn) {
        strResult = boolResult ? "true" : "false";
        if (!uloc_toLegacyType(uloc_toLegacyKey("kn"), strResult.c_str())) {
            return false;
        }
        builder->setUnicodeLocaleKeyword("kn", strResult.c_str());
    }

    bool findnu =
        JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledNumberingSystemString(), emptyValues,
                                    &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findnu) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("nu"), strResult.c_str())) {
            return false;
        }
        builder->setUnicodeLocaleKeyword("nu", strResult.c_str());
    }
    return true;
}

JSHandle<JSLocale> JSLocale::InitializeLocale(JSThread *thread, const JSHandle<JSLocale> &locale,
                                              const JSHandle<EcmaString> &localeString,
                                              const JSHandle<JSObject> &options)
{
    icu::LocaleBuilder builder;
    TagElements tagElements;
    if (!ApplyOptionsToTag(thread, localeString, options, tagElements)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "apply option to tag failed", locale);
    }

    bool res = BuildOptionsTags(localeString, &builder, tagElements.language, tagElements.script, tagElements.region);
    if (!res) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "apply option to tag failed", locale);
    }
    bool insertResult = InsertOptions(thread, options, &builder);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, locale);
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale icuLocale = builder.build(status);
    icuLocale.canonicalize(status);

    if (!insertResult || (U_FAILURE(status) != 0)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "insert or build failed", locale);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    factory->NewJSIntlIcuData(locale, icuLocale, JSLocale::FreeIcuLocale);
    return locale;
}

bool JSLocale::DealwithLanguageTag(const std::vector<std::string> &containers, size_t &address)
{
    // The abstract operation returns true if locale can be generated from the ABNF grammar in section 2.1 of the RFC,
    // starting with Language-Tag, and does not contain duplicate variant or singleton subtags
    // If language tag is empty, return false.
    if (containers.empty()) {
        return false;
    }

    // a. if the first tag is not language, return false.
    if (!IsLanguageSubtag(containers[0])) {
        return false;
    }

    // if the tag include language only, like "zh" or "de", return true;
    if (containers.size() == 1) {
        return true;
    }

    // Else, then
    // if is unique singleton subtag, script and region tag.
    if (IsExtensionSingleton(containers[1])) {
        return true;
    }

    if (IsScriptSubtag(containers[address])) {
        address++;
        if (containers.size() == address) {
            return true;
        }
    }

    if (IsRegionSubtag(containers[address])) {
        address++;
    }

    for (size_t i = address; i < containers.size(); i++) {
        if (IsExtensionSingleton(containers[i])) {
            return true;
        }
        if (!IsVariantSubtag(containers[i])) {
            return false;
        }
    }
    return true;
}

int ConvertValue(const UErrorCode &status, std::string &value, const std::string &key)
{
    if (status == U_ILLEGAL_ARGUMENT_ERROR || value.empty()) {
        return 1;
    }

    if (value == "yes") {
        value = "true";
    }

    if (key == "kf" && value == "true") {
        return 2;
    }
    return 0;
}

JSHandle<EcmaString> JSLocale::NormalizeKeywordValue(JSThread *thread, const JSHandle<JSLocale> &locale,
                                                     const std::string &key)
{
    icu::Locale *icuLocale = locale->GetIcuLocale();
    UErrorCode status = U_ZERO_ERROR;
    auto value = icuLocale->getUnicodeKeywordValue<std::string>(key, status);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    int result = ConvertValue(status, value, key);
    if (result == 1) {
        return JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledUndefinedString());
    }
    if (result == 2) {
        return factory->GetEmptyString();
    }
    return factory->NewFromStdString(value);
}

JSHandle<EcmaString> JSLocale::ToString(JSThread *thread, const JSHandle<JSLocale> &locale)
{
    icu::Locale *icuLocale = locale->GetIcuLocale();
    if (icuLocale == nullptr) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->GetEmptyString();
    }
    JSHandle<EcmaString> result = ToLanguageTag(thread, *icuLocale);
    return result;
}

JSHandle<TaggedArray> JSLocale::GetAvailableLocales(JSThread *thread, const char *localeKey, const char *localePath)
{
    UErrorCode status = U_ZERO_ERROR;
    auto globalConst = thread->GlobalConstants();
    JSHandle<EcmaString> specialValue = JSHandle<EcmaString>::Cast(globalConst->GetHandledEnUsPosixString());
    std::string specialString = ConvertToStdString(specialValue);
    UEnumeration *uenum = uloc_openAvailableByType(ULOC_AVAILABLE_WITH_LEGACY_ALIASES, &status);
    std::vector<std::string> allLocales;
    const char *loc = nullptr;
    for (loc = uenum_next(uenum, nullptr, &status); loc != nullptr; loc = uenum_next(uenum, nullptr, &status)) {
        ASSERT(U_SUCCESS(status));
        std::string locStr(loc);
        std::replace(locStr.begin(), locStr.end(), '_', '-');
        if (locStr == specialString) {
            locStr = "en-US-u-va-posix";
        }

        if (localePath != nullptr || localeKey != nullptr) {
            icu::Locale locale(locStr.c_str());
            bool res = false;
            if (!CheckLocales(locale, localeKey, localePath, res)) {
                continue;
            }
        }
        bool isScript = false;
        allLocales.push_back(locStr);
        icu::Locale formalLocale = icu::Locale::createCanonical(locStr.c_str());
        std::string scriptStr = formalLocale.getScript();
        isScript = scriptStr.empty() ? false : true;
        if (isScript) {
            std::string languageStr = formalLocale.getLanguage();
            std::string countryStr = formalLocale.getCountry();
            std::string shortLocale = icu::Locale(languageStr.c_str(), countryStr.c_str()).getName();
            std::replace(shortLocale.begin(), shortLocale.end(), '_', '-');
            allLocales.push_back(shortLocale);
        }
    }
    uenum_close(uenum);
    return ConstructLocaleList(thread, allLocales);
}
}  // namespace panda::ecmascript