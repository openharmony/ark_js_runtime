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
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/remembered_set.h"
#include "securec.h"

namespace panda {
namespace ecmascript {
class Space;
class Heap;
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
    IS_IN_NEW_TO_NEW_SET = 1 << 9,
    IS_IN_YOUNG_OR_CSET_GENERATION = IS_IN_YOUNG_GENERATION | IS_IN_COLLECT_SET,
    IS_INVALID = 1 << 10,
};

#define REGION_OFFSET_LIST(V)                                                             \
    V(GCBITSET, GCBitset, markGCBitset_, FLAG, sizeof(uint32_t), sizeof(uint64_t))              \
    V(OLDTONEWSET, OldToNewSet, oldToNewSet_, GCBITSET, sizeof(uint32_t), sizeof(uint64_t)) \
    V(BEGIN, Begin, begin_, OLDTONEWSET, sizeof(uint32_t), sizeof(uint64_t))

// |---------------------------------------------------------------------------------------|
// |                                   Region (256 kb)                                     |
// |---------------------------------|--------------------------------|--------------------|
// |     Head (sizeof(Region))       |         Mark bitset (4kb)      |      Data          |
// |---------------------------------|--------------------------------|--------------------|

class Region {
public:
    Region(Space *space, Heap *heap, uintptr_t allocateBase, uintptr_t begin, uintptr_t end, RegionFlags flags)
        : space_(space), heap_(heap),
        allocateBase_(allocateBase),
        end_(end),
        highWaterMark_(end),
        aliveObject_(0),
        wasted_(0)
    {
        SetFlag(flags);
        bitsetSize_ = IsFlagSet(RegionFlags::IS_HUGE_OBJECT) ?
            GCBitset::BYTE_PER_WORD : GCBitset::SizeOfGCBitset(GetCapacity());
        markGCBitset_ = new (ToVoidPtr(begin)) GCBitset();
        markGCBitset_->Clear(bitsetSize_);
        begin_ = AlignUp(begin + bitsetSize_, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    }
    ~Region() = default;
    NO_COPY_SEMANTIC(Region);
    NO_MOVE_SEMANTIC(Region);

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

    inline void SetSpace(Space *space)
    {
        space_ = space;
    }

    Heap *GetHeap() const
    {
        return heap_;
    }

    Space *GetSpace() const
    {
        return space_;
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

    // Mark bitset
    GCBitset *GetMarkGCBitset() const;
    bool AtomicMark(void *address);
    void ClearMark(void *address);
    bool Test(void *addr) const;
    template <typename Visitor>
    void IterateAllMarkedBits(Visitor visitor) const;
    void ClearMarkGCBitset();
    // Cross region remembered set
    void InsertCrossRegionRSet(uintptr_t addr);
    void AtomicInsertCrossRegionRSet(uintptr_t addr);
    template <typename Visitor>
    void IterateAllCrossRegionBits(Visitor visitor) const;
    void ClearCrossRegionRSet();
    void ClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteCrossRegionRSet();
    // Old to new remembered set
    void InsertOldToNewRSet(uintptr_t addr);
    template <typename Visitor>
    void IterateAllOldToNewBits(Visitor visitor);
    void ClearOldToNewRSet();
    void ClearOldToNewRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteOldToNewRSet();

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

    bool InHugeObjectGeneration() const
    {
        return IsFlagSet(RegionFlags::IS_HUGE_OBJECT);
    }

    bool InYoungOrCSetGeneration() const
    {
        return IsFlagSet(RegionFlags::IS_IN_YOUNG_OR_CSET_GENERATION);
    }

    bool InNewToNewSet() const
    {
        return IsFlagSet(RegionFlags::IS_IN_NEW_TO_NEW_SET);
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

    bool InRange(uintptr_t address) const
    {
        return address >= begin_ && address <= end_;
    }

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
#ifndef PANDA_TARGET_WINDOWS
        int res = mprotect(reinterpret_cast<void *>(allocateBase_), GetCapacity(), PROT_EXEC | PROT_READ | PROT_WRITE);
#else
        int res = 0;
#endif
        return res;
    }

    void InitializeSet()
    {
        sets_ = Span<FreeObjectSet *>(new FreeObjectSet *[FreeObjectList::NumberOfSets()](),
            FreeObjectList::NumberOfSets());
    }

    void RebuildSet()
    {
        EnumerateSets([](FreeObjectSet *set) {
            if (set != nullptr) {
                set->Rebuild();
            }
        });
    }

    void DestroySet()
    {
        for (auto set : sets_) {
            delete set;
        }
        delete[] sets_.data();
    }

    FreeObjectSet *GetFreeObjectSet(SetType type)
    {
        // Thread safe
        if (sets_[type] == nullptr) {
            sets_[type] = new FreeObjectSet(type);
        }
        return sets_[type];
    }

    template<class Callback>
    void EnumerateSets(Callback cb)
    {
        for (auto set : sets_) {
            cb(set);
        }
    }

    inline bool IsMarking() const;

    inline WorkerHelper *GetWorkList() const;

    void IncrementAliveObjectSafe(size_t size)
    {
        ASSERT(aliveObject_ + size <= GetSize());
        aliveObject_ += size;
    }

    void IncrementAliveObject(size_t size)
    {
        ASSERT(aliveObject_ + size <= GetSize());
        aliveObject_.fetch_add(size, std::memory_order_relaxed);
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

    void ResetWasted()
    {
        wasted_ = 0;
    }
    void IncrementWasted(size_t size)
    {
        wasted_ += size;
    }
    size_t GetWastedSize()
    {
        return wasted_;
    }

    static constexpr uint32_t GetOldToNewSetOffset(bool is32Bit = false)
    {
        return is32Bit ? REGION_OLDTONEWSET_OFFSET_32 : REGION_OLDTONEWSET_OFFSET_64;
    }

    static constexpr uint32_t GetGCBitsetOffset(bool is32Bit = false)
    {
        return is32Bit ? REGION_GCBITSET_OFFSET_32 : REGION_GCBITSET_OFFSET_64;
    }

    static constexpr uint32_t GetFlagOffset(bool is32Bit = false)
    {
        return is32Bit ? REGION_FLAG_OFFSET_32 : REGION_FLAG_OFFSET_64;
    }

    static constexpr uint32_t GetBeginOffset(bool is32Bit = false)
    {
        return is32Bit ? REGION_BEGIN_OFFSET_32 : REGION_BEGIN_OFFSET_64;
    }

    #define REGION_OFFSET_MACRO(name, camelName, memberName, lastName, lastSize32, lastSize64)              \
        static constexpr uint32_t REGION_##name##_OFFSET_32 = REGION_##lastName##_OFFSET_32 + (lastSize32); \
        static constexpr uint32_t REGION_##name##_OFFSET_64 = REGION_##lastName##_OFFSET_64 + (lastSize64);
    static constexpr uint32_t REGION_FLAG_OFFSET_32 = 0U;
    static constexpr uint32_t REGION_FLAG_OFFSET_64 = 0U;
    REGION_OFFSET_LIST(REGION_OFFSET_MACRO)
    #undef REGION_OFFSET_MACRO

    static constexpr bool CheckLayout()
    {
#ifdef PANDA_TARGET_32
        #define REGION_OFFSET_ASSET(name, camelName, memberName, lastName, lastSize32, lastSize64)         \
        static_assert(MEMBER_OFFSET(Region, memberName) == (Get##camelName##Offset(true)));
        REGION_OFFSET_LIST(REGION_OFFSET_ASSET)
        static_assert(GetFlagOffset(true) == MEMBER_OFFSET(Region, flags_));
        #undef REGION_OFFSET_ASSET
#endif
#ifdef PANDA_TARGET_64
        #define REGION_OFFSET_ASSET(name, camelName, memberName, lastName, lastSize32, lastSize64)         \
        static_assert(MEMBER_OFFSET(Region, memberName) == (Get##camelName##Offset(false)));
        REGION_OFFSET_LIST(REGION_OFFSET_ASSET)
        static_assert(GetFlagOffset(false) == MEMBER_OFFSET(Region, flags_));
        #undef REGION_OFFSET_ASSET
#endif
        return true;
    }

private:
    static constexpr double MOST_OBJECT_ALIVE_THRESHOLD_PERCENT = 0.8;

    RememberedSet *CreateRememberedSet();
    RememberedSet *GetOrCreateCrossRegionRememberedSet();
    RememberedSet *GetOrCreateOldToNewRememberedSet();

    uintptr_t flags_;  // Memory alignment, only low 32bits are used now
    GCBitset *markGCBitset_ {nullptr};
    RememberedSet *oldToNewSet_ {nullptr};
    uintptr_t begin_;
    Space *space_;
    Heap *heap_;

    uintptr_t allocateBase_;
    uintptr_t end_;
    uintptr_t highWaterMark_;
    std::atomic_size_t aliveObject_ {0};
    size_t bitsetSize_ {0};
    Region *next_ {nullptr};
    Region *prev_ {nullptr};

    RememberedSet *crossRegionSet_ {nullptr};
    Span<FreeObjectSet *> sets_;
    size_t wasted_;
    os::memory::Mutex lock_;
    NativeAreaAllocator* nativeAreaAllocator_ {nullptr};
    friend class SnapShot;
};

static_assert(Region::CheckLayout());
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_REGION_H
