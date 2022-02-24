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

#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_bigint.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsBigInt::BigIntConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BigInt, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is not undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "BigInt is not a constructor", JSTaggedValue::Exception());
    }
    // 2. Let prim be ? ToPrimitive(value, number).
    JSHandle<JSTaggedValue> Primitive(thread, JSTaggedValue::ToPrimitive(thread, value));
    // 3. If Type(prim) is Number, return ? NumberToBigInt(prim).
    if (Primitive->IsNumber()) {
        return BigInt::NumberToBigInt(thread, Primitive);
    }
    // 4. Otherwise, return ? ToBigInt(value).
    return JSTaggedValue::ToBigInt(thread, value);
}

JSTaggedValue BuiltinsBigInt::AsUintN(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BigInt, AsUintN);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> bits = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> bigint = GetCallArg(argv, 1);
    // 1. Let bits be ? ToIndex(bits).
    JSTaggedNumber index = JSTaggedValue::ToIndex(thread, bits);
    // 2. Let bigint be ? ToBigInt(bigint).
    JSTaggedValue jsBigint = JSTaggedValue::ToBigInt(thread, bigint);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> jsBigintVal(thread, jsBigint);
    // 3. Return a BigInt representing bigint modulo 2bits.
    return BigInt::AsUintN(thread, index, jsBigintVal);
}

JSTaggedValue BuiltinsBigInt::AsIntN(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BigInt, AsIntN);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> bits = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> bigint = GetCallArg(argv, 1);
    // 1. Let bits be ? ToIndex(bits).
    JSTaggedNumber index = JSTaggedValue::ToIndex(thread, bits);
    // 2. Let bigint be ? ToBigInt(bigint).
    JSTaggedValue jsBigint = JSTaggedValue::ToBigInt(thread, bigint);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> jsBigintVal(thread, jsBigint);
    // 3. Let mod be ℝ(bigint) modulo 2bits.
    // 4. If mod ≥ 2bits - 1, return ℤ(mod - 2bits); otherwise, return ℤ(mod).
    return BigInt::AsintN(thread, index, jsBigintVal);
}

JSTaggedValue BuiltinsBigInt::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, ToLocaleString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? ThisBigIntValue(this value).
    JSTaggedValue value = ThisBigIntValue(argv);
    JSHandle<JSTaggedValue> thisVal(thread, value);
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
    JSHandle<JSTaggedValue> result = JSNumberFormat::FormatNumeric(thread, numberFormat, thisVal.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsBigInt::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, ToString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let x be ? thisBigIntValue(this value).
    JSTaggedValue value = ThisBigIntValue(argv);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> thisBigint(thread, value);
    // 2. If radix is not present, let radixNumber be 10
    double radix = base::DECIMAL;
    JSHandle<JSTaggedValue> radixValue = GetCallArg(argv, 0);
    // 3. Else, let radixNumber be ? ToIntegerOrInfinity(radix).
    if (!radixValue->IsUndefined()) {
        JSTaggedNumber radixNumber = JSTaggedValue::ToInteger(thread, radixValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        radix = radixNumber.GetNumber();
    }
    // 4. If radixNumber < 2 or radixNumber > 36, throw a RangeError exception.
    if (radix < base::MIN_RADIX || radix > base::MAX_RADIX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "toString() radix argument must be between 2 and 36",
                                     JSTaggedValue::Exception());
    }
    // 5. If radixNumber = 10, return ToString(x).
    if (radix == base::DECIMAL) {
        return BigInt::ToString(thread, thisBigint).GetTaggedValue();
    }
    // 6. Return the String representation of this Number value using the radix specified by radixNumber
    return BigInt::ToString(thread, thisBigint, static_cast<int>(radix)).GetTaggedValue();
}

JSTaggedValue BuiltinsBigInt::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BigInt, ValueOf);
    // 1. Let x be ? thisNumberValue(this value).
    return ThisBigIntValue(argv);
}

JSTaggedValue BuiltinsBigInt::ThisBigIntValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BigInt, ThisBigIntValue);
    JSHandle<JSTaggedValue> value = GetThis(argv);
    // 1. If Type(value) is BigInt, return value.
    if (value->IsBigInt()) {
        return value.GetTaggedValue();
    }
    // 2. If Type(value) is Object and value has a [[BigIntData]] internal slot, then
    if (value->IsJSPrimitiveRef()) {
        JSTaggedValue primitive = JSPrimitiveRef::Cast(value->GetTaggedObject())->GetValue();
        // a. Assert: Type(value.[[BigIntData]]) is BigInt.
        if (primitive.IsBigInt()) {
            // b. Return value.[[BigIntData]].
            return primitive;
        }
    }
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 3. Throw a TypeError exception.
    THROW_TYPE_ERROR_AND_RETURN(thread, "not BigInt type", JSTaggedValue::Exception());
}
}  // namespace panda::ecmascript::builtins