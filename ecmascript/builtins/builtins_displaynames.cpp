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

#include "builtins_displaynames.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
// 12.2.1 Intl.DisplayNames ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDisplayNames::DisplayNamesConstructor(EcmaRuntimeCallInfo *argv)
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

    // 2. Let displayNames be ? OrdinaryCreateFromConstructor(NewTarget, "%DisplayNames.prototype%",
    // « [[InitializedDisplayNames]], [[Locale]], [[Style]], [[Type]], [[Fallback]], [[Fields]] »).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSDisplayNames> displayNames =
        JSHandle<JSDisplayNames>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Perform ? InitializeDisplayNames(displayNames, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSDisplayNames::InitializeDisplayNames(thread, displayNames, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return displayNames.GetTaggedValue();
}

// 12.3.2 Intl.DisplayNames.supportedLocalesOf ( locales [ , options ] )
JSTaggedValue BuiltinsDisplayNames::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let availableLocales be %DisplayNames%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSDisplayNames::GetAvailableLocales(thread);

    // 2. Let requestedLocales be ? CanonicaliezLocaleList(locales).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<TaggedArray> requestedLocales = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Return ? SupportedLocales(availableLocales, requestedLocales, options).
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSArray> result = JSLocale::SupportedLocales(thread, availableLocales, requestedLocales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

// 12.4.3 get Intl.DisplayNames.prototype.of
JSTaggedValue BuiltinsDisplayNames::Of(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let displayNames be this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(displayNames, [[InitializedDisplayNames]]).
    if (!thisValue->IsJSDisplayNames()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not dn object", JSTaggedValue::Exception());
    }

    // 3. Let code be ? ToString(code).
    JSHandle<JSTaggedValue> codeValue = GetCallArg(argv, 0);
    JSHandle<EcmaString> codeTemp = JSTaggedValue::ToString(thread, codeValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. Let code be ? CanonicalCodeForDisplayNames(displayNames.[[Type]], code).
    // 5. Let fields be displayNames.[[Fields]].
    // 6. If fields has a field [[<code>]], return fields.[[<code>]].
    JSHandle<JSDisplayNames> displayNames = JSHandle<JSDisplayNames>::Cast(thisValue);
    TypednsOption typeOpt = displayNames->GetType();
    JSHandle<EcmaString> code = JSDisplayNames::CanonicalCodeForDisplayNames(thread, displayNames, typeOpt, codeTemp);
    std::string codeString = JSLocale::ConvertToStdString(code);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (codeString.size()) {
        JSHandle<JSTaggedValue> codeStr = JSHandle<JSTaggedValue>::Cast(code);
        return codeStr.GetTaggedValue();
    }

    // 7. If displayNames.[[Fallback]] is "code", return code.
    FallbackOption fallback = displayNames->GetFallback();
    if (fallback == FallbackOption::CODE) {
        return codeValue.GetTaggedValue();
    }
    // 8. Return undefined.
    return JSTaggedValue::Undefined();
}

// 12.4.4 Intl.DisplayNames.prototype.resolvedOptions ()
JSTaggedValue BuiltinsDisplayNames::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let DisplayNames be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(DisplayNames, [[InitializedDisplayNames]]).
    if (!thisValue->IsJSDisplayNames()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not dn object", JSTaggedValue::Exception());
    }

    // 3. Let options be ! OrdinaryObjectCreate(%ObjectPrototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetObjectFunction();
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    // 4. For each row of Table 8, except the header row, in table order, do
    // Let p be the Property value of the current row.
    // Let v be the value of displayNames's internal slot whose name is the Internal Slot value of the current row.
    // Assert: v is not undefined.
    // Perform ! CreateDataPropertyOrThrow(options, p, v).
    JSHandle<JSDisplayNames> displayNames = JSHandle<JSDisplayNames>::Cast(thisValue);
    JSDisplayNames::ResolvedOptions(thread, displayNames, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return options.
    return options.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins