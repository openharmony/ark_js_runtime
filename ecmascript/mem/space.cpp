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

#include "ecmascript/mem/space-inl.h"

#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/region_factory.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
Space::Space(Heap *heap, MemSpaceType spaceType, size_t initialCapacity, size_t maximumCapacity)
    : heap_(heap),
      vm_(heap_->GetEcmaVM()),
      thread_(vm_->GetJSThread()),
      regionFactory_(vm_->GetRegionFactory()),
      spaceType_(spaceType),
      initialCapacity_(initialCapacity),
      maximumCapacity_(maximumCapacity),
      committedSize_(0)
{
}

void Space::Initialize()
{
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    if (spaceType_ == MemSpaceType::SEMI_SPACE) {
        region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);
    } else if (spaceType_ == MemSpaceType::SNAPSHOT_SPACE) {
        region->SetFlag(RegionFlags::IS_IN_SNAPSHOT_GENERATION);
    } else if (spaceType_ == MemSpaceType::OLD_SPACE) {
        region->InitializeSet();
        region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    } else if (spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->InitializeSet();
        region->SetFlag(RegionFlags::IS_IN_NON_MOVABLE_GENERATION);
        int res = region->SetCodeExecutableAndReadable();
        LOG_ECMA_MEM(DEBUG) << "Initialize SetCodeExecutableAndReadable" << res;
    } else if (spaceType_ == MemSpaceType::NON_MOVABLE) {
        region->InitializeSet();
        region->SetFlag(RegionFlags::IS_IN_NON_MOVABLE_GENERATION);
    }

    AddRegion(region);
}

void Space::ReclaimRegions()
{
    EnumerateRegions([this](Region *current) { ClearAndFreeRegion(current); });
    regionList_.Clear();
    committedSize_ = 0;
}

void Space::ClearAndFreeRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Clear region from:" << region << " to " << ToSpaceTypeName(spaceType_);
    region->DeleteMarkBitmap();
    region->DeleteCrossRegionRememberedSet();
    region->DeleteOldToNewRememberedSet();
    DecrementCommitted(region->GetCapacity());
    DecrementLiveObjectSize(region->AliveObject());
    if (spaceType_ == MemSpaceType::OLD_SPACE || spaceType_ == MemSpaceType::NON_MOVABLE ||
        spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->DestroySet();
    }
    regionFactory_->FreeRegion(region);
}

SemiSpace::SemiSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::SEMI_SPACE, initialCapacity, maximumCapacity), ageMark_(0)
{
}

bool SemiSpace::Expand(uintptr_t top, bool isAllow)
{
    if (committedSize_ >= maximumCapacity_ + overShootSize_) {
        return false;
    }

    auto currentRegion = GetCurrentRegion();
    currentRegion->SetHighWaterMark(top);
    if (isAllow) {
        if (currentRegion->HasAgeMark()) {
            allocateAfterLastGC_ += currentRegion->GetAllocatedBytes(top) - currentRegion->GetAllocatedBytes(ageMark_);
        } else {
            allocateAfterLastGC_ += currentRegion->GetAllocatedBytes(top);
        }
    } else {
        // For GC
        survivalObjectSize_ += currentRegion->GetAllocatedBytes(top);
    }
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);

    AddRegion(region);
    return true;
}

bool SemiSpace::SwapRegion(Region *region, SemiSpace *fromSpace)
{
    if (committedSize_ + region->GetCapacity() > maximumCapacity_ + overShootSize_) {
        return false;
    }
    fromSpace->RemoveRegion(region);

    region->SetFlag(RegionFlags::IS_IN_NEW_TO_NEW_SET);
    region->SetSpace(this);
    regionList_.AddNodeToFront(region);
    IncrementCommitted(region->GetCapacity());
    survivalObjectSize_ += region->GetAllocatedBytes();
    return true;
}

void SemiSpace::SetAgeMark(uintptr_t mark)
{
    ageMark_ = mark;
    Region *last = GetCurrentRegion();
    last->SetFlag(RegionFlags::HAS_AGE_MARK);

    EnumerateRegions([&last](Region *current) {
        if (current != last) {
            current->SetFlag(RegionFlags::BELOW_AGE_MARK);
        }
    });
    allocateAfterLastGC_ = 0;
    survivalObjectSize_ += last->GetAllocatedBytes(ageMark_);
}

size_t SemiSpace::GetHeapObjectSize() const
{
    return survivalObjectSize_ + allocateAfterLastGC_;
}

void SemiSpace::SetOverShootSize(size_t size)
{
    overShootSize_ = size;
}

void SemiSpace::Reset()
{
    overShootSize_ = 0;
    survivalObjectSize_ = 0;
    allocateAfterLastGC_ = 0;
}

bool SemiSpace::AdjustCapacity(size_t allocatedSizeSinceGC)
{
    static constexpr double growObjectSurvivalRate = 0.8;
    static constexpr double shrinkObjectSurvivalRate = 0.2;
    static constexpr int growingFactor = 2;
    if (allocatedSizeSinceGC <= maximumCapacity_ * growObjectSurvivalRate / growingFactor) {
        return false;
    }
    double curObjectSurvivalRate = static_cast<double>(survivalObjectSize_) / allocatedSizeSinceGC;
    if (curObjectSurvivalRate > growObjectSurvivalRate) {
        if (GetMaximumCapacity() >= MAX_SEMI_SPACE_CAPACITY) {
            return false;
        }
        size_t newCapacity = GetMaximumCapacity() * growingFactor;
        SetMaximumCapacity(std::min(newCapacity, MAX_SEMI_SPACE_CAPACITY));
        return true;
    } else if (curObjectSurvivalRate < shrinkObjectSurvivalRate) {
        if (GetMaximumCapacity() <= SEMI_SPACE_CAPACITY) {
            return false;
        }
        size_t newCapacity = GetMaximumCapacity() / growingFactor;
        SetMaximumCapacity(std::max(newCapacity, SEMI_SPACE_CAPACITY));
        return true;
    }
    return false;
}

bool SemiSpace::ContainObject(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            return true;
        }
        region = region->GetNext();
    }

    return false;
}

bool SemiSpace::IsLive(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            auto freeObject = FreeObject::Cast(ToUintPtr(object));
            return !freeObject->IsFreeObject();
        }
        region = region->GetNext();
    }
    return false;
}

void SemiSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    auto current = GetCurrentRegion();
    EnumerateRegions([&](Region *region) {
        auto curPtr = region->GetBegin();
        uintptr_t endPtr;
        if (region == current) {
            auto top = heap_->GetHeapManager()->GetNewSpaceAllocator().GetTop();
            endPtr = curPtr + region->GetAllocatedBytes(top);
        } else {
            endPtr = curPtr + region->GetAllocatedBytes();
        }

        size_t objSize;
        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
            if (!freeObject->IsFreeObject()) {
                auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                visitor(obj);
                objSize = obj->GetClass()->SizeFromJSHClass(obj);
            } else {
                objSize = freeObject->Available();
            }
            curPtr += objSize;
            CHECK_OBJECT_SIZE(objSize);
        }
        CHECK_REGION_END(curPtr, endPtr);
    });
}

size_t SemiSpace::GetAllocatedSizeSinceGC() const
{
    auto currentRegion = GetCurrentRegion();
    size_t currentRegionSize = currentRegion->GetAllocatedBytes();
    if (currentRegion->HasAgeMark()) {
        currentRegionSize -= currentRegion->GetAllocatedBytes(ageMark_);
    }
    return allocateAfterLastGC_ + currentRegionSize;
}

OldSpace::OldSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::OLD_SPACE, initialCapacity, maximumCapacity)
{
}

bool OldSpace::Expand()
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Expand::Committed size " << committedSize_ << " of old space is too big. ";
        return false;
    }
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    region->InitializeSet();
    AddRegion(region);
    return true;
}

bool OldSpace::AddRegionToList(Region *region)
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Expand::Committed size " << committedSize_ << " of old space is too big. ";
        return false;
    }
    region->SetSpace(this);
    AddRegion(region);
    return true;
}

bool OldSpace::ContainObject(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            return true;
        }
        region = region->GetNext();
    }
    return false;
}

bool OldSpace::IsLive(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            if (region->InCollectSet()) {
                return false;
            }
            auto freeObject = FreeObject::Cast(ToUintPtr(object));
            return !freeObject->IsFreeObject();
        }
        region = region->GetNext();
    }
    return false;
}

void OldSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    EnumerateRegions([&](Region *region) {
        if (region->InCollectSet()) {
            return;
        }
        uintptr_t curPtr = region->GetBegin();
        uintptr_t endPtr = region->GetEnd();
        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
            size_t objSize;
            if (!freeObject->IsFreeObject()) {
                auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                visitor(obj);
                objSize = obj->GetClass()->SizeFromJSHClass(obj);
            } else {
                objSize = freeObject->Available();
            }
            curPtr += objSize;
            CHECK_OBJECT_SIZE(objSize);
        }
        CHECK_REGION_END(curPtr, endPtr);
    });
}

size_t OldSpace::GetHeapObjectSize() const
{
    return liveObjectSize_;
}

void OldSpace::Merge(Space *localSpace, FreeListAllocator *localAllocator)
{
    ASSERT(GetSpaceType() == localSpace->GetSpaceType());
    auto &allocator = heap_->GetHeapManager()->GetOldSpaceAllocator();
    localSpace->EnumerateRegions([&](Region *region) {
        localAllocator->DetachFreeObjectSet(region);
        localSpace->RemoveRegion(region);
        localSpace->DecrementCommitted(region->GetCapacity());
        region->SetSpace(this);
        AddRegion(region);
        allocator.CollectFreeObjectSet(region);
    });
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Merge::Committed size " << committedSize_ << " of old space is too big. ";
    }

    localSpace->ResetLiveObjectSize();
    localSpace->GetRegionList().Clear();
    allocator.IncrementPromotedSize(localAllocator->GetAllocatedSize());
}

void OldSpace::SelectCSet()
{
    // 1、Select region which alive object largger than 80%
    EnumerateRegions([this](Region *region) {
        if (!region->MostObjectAlive()) {
            collectRegionSet_.emplace_back(region);
        }
    });
    if (collectRegionSet_.size() < PARTIAL_GC_MIN_COLLECT_REGION_SIZE) {
        LOG_ECMA_MEM(DEBUG) << "Select CSet failure: number is too few";
        isCSetEmpty_ = true;
        collectRegionSet_.clear();
        return;
    }
    // sort
    std::sort(collectRegionSet_.begin(), collectRegionSet_.end(), [](Region *first, Region *second) {
        return first->AliveObject() < second->AliveObject();
    });
    unsigned long selectedRegionNumber = GetSelectedRegionNumber();
    if (collectRegionSet_.size() > selectedRegionNumber) {
        collectRegionSet_.resize(selectedRegionNumber);
    }

    auto &allocator = heap_->GetHeapManager()->GetOldSpaceAllocator();
    EnumerateCollectRegionSet([&](Region *current) {
        RemoveRegion(current);
        allocator.DetachFreeObjectSet(current);
        current->SetFlag(RegionFlags::IS_IN_COLLECT_SET);
    });
    isCSetEmpty_ = false;
    LOG_ECMA_MEM(DEBUG) << "Select CSet success: number is " << collectRegionSet_.size();
}

void OldSpace::RevertCSet()
{
    EnumerateCollectRegionSet([&](Region *region) {
        region->ClearFlag(RegionFlags::IS_IN_COLLECT_SET);
        region->SetSpace(this);
        AddRegion(region);
        heap_->GetHeapManager()->GetOldSpaceAllocator().CollectFreeObjectSet(region);
    });
    collectRegionSet_.clear();
    isCSetEmpty_ = true;
}

void OldSpace::ReclaimCSet()
{
    EnumerateCollectRegionSet([this](Region *region) {
        region->DeleteMarkBitmap();
        region->DeleteCrossRegionRememberedSet();
        region->DeleteOldToNewRememberedSet();
        region->DestroySet();
        regionFactory_->FreeRegion(region);
    });
    collectRegionSet_.clear();
    isCSetEmpty_ = true;
}

NonMovableSpace::NonMovableSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::NON_MOVABLE, initialCapacity, maximumCapacity)
{
}

bool NonMovableSpace::Expand()
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << committedSize_ << " of non movable space is too big. ";
        return false;
    }
    MEM_ALLOCATE_AND_GC_TRACE(vm_, NonMovableSpaceExpand);
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(IS_IN_NON_MOVABLE_GENERATION);
    region->InitializeSet();
    AddRegion(region);
    return true;
}

size_t NonMovableSpace::GetHeapObjectSize() const
{
    return liveObjectSize_;
}

