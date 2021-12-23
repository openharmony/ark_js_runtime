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
#include "libpandabase/utils/logger.h"

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

void Space::AddRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Add region:" << region << " to " << ToSpaceTypeName(spaceType_);
    regionList_.AddNode(region);
    IncrementCommitted(region->GetCapacity());
}

void Space::AddRegionToFirst(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Add region to first:" << region << " to " << ToSpaceTypeName(spaceType_);
    regionList_.AddNodeToFirst(region);
    IncrementCommitted(region->GetCapacity());
}

void Space::RemoveRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Remove region:" << region << " to " << ToSpaceTypeName(spaceType_);
    if (regionList_.HasNode(region)) {
        if (spaceType_ == MemSpaceType::OLD_SPACE ||
            spaceType_ == MemSpaceType::NON_MOVABLE ||
            spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
            region->RebuildKind();
        }
        regionList_.RemoveNode(region);
        DecrementCommitted(region->GetCapacity());
    }
}

void Space::Initialize()
{
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    if (spaceType_ == MemSpaceType::SEMI_SPACE) {
        region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);
    } else if (spaceType_ == MemSpaceType::SNAPSHOT_SPACE) {
        region->SetFlag(RegionFlags::IS_IN_SNAPSHOT_GENERATION);
    } else if (spaceType_ == MemSpaceType::OLD_SPACE) {
        region->InitializeKind();
        region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    } else if (spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->InitializeKind();
        region->SetFlag(RegionFlags::IS_IN_NON_MOVABLE_GENERATION);
        int res = region->SetCodeExecutableAndReadable();
        LOG_ECMA_MEM(DEBUG) << "Initialize SetCodeExecutableAndReadable" << res;
    } else if (spaceType_ == MemSpaceType::NON_MOVABLE) {
        region->InitializeKind();
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
    region->ClearMarkBitmap();
    region->ClearCrossRegionRememberedSet();
    region->ClearOldToNewRememberedSet();
    DecrementCommitted(region->GetCapacity());
    if (spaceType_ == MemSpaceType::OLD_SPACE || spaceType_ == MemSpaceType::NON_MOVABLE ||
        spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->DestroyKind();
    }
    regionFactory_->FreeRegion(region);
}

size_t Space::GetHeapObjectSize() const
{
    Region *last = GetCurrentRegion();
    auto top = heap_->GetHeapManager()->GetNewSpaceAllocator().GetTop();
    size_t result = last->GetAllocatedBytes(top);

    EnumerateRegions([&result, &last](Region *current) {
        if (current != last) {
            result += current->GetAllocatedBytes();
        }
    });

    return result;
}

SemiSpace::SemiSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::SEMI_SPACE, initialCapacity, maximumCapacity), ageMark_(0)
{
}

bool SemiSpace::Expand(uintptr_t top)
{
    if (committedSize_ >= maximumCapacity_) {
        return false;
    }
    GetCurrentRegion()->SetHighWaterMark(top);
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);
    if (!thread_->IsNotBeginMark()) {
        region->SetMarking(true);
    }

    AddRegion(region);
    return true;
}

bool SemiSpace::AddRegionToList(Region *region)
{
    ASSERT(region != nullptr);
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        return false;
    }
    region->ResetFlag();
    region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);
    region->SetFlag(RegionFlags::IS_IN_PROMOTE_SET);
    region->SetSpace(this);
    AddRegionToFirst(region);
    return true;
}

void SemiSpace::Swap([[maybe_unused]] SemiSpace *other) {}

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

OldSpace::OldSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::OLD_SPACE, initialCapacity, maximumCapacity)
{
}

bool OldSpace::Expand()
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << committedSize_ << " of old space is too big. ";
        return false;
    }
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    if (!thread_->IsNotBeginMark()) {
        region->SetMarking(true);
    }
    region->InitializeKind();
    AddRegion(region);
    return true;
}

bool OldSpace::AddRegionToList(Region *region)
{
    ASSERT(region != nullptr);
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << GetCommittedSize() << " of old space is too big. ";
        return false;
    }
    if (!region->InOldGeneration()) {
        region->InitializeKind();
    }
    region->ResetFlag();
    region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    region->SetFlag(RegionFlags::IS_IN_PROMOTE_SET);
    region->SetSpace(this);
    AddRegionToFirst(region);
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
    size_t result;
    size_t availableSize = heap_->GetHeapManager()->GetOldSpaceAllocator().GetAvailableSize();
    result = committedSize_ - availableSize;
    result += heap_->GetHugeObjectSpace()->GetHeapObjectSize();
    return result;
}

