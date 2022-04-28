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
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
class ConcurrentMarker;
class ConcurrentSweeper;
class EcmaVM;
class FullGC;
class HeapRegionAllocator;
class HeapTracker;
class Marker;
class MemController;
class NativeAreaAllocator;
class ParallelEvacuator;
class PartialGC;
class STWYoungGC;

using DerivedDataKey = std::pair<uintptr_t, uintptr_t>;

enum class MarkType : uint8_t {
    MARK_YOUNG,
    MARK_FULL
};

class Heap {
public:
    explicit Heap(EcmaVM *ecmaVm);
    ~Heap() = default;
    NO_COPY_SEMANTIC(Heap);
    NO_MOVE_SEMANTIC(Heap);
    void Initialize();
    void Destroy();
    void Prepare();
    void Resume(TriggerGCType gcType);

    // fixme: Rename NewSpace to YoungSpace.
    // This is the active young generation space that the new objects are allocated in
    // or copied into (from the other semi space) during semi space GC.
    SemiSpace *GetNewSpace() const
    {
        return activeSpace_;
    }

    /*
     * Return the original active space where the objects are to be evacuated during semi space GC.
     * This should be invoked only in the evacuation phase of semi space GC.
     * fixme: Get rid of this interface or make it safe considering the above implicit limitation / requirement.
     */
    SemiSpace *GetFromSpaceDuringEvacuation() const
    {
        return inactiveSpace_;
    }

    OldSpace *GetOldSpace() const
    {
        return oldSpace_;
    }

    NonMovableSpace *GetNonMovableSpace() const
    {
        return nonMovableSpace_;
    }

    HugeObjectSpace *GetHugeObjectSpace() const
    {
        return hugeObjectSpace_;
    }

    MachineCodeSpace *GetMachineCodeSpace() const
    {
        return machineCodeSpace_;
    }

    SnapshotSpace *GetSnapshotSpace() const
    {
        return snapshotSpace_;
    }

    SparseSpace *GetSpaceWithType(MemSpaceType type) const
    {
        switch (type) {
            case MemSpaceType::OLD_SPACE:
                return oldSpace_;
                break;
            case MemSpaceType::NON_MOVABLE:
                return nonMovableSpace_;
                break;
            case MemSpaceType::MACHINE_CODE_SPACE:
                return machineCodeSpace_;
                break;
            default:
                UNREACHABLE();
                break;
        }
    }

    STWYoungGC *GetSTWYoungGC() const
    {
        return stwYoungGC_;
    }

    PartialGC *GetPartialGC() const
    {
        return partialGC_;
    }

    FullGC *GetFullGC() const
    {
        return fullGC_;
    }

    ConcurrentSweeper *GetSweeper() const
    {
        return sweeper_;
    }

    ParallelEvacuator *GetEvacuator() const
    {
        return evacuator_;
    }

    ConcurrentMarker *GetConcurrentMarker() const
    {
        return concurrentMarker_;
    }

    Marker *GetNonMovableMarker() const
    {
        return nonMovableMarker_;
    }

    Marker *GetSemiGCMarker() const
    {
        return semiGCMarker_;
    }

    Marker *GetCompressGCMarker() const
    {
        return compressGCMarker_;
    }

    EcmaVM *GetEcmaVM() const
    {
        return ecmaVm_;
    }

    JSThread *GetJSThread() const
    {
        return thread_;
    }

    WorkManager *GetWorkManager() const
    {
        return workManager_;
    }

    MemController *GetMemController() const
    {
        return memController_;
    }

    /*
     * For object allocations.
     */

    // Young
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass, size_t size);
    inline uintptr_t AllocateYoungSync(size_t size);
    inline TaggedObject *TryAllocateYoungGeneration(JSHClass *hclass, size_t size);
    // Old
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass, size_t size);
    // Non-movable
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateDynClassClass(JSHClass *hclass, size_t size);
    // Huge
    inline TaggedObject *AllocateHugeObject(JSHClass *hclass, size_t size);
    // Machine code
    inline TaggedObject *AllocateMachineCodeObject(JSHClass *hclass, size_t size);
    // Snapshot
    inline uintptr_t AllocateSnapshotSpace(size_t size);

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    const HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
    }

    /*
     * GC triggers.
     */
    void CollectGarbage(TriggerGCType gcType);

    void CheckAndTriggerOldGC();

    /*
     * Parallel GC related configurations and utilities.
     */
    void PostParallelGCTask(ParallelGCTaskPhase taskPhase);

    bool IsParallelGCEnabled() const
    {
        return parallelGC_;
    }

    bool CheckCanDistributeTask();

    void WaitRunningTaskFinished();

    /*
     * Concurrent marking related configurations and utilities.
     */
    void EnableConcurrentMarking(bool flag)
    {
        concurrentMarkingEnabled_ = flag;
    }

    bool ConcurrentMarkingEnabled() const
    {
        return concurrentMarkingEnabled_;
    }

    void TryTriggerConcurrentMarking();

    void TriggerConcurrentMarking();

    bool CheckConcurrentMark();

    /*
     * Functions invoked during GC.
     */
    void SetMarkType(MarkType markType)
    {
        markType_ = markType;
    }

    bool IsFullMark() const
    {
        return markType_ == MarkType::MARK_FULL;
    }

    inline void SwapNewSpace();

    inline bool MoveYoungRegionSync(Region *region);
    inline void MergeToOldSpaceSync(LocalSpace *localSpace);

    template<class Callback>
    void EnumerateOldSpaceRegions(const Callback &cb, Region *region = nullptr) const;

    template<class Callback>
    void EnumerateNonNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateSnapshotSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNonMovableRegions(const Callback &cb) const;

    template<class Callback>
    inline void EnumerateRegions(const Callback &cb) const;

    inline void ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd);

    void WaitConcurrentMarkingFinished();

    inline size_t GetCommittedSize() const;

    inline size_t GetHeapObjectSize() const;

    size_t GetPromotedSize() const
    {
        return promotedSize_;
    }

    size_t GetArrayBufferSize() const;

    ChunkMap<DerivedDataKey, uintptr_t> *GetDerivedPointers() const
    {
        return derivedPointers_;
    }
    void UpdateDerivedObjectInStack();
    static constexpr uint32_t STACK_MAP_DEFALUT_DERIVED_SIZE = 8U;

    /*
     * Heap tracking will be used by tools like heap profiler etc.
     */
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

    /*
     * Funtions used by heap verification.
     */
    template<class Callback>
    void IterateOverObjects(const Callback &cb) const;

    bool IsAlive(TaggedObject *object) const;
    bool ContainObject(TaggedObject *object) const;

    size_t VerifyHeapObjects() const;
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool IsVerifying() const
    {
        return isVerifying_;
    }
#endif

private:
    void ThrowOutOfMemoryError(size_t size, std::string functionName);
    void RecomputeLimits();
    void AdjustOldSpaceLimit();
    TriggerGCType SelectGCType() const;
    void IncreaseTaskCount();
    void ReduceTaskCount();
    void WaitClearTaskFinished();
    inline void ReclaimRegions(TriggerGCType gcType, Region *lastRegionOfToSpace = nullptr);

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

    class AsyncClearTask : public Task {
    public:
        AsyncClearTask(Heap *heap, TriggerGCType type) : heap_(heap), gcType_(type)
        {
            lastRegionOfToSpace_ = heap->GetNewSpace()->GetCurrentRegion();
        }
        ~AsyncClearTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncClearTask);
        NO_MOVE_SEMANTIC(AsyncClearTask);
    private:
        Heap *heap_;
        Region *lastRegionOfToSpace_;
        TriggerGCType gcType_;
    };

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};

    /*
     * Heap spaces.
     */

    /*
     * Young generation spaces where most new objects are allocated.
     * (only one of the spaces is active at a time in semi space GC).
     */
    SemiSpace *activeSpace_ {nullptr};
    SemiSpace *inactiveSpace_ {nullptr};

    // Old generation spaces where some long living objects are allocated or promoted.
    OldSpace *oldSpace_ {nullptr};
    OldSpace *compressSpace_ {nullptr};

    // Spaces used for special kinds of objects.
    NonMovableSpace *nonMovableSpace_ {nullptr};
    MachineCodeSpace *machineCodeSpace_ {nullptr};
    HugeObjectSpace *hugeObjectSpace_ {nullptr};
    SnapshotSpace *snapshotSpace_ {nullptr};

    /*
     * Garbage collectors collecting garbage in different scopes.
     */

    /*
     * Semi sapce GC which collects garbage only in young spaces.
     * This is however optional for now because the partial GC also covers its functionality.
     */
    STWYoungGC *stwYoungGC_ {nullptr};

    /*
     * The mostly used partial GC which collects garbage in young spaces,
     * and part of old spaces if needed determined by GC heuristics.
     */
    PartialGC *partialGC_ {nullptr};

    // Full collector which collects garbage in all valid heap spaces.
    FullGC *fullGC_ {nullptr};

    // Concurrent marker which coordinates actions of GC markers and mutators.
    ConcurrentMarker *concurrentMarker_ {nullptr};

    // Concurrent sweeper which coordinates actions of sweepers (in spaces excluding young semi spaces) and mutators.
    ConcurrentSweeper *sweeper_ {nullptr};

    // Parallel evacuator which evacuates objects from one space to another one.
    ParallelEvacuator *evacuator_ {nullptr};

    /*
     * Different kinds of markers used by different collectors.
     * Depending on the collector algorithm, some markers can do simple marking
     *  while some others need to handle object movement.
     */
    Marker *nonMovableMarker_ {nullptr};
    Marker *semiGCMarker_ {nullptr};
    Marker *compressGCMarker_ {nullptr};

    // work manager managing the tasks mostly generated in the GC mark phase.
    WorkManager *workManager_ {nullptr};

    MarkType markType_ {MarkType::MARK_YOUNG};

    bool parallelGC_ {true};
    bool concurrentMarkingEnabled_ {true};
    bool fullGCRequested_ {false};

    size_t globalSpaceAllocLimit_ {GLOBAL_SPACE_LIMIT_BEGIN};
    bool oldSpaceLimitAdjusted_ {false};
    size_t promotedSize_ {0};
    size_t semiSpaceCopiedSize_ {0};

    bool clearTaskFinished_ {true};
    os::memory::Mutex waitClearTaskFinishedMutex_;
    os::memory::ConditionVariable waitClearTaskFinishedCV_;
    uint32_t runningTaskCount_ {0};
    os::memory::Mutex waitTaskFinishedMutex_;
    os::memory::ConditionVariable waitTaskFinishedCV_;

    /*
     * The memory controller providing memory statistics (by allocations and coleections),
     * which is used for GC heuristics.
     */
    MemController *memController_ {nullptr};

    // Region allocators.
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};

    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers_ {nullptr};

    // The tracker tracking heap object allocation and movement events.
    HeapTracker *tracker_ {nullptr};

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool isVerifying_ {false};
#endif
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
