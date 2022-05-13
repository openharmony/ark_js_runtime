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

#include "ecmascript/builtins/builtins_number.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda::ecmascript::builtins {
using NumberHelper = base::NumberHelper;

JSTaggedValue BuiltinsNumber::NumberConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If value is present, then a , b , c.
    // 2. Else Let n be +0𝔽.
    JSTaggedNumber numberValue(0);
    if (argv->GetArgsNumber() > 0) {
        JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
        // a. Let prim be ? ToNumeric(value).
        JSHandle<JSTaggedValue> numericVal = JSTaggedValue::ToNumeric(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // b. If Type(prim) is BigInt, let n be 𝔽(ℝ(prim)).
        if (numericVal->IsBigInt()) {
            JSHandle<BigInt> bigNumericVal(numericVal);
            numberValue = BigInt::BigIntToNumber(bigNumericVal);
        } else {
            // c. Otherwise, let n be prim.
            numberValue = JSTaggedNumber(numericVal.GetTaggedValue());
        }
    }
    // 3. If NewTarget is undefined, return n.
    if (newTarget->IsUndefined()) {
        return numberValue;
    }
    // 4. Let O be OrdinaryCreateFromConstructor(NewTarget, "%NumberPrototype%", «[[NumberData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> result =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Set O.[[NumberData]] to n.
    JSPrimitiveRef::Cast(*result)->SetValue(thread, numberValue);
    // 6. Return O.
    return result.GetTaggedValue();
}

// 20.1.2.2
JSTaggedValue BuiltinsNumber::IsFinite(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, IsFinite);
    JSTaggedValue msg = GetCallArg(argv, 0).GetTaggedValue();
    // 1. If Type(number) is not Number, return false
    // 2. If number is NaN, +infinite, or -infinite, return false
    if (NumberHelper::IsFinite(msg)) {
        return GetTaggedBoolean(true);
    }
    return GetTaggedBoolean(false);
}

// 20.1.2.3
JSTaggedValue BuiltinsNumber::IsInteger(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, IsInteger);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    bool result = false;
    // 1. If Type(number) is not Number, return false.
    // 2. If number is NaN, +infinite, or -infinite, return false
    if (NumberHelper::IsFinite(msg.GetTaggedValue())) {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        double value = JSTaggedNumber(msg.GetTaggedValue()).GetNumber();
        // 3. Let integer be ToInteger(number).
        JSTaggedNumber number = JSTaggedValue::ToInteger(thread, msg);
        // 4. If integer is not equal to number, return false.
        // 5. Otherwise, return true.
        result = (value == number.GetNumber());
    }
    return GetTaggedBoolean(result);
}

// 20.1.2.4
JSTaggedValue BuiltinsNumber::IsNaN(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, IsNaN);
    JSTaggedValue msg = GetCallArg(argv, 0).GetTaggedValue();
    // 1. If Type(number) is not Number, return false.
    // 2. If number is NaN, return true.
    if (NumberHelper::IsNaN(msg)) {
        return GetTaggedBoolean(true);
    }
    // 3. Otherwise, return false.
    return GetTaggedBoolean(false);
}

// 20.1.2.5
JSTaggedValue BuiltinsNumber::IsSafeInteger(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, IsSafeInteger);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    bool result = false;
    // 1. If Type(number) is not Number, return false.
    // 2. If number is NaN, +infinite, or -infinite, return false
    if (NumberHelper::IsFinite(msg.GetTaggedValue())) {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        double value = JSTaggedNumber(msg.GetTaggedValue()).GetNumber();
        // 3. Let integer be ToInteger(number).
        JSTaggedNumber number = JSTaggedValue::ToInteger(thread, msg);
        // 4. If integer is not equal to number, return false.
        // 5. If abs(integer) ≤ 253−1, return true.
        result = (value == number.GetNumber()) && std::abs(value) <= base::MAX_SAFE_INTEGER;
    }
    return GetTaggedBoolean(result);
}

// 18.2.4
// 20.1.2.12
JSTaggedValue BuiltinsNumber::ParseFloat(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ParseFloat);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    if (msg->IsUndefined()) {
        return GetTaggedDouble(base::NAN_VALUE);
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let inputString be ToString(string).
    JSHandle<EcmaString> numberString = JSTaggedValue::ToString(thread, msg);
    // 2. ReturnIfAbrupt(inputString).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    Span<const uint8_t> str;
    if (UNLIKELY(numberString->IsUtf16())) {
        size_t len = base::utf_helper::Utf16ToUtf8Size(numberString->GetDataUtf16(), numberString->GetLength()) - 1;
        CVector<uint8_t> buf(len);
        len = base::utf_helper::ConvertRegionUtf16ToUtf8(numberString->GetDataUtf16(), buf.data(),
                                                         numberString->GetLength(), len, 0);
        str = Span<const uint8_t>(buf.data(), len);
    } else {
        str = Span<const uint8_t>(numberString->GetDataUtf8(), numberString->GetUtf8Length() - 1);
    }
    // 4. If neither trimmedString nor any prefix of trimmedString satisfies the syntax of a StrDecimalLiteral
    // (see 7.1.3.1), return NaN.
    if (NumberHelper::IsEmptyString(str.begin(), str.end())) {
        return BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    }
    double result = NumberHelper::StringToDouble(str.begin(), str.end(), 0, base::IGNORE_TRAILING);
    return GetTaggedDouble(result);
}

// 18.2.5
// 20.1.2.13
JSTaggedValue BuiltinsNumber::ParseInt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ParseInt);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> arg2 = GetCallArg(argv, 1);
    int32_t radix = 0;

    if (!arg2->IsUndefined()) {
        // 7. Let R = ToInt32(radix).
        radix = JSTaggedValue::ToInt32(thread, arg2);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 1. Let inputString be ToString(string).
    JSHandle<EcmaString> numberString = JSTaggedValue::ToString(thread, msg);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    Span<const uint8_t> str;
    if (UNLIKELY(numberString->IsUtf16())) {
        size_t len = base::utf_helper::Utf16ToUtf8Size(numberString->GetDataUtf16(), numberString->GetLength()) - 1;
        CVector<uint8_t> buf(len);
        len = base::utf_helper::ConvertRegionUtf16ToUtf8(numberString->GetDataUtf16(), buf.data(),
                                                         numberString->GetLength(), len, 0);
        str = Span<const uint8_t>(buf.data(), len);
    } else {
        str = Span<const uint8_t>(numberString->GetDataUtf8(), numberString->GetUtf8Length() - 1);
    }

    JSTaggedValue result = NumberHelper::StringToDoubleWithRadix(str.begin(), str.end(), radix);
    return result;
}

// prototype
// 20.1.3.2
JSTaggedValue BuiltinsNumber::ToExponential(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ToExponential);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? thisNumberValue(this value).
    JSTaggedNumber value = ThisNumberValue(argv);
    // 2. ReturnIfAbrupt(x).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let f be ToInteger(fractionDigits).
    JSHandle<JSTaggedValue> digits = GetCallArg(argv, 0);
    JSTaggedNumber digitInt = JSTaggedValue::ToInteger(thread, digits);
    // 5. ReturnIfAbrupt(f).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    double values = value.GetNumber();
    // 6. If x is NaN, return the String "NaN".
    if (std::isnan(values)) {
        return GetTaggedString(thread, "NaN");
    }
    // 8. If x < 0, then
    //    a. Let s be "-".
    //    b. Let x = –x.
    // 9. If x = +infinity, then
    //    a. Return the concatenation of the Strings s and "Infinity".
    if (!std::isfinite(values)) {
        if (values < 0) {
            return GetTaggedString(thread, "-Infinity");
        }
        return GetTaggedString(thread, "Infinity");
    }

    // 4. Assert: f is 0, when fractionDigits is undefined.
    // 10. If f < 0 or f > 20, throw a RangeError exception
    double fraction = digitInt.GetNumber();
    if (digits->IsUndefined()) {
        fraction = -1;
    } else {
        if (fraction < base::MIN_FRACTION || fraction > base::MAX_FRACTION) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "fraction must be 0 to 100", JSTaggedValue::Exception());
        }
    }
    return NumberHelper::DoubleToExponential(thread, values, static_cast<int>(fraction));
}

// 20.1.3.3
JSTaggedValue BuiltinsNumber::ToFixed(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ToFixed);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? thisNumberValue(this value).
    JSTaggedNumber value = ThisNumberValue(argv);
    // 2. ReturnIfAbrupt(x).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Let f be ToInteger(fractionDigits). (If fractionDigits is undefined, this step produces the value 0).
    JSHandle<JSTaggedValue> digitArgv = GetCallArg(argv, 0);
    JSTaggedNumber digitInt = JSTaggedValue::ToInteger(thread, digitArgv);
    if (digitArgv->IsUndefined()) {
        digitInt = JSTaggedNumber(0);
    }
    // 4. ReturnIfAbrupt(f).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    double digit = digitInt.GetNumber();
    if (digit < base::MIN_FRACTION || digit > base::MAX_FRACTION) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "fraction must be 0 to 100", JSTaggedValue::Exception());
    }

    // 6. If x is NaN, return the String "NaN".
    double valueNumber = value.GetNumber();
    if (std::isnan(valueNumber)) {
        return GetTaggedString(thread, "NaN");
    }
    // 9. If x  1021, then
    //    a. Let m = ToString(x).
    const double FIRST_NO_FIXED = 1e21;
    if (valueNumber >= FIRST_NO_FIXED) {
        return value.ToString(thread).GetTaggedValue();
    }

    return NumberHelper::DoubleToFixed(thread, valueNumber, static_cast<int>(digit));
}

