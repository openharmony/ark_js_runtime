/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SPACE_H
#define ECMASCRIPT_MEM_SPACE_H

#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"
#include "libpandabase/utils/type_helpers.h"
#include "securec.h"

namespace panda::ecmascript {
enum MemSpaceType {
    OLD_SPACE = 0,
    NON_MOVABLE,
    MACHINE_CODE_SPACE,
    HUGE_OBJECT_SPACE,
    SEMI_SPACE,
    SNAPSHOT_SPACE,
    COMPRESS_SPACE,
    LOCAL_SPACE,
    SPACE_TYPE_LAST,  // Count of different types

    FREE_LIST_NUM = MACHINE_CODE_SPACE - OLD_SPACE + 1,
};

static inline std::string ToSpaceTypeName(MemSpaceType type)
{
    switch (type) {
        case OLD_SPACE:
            return "old space";
        case NON_MOVABLE:
            return "non movable space";
        case MACHINE_CODE_SPACE:
            return "machine code space";
        case HUGE_OBJECT_SPACE:
            return "huge object space";
        case SEMI_SPACE:
            return "semi space";
        case SNAPSHOT_SPACE:
            return "snapshot space";
        case COMPRESS_SPACE:
            return "compress space";
        default:
            return "unknown space";
    }
}

class Space {
public:
    Space(HeapRegionAllocator *regionAllocator, MemSpaceType spaceType, size_t initialCapacity, size_t maximumCapacity);
    virtual ~Space() = default;
    NO_COPY_SEMANTIC(Space);
    NO_MOVE_SEMANTIC(Space);

    size_t GetMaximumCapacity() const
    {
        return maximumCapacity_;
    }

    void SetMaximumCapacity(size_t maximumCapacity)
    {
        maximumCapacity_ = maximumCapacity;
    }

    size_t GetInitialCapacity() const
    {
        return initialCapacity_;
    }

    void SetInitialCapacity(size_t initialCapacity)
    {
        initialCapacity_ = initialCapacity;
    }

    size_t GetCommittedSize() const
    {
        return committedSize_;
    }

    void IncreaseCommitted(size_t bytes)
    {
        committedSize_ += bytes;
    }

    void DecreaseCommitted(size_t bytes)
    {
        committedSize_ -= bytes;
    }

    void IncreaseObjectSize(size_t bytes)
    {
        objectSize_ += bytes;
    }

    void DecreaseObjectSize(size_t bytes)
    {
        objectSize_ -= bytes;
    }

    MemSpaceType GetSpaceType() const
    {
        return spaceType_;
    }

    inline RegionSpaceFlag GetRegionFlag() const;

    uintptr_t GetAllocateAreaBegin() const
    {
        return regionList_.GetLast()->GetBegin();
    }

    uintptr_t GetAllocateAreaEnd() const
    {
        return regionList_.GetLast()->GetEnd();
    }

    Region *GetCurrentRegion() const
    {
        return regionList_.GetLast();
    }

    Region *GetFirstRegion() const
    {
        return regionList_.GetFirst();
    }

    uint32_t GetRegionCount()
    {
        return regionList_.GetLength();
    }

    EcmaList<Region> &GetRegionList()
    {
        return regionList_;
    }

    const EcmaList<Region> &GetRegionList() const
    {
        return regionList_;
    }

    void SetRecordRegion()
    {
        recordRegion_ = GetCurrentRegion();
    }

    template <class Callback>
    inline void EnumerateRegions(const Callback &cb, Region *region = nullptr) const;
    template <class Callback>
    inline void EnumerateRegionsWithRecord(const Callback &cb) const;

    inline void AddRegion(Region *region);
    inline void AddRegionToFront(Region *region);
    inline void RemoveRegion(Region *region);

    virtual void Initialize() {};
    void Destroy();

    void ReclaimRegions();

protected:
    void ClearAndFreeRegion(Region *region);

    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    EcmaList<Region> regionList_ {};
    MemSpaceType spaceType_ {};
    size_t initialCapacity_ {0};
    size_t maximumCapacity_ {0};
    size_t committedSize_ {0};
    size_t objectSize_ {0};
    Region *recordRegion_ {nullptr};
};

class HugeObjectSpace : public Space {
public:
    explicit HugeObjectSpace(HeapRegionAllocator *regionAllocator, size_t initialCapacity, size_t maximumCapacity);
    ~HugeObjectSpace() override = default;
    NO_COPY_SEMANTIC(HugeObjectSpace);
    NO_MOVE_SEMANTIC(HugeObjectSpace);
    uintptr_t Allocate(size_t objectSize, JSThread *thread);
    void Sweep(bool isConcurrentSweep);
    void FinishConcurrentSweep();
    size_t GetHeapObjectSize() const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;

    void RecliamHugeRegion();

private:
    EcmaList<Region> hugeNeedFreeList_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPACE_H
