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

#ifndef ECMASCRIPT_BASE_UTF_HELPER_H
#define ECMASCRIPT_BASE_UTF_HELPER_H

#include <cstdint>
#include <vector>

#include "libpandabase/utils/utf.h"

namespace panda::ecmascript::base::utf_helper {
static constexpr uint16_t DECODE_LEAD_LOW = 0xD800;
static constexpr uint16_t DECODE_LEAD_HIGH = 0xDBFF;
static constexpr uint16_t DECODE_TRAIL_LOW = 0xDC00;
static constexpr uint16_t DECODE_TRAIL_HIGH = 0xDFFF;
static constexpr uint32_t DECODE_FIRST_FACTOR = 0x400;
static constexpr uint32_t DECODE_SECOND_FACTOR = 0x10000;

static constexpr uint8_t BIT_MASK_1 = 0x80;
static constexpr uint8_t BIT_MASK_2 = 0xC0;
static constexpr uint8_t BIT_MASK_3 = 0xE0;
static constexpr uint8_t BIT_MASK_4 = 0xF0;
static constexpr uint8_t BIT_MASK_5 = 0xF8;

static constexpr uint8_t UTF8_1B_MAX = 0x7f;

static constexpr uint16_t UTF8_2B_MAX = 0x7ff;
static constexpr uint8_t UTF8_2B_FIRST = 0xc0;
static constexpr uint8_t UTF8_2B_SECOND = 0x80;

static constexpr uint8_t UTF8_3B_FIRST = 0xe0;
static constexpr uint8_t UTF8_3B_SECOND = 0x80;
static constexpr uint8_t UTF8_3B_THIRD = 0x80;

static constexpr uint8_t UTF8_4B_FIRST = 0xf0;

enum UtfLength : uint8_t { ONE = 1, TWO = 2, THREE = 3, FOUR = 4 };
enum UtfOffset : uint8_t { SIX = 6, TEN = 10, TWELVE = 12, EIGHTEEN = 18 };

static constexpr size_t MAX_BYTES = 4;
struct Utf8Char {
    size_t n;
    std::array<uint8_t, MAX_BYTES> ch;
};

uint32_t UTF16Decode(uint16_t lead, uint16_t trail);

bool IsValidUTF8(const std::vector<uint8_t> &data);

Utf8Char ConvertUtf16ToUtf8(uint16_t d0, uint16_t d1, bool modify);

size_t Utf16ToUtf8Size(const uint16_t *utf16, uint32_t length, bool modify = true);

size_t ConvertRegionUtf16ToUtf8(const uint16_t *utf16In, uint8_t *utf8Out, size_t utf16Len, size_t utf8Len,
                                size_t start, bool modify = true);

std::pair<uint32_t, size_t> ConvertUtf8ToUtf16Pair(const uint8_t *data, bool combine = false);

size_t Utf8ToUtf16Size(const uint8_t *utf8);

size_t ConvertRegionUtf8ToUtf16(const uint8_t *utf8In, uint16_t *utf16Out, size_t utf16Len, size_t start);

static inline uint32_t CombineTwoU16(uint16_t d0, uint16_t d1)
{
    uint32_t codePoint = d0 - utf::HI_SURROGATE_MIN;
    codePoint <<= UtfOffset::TEN;
    codePoint |= d1 - utf::LO_SURROGATE_MIN;
    codePoint += utf::LO_SUPPLEMENTS_MIN;
    return codePoint;
}
}  // namespace panda::ecmascript::base::utf_helper

#endif  // ECMASCRIPT_BASE_UTF_HELPER_H