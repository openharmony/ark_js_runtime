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

#include "ecmascript/regexp/regexp_parser_cache.h"

namespace panda::ecmascript {
RegExpParserCache::RegExpParserCache()
{
    Clear();
}

RegExpParserCache::~RegExpParserCache()
{
    Clear();
}

void RegExpParserCache::Clear()
{
    for (ParserKey &info : info_) {
        info.pattern_ = nullptr;
        info.flags_ = UINT32_MAX; // flags cannot be UINT32_MAX, so it means invalid.
        info.codeBuffer_ = JSTaggedValue::Hole();
        info.bufferSize_ = 0;
    }
}

size_t RegExpParserCache::GetHash(EcmaString *pattern, const uint32_t flags)
{
    return (pattern->GetHashcode() ^ flags) % CACHE_SIZE;
}

std::pair<JSTaggedValue, size_t> RegExpParserCache::GetCache(EcmaString *pattern, const uint32_t flags)
{
    size_t hash = GetHash(pattern, flags);
    ParserKey &info = info_[hash];
    if (info.flags_ != flags || !EcmaString::StringsAreEqual(info.pattern_, pattern)) {
        return std::pair<JSTaggedValue, size_t>(JSTaggedValue::Hole(), 0);
    }
    return std::pair<JSTaggedValue, size_t>(info.codeBuffer_, info.bufferSize_);
}

void RegExpParserCache::SetCache(EcmaString *pattern, const uint32_t flags,
                                 const JSTaggedValue codeBuffer, const size_t bufferSize)
{
    size_t hash = GetHash(pattern, flags);
    ParserKey &info = info_[hash];
    info.pattern_ = pattern;
    info.flags_ = flags;
    info.codeBuffer_ = codeBuffer;
    info.bufferSize_ = bufferSize;
}
}  // namespace panda::ecmascript