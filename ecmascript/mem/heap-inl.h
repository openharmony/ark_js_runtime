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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/hprof/heap_tracker.h"
#include "ecmascript/mem/remembered_set.h"

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

bool Heap::FillNewSpaceAndTryGC(BumpPointerAllocator *spaceAllocator, bool allowGc)
{
    if (toSpace_->Expand(spaceAllocator->GetTop())) {
        spaceAllocator->Reset(toSpace_);
        TryTriggerConcurrentMarking(allowGc);
        return true;
    }
    if (allowGc) {
        CollectGarbage(TriggerGCType::SEMI_GC);
        return true;
    }
    return false;
}

bool Heap::FillOldSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc)
{
    if (oldSpace_->Expand()) {
        spaceAllocator->AddFree(oldSpace_->GetCurrentRegion());
        return true;
    }
    if (allowGc) {
        CollectGarbage(TriggerGCType::OLD_GC);
        return true;
    }
    return false;
}

bool Heap::FillNonMovableSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc)
{
    if (nonMovableSpace_->Expand()) {
        spaceAllocator->AddFree(nonMovableSpace_->GetCurrentRegion());
        return true;
    }
    if (allowGc) {
        CollectGarbage(TriggerGCType::NON_MOVE_GC);
        return true;
    }
    return false;
}

bool Heap::FillSnapShotSpace(BumpPointerAllocator *spaceAllocator)
{
    bool result = snapshotSpace_->Expand(spaceAllocator->GetTop());
    if (result) {
        spaceAllocator->Reset(snapshotSpace_);
    }
    return result;
}

bool Heap::FillMachineCodeSpaceAndTryGC(FreeListAllocator *spaceAllocator, bool allowGc)
{
    if (machineCodeSpace_->Expand()) {
        spaceAllocator->AddFree(machineCodeSpace_->GetCurrentRegion());
        return true;
    }
    if (allowGc) {
        CollectGarbage(TriggerGCType::MACHINE_CODE_GC);
        return true;
    }
    return false;
}

Region *Heap::ExpandCompressSpace()
{
    if (compressSpace_->Expand()) {
        return compressSpace_->GetCurrentRegion();
    }
    return nullptr;
}

bool Heap::AddRegionToCompressSpace(Region *region)
{
    return compressSpace_->AddRegionToList(region);
}

bool Heap::AddRegionToToSpace(Region *region)
{
    return toSpace_->AddRegionToList(region);
}

void Heap::OnAllocateEvent(uintptr_t address)
{
    if (tracker_ != nullptr) {
        tracker_->AllocationEvent(address);
    }
}

void Heap::OnMoveEvent(uintptr_t address, uintptr_t forwardAddress)
{
    if (tracker_ != nullptr) {
        tracker_->MoveEvent(address, forwardAddress);
    }
}

void Heap::SetNewSpaceAgeMark(uintptr_t mark)
{
    ASSERT(toSpace_ != nullptr);
    toSpace_->SetAgeMark(mark);
}

void Heap::SetNewSpaceMaximumCapacity(size_t maximumCapacity)
{
    ASSERT(toSpace_ != nullptr);
    SetMaximumCapacity(toSpace_, maximumCapacity);
}

void Heap::InitializeFromSpace()
{
    if (fromSpace_->GetCommittedSize() == 0) {
        fromSpace_->Initialize();
    }
}

void Heap::InitializeCompressSpace()
{
    if (compressSpace_->GetCommittedSize() == 0) {
        compressSpace_->Initialize();
    }
}

void Heap::SwapSpace()
{
    ASSERT(toSpace_ != nullptr);
    ASSERT(fromSpace_ != nullptr);
    toSpace_->Swap(fromSpace_);
}

void Heap::ReclaimFromSpaceRegions()
{
    ASSERT(fromSpace_ != nullptr);
    fromSpace_->ReclaimRegions();
}

void Heap::SetFromSpaceMaximumCapacity(size_t maximumCapacity)
{
    ASSERT(fromSpace_ != nullptr);
    SetMaximumCapacity(fromSpace_, maximumCapacity);
}

void Heap::ResetAppStartup()
{
    ASSERT(memController_ != nullptr);
    memController_->ResetAppStartup();
}

void Heap::SetMaximumCapacity(SemiSpace *space, size_t maximumCapacity)
{
    space->SetMaximumCapacity(maximumCapacity);
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
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_INL_H
