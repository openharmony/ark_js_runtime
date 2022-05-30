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

#ifndef ECMASCRIPT_MEM_REGION_H
#define ECMASCRIPT_MEM_REGION_H

#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/remembered_set.h"
#include "securec.h"

namespace panda {
namespace ecmascript {
class JSThread;

enum RegionFlags {
    NEVER_EVACUATE = 1,
    HAS_AGE_MARK = 1 << 1,
    BELOW_AGE_MARK = 1 << 2,
    IN_YOUNG_SPACE = 1 << 3,
    IN_SNAPSHOT_SPACE = 1 << 4,
    IN_HUGE_OBJECT_SPACE = 1 << 5,
    IN_OLD_SPACE = 1 << 6,
    IN_NON_MOVABLE_SPACE = 1 << 7,
    IN_MACHINE_CODE_SPACE = 1 << 8,
    IN_COLLECT_SET = 1 << 9,
    IN_NEW_TO_NEW_SET = 1 << 10,
    HAS_BEEN_SWEPT = 1 << 11,
    NEED_RELOCATE = 1 << 12,
    INVALID = 1 << 13,
};

static inline bool IsFlagSet(uintptr_t flags, RegionFlags target)
{
    return (flags & target) == target;
}

static inline std::string ToSpaceTypeName(uintptr_t flags)
{
    if (IsFlagSet(flags, RegionFlags::IN_YOUNG_SPACE)) {
        return "young space";
    }

    if (IsFlagSet(flags, RegionFlags::IN_OLD_SPACE)) {
        return "old space";
    }

    if (IsFlagSet(flags, RegionFlags::IN_SNAPSHOT_SPACE)) {
        return "snapshot space";
    }

    if (IsFlagSet(flags, RegionFlags::IN_HUGE_OBJECT_SPACE)) {
        return "huge object space";
    }

    if (IsFlagSet(flags, IN_MACHINE_CODE_SPACE)) {
        return "machine code space";
    }

    if (IsFlagSet(flags, IN_NON_MOVABLE_SPACE)) {
        return "non movable space";
    }

    return "other space";
}

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
    Region(JSThread *thread, uintptr_t allocateBase, uintptr_t begin, uintptr_t end, RegionFlags flags)
        : flags_(flags),
          thread_(thread),
          reclaimed_(false),
          allocateBase_(allocateBase),
          end_(end),
          highWaterMark_(end),
          aliveObject_(0),
          wasted_(0)
    {
        bitsetSize_ = IsFlagSet(flags_, RegionFlags::IN_HUGE_OBJECT_SPACE) ?
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

    uintptr_t GetHighWaterMark() const
    {
        return highWaterMark_;
    }

    size_t GetCapacity() const
    {
        return end_ - reinterpret_cast<uintptr_t>(this);
    }

    size_t GetSize() const
    {
        return end_ - begin_;
    }

    JSThread *GetJSThread() const
    {
        return thread_;
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

    std::string GetSpaceTypeName()
    {
        return ToSpaceTypeName(flags_);
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
    void AtomicClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteCrossRegionRSet();
    // Old to new remembered set
    void InsertOldToNewRSet(uintptr_t addr);
    template <typename Visitor>
    void IterateAllOldToNewBits(Visitor visitor);
    void ClearOldToNewRSet();
    void ClearOldToNewRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteOldToNewRSet();

    void AtomicClearSweepingRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteSweepingRSet();
    template <typename Visitor>
    void AtomicIterateAllSweepingRSetBits(Visitor visitor);

    static Region *ObjectAddressToRange(TaggedObject *obj)
    {
        return reinterpret_cast<Region *>(ToUintPtr(obj) & ~DEFAULT_REGION_MASK);
    }

    static Region *ObjectAddressToRange(uintptr_t objAddress)
    {
        return reinterpret_cast<Region *>(objAddress & ~DEFAULT_REGION_MASK);
    }

    bool IsReclaimed() const
    {
        return reclaimed_;
    }

    void SetReclaimed()
    {
        reclaimed_ = true;
    }

    bool InYoungSpace() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_YOUNG_SPACE);
    }

    bool InOldSpace() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_OLD_SPACE);
    }

    bool InYoungOrOldSpace() const
    {
        return InYoungSpace() || InOldSpace();
    }

