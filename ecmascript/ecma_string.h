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

#ifndef ECMASCRIPT_STRING_H
#define ECMASCRIPT_STRING_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ecmascript/base/utf_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda {
namespace ecmascript {
template<typename T>
class JSHandle;
class EcmaVM;

class EcmaString : public TaggedObject {
public:
    static EcmaString *Cast(ObjectHeader *object);
    static const EcmaString *ConstCast(const TaggedObject *object);

    static EcmaString *CreateEmptyString(const EcmaVM *vm);
    static EcmaString *CreateFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len, const EcmaVM *vm, bool canBeCompress);
    static EcmaString *CreateFromUtf16(const uint16_t *utf16Data, uint32_t utf16Len, const EcmaVM *vm,
                                       bool canBeCompress);
    static EcmaString *Concat(const JSHandle<EcmaString> &str1Handle, const JSHandle<EcmaString> &str2Handle,
                              const EcmaVM *vm);
    static EcmaString *FastSubString(const JSHandle<EcmaString> &src, uint32_t start, uint32_t utf16Len,
                                     const EcmaVM *vm);

    static constexpr uint32_t STRING_COMPRESSED_BIT = 0x1;
    static constexpr uint32_t STRING_INTERN_BIT = 0x2;
    enum CompressedStatus {
        STRING_COMPRESSED,
        STRING_UNCOMPRESSED,
    };

    template<bool verify = true>
    uint16_t At(int32_t index) const;

    int32_t Compare(const EcmaString *rhs) const;

    bool IsUtf16() const
    {
        return compressedStringsEnabled ? ((length_ & STRING_COMPRESSED_BIT) == STRING_UNCOMPRESSED) : true;
    }

    bool IsUtf8() const
    {
        return compressedStringsEnabled ? ((length_ & STRING_COMPRESSED_BIT) == STRING_COMPRESSED) : false;
    }

    static size_t ComputeDataSizeUtf16(uint32_t length)
    {
        return length * sizeof(dataUtf16_[0]);
    }

    /**
     * Methods for uncompressed strings (UTF16):
     */
    static size_t ComputeSizeUtf16(uint32_t utf16Len)
    {
        return sizeof(EcmaString) + ComputeDataSizeUtf16(utf16Len);
    }

    const uint16_t *GetDataUtf16() const
    {
        LOG_IF(!IsUtf16(), FATAL, RUNTIME) << "EcmaString: Read data as utf16 for utf8 string";
        return dataUtf16_;
    }

    /**
     * Methods for compresses strings (UTF8 or LATIN1):
     */
    static size_t ComputeSizeUtf8(uint32_t utf8Len)
    {
        return sizeof(EcmaString) + utf8Len;
    }

    /**
     * It's Utf8 format, but without 0 in the end.
     */
    const uint8_t *GetDataUtf8() const
    {
        LOG_IF(IsUtf16(), FATAL, RUNTIME) << "EcmaString: Read data as utf8 for utf16 string";
        return reinterpret_cast<const uint8_t *>(dataUtf16_);
    }

    size_t GetUtf8Length() const
    {
        if (!IsUtf16()) {
            return GetLength() + 1;  // add place for zero in the end
        }
        return base::utf_helper::Utf16ToUtf8Size(dataUtf16_, GetLength());
    }

    size_t GetUtf16Length() const
    {
        return GetLength();
    }

    inline size_t CopyDataUtf8(uint8_t *buf, size_t maxLength) const
    {
        ASSERT(maxLength > 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        buf[maxLength - 1] = '\0';
        return CopyDataRegionUtf8(buf, 0, GetLength(), maxLength) + 1;  // add place for zero in the end
    }

    size_t CopyDataRegionUtf8(uint8_t *buf, size_t start, size_t length, size_t maxLength) const
    {
        if (length > maxLength) {
            return 0;
        }
        uint32_t len = GetLength();
        if (start + length > len) {
            return 0;
        }
        if (!IsUtf16()) {
            if (length > std::numeric_limits<size_t>::max() / 2 - 1) {  // 2: half
                LOG(FATAL, RUNTIME) << " length is higher than half of size_t::max";
                UNREACHABLE();
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(buf, maxLength, GetDataUtf8() + start, length) != EOK) {
                LOG(FATAL, RUNTIME) << "memcpy_s failed";
                UNREACHABLE();
            }
            return length;
        }
        return base::utf_helper::ConvertRegionUtf16ToUtf8(GetDataUtf16(), buf, length, maxLength - 1, start);
    }

    inline uint32_t CopyDataUtf16(uint16_t *buf, uint32_t maxLength) const
    {
        return CopyDataRegionUtf16(buf, 0, GetLength(), maxLength);
    }

    uint32_t CopyDataRegionUtf16(uint16_t *buf, uint32_t start, uint32_t length, uint32_t maxLength) const
    {
        if (length > maxLength) {
            return 0;
        }
        uint32_t len = GetLength();
        if (start + length > len) {
            return 0;
        }
        if (IsUtf16()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(buf, ComputeDataSizeUtf16(maxLength), GetDataUtf16() + start, ComputeDataSizeUtf16(length)) !=
                EOK) {
                LOG(FATAL, RUNTIME) << "memcpy_s failed";
                UNREACHABLE();
            }
            return length;
        }
        return base::utf_helper::ConvertRegionUtf8ToUtf16(GetDataUtf8(), buf, len, maxLength, start);
    }

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    inline std::unique_ptr<char[]> GetCString()
    {
        auto length = GetUtf8Length();
        char *buf = new char[length]();
        CopyDataUtf8(reinterpret_cast<uint8_t *>(buf), length);
        // NOLINTNEXTLINE(modernize-avoid-c-arrays)
        return std::unique_ptr<char[]>(buf);
    }

