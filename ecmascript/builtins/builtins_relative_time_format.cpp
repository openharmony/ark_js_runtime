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

#include "builtins_relative_time_format.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsRelativeTimeFormat::RelativeTimeFormatConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If NewTarget is undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newTarget is undefined", JSTaggedValue::Exception());
    }

    // 2. Let relativeTimeFormat be ? OrdinaryCreateFromConstructor
    // (NewTarget, "%RelativeTimeFormatPrototype%", « [[InitializedRelativeTimeFormat]],
    // [[Locale]], [[DataLocale]], [[Style]], [[Numeric]], [[NumberFormat]], [[NumberingSystem]], [[PluralRules]] »).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSRelativeTimeFormat> relativeTimeFormat = JSHandle<JSRelativeTimeFormat>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Perform ? InitializeRelativeTimeFormat(relativeTimeFormat, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSRelativeTimeFormat::InitializeRelativeTimeFormat(thread, relativeTimeFormat, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4.  Intl.RelativeTimeFormat.prototype[ @@toStringTag ]
    // This property has the attributes { [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: true }.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    bool isInstanceOf = JSObject::InstanceOf(thread, thisValue, env->GetRelativeTimeFormatFunction());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (newTarget->IsUndefined() && thisValue->IsJSObject() && isInstanceOf) {
        PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(relativeTimeFormat), false, false, true);
        JSHandle<JSTaggedValue> key(thread, JSHandle<JSIntl>::Cast(env->GetIntlFunction())->GetFallbackSymbol());
        JSTaggedValue::DefinePropertyOrThrow(thread, thisValue, key, descriptor);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return thisValue.GetTaggedValue();
    }

    return relativeTimeFormat.GetTaggedValue();
}

JSTaggedValue BuiltinsRelativeTimeFormat::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let availableLocales be %RelativeTimeFormat%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSLocale::GetAvailableLocales(thread, "calendar", nullptr);

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

JSTaggedValue BuiltinsRelativeTimeFormat::Format(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let relativeTimeFormat be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(relativeTimeFormat, [[InitializedRelativeTimeFormat]]).
    if (!thisValue->IsJSRelativeTimeFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not rtf object", JSTaggedValue::Exception());
    }

    // 3. Let value be ? ToNumber(value).
    double x = 0.0;
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSTaggedNumber temp = JSTaggedValue::ToNumber(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    x = temp.GetNumber();

    // 4. Let unit be ? ToString(unit).
    JSHandle<JSTaggedValue> unitValue = GetCallArg(argv, 1);
    JSHandle<EcmaString> unit = JSTaggedValue::ToString(thread, unitValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return ? FormatRelativeTime(relativeTimeFormat, value, unit).
    JSHandle<JSRelativeTimeFormat> relativeTimeFormat = JSHandle<JSRelativeTimeFormat>::Cast(thisValue);
    JSHandle<EcmaString> result = JSRelativeTimeFormat::Format(thread, x, unit, relativeTimeFormat);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsRelativeTimeFormat::FormatToParts(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let relativeTimeFormat be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(relativeTimeFormat, [[InitializedRelativeTimeFormat]]).
    if (!thisValue->IsJSRelativeTimeFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not rtf object", JSTaggedValue::Exception());
    }

    // 3. Let value be ? ToNumber(value).
    double x = 0.0;
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSTaggedNumber temp = JSTaggedValue::ToNumber(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    x = temp.GetNumber();

    // 4. Let unit be ? ToString(unit).
    JSHandle<JSTaggedValue> unitValue = GetCallArg(argv, 1);
    JSHandle<EcmaString> unit = JSTaggedValue::ToString(thread, unitValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return ? FormatRelativeTime(relativeTimeFormat, value, unit).
    JSHandle<JSRelativeTimeFormat> relativeTimeFormat = JSHandle<JSRelativeTimeFormat>::Cast(thisValue);
    JSHandle<JSArray> result = JSRelativeTimeFormat::FormatToParts(thread, x, unit, relativeTimeFormat);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsRelativeTimeFormat::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let relativeTimeFormat be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(relativeTimeFormat, [[InitializedRelativeTimeFormat]]).
    if (!thisValue->IsJSRelativeTimeFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not rtf object", JSTaggedValue::Exception());
    }

    // 3. Let options be ! ObjectCreate(%ObjectPrototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetObjectFunction();
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    // 4. perform resolvedOptions
    JSHandle<JSRelativeTimeFormat> relativeTimeFormat = JSHandle<JSRelativeTimeFormat>::Cast(thisValue);
    JSRelativeTimeFormat::ResolvedOptions(thread, relativeTimeFormat, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Return options.
    return options.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins