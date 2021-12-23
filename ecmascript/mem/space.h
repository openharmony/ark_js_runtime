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

#ifndef ECMASCRIPT_MEM_SPACE_H
#define ECMASCRIPT_MEM_SPACE_H

#include "mem/gc/bitmap.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"
#include "libpandabase/utils/type_helpers.h"
#include "libpandafile/file.h"

namespace panda::ecmascript {
class EcmaVM;
class Heap;
class Program;

enum MemSpaceType {
    OLD_SPACE = 0,
    NON_MOVABLE,
    MACHINE_CODE_SPACE,
    HUGE_OBJECT_SPACE,
    SEMI_SPACE,
    SNAPSHOT_SPACE,
    COMPRESS_SPACE,
    SPACE_TYPE_LAST,  // Count of different types

    FREE_LIST_NUM = MACHINE_CODE_SPACE - OLD_SPACE + 1,
};

enum TriggerGCType {
    SEMI_GC,
    OLD_GC,
    NON_MOVE_GC,
    HUGE_GC,
    MACHINE_CODE_GC,
    COMPRESS_FULL_GC,
    GC_TYPE_LAST  // Count of different types
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
            return "unknow space";
    }
}

static constexpr size_t SPACE_TYPE_SIZE = helpers::ToUnderlying(MemSpaceType::SPACE_TYPE_LAST);

class Space {
public:
    Space(Heap *heap, MemSpaceType spaceType, size_t initialCapacity, size_t maximumCapacity);
    virtual ~Space() = default;
    NO_COPY_SEMANTIC(Space);
    NO_MOVE_SEMANTIC(Space);

    Heap *GetHeap() const
    {
        return heap_;
    }

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

    size_t GetCommittedSize() const
    {
        return committedSize_;
    }

    void IncrementCommitted(size_t bytes)
    {
        committedSize_ += bytes;
    }

    void DecrementCommitted(size_t bytes)
    {
        committedSize_ -= bytes;
    }

    MemSpaceType GetSpaceType() const
    {
        return spaceType_;
    }

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

    size_t GetHeapObjectSize() const;

    template <class Callback>
    void EnumerateRegions(const Callback &cb, Region *region = nullptr) const;

    void AddRegion(Region *region);
    void AddRegionToFirst(Region *region);
    void RemoveRegion(Region *region);

    void Initialize();
    void Destroy();

    void ReclaimRegions();

    void ClearAndFreeRegion(Region *region);

protected:
    Heap *heap_ {nullptr};
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};
    RegionFactory *regionFactory_ {nullptr};
    EcmaList<Region> regionList_ {};
    MemSpaceType spaceType_ {};
    size_t initialCapacity_ {0};
    size_t maximumCapacity_ {0};
    size_t committedSize_ {0};
};

class SemiSpace : public Space {
public:
    explicit SemiSpace(Heap *heap, size_t initialCapacity = DEFAULT_SEMI_SPACE_SIZE,
                       size_t maximumCapacity = SEMI_SPACE_SIZE_CAPACITY);
    ~SemiSpace() override = default;
    NO_COPY_SEMANTIC(SemiSpace);
    NO_MOVE_SEMANTIC(SemiSpace);

    void SetAgeMark(uintptr_t mark);

    uintptr_t GetAgeMark() const
    {
        return ageMark_;
    }

    bool Expand(uintptr_t top);
    bool AddRegionToList(Region *region);

    void Swap(SemiSpace *other);

    bool ContainObject(TaggedObject *object) const;
    bool IsLive(TaggedObject *object) const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;

private:
    uintptr_t ageMark_;
};

class OldSpace : public Space {
public:
    explicit OldSpace(Heap *heap, size_t initialCapacity = DEFAULT_OLD_SPACE_SIZE,
                      size_t maximumCapacity = MAX_OLD_SPACE_SIZE);
    ~OldSpace() override = default;
    NO_COPY_SEMANTIC(OldSpace);
    NO_MOVE_SEMANTIC(OldSpace);
    bool Expand();
    bool AddRegionToList(Region *region);
    bool ContainObject(TaggedObject *object) const;
    bool IsLive(TaggedObject *object) const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;
    size_t GetHeapObjectSize() const;
    void Merge(Space *fromSpace);
    void AddRegionToCSet(Region *region);
    void RemoveRegionFromCSetAndList(Region *region);
    void ClearRegionFromCSet();
    void RemoveCSetFromList();
    void ReclaimRegionCSet();
    template <class Callback>
    void EnumerateCollectRegionSet(const Callback &cb) const;
    template <class Callback>
    void EnumerateNonCollectRegionSet(const Callback &cb) const;
    void SelectCSet();

private:
    static constexpr size_t PARTIAL_GC_MAX_COLLECT_REGION_SIZE = 16;
    static constexpr size_t PARTIAL_GC_MIN_COLLECT_REGION_SIZE = 5;
    CVector<Region *> collectRegionSet_;
};

class NonMovableSpace : public Space {
public:
    explicit NonMovableSpace(Heap *heap, size_t initialCapacity = DEFAULT_NON_MOVABLE_SPACE_SIZE,
                             size_t maximumCapacity = MAX_NON_MOVABLE_SPACE_SIZE);
    ~NonMovableSpace() override = default;
    NO_COPY_SEMANTIC(NonMovableSpace);
    NO_MOVE_SEMANTIC(NonMovableSpace);
    bool Expand();
    bool ContainObject(TaggedObject *object) const;
    bool IsLive(TaggedObject *object) const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;
    size_t GetHeapObjectSize() const;
};

class SnapShotSpace : public Space {
public:
    explicit SnapShotSpace(Heap *heap, size_t initialCapacity = DEFAULT_SNAPSHOT_SPACE_SIZE,
                           size_t maximumCapacity = MAX_SNAPSHOT_SPACE_SIZE);
    ~SnapShotSpace() override = default;
    NO_COPY_SEMANTIC(SnapShotSpace);
    NO_MOVE_SEMANTIC(SnapShotSpace);
    bool Expand(uintptr_t top);
};

class HugeObjectSpace : public Space {
public:
    explicit HugeObjectSpace(Heap *heap, size_t initialCapacity = DEFAULT_OLD_SPACE_SIZE,
                             size_t maximumCapacity = MAX_HUGE_OBJECT_SPACE_SIZE);
    ~HugeObjectSpace() override = default;
    NO_COPY_SEMANTIC(HugeObjectSpace);
    NO_MOVE_SEMANTIC(HugeObjectSpace);
    uintptr_t Allocate(size_t objectSize);
    void Free(Region *region);
    size_t GetHeapObjectSize() const;
    bool ContainObject(TaggedObject *object) const;
    bool IsLive(TaggedObject *object) const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;
};

class MachineCodeSpace : public Space {
public:
    explicit MachineCodeSpace(Heap *heap, size_t initialCapacity = DEFAULT_MACHINE_CODE_SPACE_SIZE,
                              size_t maximumCapacity = MAX_MACHINE_CODE_SPACE_SIZE);
    ~MachineCodeSpace() override = default;
    NO_COPY_SEMANTIC(MachineCodeSpace);
    NO_MOVE_SEMANTIC(MachineCodeSpace);  // Note: Expand(), ContainObject(), IsLive() left for define
    bool Expand();
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SPACE_H
