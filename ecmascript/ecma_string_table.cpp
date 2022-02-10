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

#include "ecmascript/ecma_string_table.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
EcmaStringTable::EcmaStringTable(const EcmaVM *vm) : vm_(vm) {}

EcmaString *EcmaStringTable::GetString(const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString) const
{
    uint32_t hashCode = firstString->GetHashcode();
    hashCode = secondString->ComputeHashcode(hashCode);
    auto range = table_.equal_range(hashCode);
    auto item = range.first;
    for (; item != range.second; ++item) {
        auto foundString = item->second;
        if (foundString->EqualToSplicedString(*firstString, *secondString)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress) const
{
    uint32_t hashCode = EcmaString::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    auto range = table_.equal_range(hashCode);
    auto item = range.first;
    for (; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaString::StringsAreEqualUtf8(foundString, utf8Data, utf8Len, canBeCompress)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(const uint16_t *utf16Data, uint32_t utf16Len) const
{
    uint32_t hashCode = EcmaString::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    auto range = table_.equal_range(hashCode);
    auto item = range.first;
    for (; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaString::StringsAreEqualUtf16(foundString, utf16Data, utf16Len)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(EcmaString *string) const
{
    auto range = table_.equal_range(string->GetHashcode());
    auto item = range.first;
    for (; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaString::StringsAreEqual(foundString, string)) {
            return foundString;
        }
    }
    return nullptr;
}

void EcmaStringTable::InternString(EcmaString *string)
{
    if (string->IsInternString()) {
        return;
    }
    table_.insert(std::pair<uint32_t, EcmaString *>(string->GetHashcode(), string));
    string->SetIsInternString();
}

void EcmaStringTable::InternEmptyString(EcmaString *emptyStr)
{
    InternString(emptyStr);
}

EcmaString *EcmaStringTable::GetOrInternString(const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString)
{
    if (firstString->IsInternString() && secondString->IsInternString()) {
        EcmaString *concatString = GetString(firstString, secondString);
        if (concatString != nullptr) {
            return concatString;
        }
    }
    EcmaString *concatString = EcmaString::Concat(firstString, secondString, vm_);

    InternString(concatString);
    return concatString;
}

EcmaString *EcmaStringTable::GetOrInternString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress)
{
    EcmaString *result = GetString(utf8Data, utf8Len, canBeCompress);
    if (result != nullptr) {
        return result;
    }

    result = EcmaString::CreateFromUtf8(utf8Data, utf8Len, vm_, canBeCompress);
    InternString(result);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress)
{
    EcmaString *result = GetString(utf16Data, utf16Len);
    if (result != nullptr) {
        return result;
    }

    result = EcmaString::CreateFromUtf16(utf16Data, utf16Len, vm_, canBeCompress);
    InternString(result);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaString *string)
{
    if (string->IsInternString()) {
        return string;
    }

    EcmaString *result = GetString(string);
    if (result != nullptr) {
        return result;
    }
    InternString(string);
    return string;
}

void EcmaStringTable::SweepWeakReference(const WeakRootVisitor &visitor)
{
    for (auto it = table_.begin(); it != table_.end();) {
        auto *object = it->second;
        auto fwd = visitor(object);
        if (fwd == nullptr) {
            LOG(DEBUG, GC) << "StringTable: delete string " << std::hex << object
                           << ", val = " << ConvertToString(object);
            table_.erase(it++);
        } else if (fwd != object) {
            it->second = static_cast<EcmaString *>(fwd);
            ++it;
            LOG(DEBUG, GC) << "StringTable: forward " << std::hex << object << " -> " << fwd;
        } else {
            ++it;
        }
    }
}
}  // namespace panda::ecmascript
