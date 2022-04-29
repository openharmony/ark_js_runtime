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

#include "ecmascript/mem/parallel_evacuator-inl.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/utils.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
void ParallelEvacuator::Initialize()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuatorInitialize);
    heap_->SwapNewSpace();
    allocator_ = new TlabAllocator(heap_);
    waterLine_ = heap_->GetFromSpace()->GetWaterLine();
    promotedSize_ = 0;
}

void ParallelEvacuator::Finalize()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuatorFinalize);
    delete allocator_;
    heap_->GetSweeper()->PostConcurrentSweepTasks();
    heap_->Resume(OLD_GC);
}

void ParallelEvacuator::Evacuate()
{
    ClockScope clockScope;
    Initialize();
    EvacuateSpace();
    UpdateReference();
    Finalize();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticConcurrentEvacuate(clockScope.GetPauseTime());
}

void ParallelEvacuator::EvacuateSpace()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuator);
    heap_->GetFromSpace()->EnumerateRegions([this] (Region *current) {
        AddWorkload(std::make_unique<EvacuateWorkload>(this, current));
    });
    heap_->GetOldSpace()->EnumerateCollectRegionSet(
        [this](Region *current) {
            AddWorkload(std::make_unique<EvacuateWorkload>(this, current));
        });
    if (heap_->IsParallelGCEnabled()) {
        os::memory::LockHolder holder(mutex_);
        parallel_ = CalculateEvacuationThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<EvacuationTask>(this));
        }
    }

    EvacuateSpace(allocator_, true);
    WaitFinished();
}

bool ParallelEvacuator::EvacuateSpace(TlabAllocator *allocator, bool isMain)
{
    std::unique_ptr<Workload> region = GetWorkloadSafe();
    while (region != nullptr) {
        EvacuateRegion(allocator, region->GetRegion());
        region = GetWorkloadSafe();
    }
    allocator->Finalize();
    if (!isMain) {
        os::memory::LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

void ParallelEvacuator::EvacuateRegion(TlabAllocator *allocator, Region *region)
{
    bool isInOldGen = region->InOldGeneration();
    bool isBelowAgeMark = region->BelowAgeMark();
    size_t promotedSize = 0;
    if (!isBelowAgeMark && !isInOldGen && IsWholeRegionEvacuate(region)) {
        if (heap_->MoveYoungRegionSync(region)) {
            return;
        }
    }
    region->IterateAllMarkedBits([this, &region, &isInOldGen, &isBelowAgeMark,
                                  &promotedSize, &allocator](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        auto klass = header->GetClass();
        auto size = klass->SizeFromJSHClass(header);

        uintptr_t address = 0;
        bool actualPromoted = false;
        bool hasAgeMark = isBelowAgeMark || (region->HasAgeMark() && ToUintPtr(mem) < waterLine_);
        if (hasAgeMark) {
            address = allocator->Allocate(size, OLD_SPACE);
            actualPromoted = true;
            promotedSize += size;
        } else if (isInOldGen) {
            address = allocator->Allocate(size, OLD_SPACE);
            actualPromoted = true;
        } else {
            address = allocator->Allocate(size, SEMI_SPACE);
            if (address == 0) {
                address = allocator->Allocate(size, OLD_SPACE);
                actualPromoted = true;
                promotedSize += size;
            }
        }
        LOG_IF(address == 0, FATAL, RUNTIME) << "Evacuate object failed:" << size;

        Utils::Copy(ToVoidPtr(address), size, ToVoidPtr(ToUintPtr(mem)), size);

        Barriers::SetDynPrimitive(header, 0, MarkWord::FromForwardingAddress(address));
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
        VerifyHeapObject(reinterpret_cast<TaggedObject *>(address));
#endif
        if (actualPromoted) {
            SetObjectFieldRSet(reinterpret_cast<TaggedObject *>(address), klass);
        }
    });
    promotedSize_.fetch_add(promotedSize);
}

void ParallelEvacuator::VerifyHeapObject(TaggedObject *object)
{
    auto klass = object->GetClass();
    objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(object, klass,
        [&]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end, [[maybe_unused]] bool isNative) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsHeapObject()) {
                    if (value.IsWeakForHeapObject()) {
                        continue;
                    }
                    Region *objectRegion = Region::ObjectAddressToRange(value.GetTaggedObject());
                    if (!heap_->IsFullMark() && !objectRegion->InYoungGeneration()) {
                        continue;
                    }
                    if (!objectRegion->Test(value.GetTaggedObject())) {
                        LOG(FATAL, RUNTIME) << "Miss mark value: " << value.GetTaggedObject()
                                            << ", body address:" << slot.SlotAddress()
                                            << ", header address:" << object;
                    }
                }
            }
        });
}

