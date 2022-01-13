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

#include "ecmascript/base/utf_helper.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
static constexpr int32_t U16_SURROGATE_OFFSET = (0xd800 << 10UL) + 0xdc00 - 0x10000;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define U16_GET_SUPPLEMENTARY(lead, trail) \
    ((static_cast<int32_t>(lead) << 10UL) + static_cast<int32_t>(trail) - U16_SURROGATE_OFFSET)

namespace panda::ecmascript::base::utf_helper {
uint32_t UTF16Decode(uint16_t lead, uint16_t trail)
{
    ASSERT((lead >= DECODE_LEAD_LOW && lead <= DECODE_LEAD_HIGH) &&
           (trail >= DECODE_TRAIL_LOW && trail <= DECODE_TRAIL_HIGH));
    uint32_t cp = (lead - DECODE_LEAD_LOW) * DECODE_FIRST_FACTOR + (trail - DECODE_TRAIL_LOW) + DECODE_SECOND_FACTOR;
    return cp;
}

bool IsValidUTF8(const std::vector<uint8_t> &data)
{
    uint32_t length = data.size();
    switch (length) {
        case UtfLength::ONE:
            if (data.at(0) >= BIT_MASK_1) {
                return false;
            }
            break;
        case UtfLength::TWO:
            if ((data.at(0) & BIT_MASK_3) != BIT_MASK_2) {
                return false;
            }
            break;
        case UtfLength::THREE:
            if ((data.at(0) & BIT_MASK_4) != BIT_MASK_3) {
                return false;
            }
            break;
        case UtfLength::FOUR:
            if ((data.at(0) & BIT_MASK_5) != BIT_MASK_4) {
                return false;
            }
            break;
        default:
            UNREACHABLE();
            break;
    }

    for (uint32_t i = 1; i < length; i++) {
        if ((data.at(i) & BIT_MASK_2) != BIT_MASK_1) {
            return false;
        }
    }
    return true;
}

Utf8Char ConvertUtf16ToUtf8(uint16_t d0, uint16_t d1, bool modify)
{
    // when first utf16 code is in 0xd800-0xdfff and second utf16 code is 0,
    // means that is a single code point, it needs to be represented by three UTF8 code.
    if (d1 == 0 && d0 >= utf::HI_SURROGATE_MIN && d0 <= utf::LO_SURROGATE_MAX) {
        auto ch0 = static_cast<uint8_t>(UTF8_3B_FIRST | static_cast<uint8_t>(d0 >> UtfOffset::TWELVE));
        auto ch1 = static_cast<uint8_t>(UTF8_3B_SECOND | (static_cast<uint8_t>(d0 >> UtfOffset::SIX) & utf::MASK_6BIT));
        auto ch2 = static_cast<uint8_t>(UTF8_3B_THIRD | (d0 & utf::MASK_6BIT));
        return {UtfLength::THREE, {ch0, ch1, ch2}};
    }

    if (d0 == 0) {
        if (modify) {
            // special case for \u0000 ==> C080 - 1100'0000 1000'0000
            return {UtfLength::TWO, {UTF8_2B_FIRST, UTF8_2B_SECOND}};
        }
        // For print string, just skip '\u0000'
        return {0, {0x00U}};
    }
    if (d0 <= UTF8_1B_MAX) {
        return {UtfLength::ONE, {static_cast<uint8_t>(d0)}};
    }
    if (d0 <= UTF8_2B_MAX) {
        auto ch0 = static_cast<uint8_t>(UTF8_2B_FIRST | static_cast<uint8_t>(d0 >> UtfOffset::SIX));
        auto ch1 = static_cast<uint8_t>(UTF8_2B_SECOND | (d0 & utf::MASK_6BIT));
        return {UtfLength::TWO, {ch0, ch1}};
    }
    if (d0 < utf::HI_SURROGATE_MIN || d0 > utf::HI_SURROGATE_MAX) {
        auto ch0 = static_cast<uint8_t>(UTF8_3B_FIRST | static_cast<uint8_t>(d0 >> UtfOffset::TWELVE));
        auto ch1 = static_cast<uint8_t>(UTF8_3B_SECOND | (static_cast<uint8_t>(d0 >> UtfOffset::SIX) & utf::MASK_6BIT));
        auto ch2 = static_cast<uint8_t>(UTF8_3B_THIRD | (d0 & utf::MASK_6BIT));
        return {UtfLength::THREE, {ch0, ch1, ch2}};
    }
    if (d1 < utf::LO_SURROGATE_MIN || d1 > utf::LO_SURROGATE_MAX) {
        // Bad sequence
        UNREACHABLE();
    }

    uint32_t codePoint = CombineTwoU16(d0, d1);

    auto ch0 = static_cast<uint8_t>((codePoint >> UtfOffset::EIGHTEEN) | UTF8_4B_FIRST);
    auto ch1 = static_cast<uint8_t>(((codePoint >> UtfOffset::TWELVE) & utf::MASK_6BIT) | utf::MASK1);
    auto ch2 = static_cast<uint8_t>(((codePoint >> UtfOffset::SIX) & utf::MASK_6BIT) | utf::MASK1);
    auto ch3 = static_cast<uint8_t>((codePoint & utf::MASK_6BIT) | utf::MASK1);
    return {UtfLength::FOUR, {ch0, ch1, ch2, ch3}};
}

size_t Utf16ToUtf8Size(const uint16_t *utf16, uint32_t length, bool modify)
{
    size_t res = 1;  // zero byte
    // when utf16 data length is only 1 and code in 0xd800-0xdfff,
    // means that is a single code point, it needs to be represented by three UTF8 code.
    if (length == 1 && utf16[0] >= utf::HI_SURROGATE_MIN &&  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        utf16[0] <= utf::LO_SURROGATE_MAX) {                 // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        res += UtfLength::THREE;
        return res;
    }

    for (uint32_t i = 0; i < length; ++i) {
        if (utf16[i] == 0) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (modify) {
                res += UtfLength::TWO;  // special case for U+0000 => C0 80
            }
        } else if (utf16[i] <= UTF8_1B_MAX) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            res += 1;
        } else if (utf16[i] <= UTF8_2B_MAX) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            res += UtfLength::TWO;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (utf16[i] < utf::HI_SURROGATE_MIN || utf16[i] > utf::HI_SURROGATE_MAX) {
            res += UtfLength::THREE;
        } else {
            if (i < length - 1 &&
                utf16[i + 1] >= utf::LO_SURROGATE_MIN &&  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                utf16[i + 1] <= utf::LO_SURROGATE_MAX) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                res += UtfLength::FOUR;
                ++i;
            } else {
                res += UtfLength::THREE;
            }
        }
    }
    return res;
}

