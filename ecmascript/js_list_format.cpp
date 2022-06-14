/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 
#include "ecmascript/js_list_format.h"
#include <cstring>
#include <vector>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_iterator.h"
#include "unicode/fieldpos.h"
#include "unicode/fpositer.h"
#include "unicode/formattedvalue.h"
#include "unicode/stringpiece.h"
#include "unicode/unistr.h"
#include "unicode/utf8.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"

namespace panda::ecmascript {
icu::ListFormatter *JSListFormat::GetIcuListFormatter() const
{
    ASSERT(GetIcuLF().IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetIcuLF().GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::ListFormatter *>(result);
}

void JSListFormat::FreeIcuListFormatter(void *pointer, [[maybe_unused]] void* hint)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuListFormat = reinterpret_cast<icu::ListFormatter *>(pointer);
    icuListFormat->~ListFormatter();
}

void JSListFormat::SetIcuListFormatter(JSThread *thread, const JSHandle<JSListFormat> listFormat,
                                       icu::ListFormatter *icuListFormatter, const DeleteEntryPoint &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    ASSERT(icuListFormatter != nullptr);
    JSTaggedValue data = listFormat->GetIcuLF();
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(icuListFormatter);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuListFormatter);
    pointer->SetDeleter(callback);
    listFormat->SetIcuLF(thread, pointer.GetTaggedValue());
}

JSHandle<TaggedArray> JSListFormat::GetAvailableLocales(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> listFormatLocales = env->GetListFormatLocales();
    if (!listFormatLocales->IsUndefined()) {
        return JSHandle<TaggedArray>::Cast(listFormatLocales);
    }
    const char *key = "listPattern";
    const char *path = nullptr;
    JSHandle<TaggedArray> availableLocales = JSLocale::GetAvailableLocales(thread, key, path);
    env->SetListFormatLocales(thread, availableLocales);
    return availableLocales;
}

// 13. InitializeListFormat ( listformat, locales, options )
JSHandle<JSListFormat> JSListFormat::InitializeListFormat(JSThread *thread,
                                                          const JSHandle<JSListFormat> &listFormat,
                                                          const JSHandle<JSTaggedValue> &locales,
                                                          const JSHandle<JSTaggedValue> &options)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    auto globalConst = thread->GlobalConstants();

    // 3. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSListFormat, thread);

    // 4. Let options be ? GetOptionsObject(options).
    JSHandle<JSObject> optionsObject;
    if (options->IsUndefined()) {
        optionsObject = factory->CreateNullJSObject();
    } else {
        optionsObject = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSListFormat, thread);
    }

    // 5. Let opt be a new Record.
    // 6. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleMatcherString();
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, optionsObject, property, {LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT},
        {"lookup", "best fit"}, LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSListFormat, thread);

    // 8. Let localeData be %ListFormat%.[[LocaleData]].
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = GetAvailableLocales(thread);
    }

    // 9. Let r be ResolveLocale(%ListFormat%.[[AvailableLocales]], requestedLocales,
    // opt, %ListFormat%.[[RelevantExtensionKeys]], localeData).
    std::set<std::string> relevantExtensionKeys {""};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSListFormat, thread);

    // 10. Set listFormat.[[Locale]] to r.[[locale]].
    icu::Locale icuLocale = r.localeData;
    JSHandle<EcmaString> localeStr = JSLocale::ToLanguageTag(thread, icuLocale);
    listFormat->SetLocale(thread, localeStr.GetTaggedValue());

    // 11. Let type be ? GetOption(options, "type", "string", « "conjunction", "disjunction", "unit" », "conjunction").
    property = globalConst->GetHandledTypeString();
    auto type = JSLocale::GetOptionOfString<ListTypeOption>(thread, optionsObject, property,
                                                            {ListTypeOption::CONJUNCTION, ListTypeOption::DISJUNCTION,
                                                            ListTypeOption::UNIT},
                                                            {"conjunction", "disjunction", "unit"},
                                                            ListTypeOption::CONJUNCTION);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSListFormat, thread);

    // 12. Set listFormat.[[Type]] to type.
    listFormat->SetType(type);

    // 13. Let style be ? GetOption(options, "style", "string", « "long", "short", "narrow" », "long").
    property = globalConst->GetHandledStyleString();
    auto style = JSLocale::GetOptionOfString<ListStyleOption>(thread, optionsObject, property,
                                                              {ListStyleOption::LONG, ListStyleOption::SHORT,
                                                              ListStyleOption::NARROW},
                                                              {"long", "short", "narrow"}, ListStyleOption::LONG);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSListFormat, thread);

    // 14. Set listFormat.[[Style]] to style.
    listFormat->SetStyle(style);

    // 15. Let dataLocale be r.[[dataLocale]].
    // 16. Let dataLocaleData be localeData.[[<dataLocale>]].
    // 17. Let dataLocaleTypes be dataLocaleData.[[<type>]].
    // 18. Set listFormat.[[Templates]] to dataLocaleTypes.[[<style>]].
    // 19. Return listFormat.

    // Trans typeOption to ICU type
    UListFormatterType uType;
    switch (type) {
        case ListTypeOption::CONJUNCTION:
            uType = ULISTFMT_TYPE_AND;
            break;
        case ListTypeOption::DISJUNCTION:
            uType = ULISTFMT_TYPE_OR;
            break;
        case ListTypeOption::UNIT:
            uType = ULISTFMT_TYPE_UNITS;
            break;
        default:
            UNREACHABLE();
    }

    // Trans StyleOption to ICU Style
    UListFormatterWidth uStyle;
    switch (style) {
        case ListStyleOption::LONG:
            uStyle = ULISTFMT_WIDTH_WIDE;
            break;
        case ListStyleOption::SHORT:
            uStyle = ULISTFMT_WIDTH_SHORT;
            break;
        case ListStyleOption::NARROW:
            uStyle = ULISTFMT_WIDTH_NARROW;
            break;
        default:
            UNREACHABLE();
    }
    UErrorCode status = U_ZERO_ERROR;
    icu::ListFormatter *icuListFormatter = icu::ListFormatter::createInstance(icuLocale, uType, uStyle, status);
    if (U_FAILURE(status) || icuListFormatter == nullptr) {
        delete icuListFormatter;
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu ListFormatter Error", listFormat);
    }
    SetIcuListFormatter(thread, listFormat, icuListFormatter, JSListFormat::FreeIcuListFormatter);
    return listFormat;
}

// 13.1.5 StringListFromIterable ( iterable )
JSHandle<JSTaggedValue> JSListFormat::StringListFromIterable(JSThread *thread, const JSHandle<JSTaggedValue> &iterable)
{
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSTaggedValue> arrayList = JSHandle<JSTaggedValue>::Cast(array);
    // 1. If iterable is undefined, then
    // a. Return a new empty List.
    if (iterable->IsUndefined()) {
        return arrayList;
    }
    // 2. Let iteratorRecord be ? GetIterator(iterable).
    JSHandle<JSTaggedValue> iteratorRecord(JSIterator::GetIterator(thread, iterable));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // 3. Let list be a new empty List.
    // 4. Let next be true.
    JSHandle<JSTaggedValue> next(thread, JSTaggedValue::True());
    // 5. Repeat, while next is not false,
    // a. Set next to ? IteratorStep(iteratorRecord).
    // b. If next is not false, then
    // i. Let nextValue be ? IteratorValue(next).
    // ii. If Type(nextValue) is not String, then
    // 1. Let error be ThrowCompletion(a newly created TypeError object).
    // 2. Return ? IteratorClose(iteratorRecord, error).
    // iii. Append nextValue to the end of the List list.
    uint32_t k = 0;
    while (!next->IsFalse()) {
        next = JSIterator::IteratorStep(thread, iteratorRecord);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        if (!next->IsFalse()) {
            JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            if (!nextValue->IsString()) {
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSObject> typeError = factory->GetJSError(ErrorType::TYPE_ERROR, "nextValue is not string");
                JSHandle<JSTaggedValue> error(
                    factory->NewCompletionRecord(CompletionRecordType::THROW, JSHandle<JSTaggedValue>(typeError)));
                JSTaggedValue result = JSIterator::IteratorClose(thread, iteratorRecord, error).GetTaggedValue();
                THROW_TYPE_ERROR_AND_RETURN(thread, "type error", JSHandle<JSTaggedValue>(thread, result));
            }
            JSArray::FastSetPropertyByValue(thread, arrayList, k, nextValue);
            k++;
        }
    }
    // 6. Return list.
    return arrayList;
}

