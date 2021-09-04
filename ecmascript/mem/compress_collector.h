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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_COMPRESS_COLLECTOR_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_COMPRESS_COLLECTOR_H

#include "ecmascript/mem/compress_gc_marker.h"
#include "ecmascript/mem/semi_space_collector.h"
#include "ecmascript/mem/semi_space_worker.h"

namespace panda {
namespace ecmascript {
class Heap;
class JSHClass;

class CompressCollector : public GarbageCollector {
public:
    explicit CompressCollector(Heap *heap, bool parallelGc);
    ~CompressCollector() override;

    NO_COPY_SEMANTIC(CompressCollector);
    NO_MOVE_SEMANTIC(CompressCollector);

    void RunPhases();

    Heap *GetHeap() const
    {
        return heap_;
    }

private:
    void InitializePhase();
    void MarkingPhase();
    void SweepPhases();
    void FinishPhase();
    void ProcessMarkStack(uint32_t threadId);

    uintptr_t AllocateOld(size_t size);
    void RecordWeakReference(uint32_t threadId, JSTaggedType *ref);
    void SweepSpace(Space *space, FreeListAllocator &allocator);
    void SweepSpace(LargeObjectSpace *space);  // Only sweep large space.
    void FreeLiveRange(FreeListAllocator &allocator, Region *current, uintptr_t freeStart, uintptr_t freeEnd);

    Heap *heap_;
    bool paralledGC_;
    CompressGCMarker marker_;
    HeapRootManager rootManager_;
    CompressGCWorker *workList_;
    os::memory::Mutex mtx_;
    BumpPointerAllocator fromSpaceAllocator_{};
    FreeListAllocator oldSpaceAllocator_{};
    FreeListAllocator nonMovableAllocator_{};

    size_t youngAndOldAliveSize_ = 0;
    size_t nonMoveSpaceFreeSize_ = 0;
    size_t youngSpaceCommitSize_ = 0;
    size_t oldSpaceCommitSize_ = 0;
    size_t nonMoveSpaceCommitSize_ = 0;

    friend class TlabAllocator;
    friend class CompressGCMarker;
    friend class CompressGCWorker;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_COMPRESS_COLLECTOR_H
