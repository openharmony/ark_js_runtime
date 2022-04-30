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

#ifndef ECMASCRIPT_MEM_ALLOCATOR_INL_H
#define ECMASCRIPT_MEM_ALLOCATOR_INL_H

#include <cstdlib>

#include "ecmascript/free_object.h"
#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
BumpPointerAllocator::BumpPointerAllocator(uintptr_t begin, uintptr_t end) : begin_(begin), top_(begin), end_(end) {}

void BumpPointerAllocator::Reset()
{
    begin_ = 0;
    top_ = 0;
    end_ = 0;
}

void BumpPointerAllocator::Reset(uintptr_t begin, uintptr_t end)
{
    begin_ = begin;
    top_ = begin;
    end_ = end;
}

void BumpPointerAllocator::Reset(uintptr_t begin, uintptr_t end, uintptr_t top)
{
    begin_ = begin;
    top_ = top;
    end_ = end;
}

uintptr_t BumpPointerAllocator::Allocate(size_t size)
{
    ASSERT(size != 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(top_ + size > end_)) {
        return 0;
    }
    uintptr_t result = top_;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    top_ += size;
    return result;
}

FreeListAllocator::FreeListAllocator(Heap *heap) : heap_(heap)
{
    freeList_ = std::make_unique<FreeObjectList>();
}

void FreeListAllocator::Initialize(Region *region)
{
    bpAllocator_.Reset(region->GetBegin(), region->GetEnd());
}

void FreeListAllocator::Reset(Heap *heap)
{
    heap_ = heap;
    freeList_ = std::make_unique<FreeObjectList>();
    FreeBumpPoint();
}

void FreeListAllocator::AddFree(Region *region)
{
    auto begin = region->GetBegin();
    auto end = region->GetEnd();
    FreeBumpPoint();
    bpAllocator_.Reset(begin, end);
}

uintptr_t FreeListAllocator::Allocate(size_t size)
{
    auto ret = bpAllocator_.Allocate(size);
    if (LIKELY(ret != 0)) {
        allocationSizeAccumulator_ += size;
        Region::ObjectAddressToRange(ret)->IncreaseAliveObject(size);
        return ret;
    }
    FreeObject *object = freeList_->Allocate(size);
    if (object != nullptr) {
        ret = Allocate(object, size);
    }
    return ret;
}

uintptr_t FreeListAllocator::Allocate(FreeObject *object, size_t size)
{
    uintptr_t begin = object->GetBegin();
    uintptr_t end = object->GetEnd();
    uintptr_t remainSize = end - begin - size;
    ASSERT(remainSize >= 0);
    // Keep a longest freeObject between bump-pointer and free object that just allocated
    allocationSizeAccumulator_ += size;
    if (remainSize <= bpAllocator_.Available()) {
        Free(begin + size, remainSize);
        Region::ObjectAddressToRange(begin)->IncreaseAliveObject(size);
        return begin;
    } else {
        FreeBumpPoint();
        bpAllocator_.Reset(begin, end);
        auto ret = bpAllocator_.Allocate(size);
        if (ret != 0) {
            Region::ObjectAddressToRange(ret)->IncreaseAliveObject(size);
        }
        return ret;
    }
}

void FreeListAllocator::FreeBumpPoint()
{
    auto begin = bpAllocator_.GetTop();
    auto size = bpAllocator_.Available();
    bpAllocator_.Reset();
    Free(begin, size);
}

void FreeListAllocator::FillBumpPoint()
{
    size_t size = bpAllocator_.Available();
    if (size != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), bpAllocator_.GetTop(), size);
    }
}

void FreeListAllocator::Free(uintptr_t begin, size_t size, bool isAdd)
{
    ASSERT(heap_ != nullptr);
    ASSERT(size >= 0);
    if (size != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), begin, size);
        freeList_->Free(begin, size, isAdd);
    }
}

uintptr_t FreeListAllocator::LookupSuitableFreeObject(size_t size)
{
    auto freeObject = freeList_->LookupSuitableFreeObject(size);
    if (freeObject != nullptr) {
        return freeObject->GetBegin();
    }
    return 0;
}

void FreeListAllocator::RebuildFreeList()
{
    bpAllocator_.Reset();
    freeList_->Rebuild();
}

inline void FreeListAllocator::CollectFreeObjectSet(Region *region)
{
    region->EnumerateSets([&](FreeObjectSet *set) {
        if (set == nullptr || set->Empty()) {
            return;
        }
        freeList_->AddSet(set);
    });
    freeList_->IncreaseWastedSize(region->GetWastedSize());
}

inline void FreeListAllocator::DetachFreeObjectSet(Region *region)
{
    region->EnumerateSets([&](FreeObjectSet *set) {
        if (set == nullptr || set->Empty()) {
            return;
        }
        freeList_->RemoveSet(set);
    });
    freeList_->DecreaseWastedSize(region->GetWastedSize());
}

size_t FreeListAllocator::GetAvailableSize() const
{
    return freeList_->GetFreeObjectSize() + bpAllocator_.Available();
}

size_t FreeListAllocator::GetWastedSize() const
{
    return freeList_->GetWastedSize();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_ALLOCATOR_INL_H
