/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_GC_BITSET__H
#define ECMASCRIPT_MEM_GC_BITSET__H

#include <atomic>

#include "ecmascript/mem/mem.h"

// |----word(32 bit)----|----word(32 bit)----|----...----|----word(32 bit)----|----word(32 bit)----|
// |---------------------------------------GCBitset(4 kb)------------------------------------------|

namespace panda::ecmascript {
enum class AccessType { ATOMIC, NON_ATOMIC };

class GCBitset {
public:
    using GCBitsetWord = uint32_t;
    static constexpr uint32_t BYTE_PER_WORD = sizeof(GCBitsetWord);
    static constexpr uint32_t BYTE_PER_WORD_LOG2 = panda::helpers::math::GetIntLog2(BYTE_PER_WORD);
    static constexpr uint32_t BIT_PER_BYTE = 8;
    static constexpr uint32_t BIT_PER_BYTE_LOG2 = panda::helpers::math::GetIntLog2(BIT_PER_BYTE);
    static constexpr uint32_t BIT_PER_WORD = BYTE_PER_WORD * BIT_PER_BYTE;
    static constexpr uint32_t BIT_PER_WORD_LOG2 = panda::helpers::math::GetIntLog2(BIT_PER_WORD);
    static constexpr uint32_t BIT_PER_WORD_MASK = BIT_PER_WORD - 1;

    GCBitset() = default;
    ~GCBitset() = default;

    NO_COPY_SEMANTIC(GCBitset);
    NO_MOVE_SEMANTIC(GCBitset);

    static size_t SizeOfGCBitset(size_t heapSize)
    {
        size_t bitSize = AlignUp(heapSize, TAGGED_TYPE_SIZE) >> TAGGED_TYPE_SIZE_LOG;
        return AlignUp(AlignUp(bitSize, BIT_PER_BYTE) >> BIT_PER_BYTE_LOG2, BYTE_PER_WORD);
    }

    GCBitsetWord *Words()
    {
        return reinterpret_cast<GCBitsetWord *>(this);
    }

    const GCBitsetWord *Words() const
    {
        return reinterpret_cast<const GCBitsetWord *>(this);
    }

    void Clear(size_t bitSize)
    {
        GCBitsetWord *words = Words();
        uint32_t wordCount = static_cast<uint32_t>(WordCount(bitSize));
        for (uint32_t i = 0; i < wordCount; i++) {
            words[i] = 0;
        }
    }

    template <AccessType mode = AccessType::NON_ATOMIC>
    bool SetBit(uintptr_t offset);

    void ClearBit(uintptr_t offset)
    {
        Words()[Index(offset)] &= ~Mask(IndexInWord(offset));
    }

    template <AccessType mode = AccessType::NON_ATOMIC>
    void ClearBitRange(uintptr_t offsetBegin, uintptr_t offsetEnd)
    {
        GCBitsetWord *words = Words();
        uint32_t startIndex = Index(offsetBegin);
        uint32_t startIndexMask = Mask(IndexInWord(offsetBegin));
        uint32_t endIndex = Index(offsetEnd - 1);
        uint32_t endIndexMask = Mask(IndexInWord(offsetEnd - 1));
        ASSERT(startIndex <= endIndex);
        if (startIndex != endIndex) {
            ClearWord<mode>(startIndex, ~(startIndexMask - 1));
            ClearWord<mode>(endIndex, endIndexMask | (endIndexMask - 1));
            while (++startIndex < endIndex) {
                words[startIndex] = 0;
            }
        } else {
            ClearWord<mode>(endIndex, endIndexMask | (endIndexMask - startIndexMask));
        }
    }

    bool TestBit(uintptr_t offset) const
    {
        return Words()[Index(offset)] & Mask(IndexInWord(offset));
    }

