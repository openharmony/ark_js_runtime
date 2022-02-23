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

#include "ecmascript/mem/parallel_evacuation-inl.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/utils.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
void ParallelEvacuation::Initialize()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuationInitialize);
    heap_->ResetNewSpace();
    allocator_ = new TlabAllocator(heap_);
    ageMark_ = heap_->GetFromSpace()->GetAgeMark();
    promotedSize_ = 0;
}

void ParallelEvacuation::Finalize()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuationFinalize);
    delete allocator_;
    heap_->Resume(OLD_GC);
}

void ParallelEvacuation::Evacuate()
{
    ClockScope clockScope;
    Initialize();
    EvacuateSpace();
    UpdateReference();
    Finalize();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticConcurrentEvacuate(clockScope.GetPauseTime());
}

void ParallelEvacuation::EvacuateSpace()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuation);
    heap_->GetFromSpace()->EnumerateRegions([this] (Region *current) {
        AddFragment(std::make_unique<EvacuationFragment>(this, current));
    });
    if (!heap_->GetOldSpace()->IsCSetEmpty()) {
        heap_->GetOldSpace()->EnumerateCollectRegionSet([this](Region *current) {
            AddFragment(std::make_unique<EvacuationFragment>(this, current));
        });
    }
    if (heap_->IsParallelGCEnabled()) {
        os::memory::LockHolder holder(mutex_);
        parallel_ = CalculateEvacuationThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Platform::GetCurrentPlatform()->PostTask(std::make_unique<EvacuationTask>(this));
        }
    }

    EvacuateSpace(allocator_, true);
    WaitFinished();
}

bool ParallelEvacuation::EvacuateSpace(TlabAllocator *allocator, bool isMain)
{
    std::unique_ptr<Fragment> region = GetFragmentSafe();
    while (region != nullptr) {
        EvacuateRegion(allocator, region->GetRegion());
        region = GetFragmentSafe();
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

void ParallelEvacuation::EvacuateRegion(TlabAllocator *allocator, Region *region)
{
    bool isInOldGen = region->InOldGeneration();
    bool isBelowAgeMark = region->BelowAgeMark();
    size_t promotedSize = 0;
    if (!isBelowAgeMark && !isInOldGen && IsWholeRegionEvacuate(region)) {
        if (heap_->GetHeapManager()->MoveYoungRegionSync(region)) {
            return;
        }
    }
    auto markBitmap = region->GetMarkBitmap();
    ASSERT(markBitmap != nullptr);
    markBitmap->IterateOverMarkedChunks([this, &region, &isInOldGen, &isBelowAgeMark,
                                         &promotedSize, &allocator](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        auto klass = header->GetClass();
        auto size = klass->SizeFromJSHClass(header);

        uintptr_t address = 0;
        bool actualPromoted = false;
        bool hasAgeMark = isBelowAgeMark || (region->HasAgeMark() && ToUintPtr(mem) < ageMark_);
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
                if (hasAgeMark) {
                    promotedSize += size;
                }
            }
        }
        LOG_IF(address == 0, FATAL, RUNTIME) << "Evacuate object failed:" << size;

        Utils::Copy(ToVoidPtr(address), size, ToVoidPtr(ToUintPtr(mem)), size);

        Barriers::SetDynPrimitive(header, 0, MarkWord::FromForwardingAddress(address));
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
        VerifyHeapObject(reinterpret_cast<TaggedObject *>(address));
#endif
        if (actualPromoted && klass->HasReferenceField()) {
            SetObjectFieldRSet(reinterpret_cast<TaggedObject *>(address), klass);
        }
    });
    promotedSize_.fetch_add(promotedSize);
}

void ParallelEvacuation::VerifyHeapObject(TaggedObject *object)
{
    auto klass = object->GetClass();
    objXRay_.VisitObjectBody<GCType::OLD_GC>(object, klass,
        [&](TaggedObject *root, ObjectSlot start, ObjectSlot end) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsHeapObject()) {
                    if (value.IsWeakForHeapObject()) {
                        continue;
                    }
                    Region *object_region = Region::ObjectAddressToRange(value.GetTaggedObject());
                    if (!heap_->IsFullMark() && !object_region->InYoungGeneration()) {
                        continue;
                    }
                    auto rset = object_region->GetMarkBitmap();
                    if (!rset->Test(value.GetTaggedObject())) {
                        LOG(FATAL, RUNTIME) << "Miss mark value: " << value.GetTaggedObject()
                                            << ", body address:" << slot.SlotAddress()
                                            << ", header address:" << object;
                    }
                }
            }
        });
}

