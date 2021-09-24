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

#include <cstdlib>

#include "ecmascript/free_object.h"
#include "ecmascript/mem/compress_collector.h"
#include "ecmascript/mem/semi_space_collector-inl.h"
#include "ecmascript/mem/tlab_allocator.h"

namespace panda::ecmascript {
static constexpr size_t YOUNG_BUFFER_SIZE = 31 * 1024;
static constexpr size_t OLD_BUFFER_SIZE = 255 * 1024;

TlabAllocator::TlabAllocator(Heap *heap, TriggerGCType gcType)
    : heap_(heap), gcType_(gcType), youngBegin_(0), youngTop_(0), youngEnd_(0), youngEnable_(true), oldBegin_(0),
    oldTop_(0), oldEnd_(0)
{
}

TlabAllocator::~TlabAllocator()
{
    if (youngTop_ != 0 && youngTop_ != youngEnd_) {
        FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngTop_, youngEnd_ - youngTop_);
    }
    if (oldTop_ != 0 && oldTop_ != oldEnd_) {
        if (gcType_ == TriggerGCType::SEMI_GC) {
            heap_->GetSemiSpaceCollector()->oldSpaceAllocator_.Free(oldTop_, oldEnd_);
        } else if (gcType_ == TriggerGCType::COMPRESS_FULL_GC) {
            heap_->GetCompressCollector()->oldSpaceAllocator_.Free(oldTop_, oldEnd_);
        }
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
        uintptr_t largeObject = 0;
        if (gcType_ == TriggerGCType::SEMI_GC) {
            largeObject = heap_->GetSemiSpaceCollector()->AllocateOld(size);
        } else {
            UNREACHABLE();
        }
        return largeObject;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(youngTop_ + size > youngEnd_)) {
        // set FreeObject and extend
        if (youngEnd_ - youngTop_ != 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), youngTop_, youngEnd_ - youngTop_);
        }
        if (!youngEnable_ || !ExpandYoung()) {
            youngEnable_ = false;
            return 0;
        }
    }

    uintptr_t result = youngTop_;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    youngTop_ += size;
    return result;
}

uintptr_t TlabAllocator::TlabAllocatorOldSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(oldTop_ + size > oldEnd_)) {
        // set FreeObject and extend
        if (oldEnd_ - oldTop_ != 0) {
            FreeObject::FillFreeObject(heap_->GetEcmaVM(), oldTop_, oldEnd_ - oldTop_);
        }
        if (!ExpandOld()) {
            return 0;
        }
    }

    uintptr_t result = oldTop_;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    oldTop_ += size;
    return result;
}

bool TlabAllocator::ExpandYoung()
{
    uintptr_t buffer = 0;
    if (gcType_ == TriggerGCType::SEMI_GC) {
        buffer = heap_->GetSemiSpaceCollector()->AllocateYoung(YOUNG_BUFFER_SIZE);
    } else {
        UNREACHABLE();
    }

    if (buffer == 0) {
        return false;
    }
    youngBegin_ = buffer;
    youngTop_ = youngBegin_;
    youngEnd_ = youngBegin_ + YOUNG_BUFFER_SIZE;
    return true;
}

bool TlabAllocator::ExpandOld()
{
    uintptr_t buffer = 0;
    if (gcType_ == TriggerGCType::SEMI_GC) {
        buffer = heap_->GetSemiSpaceCollector()->AllocateOld(YOUNG_BUFFER_SIZE);
    } else if (gcType_ == TriggerGCType::COMPRESS_FULL_GC) {
        buffer = heap_->GetCompressCollector()->AllocateOld(OLD_BUFFER_SIZE);
    } else {
        UNREACHABLE();
    }

    if (buffer == 0) {
        return false;
    }
    oldBegin_ = buffer;
    oldTop_ = oldBegin_;
    if (gcType_ == TriggerGCType::SEMI_GC) {
        oldEnd_ = oldBegin_ + YOUNG_BUFFER_SIZE;
    } else if (gcType_ == TriggerGCType::COMPRESS_FULL_GC) {
        oldEnd_ = oldBegin_ + OLD_BUFFER_SIZE;
    } else {
        UNREACHABLE();
    }
    return true;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_TLAB_ALLOCATOR_INL_H
