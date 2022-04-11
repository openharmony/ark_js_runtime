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

#include "builtins_math.h"
#include <cmath>
#include <random>
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_number.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::builtins {
using NumberHelper = base::NumberHelper;

// 20.2.2.1
JSTaggedValue BuiltinsMath::Abs(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Abs);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    if (numberValue.IsDouble()) {
        // if number_value is double,NaN,Undefine, deal in this case
        // if number_value is a String ,which can change to double. e.g."100",deal in this case
        return GetTaggedDouble(std::fabs(numberValue.GetDouble()));
    }
    // if number_value is int,boolean,null, deal in this case
    return GetTaggedInt(std::abs(numberValue.GetInt()));
}

// 20.2.2.2
JSTaggedValue BuiltinsMath::Acos(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Acos);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // value == -NaN , <-1  or > 1,result is  NaN
    if (!std::isnan(std::abs(value)) && value <= 1 && value >= -1) {
        result = std::acos(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.3
JSTaggedValue BuiltinsMath::Acosh(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Acosh);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    if (value >= 1) {
        result = std::acosh(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.4
JSTaggedValue BuiltinsMath::Asin(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Asin);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    if (value >= -1 && value <= 1) {
        result = std::asin(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.5
JSTaggedValue BuiltinsMath::Asinh(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Asinh);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // value == -NaN, NaN, result is  NaN
    if (!std::isnan(std::abs(value))) {
        result = std::asinh(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.6
JSTaggedValue BuiltinsMath::Atan(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Atan);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // value == -NaN, NaN, result is  NaN
    if (!std::isnan(std::abs(value))) {
        result = std::atan(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.7
JSTaggedValue BuiltinsMath::Atanh(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Atanh);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    if (value >= -1 && value <= 1) {
        result = std::atanh(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.8
JSTaggedValue BuiltinsMath::Atan2(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Atan2);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msgY = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> msgX = GetCallArg(argv, 1);
    double result = base::NAN_VALUE;
    JSTaggedNumber numberValueY = JSTaggedValue::ToNumber(thread, msgY);
    JSTaggedNumber numberValueX = JSTaggedValue::ToNumber(thread, msgX);
    double valueY = numberValueY.GetNumber();
    double valueX = numberValueX.GetNumber();
    // y = +0 and x > +0, return +0
    // y = -0 and x > +0, return -0
    if (valueY == 0 && valueX > 0) {
        result = valueY;
    } else if (std::isfinite(valueY) && valueX == std::numeric_limits<double>::infinity()) {
        // y < 0 and y is finite and x is POSITIVE_INFINITY,return -0
        // y >= 0 and y is finite and x is POSITIVE_INFINITY,return +0
        result = valueY >= 0 ? 0 : -0.0;
    } else if (!std::isnan(std::abs(valueY)) && !std::isnan(std::abs(valueX))) {
        // If either x or y is NaN, the result is NaN
        result = std::atan2(valueY, valueX);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.9
JSTaggedValue BuiltinsMath::Cbrt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Cbrt);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // if value == -NaN, NaN, result is NaN
    if (!std::isnan(std::abs(value))) {
        result = std::cbrt(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.10
JSTaggedValue BuiltinsMath::Ceil(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Ceil);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN or -NaN, +infinite, -infinite,return value
    if (!std::isfinite(value)) {
        // if value is -NaN , return NaN, else return value
        if (!std::isnan(std::abs(value))) {
            result = value;
        }
    } else {
        result = std::ceil(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.11
JSTaggedValue BuiltinsMath::Clz32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Clz32);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    constexpr int defaultValue = 32;
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    auto tmpValue = std::abs(value);
    auto result = numberValue.ToUint32();
    if (!std::isfinite(tmpValue) || tmpValue == 0 || result == 0) {
        // If value is NaN or -NaN, +infinite, -infinite, 0,return 32
        return GetTaggedInt(defaultValue);
    }
    return GetTaggedInt(__builtin_clz(result));
}

// 20.2.2.12
JSTaggedValue BuiltinsMath::Cos(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Cos);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    //  If value is NaN or -NaN, +infinite, -infinite, result is NaN
    if (std::isfinite(std::abs(value))) {
        result = std::cos(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.13
JSTaggedValue BuiltinsMath::Cosh(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Cosh);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // if value is NaN or -NaN, result is NaN
    if (!std::isnan(std::abs(value))) {
        result = std::cosh(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.14
JSTaggedValue BuiltinsMath::Exp(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Exp);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // if value is NaN or -NaN, result is NaN
    if (!std::isnan(std::abs(value))) {
        result = std::exp(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.15
JSTaggedValue BuiltinsMath::Expm1(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Expm1);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // if value is NaN or -NaN, result is NaN
    if (!std::isnan(std::abs(value))) {
        result = std::expm1(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.16
JSTaggedValue BuiltinsMath::Floor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Floor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN or -NaN, +infinite, -infinite, +0, -0, return value
    if (!std::isfinite(value) || value == 0) {
        // If value is -NaN, return NaN, else return value
        if (!std::isnan(std::abs(value))) {
            result = value;
        }
    } else if (value > 0 && value < 1) {
        // If x is greater than 0 but less than 1, the result is +0
        result = 0;
    } else {
        result = std::floor(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.17
JSTaggedValue BuiltinsMath::Fround(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Fround);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result;
    if (std::isnan(std::abs(value))) {
        // If result is NaN or -NaN, the result is NaN
        result = base::NAN_VALUE;
    } else {
        result = static_cast<float>(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.18
JSTaggedValue BuiltinsMath::Hypot(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Hypot);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    double result = 0;
    double value = 0;
    uint32_t argLen = argv->GetArgsNumber();
    auto numberValue = JSTaggedNumber(0);
    for (uint32_t i = 0; i < argLen; i++) {
        JSHandle<JSTaggedValue> msg = GetCallArg(argv, i);
        numberValue = JSTaggedValue::ToNumber(thread, msg);
        value = numberValue.GetNumber();
        result = std::hypot(result, value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.19
JSTaggedValue BuiltinsMath::Imul(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Imul);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg1 = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> msg2 = GetCallArg(argv, 1);
    JSTaggedNumber numberValue1 = JSTaggedValue::ToNumber(thread, msg1);
    JSTaggedNumber numberValue2 = JSTaggedValue::ToNumber(thread, msg2);
    auto value1 = numberValue1.GetNumber();
    auto value2 = numberValue2.GetNumber();
    if (!std::isfinite(value1) || !std::isfinite(value2)) {
        // If value is NaN or -NaN, +infinite, -infinite
        return GetTaggedInt(0);
    }
    value1 = numberValue1.ToInt32();
    value2 = numberValue2.ToInt32();
    // purposely ignoring overflow
    auto result = static_cast<int32_t>(static_cast<int64_t>(value1) * static_cast<int64_t>(value2));
    return GetTaggedInt(result);
}

// 20.2.2.20
JSTaggedValue BuiltinsMath::Log(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Log);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN , -NaN , or < 0,result is NaN
    if (!std::isnan(std::abs(value)) && value >= 0) {
        result = std::log(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.21
JSTaggedValue BuiltinsMath::Log1p(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Log1p);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN , -NaN , or < -1,result is NaN
    if (!std::isnan(std::abs(value)) && value >= -1) {
        result = std::log1p(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.22
JSTaggedValue BuiltinsMath::Log10(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Log10);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN , -NaN , or < 0,result is NaN
    if (!std::isnan(std::abs(value)) && value >= 0) {
        result = std::log10(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.23
JSTaggedValue BuiltinsMath::Log2(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Log2);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN , -NaN , or < 0,result is NaN
    if (!std::isnan(std::abs(value)) && value >= 0) {
        result = std::log2(value);
    }
    return GetTaggedDouble(result);
}

inline bool IsNegZero(double value)
{
    return (value == 0.0 && (bit_cast<uint64_t>(value) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK);
}

// 20.2.2.24
JSTaggedValue BuiltinsMath::Max(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Max);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    uint32_t argLen = argv->GetArgsNumber();
    auto numberValue = JSTaggedNumber(-base::POSITIVE_INFINITY);
    // If no arguments are given, the result is -inf
    auto result = JSTaggedNumber(-base::POSITIVE_INFINITY);
    auto tmpMax = -base::POSITIVE_INFINITY;
    auto value = -base::POSITIVE_INFINITY;
    for (uint32_t i = 0; i < argLen; i++) {
        JSHandle<JSTaggedValue> msg = GetCallArg(argv, i);
        numberValue = JSTaggedValue::ToNumber(thread, msg);
        value = numberValue.GetNumber();
        if (std::isnan(std::abs(value))) {
            // If any value is NaN, or -NaN, the max result is NaN
            result = numberValue;
            break;
        }
        if (value > tmpMax) {
            result = numberValue;
            tmpMax = value;
        } else if (value == 0 && tmpMax == 0 && IsNegZero(tmpMax) && !IsNegZero(value)) {
            // if tmp_max is -0, value is 0, max is 0
            result = numberValue;
            tmpMax = value;
        }
    }
    return result;
}

// 20.2.2.25
JSTaggedValue BuiltinsMath::Min(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Min);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    uint32_t argLen = argv->GetArgsNumber();
    auto numberValue = JSTaggedNumber(base::POSITIVE_INFINITY);
    // If no arguments are given, the result is inf
    auto result = JSTaggedNumber(base::POSITIVE_INFINITY);
    auto tmpMin = base::POSITIVE_INFINITY;
    auto value = base::POSITIVE_INFINITY;
    for (uint32_t i = 0; i < argLen; i++) {
        JSHandle<JSTaggedValue> msg = GetCallArg(argv, i);
        numberValue = JSTaggedValue::ToNumber(thread, msg);
        value = numberValue.GetNumber();
        if (std::isnan(std::abs(value))) {
            // If any value is NaN or -NaN, the min result is NaN
            result = numberValue;
            break;
        }
        if (value < tmpMin) {
            result = numberValue;
            tmpMin = value;
        } else if (value == 0 && tmpMin == 0 && !IsNegZero(tmpMin) && IsNegZero(value)) {
            // if tmp_min is 0, value is -0, min is -0
            result = numberValue;
            tmpMin = value;
        }
    }
    return result;
}

// 20.2.2.26
JSTaggedValue BuiltinsMath::Pow(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Pow);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msgX = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> msgY = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> baseVale = JSTaggedValue::ToNumeric(thread, msgX.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> exponentValue = JSTaggedValue::ToNumeric(thread, msgY.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (baseVale->IsBigInt() || exponentValue->IsBigInt()) {
        if (baseVale->IsBigInt() && exponentValue->IsBigInt()) {
            JSHandle<BigInt> bigBaseVale(baseVale);
            JSHandle<BigInt> bigExponentValue(exponentValue);
            return  BigInt::Exponentiate(thread, bigBaseVale, bigExponentValue).GetTaggedValue();
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot mix BigInt and other types, use explicit conversions",
                                    JSTaggedValue::Exception());
    }
    double valueX = baseVale->GetNumber();
    double valueY = exponentValue->GetNumber();
    // If abs(x) is 1 and y is inf or -inf, the result is NaN
    if (std::abs(valueX) == 1 && !std::isfinite(valueY)) {
        return GetTaggedDouble(base::NAN_VALUE);
    }
    double result = std::pow(valueX, valueY);
    if (std::isnan(std::abs(result))) {
        // If result is NaN or -NaN, the result is NaN
        result = base::NAN_VALUE;
    }
    return GetTaggedDouble(result);
}

// 20.2.2.27
JSTaggedValue BuiltinsMath::Random([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Random);
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_real_distribution<double> dis(0, std::random_device::max() - 1);
    // result range [0,1)
    double result = dis(engine) / std::random_device::max();
    return GetTaggedDouble(result);
}

// 20.2.2.28
JSTaggedValue BuiltinsMath::Round(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Round);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    auto result = base::NAN_VALUE;
    const double diff = 0.5;
    double absValue = std::abs(value);
    if (!std::isfinite(absValue) || absValue == 0) {
        // If value is NaN, +infinite, or -infinite, VRegisterTag is DOUBLE
        if (!std::isnan(absValue)) {
            // If value is NaN or -NaN, the result is default NaN, else is value
            result = value;
        }
        return GetTaggedDouble(result);
    }
    // If x is less than 0 but greater than or equal to -0.5, the result is -0
    if (value < 0 && value >= -diff) {
        return GetTaggedDouble(-0.0);
    }
    // If x is greater than 0 but less than 0.5, the result is +0
    if (value > 0 && value < diff) {
        return GetTaggedInt(0);
    }
    // For huge integers
    result = std::ceil(value);
    if (result - value > diff) {
        result -= 1;
    }
    return GetTaggedDouble(result);
}

// 20.2.2.29
JSTaggedValue BuiltinsMath::Sign(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Sign);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    if (std::isnan(std::abs(value))) {
        return GetTaggedDouble(std::abs(value));
    }
    if (value == 0.0) {
        return GetTaggedDouble(value);
    }
    if (value < 0) {
        return GetTaggedInt(-1);
    }
    return GetTaggedInt(1);
}

// 20.2.2.30
JSTaggedValue BuiltinsMath::Sin(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Sin);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN or -NaN, the result is NaN
    if (std::isfinite(std::abs(value))) {
        result = std::sin(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.31
JSTaggedValue BuiltinsMath::Sinh(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Sinh);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN or -NaN, the result is NaN
    if (!std::isnan(std::abs(value))) {
        result = std::sinh(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.32
JSTaggedValue BuiltinsMath::Sqrt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Sqrt);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN or -NaN, or value < 0, the result is NaN
    if (!std::isnan(std::abs(value)) && value >= 0) {
        result = std::sqrt(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.33
JSTaggedValue BuiltinsMath::Tan(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Tan);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    // If value is NaN or -NaN, +infinite, -infinite, result is NaN
    if (std::isfinite(value)) {
        result = std::tan(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.34
JSTaggedValue BuiltinsMath::Tanh(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Tanh);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    if (!std::isnan(std::abs(value))) {
        result = std::tanh(value);
    }
    return GetTaggedDouble(result);
}

// 20.2.2.35
JSTaggedValue BuiltinsMath::Trunc(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Math, Trunc);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, msg);
    double value = numberValue.GetNumber();
    double result = base::NAN_VALUE;
    if (!std::isfinite(value)) {
        // if value is +infinite, -infinite, NaN, -NaN, VRegisterTag is double
        if (!std::isnan(std::abs(value))) {
            // if value is +infinite, -infinite, result is value ;
            result = value;
        }
    } else {
        result = std::trunc(value);
    }
    return GetTaggedDouble(result);
}
}  // namespace panda::ecmascript::builtins
