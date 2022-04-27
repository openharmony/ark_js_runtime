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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATOR_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATOR_H

#include <atomic>
#include <memory>

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/tlab_allocator.h"
#include "ecmascript/taskpool/task.h"
#include "os/mutex.h"

namespace panda::ecmascript {
class ParallelEvacuator {
public:
    explicit ParallelEvacuator(Heap *heap) : heap_(heap), objXRay_(heap->GetEcmaVM()) {}
    ~ParallelEvacuator() = default;
    void Initialize();
    void Finalize();
    void Evacuate();

    size_t GetPromotedSize() const
    {
        return promotedSize_;
    }

private:
    class EvacuationTask : public Task {
    public:
        explicit EvacuationTask(ParallelEvacuator *evacuator);
        ~EvacuationTask() override;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(EvacuationTask);
        NO_MOVE_SEMANTIC(EvacuationTask);

    private:
        ParallelEvacuator *evacuator_;
        TlabAllocator *allocator_ {nullptr};
    };

    class UpdateReferenceTask : public Task {
    public:
        explicit UpdateReferenceTask(ParallelEvacuator *evacuator) : evacuator_(evacuator) {};
        ~UpdateReferenceTask() override = default;

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(UpdateReferenceTask);
        NO_MOVE_SEMANTIC(UpdateReferenceTask);

    private:
        ParallelEvacuator *evacuator_;
    };

    class Workload {
    public:
        Workload(ParallelEvacuator *evacuator, Region *region) : evacuator_(evacuator), region_(region) {};
        virtual ~Workload() = default;
        virtual bool Process(bool isMain) = 0;
        inline Region *GetRegion()
        {
            return region_;
        }

        inline ParallelEvacuator *GetEvacuator()
        {
            return evacuator_;
        }
    protected:
        ParallelEvacuator *evacuator_;
        Region *region_;
    };

    class EvacuateWorkload : public Workload {
    public:
        EvacuateWorkload(ParallelEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        ~EvacuateWorkload() = default;
        bool Process(bool isMain) override;
    };

    class UpdateRSetWorkload : public Workload {
    public:
        UpdateRSetWorkload(ParallelEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        ~UpdateRSetWorkload() = default;
        bool Process(bool isMain) override;
    };

    class UpdateNewRegionWorkload : public Workload {
    public:
        UpdateNewRegionWorkload(ParallelEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        ~UpdateNewRegionWorkload() = default;
        bool Process(bool isMain) override;
    };

    class UpdateAndSweepNewRegionWorkload : public Workload {
    public:
        UpdateAndSweepNewRegionWorkload(ParallelEvacuator *evacuator, Region *region)
            : Workload(evacuator, region) {}
        ~UpdateAndSweepNewRegionWorkload() = default;
        bool Process(bool isMain) override;
    };

    bool ProcessWorkloads(bool isMain = false);

    void EvacuateSpace();
    bool EvacuateSpace(TlabAllocator *allocation, bool isMain = false);
    void EvacuateRegion(TlabAllocator *allocator, Region *region);
    inline void SetObjectFieldRSet(TaggedObject *object, JSHClass *cls);

    inline bool IsWholeRegionEvacuate(Region *region);
    void VerifyHeapObject(TaggedObject *object);

    void UpdateReference();
    void UpdateRoot();
    void UpdateWeakReference();
    void UpdateRecordWeakReference();
    void UpdateRSet(Region *region);
    void UpdateNewRegionReference(Region *region);
    void UpdateAndSweepNewRegionReference(Region *region);
    void UpdateNewObjectField(TaggedObject *object, JSHClass *cls);

    inline bool UpdateObjectSlot(ObjectSlot &slot);
    inline bool UpdateWeakObjectSlot(TaggedObject *object, ObjectSlot &slot);

    inline std::unique_ptr<Workload> GetWorkloadSafe();
    inline void AddWorkload(std::unique_ptr<Workload> region);

    inline int CalculateEvacuationThreadNum();
    inline int CalculateUpdateThreadNum();
    void WaitFinished();

    Heap *heap_;
    TlabAllocator *allocator_ {nullptr};
    ObjectXRay objXRay_;

    uintptr_t waterLine_ = 0;
    std::vector<std::unique_ptr<Workload>> workloads_;
    std::atomic_int parallel_ = 0;
    os::memory::Mutex mutex_;
    os::memory::ConditionVariable condition_;
    std::atomic<size_t> promotedSize_ = 0;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATOR_H
