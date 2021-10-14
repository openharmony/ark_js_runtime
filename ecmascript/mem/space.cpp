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

#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/region_factory.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/space-inl.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript {
void Space::AddRegion(Region *region)
{
    regionList_.AddNode(region);
    IncrementCommitted(region->GetCapacity());
}

void Space::Initialize()
{
    Region *region =
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    if (spaceType_ == MemSpaceType::SEMI_SPACE) {
        region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);
    } else if (spaceType_ == MemSpaceType::SNAPSHOT_SPACE) {
        region->SetFlag(RegionFlags::IS_IN_SNAPSHOT_GENERATION);
    } else if (spaceType_ == MemSpaceType::OLD_SPACE) {
        region->InitializeKind();
        region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    } else if (spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->InitializeKind();
        region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
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
    if (region->GetMarkBitmap() != nullptr) {
        auto bitmap = region->GetMarkBitmap();
        auto size = RangeBitmap::GetBitMapSizeInByte(region->GetCapacity());
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->Free(bitmap->GetBitMap().Data(), size);
        delete bitmap;
    }
    if (region->GetCrossRegionRememberedSet() != nullptr) {
        auto rememberedSet = region->GetCrossRegionRememberedSet();
        auto size = RememberedSet::GetSizeInByte(region->GetCapacity());
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->Free(rememberedSet->GetBitMap().Data(), size);
        delete rememberedSet;
    }
    if (region->GetOldToNewRememberedSet() != nullptr) {
        auto rememberedSet = region->GetOldToNewRememberedSet();
        auto size = RememberedSet::GetSizeInByte(region->GetCapacity());
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->Free(rememberedSet->GetBitMap().Data(), size);
        delete rememberedSet;
    }
    DecrementCommitted(region->GetCapacity());
    if (spaceType_ == MemSpaceType::OLD_SPACE ||
        spaceType_ == MemSpaceType::NON_MOVABLE ||
        spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->DestoryKind();
    }
    const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeRegion(region);
}

size_t Space::GetHeapObjectSize() const
{
    Region *last = GetCurrentRegion();
    auto top = GetHeap()->GetHeapManager()->GetNewSpaceAllocator().GetTop();
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
    Heap *heap = GetHeap();
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        return false;
    }
    GetCurrentRegion()->SetHighWaterMark(top);
    Region *region =
        const_cast<RegionFactory *>(heap->GetRegionFactory())->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(RegionFlags::IS_IN_YOUNG_GENERATION);

    AddRegion(region);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

void SemiSpace::Swap([[maybe_unused]] SemiSpace *other) {}

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
    return ContainObject(object);
}

void SemiSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    auto current = GetCurrentRegion();
    EnumerateRegions([&](Region *region) {
        auto curPtr = region->GetBegin();
        uintptr_t endPtr;
        if (region == current) {
            auto top = GetHeap()->GetHeapManager()->GetNewSpaceAllocator().GetTop();
            endPtr = curPtr + region->GetAllocatedBytes(top);
        } else {
            endPtr = curPtr + region->GetAllocatedBytes();
        }

        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
            size_t objSize;
            if (!freeObject->IsFreeObject()) {
                auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                visitor(obj);
                objSize = obj->GetObjectSize();
            } else {
                objSize = freeObject->Available();
            }
            LOG_IF(objSize == 0, FATAL, RUNTIME) << "SemiSpace IterateOverObjects objSize==0 invalid: " << curPtr;
            curPtr += AlignUp(objSize, sizeof(JSTaggedType));
        }
    });
}

OldSpace::OldSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::OLD_SPACE, initialCapacity, maximumCapacity)
{
}

bool OldSpace::Expand()
{
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << GetCommittedSize() << " of old space is too big. ";
        return false;
    }
    Region *region =
        const_cast<RegionFactory *>(GetHeap()->GetRegionFactory())->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(RegionFlags::IS_IN_OLD_GENERATION);
    region->InitializeKind();
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
        uintptr_t curPtr = region->GetBegin();
        uintptr_t endPtr = region->GetEnd();
        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
            size_t objSize;
            if (!freeObject->IsFreeObject()) {
                auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                visitor(obj);
                objSize = obj->GetObjectSize();
            } else {
                objSize = freeObject->Available();
            }
            LOG_IF(objSize == 0, FATAL, RUNTIME) << "OldSpace IterateOverObjects objSize==0 invalid: " << curPtr;
            curPtr += AlignUp(objSize, sizeof(JSTaggedType));
        }
    });
}

