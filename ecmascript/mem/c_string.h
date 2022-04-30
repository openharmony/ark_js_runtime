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

#ifndef ECMASCRIPT_MEM_C_STRING_H
#define ECMASCRIPT_MEM_C_STRING_H

#include <sstream>
#include <string>
#include <string_view>

#include "ecmascript/common.h"
#include "ecmascript/mem/caddress_allocator.h"

namespace panda::ecmascript {
class EcmaString;
class JSTaggedValue;

using CString = std::basic_string<char, std::char_traits<char>, CAddressAllocator<char>>;
using CStringStream = std::basic_stringstream<char, std::char_traits<char>, CAddressAllocator<char>>;

struct CStringHash {
    using argument_type = panda::ecmascript::CString;
    using result_type = std::size_t;

    size_t operator()(const CString &str) const noexcept
    {
        return std::hash<std::string_view>()(std::string_view(str.data(), str.size()));
    }
};

constexpr int BASE = 10;

// PRINT will skip '\0' in utf16 during conversion of utf8
enum StringConvertedUsage { PRINT, LOGICOPERATION };

long CStringToL(const CString &str);
int64_t CStringToLL(const CString &str);
uint64_t CStringToULL(const CString &str);
float CStringToF(const CString &str);
double CStringToD(const CString &str);

CString ConvertToString(const std::string &str);
std::string CstringConvertToStdString(const CString &str);

// '\u0000' is skip according to holdZero
CString ConvertToString(const ecmascript::EcmaString *s, StringConvertedUsage usage = StringConvertedUsage::PRINT);
CString ConvertToString(ecmascript::JSTaggedValue key);

template<class T>
std::enable_if_t<std::is_floating_point_v<T>, CString> FloatToCString(T number)
{
    CStringStream strStream;
    strStream << number;
    return strStream.str();
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, CString> ToCString(T number)
{
    if (number == 0) {
        return CString("0");
    }
    bool IsNeg = false;
    if (number < 0) {
        number = -number;
        IsNeg = true;
    }

    static constexpr uint32_t BUFF_SIZE = std::numeric_limits<T>::digits10 + 3;  // 3: Reserved for sign bit and '\0'.
    char buf[BUFF_SIZE];
    uint32_t position = BUFF_SIZE - 1;
    buf[position] = '\0';
    while (number > 0) {
        buf[--position] = static_cast<uint8_t>(number % 10 + '0'); // 10 : decimal
        number /= 10; // 10 : decimal
    }
    if (IsNeg) {
        buf[--position] = '-';
    }
    return CString(&buf[position]);
}
}  // namespace panda::ecmascript

namespace std {
template <>
struct hash<panda::ecmascript::CString> {
    using argument_type = panda::ecmascript::CStringHash::argument_type;
    using result_type = panda::ecmascript::CStringHash::result_type;

    size_t operator()(const panda::ecmascript::CString &str) const noexcept
    {
        return panda::ecmascript::CStringHash()(str);
    }
};
}  // namespace std

#endif  // ECMASCRIPT_MEM_C_STRING_H
