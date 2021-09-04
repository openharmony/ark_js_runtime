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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_REMEMBERED_SET_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_REMEMBERED_SET_H

#include "ecmascript/mem/mem.h"
#include "mem/gc/bitmap.h"

namespace panda::ecmascript {
enum class SlotStatus : bool {
    KEEP_SLOT,
    CLEAR_SLOT,
};

class RememberedSet : public mem::Bitmap {
public:
    using RememberedWordType = uintptr_t;
    static const size_t BYTESPERCHUNK = static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT);

    RememberedSet(uintptr_t begin_addr, size_t range_size, uintptr_t bitset_addr)
        : mem::Bitmap(reinterpret_cast<mem::Bitmap::BitmapWordType *>(bitset_addr), range_size / BYTESPERCHUNK),
          begin_addr_(begin_addr)
    {
    }
    ~RememberedSet() = default;
    NO_COPY_SEMANTIC(RememberedSet);
    NO_MOVE_SEMANTIC(RememberedSet);

    void Insert(uintptr_t address)
    {
        SetBit(AddrToBitOffset(address));
    }

    template <typename VisitorType>
    void IterateOverSetBits(VisitorType visitor)
    {
        IterateOverSetBitsInRange(0, Size(), visitor);
    }

    template <typename MemVisitor>
    void IterateOverMarkedChunks(MemVisitor visitor)
    {
        IterateOverSetBits(
            [&visitor, this](size_t bit_offset) -> bool { return visitor(BitOffsetToAddr(bit_offset)); });
    }

    void Clear(uintptr_t address)
    {
        ClearBit(AddrToBitOffset(address));
    }

    void ClearRange(uintptr_t begin, uintptr_t end)
    {
        ClearBitsInRange(AddrToBitOffset(begin), AddrToBitOffset(end));
    }

    static size_t GetSizeInByte(size_t range_size)
    {
        return (range_size >> mem::Bitmap::LOG_BITSPERWORD) / BYTESPERCHUNK * sizeof(mem::Bitmap::BitmapWordType);
    }

private:
    void *BitOffsetToAddr(size_t bit_offset) const
    {
        return ToVoidPtr(begin_addr_ + bit_offset * BYTESPERCHUNK);
    }

    size_t AddrToBitOffset(uintptr_t addr) const
    {
        return (addr - begin_addr_) / BYTESPERCHUNK;
    }

    uintptr_t begin_addr_;
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_REMEMBERED_SET_H