void ParallelEvacuator::UpdateReference()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelUpdateReference);
    // Update reference pointers
    uint32_t youngeRegionMoveCount = 0;
    uint32_t youngeRegionCopyCount = 0;
    uint32_t oldRegionCount = 0;
    heap_->GetNewSpace()->EnumerateRegions([&] (Region *current) {
        if (current->InNewToNewSet()) {
            AddWorkload(std::make_unique<UpdateAndSweepNewRegionWorkload>(this, current));
            youngeRegionMoveCount++;
        } else {
            AddWorkload(std::make_unique<UpdateNewRegionWorkload>(this, current));
            youngeRegionCopyCount++;
        }
    });
    heap_->EnumerateOldSpaceRegions([this, &oldRegionCount] (Region *current) {
        if (current->InCollectSet()) {
            return;
        }
        AddWorkload(std::make_unique<UpdateRSetWorkload>(this, current));
        oldRegionCount++;
    });
    heap_->EnumerateSnapShotSpaceRegions([this] (Region *current) {
        AddWorkload(std::make_unique<UpdateRSetWorkload>(this, current));
    });
    LOG(DEBUG, RUNTIME) << "UpdatePointers statistic: younge space region compact moving count:"
                        << youngeRegionMoveCount
                        << "younge space region compact coping count:" << youngeRegionCopyCount
                        << "old space region count:" << oldRegionCount;

    if (heap_->IsParallelGCEnabled()) {
        os::memory::LockHolder holder(mutex_);
        parallel_ = CalculateUpdateThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<UpdateReferenceTask>(this));
        }
    }

    UpdateRoot();
    UpdateWeakReference();
    ProcessWorkloads(true);
    WaitFinished();
}

void ParallelEvacuator::UpdateRoot()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), UpdateRoot);
    RootVisitor gcUpdateYoung = [this]([[maybe_unused]] Root type, ObjectSlot slot) {
        UpdateObjectSlot(slot);
    };
    RootRangeVisitor gcUpdateRangeYoung = [this]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            UpdateObjectSlot(slot);
        }
    };

    objXRay_.VisitVMRoots(gcUpdateYoung, gcUpdateRangeYoung);
}

void ParallelEvacuator::UpdateRecordWeakReference()
{
    auto totalThreadCount = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1;
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        ProcessQueue *queue = heap_->GetWorkManager()->GetWeakReferenceQueue(i);

        while (true) {
            auto obj = queue->PopBack();
            if (UNLIKELY(obj == nullptr)) {
                break;
            }
            ObjectSlot slot(ToUintPtr(obj));
            JSTaggedValue value(slot.GetTaggedType());
            ASSERT(value.IsWeak() || value.IsUndefined());
            if (value.IsWeak()) {
                UpdateWeakObjectSlot(value.GetTaggedWeakRef(), slot);
            }
        }
    }
}

void ParallelEvacuator::UpdateWeakReference()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), UpdateWeakReference);
    UpdateRecordWeakReference();
    auto stringTable = heap_->GetEcmaVM()->GetEcmaStringTable();
    bool isFullMark = heap_->IsFullMark();
    WeakRootVisitor gcUpdateWeak = [isFullMark](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (objectRegion->InYoungOrCSetGeneration()) {
            if (objectRegion->InNewToNewSet()) {
                if (objectRegion->Test(header)) {
                    return header;
                }
            } else {
                MarkWord markWord(header);
                if (markWord.IsForwardingAddress()) {
                    return markWord.ToForwardingAddress();
                }
            }
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (isFullMark) {
            if (objectRegion->GetMarkGCBitset() == nullptr || !objectRegion->Test(header)) {
                return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
            }
        }
        return header;
    };

    stringTable->SweepWeakReference(gcUpdateWeak);
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);
}