// 20.1.3.4
JSTaggedValue BuiltinsNumber::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Number, ToLocaleString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? thisNumberValue(this value).
    JSTaggedNumber x = ThisNumberValue(argv);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let numberFormat be ? Construct(%NumberFormat%, « locales, options »).
    JSHandle<JSTaggedValue> ctor = thread->GetEcmaVM()->GetGlobalEnv()->GetNumberFormatFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor);
    JSHandle<JSNumberFormat> numberFormat = JSHandle<JSNumberFormat>::Cast(obj);
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSNumberFormat::InitializeNumberFormat(thread, numberFormat, locales, options);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Return ? FormatNumeric(numberFormat, x).
    JSHandle<JSTaggedValue> result = JSNumberFormat::FormatNumeric(thread, numberFormat, x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

// 20.1.3.5
JSTaggedValue BuiltinsNumber::ToPrecision(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ToPrecision);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? thisNumberValue(this value).
    JSTaggedNumber value = ThisNumberValue(argv);
    // 2. ReturnIfAbrupt(x).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If precision is undefined, return ToString(x).
    JSHandle<JSTaggedValue> digitArgv = GetCallArg(argv, 0);
    if (digitArgv->IsUndefined()) {
        return value.ToString(thread).GetTaggedValue();
    }
    // 4. Let p be ToInteger(precision).
    JSTaggedNumber digitInt = JSTaggedValue::ToInteger(thread, digitArgv);
    // 5. ReturnIfAbrupt(p).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. If x is NaN, return the String "NaN".
    double valueNumber = value.GetNumber();
    if (std::isnan(valueNumber)) {
        return GetTaggedString(thread, "NaN");
    }
    // 9. If x = +infinity, then
    //    a. Return the String that is the concatenation of s and "Infinity".
    if (!std::isfinite(valueNumber)) {
        if (valueNumber < 0) {
            return GetTaggedString(thread, "-Infinity");
        }
        return GetTaggedString(thread, "Infinity");
    }

    // If p < 1 or p > 21, throw a RangeError exception
    double digit = digitInt.GetNumber();
    if (digit < base::MIN_FRACTION + 1 || digit > base::MAX_FRACTION) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "fraction must be 1 to 100", JSTaggedValue::Exception());
    }
    return NumberHelper::DoubleToPrecision(thread, valueNumber, static_cast<int>(digit));
}

// 20.1.3.6
JSTaggedValue BuiltinsNumber::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? thisNumberValue(this value).
    JSTaggedNumber value = ThisNumberValue(argv);
    // 2. ReturnIfAbrupt(x).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If radix is not present, let radixNumber be 10.
    // 4. Else if radix is undefined, let radixNumber be 10.
    double radix = base::DECIMAL;
    JSHandle<JSTaggedValue> radixValue = GetCallArg(argv, 0);
    // 5. Else let radixNumber be ToInteger(radix).
    if (!radixValue->IsUndefined()) {
        JSTaggedNumber radixNumber = JSTaggedValue::ToInteger(thread, radixValue);
        // 6. ReturnIfAbrupt(x).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        radix = radixNumber.GetNumber();
    }

    // 7. If radixNumber < 2 or radixNumber > 36, throw a RangeError exception.
    if (radix < base::MIN_RADIX || radix > base::MAX_RADIX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "radix must be 2 to 36", JSTaggedValue::Exception());
    }
    // 8. If radixNumber = 10, return ToString(x).
    if (radix == base::DECIMAL) {
        return value.ToString(thread).GetTaggedValue();
    }

    double valueNumber = value.GetNumber();
    // If x is NaN, return the String "NaN".
    if (std::isnan(valueNumber)) {
        return GetTaggedString(thread, "NaN");
    }
    //  If x = +infinity, then
    //     Return the String that is the concatenation of s and "Infinity".
    if (!std::isfinite(valueNumber)) {
        if (valueNumber < 0) {
            return GetTaggedString(thread, "-Infinity");
        }
        return GetTaggedString(thread, "Infinity");
    }
    return NumberHelper::DoubleToString(thread, valueNumber, static_cast<int>(radix));
}

// 20.1.3.7
JSTaggedValue BuiltinsNumber::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Number, ValueOf);
    // 1. Let x be ? thisNumberValue(this value).
    JSTaggedValue x = ThisNumberValue(argv);
    JSThread *thread = argv->GetThread();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return x;
}

JSTaggedNumber BuiltinsNumber::ThisNumberValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Number, ThisNumberValue);
    JSHandle<JSTaggedValue> value = GetThis(argv);
    if (value->IsNumber()) {
        return JSTaggedNumber(value.GetTaggedValue());
    }
    if (value->IsJSPrimitiveRef()) {
        JSTaggedValue primitive = JSPrimitiveRef::Cast(value->GetTaggedObject())->GetValue();
        if (primitive.IsNumber()) {
            return JSTaggedNumber(primitive);
        }
    }
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "not number type", JSTaggedNumber::Exception());
}
}  // namespace panda::ecmascript::builtins
