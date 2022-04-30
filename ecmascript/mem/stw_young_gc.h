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

#ifndef ECMASCRIPT_MEM_STW_YOUNG_GC_H
#define ECMASCRIPT_MEM_STW_YOUNG_GC_H

#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/tlab_allocator.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"

#include "os/mutex.h"

namespace panda {
namespace ecmascript {
class STWYoungGC : public GarbageCollector {
public:
    explicit STWYoungGC(Heap *heap, bool parallelGC);
    ~STWYoungGC() override = default;
    NO_COPY_SEMANTIC(STWYoungGC);
    NO_MOVE_SEMANTIC(STWYoungGC);

    virtual void RunPhases() override;

protected:
    virtual void Initialize() override;
    virtual void Mark() override;
    virtual void Sweep() override;
    virtual void Finish() override;

private:
    inline void UpdatePromotedSlot(TaggedObject *object, ObjectSlot slot)
    {
#ifndef NDEBUG
        JSTaggedValue value(slot.GetTaggedType());
        ASSERT(value.IsHeapObject());
#endif
        Region *objectRegion = Region::ObjectAddressToRange(object);
        ASSERT(!objectRegion->InYoungGeneration());
        objectRegion->InsertOldToNewRSet(slot.SlotAddress());
    }

    Heap *heap_;
    size_t promotedSize_ {0};
    size_t semiCopiedSize_ {0};
    size_t commitSize_ {0};

    // obtain from heap
    bool parallelGC_ {false};
    WorkManager *workManager_ {nullptr};

    friend class TlabAllocator;
    friend class WorkManager;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_STW_YOUNG_GC_H
