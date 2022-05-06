/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_BASE_BIT_HELPER_H
#define ECMASCRIPT_BASE_BIT_HELPER_H
#include <stdint.h>
#include <type_traits>
namespace panda::ecmascript::base {
template <typename T>
inline constexpr int CountLeadingZeros(T value)
{
    constexpr size_t RADIX = 2;
    static_assert(std::is_integral<T>::value, "T must be integral");
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(std::numeric_limits<T>::radix == RADIX, "Unexpected radix!");
    static_assert(sizeof(T) == sizeof(uint64_t) || sizeof(T) <= sizeof(uint32_t), "Unsupported sizeof(T)");
    if (value == 0) {
        return sizeof(T);
    }
    if (sizeof(T) == sizeof(uint64_t)) {
        return __builtin_clzll(static_cast<uint64_t>(value));
    }
    // 32 : 32 mean the bits of type T is less than 32
    return __builtin_clz(static_cast<uint32_t>(value)) - (32 - std::numeric_limits<T>::digits);
}

inline constexpr int CountLeadingZeros32(uint32_t value)
{
    return CountLeadingZeros(value);
}

inline constexpr int CountLeadingZeros64(uint64_t value)
{
    return CountLeadingZeros(value);
}

inline constexpr int CountLeadingOnes32(uint32_t value)
{
    return CountLeadingZeros(~value);
}

inline constexpr int CountLeadingOnes64(uint64_t value)
{
    return CountLeadingZeros(~value);
}

template <typename T>
inline constexpr int CountTrailingZeros(T value)
{
    constexpr size_t RADIX = 2;
    static_assert(std::is_integral<T>::value, "T must be integral");
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(std::numeric_limits<T>::radix == RADIX, "Unexpected radix!");
    static_assert(sizeof(T) == sizeof(uint64_t) || sizeof(T) <= sizeof(uint32_t), "Unsupported sizeof(T)");
    if (value == 0) {
        return sizeof(T);
    }
    if (sizeof(T) == sizeof(uint64_t)) {
        return __builtin_ctzll(static_cast<uint64_t>(value));
    }
    return __builtin_ctz(static_cast<uint32_t>(value));
}

inline constexpr unsigned CountTrailingZeros32(uint32_t value)
{
    return CountTrailingZeros(value);
}

inline constexpr unsigned CountTrailingZeros64(uint64_t value)
{
    return CountTrailingZeros(value);
}

inline constexpr unsigned CountTrailingOnes32(uint32_t value)
{
    return CountTrailingZeros(~value);
}

inline constexpr unsigned CountTrailingOnes64(uint64_t value)
{
    return CountTrailingZeros(~value);
}

/// isMask_64 - This function returns true if the argument is a non-empty
/// sequence of ones starting at the least significant bit with the remainder
/// zero (64 bit version).
constexpr inline bool IsMask_64(uint64_t Value)
{
    return Value && ((Value + 1) & Value) == 0;
}

/// isShiftedMask_64 - This function returns true if the argument contains a
/// non-empty sequence of ones with the remainder zero (64 bit version.)
constexpr inline bool IsShiftedMask_64(uint64_t Value)
{
    return Value && IsMask_64((Value - 1) | Value);
}
}  // panda::ecmascript::base
#endif