/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/space-inl.h"

namespace panda::ecmascript {
Region *HeapRegionAllocator::AllocateAlignedRegion(Space *space, size_t capacity, JSThread* thread)
{
    if (capacity == 0) {
        LOG_ECMA_MEM(FATAL) << "capacity must have a size bigger than 0";
        UNREACHABLE();
    }
    RegionFlags flags = space->GetRegionFlag();
    bool isRegular = (flags == RegionFlags::IN_HUGE_OBJECT_SPACE) ? false : true;
    auto pool = MemMapAllocator::GetInstance()->Allocate(capacity, DEFAULT_REGION_SIZE, isRegular);
    void *mapMem = pool.GetMem();
    if (mapMem == nullptr) {
        LOG_ECMA_MEM(FATAL) << "pool is empty " << annoMemoryUsage_.load(std::memory_order_relaxed);
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(mapMem, capacity, 0, capacity) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    IncreaseAnnoMemoryUsage(capacity);

    uintptr_t mem = ToUintPtr(mapMem);
    // Check that the address is 256K byte aligned
    LOG_IF(AlignUp(mem, PANDA_POOL_ALIGNMENT_IN_BYTES) != mem, FATAL, RUNTIME) << "region not align by 256KB";

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uintptr_t begin = AlignUp(mem + sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
    uintptr_t end = mem + capacity;

    return new (ToVoidPtr(mem)) Region(thread, mem, begin, end, flags);
}

void HeapRegionAllocator::FreeRegion(Region *region)
{
    auto size = region->GetCapacity();
    DecreaseAnnoMemoryUsage(size);
    bool isRegular = region->InHugeObjectSpace() ? false : true;
    auto allocateBase = region->GetAllocateBase();
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ToVoidPtr(allocateBase), size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    MemMapAllocator::GetInstance()->Free(ToVoidPtr(allocateBase), size, isRegular);
}
}  // namespace panda::ecmascript
