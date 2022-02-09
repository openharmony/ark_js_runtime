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
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"

namespace panda::ecmascript {
BumpPointerAllocator::BumpPointerAllocator(const Space *space)
    : BumpPointerAllocator(space->GetAllocateAreaBegin(), space->GetAllocateAreaEnd())
{
}

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

void BumpPointerAllocator::Reset(const Space *space)
{
    Reset(space->GetAllocateAreaBegin(), space->GetAllocateAreaEnd());
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

FreeListAllocator::FreeListAllocator(const Space *space) : heap_(space->GetHeap()), type_(space->GetSpaceType()),
    sweeping_(false)
{
    freeList_ = std::make_unique<FreeObjectList>();
    bpAllocator_.Reset(space);
    FreeObject::Cast(bpAllocator_.GetTop())->SetAvailable(bpAllocator_.Available());
    FreeObject::Cast(bpAllocator_.GetTop())->SetNext(nullptr);
}

void FreeListAllocator::Reset(Heap *heap)
{
    heap_ = heap;
    freeList_ = std::make_unique<FreeObjectList>();
    sweeping_ = false;
    FreeBumpPoint();
}

void FreeListAllocator::AddFree(Region *region)
{
    auto begin = region->GetBegin();
    auto end = region->GetEnd();
    FreeBumpPoint();
    bpAllocator_.Reset(begin, end);
    FreeObject::FillFreeObject(heap_->GetEcmaVM(), bpAllocator_.GetTop(), bpAllocator_.Available());
}

template<bool isLocal>
uintptr_t FreeListAllocator::Allocate(size_t size)
{
    auto ret = bpAllocator_.Allocate(size);
    if (LIKELY(ret != 0)) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), bpAllocator_.GetTop(), bpAllocator_.Available());
        allocationSizeAccumulator_ += size;
        Region::ObjectAddressToRange(ret)->IncrementAliveObject(size);
        return ret;
    }
    FreeObject *object = freeList_->Allocate(size);
    if (object != nullptr) {
        return Allocate(object, size);
    }

    if (isLocal) {
        return ret;
    }

    if (sweeping_) {
        if (heap_->GetSweeper()->FillSweptRegion(type_, this)) {
            object = freeList_->Allocate(size);
            if (object != nullptr) {
                return Allocate(object, size);
            }
        }

        // Parallel
        heap_->GetSweeper()->EnsureTaskFinished(type_);
        object = freeList_->Allocate(size);
        if (object != nullptr) {
            return Allocate(object, size);
        }
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
        Free(begin + size, end);
        Region::ObjectAddressToRange(begin)->IncrementAliveObject(size);
        return begin;
    } else {
        FreeBumpPoint();
        bpAllocator_.Reset(begin, end);
        auto ret = bpAllocator_.Allocate(size);
        if (ret != 0) {
            Region::ObjectAddressToRange(ret)->IncrementAliveObject(size);
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), bpAllocator_.GetTop(), bpAllocator_.Available());
        }
        return ret;
    }
}

void FreeListAllocator::FreeBumpPoint()
{
    auto begin = bpAllocator_.GetTop();
    auto end = bpAllocator_.GetEnd();
    bpAllocator_.Reset();
    Free(begin, end);
}

void FreeListAllocator::Free(uintptr_t begin, uintptr_t end, bool isAdd)
{
    size_t size = end - begin;
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

inline void FreeListAllocator::LinkFreeObjectKind(Region *region)
{
    region->EnumerateKinds([&](FreeObjectKind *kind) {
        if (kind == nullptr || kind->Empty()) {
            return;
        }
        freeList_->AddKind(kind);
    });
#ifndef NDEBUG
    freeList_->IncrementWastedSize(region->GetWastedSize());
#endif
}

inline void FreeListAllocator::UnlinkFreeObjectKind(Region *region)
{
    region->EnumerateKinds([&](FreeObjectKind *kind) {
        if (kind == nullptr || kind->Empty()) {
            return;
        }
        freeList_->RemoveKind(kind);
    });
#ifndef NDEBUG
    freeList_->DecrementWastedSize(region->GetWastedSize());
#endif
}

#ifndef NDEBUG
size_t FreeListAllocator::GetAvailableSize() const
{
    if (sweeping_) {
        heap_->GetSweeper()->EnsureTaskFinished(type_);
    }
    return freeList_->GetFreeObjectSize() + bpAllocator_.Available();
}

size_t FreeListAllocator::GetWastedSize() const
{
    if (sweeping_) {
        heap_->GetSweeper()->EnsureTaskFinished(type_);
    }
    return freeList_->GetWastedSize();
}
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_ALLOCATOR_INL_H
