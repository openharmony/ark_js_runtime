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

#ifndef ECMASCRIPT_MEM_HEAP_INL_H
#define ECMASCRIPT_MEM_HEAP_INL_H

#include "ecmascript/mem/heap.h"

#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/mem_map_allocator.h"

namespace panda::ecmascript {
template<class Callback>
void Heap::EnumerateOldSpaceRegions(const Callback &cb, Region *region) const
{
    oldSpace_->EnumerateRegions(cb, region);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateSnapshotSpaceRegions(const Callback &cb) const
{
    snapshotSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonNewSpaceRegions(const Callback &cb) const
{
    oldSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateCollectRegionSet(cb);
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonNewSpaceRegionsWithRecord(const Callback &cb) const
{
    oldSpace_->EnumerateRegionsWithRecord(cb);
    snapshotSpace_->EnumerateRegionsWithRecord(cb);
    nonMovableSpace_->EnumerateRegionsWithRecord(cb);
    hugeObjectSpace_->EnumerateRegionsWithRecord(cb);
    machineCodeSpace_->EnumerateRegionsWithRecord(cb);
}

template<class Callback>
void Heap::EnumerateNewSpaceRegions(const Callback &cb) const
{
    activeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonMovableRegions(const Callback &cb) const
{
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateRegions(const Callback &cb) const
{
    activeSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateCollectRegionSet(cb);
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::IterateOverObjects(const Callback &cb) const
{
    activeSpace_->IterateOverObjects(cb);
    oldSpace_->IterateOverObjects(cb);
    nonMovableSpace_->IterateOverObjects(cb);
    hugeObjectSpace_->IterateOverObjects(cb);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateYoungOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(size);
    }

    auto object = reinterpret_cast<TaggedObject *>(activeSpace_->Allocate(size));
    if (object == nullptr) {
        CollectGarbage(SelectGCType());
        object = reinterpret_cast<TaggedObject *>(activeSpace_->Allocate(size));
        if (object == nullptr) {
            CollectGarbage(SelectGCType());
            object = reinterpret_cast<TaggedObject *>(activeSpace_->Allocate(size));
            if  (UNLIKELY(object == nullptr)) {
                ThrowOutOfMemoryError(size, "AllocateYoungObject");
                UNREACHABLE();
            }
        }
    }
    return object;
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass, size_t size)
{
    auto object = AllocateYoungOrHugeObject(size);
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

uintptr_t Heap::AllocateYoungSync(size_t size)
{
    return activeSpace_->AllocateSync(size);
}

bool Heap::MoveYoungRegionSync(Region *region)
{
    return activeSpace_->SwapRegion(region, inactiveSpace_);
}

void Heap::MergeToOldSpaceSync(LocalSpace *localSpace)
{
    oldSpace_->Merge(localSpace);
}

TaggedObject *Heap::TryAllocateYoungGeneration(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    auto object = reinterpret_cast<TaggedObject *>(activeSpace_->Allocate(size));
    if (object != nullptr) {
        object->SetClass(hclass);
    }
    return object;
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateOldOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(oldSpace_->Allocate(size));
    if (UNLIKELY(object == 0)) {
        ThrowOutOfMemoryError(size, "AllocateOldGenerationOrHugeObject");
        UNREACHABLE();
    }
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *Heap::AllocateNonMovableOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateNonMovableOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        ThrowOutOfMemoryError(size, "AllocateNonMovableOrHugeObject");
        UNREACHABLE();
    }
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *Heap::AllocateDynClassClass(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    auto object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        LOG_ECMA_MEM(FATAL) << "Heap::AllocateDynClassClass can not allocate any space";
    }
    *reinterpret_cast<MarkWordType *>(ToUintPtr(object)) = reinterpret_cast<MarkWordType>(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *Heap::AllocateHugeObject(size_t size)
{
    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    CheckAndTriggerOldGC();

    auto *object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
    if (UNLIKELY(object == nullptr)) {
        CollectGarbage(TriggerGCType::OLD_GC);
        object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
        if (UNLIKELY(object == nullptr)) {
            ThrowOutOfMemoryError(size, "Heap::AllocateHugeObject");
        }
    }
    return object;
}

TaggedObject *Heap::AllocateHugeObject(JSHClass *hclass, size_t size)
{
    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    CheckAndTriggerOldGC();
    auto object = AllocateHugeObject(size);
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *Heap::AllocateMachineCodeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    auto object = reinterpret_cast<TaggedObject *>(machineCodeSpace_->Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        ThrowOutOfMemoryError(size, "Heap::AllocateMachineCodeObject");
        return nullptr;
    }
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

uintptr_t Heap::AllocateSnapshotSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t object = snapshotSpace_->Allocate(size);
    if (UNLIKELY(object == 0)) {
        LOG_ECMA_MEM(FATAL) << "alloc failed";
        UNREACHABLE();
    }
    return object;
}

void Heap::OnAllocateEvent([[maybe_unused]] uintptr_t address)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    if (tracker_ != nullptr) {
        tracker_->AllocationEvent(address);
    }
#endif
}

void Heap::OnMoveEvent([[maybe_unused]] uintptr_t address, [[maybe_unused]] uintptr_t forwardAddress)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    if (tracker_ != nullptr) {
        tracker_->MoveEvent(address, forwardAddress);
    }
#endif
}

void Heap::SwapNewSpace()
{
    activeSpace_->Stop();
    inactiveSpace_->Restart();

    SemiSpace *newSpace = inactiveSpace_;
    inactiveSpace_ = activeSpace_;
    activeSpace_ = newSpace;
    auto topAddress = activeSpace_->GetAllocationTopAddress();
    auto endAddress = activeSpace_->GetAllocationEndAddress();
    thread_->ReSetNewSpaceAllocationAddress(topAddress, endAddress);
}

void Heap::ReclaimRegions(TriggerGCType gcType)
{
    activeSpace_->EnumerateRegionsWithRecord([] (Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
        region->ResetAliveObject();
        region->ClearFlag(RegionFlags::IS_IN_NEW_TO_NEW_SET);
    });
    if (gcType == TriggerGCType::FULL_GC) {
        compressSpace_->Reset();
    } else if (gcType == TriggerGCType::OLD_GC) {
        oldSpace_->ReclaimCSet();
    }
    inactiveSpace_->ReclaimRegions();

    sweeper_->WaitAllTaskFinished();
    EnumerateNonNewSpaceRegionsWithRecord([] (Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
    });
    if (!clearTaskFinished_) {
        os::memory::LockHolder holder(waitClearTaskFinishedMutex_);
        clearTaskFinished_ = true;
        waitClearTaskFinishedCV_.SignalAll();
    }
}

// only call in js-thread
void Heap::ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd)
{
    current->AtomicClearSweepingRSetInRange(freeStart, freeEnd);
    current->ClearOldToNewRSetInRange(freeStart, freeEnd);
    current->AtomicClearCrossRegionRSetInRange(freeStart, freeEnd);
}

size_t Heap::GetCommittedSize() const
{
    size_t result = activeSpace_->GetCommittedSize()
                    + oldSpace_->GetCommittedSize()
                    + hugeObjectSpace_->GetCommittedSize()
                    + nonMovableSpace_->GetCommittedSize()
                    + machineCodeSpace_->GetCommittedSize()
                    + snapshotSpace_->GetCommittedSize();
    return result;
}

size_t Heap::GetHeapObjectSize() const
{
    size_t result = activeSpace_->GetHeapObjectSize()
                    + oldSpace_->GetHeapObjectSize()
                    + hugeObjectSpace_->GetHeapObjectSize()
                    + nonMovableSpace_->GetHeapObjectSize()
                    + machineCodeSpace_->GetCommittedSize()
                    + snapshotSpace_->GetHeapObjectSize();
    return result;
}

int32_t Heap::GetHeapObjectCount() const
{
    int32_t count = 0;
    this->IterateOverObjects([&count]([[maybe_unused]]TaggedObject *obj) {
        ++count;
    });
    return count;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_INL_H
