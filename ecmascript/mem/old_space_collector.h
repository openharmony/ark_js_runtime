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

#ifndef ECMASCRIPT_MEM_OLD_SAPACE_COLLECTOR_H
#define ECMASCRIPT_MEM_OLD_SAPACE_COLLECTOR_H

#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/mark_stack-inl.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/semi_space_worker.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/heap_roots.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/semi_space_collector.h"

namespace panda {
namespace ecmascript {
class Heap;
class JSHClass;

class OldSpaceCollector : public GarbageCollector {
public:
    explicit OldSpaceCollector(Heap *heap, bool parallelGc);
    ~OldSpaceCollector() override = default;
    NO_COPY_SEMANTIC(OldSpaceCollector);
    NO_MOVE_SEMANTIC(OldSpaceCollector);
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

    void ProcessMarkStack(uint64_t threadId);
    void MarkObjectBody(TaggedObject *object, JSHClass *klass, const EcmaObjectRangeVisitor &visitor);

    inline void MarkObject(uint64_t threadId, TaggedObject *object);
    inline void RecordWeakReference(uint64_t threadId, JSTaggedType *ref);

    Heap *heap_;
    HeapRootManager rootManager_;
    bool paralledGC_{false};
    OldGCWorker *workList_{nullptr};
    size_t freeSize_{0};
    size_t hugeSpaceFreeSize_ = 0;
    size_t oldSpaceCommitSize_ = 0;
    size_t nonMoveSpaceCommitSize_ = 0;

    friend class OldGCWorker;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_OLD_SAPACE_COLLECTOR_H
