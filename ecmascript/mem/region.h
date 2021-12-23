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

#ifndef ECMASCRIPT_MEM_REGION_H
#define ECMASCRIPT_MEM_REGION_H

#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/mem.h"
#include "mem/gc/bitmap.h"

namespace panda {
using RangeBitmap = mem::MemBitmap<static_cast<size_t>(ecmascript::MemAlignment::MEM_ALIGN_OBJECT)>;

namespace ecmascript {
class Space;
class RememberedSet;
class WorkerHelper;

enum RegionFlags {
    NEVER_EVACUATE = 1,
    HAS_AGE_MARK = 1 << 1,
    BELOW_AGE_MARK = 1 << 2,
    IS_IN_YOUNG_GENERATION = 1 << 3,
    IS_IN_SNAPSHOT_GENERATION = 1 << 4,
    IS_HUGE_OBJECT = 1 << 5,
    IS_IN_OLD_GENERATION = 1 << 6,
    IS_IN_NON_MOVABLE_GENERATION = 1 << 7,
    IS_IN_YOUNG_OR_OLD_GENERATION = IS_IN_YOUNG_GENERATION | IS_IN_OLD_GENERATION,
    IS_IN_COLLECT_SET = 1 << 8,
    IS_IN_PROMOTE_SET = 1 << 9,
    IS_IN_YOUNG_OR_CSET_GENERATION = IS_IN_YOUNG_GENERATION | IS_IN_COLLECT_SET,
    IS_INVALID = 1 << 10,
};

class Region {
public:
    Region(Space *space, uintptr_t allocateBase, uintptr_t begin, uintptr_t end)
        : space_(space),
          flags_(0),
          allocateBase_(allocateBase),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          begin_(begin),
          end_(end),
          highWaterMark_(end),
          aliveObject_(0)
    {
    }
    ~Region() = default;
    NO_COPY_SEMANTIC(Region);
    NO_MOVE_SEMANTIC(Region);

    void Reset()
    {
        flags_ = 0;
        highWaterMark_ = end_;
        memset_s(reinterpret_cast<void *>(begin_), GetSize(), 0, GetSize());
    }

    void LinkNext(Region *next)
    {
        next_ = next;
    }

    Region *GetNext() const
    {
        return next_;
    }

    void LinkPrev(Region *prev)
    {
        prev_ = prev;
    }

    Region *GetPrev() const
    {
        return prev_;
    }

    uintptr_t GetBegin() const
    {
        return begin_;
    }

    uintptr_t GetEnd() const
    {
        return end_;
    }

    size_t GetCapacity() const
    {
        return end_ - reinterpret_cast<uintptr_t>(this);
    }

    size_t GetSize() const
    {
        return end_ - begin_;
    }

    Space *GetSpace() const
    {
        return space_;
    }

    void SetSpace(Space *space)
    {
        space_ = space;
    }

    void ResetFlag()
    {
        flags_ = 0;
    }

    void SetFlag(RegionFlags flag)
    {
        flags_ |= flag;
    }

    void ClearFlag(RegionFlags flag)
    {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        flags_ &= ~flag;
    }

    bool IsFlagSet(RegionFlags flag) const
    {
        return (flags_ & flag) != 0;
    }

    RangeBitmap *GetMarkBitmap()
    {
        return markBitmap_;
    }

    RememberedSet *GetCrossRegionRememberedSet()
    {
        return crossRegionSet_;
    }

    RememberedSet *GetOldToNewRememberedSet()
    {
        return oldToNewSet_;
    }

    static Region *ObjectAddressToRange(TaggedObject *obj)
    {
        return reinterpret_cast<Region *>(ToUintPtr(obj) & ~DEFAULT_REGION_MASK);
    }

    static Region *ObjectAddressToRange(uintptr_t objAddress)
    {
        return reinterpret_cast<Region *>(objAddress & ~DEFAULT_REGION_MASK);
    }

    bool InYoungGeneration() const
    {
        return IsFlagSet(RegionFlags::IS_IN_YOUNG_GENERATION);
    }

    bool InOldGeneration() const
    {
        return IsFlagSet(RegionFlags::IS_IN_OLD_GENERATION);
    }

    bool InYoungAndOldGeneration() const
    {
        return IsFlagSet(RegionFlags::IS_IN_YOUNG_OR_OLD_GENERATION);
    }

    bool InYoungAndCSetGeneration() const
    {
        return IsFlagSet(RegionFlags::IS_IN_YOUNG_OR_CSET_GENERATION);
    }

    bool InPromoteSet() const
    {
        return IsFlagSet(RegionFlags::IS_IN_PROMOTE_SET);
    }