    template <typename Visitor, AccessType mode = AccessType::NON_ATOMIC>
    void IterateMarkedBits(uintptr_t begin, size_t bitSize, Visitor visitor)
    {
        auto words = Words();
        uint32_t wordCount = WordCount(bitSize);
        uint32_t index = BIT_PER_WORD;
        for (uint32_t i = 0; i < wordCount; i++) {
            uint32_t word = words[i];
            while (word != 0) {
                index = static_cast<uint32_t>(__builtin_ctz(word));
                ASSERT(index < BIT_PER_WORD);
                if (!visitor(reinterpret_cast<void *>(begin + (index << TAGGED_TYPE_SIZE_LOG)))) {
                    ClearWord<mode>(i, Mask(index));
                }
                word &= ~(1u << index);
            }
            begin += TAGGED_TYPE_SIZE * BIT_PER_WORD;
        }
    }

    template <typename Visitor>
    void IterateMarkedBitsConst(uintptr_t begin, size_t bitSize, Visitor visitor) const
    {
        auto words = Words();
        uint32_t wordCount = WordCount(bitSize);
        uint32_t index = BIT_PER_WORD;
        for (uint32_t i = 0; i < wordCount; i++) {
            uint32_t word = words[i];
            while (word != 0) {
                index = static_cast<uint32_t>(__builtin_ctz(word));
                ASSERT(index < BIT_PER_WORD);
                visitor(reinterpret_cast<void *>(begin + (index << TAGGED_TYPE_SIZE_LOG)));
                word &= ~(1u << index);
            }
            begin += TAGGED_TYPE_SIZE * BIT_PER_WORD;
        }
    }

    void Merge(GCBitset *bitset, size_t bitSize)
    {
        auto words = Words();
        uint32_t wordCount = WordCount(bitSize);
        for (uint32_t i = 0; i < wordCount; i++) {
            words[i] |= bitset->Words()[i];
        }
    }

private:
    GCBitsetWord Mask(size_t index) const
    {
        return 1 << index;
    }

    size_t IndexInWord(uintptr_t offset) const
    {
        return offset & BIT_PER_WORD_MASK;
    }

    size_t Index(uintptr_t offset) const
    {
        return offset >> BIT_PER_WORD_LOG2;
    }

    size_t WordCount(size_t size) const
    {
        return size >> BYTE_PER_WORD_LOG2;
    }

    template <AccessType mode = AccessType::NON_ATOMIC>
    bool ClearWord(uint32_t index, uint32_t mask);

    template <>
    bool ClearWord<AccessType::NON_ATOMIC>(uint32_t index, uint32_t mask)
    {
        if ((Words()[index] & mask) == 0) {
            return false;
        }
        Words()[index] &= ~mask;
        return true;
    }

    template <>
    bool ClearWord<AccessType::ATOMIC>(uint32_t index, uint32_t mask)
    {
        auto word = reinterpret_cast<std::atomic<GCBitsetWord> *>(&Words()[index]);
        auto oldValue = word->load(std::memory_order_relaxed);
        do {
            if ((oldValue & mask) == 0) {
                return false;
            }
        } while (!word->compare_exchange_weak(oldValue, oldValue & (~mask), std::memory_order_seq_cst));
        return true;
    }
};

template <>
inline bool GCBitset::SetBit<AccessType::NON_ATOMIC>(uintptr_t offset)
{
    size_t index = Index(offset);
    GCBitsetWord mask = Mask(IndexInWord(offset));
    if (Words()[index] & mask) {
        return false;
    }
    Words()[index] |= mask;
    return true;
}

template <>
inline bool GCBitset::SetBit<AccessType::ATOMIC>(uintptr_t offset)
{
    auto word = reinterpret_cast<std::atomic<GCBitsetWord> *>(&Words()[Index(offset)]);
    auto mask = Mask(IndexInWord(offset));
    auto oldValue = word->load(std::memory_order_relaxed);
    do {
        if (oldValue & mask) {
            return false;
        }
    } while (!word->compare_exchange_weak(oldValue, oldValue | mask, std::memory_order_seq_cst));
    return true;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_GC_BITSET_H