void ParallelEvacuation::UpdateReference()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelUpdateReference);
    // Update reference pointers
    uint32_t youngeRegionMoveCount = 0;
    uint32_t youngeRegionCopyCount = 0;
    uint32_t oldRegionCount = 0;
    heap_->GetNewSpace()->EnumerateRegions([&] (Region *current) {
        if (current->InNewToNewSet()) {
            AddFragment(std::make_unique<UpdateAndSweepNewRegionFragment>(this, current));
            youngeRegionMoveCount++;
        } else {
            AddFragment(std::make_unique<UpdateNewRegionFragment>(this, current));
            youngeRegionCopyCount++;
        }
    });
    heap_->EnumerateOldSpaceRegions([this, &oldRegionCount] (Region *current) {
        if (current->InCollectSet()) {
            return;
        }
        AddFragment(std::make_unique<UpdateRSetFragment>(this, current));
        oldRegionCount++;
    });
    LOG(DEBUG, RUNTIME) << "UpdatePointers statistic: younge space region compact moving count:"
                        << youngeRegionMoveCount
                        << "younge space region compact coping count:" << youngeRegionCopyCount
                        << "old space region count:" << oldRegionCount;

    if (heap_->IsParallelGCEnabled()) {
        os::memory::LockHolder holder(mutex_);
        parallel_ = CalculateUpdateThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Platform::GetCurrentPlatform()->PostTask(std::make_unique<UpdateReferenceTask>(this));
        }
    }

    UpdateRoot();
    UpdateWeakReference();
    ProcessFragments(true);
    WaitFinished();
}

void ParallelEvacuation::UpdateRoot()
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

void ParallelEvacuation::UpdateWeakReference()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), UpdateWeakReference);
    auto stringTable = heap_->GetEcmaVM()->GetEcmaStringTable();
    bool isFullMark = heap_->IsFullMark();
    WeakRootVisitor gcUpdateWeak = [isFullMark](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (objectRegion->InYoungOrCSetGeneration()) {
            if (objectRegion->InNewToNewSet()) {
                auto markBitmap = objectRegion->GetMarkBitmap();
                if (markBitmap->Test(header)) {
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
            auto markBitmap = objectRegion->GetMarkBitmap();
            if (!markBitmap->Test(header)) {
                return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
            }
        }
        return header;
    };

    stringTable->SweepWeakReference(gcUpdateWeak);
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);
}

void ParallelEvacuation::UpdateRSet(Region *region)
{
    auto rememberedSet = region->GetOldToNewRememberedSet();
    if (LIKELY(rememberedSet != nullptr)) {
        rememberedSet->IterateOverMarkedChunks([this, rememberedSet](void *mem) -> bool {
            ObjectSlot slot(ToUintPtr(mem));
            if (UpdateObjectSlot(slot)) {
                Region *valueRegion = Region::ObjectAddressToRange(slot.GetTaggedObjectHeader());
                if (!valueRegion->InYoungGeneration()) {
                    rememberedSet->Clear(slot.SlotAddress());
                }
            }
            return true;
        });
    }
    if (!heap_->GetOldSpace()->IsCSetEmpty()) {
        rememberedSet = region->GetCrossRegionRememberedSet();
        if (LIKELY(rememberedSet != nullptr)) {
            rememberedSet->IterateOverMarkedChunks([this](void *mem) -> bool {
                ObjectSlot slot(ToUintPtr(mem));
                UpdateObjectSlot(slot);
                return true;
            });
            rememberedSet->ClearAllBits();
        }
    }
}

