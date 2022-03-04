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

#include "ecmascript/dfx/hprof/string_hashmap.h"

namespace panda::ecmascript {
CString *StringHashMap::FindOrInsertString(CString *string)
{
    StringKey key = GenerateStringKey(string);
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        return it->second;
    } else {  // NOLINT(readability-else-after-return)
        index_++;
        hashmap_.insert(std::make_pair(key, string));
        orderedKey_.emplace_back(key);
        indexMap_.insert(std::make_pair(key, index_));
        return string;
    }
}

StringId StringHashMap::GetStringId(const CString *string) const
{
    auto it = indexMap_.find(GenerateStringKey(string));
    return it != indexMap_.end() ? it->second : 1;  // ""
}

CString *StringHashMap::GetStringByKey(StringKey key) const
{
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        return FormatString(it->second);
    }
    return nullptr;
}

CString *StringHashMap::FormatString(CString *string) const
{
    // remove "\"" | "\r\n" | "\\" | "\t" | "\f"
    int length = string->length();
    char *charSeq = const_cast<char *>(string->c_str());
    for (int i = 0; i < length; i++) {
        if (charSeq[i] == '\"') {         // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            charSeq[i] = '`';             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (charSeq[i] == '\r') {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            charSeq[i] = '`';             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (charSeq[i] == '\n') {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            charSeq[i] = '`';             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (charSeq[i] == '\\') {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            charSeq[i] = '`';             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (charSeq[i] == '\t') {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            charSeq[i] = '`';             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (charSeq[i] == '\f') {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            charSeq[i] = '`';             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else if (charSeq[i] < ' ') {    // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            // ctrl chars 0~31
            charSeq[i] = '`';  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    }
    *string = charSeq;
    return string;
}

StringKey StringHashMap::GenerateStringKey(const CString *string) const
{
    return std::hash<std::string>{} (std::string(*string));
}

CString *StringHashMap::GetString(CString as)
{
    auto *tempString = const_cast<NativeAreaAllocator *>(heap_->GetNativeAreaAllocator())->New<CString>(std::move(as));
    CString *oldString = FindOrInsertString(tempString);
    if (tempString != oldString) {
        const_cast<NativeAreaAllocator *>(heap_->GetNativeAreaAllocator())->Delete(tempString);
        return oldString;
    }
    return tempString;
}

void StringHashMap::Clear()
{
    for (auto it : hashmap_) {
        if (it.second != nullptr) {
            const_cast<NativeAreaAllocator *>(heap_->GetNativeAreaAllocator())->Delete(it.second);
        }
    }
}
}  // namespace panda::ecmascript
