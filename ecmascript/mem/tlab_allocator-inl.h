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

#include <cstdlib>

#include "ecmascript/free_object.h"
#include "ecmascript/mem/compress_collector.h"
#include "ecmascript/mem/evacuation_allocator-inl.h"

namespace panda::ecmascript {
static constexpr size_t YOUNG_BUFFER_SIZE = 31 * 1024;
static constexpr size_t OLD_BUFFER_SIZE = 255 * 1024;

TlabAllocator::TlabAllocator(Heap *heap, TriggerGCType gcType)
    : heap_(heap), gcType_(gcType), youngEnable_(true), allocator_(heap_->GetEvacuationAllocator())
{
}

TlabAllocator::~TlabAllocator()
{
    Finalize();
}

inline void TlabAllocator::Finalize()
{
    if (youngerAllocator_.Available() != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngerAllocator_.GetTop(), youngerAllocator_.Available());
        youngerAllocator_.Reset();
    }
    if (oldBumpPointerAllocator_.Available() != 0) {
        allocator_->FreeSafe(oldBumpPointerAllocator_.GetTop(), oldBumpPointerAllocator_.GetEnd());
        Region *current = Region::ObjectAddressToRange(oldBumpPointerAllocator_.GetTop());
        current->DecreaseAliveObject(oldBumpPointerAllocator_.Available());
        oldBumpPointerAllocator_.Reset();
    }
}

uintptr_t TlabAllocator::Allocate(size_t size, SpaceAlloc spaceAlloc)
{
    uintptr_t result = 0;
    switch (spaceAlloc) {
        case SpaceAlloc::YOUNG_SPACE:
            result = TlabAllocatorYoungSpace(size);
            break;
        case SpaceAlloc::OLD_SPACE:
            result = TlabAllocatorOldSpace(size);
            break;
        default:
            UNREACHABLE();
    }
    return result;
}

uintptr_t TlabAllocator::TlabAllocatorYoungSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (UNLIKELY(size >= SMALL_OBJECT_SIZE)) {
        uintptr_t address =  allocator_->AllocateYoung(size);
        LOG(DEBUG, RUNTIME) << "AllocatorYoungSpace:" << address;
        return address;
    }
    uintptr_t result = youngerAllocator_.Allocate(size);
    if (result != 0) {
        return result;
    }
    if (youngerAllocator_.Available() != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngerAllocator_.GetTop(), youngerAllocator_.Available());
    }
    if (!youngEnable_ || !ExpandYoung()) {
        youngEnable_ = false;
        return 0;
    }
    return youngerAllocator_.Allocate(size);
}

uintptr_t TlabAllocator::TlabAllocatorOldSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t result = oldBumpPointerAllocator_.Allocate(size);
    if (result != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), oldBumpPointerAllocator_.GetTop(),
            oldBumpPointerAllocator_.Available());
        return result;
    }
    Region *current = Region::ObjectAddressToRange(oldBumpPointerAllocator_.GetTop());
    if (current != nullptr) {
        current->DecreaseAliveObject(oldBumpPointerAllocator_.Available());
    }
    allocator_->FreeSafe(oldBumpPointerAllocator_.GetTop(), oldBumpPointerAllocator_.GetEnd());
    if (!ExpandOld()) {
        return 0;
    }
    result = oldBumpPointerAllocator_.Allocate(size);
    if (result != 0) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), oldBumpPointerAllocator_.GetTop(),
            oldBumpPointerAllocator_.Available());
    }
    return result;
}

bool TlabAllocator::ExpandYoung()
{
    uintptr_t buffer = 0;
    buffer = allocator_->AllocateYoung(YOUNG_BUFFER_SIZE);

    if (buffer == 0) {
        return false;
    }
    youngerAllocator_.Reset(buffer, buffer + YOUNG_BUFFER_SIZE);
    return true;
}

bool TlabAllocator::ExpandOld()
{
    uintptr_t buffer = 0;
    if (gcType_ == TriggerGCType::SEMI_GC) {
        buffer = allocator_->AllocateOld(YOUNG_BUFFER_SIZE);
        if (buffer == 0) {
            return false;
        }
        oldBumpPointerAllocator_.Reset(buffer, buffer + YOUNG_BUFFER_SIZE);
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), oldBumpPointerAllocator_.GetTop(),
            oldBumpPointerAllocator_.Available());
    } else if (gcType_ == TriggerGCType::COMPRESS_FULL_GC) {
        Region *region = allocator_->ExpandOldSpace();
        if (region == nullptr) {
            return false;
        }
        region->SetAliveObject(region->GetSize());
        oldBumpPointerAllocator_.Reset(region->GetBegin(), region->GetEnd());
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), oldBumpPointerAllocator_.GetTop(),
            oldBumpPointerAllocator_.Available());
    } else {
        UNREACHABLE();
    }

    return true;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_TLAB_ALLOCATOR_INL_H
