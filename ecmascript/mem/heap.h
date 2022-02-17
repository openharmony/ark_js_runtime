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
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/platform/platform.h"

namespace panda::ecmascript {
class EcmaVM;
class MemManager;
class STWYoungGC;
class MixGC;
class FullGC;
class BumpPointerAllocator;
class FreeListAllocator;
class NativeAreaAllocator;
class HeapRegionAllocator;
class HeapTracker;
class MemController;
class ConcurrentSweeper;
class ConcurrentMarker;
class Marker;
class ParallelEvacuation;
class WorkerHelper;

using DerivedDataKey = std::pair<uintptr_t, uintptr_t>;

enum class MarkType : uint8_t {
    SEMI_MARK,
    FULL_MARK
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

    const SemiSpace *GetNewSpace() const
    {
        return toSpace_;
    }

    const SemiSpace *GetFromSpace() const
    {
        return fromSpace_;
    }

    const OldSpace *GetCompressSpace() const
    {
        return compressSpace_;
    }

    inline void ResetNewSpace();
    inline void ReclaimRegions(TriggerGCType gcType);

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

    STWYoungGC *GetSTWYoungGC() const
    {
        return stwYoungGC_;
    }

    MixGC *GetMixGC() const
    {
        return mixGC_;
    }

    FullGC *GetFullGC() const
    {
        return fullGC_;
    }

    ConcurrentSweeper *GetSweeper() const
    {
        return sweeper_;
    }

    ParallelEvacuation *GetEvacuation() const
    {
        return evacuation_;
    }

    ConcurrentMarker *GetConcurrentMarker() const
    {
        return concurrentMarker_;
    }

    EcmaVM *GetEcmaVM() const
    {
        return ecmaVm_;
    }

    JSThread *GetJSThread() const
    {
        return thread_;
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
    void EnumerateNonNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateSnapShotSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNonMovableRegions(const Callback &cb) const;

    template<class Callback>
    inline void EnumerateRegions(const Callback &cb) const;

    template<class Callback>
    void IteratorOverObjects(const Callback &cb) const;

    void CollectGarbage(TriggerGCType gcType);

    inline bool FillNewSpaceAndTryGC(BumpPointerAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillOldSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillNonMovableSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillSnapShotSpace(BumpPointerAllocator *spaceAllocator);
    inline bool FillMachineCodeSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc = true);
    inline bool FillNewSpaceWithRegion(Region *region);

    void ThrowOutOfMemoryError(size_t size, std::string functionName);

    void SetHeapManager(MemManager *heapManager)
    {
        heapManager_ = heapManager;
    }

    MemManager *GetHeapManager() const
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

    void TryTriggerConcurrentMarking();

    void TriggerConcurrentMarking();

    bool CheckConcurrentMark();

    bool CheckAndTriggerOldGC();

    bool CheckAndTriggerFullGC();

    bool CheckAndTriggerNonMovableGC();

    bool CheckAndTriggerMachineCodeGC();

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    const HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
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

    MemController *GetMemController() const
    {
        return memController_;
    }

    size_t GetOldSpaceAllocLimit()
    {
        return oldSpaceAllocLimit_;
    }

    void SetGlobalSpaceAllocLimit(size_t limit)
    {
        globalSpaceAllocLimit_ = limit;
    }

    size_t VerifyHeapObjects() const;

    inline void ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd);

    ChunkMap<DerivedDataKey, uintptr_t> *GetDerivedPointers() const
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
    void WaitClearTaskFinished();

    bool CheckCanDistributeTask();

    void PostParallelGCTask(ParallelGCTaskPhase gcTask);

    bool IsParallelGCEnabled() const
    {
        return paralledGc_;
    }

    void WaitConcurrentMarkingFinished();

    void SetConcurrentMarkingEnable(bool flag);

    bool ConcurrentMarkingEnable() const;

    void SetMarkType(MarkType markType)
    {
        markType_ = markType;
    }

    bool IsFullMark() const
    {
        return markType_ == MarkType::FULL_MARK;
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

    size_t GetArrayBufferSize() const;

    inline size_t GetCommittedSize() const;

    inline size_t GetHeapObjectSize() const;

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

    class AsyncClearTask : public Task {
    public:
        AsyncClearTask(Heap *heap, TriggerGCType type) : heap_(heap), gcType_(type) {}
        ~AsyncClearTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncClearTask);
        NO_MOVE_SEMANTIC(AsyncClearTask);
    private:
        Heap *heap_;
        TriggerGCType gcType_;
    };

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};
    SemiSpace *fromSpace_ {nullptr};
    SemiSpace *toSpace_ {nullptr};
    OldSpace *oldSpace_ {nullptr};
    OldSpace *compressSpace_ {nullptr};
    HugeObjectSpace *hugeObjectSpace_ {nullptr};
    SnapShotSpace *snapshotSpace_ {nullptr};
    NonMovableSpace *nonMovableSpace_ {nullptr};
    MachineCodeSpace *machineCodeSpace_ {nullptr};
    STWYoungGC *stwYoungGC_ {nullptr};
    MixGC *mixGC_ {nullptr};
    FullGC *fullGC_ {nullptr};
    ConcurrentSweeper *sweeper_ {nullptr};
    Marker *nonMovableMarker_ {nullptr};
    Marker *semiGcMarker_ {nullptr};
    Marker *compressGcMarker_ {nullptr};
    ParallelEvacuation *evacuation_ {nullptr};
    MemManager *heapManager_ {nullptr};
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    HeapTracker *tracker_ {nullptr};
    MemController *memController_ {nullptr};
    size_t oldSpaceAllocLimit_ {OLD_SPACE_LIMIT_BEGIN};
    size_t globalSpaceAllocLimit_ {GLOBAL_SPACE_LIMIT_BEGIN};
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers_ {nullptr};
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool isVerifying_ {false};
#endif

    ConcurrentMarker *concurrentMarker_;
    bool isClearTaskFinished_ = true;
    os::memory::Mutex waitClearTaskFinishedMutex_;
    os::memory::ConditionVariable waitClearTaskFinishedCV_;
    uint32_t runningTastCount_ {0};
    os::memory::Mutex waitTaskFinishedMutex_;
    os::memory::ConditionVariable waitTaskFinishedCV_;
    bool paralledGc_ {true};
    WorkerHelper *workList_ {nullptr};

    MarkType markType_;
    bool concurrentMarkingEnabled_ {true};
    bool isFullGCRequested_ {false};
    inline void SetMaximumCapacity(SemiSpace *space, size_t maximumCapacity);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
