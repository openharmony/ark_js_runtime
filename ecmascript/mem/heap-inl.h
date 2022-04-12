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
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/barriers-inl.h"

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
void Heap::EnumerateSnapShotSpaceRegions(const Callback &cb) const
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
void Heap::EnumerateNewSpaceRegions(const Callback &cb) const
{
    toSpace_->EnumerateRegions(cb);
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
    toSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateCollectRegionSet(cb);
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::IteratorOverObjects(const Callback &cb) const
{
    toSpace_->IterateOverObjects(cb);
    oldSpace_->IterateOverObjects(cb);
    nonMovableSpace_->IterateOverObjects(cb);
    hugeObjectSpace_->IterateOverObjects(cb);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateYoungOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }

    auto object = reinterpret_cast<TaggedObject *>(toSpace_->Allocate(size));
    if (object == nullptr) {
        CollectGarbage(SelectGCType());
        object = reinterpret_cast<TaggedObject *>(toSpace_->Allocate(size));
        if (object == nullptr) {
            CollectGarbage(SelectGCType());
            object = reinterpret_cast<TaggedObject *>(toSpace_->Allocate(size));
            if  (UNLIKELY(object == nullptr)) {
                ThrowOutOfMemoryError(size, "AllocateYoungObject");
                UNREACHABLE();
            }
        }
    }

    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

uintptr_t Heap::AllocateYoungSync(size_t size)
{
    return toSpace_->AllocateSync(size);
}

bool Heap::MoveYoungRegionSync(Region *region)
{
    return toSpace_->SwapRegion(region, fromSpace_);
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
    auto object = reinterpret_cast<TaggedObject *>(toSpace_->Allocate(size));
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

TaggedObject *Heap::AllocateHugeObject(JSHClass *hclass, size_t size)
{
    auto *object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        CollectGarbage(TriggerGCType::OLD_GC);
        object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size));
        if (UNLIKELY(object == nullptr)) {
            ThrowOutOfMemoryError(size, "Heap::AllocateHugeObject");
        }
    }
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

uintptr_t Heap::AllocateSnapShotSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t object = snapshotSpace_->Allocate(size);
    if (UNLIKELY(object == 0)) {
        LOG_ECMA_MEM(FATAL) << "alloc failed";
        UNREACHABLE();
    }
    return object;
}

void Heap::OnAllocateEvent(uintptr_t address)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    if (tracker_ != nullptr) {
        tracker_->AllocationEvent(address);
    }
#endif
}

void Heap::OnMoveEvent(uintptr_t address, uintptr_t forwardAddress)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    if (tracker_ != nullptr) {
        tracker_->MoveEvent(address, forwardAddress);
    }
#endif
}

void Heap::SwapNewSpace()
{
    toSpace_->Stop();
    fromSpace_->Restart();

    SemiSpace *newSpace = fromSpace_;
    fromSpace_ = toSpace_;
    toSpace_ = newSpace;
}

void Heap::ReclaimRegions(TriggerGCType gcType, Region *lastRegionOfToSpace)
{
    toSpace_->EnumerateRegions([] (Region *region) {
        region->ClearMarkBitmap();
        region->ClearCrossRegionRememberedSet();
        region->ResetAliveObject();
        region->ClearFlag(RegionFlags::IS_IN_NEW_TO_NEW_SET);
    }, lastRegionOfToSpace);
    if (gcType == TriggerGCType::FULL_GC) {
        compressSpace_->Reset();
    } else if (gcType == TriggerGCType::OLD_GC) {
        oldSpace_->ReclaimCSet();
    }
    fromSpace_->ReclaimRegions();

    if (!isClearTaskFinished_) {
        os::memory::LockHolder holder(waitClearTaskFinishedMutex_);
        isClearTaskFinished_ = true;
        waitClearTaskFinishedCV_.SignalAll();
    }
}

void Heap::ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd)
{
    auto set = current->GetOldToNewRememberedSet();
    if (set != nullptr) {
        set->ClearRange(freeStart, freeEnd);
    }
    set = current->GetCrossRegionRememberedSet();
    if (set != nullptr) {
        set->ClearRange(freeStart, freeEnd);
    }
}

size_t Heap::GetCommittedSize() const
{
    size_t result = toSpace_->GetCommittedSize() + oldSpace_->GetCommittedSize() + hugeObjectSpace_->GetCommittedSize()
                    + nonMovableSpace_->GetCommittedSize() + machineCodeSpace_->GetCommittedSize();
    return result;
}

size_t Heap::GetHeapObjectSize() const
{
    size_t result = toSpace_->GetHeapObjectSize() + oldSpace_->GetHeapObjectSize()
                    + hugeObjectSpace_->GetHeapObjectSize() + nonMovableSpace_->GetHeapObjectSize()
                    + machineCodeSpace_->GetCommittedSize();
    return result;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_INL_H
