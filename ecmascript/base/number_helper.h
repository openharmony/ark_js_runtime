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

#ifndef ECMASCRIPT_BASE_NUMBER_HELPER_H
#define ECMASCRIPT_BASE_NUMBER_HELPER_H

#include <cstdint>
#include "ecmascript/ecma_string.h"

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::base {
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static constexpr uint16_t SPACE_OR_LINE_TERMINAL[] = {
    0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x0020, 0x00A0, 0x1680, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004,
    0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x2028, 0x2029, 0x202F, 0x205F, 0x3000, 0xFEFF,
};

constexpr double MIN_RADIX = 2;
constexpr double MAX_RADIX = 36;
constexpr double MIN_FRACTION = 0;
constexpr double MAX_FRACTION = 100;

// Coversion flags
static constexpr uint32_t NO_FLAGS = 0U;
static constexpr uint32_t ALLOW_BINARY = 1U << 0U;
static constexpr uint32_t ALLOW_OCTAL = 1U << 1U;
static constexpr uint32_t ALLOW_HEX = 1U << 2U;
static constexpr uint32_t IGNORE_TRAILING = 1U << 3U;

static constexpr uint32_t MAX_PRECISION = 16;
static constexpr uint8_t BINARY = 2;
static constexpr uint8_t OCTAL = 8;
static constexpr uint8_t DECIMAL = 10;
static constexpr uint8_t HEXADECIMAL = 16;
static constexpr double HALF = 0.5;
static constexpr double EPSILON = std::numeric_limits<double>::epsilon();
static constexpr double MAX_SAFE_INTEGER = 9007199254740991;
static constexpr double MAX_VALUE = std::numeric_limits<double>::max();
static constexpr double MIN_VALUE = std::numeric_limits<double>::min();
static constexpr double POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
static constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

// Helper defines for double
static constexpr int DOUBLE_MAX_PRECISION = 15;
static constexpr int DOUBLE_EXPONENT_BIAS = 0x3FF;
static constexpr size_t DOUBLE_SIGNIFICAND_SIZE = 52;
static constexpr uint64_t DOUBLE_SIGN_MASK = 0x8000000000000000ULL;
static constexpr uint64_t DOUBLE_EXPONENT_MASK = 0x7FFULL << DOUBLE_SIGNIFICAND_SIZE;
static constexpr uint64_t DOUBLE_SIGNIFICAND_MASK = 0x000FFFFFFFFFFFFFULL;
static constexpr uint64_t DOUBLE_HIDDEN_BIT = 1ULL << DOUBLE_SIGNIFICAND_SIZE;

static constexpr size_t INT64_BITS = 64;
static constexpr size_t INT32_BITS = 32;
static constexpr size_t INT16_BITS = 16;
static constexpr size_t INT8_BITS = 8;

class NumberHelper {
public:
    static bool IsFinite(JSTaggedValue number)
    {
        return number.IsInt() || (number.IsDouble() && std::isfinite(number.GetDouble()));
    }
    static bool IsNaN(JSTaggedValue number)
    {
        return number.IsDouble() && std::isnan(number.GetDouble());
    }
    static JSTaggedValue DoubleToString(JSThread *thread, double number, int radix);
    static bool IsEmptyString(const uint8_t *start, const uint8_t *end);
    static JSHandle<EcmaString> NumberToString(const JSThread *thread, JSTaggedValue number);
    static double TruncateDouble(double d);
    static double StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix, uint32_t flags = NO_FLAGS);
    static int32_t DoubleToInt(double d, size_t bits);
    static int32_t DoubleInRangeInt32(double d);
    static JSTaggedValue DoubleToExponential(JSThread *thread, double number, int digit);
    static JSTaggedValue DoubleToFixed(JSThread *thread, double number, int digit);
    static JSTaggedValue DoubleToPrecision(JSThread *thread, double number, int digit);
    static JSTaggedValue StringToDoubleWithRadix(const uint8_t *start, const uint8_t *end, int radix);
    static CString IntToString(int number);

private:
    static char Carry(char current, int radix);
    static double Strtod(const char *str, int exponent, uint8_t radix);
    static CString IntergerToString(double number, int radix);
    static CString DecimalsToString(double *numberInteger, double fraction, int radix, double delta);
    static bool IsNonspace(uint16_t c);
    static bool GotoNonspace(uint8_t **ptr, const uint8_t *end);
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_NUMBER_HELPER_H
