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

namespace panda::ecmascript {
void ParallelEvacuation::Initialize()
{
    evacuationAllocator_->Initialize(TriggerGCType::OLD_GC);
    allocator_ = new TlabAllocator(heap_, TriggerGCType::COMPRESS_FULL_GC);
    ageMark_ = heap_->GetFromSpace()->GetAgeMark();
}

void ParallelEvacuation::Finalize()
{
    delete allocator_;
    evacuationAllocator_->Finalize(TriggerGCType::OLD_GC);
}

void ParallelEvacuation::Evacuate()
{
    ClockScope clockScope;
    Initialize();
    EvacuateSpace();
    UpdateReference();
}

void ParallelEvacuation::EvacuateSpace()
{
    heap_->GetFromSpace()->EnumerateRegions([this] (Region *current) {
        AddFragment(std::make_unique<EvacuationFragment>(this, current));
    });
    if (!heap_->IsSemiMarkNeeded()) {
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
    bool isPromoted = region->BelowAgeMark() || region->InOldGeneration();
    if (IsPromoteComplete(region)) {
        bool ret = false;
        if (isPromoted) {
            if (region->InYoungGeneration()) {
                promotedAccumulatorSize_.fetch_add(region->AliveObject());
            }
            ret = evacuationAllocator_->AddRegionToOld(region);
        } else {
            ret = evacuationAllocator_->AddRegionToYoung(region);
            if (!ret) {
                if (region->InYoungGeneration()) {
                    promotedAccumulatorSize_.fetch_add(region->AliveObject());
                }
                ret = evacuationAllocator_->AddRegionToOld(region);
            }
        }
        if (!ret) {
            LOG(FATAL, RUNTIME) << "Add Region failed:";
        }
    } else {
        auto markBitmap = region->GetMarkBitmap();
        ASSERT(markBitmap != nullptr);
        markBitmap->IterateOverMarkedChunks([this, &region, &isPromoted, &allocator](void *mem) {
            ASSERT(region->InRange(ToUintPtr(mem)));
            auto header = reinterpret_cast<TaggedObject *>(mem);
            auto klass = header->GetClass();
            auto size = klass->SizeFromJSHClass(header);

            uintptr_t address = 0;
            if (isPromoted || (region->HasAgeMark() && ToUintPtr(mem) < ageMark_)) {
                address = allocator->Allocate(size, SpaceAlloc::OLD_SPACE);
                promotedAccumulatorSize_.fetch_add(size);
            } else {
                address = allocator->Allocate(size, SpaceAlloc::YOUNG_SPACE);
                if (address == 0) {
                    address = allocator->Allocate(size, SpaceAlloc::OLD_SPACE);
                    promotedAccumulatorSize_.fetch_add(size);
                }
            }
            LOG_IF(address == 0, FATAL, RUNTIME) << "Evacuate object failed:" << size;

            if (memcpy_sp(ToVoidPtr(address), size, ToVoidPtr(ToUintPtr(mem)), size) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
            }

            Barriers::SetDynPrimitive(header, 0, MarkWord::FromForwardingAddress(address));
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
            VerifyHeapObject(reinterpret_cast<TaggedObject *>(address));
#endif
        });
    }
}

void ParallelEvacuation::VerifyHeapObject(TaggedObject *object)
{
    auto klass = object->GetClass();
    objXRay_.VisitObjectBody<GCType::OLD_GC>(object, klass,
        [&](TaggedObject *root, ObjectSlot start, ObjectSlot end) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsHeapObject()) {
                    if (value.IsWeak()) {
                        continue;
                    }
                    Region *object_region = Region::ObjectAddressToRange(value.GetTaggedObject());
                    if (heap_->IsSemiMarkNeeded() && !object_region->InYoungGeneration()) {
                        continue;
                    }
                    auto reset = object_region->GetMarkBitmap();
                    if (!reset->Test(value.GetTaggedObject())) {
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
    // Update reference pointers
    uint32_t youngeRegionMoveCount = 0;
    uint32_t oldRegionMoveCount = 0;
    uint32_t youngeRegionCopyCount = 0;
    uint32_t oldRegionCopyCount = 0;
    heap_->GetNewSpace()->EnumerateRegions([&] (Region *current) {
        if (current->InPromoteSet()) {
            AddFragment(std::make_unique<UpdateAndSweepNewRegionFragment>(this, current));
            youngeRegionMoveCount++;
        } else {
            AddFragment(std::make_unique<UpdateNewRegionFragment>(this, current));
            youngeRegionCopyCount++;
        }
    });
    heap_->GetCompressSpace()->EnumerateRegions([&] (Region *current) {
        if (current->InPromoteSet()) {
            AddFragment(std::make_unique<UpdateAndSweepCompressRegionFragment>(this, current));
            oldRegionMoveCount++;
        } else {
            AddFragment(std::make_unique<UpdateCompressRegionFragment>(this, current));
            oldRegionCopyCount++;
        }
    });
    heap_->EnumerateOldSpaceRegions([this] (Region *current) {
        if (current->InCollectSet()) {
            return;
        }
        AddFragment(std::make_unique<UpdateRSetFragment>(this, current));
    });
    LOG(INFO, RUNTIME) << "UpdatePointers statistic: younge space region compact moving count:" << youngeRegionMoveCount
                       << "younge space region compact coping count:" << youngeRegionCopyCount
                       << "old space region compact moving count:" << oldRegionMoveCount
                       << "old space region compact coping count:" << oldRegionCopyCount;

    // Optimization weak reference for native pointer
    UpdateWeakReference();
    if (heap_->IsParallelGCEnabled()) {
        os::memory::LockHolder holder(mutex_);
        parallel_ = CalculateUpdateThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Platform::GetCurrentPlatform()->PostTask(std::make_unique<UpdateReferenceTask>(this));
        }
    }

    UpdateRoot();
    ProcessFragments(true);
    WaitFinished();
    FillSweptRegion();
}

void ParallelEvacuation::UpdateRoot()
{
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
    bool isOnlySemi = heap_->IsSemiMarkNeeded();
    auto stringTable = heap_->GetEcmaVM()->GetEcmaStringTable();
    WeakRootVisitor gcUpdateWeak = [&](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (objectRegion->InYoungAndCSetGeneration() && !objectRegion->InPromoteSet()) {
            MarkWord markWord(header);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        } else {
            if (!isOnlySemi || objectRegion->InPromoteSet()) {
                auto markBitmap = objectRegion->GetMarkBitmap();
                ASSERT(markBitmap != nullptr);
                if (!markBitmap->Test(header)) {
                    return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
                }
            }
            return header;
        }
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
    if (!heap_->IsSemiMarkNeeded()) {
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

void ParallelEvacuation::UpdateCompressRegionReference(Region *region)
{
    auto curPtr = region->GetBegin();
    auto endPtr = region->GetEnd();
    auto rset = region->GetOldToNewRememberedSet();
    if (rset != nullptr) {
        rset->ClearAllBits();
    }

    size_t objSize = 0;
    while (curPtr < endPtr) {
        auto freeObject = FreeObject::Cast(curPtr);
        if (!freeObject->IsFreeObject()) {
            auto obj = reinterpret_cast<TaggedObject *>(curPtr);
            auto klass = obj->GetClass();
            if (klass->HasReferenceField()) {
                UpdateCompressObjectField(region, obj, klass);
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

void ParallelEvacuation::UpdateNewRegionReference(Region *region)
{
    Region *current = heap_->GetNewSpace()->GetCurrentRegion();
    auto curPtr = region->GetBegin();
    uintptr_t endPtr;
    if (region == current) {
        auto top = evacuationAllocator_->GetNewSpaceTop();
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
                FreeObject::FillFreeObject(heap_->GetEcmaVM(), freeStart, freeEnd - freeStart);
            }

            freeStart = freeEnd + klass->SizeFromJSHClass(header);
        });
    }
    CHECK_REGION_END(freeStart, freeEnd);
    if (freeStart < freeEnd) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), freeStart, freeEnd - freeStart);
    }
}

void ParallelEvacuation::UpdateAndSweepCompressRegionReference(Region *region, bool isMain)
{
    auto markBitmap = region->GetMarkBitmap();
    auto rset = region->GetOldToNewRememberedSet();
    if (rset != nullptr) {
        rset->ClearAllBits();
    }
    uintptr_t freeStart = region->GetBegin();
    if (markBitmap != nullptr) {
        markBitmap->IterateOverMarkedChunks([this, &region, &freeStart, &isMain](void *mem) {
            ASSERT(region->InRange(ToUintPtr(mem)));
            ASSERT(!FreeObject::Cast(ToUintPtr(mem))->IsFreeObject());
            auto header = reinterpret_cast<TaggedObject *>(mem);
            auto klass = header->GetClass();
            ObjectSlot slot(ToUintPtr(&klass));
            UpdateObjectSlot(slot);
            if (klass->HasReferenceField()) {
                UpdateCompressObjectField(region, header, klass);
            }

            uintptr_t freeEnd = ToUintPtr(mem);
            if (freeStart != freeEnd) {
                evacuationAllocator_->Free(freeStart, freeEnd, isMain);
            }
            freeStart = freeEnd + klass->SizeFromJSHClass(header);
        });
    }
    uintptr_t freeEnd = region->GetEnd();
    CHECK_REGION_END(freeStart, freeEnd);
    if (freeStart < freeEnd) {
        evacuationAllocator_->Free(freeStart, freeEnd, isMain);
    }
    if (!isMain) {
        AddSweptRegionSafe(region);
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

void ParallelEvacuation::UpdateCompressObjectField(Region *region, TaggedObject *object, JSHClass *cls)
{
    objXRay_.VisitObjectBody<GCType::OLD_GC>(object, cls,
        [this, region](TaggedObject *root, ObjectSlot start, ObjectSlot end) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                if (UpdateObjectSlot(slot)) {
                    Region *valueRegion = Region::ObjectAddressToRange(slot.GetTaggedObjectHeader());
                    if (valueRegion->InYoungGeneration()) {
                        region->InsertOldToNewRememberedSet(slot.SlotAddress());
                    }
                }
            }
        });
}

void ParallelEvacuation::WaitFinished()
{
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
    allocator_ = new TlabAllocator(evacuation->heap_, TriggerGCType::COMPRESS_FULL_GC);
}

ParallelEvacuation::EvacuationTask::~EvacuationTask()
{
    allocator_->Finalize();
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

bool ParallelEvacuation::UpdateCompressRegionFragment::Process(bool isMain)
{
    GetEvacuation()->UpdateCompressRegionReference(GetRegion());
    return true;
}

bool ParallelEvacuation::UpdateAndSweepNewRegionFragment::Process(bool isMain)
{
    GetEvacuation()->UpdateAndSweepNewRegionReference(GetRegion());
    return true;
}

bool ParallelEvacuation::UpdateAndSweepCompressRegionFragment::Process(bool isMain)
{
    GetEvacuation()->UpdateAndSweepCompressRegionReference(GetRegion(), isMain);
    return true;
}
}  // namespace panda::ecmascript
