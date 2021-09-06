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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_SPACE_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_SPACE_H

#include "mem/gc/bitmap.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"
#include "libpandabase/utils/type_helpers.h"
#include "libpandafile/file.h"

namespace panda::ecmascript {
class Heap;
class Program;

enum MemSpaceType {
    SEMI_SPACE,
    OLD_SPACE,
    NON_MOVABLE,
    LARGE_OBJECT_SPACE,
    SNAPSHOT_SPACE,
    SPACE_TYPE_LAST  // Count of different types
};

enum TriggerGCType {
    SEMI_GC,
    OLD_GC,
    NON_MOVE_GC,
    LARGE_GC,
    COMPRESS_FULL_GC,
    GC_TYPE_LAST  // Count of different types
};

constexpr MemSpaceType ToSpaceType(size_t index)
{
    return static_cast<MemSpaceType>(index);
}

static constexpr size_t SPACE_TYPE_SIZE = helpers::ToUnderlying(MemSpaceType::SPACE_TYPE_LAST);

class Space {
public:
    Space(Heap *heap, MemSpaceType spaceType, size_t initialCapacity, size_t maximumCapacity)
        : heap_(heap),
          spaceType_(spaceType),
          initialCapacity_(initialCapacity),
          maximumCapacity_(maximumCapacity),
          committedSize_(0)
    {
    }
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

    void SetUp();
    void TearDown();

    void ReclaimRegions();

    void ClearAndFreeRegion(Region *region);

private:
    Heap *heap_;
    EcmaList<Region> regionList_;
    MemSpaceType spaceType_;
    size_t initialCapacity_;
    size_t maximumCapacity_;
    size_t committedSize_;
};

class SemiSpace : public Space {
public:
    explicit SemiSpace(Heap *heap, size_t initialCapacity = DEFAULT_SEMI_SPACE_SIZE,
                       size_t maximumCapacity = SEMI_SPACE_SIZE_4M);
    ~SemiSpace() override = default;
    NO_COPY_SEMANTIC(SemiSpace);
    NO_MOVE_SEMANTIC(SemiSpace);

    void SetAgeMark(uintptr_t mark)
    {
        ageMark_ = mark;
    }

    uintptr_t GetAgeMark() const
    {
        return ageMark_;
    }

    bool Expand(uintptr_t top);

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
    bool ContainObject(TaggedObject *object) const;
    bool IsLive(TaggedObject *object) const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;
    size_t GetHeapObjectSize() const;
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

class LargeObjectSpace : public Space {
public:
    explicit LargeObjectSpace(Heap *heap, size_t initialCapacity = DEFAULT_OLD_SPACE_SIZE,
                              size_t maximumCapacity = MAX_LARGE_OBJECT_SPACE_SIZE);
    ~LargeObjectSpace() override = default;
    NO_COPY_SEMANTIC(LargeObjectSpace);
    NO_MOVE_SEMANTIC(LargeObjectSpace);
    uintptr_t Allocate(size_t objectSize);
    size_t GetHeapObjectSize() const;
    bool ContainObject(TaggedObject *object) const;
    bool IsLive(TaggedObject *object) const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_SPACE_H