void ParallelEvacuator::UpdateRSet(Region *region)
{
    region->IterateAllOldToNewBits([this](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        if (UpdateObjectSlot(slot)) {
            Region *valueRegion = Region::ObjectAddressToRange(slot.GetTaggedObjectHeader());
            if (!valueRegion->InYoungGeneration()) {
                return false;
            }
        }
        return true;
    });
    region->IterateAllCrossRegionBits([this](void *mem) {
        ObjectSlot slot(ToUintPtr(mem));
        UpdateObjectSlot(slot);
    });
    region->ClearCrossRegionRSet();
}

void ParallelEvacuator::UpdateNewRegionReference(Region *region)
{
    Region *current = heap_->GetNewSpace()->GetCurrentRegion();
    auto curPtr = region->GetBegin();
    uintptr_t endPtr;
    if (region == current) {
        auto top = heap_->GetNewSpace()->GetTop();
        endPtr = curPtr + region->GetAllocatedBytes(top);
    } else {
        endPtr = curPtr + region->GetAllocatedBytes();
    }

    size_t objSize = 0;
    while (curPtr < endPtr) {
        auto freeObject = FreeObject::Cast(curPtr);
        if (!freeObject->IsFreeObject()) {
            auto obj = reinterpret_cast<TaggedObject *>(curPtr);
            auto klass = obj->GetClass();
            UpdateNewObjectField(obj, klass);
            objSize = klass->SizeFromJSHClass(obj);
        } else {
            objSize = freeObject->Available();
        }
        curPtr += objSize;
        CHECK_OBJECT_SIZE(objSize);
    }
    CHECK_REGION_END(curPtr, endPtr);
}

void ParallelEvacuator::UpdateAndSweepNewRegionReference(Region *region)
{
    uintptr_t freeStart = region->GetBegin();
    uintptr_t freeEnd = freeStart + region->GetAllocatedBytes();
    region->IterateAllMarkedBits([&](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        JSHClass *klass = header->GetClass();
        UpdateNewObjectField(header, klass);

        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            size_t freeSize = freeEnd - freeStart;
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), freeStart, freeSize);
            SemiSpace *toSpace = const_cast<SemiSpace *>(heap_->GetNewSpace());
            toSpace->DecrementSurvivalObjectSize(freeSize);
        }

        freeStart = freeEnd + klass->SizeFromJSHClass(header);
    });
    CHECK_REGION_END(freeStart, freeEnd);
    if (freeStart < freeEnd) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), freeStart, freeEnd - freeStart);
    }
}

void ParallelEvacuator::UpdateNewObjectField(TaggedObject *object, JSHClass *cls)
{
    objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls,
        [this]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end, [[maybe_unused]] bool isNative) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                UpdateObjectSlot(slot);
            }
        });
}

void ParallelEvacuator::WaitFinished()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), WaitUpdateFinished);
    if (parallel_ > 0) {
        os::memory::LockHolder holder(mutex_);
        while (parallel_ > 0) {
            condition_.Wait(&mutex_);
        }
    }
}

bool ParallelEvacuator::ProcessWorkloads(bool isMain)
{
    std::unique_ptr<Workload> region = GetWorkloadSafe();
    while (region != nullptr) {
        region->Process(isMain);
        region = GetWorkloadSafe();
    }
    if (!isMain) {
        os::memory::LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

ParallelEvacuator::EvacuationTask::EvacuationTask(ParallelEvacuator *evacuator)
    : evacuator_(evacuator)
{
    allocator_ = new TlabAllocator(evacuator->heap_);
}

ParallelEvacuator::EvacuationTask::~EvacuationTask()
{
    delete allocator_;
}

bool ParallelEvacuator::EvacuationTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    return evacuator_->EvacuateSpace(allocator_);
}

bool ParallelEvacuator::UpdateReferenceTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    evacuator_->ProcessWorkloads(false);
    return true;
}

bool ParallelEvacuator::EvacuateWorkload::Process([[maybe_unused]] bool isMain)
{
    return true;
}

bool ParallelEvacuator::UpdateRSetWorkload::Process([[maybe_unused]] bool isMain)
{
    GetEvacuator()->UpdateRSet(GetRegion());
    return true;
}

bool ParallelEvacuator::UpdateNewRegionWorkload::Process([[maybe_unused]] bool isMain)
{
    GetEvacuator()->UpdateNewRegionReference(GetRegion());
    return true;
}

bool ParallelEvacuator::UpdateAndSweepNewRegionWorkload::Process([[maybe_unused]] bool isMain)
{
    GetEvacuator()->UpdateAndSweepNewRegionReference(GetRegion());
    return true;
}
}  // namespace panda::ecmascript
