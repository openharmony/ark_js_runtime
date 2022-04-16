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

#ifndef ECMASCRIPT_MEM_MIX_GC_H
#define ECMASCRIPT_MEM_MIX_GC_H

#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/stw_young_gc_for_testing.h"

namespace panda {
namespace ecmascript {
class Heap;
class JSHClass;

class MixGC : public GarbageCollector {
public:
    explicit MixGC(Heap *heap);
    ~MixGC() override = default;
    NO_COPY_SEMANTIC(MixGC);
    NO_MOVE_SEMANTIC(MixGC);
    void RunPhases();

    Heap *GetHeap() const
    {
        return heap_;
    }

private:
    void InitializePhase();
    void MarkingPhase();
    void SweepPhases();
    void ProcessNativeDelete();
    void EvacuaPhases();
    void FinishPhase();

    Heap *heap_;
    size_t freeSize_ {0};
    size_t hugeSpaceFreeSize_ = 0;
    size_t oldSpaceCommitSize_ = 0;
    size_t nonMoveSpaceCommitSize_ = 0;
    bool concurrentMark_ {false};
    // obtain from heap
    WorkerHelper *workList_ {nullptr};

    friend class WorkerHelper;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_MIX_GC_H
