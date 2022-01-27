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

#ifndef ECMASCRIPT_STRING_INL_H
#define ECMASCRIPT_STRING_INL_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
/* static */
inline EcmaString *EcmaString::Cast(ObjectHeader *object)
{
    ASSERT(JSTaggedValue(object).IsString());
    return static_cast<EcmaString *>(object);
}

/* static */
inline const EcmaString *EcmaString::ConstCast(const TaggedObject *object)
{
    ASSERT(JSTaggedValue(object).IsString());
    return static_cast<const EcmaString *>(object);
}

/* static */
inline EcmaString *EcmaString::CreateEmptyString(const EcmaVM *vm)
{
    auto string = vm->GetFactory()->AllocNonMovableStringObject(EcmaString::SIZE);
    string->SetLength(0, GetCompressedStringsEnabled());
    string->SetRawHashcode(0);
    return string;
}

/* static */
inline EcmaString *EcmaString::CreateFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len, const EcmaVM *vm,
                                              bool canBeCompress)
{
    if (utf8Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    EcmaString *string = nullptr;
    if (canBeCompress) {
        string = AllocStringObject(utf8Len, true, vm);
        ASSERT(string != nullptr);

        if (memcpy_s(string->GetDataUtf8Writable(), utf8Len, utf8Data, utf8Len) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    } else {
        auto utf16Len = base::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        string = AllocStringObject(utf16Len, false, vm);
        ASSERT(string != nullptr);

        [[maybe_unused]] auto len =
            base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, string->GetDataUtf16Writable(), utf8Len, utf16Len, 0);
        ASSERT(len == utf16Len);
    }

    return string;
}

inline EcmaString *EcmaString::CreateFromUtf16(const uint16_t *utf16Data, uint32_t utf16Len, const EcmaVM *vm,
                                               bool canBeCompress)
{
    if (utf16Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    auto string = AllocStringObject(utf16Len, canBeCompress, vm);
    ASSERT(string != nullptr);

    if (canBeCompress) {
        CopyUtf16AsUtf8(utf16Data, string->GetDataUtf8Writable(), utf16Len);
    } else {
        uint32_t len = utf16Len * (sizeof(uint16_t) / sizeof(uint8_t));
        if (memcpy_s(string->GetDataUtf16Writable(), len, utf16Data, len) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }

    return string;
}

template<bool verify>
inline uint16_t EcmaString::At(int32_t index) const
{
    int32_t length = GetLength();
    if (verify) {
        if ((index < 0) || (index >= length)) {
            return 0;
        }
    }
    if (!IsUtf16()) {
        Span<const uint8_t> sp(GetDataUtf8(), length);
        return sp[index];
    }
    Span<const uint16_t> sp(GetDataUtf16(), length);
    return sp[index];
}

/* static */
inline EcmaString *EcmaString::AllocStringObject(size_t length, bool compressed, const EcmaVM *vm)
{
    size_t size = compressed ? ComputeSizeUtf8(length) : ComputeSizeUtf16(length);
    auto string = reinterpret_cast<EcmaString *>(vm->GetFactory()->AllocStringObject(size));
    string->SetLength(length, compressed);
    string->SetRawHashcode(0);
    return reinterpret_cast<EcmaString *>(string);
}

void EcmaString::WriteData(EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length)
{
    if (IsUtf8()) {
        ASSERT(src->IsUtf8());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (length != 0 && memcpy_s(GetDataUtf8Writable() + start, destSize, src->GetDataUtf8(), length) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    } else if (src->IsUtf8()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        Span<uint16_t> to(GetDataUtf16Writable() + start, length);
        Span<const uint8_t> from(src->GetDataUtf8(), length);
        for (uint32_t i = 0; i < length; i++) {
            to[i] = from[i];
        }
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (length != 0 && memcpy_s(GetDataUtf16Writable() + start, destSize, src->GetDataUtf16(), length) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
}

void EcmaString::WriteData(char src, uint32_t start)
{
    if (IsUtf8()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *(GetDataUtf8Writable() + start) = src;
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *(GetDataUtf16Writable() + start) = src;
    }
}
}  // namespace panda::ecmascript

#endif
