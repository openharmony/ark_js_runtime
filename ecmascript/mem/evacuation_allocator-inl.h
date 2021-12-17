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

#ifndef ECMASCRIPT_MEM_EVACUATION_ALLOCATOR_INL_H
#define ECMASCRIPT_MEM_EVACUATION_ALLOCATOR_INL_H

#include "ecmascript/mem/evacuation_allocator.h"

#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {
Region *EvacuationAllocator::ExpandOldSpace()
{
    os::memory::LockHolder lock(oldAllocatorLock_);
    return heap_->ExpandCompressSpace();
}

void EvacuationAllocator::FreeSafe(uintptr_t begin, uintptr_t end)
{
    os::memory::LockHolder lock(oldAllocatorLock_);
    oldSpaceAllocator_.Free(begin, end);
}

void EvacuationAllocator::Free(uintptr_t begin, uintptr_t end, bool isAdd)
{
    oldSpaceAllocator_.Free(begin, end, isAdd);
}

void EvacuationAllocator::FillFreeList(FreeObjectKind *kind)
{
    oldSpaceAllocator_.FillFreeList(kind);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_EVACUATION_ALLOCATOR_INL_H
