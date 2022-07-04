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

#ifndef ECMASCRIPT_REGEXP_PARSER_CACHE_H
#define ECMASCRIPT_REGEXP_PARSER_CACHE_H

#include <array>

#include "ecmascript/ecma_string.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class RegExpParserCache {
public:
    RegExpParserCache();
    ~RegExpParserCache();
    static constexpr size_t CACHE_SIZE = 128;

    bool IsInCache(EcmaString *pattern, const uint32_t flags);
    std::pair<JSTaggedValue, size_t> GetCache(EcmaString *pattern, const uint32_t flags,
                                              CVector<CString> &groupName);
    void SetCache(EcmaString *pattern, const uint32_t flags, const JSTaggedValue codeBuffer,
                  const size_t bufferSize, CVector<CString> groupName);
    void Clear();

private:
    size_t GetHash(EcmaString *pattern, const uint32_t flags);

    struct ParserKey {
        EcmaString *pattern_{nullptr};
        uint32_t flags_{UINT32_MAX};
        JSTaggedValue codeBuffer_{JSTaggedValue::Hole()};
        size_t bufferSize_{0};
        CVector<CString> newGroupNames_;
    };

    std::array<ParserKey, CACHE_SIZE> info_{};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_REGEXP_PARSER_CACHE_H
