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
 
#include "builtins_list_format.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsListFormat::ListFormatConstructor(EcmaRuntimeCallInfo *argv)
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

    // 2. Let listFormat be ? OrdinaryCreateFromConstructor
    // (NewTarget, "%ListFormat.prototype%", « [[InitializedListFormat]], [[Locale]],
    // [[Type]], [[Style]], [[Templates]] »).

    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSListFormat> listFormat = JSHandle<JSListFormat>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Perform ? InitializeListFormat(listFormat, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    listFormat = JSListFormat::InitializeListFormat(thread, listFormat, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return listFormat.GetTaggedValue();
}

// 13.3.2 Intl.ListFormat.supportedLocalesOf ( locales [ , options ] )
JSTaggedValue BuiltinsListFormat::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let availableLocales be %ListFormat%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSListFormat::GetAvailableLocales(thread);

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

// 13.4.3 Intl.ListFormat.prototype.format( list )
JSTaggedValue BuiltinsListFormat::Format(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let lf be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(lf, [[InitializedListFormat]]).
    if (!thisValue->IsJSListFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not JSListFormat", JSTaggedValue::Exception());
    }

    // 3. Let stringList be ? StringListFromIterable(list).
    JSHandle<JSTaggedValue> list = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> listArray = JSListFormat::StringListFromIterable(thread, list);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. Return FormatList(lf, stringList).
    JSHandle<JSListFormat> listFormat = JSHandle<JSListFormat>::Cast(thisValue);
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(listArray);
    JSHandle<EcmaString> result = JSListFormat::FormatList(thread, listFormat, array);
    return result.GetTaggedValue();
}

// 13.4.4 Intl.ListFormat.prototype.formatToParts ( list )
JSTaggedValue BuiltinsListFormat::FormatToParts(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let lf be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(lf, [[InitializedListFormat]]).
    if (!thisValue->IsJSListFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not JSListFormat", JSTaggedValue::Exception());
    }

    // 3. Let stringList be ? StringListFromIterable(list).
    JSHandle<JSTaggedValue> list = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> listArray = JSListFormat::StringListFromIterable(thread, list);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4 Return FormatListToParts(lf, stringList).
    JSHandle<JSListFormat> listFormat = JSHandle<JSListFormat>::Cast(thisValue);
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(listArray);

    JSHandle<JSArray> result = JSListFormat::FormatListToParts(thread, listFormat, array);
    return result.GetTaggedValue();
}

// 13.4.5 Intl.ListFormat.prototype.resolvedOptions()
JSTaggedValue BuiltinsListFormat::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1. Let lf be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(lf, [[InitializedListFormat]]).
    if (!thisValue->IsJSListFormat()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not JSListFormat", JSTaggedValue::Exception());
    }

    // 3 .Let options be ! OrdinaryObjectCreate(%Object.prototype%).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetObjectFunction();
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    // 4. For each row of Table 9, except the header row, in table order, do
    //  Let p be the Property value of the current row.
    //  Let v be the value of lf's internal slot whose name is the Internal Slot value of the current row.
    //  Assert: v is not undefined.
    //  Perform ! CreateDataPropertyOrThrow(options, p, v).
    JSHandle<JSListFormat> listFormat = JSHandle<JSListFormat>::Cast(thisValue);
    JSListFormat::ResolvedOptions(thread, listFormat, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return options.
    return options.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins