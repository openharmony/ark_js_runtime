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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATION_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATION_H

#include <atomic>
#include <memory>

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/taskpool/task.h"
#include "os/mutex.h"

namespace panda::ecmascript {
class JSHClass;
class TlabAllocator;
class ParallelEvacuation {
public:
    explicit ParallelEvacuation(Heap *heap) : heap_(heap), objXRay_(heap->GetEcmaVM()) {}
    ~ParallelEvacuation() = default;
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
        explicit EvacuationTask(ParallelEvacuation *evacuation);
        ~EvacuationTask() override;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(EvacuationTask);
        NO_MOVE_SEMANTIC(EvacuationTask);

    private:
        ParallelEvacuation *evacuation_;
        TlabAllocator *allocator_ {nullptr};
    };

    class UpdateReferenceTask : public Task {
    public:
        explicit UpdateReferenceTask(ParallelEvacuation *evacuation) : evacuation_(evacuation) {};
        ~UpdateReferenceTask() override = default;

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(UpdateReferenceTask);
        NO_MOVE_SEMANTIC(UpdateReferenceTask);

    private:
        ParallelEvacuation *evacuation_;
    };

    class Workload {
    public:
        Workload(ParallelEvacuation *evacuation, Region *region) : evacuation_(evacuation), region_(region) {};
        virtual ~Workload() = default;
        virtual bool Process(bool isMain) = 0;
        inline Region *GetRegion()
        {
            return region_;
        }

        inline ParallelEvacuation *GetEvacuation()
        {
            return evacuation_;
        }
    protected:
        ParallelEvacuation *evacuation_;
        Region *region_;
    };

    class EvacuationWorkload : public Workload {
    public:
        EvacuationWorkload(ParallelEvacuation *evacuation, Region *region) : Workload(evacuation, region) {}
        ~EvacuationWorkload() = default;
        bool Process(bool isMain) override;
    };

    class UpdateRSetWorkload : public Workload {
    public:
        UpdateRSetWorkload(ParallelEvacuation *evacuation, Region *region) : Workload(evacuation, region) {}
        ~UpdateRSetWorkload() = default;
        bool Process(bool isMain) override;
    };

    class UpdateNewRegionWorkload : public Workload {
    public:
        UpdateNewRegionWorkload(ParallelEvacuation *evacuation, Region *region) : Workload(evacuation, region) {}
        ~UpdateNewRegionWorkload() = default;
        bool Process(bool isMain) override;
    };

    class UpdateAndSweepNewRegionWorkload : public Workload {
    public:
        UpdateAndSweepNewRegionWorkload(ParallelEvacuation *evacuation, Region *region)
            : Workload(evacuation, region) {}
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
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATION_H
