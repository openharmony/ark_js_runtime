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
#include "ecmascript/mem/mem_manager-inl.h"

namespace panda::ecmascript {
static constexpr size_t MIN_BUFFER_SIZE = 31 * 1024;
static constexpr size_t SMALL_OBJECT_SIZE = 8 * 1024;

TlabAllocator::TlabAllocator(Heap *heap)
    : heap_(heap), memManager_(heap_->GetHeapManager()), enableExpandYoung_(true), localSpace_(heap_)
{
    youngerAllocator_.Reset();
    localAllocator_.Reset(heap_);
}

inline void TlabAllocator::Finalize()
{
    if (youngerAllocator_.Available() != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngerAllocator_.GetTop(), youngerAllocator_.Available());
        youngerAllocator_.Reset();
    }

    localAllocator_.FreeBumpPoint();
    memManager_->MergeToOldSpaceSync(&localSpace_, &localAllocator_);
}

uintptr_t TlabAllocator::Allocate(size_t size, MemSpaceType spaceAlloc)
{
    uintptr_t result = 0;
    switch (spaceAlloc) {
        case SEMI_SPACE:
            result = TlabAllocatorYoungSpace(size);
            break;
        case OLD_SPACE:
            result = TlabAllocatorOldSpace(size);
            break;
        case COMPRESS_SPACE:
            result = TlabAllocatorCompressSpace(size);
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

uintptr_t TlabAllocator::TlabAllocatorYoungSpace(size_t size)
{
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    if (UNLIKELY(size > SMALL_OBJECT_SIZE)) {
        uintptr_t address = memManager_->AllocateYoungSync(size);
        return address;
    }
    uintptr_t result = youngerAllocator_.Allocate(size);
    if (result != 0) {
        return result;
    }
    if (!enableExpandYoung_ || !ExpandYoung()) {
        enableExpandYoung_ = false;
        return 0;
    }
    return youngerAllocator_.Allocate(size);
}

uintptr_t TlabAllocator::TlabAllocatorCompressSpace(size_t size)
{
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t result = localAllocator_.Allocate<true>(size);
    if (result == 0) {
        if (ExpandCompress()) {
            result = localAllocator_.Allocate<true>(size);
        }
    }
    ASSERT(result != 0);
    return result;
}

uintptr_t TlabAllocator::TlabAllocatorOldSpace(size_t size)
{
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    // 1. Allocate from freelist in compress allocator
    uintptr_t result = localAllocator_.Allocate<true>(size);
    if (result == 0) {
        // 2. Expand region from old space
        if (ExpandCompressFromOld(size)) {
            result = localAllocator_.Allocate<true>(size);
            if (result != 0) {
                return result;
            }
        }
        if (ExpandCompress()) {
            result = localAllocator_.Allocate<true>(size);
        }
    }
    ASSERT(result != 0);
    return result;
}

bool TlabAllocator::ExpandYoung()
{
    uintptr_t buffer = memManager_->AllocateYoungSync(MIN_BUFFER_SIZE);
    if (buffer == 0) {
        if (youngerAllocator_.Available() != 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngerAllocator_.GetTop(), youngerAllocator_.Available());
        }
        return false;
    }
    uintptr_t end = buffer + MIN_BUFFER_SIZE;

    if (buffer == youngerAllocator_.GetEnd()) {
        buffer = youngerAllocator_.GetTop();
    } else {
        if (youngerAllocator_.Available() != 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngerAllocator_.GetTop(), youngerAllocator_.Available());
        }
    }
    youngerAllocator_.Reset(buffer, end);
    return true;
}

bool TlabAllocator::ExpandCompress()
{
    if (localSpace_.Expand()) {
        auto region = localSpace_.GetCurrentRegion();
        localAllocator_.AddFree(region);
        return true;
    }
    return false;
}

bool TlabAllocator::ExpandCompressFromOld(size_t size)
{
    auto region = memManager_->TryToGetExclusiveRegion(size);
    if (region != nullptr) {
        localSpace_.AddRegionToList(region);
        localAllocator_.CollectFreeObjectSet(region);
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_TLAB_ALLOCATOR_INL_H
