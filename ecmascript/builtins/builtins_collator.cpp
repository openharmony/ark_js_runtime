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

#include "builtins_collator.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/mem/barriers-inl.h"

namespace panda::ecmascript::builtins {
constexpr uint32_t FUNCTION_LENGTH_TWO = 2;

// 11.1.2 Intl.Collator ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsCollator::CollatorConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. If NewTarget is undefined, let newTarget be the active function object, else let newTarget be NewTarget.
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        newTarget = constructor;
    }
    // 2. Let internalSlotsList be « [[InitializedCollator]], [[Locale]], [[Usage]], [[Sensitivity]],
    //    [[IgnorePunctuation]], [[Collation]], [[BoundCompare]] ».
    // 3. If %Collator%.[[RelevantExtensionKeys]] contains "kn", then
    //    a. Append [[Numeric]] as the last element of internalSlotsList.
    // 4. If %Collator%.[[RelevantExtensionKeys]] contains "kf", then
    //    a. Append [[CaseFirst]] as the last element of internalSlotsList.

    // 5. Let collator be ? OrdinaryCreateFromConstructor(newTarget, "%CollatorPrototype%", internalSlotsList).
    JSHandle<JSCollator> collator =
        JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. Return ? InitializeCollator(collator, locales, options).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSCollator> result = JSCollator::InitializeCollator(thread, collator, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

// 11.2.2 Intl.Collator.supportedLocalesOf ( locales [ , options ] )
JSTaggedValue BuiltinsCollator::SupportedLocalesOf(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let availableLocales be %Collator%.[[AvailableLocales]].
    JSHandle<TaggedArray> availableLocales = JSCollator::GetAvailableLocales(thread);

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

// 11.3.3  get Intl.Collator.prototype.compare
JSTaggedValue BuiltinsCollator::Compare(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    // 1. Let collator be this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);

    // 2. Perform ? RequireInternalSlot(collator, [[InitializedCollator]]).
    if (!thisValue->IsJSCollator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not collator", JSTaggedValue::Exception());
    }
    // 3. If collator.[[BoundCompare]] is undefined, then
    //    a. Let F be a new built-in function object as defined in 11.3.3.1.
    //    b. Set F.[[Collator]] to collator.
    //    c. Set collator.[[BoundCompare]] to F.
    // 4. Return collator.[[BoundCompare]].
    JSHandle<JSCollator> collator = JSHandle<JSCollator>::Cast(thisValue);
    JSHandle<JSTaggedValue> boundCompare(thread, collator->GetBoundCompare());
    if (boundCompare->IsUndefined()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSIntlBoundFunction> intlBoundFunc = factory->NewJSIntlBoundFunction(
            MethodIndex::BUILTINS_COLLATOR_ANONYMOUS_COLLATOR, FUNCTION_LENGTH_TWO);
        intlBoundFunc->SetCollator(thread, collator);
        collator->SetBoundCompare(thread, intlBoundFunc);
    }
    return collator->GetBoundCompare();
}

// 11.3.3.1 Collator Compare Functions
JSTaggedValue BuiltinsCollator::AnonymousCollator(EcmaRuntimeCallInfo *argv)
{
    // A Collator compare function is an anonymous built-in function that has a [[Collator]] internal slot.
    // When a Collator compare function F is called with arguments x and y, the following steps are taken:
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSIntlBoundFunction> intlBoundFunc = JSHandle<JSIntlBoundFunction>::Cast(GetConstructor(argv));

    // 1. Let collator be F.[[Collator]].
    JSHandle<JSTaggedValue> collator(thread, intlBoundFunc->GetCollator());

    // 2. Assert: Type(collator) is Object and collator has an [[InitializedCollator]] internal slot.
    ASSERT_PRINT(collator->IsJSObject() && collator->IsJSCollator(), "collator is not object or JSCollator");

    // 3. If x is not provided, let x be undefined.
    JSHandle<JSTaggedValue> x = GetCallArg(argv, 0);

    // 4. If y is not provided, let y be undefined.
    JSHandle<JSTaggedValue> y = GetCallArg(argv, 1);

    // 5. Let X be ? ToString(x).
    JSHandle<EcmaString> xValue = JSTaggedValue::ToString(thread, x);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    // 6. Let Y be ? ToString(y).
    JSHandle<EcmaString> yValue = JSTaggedValue::ToString(thread, y);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    // 7. Return CompareStrings(collator, X, Y).
    icu::Collator *icuCollator = (JSHandle<JSCollator>::Cast(collator))->GetIcuCollator();
    return JSCollator::CompareStrings(icuCollator, xValue, yValue);
}

// 11.3.4 Intl.Collator.prototype.resolvedOptions ()
JSTaggedValue BuiltinsCollator::ResolvedOptions(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    if (!thisValue->IsJSCollator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Collator object", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> options = JSCollator::ResolvedOptions(thread, JSHandle<JSCollator>::Cast(thisValue));
    return options.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins