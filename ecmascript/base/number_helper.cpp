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

#include "ecmascript/base/number_helper.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::base {
enum class Sign { NONE, NEG, POS };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_IF_CONVERSION_END(p, end, result) \
    if ((p) == (end)) {                          \
        return (result);                         \
    }

constexpr char CHARS[] = "0123456789abcdefghijklmnopqrstuvwxyz";  // NOLINT (modernize-avoid-c-arrays)
constexpr uint64_t MAX_MANTISSA = 0x1ULL << 52U;

static inline uint8_t ToDigit(uint8_t c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + DECIMAL;
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + DECIMAL;
    }
    return '$';
}

bool NumberHelper::IsNonspace(uint16_t c)
{
    int i;
    int len = sizeof(SPACE_OR_LINE_TERMINAL) / sizeof(SPACE_OR_LINE_TERMINAL[0]);
    for (i = 0; i < len; i++) {
        if (c == SPACE_OR_LINE_TERMINAL[i]) {
            return false;
        }
        if (c < SPACE_OR_LINE_TERMINAL[i]) {
            return true;
        }
    }
    return true;
}

bool NumberHelper::GotoNonspace(uint8_t **ptr, const uint8_t *end)
{
    while (*ptr < end) {
        uint16_t c = **ptr;
        size_t size = 1;
        if (c > INT8_MAX) {
            size = 0;
            uint16_t utf8Bit = INT8_MAX + 1;  // equal 0b1000'0000
            while (utf8Bit > 0 && (c & utf8Bit) == utf8Bit) {
                ++size;
                utf8Bit >>= 1UL;
            }
            if (base::utf_helper::ConvertRegionUtf8ToUtf16(*ptr, &c, end - *ptr, 1, 0) <= 0) {
                return true;
            }
        }
        if (IsNonspace(c)) {
            return true;
        }
        *ptr += size;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return false;
}

static inline double SignedZero(Sign sign)
{
    return sign == Sign::NEG ? -0.0 : 0.0;
}

bool NumberHelper::IsEmptyString(const uint8_t *start, const uint8_t *end)
{
    auto p = const_cast<uint8_t *>(start);
    return !NumberHelper::GotoNonspace(&p, end);
}

JSTaggedValue NumberHelper::DoubleToString(JSThread *thread, double number, int radix)
{
    bool negative = false;
    if (number < 0.0) {
        negative = true;
        number = -number;
    }

    double numberInteger = std::floor(number);
    double numberFraction = number - numberInteger;

    auto value = bit_cast<uint64_t>(number);
    value += 1;
    double delta = HALF * (bit_cast<double>(value) - number);

    CString result;
    if (numberFraction != 0 && numberFraction >= delta) {
        result += ".";
        result += DecimalsToString(&numberInteger, numberFraction, radix, delta);
    }

    result = IntegerToString(numberInteger, radix) + result;

    if (negative) {
        result = "-" + result;
    }

    return BuiltinsBase::GetTaggedString(thread, result.c_str());
}

JSTaggedValue NumberHelper::DoubleToExponential(JSThread *thread, double number, int digit)
{
    CStringStream ss;
    if (digit < 0) {
        ss << std::setiosflags(std::ios::scientific) << std::setprecision(base::MAX_PRECISION) << number;
    } else {
        ss << std::setiosflags(std::ios::scientific) << std::setprecision(digit) << number;
    }
    CString result = ss.str();
    size_t found = result.find_last_of('e');
    if (found != CString::npos && found < result.size() - 2 && result[found + 2] == '0') {
        result.erase(found + 2, 1); // 2:offset of e
    }
    if (digit < 0) {
        size_t end = found;
        while (--found > 0) {
            if (result[found] != '0') {
                break;
            }
        }
        if (result[found] == '.') {
            found--;
        }
        if (found < end - 1) {
            result.erase(found + 1, end - found - 1);
        }
    }
    return BuiltinsBase::GetTaggedString(thread, result.c_str());
}

JSTaggedValue NumberHelper::DoubleToFixed(JSThread *thread, double number, int digit)
{
    CStringStream ss;
    ss << std::setiosflags(std::ios::fixed) << std::setprecision(digit) << number;
    return BuiltinsBase::GetTaggedString(thread, ss.str().c_str());
}

JSTaggedValue NumberHelper::DoubleToPrecision(JSThread *thread, double number, int digit)
{
    if (number == 0.0) {
        return DoubleToFixed(thread, number, digit - 1);
    }
    CStringStream ss;
    double positiveNumber = number > 0 ? number : -number;
    int logDigit = std::floor(log10(positiveNumber));
    int radixDigit = digit - logDigit - 1;
    const int MAX_EXPONENT_DIGIT = 6;
    if ((logDigit >= 0 && radixDigit >= 0) || (logDigit < 0 && radixDigit <= MAX_EXPONENT_DIGIT)) {
        return DoubleToFixed(thread, number, std::abs(radixDigit));
    }
    return DoubleToExponential(thread, number, digit - 1);
}

JSTaggedValue NumberHelper::StringToDoubleWithRadix(const uint8_t *start, const uint8_t *end, int radix)
{
    auto p = const_cast<uint8_t *>(start);
    JSTaggedValue nanResult = BuiltinsBase::GetTaggedDouble(NAN_VALUE);
    // 1. skip space and line terminal
    if (!NumberHelper::GotoNonspace(&p, end)) {
        return nanResult;
    }

    // 2. sign bit
    bool negative = false;
    if (*p == '-') {
        negative = true;
        RETURN_IF_CONVERSION_END(++p, end, nanResult);
    } else if (*p == '+') {
        RETURN_IF_CONVERSION_END(++p, end, nanResult);
    }
    // 3. 0x or 0X
    bool stripPrefix = true;
    // 4. If R  0, then
    //     a. If R < 2 or R > 36, return NaN.
    //     b. If R  16, let stripPrefix be false.
    if (radix != 0) {
        if (radix < MIN_RADIX || radix > MAX_RADIX) {
            return nanResult;
        }
        if (radix != HEXADECIMAL) {
            stripPrefix = false;
        }
    } else {
        radix = DECIMAL;
    }
    int size = 0;
    if (stripPrefix) {
        if (*p == '0') {
            size++;
            if (++p != end && (*p == 'x' || *p == 'X')) {
                RETURN_IF_CONVERSION_END(++p, end, nanResult);
                radix = HEXADECIMAL;
            }
        }
    }

    double result = 0;
    bool isDone = false;
    do {
        double part = 0;
        uint32_t multiplier = 1;
        for (; p != end; ++p) {
            // The maximum value to ensure that uint32_t will not overflow
            const uint32_t MAX_MULTIPER = 0xffffffffU / 36;
            uint32_t m = multiplier * radix;
            if (m > MAX_MULTIPER) {
                break;
            }

            int currentBit = static_cast<int>(ToDigit(*p));
            if (currentBit >= radix) {
                isDone = true;
                break;
            }
            size++;
            part = part * radix + currentBit;
            multiplier = m;
        }
        result = result * multiplier + part;
        if (isDone) {
            break;
        }
    } while (p != end);

    if (size == 0) {
        return nanResult;
    }

    if (negative) {
        result = -result;
    }
    return BuiltinsBase::GetTaggedDouble(result);
}

char NumberHelper::Carry(char current, int radix)
{
    int digit = static_cast<int>((current > '9') ? (current - 'a' + DECIMAL) : (current - '0'));
    digit = (digit == (radix - 1)) ? 0 : digit + 1;
    return CHARS[digit];
}

CString NumberHelper::IntegerToString(double number, int radix)
{
    ASSERT(radix >= MIN_RADIX && radix <= MAX_RADIX);
    CString result;
    while (number / radix > MAX_MANTISSA) {
        number /= radix;
        result = CString("0").append(result);
    }
    do {
        double remainder = std::fmod(number, radix);
        result = CHARS[static_cast<int>(remainder)] + result;
        number = (number - remainder) / radix;
    } while (number > 0);
    return result;
}

CString NumberHelper::DecimalsToString(double *numberInteger, double fraction, int radix, double delta)
{
    CString result;
    while (fraction >= delta) {
        fraction *= radix;
        delta *= radix;
        int64_t integer = std::floor(fraction);
        fraction -= integer;
        result += CHARS[integer];
        if (fraction > HALF && fraction + delta > 1) {
            size_t fractionEnd = result.size() - 1;
            result[fractionEnd] = Carry(*result.rbegin(), radix);
            for (; fractionEnd > 0; fractionEnd--) {
                if (result[fractionEnd] == '0') {
                    result[fractionEnd - 1] = Carry(result[fractionEnd - 1], radix);
                } else {
                    break;
                }
            }
            if (fractionEnd == 0) {
                (*numberInteger)++;
            }
            break;
        }
    }
    // delete 0 in the end
    size_t found = result.find_last_not_of('0');
    if (found != CString::npos) {
        result.erase(found + 1);
    }

    return result;
}

CString NumberHelper::IntToString(int number)
{
    return ToCString(number);
}

// 7.1.12.1 ToString Applied to the Number Type
JSHandle<EcmaString> NumberHelper::NumberToString(const JSThread *thread, JSTaggedValue number)
{
    ASSERT(number.IsNumber());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (number.IsInt()) {
        return factory->NewFromASCII(IntToString(number.GetInt()));
    }

    double d = number.GetDouble();
    if (std::isnan(d)) {
        return factory->NewFromASCII("NaN");
    }
    if (d == 0.0) {
        return factory->NewFromASCII("0");
    }
    if (d >= INT32_MIN + 1 && d <= INT32_MAX && d == static_cast<double>(static_cast<int32_t>(d))) {
        return factory->NewFromASCII(IntToString(static_cast<int32_t>(d)));
    }

    std::string result;
    if (d < 0) {
        result += "-";
        d = -d;
    }

    if (std::isinf(d)) {
        result += "Infinity";
        return factory->NewFromASCII(result.c_str());
    }

    ASSERT(d > 0);

    // 5. Otherwise, let n, k, and s be integers such that k ≥ 1, 10k−1 ≤ s < 10k, the Number value for s × 10n−k is m,
    // and k is as small as possible. If there are multiple possibilities for s, choose the value of s for which s ×
    // 10n−k is closest in value to m. If there are two such possible values of s, choose the one that is even. Note
    // that k is the number of digits in the decimal representation of s and that s is not divisible by 10.
    char buffer[JS_DTOA_BUF_SIZE] = {0};
    int n = 0;
    int k = GetMinmumDigits(d, &n, buffer);
    std::string base = buffer;
    if (n > 0 && n <= 21) {  // NOLINT(readability-magic-numbers)
        base.erase(1, 1);
        if (k <= n) {
            // 6. If k ≤ n ≤ 21, return the String consisting of the code units of the k digits of the decimal
            // representation of s (in order, with no leading zeroes), followed by n−k occurrences of the code unit
            // 0x0030 (DIGIT ZERO).
            base += std::string(n - k, '0');
        } else {
            // 7. If 0 < n ≤ 21, return the String consisting of the code units of the most significant n digits of the
            // decimal representation of s, followed by the code unit 0x002E (FULL STOP), followed by the code units of
            // the remaining k−n digits of the decimal representation of s.
            base.insert(n, 1, '.');
        }
    } else if (-6 < n && n <= 0) {  // NOLINT(readability-magic-numbers)
        // 8. If −6 < n ≤ 0, return the String consisting of the code unit 0x0030 (DIGIT ZERO), followed by the code
        // unit 0x002E (FULL STOP), followed by −n occurrences of the code unit 0x0030 (DIGIT ZERO), followed by the
        // code units of the k digits of the decimal representation of s.
        base.erase(1, 1);
        base = std::string("0.") + std::string(-n, '0') + base;
    } else {
        if (k == 1) {
            // 9. Otherwise, if k = 1, return the String consisting of the code unit of the single digit of s
            base.erase(1, 1);
        }
        // followed by code unit 0x0065 (LATIN SMALL LETTER E), followed by the code unit 0x002B (PLUS SIGN) or the code
        // unit 0x002D (HYPHEN-MINUS) according to whether n−1 is positive or negative, followed by the code units of
        // the decimal representation of the integer abs(n−1) (with no leading zeroes).
        base += "e" + (n >= 1 ? std::string("+") : "") + std::to_string(n - 1);
    }
    result += base;
    return factory->NewFromASCII(result.c_str());
}

double NumberHelper::TruncateDouble(double d)
{
    if (std::isnan(d)) {
        return 0;
    }
    if (!std::isfinite(d)) {
        return d;
    }
    // -0 to +0
    if (d == 0.0) {
        return 0;
    }
    return (d >= 0) ? std::floor(d) : std::ceil(d);
}

double NumberHelper::StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix, uint32_t flags)
{
    auto p = const_cast<uint8_t *>(start);
    // 1. skip space and line terminal
    if (!NumberHelper::GotoNonspace(&p, end)) {
        return 0.0;
    }

    // 2. get number sign
    Sign sign = Sign::NONE;
    if (*p == '+') {
        RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
        sign = Sign::POS;
    } else if (*p == '-') {
        RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
        sign = Sign::NEG;
    }
    bool ignoreTrailing = (flags & IGNORE_TRAILING) != 0;

    // 3. judge Infinity
    static const char INF[] = "Infinity";  // NOLINT(modernize-avoid-c-arrays)
    if (*p == INF[0]) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (const char *i = &INF[1]; *i != '\0'; ++i) {
            if (++p == end || *p != *i) {
                return NAN_VALUE;
            }
        }
        ++p;
        if (!ignoreTrailing && NumberHelper::GotoNonspace(&p, end)) {
            return NAN_VALUE;
        }
        return sign == Sign::NEG ? -POSITIVE_INFINITY : POSITIVE_INFINITY;
    }

    // 4. get number radix
    bool leadingZero = false;
    bool prefixRadix = false;
    if (*p == '0' && radix == 0) {
        RETURN_IF_CONVERSION_END(++p, end, SignedZero(sign));
        if (*p == 'x' || *p == 'X') {
            if ((flags & ALLOW_HEX) == 0) {
                return ignoreTrailing ? SignedZero(sign) : NAN_VALUE;
            }
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
            if (sign != Sign::NONE) {
                return NAN_VALUE;
            }
            prefixRadix = true;
            radix = HEXADECIMAL;
        } else if (*p == 'o' || *p == 'O') {
            if ((flags & ALLOW_OCTAL) == 0) {
                return ignoreTrailing ? SignedZero(sign) : NAN_VALUE;
            }
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
            if (sign != Sign::NONE) {
                return NAN_VALUE;
            }
            prefixRadix = true;
            radix = OCTAL;
        } else if (*p == 'b' || *p == 'B') {
            if ((flags & ALLOW_BINARY) == 0) {
                return ignoreTrailing ? SignedZero(sign) : NAN_VALUE;
            }
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
            if (sign != Sign::NONE) {
                return NAN_VALUE;
            }
            prefixRadix = true;
            radix = BINARY;
        } else {
            leadingZero = true;
        }
    }

    if (radix == 0) {
        radix = DECIMAL;
    }
    auto pStart = p;
    // 5. skip leading '0'
    while (*p == '0') {
        RETURN_IF_CONVERSION_END(++p, end, SignedZero(sign));
        leadingZero = true;
    }
    // 6. parse to number
    uint64_t intNumber = 0;
    uint64_t numberMax = (UINT64_MAX - (radix - 1)) / radix;
    int digits = 0;
    int exponent = 0;
    do {
        uint8_t c = ToDigit(*p);
        if (c >= radix) {
            if (!prefixRadix || ignoreTrailing || (pStart != p && !NumberHelper::GotoNonspace(&p, end))) {
                break;
            }
            // "0b" "0x1.2" "0b1e2" ...
            return NAN_VALUE;
        }
        ++digits;
        if (intNumber < numberMax) {
            intNumber = intNumber * radix + c;
        } else {
            ++exponent;
        }
    } while (++p != end);

    auto number = static_cast<double>(intNumber);
    if (sign == Sign::NEG) {
        if (number == 0) {
            number = -0.0;
        } else {
            number = -number;
        }
    }

    // 7. deal with other radix except DECIMAL
    if (p == end || radix != DECIMAL) {
        if ((digits == 0 && !leadingZero) || (p != end && !ignoreTrailing && NumberHelper::GotoNonspace(&p, end))) {
            // no digits there, like "0x", "0xh", or error trailing of "0x3q"
            return NAN_VALUE;
        }
        return number * std::pow(radix, exponent);
    }

    // 8. parse '.'
    if (radix == DECIMAL && *p == '.') {
        RETURN_IF_CONVERSION_END(++p, end, (digits > 0) ? (number * std::pow(radix, exponent)) : NAN_VALUE);
        while (ToDigit(*p) < radix) {
            --exponent;
            ++digits;
            if (++p == end) {
                break;
            }
        }
    }
    if (digits == 0 && !leadingZero) {
        // no digits there, like ".", "sss", or ".e1"
        return NAN_VALUE;
    }
    auto pEnd = p;

    // 9. parse 'e/E' with '+/-'
    char exponentSign = '+';
    int additionalExponent = 0;
    constexpr int MAX_EXPONENT = INT32_MAX / 2;
    if (radix == DECIMAL && (p != end && (*p == 'e' || *p == 'E'))) {
        RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);

        // 10. parse exponent number
        if (*p == '+' || *p == '-') {
            exponentSign = static_cast<char>(*p);
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
        }
        uint8_t digit;
        while ((digit = ToDigit(*p)) < radix) {
            if (additionalExponent > static_cast<int>(MAX_EXPONENT / radix)) {
                additionalExponent = MAX_EXPONENT;
            } else {
                additionalExponent = additionalExponent * static_cast<int>(radix) + static_cast<int>(digit);
            }
            if (++p == end) {
                break;
            }
        }
    }
    exponent += (exponentSign == '-' ? -additionalExponent : additionalExponent);
    if (!ignoreTrailing && NumberHelper::GotoNonspace(&p, end)) {
        return NAN_VALUE;
    }

    // 10. build StringNumericLiteral string
    CString buffer;
    if (sign == Sign::NEG) {
        buffer += "-";
    }
    for (uint8_t *i = pStart; i < pEnd; ++i) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (*i != static_cast<uint8_t>('.')) {
            buffer += *i;
        }
    }

    // 11. convert none-prefix radix string
    return Strtod(buffer.c_str(), exponent, radix);
}

double NumberHelper::Strtod(const char *str, int exponent, uint8_t radix)
{
    ASSERT(str != nullptr);
    ASSERT(radix >= base::MIN_RADIX && radix <= base::MAX_RADIX);
    auto p = const_cast<char *>(str);
    Sign sign = Sign::NONE;
    uint64_t number = 0;
    uint64_t numberMax = (UINT64_MAX - (radix - 1)) / radix;
    double result = 0.0;
    if (*p == '-') {
        sign = Sign::NEG;
        ++p;
    }
    while (*p == '0') {
        ++p;
    }
    while (*p != '\0') {
        uint8_t digit = ToDigit(static_cast<uint8_t>(*p));
        if (digit >= radix) {
            break;
        }
        if (number < numberMax) {
            number = number * radix + digit;
        } else {
            ++exponent;
        }
        ++p;
    }
    if (exponent < 0) {
        result = number / std::pow(radix, -exponent);
    } else {
        result = number * std::pow(radix, exponent);
    }
    return sign == Sign::NEG ? -result : result;
}

int32_t NumberHelper::DoubleToInt(double d, size_t bits)
{
    int32_t ret = 0;
    auto u64 = bit_cast<uint64_t>(d);
    int exp = static_cast<int>((u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_SIGNIFICAND_SIZE) - DOUBLE_EXPONENT_BIAS;
    if (exp < static_cast<int>(bits - 1)) {
        // smaller than INT<bits>_MAX, fast conversion
        ret = static_cast<int32_t>(d);
    } else if (exp < static_cast<int>(bits + DOUBLE_SIGNIFICAND_SIZE)) {
        // Still has significand bits after mod 2^<bits>
        // Get low <bits> bits by shift left <64 - bits> and shift right <64 - bits>
        uint64_t value = (((u64 & DOUBLE_SIGNIFICAND_MASK) | DOUBLE_HIDDEN_BIT)
                          << (exp - DOUBLE_SIGNIFICAND_SIZE + INT64_BITS - bits)) >>
                         (INT64_BITS - bits);
        ret = static_cast<int32_t>(value);
        if ((u64 & DOUBLE_SIGN_MASK) == DOUBLE_SIGN_MASK && ret != INT32_MIN) {
            ret = -ret;
        }
    } else {
        // No significand bits after mod 2^<bits>, contains NaN and INF
        ret = 0;
    }
    return ret;
}

int32_t NumberHelper::DoubleInRangeInt32(double d)
{
    if (d > INT_MAX) {
        return INT_MAX;
    }
    if (d < INT_MIN) {
        return INT_MIN;
    }
    return base::NumberHelper::DoubleToInt(d, base::INT32_BITS);
}

JSTaggedValue NumberHelper::StringToBigInt(JSThread *thread, JSHandle<JSTaggedValue> strVal)
{
    Span<const uint8_t> str;
    auto strObj = static_cast<EcmaString *>(strVal->GetTaggedObject());
    size_t strLen = strObj->GetLength();
    if (strLen == 0) {
        return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    if (UNLIKELY(strObj->IsUtf16())) {
        CVector<uint8_t> buf;
        size_t len = base::utf_helper::Utf16ToUtf8Size(strObj->GetDataUtf16(), strLen) - 1;
        buf.reserve(len);
        len = base::utf_helper::ConvertRegionUtf16ToUtf8(strObj->GetDataUtf16(), buf.data(), strLen, len, 0);
        str = Span<const uint8_t>(buf.data(), len);
    } else {
        str = Span<const uint8_t>(strObj->GetDataUtf8(), strLen);
    }
    auto p = const_cast<uint8_t *>(str.begin());
    auto end = str.end();
    // 1. skip space and line terminal
    if (!NumberHelper::GotoNonspace(&p, end)) {
        return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    // 2. get bigint sign
    Sign sign = Sign::NONE;
    if (*p == '+') {
        RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
        sign = Sign::POS;
    } else if (*p == '-') {
        RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
        sign = Sign::NEG;
    }
    // 3. bigint not allow Infinity, decimal points, or exponents.
    if (isalpha(*p)) {
        return JSTaggedValue(NAN_VALUE);
    }
    // 4. get bigint radix
    uint8_t radix = DECIMAL;
    if (*p == '0') {
        if (++p == end) {
            return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
        }
        if (*p == 'x' || *p == 'X') {
            RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
            if (sign != Sign::NONE) {
                return JSTaggedValue(NAN_VALUE);
            }
            radix = HEXADECIMAL;
        } else if (*p == 'o' || *p == 'O') {
            RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
            if (sign != Sign::NONE) {
                return JSTaggedValue(NAN_VALUE);
            }
            radix = OCTAL;
        } else if (*p == 'b' || *p == 'B') {
            RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
            if (sign != Sign::NONE) {
                return JSTaggedValue(NAN_VALUE);
            }
            radix = BINARY;
        }
    }

    auto pStart = p;
    // 5. skip leading '0'
    while (*p == '0') {
        if (++p == end) {
            return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
        }
    }
    // 6. parse to bigint
    std::string buffer;
    if (sign == Sign::NEG) {
        buffer += "-";
    }
    do {
        uint8_t c = ToDigit(*p);
        if (c >= radix) {
            if (pStart != p && !NumberHelper::GotoNonspace(&p, end)) {
                break;
            }
            return JSTaggedValue(NAN_VALUE);
        }
        buffer += *p;
    } while (++p != end);
    return BigIntHelper::SetBigInt(thread, buffer, radix).GetTaggedValue();
}

void NumberHelper::GetBase(double d, int digits, int *decpt, char *buf, char *bufTmp, int size)
{
    int result = snprintf_s(bufTmp, size, size - 1, "%+.*e", digits - 1, d);
    if (result == -1) {
        LOG_ECMA(FATAL) << "snprintf_s failed";
        UNREACHABLE();
    }
    // mantissa
    buf[0] = bufTmp[1];
    if (digits > 1) {
        if (memcpy_s(buf + 1, digits, bufTmp + 2, digits) != EOK) { // 2 means add the point char to buf
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    buf[digits + 1] = '\0';
    // exponent
    *decpt = atoi(bufTmp + digits + 2 + (digits > 1)) + 1; // 2 means ignore the integer and point
}

int NumberHelper::GetMinmumDigits(double d, int *decpt, char *buf)
{
    int digits = 0;
    char bufTmp[JS_DTOA_BUF_SIZE] = {0};

    // find the minimum amount of digits
    int MinDigits = 1;
    int MaxDigits = DOUBLE_MAX_PRECISION;
    while (MinDigits < MaxDigits) {
        digits = (MinDigits + MaxDigits) / 2;
        GetBase(d, digits, decpt, buf, bufTmp, sizeof(bufTmp));
        if (strtod(bufTmp, NULL) == d) {
            // no need to keep the trailing zeros
            while (digits >= 2 && buf[digits] == '0') { // 2 means ignore the integer and point
                digits--;
            }
            MaxDigits = digits;
        } else {
            MinDigits = digits + 1;
        }
    }
    digits = MaxDigits;
    GetBase(d, digits, decpt, buf, bufTmp, sizeof(bufTmp));

    return digits;
}
}  // namespace panda::ecmascript::base