size_t ConvertRegionUtf16ToUtf8(const uint16_t *utf16In, uint8_t *utf8Out, size_t utf16Len, size_t utf8Len,
                                size_t start, bool modify)
{
    size_t utf8Pos = 0;
    if (utf16In == nullptr || utf8Out == nullptr || utf8Len == 0) {
        return 0;
    }
    size_t end = start + utf16Len;
    for (size_t i = start; i < end; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        uint16_t next16Code = 0;
        if ((i + 1) != end && utf::IsAvailableNextUtf16Code(utf16In[i + 1])) {
            next16Code = utf16In[i + 1];
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        Utf8Char ch = ConvertUtf16ToUtf8(utf16In[i], next16Code, modify);
        if (utf8Pos + ch.n > utf8Len) {
            break;
        }
        for (size_t c = 0; c < ch.n; ++c) {
            utf8Out[utf8Pos++] = ch.ch[c];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        if (ch.n == UtfLength::FOUR) {  // Two UTF-16 chars are used
            ++i;
        }
    }
    return utf8Pos;
}

std::pair<uint32_t, size_t> ConvertUtf8ToUtf16Pair(const uint8_t *data, bool combine)
{
    uint8_t d0 = data[0];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if ((d0 & utf::MASK1) == 0) {
        return {d0, 1};
    }

    uint8_t d1 = data[1];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if ((d0 & utf::MASK2) == 0) {
        return {((d0 & utf::MASK_5BIT) << utf::DATA_WIDTH) | (d1 & utf::MASK_6BIT), UtfLength::TWO};
    }

    uint8_t d2 = data[UtfLength::TWO];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if ((d0 & utf::MASK3) == 0) {
        return {((d0 & utf::MASK_4BIT) << UtfOffset::TWELVE) | ((d1 & utf::MASK_6BIT) << utf::DATA_WIDTH) |
                    (d2 & utf::MASK_6BIT),
                UtfLength::THREE};
    }

    uint8_t d3 = data[UtfLength::THREE];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uint32_t codePoint = ((d0 & utf::MASK_4BIT) << UtfOffset::EIGHTEEN) | ((d1 & utf::MASK_6BIT) << UtfOffset::TWELVE) |
                         ((d2 & utf::MASK_6BIT) << utf::DATA_WIDTH) | (d3 & utf::MASK_6BIT);

    uint32_t pair = 0;
    if (combine) {
        uint32_t lead = ((codePoint >> (utf::PAIR_ELEMENT_WIDTH - utf::DATA_WIDTH)) + utf::U16_LEAD);
        uint32_t tail = ((codePoint & utf::MASK_10BIT) + utf::U16_TAIL) & utf::MASK_16BIT;
        pair = U16_GET_SUPPLEMENTARY(lead, tail);  // NOLINTNEXTLINE(hicpp-signed-bitwise)
    } else {
        pair |= ((codePoint >> (utf::PAIR_ELEMENT_WIDTH - utf::DATA_WIDTH)) + utf::U16_LEAD) << utf::PAIR_ELEMENT_WIDTH;
        pair |= ((codePoint & utf::MASK_10BIT) + utf::U16_TAIL) & utf::MASK_16BIT;
    }

    return {pair, UtfLength::FOUR};
}

size_t Utf8ToUtf16Size(const uint8_t *utf8, size_t utf8Len)
{
    return utf::MUtf8ToUtf16Size(utf8, utf8Len);
}

size_t ConvertRegionUtf8ToUtf16(const uint8_t *utf8In, uint16_t *utf16Out, size_t utf8Len, size_t utf16Len,
                                size_t start)
{
    return utf::ConvertRegionMUtf8ToUtf16(utf8In, utf16Out, utf8Len, utf16Len, start);
}
}  // namespace panda::ecmascript::base::utf_helper