void ParallelEvacuation::UpdateNewRegionReference(Region *region)
{
    Region *current = heap_->GetNewSpace()->GetCurrentRegion();
    auto curPtr = region->GetBegin();
    uintptr_t endPtr;
    if (region == current) {
        auto top = heap_->GetHeapManager()->GetNewSpaceAllocator().GetTop();
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
            if (klass->HasReferenceField()) {
                UpdateNewObjectField(obj, klass);
            }
            objSize = klass->SizeFromJSHClass(obj);
        } else {
            objSize = freeObject->Available();
        }
        curPtr += objSize;
        CHECK_OBJECT_SIZE(objSize);
    }
    CHECK_REGION_END(curPtr, endPtr);
}

void ParallelEvacuation::UpdateAndSweepNewRegionReference(Region *region)
{
    auto markBitmap = region->GetMarkBitmap();
    uintptr_t freeStart = region->GetBegin();
    uintptr_t freeEnd = freeStart + region->GetAllocatedBytes();
    if (markBitmap != nullptr) {
        markBitmap->IterateOverMarkedChunks([this, &region, &freeStart](void *mem) {
            ASSERT(region->InRange(ToUintPtr(mem)));
            auto header = reinterpret_cast<TaggedObject *>(mem);
            JSHClass *klass = header->GetClass();
            if (klass->HasReferenceField()) {
                UpdateNewObjectField(header, klass);
            }

            uintptr_t freeEnd = ToUintPtr(mem);
            if (freeStart != freeEnd) {
                size_t freeSize = freeEnd - freeStart;
                FreeObject::FillFreeObject(heap_->GetEcmaVM(), freeStart, freeSize);
                SemiSpace *toSpace = const_cast<SemiSpace *>(heap_->GetNewSpace());
                toSpace->DecrementLiveObjectSize(freeSize);
            }

            freeStart = freeEnd + klass->SizeFromJSHClass(header);
        });
    }
    CHECK_REGION_END(freeStart, freeEnd);
    if (freeStart < freeEnd) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), freeStart, freeEnd - freeStart);
    }
}

void ParallelEvacuation::UpdateNewObjectField(TaggedObject *object, JSHClass *cls)
{
    objXRay_.VisitObjectBody<GCType::OLD_GC>(object, cls,
        [this](TaggedObject *root, ObjectSlot start, ObjectSlot end) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                UpdateObjectSlot(slot);
            }
        });
}

void ParallelEvacuation::WaitFinished()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), WaitUpdateFinished);
    if (parallel_ > 0) {
        os::memory::LockHolder holder(mutex_);
        while (parallel_ > 0) {
            condition_.Wait(&mutex_);
        }
    }
}

bool ParallelEvacuation::ProcessFragments(bool isMain)
{
    std::unique_ptr<Fragment> region = GetFragmentSafe();
    while (region != nullptr) {
        region->Process(isMain);
        region = GetFragmentSafe();
    }
    if (!isMain) {
        os::memory::LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

ParallelEvacuation::EvacuationTask::EvacuationTask(ParallelEvacuation *evacuation)
    : evacuation_(evacuation)
{
    allocator_ = new TlabAllocator(evacuation->heap_);
}

ParallelEvacuation::EvacuationTask::~EvacuationTask()
{
    delete allocator_;
}

bool ParallelEvacuation::EvacuationTask::Run(uint32_t threadIndex)
{
    return evacuation_->EvacuateSpace(allocator_);
}

bool ParallelEvacuation::UpdateReferenceTask::Run(uint32_t threadIndex)
{
    evacuation_->ProcessFragments(false);
    return true;
}

bool ParallelEvacuation::EvacuationFragment::Process(bool isMain)
{
    return true;
}

bool ParallelEvacuation::UpdateRSetFragment::Process(bool isMain)
{
    GetEvacuation()->UpdateRSet(GetRegion());
    return true;
}

bool ParallelEvacuation::UpdateNewRegionFragment::Process(bool isMain)
{
    GetEvacuation()->UpdateNewRegionReference(GetRegion());
    return true;
}

bool ParallelEvacuation::UpdateAndSweepNewRegionFragment::Process(bool isMain)
{
    GetEvacuation()->UpdateAndSweepNewRegionReference(GetRegion());
    return true;
}
}  // namespace panda::ecmascript