SnapShotSpace::SnapShotSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::SNAPSHOT_SPACE, initialCapacity, maximumCapacity)
{
}

bool SnapShotSpace::Expand(uintptr_t top)
{
    if (committedSize_ >= maximumCapacity_) {
        return false;
    }
    Region *current = GetCurrentRegion();
    if (current != nullptr) {
        current->SetHighWaterMark(top);
    }
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_SNAPSHOT_SPACE_SIZE);
    region->SetFlag(RegionFlags::IS_IN_SNAPSHOT_GENERATION);
    AddRegion(region);
    return true;
}

void Space::Destroy()
{
    ReclaimRegions();
}

void NonMovableSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    EnumerateRegions([&](Region *region) {
        uintptr_t curPtr = region->GetBegin();
        uintptr_t endPtr = region->GetEnd();
        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
            size_t objSize;
            if (!freeObject->IsFreeObject()) {
                auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                visitor(obj);
                objSize = obj->GetClass()->SizeFromJSHClass(obj);
            } else {
                objSize = freeObject->Available();
            }
            curPtr += objSize;
            CHECK_OBJECT_SIZE(objSize);
        }
        CHECK_REGION_END(curPtr, endPtr);
    });
}

bool NonMovableSpace::ContainObject(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            return true;
        }
        region = region->GetNext();
    }
    return false;
}

bool NonMovableSpace::IsLive(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            auto freeObject = FreeObject::Cast(ToUintPtr(object));
            return !freeObject->IsFreeObject();
        }
        region = region->GetNext();
    }
    return false;
}

HugeObjectSpace::HugeObjectSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::HUGE_OBJECT_SPACE, initialCapacity, maximumCapacity)
{
}

uintptr_t HugeObjectSpace::Allocate(size_t objectSize)
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << committedSize_ << " of huge object space is too big. "
                            << " old space committed: " << heap_->GetOldSpace()->GetCommittedSize()
                            << " old space limit: " << heap_->GetOldSpaceAllocLimit()
                            << " length: " << GetRegionList().GetLength();
        return 0;
    }
    size_t alignedSize = AlignUp(objectSize + sizeof(Region), PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (UNLIKELY(alignedSize > MAX_HUGE_OBJECT_SIZE)) {
        LOG_ECMA_MEM(FATAL) << "The size is too big for this allocator. Return nullptr.";
        return 0;
    }
    MEM_ALLOCATE_AND_GC_TRACE(vm_, HugeSpaceExpand);
    Region *region = regionFactory_->AllocateAlignedRegion(this, alignedSize);
    region->SetFlag(RegionFlags::IS_HUGE_OBJECT);
    AddRegion(region);
    return region->GetBegin();
}

void HugeObjectSpace::Free(Region *region)
{
    GetRegionList().RemoveNode(region);
    ClearAndFreeRegion(region);
}

bool HugeObjectSpace::ContainObject(TaggedObject *object) const
{
    auto region = GetRegionList().GetFirst();
    while (region != nullptr) {
        if (region->InRange(ToUintPtr(object))) {
            return true;
        }
        region = region->GetNext();
    }
    return false;
}

bool HugeObjectSpace::IsLive(TaggedObject *object) const
{
    return ContainObject(object);
}

size_t HugeObjectSpace::GetHeapObjectSize() const
{
    return committedSize_;
}

void HugeObjectSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const
{
    EnumerateRegions([&](Region *region) {
        uintptr_t curPtr = region->GetBegin();
        objectVisitor(reinterpret_cast<TaggedObject *>(curPtr));
    });
}

MachineCodeSpace::MachineCodeSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::MACHINE_CODE_SPACE, initialCapacity, maximumCapacity)
{
}

bool MachineCodeSpace::Expand()
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << committedSize_ << " of machine Code space is too big. ";
        return false;
    }
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(IS_IN_NON_MOVABLE_GENERATION);
    region->InitializeSet();
    AddRegion(region);
    int res = region->SetCodeExecutableAndReadable();
    LOG_ECMA_MEM(DEBUG) << "MachineCodeSpace::Expand() SetCodeExecutableAndReadable" << res;
    return true;
}

size_t MachineCodeSpace::GetHeapObjectSize() const
{
    return liveObjectSize_;
}
}  // namespace panda::ecmascript
