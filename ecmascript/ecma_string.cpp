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

#include "ecmascript/ecma_string-inl.h"

#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
bool EcmaString::compressedStringsEnabled = true;
static constexpr int SMALL_STRING_SIZE = 128;

EcmaString *EcmaString::Concat(const JSHandle<EcmaString> &str1Handle, const JSHandle<EcmaString> &str2Handle,
                               const EcmaVM *vm)
{
    // allocator may trig gc and move src, need to hold it
    EcmaString *string1 = *str1Handle;
    EcmaString *string2 = *str2Handle;

    uint32_t length1 = string1->GetLength();

    uint32_t length2 = string2->GetLength();
    uint32_t newLength = length1 + length2;
    if (newLength == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    bool compressed = GetCompressedStringsEnabled() && (!string1->IsUtf16() && !string2->IsUtf16());
    auto newString = AllocStringObject(newLength, compressed, vm);

    // retrieve strings after gc
    string1 = *str1Handle;
    string2 = *str2Handle;
    if (compressed) {
        Span<uint8_t> sp(newString->GetDataUtf8Writable(), newLength);
        Span<const uint8_t> src1(string1->GetDataUtf8(), length1);
        EcmaString::StringCopy(sp, newLength, src1, length1);

        sp = sp.SubSpan(length1);
        Span<const uint8_t> src2(string2->GetDataUtf8(), length2);
        EcmaString::StringCopy(sp, newLength - length1, src2, length2);
    } else {
        Span<uint16_t> sp(newString->GetDataUtf16Writable(), newLength);
        if (!string1->IsUtf16()) {
            for (uint32_t i = 0; i < length1; ++i) {
                sp[i] = string1->At<false>(i);
            }
        } else {
            Span<const uint16_t> src1(string1->GetDataUtf16(), length1);
            EcmaString::StringCopy(sp, newLength << 1U, src1, length1 << 1U);
        }
        sp = sp.SubSpan(length1);
        if (!string2->IsUtf16()) {
            for (uint32_t i = 0; i < length2; ++i) {
                sp[i] = string2->At<false>(i);
            }
        } else {
            uint32_t length = length2 << 1U;
            Span<const uint16_t> src2(string2->GetDataUtf16(), length2);
            EcmaString::StringCopy(sp, length, src2, length);
        }
    }

    return newString;
}

/* static */
EcmaString *EcmaString::FastSubString(const JSHandle<EcmaString> &src, uint32_t start, uint32_t utf16Len,
                                      const EcmaVM *vm)
{
    if (utf16Len == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    bool canBeCompressed = !src->IsUtf16();

    // allocator may trig gc and move src, need to hold it
    auto string = AllocStringObject(utf16Len, canBeCompressed, vm);

    if (src->IsUtf16()) {
        if (canBeCompressed) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            CopyUtf16AsUtf8(src->GetDataUtf16() + start, string->GetDataUtf8Writable(), utf16Len);
        } else {
            uint32_t len = utf16Len * (sizeof(uint16_t) / sizeof(uint8_t));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            Span<uint16_t> dst(string->GetDataUtf16Writable(), utf16Len);
            Span<const uint16_t> source(src->GetDataUtf16() + start, utf16Len);
            EcmaString::StringCopy(dst, len, source, len);
        }
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        Span<uint8_t> dst(string->GetDataUtf8Writable(), utf16Len);
        Span<const uint8_t> source(src->GetDataUtf8() + start, utf16Len);
        EcmaString::StringCopy(dst, utf16Len, source, utf16Len);
    }
    return string;
}

template<typename T1, typename T2>
int32_t CompareStringSpan(Span<T1> &lhsSp, Span<T2> &rhsSp, int32_t count)
{
    for (int32_t i = 0; i < count; ++i) {
        auto left = static_cast<int32_t>(lhsSp[i]);
        auto right = static_cast<int32_t>(rhsSp[i]);
        if (left != right) {
            return left - right;
        }
    }
    return 0;
}

int32_t EcmaString::Compare(const EcmaString *rhs) const
{
    const EcmaString *lhs = this;
    if (lhs == rhs) {
        return 0;
    }
    int32_t lhsCount = lhs->GetLength();
    int32_t rhsCount = rhs->GetLength();
    int32_t countDiff = lhsCount - rhsCount;
    int32_t minCount = (countDiff < 0) ? lhsCount : rhsCount;
    if (!lhs->IsUtf16() && !rhs->IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!lhs->IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!rhs->IsUtf16()) {
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), rhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), lhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else {
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    }
    return countDiff;
}

