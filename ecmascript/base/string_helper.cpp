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

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"

namespace panda::ecmascript::base {
std::string StringHelper::ToStdString(EcmaString *string)
{
    return CstringConvertToString(ConvertToString(string));
}

bool StringHelper::CheckDuplicate(EcmaString *string)
{
    if (string->IsUtf8()) {
        const uint8_t *array = string->GetDataUtf8();
        size_t length = string->GetUtf8Length() - 1;
        std::bitset<UINT8_MAX> bitSet;
        for (size_t i = 0; i < length; ++i) {
            char idx = *array;
            if (bitSet.test(idx)) {
                return true;
            }
            bitSet.set(idx);
            array++;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    } else {
        UNREACHABLE();
    }
    return false;
}

EcmaString *StringHelper::Repeat(JSThread *thread, const std::u16string &thisStr, int32_t repeatLen, bool canBeCompress)
{
    ecmascript::ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (repeatLen == 0) {
        return *factory->GetEmptyString();  // Create empty EcmaString.
    }
    std::u16string tmpStr = thisStr;
    for (int32_t i = 1; i < repeatLen; i++) {
        tmpStr.append(thisStr);
    }
    const char16_t *constChar16tData = tmpStr.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    int32_t length = tmpStr.size();
    return *factory->NewFromUtf16UnCheck(uint16tData, length, canBeCompress);
}

EcmaString *StringHelper::Trim(JSThread *thread, const std::u16string &thisStr)
{
    ecmascript::ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string tmpStr = thisStr;
    if (tmpStr.empty()) {
        return *factory->GetEmptyString();
    }
    std::string str = U16stringToString(tmpStr);
    std::wstring wstr = StringToWstring(str);
    std::wregex r(
        L"^["
        L"\u0009\u000A\u000B\u000C\u000D\u0020\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007"
        L"\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\uFEFF]+|["
        L"\u0009\u000A\u000B\u000C\u000D\u0020\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007"
        L"\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\uFEFF]+$");
    wstr = regex_replace(wstr, r, L"");
    str = WstringToString(wstr);
    tmpStr = StringToU16string(str);
    const char16_t *constChar16tData = tmpStr.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    int32_t length = tmpStr.size();
    return *factory->NewFromUtf16(uint16tData, length);
}
}  // namespace panda::ecmascript::base
