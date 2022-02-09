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

#ifndef ECMASCRIPT_MEM_ALLOCATOR_H
#define ECMASCRIPT_MEM_ALLOCATOR_H

#include <memory>

#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
class Space;
class Region;
class Heap;

class Allocator {
public:
    Allocator() = default;
    virtual ~Allocator() = default;
    NO_COPY_SEMANTIC(Allocator);
    NO_MOVE_SEMANTIC(Allocator);
};

class BumpPointerAllocator : public Allocator {
public:
    BumpPointerAllocator() = default;
    ~BumpPointerAllocator() override = default;
    NO_COPY_SEMANTIC(BumpPointerAllocator);
    NO_MOVE_SEMANTIC(BumpPointerAllocator);

    inline explicit BumpPointerAllocator(const Space *space);
    inline BumpPointerAllocator(uintptr_t begin, uintptr_t end);

    inline void Reset();
    inline void Reset(const Space *space);
    inline void Reset(uintptr_t begin, uintptr_t end);
    inline uintptr_t Allocate(size_t size);

    uintptr_t GetTop() const
    {
        return top_;
    }

    uintptr_t GetEnd() const
    {
        return end_;
    }

    void Swap(const BumpPointerAllocator &other)
    {
        begin_ = other.begin_;
        top_ = other.top_;
        end_ = other.end_;
    }

    size_t Available() const
    {
        return (end_ - top_);
    }

private:
    uintptr_t begin_ {0};
    uintptr_t top_ {0};
    uintptr_t end_ {0};
};

class FreeListAllocator : public Allocator {
public:
    FreeListAllocator() = default;
    ~FreeListAllocator() override = default;

    NO_COPY_SEMANTIC(FreeListAllocator);
    NO_MOVE_SEMANTIC(FreeListAllocator);

    inline explicit FreeListAllocator(const Space *space);

    inline void Reset(Heap *heap);

    template<bool isLocal = false>
    inline uintptr_t Allocate(size_t size);
    inline void AddFree(Region *region);
    inline uintptr_t LookupSuitableFreeObject(size_t size);

    inline void RebuildFreeList();

    inline void LinkFreeObjectKind(Region *region);
    inline void UnlinkFreeObjectKind(Region *region);

    inline void Swap(FreeListAllocator &other)
    {
        heap_ = other.heap_;
        bpAllocator_.Swap(other.bpAllocator_);
        freeList_.swap(other.freeList_);
        type_ = other.type_;
        sweeping_ = other.sweeping_;
    }

    inline void FreeBumpPoint();

    inline void Free(uintptr_t begin, uintptr_t end, bool isAdd = true);

#ifndef NDEBUG
    inline size_t GetAvailableSize() const;
    inline size_t GetWastedSize() const;
#endif

    void SetSweeping(bool sweeping)
    {
        sweeping_ = sweeping;
    }

    size_t GetAllocatedSize() const
    {
        return allocationSizeAccumulator_ + promotedSizeAccumulator_;
    }

    void IncrementPromotedSize(size_t size)
    {
        promotedSizeAccumulator_ += size;
    }

private:
    inline uintptr_t Allocate(FreeObject *object, size_t size);

    BumpPointerAllocator bpAllocator_;
    std::unique_ptr<FreeObjectList> freeList_;
    Heap *heap_{nullptr};
    MemSpaceType type_ = OLD_SPACE;
    bool sweeping_ = false;
    size_t allocationSizeAccumulator_ {0};
    size_t promotedSizeAccumulator_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_ALLOCATOR_H