size_t OldSpace::GetHeapObjectSize() const
{
    size_t result;
    size_t availableSize = GetHeap()->GetHeapManager()->GetOldSpaceAllocator().GetAvailableSize();
    result = GetCommittedSize() - availableSize;
    result += GetHeap()->GetHugeObjectSpace()->GetHeapObjectSize();
    return result;
}

NonMovableSpace::NonMovableSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::NON_MOVABLE, initialCapacity, maximumCapacity)
{
}

bool NonMovableSpace::Expand()
{
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << GetCommittedSize() << " of non movable space is too big. ";
        return false;
    }
    Region *region =
        const_cast<RegionFactory *>(GetHeap()->GetRegionFactory())->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->SetFlag(IS_IN_NON_MOVABLE_GENERATION);
    region->InitializeKind();
    AddRegion(region);
    return true;
}

size_t NonMovableSpace::GetHeapObjectSize() const
{
    size_t result;
    size_t availableSize = GetHeap()->GetHeapManager()->GetNonMovableSpaceAllocator().GetAvailableSize();
    size_t regionSize = GetRegionList().GetLength() * DEFAULT_REGION_SIZE;
    result = regionSize - availableSize;
    return result;
}

SnapShotSpace::SnapShotSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, MemSpaceType::SNAPSHOT_SPACE, initialCapacity, maximumCapacity)
{
}

bool SnapShotSpace::Expand(uintptr_t top)
{
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        return false;
    }
    Region *current = GetCurrentRegion();
    if (current != nullptr) {
        current->SetHighWaterMark(top);
    }
    Region *region = const_cast<RegionFactory *>(GetHeap()->GetRegionFactory())->
                        AllocateAlignedRegion(this, DEFAULT_SNAPSHOT_SPACE_SIZE);
    region->SetFlag(RegionFlags::IS_IN_SNAPSHOT_GENERATION);
    AddRegion(region);
    return true;
}

RangeBitmap *Region::CreateMarkBitmap()
{
    size_t heapSize = IsFlagSet(RegionFlags::IS_HUGE_OBJECT) ? LARGE_BITMAP_MIN_SIZE : GetCapacity();
    // Only one huge object is stored in a region. The BitmapSize of a huge region will always be 8 Bytes.
    size_t bitmapSize = RangeBitmap::GetBitMapSizeInByte(heapSize);

    auto bitmapData = const_cast<RegionFactory *>(space_->GetHeap()->GetRegionFactory())->Allocate(bitmapSize);
    auto *ret = new RangeBitmap(this, heapSize, bitmapData);

    ret->ClearAllBits();
    return ret;
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
                objSize = obj->GetObjectSize();
            } else {
                objSize = freeObject->Available();
            }
            LOG_IF(objSize == 0, FATAL, RUNTIME) << "NonMovableSpace IterateOverObjects objSize==0 invalid: " << curPtr;
            curPtr += AlignUp(objSize, sizeof(JSTaggedType));
        }
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
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << GetCommittedSize() << " of huge object space is too big. "
                            << "length: " << GetRegionList().GetLength();
        return 0;
    }
    size_t alignedSize = AlignUp(objectSize + sizeof(Region), PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (UNLIKELY(alignedSize > MAX_HUGE_OBJECT_SIZE)) {
        LOG_ECMA_MEM(FATAL) << "The size is too big for this allocator. Return nullptr.";
        return 0;
    }
    Region *region =
        const_cast<RegionFactory *>(GetHeap()->GetRegionFactory())->AllocateAlignedRegion(this, alignedSize);
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
    return GetCommittedSize();
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
    if (GetCommittedSize() >= GetMaximumCapacity()) {
        LOG_ECMA_MEM(FATAL) << "Committed size " << GetCommittedSize() << " of machine Code space is too big. ";
        return false;
    }
    Region *region =
        const_cast<RegionFactory *>(GetHeap()->GetRegionFactory())->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE);
    region->InitializeKind();
    AddRegion(region);
    int res = region->SetCodeExecutableAndReadable();
    LOG_ECMA_MEM(DEBUG) << "MachineCodeSpace::Expand() SetCodeExecutableAndReadable" << res;
    return true;
}
}  // namespace panda::ecmascript
