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

#include "builtins_number_format.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
// 13.2.1 Intl.NumberFormat  ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsNumberFormat::NumberFormatConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If NewTarget is undefined, let newTarget be the active function object, else let newTarget be NewTarget.
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        newTarget = constructor;
    }

    // Let numberFormat be ? OrdinaryCreateFromConstructor(newTarget, "%NumberFormatPrototype%",
    // « [[InitializedNumberFormat]], [[Locale]], [[DataLocale]], [[NumberingSystem]], [[Style]], [[Unit]],
    // [[UnitDisplay]], [[Currency]], [[CurrencyDisplay]], [[CurrencySign]], [[MinimumIntegerDigits]],
    // [[MinimumFractionDigits]], [[MaximumFractionDigits]], [[MinimumSignificantDigits]], [[MaximumSignificantDigits]],
    // [[RoundingType]], [[Notation]], [[CompactDisplay]], [[UseGrouping]], [[SignDisplay]], [[BoundFormat]] »).
    JSHandle<JSNumberFormat> numberFormat =
        JSHandle<JSNumberFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Perform ? InitializeNumberFormat(numberFormat, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSNumberFormat::InitializeNumberFormat(thread, numberFormat, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. Let this be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 5. If NewTarget is undefined and Type(this) is Object and ? InstanceofOperator(this, %NumberFormat%) is true,
    //    then
    //    a. Perform ? DefinePropertyOrThrow(this, %Intl%.[[FallbackSymbol]], PropertyDescriptor{
    //       [[Value]]: numberFormat, [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: false }).
    //    b. Return this.
    bool isInstanceOf = JSObject::InstanceOf(thread, thisValue, env->GetNumberFormatFunction());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (newTarget->IsUndefined() && thisValue->IsJSObject() && isInstanceOf) {
        PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(numberFormat), false, false, false);
        JSHandle<JSTaggedValue> key(thread, JSHandle<JSIntl>::Cast(env->GetIntlFunction())->GetFallbackSymbol());
        JSTaggedValue::DefinePropertyOrThrow(thread, thisValue, key, descriptor);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return thisValue.GetTaggedValue();
    }

    // 6. Return numberFormat.
    return numberFormat.GetTaggedValue();
}

// 13.3.2 Intl.NumberFormat.supportedLocalesOf ( locales [ , options ] )
JSTaggedValue BuiltinsNumberFormat::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let availableLocales be %NumberFormat%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSNumberFormat::GetAvailableLocales(thread);

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

// 13.4.3 get Intl.NumberFormat.prototype.format
JSTaggedValue BuiltinsNumberFormat::Format(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let nf be this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. If Type(nf) is not Object, throw a TypeError exception.
    if (!thisValue->IsJSObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "nf is not object", JSTaggedValue::Exception());
    }
    // 3. Let nf be ? UnwrapNumberFormat(nf).
    JSHandle<JSTaggedValue> nf = JSNumberFormat::UnwrapNumberFormat(thread, thisValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (nf->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "nf is not object", JSTaggedValue::Exception());
    }

    JSHandle<JSNumberFormat> typpedNf = JSHandle<JSNumberFormat>::Cast(nf);
    JSHandle<JSTaggedValue> boundFunc(thread, typpedNf->GetBoundFormat());
    // 4. If nf.[[BoundFormat]] is undefined, then
    //      a. Let F be a new built-in function object as defined in Number Format Functions (12.1.4).
    //      b. Set F.[[NumberFormat]] to nf.
    //      c. Set nf.[[BoundFormat]] to F.
    if (boundFunc->IsUndefined()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSIntlBoundFunction> intlBoundFunc = factory->NewJSIntlBoundFunction(
            reinterpret_cast<void *>(BuiltinsNumberFormat::NumberFormatInternalFormatNumber));
        intlBoundFunc->SetNumberFormat(thread, typpedNf);
        typpedNf->SetBoundFormat(thread, intlBoundFunc);
    }
    return typpedNf->GetBoundFormat();
}

// 13.4.4 Intl.NumberFormat.prototype.formatToParts ( date )
JSTaggedValue BuiltinsNumberFormat::FormatToParts(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let nf be the this value.
    JSHandle<JSTaggedValue> nf = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(nf, [[InitializedNumberFormat]]).
    if (!nf->IsJSNumberFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "", JSTaggedValue::Exception());
    }
    // 3. Let x be ? ToNumeric(value).
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSTaggedNumber x = JSTaggedValue::ToNumber(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSArray> result = JSNumberFormat::FormatNumericToParts(thread, JSHandle<JSNumberFormat>::Cast(nf), x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result.GetTaggedValue();
}
// 13.4.5 Intl.NumberFormat.prototype.resolvedOptions ()
JSTaggedValue BuiltinsNumberFormat::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let nf be this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. If Type(nf) is not Object, throw a TypeError exception.
    if (!thisValue->IsJSObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not object", JSTaggedValue::Exception());
    }
    // 3. Let nf be ? UnwrapNumberFormat(nf).
    JSHandle<JSTaggedValue> nf = JSNumberFormat::UnwrapNumberFormat(thread, thisValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Let options be ! ObjectCreate(%ObjectPrototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetObjectFunction();
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    // 5. For each row of Table 5, except the header row, in table order, do
    //  Let p be the Property value of the current row.
    //  Let v be the value of nf's internal slot whose name is the Internal Slot value of the current row.
    //  If v is not undefined, then
    //  Perform ! CreateDataPropertyOrThrow(options, p, v).
    JSNumberFormat::ResolvedOptions(thread, JSHandle<JSNumberFormat>::Cast(nf), options);
    return options.GetTaggedValue();
}

JSTaggedValue BuiltinsNumberFormat::NumberFormatInternalFormatNumber(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSIntlBoundFunction> intlBoundFunc = JSHandle<JSIntlBoundFunction>::Cast(GetConstructor(argv));

    // 1. Let nf be F.[[NumberFormat]].
    JSHandle<JSTaggedValue> nf(thread, intlBoundFunc->GetNumberFormat());
    // 2. Assert: Type(nf) is Object and nf has an [[InitializedNumberFormat]] internal slot.
    ASSERT(nf->IsJSObject() && nf->IsJSNumberFormat());
    // 3. If value is not provided, let value be undefined.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    // 4 Let x be ? ToNumeric(value).
    JSTaggedNumber x = JSTaggedValue::ToNumber(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5 Return ? FormatNumeric(nf, x).
    JSHandle<JSTaggedValue> result = JSNumberFormat::FormatNumeric(thread, JSHandle<JSNumberFormat>::Cast(nf), x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins