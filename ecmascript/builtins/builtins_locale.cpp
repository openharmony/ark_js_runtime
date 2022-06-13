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

#include "ecmascript/builtins/builtins_locale.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
// 10.1.3 Intl.Locale( tag [, options] )
JSTaggedValue BuiltinsLocale::LocaleConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If NewTarget is undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newTarget is undefined", JSTaggedValue::Exception());
    }

    // 6. Let locale be ? OrdinaryCreateFromConstructor(NewTarget, %LocalePrototype%, internalSlotsList).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSLocale> locale =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. If Type(tag) is not String or Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> tag = GetCallArg(argv, 0);
    if (!tag->IsString() && !tag->IsJSObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "tag is not String or Object", JSTaggedValue::Exception());
    }

    // 8. If Type(tag) is Object and tag has an [[InitializedLocale]] internal slot, then
    //    a.Let tag be tag.[[Locale]].
    // 9. Else,
    //    a.Let tag be ? ToString(tag).
    JSHandle<EcmaString> localeString = factory->GetEmptyString();
    if (!tag->IsJSLocale()) {
        localeString = JSTaggedValue::ToString(thread, tag);
    } else {
        icu::Locale *icuLocale = (JSHandle<JSLocale>::Cast(tag))->GetIcuLocale();
        localeString = JSLocale::ToLanguageTag(thread, *icuLocale);
    }
    // 10. If options is undefined, then
    //    a.Let options be ! ObjectCreate(null).
    // 11. Else
    //    a.Let options be ? ToObject(options).
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSObject> optionsObj;
    if (options->IsUndefined()) {
        optionsObj = factory->CreateNullJSObject();
    } else {
        optionsObj = JSTaggedValue::ToObject(thread, options);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSLocale> result = JSLocale::InitializeLocale(thread, locale, localeString, optionsObj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::Maximize(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Let maximal be the result of the Add Likely Subtags algorithm applied to loc.[[Locale]]. If an error is
    //    signaled, set maximal to loc.[[Locale]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    icu::Locale source(*(locale->GetIcuLocale()));
    UErrorCode status = U_ZERO_ERROR;
    source.addLikelySubtags(status);
    ASSERT(U_SUCCESS(status));
    ASSERT(!source.isBogus());

    // 4. Return ! Construct(%Locale%, maximal).
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> ctor = env->GetLocaleFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor);
    factory->NewJSIntlIcuData(JSHandle<JSLocale>::Cast(obj), source, JSLocale::FreeIcuLocale);
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::Minimize(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }

    // 3. Let minimal be the result of the Remove Likely Subtags algorithm applied to loc.[[Locale]].
    //    If an error is signaled, set minimal to loc.[[Locale]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    icu::Locale source(*(locale->GetIcuLocale()));
    UErrorCode status = U_ZERO_ERROR;
    source.minimizeSubtags(status);
    ASSERT(U_SUCCESS(status));
    ASSERT(!source.isBogus());

    [[maybe_unused]] auto res = source.toLanguageTag<CString>(status);

    // 4. Return ! Construct(%Locale%, minimal).
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> ctor = env->GetLocaleFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor);
    factory->NewJSIntlIcuData(JSHandle<JSLocale>::Cast(obj), source, JSLocale::FreeIcuLocale);
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::ToString(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[Locale]].
    JSHandle<EcmaString> result = JSLocale::ToString(thread, JSHandle<JSLocale>::Cast(loc));
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetBaseName(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Let locale be loc.[[Locale]].
    // 4. Return the substring of locale corresponding to the unicode_language_id production.
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    icu::Locale icuLocale = icu::Locale::createFromName(locale->GetIcuLocale()->getBaseName());
    JSHandle<EcmaString> baseName = JSLocale::ToLanguageTag(thread, icuLocale);
    return baseName.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetCalendar(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[Calendar]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    JSHandle<EcmaString> calendar = JSLocale::NormalizeKeywordValue(thread, locale, "ca");
    return calendar.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetCaseFirst(EcmaRuntimeCallInfo *argv)
{
    // This property only exists if %Locale%.[[RelevantExtensionKeys]] contains "kf".
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[CaseFirst]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    JSHandle<EcmaString> caseFirst = JSLocale::NormalizeKeywordValue(thread, locale, "kf");
    return caseFirst.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetCollation(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[Collation]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    JSHandle<EcmaString> collation = JSLocale::NormalizeKeywordValue(thread, locale, "co");
    return collation.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetHourCycle(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[HourCycle]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    JSHandle<EcmaString> hourCycle = JSLocale::NormalizeKeywordValue(thread, locale, "hc");
    return hourCycle.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetNumeric(EcmaRuntimeCallInfo *argv)
{
    // This property only exists if %Locale%.[[RelevantExtensionKeys]] contains "kn".
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[Numeric]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    icu::Locale *icuLocale = locale->GetIcuLocale();
    UErrorCode status = U_ZERO_ERROR;
    auto numeric = icuLocale->getUnicodeKeywordValue<CString>("kn", status);
    JSTaggedValue result = (numeric == "true") ? JSTaggedValue::True() : JSTaggedValue::False();
    return result;
}

JSTaggedValue BuiltinsLocale::GetNumberingSystem(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Return loc.[[NumberingSystem]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    JSHandle<EcmaString> numberingSystem = JSLocale::NormalizeKeywordValue(thread, locale, "nu");
    return numberingSystem.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetLanguage(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Let locale be loc.[[Locale]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    // 4. Assert: locale matches the unicode_locale_id production.
    // 5. Return the substring of locale corresponding to the unicode_language_subtag production of the
    //    unicode_language_id.
    JSHandle<EcmaString> result = JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledUndefinedString());
    CString language = locale->GetIcuLocale()->getLanguage();
    if (language.empty()) {
        return result.GetTaggedValue();
    }
    result = factory->NewFromUtf8(language);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetScript(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Let locale be loc.[[Locale]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);

    // 4. Assert: locale matches the unicode_locale_id production.
    // 5. If the unicode_language_id production of locale does not contain the ["-" unicode_script_subtag] sequence,
    //    return undefined.
    // 6. Return the substring of locale corresponding to the unicode_script_subtag production of the
    //    unicode_language_id.
    JSHandle<EcmaString> result(thread, JSTaggedValue::Undefined());
    CString script = locale->GetIcuLocale()->getScript();
    if (script.empty()) {
        return result.GetTaggedValue();
    }
    result = factory->NewFromUtf8(script);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsLocale::GetRegion(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let loc be the this value.
    JSHandle<JSTaggedValue> loc = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(loc, [[InitializedLocale]]).
    if (!loc->IsJSLocale()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "not locale", JSTaggedValue::Exception());
    }
    // 3. Let locale be loc.[[Locale]].
    JSHandle<JSLocale> locale = JSHandle<JSLocale>::Cast(loc);
    // 4. Assert: locale matches the unicode_locale_id production.
    // 5. If the unicode_language_id production of locale does not contain the ["-" unicode_region_subtag] sequence,
    //    return undefined.
    // 6. Return the substring of locale corresponding to the unicode_region_subtag production of the
    //    unicode_language_id.
    CString region = locale->GetIcuLocale()->getCountry();
    if (region.empty()) {
        return globalConst->GetUndefined();
    }
    return factory->NewFromUtf8(region).GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