/* static */
template<typename T1, typename T2>
int32_t EcmaString::IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max)
{
    ASSERT(rhsSp.size() > 0);
    auto first = static_cast<int32_t>(rhsSp[0]);
    int32_t i;
    for (i = pos; i <= max; i++) {
        if (static_cast<int32_t>(lhsSp[i]) != first) {
            i++;
            while (i <= max && static_cast<int32_t>(lhsSp[i]) != first) {
                i++;
            }
        }
        /* Found first character, now look at the rest of rhsSp */
        if (i <= max) {
            int j = i + 1;
            int end = j + rhsSp.size() - 1;

            for (int k = 1; j < end && static_cast<int32_t>(lhsSp[j]) == static_cast<int32_t>(rhsSp[k]); j++, k++) {
            }
            if (j == end) {
                /* Found whole string. */
                return i;
            }
        }
    }
    return -1;
}

int32_t EcmaString::IndexOf(const EcmaString *rhs, int32_t pos) const
{
    if (rhs == nullptr) {
        return -1;
    }
    const EcmaString *lhs = this;
    int32_t lhsCount = lhs->GetLength();
    int32_t rhsCount = rhs->GetLength();
    if (rhsCount == 0) {
        return pos;
    }

    if (pos >= lhsCount) {
        return -1;
    }

    if (pos < 0) {
        pos = 0;
    }

    int32_t max = lhsCount - rhsCount;
    if (max < 0) {
        return -1;
    }
    if (rhs->IsUtf8() && lhs->IsUtf8()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else if (rhs->IsUtf16() && lhs->IsUtf16()) {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else if (rhs->IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    }

    return -1;
}

// static
bool EcmaString::CanBeCompressed(const uint8_t *utf8Data)
{
    if (!compressedStringsEnabled) {
        return false;
    }
    bool isCompressed = true;
    int index = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    while (utf8Data[index] != '\0') {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (!IsASCIICharacter(utf8Data[index])) {
            isCompressed = false;
            break;
        }
        ++index;
    }
    return isCompressed;
}

/* static */
bool EcmaString::CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len)
{
    if (!compressedStringsEnabled) {
        return false;
    }
    bool isCompressed = true;
    Span<const uint16_t> data(utf16Data, utf16Len);
    for (uint32_t i = 0; i < utf16Len; i++) {
        if (!IsASCIICharacter(data[i])) {
            isCompressed = false;
            break;
        }
    }
    return isCompressed;
}

/* static */
void EcmaString::CopyUtf16AsUtf8(const uint16_t *utf16From, uint8_t *utf8To, uint32_t utf16Len)
{
    Span<const uint16_t> from(utf16From, utf16Len);
    Span<uint8_t> to(utf8To, utf16Len);
    for (uint32_t i = 0; i < utf16Len; i++) {
        to[i] = from[i];
    }
}

/* static */
bool EcmaString::StringsAreEqual(EcmaString *str1, EcmaString *str2)
{
    if ((str1->IsUtf16() != str2->IsUtf16()) || (str1->GetLength() != str2->GetLength()) ||
        (str1->GetHashcode() != str2->GetHashcode())) {
        return false;
    }

    if (str1->IsUtf16()) {
        Span<const uint16_t> data1(str1->GetDataUtf16(), str1->GetLength());
        Span<const uint16_t> data2(str2->GetDataUtf16(), str1->GetLength());
        return EcmaString::StringsAreEquals(data1, data2);
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint8_t> data1(str1->GetDataUtf8(), str1->GetLength());
        Span<const uint8_t> data2(str2->GetDataUtf8(), str1->GetLength());
        return EcmaString::StringsAreEquals(data1, data2);
    }
}

/* static */
bool EcmaString::StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                     bool canBeCompress)
{
    if (canBeCompress != str1->IsUtf8()) {
        return false;
    }

    if (canBeCompress && str1->GetLength() != utf8Len) {
        return false;
    }

    if (canBeCompress) {
        Span<const uint8_t> data1(str1->GetDataUtf8(), utf8Len);
        Span<const uint8_t> data2(utf8Data, utf8Len);
        return EcmaString::StringsAreEquals(data1, data2);
    }
    return IsUtf8EqualsUtf16(utf8Data, utf8Len, str1->GetDataUtf16(), str1->GetLength());
}

