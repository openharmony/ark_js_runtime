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

#include "ecmascript/mem/evacuation_allocator-inl.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/mem_manager.h"
#include "ecmascript/mem/free_object_kind.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/platform/platform.h"

namespace panda::ecmascript {
void EvacuationAllocator::Initialize(TriggerGCType type)
{
    // Reset new space allocator
    auto heapManager = heap_->GetHeapManager();
    heap_->GetNewSpace()->GetCurrentRegion()->SetHighWaterMark(heapManager->GetNewSpaceAllocator().GetTop());
    heap_->InitializeFromSpace();
    auto fromSpace = heap_->GetFromSpace();
    newSpaceAllocator_.Reset(fromSpace);
    heap_->FlipNewSpace();
    // Reset old space allocator
    if (type == TriggerGCType::OLD_GC) {
        oldSpaceAllocator_.Reset(heap_);
    } else if (type == TriggerGCType::COMPRESS_FULL_GC) {
        heap_->InitializeCompressSpace();
        auto compressSpace = const_cast<OldSpace *>(heap_->GetCompressSpace());
        auto currentRegion = compressSpace->GetCurrentRegion();
        currentRegion->SetAliveObject(currentRegion->GetSize());
        oldSpaceAllocator_.Reset(compressSpace);
    } else {
        oldSpaceAllocator_.Swap(heapManager->GetOldSpaceAllocator());
    }
}

void EvacuationAllocator::Finalize(TriggerGCType type)
{
    // Swap
    auto heapManager = heap_->GetHeapManager();
    if (type == TriggerGCType::OLD_GC) {
        auto oldSpace = const_cast<OldSpace *>(heap_->GetOldSpace());
        oldSpace->RemoveCSetFromList();
        oldSpace->Merge(const_cast<OldSpace *>(heap_->GetCompressSpace()));
        heapManager->GetOldSpaceAllocator().Merge(&oldSpaceAllocator_);
    } else {
        if (type == TriggerGCType::COMPRESS_FULL_GC) {
            heap_->FlipCompressSpace();
        }
        heapManager->GetOldSpaceAllocator().Swap(oldSpaceAllocator_);
    }
    heapManager->GetNewSpaceAllocator().Swap(newSpaceAllocator_);

    // Reclaim Region
    if (heap_->IsParallelGCEnabled()) {
        isFreeTaskFinish_ = false;
        Platform::GetCurrentPlatform()->PostTask(std::make_unique<AsyncFreeRegionTask>(this, type));
    } else {
        ReclaimRegions(type);
    }

    heap_->SetNewSpaceAgeMark(newSpaceAllocator_.GetTop());
}

bool EvacuationAllocator::AddRegionToOld(Region *region)
{
    if (!region->InYoungGeneration()) {
        os::memory::LockHolder lock(oldAllocatorLock_);
        const_cast<OldSpace *>(heap_->GetOldSpace())->RemoveRegionFromCSetAndList(region);
        return heap_->AddRegionToCompressSpace(region);
    }
    {
        os::memory::LockHolder lock(youngAllocatorLock_);
        const_cast<SemiSpace *>(heap_->GetFromSpace())->RemoveRegion(region);
    }
    os::memory::LockHolder lock(oldAllocatorLock_);
    return heap_->AddRegionToCompressSpace(region);
}

bool EvacuationAllocator::AddRegionToYoung(Region *region)
{
    ASSERT(region->InYoungGeneration());
    os::memory::LockHolder lock(youngAllocatorLock_);
    const_cast<SemiSpace *>(heap_->GetFromSpace())->RemoveRegion(region);
    return heap_->AddRegionToToSpace(region);
}

uintptr_t EvacuationAllocator::AllocateOld(size_t size)
{
    os::memory::LockHolder lock(oldAllocatorLock_);
    uintptr_t result = oldSpaceAllocator_.Allocate(size);
    if (UNLIKELY(result == 0)) {
        // Compress bugfix
        if (!heap_->FillOldSpaceAndTryGC(&oldSpaceAllocator_, false)) {
            return 0;
        }
        result = oldSpaceAllocator_.Allocate(size);
    }
    return result;
}

uintptr_t EvacuationAllocator::AllocateYoung(size_t size)
{
    os::memory::LockHolder lock(youngAllocatorLock_);
    uintptr_t result = newSpaceAllocator_.Allocate(size);
    if (UNLIKELY(result == 0)) {
        if (!heap_->FillNewSpaceAndTryGC(&newSpaceAllocator_, false)) {
            return 0;
        }
        result = newSpaceAllocator_.Allocate(size);
    }
    return result;
}

void EvacuationAllocator::ReclaimRegions(TriggerGCType gcType)
{
    if (gcType == TriggerGCType::COMPRESS_FULL_GC) {
        const_cast<OldSpace *>(heap_->GetCompressSpace())->ReclaimRegions();
    } else if (gcType == TriggerGCType::OLD_GC) {
        const_cast<OldSpace *>(heap_->GetOldSpace())->ReclaimRegionCSet();
    }
    const_cast<SemiSpace *>(heap_->GetFromSpace())->ReclaimRegions();
    if (!isFreeTaskFinish_) {
        os::memory::LockHolder holder(mutex_);
        isFreeTaskFinish_ = true;
        condition_.SignalAll();
    }
}

void EvacuationAllocator::WaitFreeTaskFinish()
{
    if (!isFreeTaskFinish_) {
        os::memory::LockHolder holder(mutex_);
        while (!isFreeTaskFinish_) {
            condition_.Wait(&mutex_);
        }
    }
}

bool EvacuationAllocator::AsyncFreeRegionTask::Run(uint32_t threadIndex)
{
    allocator_->ReclaimRegions(gcType_);
    return true;
}
}  // namespace panda::ecmascript