    inline void WriteData(EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length);
    inline void WriteData(char src, uint32_t start);
    uint32_t GetLength() const
    {
        return length_ >> 2U;
    }

    void SetIsInternString()
    {
        length_ |= STRING_INTERN_BIT;
    }

    bool IsInternString() const
    {
        return (length_ & STRING_INTERN_BIT) != 0;
    }

    size_t ObjectSize() const
    {
        uint32_t length = GetLength();
        return IsUtf16() ? ComputeSizeUtf16(length) : ComputeSizeUtf8(length);
    }

    uint32_t GetHashcode()
    {
        if (hashcode_ == 0) {
            hashcode_ = ComputeHashcode();
        }
        return hashcode_;
    }

    int32_t IndexOf(const EcmaString *rhs, int pos = 0) const;

    static constexpr uint32_t GetLengthOffset()
    {
        return MEMBER_OFFSET(EcmaString, length_);
    }

    static constexpr uint32_t GetDataOffset()
    {
        return MEMBER_OFFSET(EcmaString, dataUtf16_);
    }

    static constexpr uint32_t GetStringCompressionMask()
    {
        return STRING_COMPRESSED_BIT;
    }

    /**
     * Compares strings by bytes, It doesn't check canonical unicode equivalence.
     */
    static bool StringsAreEqual(EcmaString *str1, EcmaString *str2);
    /**
     * Compares strings by bytes, It doesn't check canonical unicode equivalence.
     */
    static bool StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                    bool canBeCompress);
    /**
     * Compares strings by bytes, It doesn't check canonical unicode equivalence.
     */
    static bool StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len);
    static uint32_t ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress);
    static uint32_t ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length);

    static void SetCompressedStringsEnabled(bool val)
    {
        compressedStringsEnabled = val;
    }

    static bool GetCompressedStringsEnabled()
    {
        return compressedStringsEnabled;
    }

    static EcmaString *AllocStringObject(size_t length, bool compressed, const EcmaVM *vm);

    static bool CanBeCompressed(const uint8_t *utf8Data);
    static bool CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len);

private:
    void SetLength(uint32_t length, bool compressed = false)
    {
        ASSERT(length < 0x40000000U);
        // Use 0u for compressed/utf8 expression
        length_ = (length << 2U) | (compressed ? STRING_COMPRESSED : STRING_UNCOMPRESSED);
    }

    void SetHashcode(uint32_t hashcode)
    {
        hashcode_ = hashcode;
    }

    uint16_t *GetDataUtf16Writable()
    {
        LOG_IF(!IsUtf16(), FATAL, RUNTIME) << "EcmaString: Read data as utf16 for utf8 string";
        return dataUtf16_;
    }

    uint8_t *GetDataUtf8Writable()
    {
        LOG_IF(IsUtf16(), FATAL, RUNTIME) << "EcmaString: Read data as utf8 for utf16 string";
        return reinterpret_cast<uint8_t *>(dataUtf16_);
    }

    uint32_t ComputeHashcode() const;
    static void CopyUtf16AsUtf8(const uint16_t *utf16From, uint8_t *utf8To, uint32_t utf16Len);

    static bool compressedStringsEnabled;

    static bool IsASCIICharacter(uint16_t data)
    {
        // \0 is not considered ASCII in Ecma-Modified-UTF8 [only modify '\u0000']
        return data - 1U < base::utf_helper::UTF8_1B_MAX;
    }

    /**
     * str1 should have the same length as utf16_data.
     * Converts utf8Data to utf16 and compare it with given utf16_data.
     */
    static bool IsUtf8EqualsUtf16(const uint8_t *utf8Data, size_t utf8Len, const uint16_t *utf16Data,
                                  uint32_t utf16Len);

    template<typename T>
    /**
     * Check that two spans are equal. Should have the same length.
     */
    static bool StringsAreEquals(Span<const T> &str1, Span<const T> &str2);

    template<typename T>
    /**
     * Copy String from src to dst
     * */
    static bool StringCopy(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count);

    template<typename T1, typename T2>
    static int32_t IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max);

    // In last bit of length_ we store if this string is compressed or not.
    uint32_t length_;
    uint32_t hashcode_;
    // A pointer to the string data stored after the string header.
    // Data can be stored in utf8 or utf16 form according to compressed bit.
    __extension__ uint16_t dataUtf16_[0];  // NOLINT(modernize-avoid-c-arrays)
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_STRING_H
