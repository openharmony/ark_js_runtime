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

#ifndef ECMASCRIPT_MEM_CONCURRENT_SWEEPER_H
#define ECMASCRIPT_MEM_CONCURRENT_SWEEPER_H

#include <array>
#include <atomic>

#include "ecmascript/mem/space.h"
#include "ecmascript/platform/task.h"
#include "os/mutex.h"

namespace panda::ecmascript {
class FreeListAllocator;

class ConcurrentSweeper {
public:
    ConcurrentSweeper(Heap *heap, bool concurrentSweep);
    ~ConcurrentSweeper() = default;

    NO_COPY_SEMANTIC(ConcurrentSweeper);
    NO_MOVE_SEMANTIC(ConcurrentSweeper);

    void SweepPhases(bool compressGC = false);

    void WaitAllTaskFinished();
    // Help to finish sweeping task. It can be called through js thread
    void EnsureAllTaskFinished();
    // Ensure task finish
    void EnsureTaskFinished(MemSpaceType type);

    bool FillSweptRegion(MemSpaceType type, FreeListAllocator *allocator);

    bool IsSweeping()
    {
        return isSweeping_;
    }

    bool IsOldSpaceSweeped() const
    {
        return isOldSpaceSweeped_;
    }

    void SetOldSpaceSweeped(bool isSweeped)
    {
        isOldSpaceSweeped_ = isSweeped;
    }

private:
    class SweeperTask : public Task {
    public:
        SweeperTask(ConcurrentSweeper *sweeper, MemSpaceType type) : sweeper_(sweeper), type_(type) {};
        ~SweeperTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(SweeperTask);
        NO_MOVE_SEMANTIC(SweeperTask);

    private:
        ConcurrentSweeper *sweeper_;
        MemSpaceType type_;
    };

    void PrepareSpace(bool compressGC);

    void SweepSpace(MemSpaceType type, FreeListAllocator *allocator = nullptr, bool isMain = true);
    void SweepSpace(MemSpaceType type, Space *space, FreeListAllocator &allocator);
    void SweepHugeSpace();
    void FinishSweeping(MemSpaceType type);

    void FreeRegion(Region *current, FreeListAllocator &allocator, bool isMain = true);
    void FreeLiveRange(FreeListAllocator &allocator, Region *current, uintptr_t freeStart, uintptr_t freeEnd,
        bool isMain);

    void AddRegion(MemSpaceType type, Space *space, Region *region);
    void SortRegion(MemSpaceType type);
    Region *GetRegionSafe(MemSpaceType type);

    void AddSweptRegionSafe(MemSpaceType type, Region *region);
    Region *GetSweptRegionSafe(MemSpaceType type);

    void WaitingTaskFinish(MemSpaceType type);

    std::array<os::memory::Mutex, FREE_LIST_NUM> mutexs_;
    std::array<os::memory::ConditionVariable, FREE_LIST_NUM> cvs_;
    std::array<std::atomic_int, FREE_LIST_NUM> remainderTaskNum_ = {0, 0, 0};

    std::array<std::vector<Region *>, FREE_LIST_NUM> sweepingList_;
    std::array<std::vector<Region *>, FREE_LIST_NUM> sweptList_;

    Heap *heap_;
    bool concurrentSweep_ {false};
    bool isSweeping_ {false};
    bool isOldSpaceSweeped_ {false};
    MemSpaceType startSpaceType_ = MemSpaceType::OLD_SPACE;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CONCURRENT_SWEEPER_H
