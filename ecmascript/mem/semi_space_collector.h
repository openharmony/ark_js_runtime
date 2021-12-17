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

#include "os/mutex.h"

namespace panda {
namespace ecmascript {
class Heap;
class JSHClass;
class WorkerHelper;

class GarbageCollector {
public:
    GarbageCollector() = default;
    virtual ~GarbageCollector() = default;
    DEFAULT_COPY_SEMANTIC(GarbageCollector);
    DEFAULT_MOVE_SEMANTIC(GarbageCollector);
};

class SemiSpaceCollector : public GarbageCollector {
public:
    explicit SemiSpaceCollector(Heap *heap, bool paralledGc);
    ~SemiSpaceCollector() override = default;
    NO_COPY_SEMANTIC(SemiSpaceCollector);
    NO_MOVE_SEMANTIC(SemiSpaceCollector);

    void RunPhases();

private:
    void InitializePhase();
    void ParallelMarkingPhase();
    void SweepPhases();
    void FinishPhase();

    inline void UpdatePromotedSlot(TaggedObject *object, ObjectSlot slot);

    Heap *heap_;
    size_t promotedSize_{0};
    size_t semiCopiedSize_{0};
    size_t commitSize_ = 0;

    // obtain from heap
    bool paralledGc_ {false};
    WorkerHelper *workList_ {nullptr};

    friend class TlabAllocator;
    friend class WorkerHelper;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_H