namespace {
    std::vector<icu::UnicodeString> ToUnicodeStringArray(JSThread *thread, const JSHandle<JSArray> &array)
    {
        uint32_t length = array->GetArrayLength();
        std::vector<icu::UnicodeString> result;
        for (uint32_t k = 0; k < length; k++) {
            JSHandle<JSTaggedValue> listArray = JSHandle<JSTaggedValue>::Cast(array);
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, listArray, k);
            ASSERT(kValue->IsString());
            JSHandle<EcmaString> kValueString = JSTaggedValue::ToString(thread, kValue);
            std::string stdString = JSLocale::ConvertToStdString(kValueString);
            icu::StringPiece sp(stdString);
            icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sp);
            result.push_back(uString);
        }
        return result;
    }

    icu::FormattedList GetIcuFormatted(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                       const JSHandle<JSArray> &listArray)
    {
        icu::ListFormatter *icuListFormat = listFormat->GetIcuListFormatter();
        ASSERT(icuListFormat != nullptr);
        std::vector<icu::UnicodeString> usArray = ToUnicodeStringArray(thread, listArray);
        UErrorCode status = U_ZERO_ERROR;
        icu::FormattedList formatted = icuListFormat->formatStringsToValue(usArray.data(),
                                                                           static_cast<int32_t>(usArray.size()),
                                                                           status);
        return formatted;
    }

    void FormatListToArray(JSThread *thread, const icu::FormattedList &formatted, const JSHandle<JSArray> &receiver,
                           UErrorCode &status, icu::UnicodeString &listString)
    {
        icu::ConstrainedFieldPosition cfpo;
        cfpo.constrainCategory(UFIELD_CATEGORY_LIST);
        auto globalConst = thread->GlobalConstants();
        JSMutableHandle<JSTaggedValue> typeString(thread, JSTaggedValue::Undefined());
        int index = 0;
        while (formatted.nextPosition(cfpo, status) && U_SUCCESS(status)) {
            int32_t fieldId = cfpo.getField();
            int32_t start = cfpo.getStart();
            int32_t limit = cfpo.getLimit();
            if (static_cast<UListFormatterField>(fieldId) == ULISTFMT_ELEMENT_FIELD) {
                JSHandle<EcmaString> substring = JSLocale::IcuToString(thread, listString, start, limit);
                typeString.Update(globalConst->GetElementString());
                JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                index++;
            } else {
                JSHandle<EcmaString> substring = JSLocale::IcuToString(thread, listString, start, limit);
                typeString.Update(globalConst->GetLiteralString());
                JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                index++;
            }
        }
    }

    JSHandle<JSTaggedValue> ListOptionStyleToEcmaString(JSThread *thread, ListStyleOption style)
    {
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        auto globalConst = thread->GlobalConstants();
        switch (style) {
            case ListStyleOption::LONG:
                result.Update(globalConst->GetHandledLongString().GetTaggedValue());
                break;
            case ListStyleOption::SHORT:
                result.Update(globalConst->GetHandledShortString().GetTaggedValue());
                break;
            case ListStyleOption::NARROW:
                result.Update(globalConst->GetHandledNarrowString().GetTaggedValue());
                break;
            default:
                UNREACHABLE();
        }
        return result;
    }

    JSHandle<JSTaggedValue> ListOptionTypeToEcmaString(JSThread *thread, ListTypeOption type)
    {
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        auto globalConst = thread->GlobalConstants();
        switch (type) {
            case ListTypeOption::CONJUNCTION:
                result.Update(globalConst->GetHandledConjunctionString().GetTaggedValue());
                break;
            case ListTypeOption::DISJUNCTION:
                result.Update(globalConst->GetHandledDisjunctionString().GetTaggedValue());
                break;
            case ListTypeOption::UNIT:
                result.Update(globalConst->GetHandledUnitString().GetTaggedValue());
                break;
            default:
                UNREACHABLE();
        }
        return result;
    }
}

// 13.1.3 FormatList ( listFormat, list )
JSHandle<EcmaString> JSListFormat::FormatList(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                              const JSHandle<JSArray> &listArray)
{
    JSHandle<EcmaString> stringValue;
    UErrorCode status = U_ZERO_ERROR;
    icu::FormattedList formatted = GetIcuFormatted(thread, listFormat, listArray);
    if (U_FAILURE(status)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu listformat failed", stringValue);
    }
    icu::UnicodeString result = formatted.toString(status);
    if (U_FAILURE(status)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "formatted list toString failed", stringValue);
    }
    stringValue = JSLocale::IcuToString(thread, result);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, stringValue);
    // 4. Return result
    return stringValue;
}

// 13.1.4 FormatListToParts ( listFormat, list )
JSHandle<JSArray> JSListFormat::FormatListToParts(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                                  const JSHandle<JSArray> &listArray)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::FormattedList formatted = GetIcuFormatted(thread, listFormat, listArray);
    if (U_FAILURE(status)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu listformat failed", listArray);
    }
    icu::UnicodeString result = formatted.toString(status);
    if (U_FAILURE(status)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "formatted list toString failed", listArray);
    }
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    FormatListToArray(thread, formatted, array, status, result);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    return array;
}

void JSListFormat::ResolvedOptions(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                   const JSHandle<JSObject> &options)
{
    auto globalConst = thread->GlobalConstants();

    // [[Locale]]
    JSHandle<JSTaggedValue> propertyKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> locale(thread, listFormat->GetLocale());
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, locale);

    // [[type]]
    ListTypeOption type = listFormat->GetType();
    propertyKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> typeString = ListOptionTypeToEcmaString(thread, type);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, typeString);

    // [[Style]]
    ListStyleOption style = listFormat->GetStyle();
    propertyKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleString = ListOptionStyleToEcmaString(thread, style);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, styleString);
}
}  // namespace panda::ecmascript