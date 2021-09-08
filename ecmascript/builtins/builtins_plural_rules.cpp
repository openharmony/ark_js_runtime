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

#include "ecmascript/builtins/builtins_plural_rules.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsPluralRules::PluralRulesConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If NewTarget is undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newTarget is undefined", JSTaggedValue::Exception());
    }

    // 2. Let pluralRules be ? OrdinaryCreateFromConstructor(NewTarget, "%PluralRulesPrototype%",
    // « [[InitializedPluralRules]], [[Locale]], [[Type]], [[MinimumIntegerDigits]], [[MinimumFractionDigits]],
    // [[MaximumFractionDigits]], [[MinimumSignificantDigits]], [[MaximumSignificantDigits]], [[RoundingType]] »).
    JSHandle<JSPluralRules> pluralRules =
        JSHandle<JSPluralRules>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Return ? InitializePluralRules(pluralRules, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSPluralRules::InitializePluralRules(thread, pluralRules, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return pluralRules.GetTaggedValue();
}

JSTaggedValue BuiltinsPluralRules::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let availableLocales be %PluralRules%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSPluralRules::GetAvailableLocales(thread);

    // 2. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Return ? SupportedLocales(availableLocales, requestedLocales, options).
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSArray> result = JSLocale::SupportedLocales(thread, availableLocales, requestedLocales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsPluralRules::Select(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let pr be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(pr, [[InitializedPluralRules]]).
    if (!thisValue->IsJSPluralRules()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not pr object", JSTaggedValue::Exception());
    }

    // 3. Let n be ? ToNumber(value).
    double x = 0.0;
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSTaggedNumber temp = JSTaggedValue::ToNumber(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    x = temp.GetNumber();

    // 4. Return ? ResolvePlural(pr, n).
    JSHandle<JSPluralRules> pluralRules = JSHandle<JSPluralRules>::Cast(thisValue);
    JSHandle<EcmaString> result = JSPluralRules::ResolvePlural(thread, pluralRules, x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsPluralRules::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let thisValue be the this value;
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(pr, [[InitializedPluralRules]]).
    if (!thisValue->IsJSPluralRules()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not pr object", JSTaggedValue::Exception());
    }

    // 3. Let options be ! ObjectCreate(%ObjectPrototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetObjectFunction();
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    // 4. Perform resolvedOptions
    JSHandle<JSPluralRules> pluralRules = JSHandle<JSPluralRules>::Cast(thisValue);
    JSPluralRules::ResolvedOptions(thread, pluralRules, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return options.
    return options.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins