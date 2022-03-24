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
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
class EcmaVM;
class STWYoungGC;
class MixGC;
class FullGC;
class BumpPointerAllocator;
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

    SemiSpace *GetNewSpace() const
    {
        return toSpace_;
    }

    SemiSpace *GetFromSpace() const
    {
        return fromSpace_;
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

    SnapShotSpace *GetSnapShotSpace() const
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

    MemController *GetMemController() const
    {
        return memController_;
    }

    inline void SwapNewSpace();

    template<class Callback>
    void EnumerateOldSpaceRegions(const Callback &cb, Region *region = nullptr) const;

    template<class Callback>
    void EnumerateNonNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateSnapShotSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNonMovableRegions(const Callback &cb) const;

    template<class Callback>
    inline void EnumerateRegions(const Callback &cb) const;

    template<class Callback>
    void IteratorOverObjects(const Callback &cb) const;

    TriggerGCType SelectGCType() const;
    void CollectGarbage(TriggerGCType gcType);

    // Young
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass, size_t size);
    inline uintptr_t AllocateYoungSync(size_t size);
    inline TaggedObject *TryAllocateYoungGeneration(JSHClass *hclass, size_t size);
    // Old
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass, size_t size);
    // Nonmovable
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateDynClassClass(JSHClass *hclass, size_t size);
    // Huge
    inline TaggedObject *AllocateHugeObject(JSHClass *hclass, size_t size);
    // Machine code
    inline TaggedObject *AllocateMachineCodeObject(JSHClass *hclass, size_t size);
    // Snapshot
    inline uintptr_t AllocateSnapShotSpace(size_t size);

    inline bool MoveYoungRegionSync(Region *region);
    inline void MergeToOldSpaceSync(LocalSpace *localSpace);

    void ThrowOutOfMemoryError(size_t size, std::string functionName);

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

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    const HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
    }

    bool IsLive(TaggedObject *object) const;
    bool ContainObject(TaggedObject *object) const;

    void RecomputeLimits();

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

    bool CheckCanDistributeTask();

    void PostParallelGCTask(ParallelGCTaskPhase gcTask);

    bool IsParallelGCEnabled() const
    {
        return paralledGc_;
    }

    void WaitConcurrentMarkingFinished();

    void SetConcurrentMarkingEnable(bool flag)
    {
        concurrentMarkingEnabled_ = flag;
    }

    bool ConcurrentMarkingEnable() const
    {
        return concurrentMarkingEnabled_;
    }

    void SetMarkType(MarkType markType)
    {
        markType_ = markType;
    }

    bool IsFullMark() const
    {
        return markType_ == MarkType::FULL_MARK;
    }

    size_t GetArrayBufferSize() const;

    inline size_t GetCommittedSize() const;

    inline size_t GetHeapObjectSize() const;

    void AdjustOldSpaceLimit();

    size_t GetPromotedSize() const
    {
        return promotedSize_;
    }

    size_t GetSemiSpaceCopiedSize() const
    {
        return semiSpaceCopiedSize_;
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

    inline void ReclaimRegions(TriggerGCType gcType);
    void WaitClearTaskFinished();

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};
    SemiSpace *fromSpace_ {nullptr};
    SemiSpace *toSpace_ {nullptr};
    OldSpace *oldSpace_ {nullptr};
    OldSpace *compressSpace_ {nullptr};
    NonMovableSpace *nonMovableSpace_ {nullptr};
    MachineCodeSpace *machineCodeSpace_ {nullptr};
    HugeObjectSpace *hugeObjectSpace_ {nullptr};
    SnapShotSpace *snapshotSpace_ {nullptr};
    STWYoungGC *stwYoungGC_ {nullptr};
    MixGC *mixGC_ {nullptr};
    FullGC *fullGC_ {nullptr};
    ConcurrentSweeper *sweeper_ {nullptr};
    ConcurrentMarker *concurrentMarker_;
    WorkerHelper *workList_ {nullptr};
    Marker *nonMovableMarker_ {nullptr};
    Marker *semiGcMarker_ {nullptr};
    Marker *compressGcMarker_ {nullptr};
    ParallelEvacuation *evacuation_ {nullptr};
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    HeapTracker *tracker_ {nullptr};
    MemController *memController_ {nullptr};
    size_t globalSpaceAllocLimit_ {GLOBAL_SPACE_LIMIT_BEGIN};
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers_ {nullptr};
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool isVerifying_ {false};
#endif

    bool isClearTaskFinished_ = true;
    os::memory::Mutex waitClearTaskFinishedMutex_;
    os::memory::ConditionVariable waitClearTaskFinishedCV_;
    uint32_t runningTastCount_ {0};
    os::memory::Mutex waitTaskFinishedMutex_;
    os::memory::ConditionVariable waitTaskFinishedCV_;
    bool paralledGc_ {true};

    MarkType markType_;
    bool concurrentMarkingEnabled_ {true};
    bool isFullGCRequested_ {false};
    bool oldSpaceLimitAdjusted_ {false};
    size_t startNewSpaceSize_ {0};
    size_t promotedSize_ {0};
    size_t semiSpaceCopiedSize_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
