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

#ifndef ECMASCRIPT_MEM_TLAB_ALLOCATOR_INL_H
#define ECMASCRIPT_MEM_TLAB_ALLOCATOR_INL_H

#include "ecmascript/mem/tlab_allocator.h"

#include "ecmascript/free_object.h"
#include "ecmascript/mem/full_gc.h"

namespace panda::ecmascript {
static constexpr size_t MIN_BUFFER_SIZE = 31 * 1024;
static constexpr size_t SMALL_OBJECT_SIZE = 8 * 1024;

TlabAllocator::TlabAllocator(Heap *heap)
    : heap_(heap), enableExpandYoung_(true)
{
    size_t maxOldSpaceCapacity = heap->GetEcmaVM()->GetJSOptions().MaxOldSpaceCapacity();
    localSpace_ = new LocalSpace(heap, maxOldSpaceCapacity, maxOldSpaceCapacity);
    youngAllocator_.Reset();
}

inline void TlabAllocator::Finalize()
{
    if (youngAllocator_.Available() != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngAllocator_.GetTop(), youngAllocator_.Available());
        youngAllocator_.Reset();
    }

    heap_->MergeToOldSpaceSync(localSpace_);
}

uintptr_t TlabAllocator::Allocate(size_t size, MemSpaceType space)
{
    uintptr_t result = 0;
    switch (space) {
        case SEMI_SPACE:
            result = AllocateInYoungSpace(size);
            break;
        case OLD_SPACE:
            result = AllocateInOldSpace(size);
            break;
        case COMPRESS_SPACE:
            result = AllocateInCompressSpace(size);
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

uintptr_t TlabAllocator::AllocateInYoungSpace(size_t size)
{
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    if (UNLIKELY(size > SMALL_OBJECT_SIZE)) {
        uintptr_t address = heap_->AllocateYoungSync(size);
        return address;
    }
    uintptr_t result = youngAllocator_.Allocate(size);
    if (result != 0) {
        return result;
    }
    if (!enableExpandYoung_ || !ExpandYoung()) {
        enableExpandYoung_ = false;
        return 0;
    }
    return youngAllocator_.Allocate(size);
}

uintptr_t TlabAllocator::AllocateInCompressSpace(size_t size)
{
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t result = localSpace_->Allocate(size, true);
    ASSERT(result != 0);
    return result;
}

uintptr_t TlabAllocator::AllocateInOldSpace(size_t size)
{
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    // 1. Allocate from freelist in compress allocator
    uintptr_t result = localSpace_->Allocate(size, false);
    if (result == 0) {
        // 2. Expand region from old space
        ExpandCompressFromOld(size);
        result = localSpace_->Allocate(size, true);
    }
    ASSERT(result != 0);
    return result;
}

bool TlabAllocator::ExpandYoung()
{
    uintptr_t buffer = heap_->AllocateYoungSync(MIN_BUFFER_SIZE);
    if (buffer == 0) {
        if (youngAllocator_.Available() != 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngAllocator_.GetTop(), youngAllocator_.Available());
        }
        return false;
    }
    uintptr_t end = buffer + MIN_BUFFER_SIZE;

    if (buffer == youngAllocator_.GetEnd()) {
        buffer = youngAllocator_.GetTop();
    } else {
        if (youngAllocator_.Available() != 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngAllocator_.GetTop(), youngAllocator_.Available());
        }
    }
    youngAllocator_.Reset(buffer, end);
    return true;
}

bool TlabAllocator::ExpandCompressFromOld(size_t size)
{
    auto region = heap_->GetOldSpace()->TryToGetExclusiveRegion(size);
    if (region != nullptr) {
        localSpace_->AddRegionToList(region);
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_TLAB_ALLOCATOR_INL_H
