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

#ifndef ECMASCRIPT_MEM_REMEMBERED_SET_H
#define ECMASCRIPT_MEM_REMEMBERED_SET_H

#include "ecmascript/mem/gc_bitset.h"

namespace panda::ecmascript {
class RememberedSet {
public:
    static constexpr size_t GCBITSET_DATA_OFFSET = sizeof(size_t);
    RememberedSet(size_t size) : size_(size) {}

    NO_COPY_SEMANTIC(RememberedSet);
    NO_MOVE_SEMANTIC(RememberedSet);

    GCBitset *GCBitsetData()
    {
        return reinterpret_cast<GCBitset *>(reinterpret_cast<uintptr_t>(this) + GCBITSET_DATA_OFFSET);
    }

    const GCBitset *GCBitsetData() const
    {
        return reinterpret_cast<GCBitset *>(reinterpret_cast<uintptr_t>(this) + GCBITSET_DATA_OFFSET);
    }

    void ClearAll()
    {
        GCBitsetData()->Clear(size_);
    }

    bool Insert(uintptr_t begin, uintptr_t addr)
    {
        return GCBitsetData()->SetBit<AccessType::NON_ATOMIC>((addr - begin) >> TAGGED_TYPE_SIZE_LOG);
    }

    bool AtomicInsert(uintptr_t begin, uintptr_t addr)
    {
        return GCBitsetData()->SetBit<AccessType::ATOMIC>((addr - begin) >> TAGGED_TYPE_SIZE_LOG);
    }

    void ClearRange(uintptr_t begin, uintptr_t start, uintptr_t end)
    {
        GCBitsetData()->ClearBitRange<AccessType::NON_ATOMIC>(
            (start - begin) >> TAGGED_TYPE_SIZE_LOG, (end - begin) >> TAGGED_TYPE_SIZE_LOG);
    }

    void AtomicClearRange(uintptr_t begin, uintptr_t start, uintptr_t end)
    {
        GCBitsetData()->ClearBitRange<AccessType::ATOMIC>(
            (start - begin) >> TAGGED_TYPE_SIZE_LOG, (end - begin) >> TAGGED_TYPE_SIZE_LOG);
    }

    template <typename Visitor>
    void IterateAllMarkedBits(uintptr_t begin, Visitor visitor)
    {
        GCBitsetData()->IterateMarkedBits<Visitor, AccessType::NON_ATOMIC>(begin, size_, visitor);
    }

    template <typename Visitor>
    void AtomicIterateAllMarkedBits(uintptr_t begin, Visitor visitor)
    {
        GCBitsetData()->IterateMarkedBits<Visitor, AccessType::ATOMIC>(begin, size_, visitor);
    }

    template <typename Visitor>
    void IterateAllMarkedBitsConst(uintptr_t begin, Visitor visitor) const
    {
        GCBitsetData()->IterateMarkedBitsConst(begin, size_, visitor);
    }

    void Merge(RememberedSet *rset)
    {
        GCBitset *bitset = rset->GCBitsetData();
        GCBitsetData()->Merge(bitset, size_);
    }

    size_t Size() const
    {
        return size_ + GCBITSET_DATA_OFFSET;
    }

private:
    size_t size_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_REMEMBERED_SET_H