    bool InCollectSet() const
    {
        return IsFlagSet(RegionFlags::IS_IN_COLLECT_SET);
    }

    bool HasAgeMark() const
    {
        return IsFlagSet(RegionFlags::HAS_AGE_MARK);
    }

    bool BelowAgeMark() const
    {
        return IsFlagSet(RegionFlags::BELOW_AGE_MARK);
    }

    bool InRange(uintptr_t address)
    {
        return address >= begin_ && address <= end_;
    }

    inline RangeBitmap *GetOrCreateMarkBitmap();
    inline RangeBitmap *CreateMarkBitmap();
    inline void ClearMarkBitmap();
    inline RememberedSet *CreateRememberedSet();
    inline RememberedSet *GetOrCreateCrossRegionRememberedSet();
    inline RememberedSet *GetOrCreateOldToNewRememberedSet();
    inline void InsertCrossRegionRememberedSet(uintptr_t addr);
    inline void AtomicInsertCrossRegionRememberedSet(uintptr_t addr);
    inline void InsertOldToNewRememberedSet(uintptr_t addr);
    inline void AtomicInsertOldToNewRememberedSet(uintptr_t addr);
    inline void ClearCrossRegionRememberedSet();
    inline void ClearOldToNewRememberedSet();

    uintptr_t GetAllocateBase() const
    {
        return allocateBase_;
    }

    size_t GetAllocatedBytes(uintptr_t top = 0)
    {
        ASSERT(top == 0 || InRange(top));
        return (top == 0) ? (highWaterMark_ - begin_) : (top - begin_);
    }

    void SetHighWaterMark(uintptr_t mark)
    {
        ASSERT(InRange(mark));
        highWaterMark_ = mark;
    }

    int SetCodeExecutableAndReadable()
    {
        // NOLINT(hicpp-signed-bitwise)
        int res = mprotect(reinterpret_cast<void *>(allocateBase_), GetCapacity(), PROT_EXEC | PROT_READ | PROT_WRITE);
        return res;
    }

    void InitializeKind()
    {
        kinds_ = Span<FreeObjectKind *>(new FreeObjectKind *[FreeObjectList::NumberOfKinds()](),
                                        FreeObjectList::NumberOfKinds());
    }

    void RebuildKind()
    {
        EnumerateKinds([](FreeObjectKind *kind) {
            if (kind != nullptr) {
                kind->Rebuild();
            }
        });
    }

    void DestroyKind()
    {
        for (auto kind : kinds_) {
            delete kind;
        }
        delete[] kinds_.data();
    }

    FreeObjectKind *GetFreeObjectKind(KindType type)
    {
        // Thread safe
        if (kinds_[type] == nullptr) {
            kinds_[type] = new FreeObjectKind(type);
        }
        return kinds_[type];
    }

    template<class Callback>
    void EnumerateKinds(Callback cb)
    {
        for (auto kind : kinds_) {
            cb(kind);
        }
    }

    bool IsMarking() const
    {
        return marking_;
    }

    void SetMarking(bool isMarking)
    {
        marking_ = isMarking;
    }

    inline WorkerHelper *GetWorkList() const;

    void IncrementAliveObject(size_t size)
    {
        aliveObject_ += size;
    }

    void DecreaseAliveObject(size_t size)
    {
        aliveObject_ -= size;
    }

    void SetAliveObject(size_t size)
    {
        aliveObject_ = size;
    }

    void ResetAliveObject()
    {
        aliveObject_ = 0;
    }

    size_t AliveObject() const
    {
        return aliveObject_;
    }

    bool MostObjectAlive() const
    {
        return aliveObject_ > MOST_OBJECT_ALIVE_THRESHOLD_PERCENT * GetSize();
    }

private:
    static constexpr double MOST_OBJECT_ALIVE_THRESHOLD_PERCENT = 0.8;
    Space *space_;
    uintptr_t flags_;  // Memory alignment, only low 32bits are used now
    uintptr_t allocateBase_;
    uintptr_t begin_;
    uintptr_t end_;
    uintptr_t highWaterMark_;
    bool marking_ {false};
    std::atomic_size_t aliveObject_ {0};
    Region *next_ {nullptr};
    Region *prev_ {nullptr};
    RangeBitmap *markBitmap_ {nullptr};
    RememberedSet *crossRegionSet_ {nullptr};
    RememberedSet *oldToNewSet_ {nullptr};
    Span<FreeObjectKind *> kinds_;
    os::memory::Mutex lock_;
    friend class SnapShot;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_REGION_H