    bool InHugeObjectSpace() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_HUGE_OBJECT_SPACE);
    }

    bool InMachineCodeSpace() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_MACHINE_CODE_SPACE);
    }

    bool InNonMovableSpace() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_NON_MOVABLE_SPACE);
    }

    bool InCollectSet() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_COLLECT_SET);
    }

    bool InYoungSpaceOrCSet() const
    {
        return InYoungSpace() || InCollectSet();
    }

    bool InNewToNewSet() const
    {
        return IsFlagSet(flags_, RegionFlags::IN_NEW_TO_NEW_SET);
    }

    bool HasAgeMark() const
    {
        return IsFlagSet(flags_, RegionFlags::HAS_AGE_MARK);
    }

    bool BelowAgeMark() const
    {
        return IsFlagSet(flags_, RegionFlags::BELOW_AGE_MARK);
    }

    bool NeedRelocate() const
    {
        return IsFlagSet(flags_, RegionFlags::NEED_RELOCATE);
    }

    void SetSwept()
    {
        SetFlag(RegionFlags::HAS_BEEN_SWEPT);
    }

    void ResetSwept()
    {
        ClearFlag(RegionFlags::HAS_BEEN_SWEPT);
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

    size_t GetHighWaterMarkSize() const
    {
        return highWaterMark_ - allocateBase_;
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

    void InitializeFreeObjectSets()
    {
        freeObjectSets_ = Span<FreeObjectSet *>(new FreeObjectSet *[FreeObjectList::NumberOfSets()](),
            FreeObjectList::NumberOfSets());
    }

    void DestroyFreeObjectSets()
    {
        for (auto set : freeObjectSets_) {
            delete set;
        }
        delete[] freeObjectSets_.data();
    }

    FreeObjectSet *GetFreeObjectSet(SetType type)
    {
        // Thread safe
        if (freeObjectSets_[type] == nullptr) {
            freeObjectSets_[type] = new FreeObjectSet(type);
        }
        return freeObjectSets_[type];
    }

    template<class Callback>
    void EnumerateFreeObjectSets(Callback cb)
    {
        for (auto set : freeObjectSets_) {
            cb(set);
        }
    }

    inline bool IsMarking() const;

    void IncreaseAliveObjectSafe(size_t size)
    {
        ASSERT(aliveObject_ + size <= GetSize());
        aliveObject_ += size;
    }

    void IncreaseAliveObject(size_t size)
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
    void IncreaseWasted(size_t size)
    {
        wasted_ += size;
    }
    size_t GetWastedSize()
    {
        return wasted_;
    }

    void SwapRSetForConcurrentSweeping()
    {
        sweepingRSet_ = oldToNewSet_;
        oldToNewSet_ = nullptr;
    }

    // should call in js-thread
    void MergeRSetForConcurrentSweeping();

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
        #define REGION_OFFSET_ASSERT(name, camelName, memberName, lastName, lastSize32, lastSize64)         \
        static_assert(MEMBER_OFFSET(Region, memberName) == (Get##camelName##Offset(true)));
        REGION_OFFSET_LIST(REGION_OFFSET_ASSERT)
        static_assert(GetFlagOffset(true) == MEMBER_OFFSET(Region, flags_));
        #undef REGION_OFFSET_ASSERT
#endif
#ifdef PANDA_TARGET_64
        #define REGION_OFFSET_ASSERT(name, camelName, memberName, lastName, lastSize32, lastSize64)         \
        static_assert(MEMBER_OFFSET(Region, memberName) == (Get##camelName##Offset(false)));
        REGION_OFFSET_LIST(REGION_OFFSET_ASSERT)
        static_assert(GetFlagOffset(false) == MEMBER_OFFSET(Region, flags_));
        #undef REGION_OFFSET_ASSERT
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
    /*
     * The thread instance here is used by the GC barriers to get marking related information
     * and perform marking related operations. The barriers will indirectly access such information
     * via. the objects' associated regions.
     * fixme: Figure out a more elegant solution to bridging the barrier
     * and the information / operations it depends on. Then we can get rid of this from the region,
     * and consequently, the region allocator, the spaces using the region allocator, etc.
     */
    JSThread *thread_;
    bool reclaimed_ {false};

    uintptr_t allocateBase_;
    uintptr_t end_;
    uintptr_t highWaterMark_;
    std::atomic_size_t aliveObject_ {0};
    size_t bitsetSize_ {0};
    Region *next_ {nullptr};
    Region *prev_ {nullptr};

    RememberedSet *crossRegionSet_ {nullptr};
    RememberedSet *sweepingRSet_ {nullptr};
    Span<FreeObjectSet *> freeObjectSets_;
    size_t wasted_;
    os::memory::Mutex lock_;
    friend class Snapshot;
    friend class SnapshotProcessor;
};

static_assert(Region::CheckLayout());
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_REGION_H
