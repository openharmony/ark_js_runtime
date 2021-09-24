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

#ifndef ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_H
#define ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_H

#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/mark_stack-inl.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/heap_roots.h"
#include "ecmascript/mem/remembered_set.h"

#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/tlab_allocator.h"

#include "ecmascript/thread/thread_pool.h"
#include "ecmascript/mem/semi_space_marker.h"

#include "os/mutex.h"

namespace panda {
namespace ecmascript {
class Heap;
class JSHClass;
class SemiSpaceWorker;

class GarbageCollector {
public:
    GarbageCollector() = default;
    virtual ~GarbageCollector() = default;
    DEFAULT_COPY_SEMANTIC(GarbageCollector);
    DEFAULT_MOVE_SEMANTIC(GarbageCollector);
};

class SemiSpaceCollector : public GarbageCollector {
public:
    explicit SemiSpaceCollector(Heap *heap, bool parallelGc);
    ~SemiSpaceCollector() override;
    NO_COPY_SEMANTIC(SemiSpaceCollector);
    NO_MOVE_SEMANTIC(SemiSpaceCollector);

    void RunPhases();

    Heap *GetHeap() const
    {
        return heap_;
    }

private:
    bool ParallelHandleOldToNew(uint32_t threadId, Region *region);
    bool ParallelHandleThreadRoots(uint32_t threadId);
    bool ParallelHandleSnapShot(uint32_t threadId);
    bool ParallelHandleGlobalPool(uint32_t threadId);
    void InitializePhase();
    void ParallelMarkingPhase();
    void SweepPhases();
    void FinishPhase();
    void ProcessMarkStack(uint64_t threadId);

    inline uintptr_t AllocateYoung(size_t size);
    inline uintptr_t AllocateOld(size_t size);
    inline void UpdatePromotedSlot(TaggedObject *object, ObjectSlot slot);
    inline void RecordWeakReference(uint32_t threadId, JSTaggedType *ref);
    inline bool BlowAgeMark(uintptr_t address);

    Heap *heap_;
    HeapRootManager rootManager_;
    os::memory::Mutex allocatorLock_;
    BumpPointerAllocator fromSpaceAllocator_{};
    FreeListAllocator oldSpaceAllocator_{};
    bool paralledGC_{false};
    SemiSpaceWorker *workList_{nullptr};
    SemiSpaceMarker markObject_;
    size_t promotedSize_{0};
    size_t semiCopiedSize_{0};
    size_t commitSize_ = 0;
    uintptr_t ageMark_{0};
    friend class TlabAllocator;
    friend class SemiSpaceWorker;
    friend class SemiSpaceMarker;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_H
