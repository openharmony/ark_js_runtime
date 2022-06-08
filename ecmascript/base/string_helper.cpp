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
    return CstringConvertToStdString(ConvertToString(string));
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
    uint32_t length = tmpStr.size();
    return canBeCompress ? *factory->NewFromUtf16Compress(uint16tData, length) :
                           *factory->NewFromUtf16NotCompress(uint16tData, length);
}
}  // namespace panda::ecmascript::base
