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

#include "ecmascript/base/config.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/platform/platform.h"
#include "ecmascript/mem/parallel_work_helper.h"

namespace panda::ecmascript {
class EcmaVM;
class EcmaHeapManager;
class SemiSpaceCollector;
class MixSpaceCollector;
class CompressCollector;
class BumpPointerAllocator;
class FreeListAllocator;
class RegionFactory;
class HeapTracker;
class MemController;
class ConcurrentSweeper;
class ConcurrentMarker;
class Marker;
class ParallelEvacuation;
class EvacuationAllocator;
class WorkerHelper;

using DerivedData = std::tuple<uintptr_t, uintptr_t, uintptr_t>;

class Heap {
public:
    explicit Heap(EcmaVM *ecmaVm);
    ~Heap() = default;
    NO_COPY_SEMANTIC(Heap);
    NO_MOVE_SEMANTIC(Heap);
    void Initialize();
    void Destroy();
    void Prepare();

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
    inline void InitializeCompressSpace();

    inline void SwapSpace();

    inline void ReclaimFromSpaceRegions();

    inline void SetFromSpaceMaximumCapacity(size_t maximumCapacity);

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

    MixSpaceCollector *GetMixSpaceCollector() const
    {
        return mixSpaceCollector_;
    }

    CompressCollector *GetCompressCollector() const
    {
        return compressCollector_;
    }

    ConcurrentSweeper *GetSweeper() const
    {
        return sweeper_;
    }

    ParallelEvacuation *GetEvacuation() const
    {
        return evacuation_;
    }

    EvacuationAllocator *GetEvacuationAllocator() const
    {
        return evacuationAllocator_;
    }

    ConcurrentMarker *GetConcurrentMarker() const
    {
        return concurrentMarker_;
    }

    EcmaVM *GetEcmaVM() const
    {
        return ecmaVm_;
    }

    WorkerHelper *GetWorkList() const
    {
        return workList_;
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
    void EnumerateNonMovableRegions(const Callback &cb) const;

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
    inline Region *ExpandCompressSpace();
    inline bool AddRegionToCompressSpace(Region *region);
    inline bool AddRegionToToSpace(Region *region);

    void ThrowOutOfMemoryError(size_t size, std::string functionName);

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

    void TryTriggerConcurrentMarking(bool allowGc);

    void TriggerConcurrentMarking();

    void CheckNeedFullMark();

    bool CheckConcurrentMark(JSThread *thread);

    bool CheckAndTriggerOldGC();

    bool CheckAndTriggerCompressGC();

    bool CheckAndTriggerNonMovableGC();

    bool CheckAndTriggerMachineCodeGC();

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

    ChunkVector<DerivedData> *GetDerivedPointers() const
    {
        return derivedPointers_;
    }
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool GetIsVerifying() const
    {
        return isVerifying_;
    }
#endif

    void UpdateDerivedObjectInStack();
    static constexpr uint32_t STACK_MAP_DEFALUT_DERIVED_SIZE = 8U;

    void WaitRunningTaskFinished();

    bool CheckCanDistributeTask();

    void PostParallelGCTask(ParallelGCTaskPhase gcTask);

    bool IsEnableParallelGC() const
    {
        return paralledGc_;
    }

    void WaitConcurrentMarkingFinished();

    void SetConcurrentMarkingEnable(bool flag);

    bool ConcurrentMarkingEnable() const;

    inline bool IsOnlyMarkSemi() const
    {
        return isOnlySemi_;
    }

    void SetOnlyMarkSemi(bool onlySemi)
    {
        isOnlySemi_ = onlySemi;
    }

    Marker *GetNonMovableMarker() const
    {
        return nonMovableMarker_;
    }

    Marker *GetSemiGcMarker() const
    {
        return semiGcMarker_;
    }

    Marker *GetCompressGcMarker() const
    {
        return compressGcMarker_;
    }

private:
    void IncreaseTaskCount();

    void ReduceTaskCount();

    class ParallelGCTask : public Task {
    public:
        ParallelGCTask(Heap *heap, ParallelGCTaskPhase taskPhase) : heap_(heap), taskPhase_(taskPhase) {};
        ~ParallelGCTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ParallelGCTask);
        NO_MOVE_SEMANTIC(ParallelGCTask);

    private:
        Heap *heap_ {nullptr};
        ParallelGCTaskPhase taskPhase_;
    };

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
    MixSpaceCollector *mixSpaceCollector_ {nullptr};
    CompressCollector *compressCollector_ {nullptr};
    ConcurrentSweeper *sweeper_ {nullptr};
    Marker *nonMovableMarker_ {nullptr};
    Marker *semiGcMarker_ {nullptr};
    Marker *compressGcMarker_ {nullptr};
    ParallelEvacuation *evacuation_ {nullptr};
    EvacuationAllocator *evacuationAllocator_ {nullptr};
    EcmaHeapManager *heapManager_ {nullptr};
    RegionFactory *regionFactory_ {nullptr};
    HeapTracker *tracker_ {nullptr};
    MemController *memController_ {nullptr};
    size_t oldSpaceAllocLimit_ {OLD_SPACE_LIMIT_BEGIN};
    ChunkVector<DerivedData> *derivedPointers_ {nullptr};
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool isVerifying_ {false};
#endif

    ConcurrentMarker *concurrentMarker_;
    uint32_t runningTastCount_ {0};
    os::memory::Mutex waitTaskFinishedMutex_;
    os::memory::ConditionVariable waitTaskFinishedCV_;
    bool paralledGc_ {true};
    WorkerHelper *workList_ {nullptr};

    bool concurrentMarkingEnable_ {true};
    bool isOnlySemi_ {true};
    inline void SetMaximumCapacity(SemiSpace *space, size_t maximumCapacity);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
