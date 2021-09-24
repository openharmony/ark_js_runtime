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

#ifndef ECMASCRIPT_MEM_HEAP_H
#define ECMASCRIPT_MEM_HEAP_H

#include "ecmascript/thread/thread_pool.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/space.h"

namespace panda::ecmascript {
class EcmaVM;
class EcmaHeapManager;
class SemiSpaceCollector;
class OldSpaceCollector;
class CompressCollector;
class BumpPointerAllocator;
class FreeListAllocator;
class RegionFactory;
class HeapTracker;
class MemController;
class ConcurrentSweeper;

class Heap {
public:
    explicit Heap(EcmaVM *ecmaVm);
    ~Heap() = default;
    NO_COPY_SEMANTIC(Heap);
    NO_MOVE_SEMANTIC(Heap);
    void Initialize();
    void Destroy();

    const SemiSpace *GetNewSpace() const
    {
        return toSpace_;
    }

    inline void SetNewSpaceAgeMark(uintptr_t mark);

    inline void SetNewSpaceMaximumCapacity(size_t maximumCapacity);

    const SemiSpace *GetFromSpace() const
    {
        return fromSpace_;
    }

    const OldSpace *GetCompressSpace() const
    {
        return compressSpace_;
    }

    inline void InitializeFromSpace();

    inline void SwapSpace();

    inline void ReclaimFromSpaceRegions();

    inline void SetFromSpaceMaximumCapacity(size_t maximumCapacity);

    void SetFromSpace(SemiSpace *space)
    {
        fromSpace_ = space;
    }

    const OldSpace *GetOldSpace() const
    {
        return oldSpace_;
    }

    const NonMovableSpace *GetNonMovableSpace() const
    {
        return nonMovableSpace_;
    }

    const HugeObjectSpace *GetHugeObjectSpace() const
    {
        return hugeObjectSpace_;
    }

    const MachineCodeSpace *GetMachineCodeSpace() const
    {
        return machineCodeSpace_;
    }

    SemiSpaceCollector *GetSemiSpaceCollector() const
    {
        return semiSpaceCollector_;
    }

    OldSpaceCollector *GetOldSpaceCollector() const
    {
        return oldSpaceCollector_;
    }

    CompressCollector *GetCompressCollector() const
    {
        return compressCollector_;
    }

    ConcurrentSweeper *GetSweeper() const
    {
        return sweeper_;
    }

    EcmaVM *GetEcmaVM() const
    {
        return ecmaVm_;
    }

    ThreadPool *GetThreadPool() const
    {
        return pool_;
    }

    void FlipNewSpace();

    void FlipCompressSpace();

    template<class Callback>
    void EnumerateOldSpaceRegions(const Callback &cb, Region *region = nullptr) const;

    template<class Callback>
    void EnumerateNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateSnapShotSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateRegions(const Callback &cb) const;

    template<class Callback>
    void IteratorOverObjects(const Callback &cb) const;

    void CollectGarbage(TriggerGCType gcType);

    inline bool FillNewSpaceAndTryGC(BumpPointerAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillOldSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillNonMovableSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillSnapShotSpace(BumpPointerAllocator *spaceAllocator);
    inline bool FillMachineCodeSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc = true);

    void ThrowOutOfMemoryError(size_t size);

    void SetHeapManager(EcmaHeapManager *heapManager)
    {
        heapManager_ = heapManager;
    }

    EcmaHeapManager *GetHeapManager() const
    {
        return heapManager_;
    }

    void StartHeapTracking(HeapTracker *tracker)
    {
        tracker_ = tracker;
    }

    void StopHeapTracking()
    {
        tracker_ = nullptr;
    }

    inline void OnAllocateEvent(uintptr_t address);
    inline void OnMoveEvent(uintptr_t address, uintptr_t forwardAddress);

    bool CheckAndTriggerOldGC();

    bool CheckAndTriggerCompressGC();

    bool CheckAndTriggerNonMovableGC();

    const RegionFactory *GetRegionFactory() const
    {
        return regionFactory_;
    }

    SnapShotSpace *GetSnapShotSpace() const
    {
        return snapshotSpace_;
    }

    bool IsLive(TaggedObject *object) const
    {
        if (!ContainObject(object)) {
            return false;
        }

        // semi space
        if (toSpace_->IsLive(object)) {
            return true;
        }
        // old space
        if (oldSpace_->IsLive(object)) {
            return true;
        }
        // non movable space
        if (nonMovableSpace_->IsLive(object)) {
            return true;
        }
        // huge object space
        if (hugeObjectSpace_->IsLive(object)) {
            return true;
        }
        return false;
    }

    bool ContainObject(TaggedObject *object) const
    {
        // semi space
        if (toSpace_->ContainObject(object)) {
            return true;
        }
        // old space
        if (oldSpace_->ContainObject(object)) {
            return true;
        }
        // non movable space
        if (nonMovableSpace_->ContainObject(object)) {
            return true;
        }
        // huge object space
        if (hugeObjectSpace_->ContainObject(object)) {
            return true;
        }

        return false;
    }

    void RecomputeLimits();

    const MemController *GetMemController() const
    {
        return memController_;
    }

    void SetOldSpaceAllocLimit(size_t limit)
    {
        oldSpaceAllocLimit_ = limit;
    }

    inline void ResetAppStartup();

    size_t VerifyHeapObjects() const;

    inline void ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd);

private:
    EcmaVM *ecmaVm_ {nullptr};
    SemiSpace *fromSpace_ {nullptr};
    SemiSpace *toSpace_ {nullptr};
    OldSpace *oldSpace_ {nullptr};
    OldSpace *compressSpace_ {nullptr};
    HugeObjectSpace *hugeObjectSpace_ {nullptr};
    SnapShotSpace *snapshotSpace_ {nullptr};
    NonMovableSpace *nonMovableSpace_ {nullptr};
    MachineCodeSpace *machineCodeSpace_ {nullptr};
    SemiSpaceCollector *semiSpaceCollector_ {nullptr};
    OldSpaceCollector *oldSpaceCollector_ {nullptr};
    CompressCollector *compressCollector_ {nullptr};
    ConcurrentSweeper *sweeper_ {nullptr};
    EcmaHeapManager *heapManager_ {nullptr};
    RegionFactory *regionFactory_ {nullptr};
    HeapTracker *tracker_ {nullptr};
    MemController *memController_ {nullptr};
    ThreadPool *pool_ {nullptr};
    size_t oldSpaceAllocLimit_ {OLD_SPACE_LIMIT_BEGIN};

    inline void SetMaximumCapacity(SemiSpace *space, size_t maximumCapacity);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
