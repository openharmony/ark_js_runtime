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

#ifndef ECMASCRIPT_MEM_EVACUATION_ALLOCATOR_H
#define ECMASCRIPT_MEM_EVACUATION_ALLOCATOR_H

#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/platform/task.h"
#include "os/mutex.h"

namespace panda::ecmascript {
class Heap;
class EvacuationAllocator {
public:
    EvacuationAllocator(Heap *heap) : heap_(heap), isFreeTaskFinish_(true), oldSpaceAllocator_(){};
    ~EvacuationAllocator() = default;
    NO_COPY_SEMANTIC(EvacuationAllocator);
    NO_MOVE_SEMANTIC(EvacuationAllocator);

    void Initialize(TriggerGCType type);
    void Finalize(TriggerGCType type);

    inline uintptr_t GetNewSpaceTop()
    {
        return newSpaceAllocator_.GetTop();
    }

    bool AddRegionToOld(Region *region);
    bool AddRegionToYoung(Region *region);

    inline Region *ExpandOldSpace();
    uintptr_t AllocateOld(size_t size);
    uintptr_t AllocateYoung(size_t size);

    inline void FreeSafe(uintptr_t begin, uintptr_t end);
    inline void Free(uintptr_t begin, uintptr_t end, bool isAdd = true);
    inline void FillFreeList(FreeObjectKind *kind);

    void ReclaimRegions(TriggerGCType type);

    void WaitFreeTaskFinish();

private:
    class AsyncFreeRegionTask : public Task {
    public:
        AsyncFreeRegionTask(EvacuationAllocator *allocator, TriggerGCType type)
            : allocator_(allocator), gcType_(type) {}
        ~AsyncFreeRegionTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncFreeRegionTask);
        NO_MOVE_SEMANTIC(AsyncFreeRegionTask);
    private:
        EvacuationAllocator *allocator_;
        TriggerGCType gcType_;
    };

    Heap *heap_;
    bool isFreeTaskFinish_ = true;
    BumpPointerAllocator newSpaceAllocator_;
    FreeListAllocator oldSpaceAllocator_;
    os::memory::Mutex youngAllocatorLock_;
    os::memory::Mutex oldAllocatorLock_;
    // Async free region task
    os::memory::Mutex mutex_;
    os::memory::ConditionVariable condition_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_EVACUATION_ALLOCATOR_H
