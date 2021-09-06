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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_ALLOCATOR_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_ALLOCATOR_INL_H

#include <cstdlib>

#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/free_object.h"

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
    if (size == 0) {
        LOG_ECMA_MEM(FATAL) << "size must have a size bigger than 0";
        UNREACHABLE();
    }
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(top_ + size > end_)) {
        return 0;
    }
    uintptr_t result = top_;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    top_ += size;
    return result;
}

FreeListAllocator::FreeListAllocator(const Space *space)
{
    freeList_ = std::make_unique<FreeObjectList>();
    heap_ = space->GetHeap();
    uintptr_t begin = space->GetCurrentRegion()->GetBegin();
    size_t size = space->GetCurrentRegion()->GetSize();
    FreeObject::Cast(begin)->SetAvailable(size);
    freeList_->Free(begin, size);
}

void FreeListAllocator::AddFree(Region *region)
{
    auto begin = region->GetBegin();
    auto end = region->GetEnd();
    Free(begin, end);
}

uintptr_t FreeListAllocator::Allocate(size_t size)
{
    if (UNLIKELY(size < static_cast<size_t>(TaggedObject::ObjectHeaderSize()))) {
        return 0;
    }
    auto ret = bpAllocator_.Allocate(size);
    if (LIKELY(ret != 0)) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), bpAllocator_.GetTop(), bpAllocator_.Available());
        return ret;
    }
    FreeObject *object = freeList_->Allocator(size);
    if (LIKELY(object != nullptr && !object->IsEmpty())) {
        FreeBumpPoint();
        bpAllocator_.Reset(object->GetBegin(), object->GetEnd());
        ret = bpAllocator_.Allocate(size);
        if (ret != 0 && bpAllocator_.Available() > 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), bpAllocator_.GetTop(), bpAllocator_.Available());
        }
        return ret;
    }

    return 0;
}

void FreeListAllocator::FreeBumpPoint()
{
    auto begin = bpAllocator_.GetTop();
    auto end = bpAllocator_.GetEnd();
    Free(begin, end);
    bpAllocator_.Reset();
}

void FreeListAllocator::Free(uintptr_t begin, uintptr_t end)
{
    ASSERT(heap_ != nullptr);
    size_t size = end - begin;
    if (UNLIKELY(size < FreeObject::SIZE_OFFSET)) {
        return;
    }
    FreeObject::FillFreeObject(heap_->GetEcmaVM(), begin, size);

    freeList_->Free(begin, static_cast<int>(end - begin));
}

void FreeListAllocator::RebuildFreeList()
{
    bpAllocator_.Reset();
    freeList_->Rebuild();
}
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_ALLOCATOR_INL_H
