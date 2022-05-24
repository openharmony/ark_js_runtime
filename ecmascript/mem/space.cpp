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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/region-inl.h"

namespace panda::ecmascript {
Space::Space(HeapRegionAllocator *heapRegionAllocator,
             MemSpaceType spaceType, size_t initialCapacity,
             size_t maximumCapacity)
    : heapRegionAllocator_(heapRegionAllocator),
      spaceType_(spaceType),
      initialCapacity_(initialCapacity),
      maximumCapacity_(maximumCapacity),
      committedSize_(0)
{
}

void Space::Destroy()
{
    ReclaimRegions();
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
    region->SetFlag(RegionFlags::INVALID);
    region->DeleteCrossRegionRSet();
    region->DeleteOldToNewRSet();
    region->DeleteSweepingRSet();
    DecreaseCommitted(region->GetCapacity());
    DecreaseObjectSize(region->GetSize());
    if (spaceType_ == MemSpaceType::OLD_SPACE || spaceType_ == MemSpaceType::NON_MOVABLE ||
        spaceType_ == MemSpaceType::MACHINE_CODE_SPACE) {
        region->DestroySet();
    }
    heapRegionAllocator_->FreeRegion(region);
}

HugeObjectSpace::HugeObjectSpace(HeapRegionAllocator *heapRegionAllocator,
                                 size_t initialCapacity, size_t maximumCapacity)
    : Space(heapRegionAllocator,
            MemSpaceType::HUGE_OBJECT_SPACE,
            initialCapacity,
            maximumCapacity)
{
}

uintptr_t HugeObjectSpace::Allocate(size_t objectSize, JSThread *thread)
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(INFO) << "Committed size " << committedSize_ << " of huge object space is too big.";
        return 0;
    }

    size_t alignedSize = AlignUp(objectSize + sizeof(Region), PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (UNLIKELY(alignedSize > MAX_HUGE_OBJECT_SIZE)) {
        LOG_ECMA_MEM(FATAL) << "The size is too big for this allocator. Return nullptr.";
        return 0;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, alignedSize, thread);
    AddRegion(region);
    return region->GetBegin();
}

void HugeObjectSpace::Sweep(bool isConcurrentSweep)
{
    Region *currentRegion = GetRegionList().GetFirst();
    while (currentRegion != nullptr) {
        if (isConcurrentSweep) {
            currentRegion->SwapRSetForConcurrentSweeping();
        }
        Region *next = currentRegion->GetNext();
        bool isMarked = false;
        currentRegion->IterateAllMarkedBits([&isMarked]([[maybe_unused]] void *mem) { isMarked = true; });
        if (!isMarked) {
            GetRegionList().RemoveNode(currentRegion);
            hugeNeedFreeList_.AddNode(currentRegion);
        }
        currentRegion = next;
    }
}

void HugeObjectSpace::FinishConcurrentSweep()
{
    Region *currentRegion = GetRegionList().GetFirst();
    while (currentRegion != nullptr) {
        currentRegion->MergeRSetForConcurrentSweeping();
        currentRegion = currentRegion->GetNext();
    }
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

void HugeObjectSpace::RecliamHugeRegion()
{
    if (hugeNeedFreeList_.IsEmpty()) {
        return;
    }
    do {
        Region *last = hugeNeedFreeList_.PopBack();
        ClearAndFreeRegion(last);
    } while (!hugeNeedFreeList_.IsEmpty());
}
}  // namespace panda::ecmascript