/* static */
bool EcmaString::StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)
{
    bool result = false;
    if (str1->GetLength() != utf16Len) {
        result = false;
    } else if (!str1->IsUtf16()) {
        result = IsUtf8EqualsUtf16(str1->GetDataUtf8(), str1->GetLength(), utf16Data, utf16Len);
    } else {
        Span<const uint16_t> data1(str1->GetDataUtf16(), str1->GetLength());
        Span<const uint16_t> data2(utf16Data, utf16Len);
        result = EcmaString::StringsAreEquals(data1, data2);
    }
    return result;
}

/* static */
template<typename T>
bool EcmaString::StringsAreEquals(Span<const T> &str1, Span<const T> &str2)
{
    ASSERT(str1.Size() <= str2.Size());
    size_t size = str1.Size();
    if (size < SMALL_STRING_SIZE) {
        for (size_t i = 0; i < size; i++) {
            if (str1[i] != str2[i]) {
                return false;
            }
        }
        return true;
    }
    return !memcmp(str1.data(), str2.data(), size);
}

template<typename T>
bool EcmaString::StringCopy(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count)
{
    ASSERT(dstMax >= count);
    ASSERT(dst.Size() >= src.Size());
    if (src.Size() < SMALL_STRING_SIZE) {
        for (size_t i = 0; i < src.Size(); i++) {
            dst[i] = src[i];
        }
        return true;
    }
    if (memcpy_s(dst.data(), dstMax, src.data(), count) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    return true;
}

template<class T>
static int32_t ComputeHashForData(const T *data, size_t size)
{
    uint32_t hash = 0;
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
    Span<const T> sp(data, size);
#pragma GCC diagnostic pop
#endif
    for (auto c : sp) {
        constexpr size_t SHIFT = 5;
        hash = (hash << SHIFT) - hash + c;
    }
    return static_cast<int32_t>(hash);
}

static int32_t ComputeHashForUtf8(const uint8_t *utf8Data)
{
    if (utf8Data == nullptr) {
        return 0;
    }
    uint32_t hash = 0;
    while (*utf8Data != '\0') {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        constexpr size_t SHIFT = 5;
        hash = (hash << SHIFT) - hash + *utf8Data++;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return static_cast<int32_t>(hash);
}

uint32_t EcmaString::ComputeHashcode() const
{
    uint32_t hash;
    if (compressedStringsEnabled) {
        if (!IsUtf16()) {
            hash = ComputeHashForData(GetDataUtf8(), GetLength());
        } else {
            hash = ComputeHashForData(GetDataUtf16(), GetLength());
        }
    } else {
        ASSERT(static_cast<size_t>(GetLength())<(std::numeric_limits<size_t>::max()>>1U));
        hash = ComputeHashForData(GetDataUtf16(), GetLength());
    }
    return hash;
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
{
    uint32_t hash;
    if (canBeCompress) {
        hash = ComputeHashForUtf8(utf8Data);
    } else {
        auto utf16Len = base::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        CVector<uint16_t> tmpBuffer(utf16Len);
        [[maybe_unused]] auto len = base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len,
                                                                               utf16Len, 0);
        ASSERT(len == utf16Len);
        hash = ComputeHashForData(tmpBuffer.data(), utf16Len);
    }
    return hash;
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length)
{
    return ComputeHashForData(utf16Data, length);
}

/* static */
bool EcmaString::IsUtf8EqualsUtf16(const uint8_t *utf8Data, size_t utf8Len, const uint16_t *utf16Data,
                                   uint32_t utf16Len)
{
    // length is one more than compared utf16Data, don't need convert all utf8Data to utf16Data
    uint32_t utf8ConvertLength = utf16Len + 1;
    CVector<uint16_t> tmpBuffer(utf8ConvertLength);
    auto len = base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len, utf8ConvertLength, 0);
    if (len != utf16Len) {
        return false;
    }

    Span<const uint16_t> data1(tmpBuffer.data(), len);
    Span<const uint16_t> data2(utf16Data, utf16Len);
    return EcmaString::StringsAreEquals(data1, data2);
}
}  // namespace panda::ecmascript