void OldSpace::Merge(Space *fromSpace)
{
    ASSERT(GetSpaceType() == fromSpace->GetSpaceType());
    fromSpace->EnumerateRegions([&](Region *region) {
        fromSpace->DecrementCommitted(region->GetCapacity());
        AddRegion(region);
    });
    fromSpace->GetRegionList().Clear();
}

void OldSpace::AddRegionToCSet(Region *region)
{
    region->SetFlag(RegionFlags::IS_IN_COLLECT_SET);
    collectRegionSet_.emplace_back(region);
}

void OldSpace::ClearRegionFromCSet()
{
    EnumerateCollectRegionSet([](Region *region) { region->ClearFlag(RegionFlags::IS_IN_COLLECT_SET); });
    collectRegionSet_.clear();
}

void OldSpace::RemoveRegionFromCSetAndList(Region *region)
{
    for (auto current = collectRegionSet_.begin(); current != collectRegionSet_.end(); current++) {
        if (*current == region) {
            region->ClearFlag(RegionFlags::IS_IN_COLLECT_SET);
            current = collectRegionSet_.erase(current);
            break;
        }
    }
    RemoveRegion(region);
}

void OldSpace::RemoveCSetFromList()
{
    EnumerateCollectRegionSet([this](Region *current) {
        GetRegionList().RemoveNode(current);
    });
}

void OldSpace::ReclaimRegionCSet()
{
    EnumerateCollectRegionSet([this](Region *current) {
        ClearAndFreeRegion(current);
    });
    collectRegionSet_.clear();
}

void OldSpace::SelectCSet()
{
    EnumerateRegions([this](Region *region) {
        if (!region->MostObjectAlive()) {
            collectRegionSet_.emplace_back(region);
        }
    });
    if (collectRegionSet_.size() < PARTIAL_GC_MIN_COLLECT_REGION_SIZE) {
        heap_->SetOnlyMarkSemi(true);
        collectRegionSet_.clear();
        return;
    }
    // sort
    std::sort(collectRegionSet_.begin(), collectRegionSet_.end(), [](Region *first, Region *second) {
        return first->AliveObject() < second->AliveObject();
    });
    if (collectRegionSet_.size() > PARTIAL_GC_MAX_COLLECT_REGION_SIZE) {
        collectRegionSet_.resize(PARTIAL_GC_MAX_COLLECT_REGION_SIZE);
    }
    for (Region *region : collectRegionSet_) {
        region->SetFlag(RegionFlags::IS_IN_COLLECT_SET);
    }
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
    Region *region = regionFactory_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(IS_IN_NON_MOVABLE_GENERATION);
    if (!thread_->IsNotBeginMark()) {
        region->SetMarking(true);
    }
    region->InitializeKind();
    AddRegion(region);
    return true;
}

size_t NonMovableSpace::GetHeapObjectSize() const
{
    size_t result;
    size_t availableSize = heap_->GetHeapManager()->GetNonMovableSpaceAllocator().GetAvailableSize();
    result = GetCommittedSize() - availableSize;
    return result;
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
    if (!thread_->IsNotBeginMark()) {
        region->SetMarking(true);
    }
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
                            << "length: " << GetRegionList().GetLength();
        return 0;
    }
    size_t alignedSize = AlignUp(objectSize + sizeof(Region), PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (UNLIKELY(alignedSize > MAX_HUGE_OBJECT_SIZE)) {
        LOG_ECMA_MEM(FATAL) << "The size is too big for this allocator. Return nullptr.";
        return 0;
    }
    Region *region = regionFactory_->AllocateAlignedRegion(this, alignedSize);
    region->SetFlag(RegionFlags::IS_HUGE_OBJECT);
    if (!thread_->IsNotBeginMark()) {
        region->SetMarking(true);
    }
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
    region->InitializeKind();
    AddRegion(region);
    if (!thread_->IsNotBeginMark()) {
        region->SetMarking(true);
    }
    int res = region->SetCodeExecutableAndReadable();
    LOG_ECMA_MEM(DEBUG) << "MachineCodeSpace::Expand() SetCodeExecutableAndReadable" << res;
    return true;
}
}  // namespace panda::ecmascript
