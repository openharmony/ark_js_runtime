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

#ifndef ECMASCRIPT_MM_REGION_FACTORY_H
#define ECMASCRIPT_MM_REGION_FACTORY_H

#include "ecmascript/mem/region_factory.h"

#include <malloc.h>

#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/heap.h"
#include "libpandabase/mem/pool_manager.h"
#include "os/mem.h"

namespace panda::ecmascript {
Region *RegionFactory::AllocateAlignedRegion(Space *space, size_t capacity)
{
    if (capacity == 0) {
        LOG_ECMA_MEM(FATAL) << "capacity must have a size bigger than 0";
        UNREACHABLE();
    }
    size_t commitSize = capacity;

    auto pool = PoolManager::GetMmapMemPool()->AllocPool(commitSize, panda::SpaceType::SPACE_TYPE_OBJECT,
                                                         AllocatorType::RUNSLOTS_ALLOCATOR, nullptr);
    void *mapMem = pool.GetMem();
    if (mapMem == nullptr) {
        LOG_ECMA_MEM(FATAL) << "pool is empty " << annoMemoryUsage_.load(std::memory_order_relaxed);
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(mapMem, commitSize, 0, commitSize);
#endif
    IncreaseAnnoMemoryUsage(capacity);

    uintptr_t mem = ToUintPtr(mapMem);
    // Check that the address is 256K byte aligned
    LOG_IF(AlignUp(mem, PANDA_POOL_ALIGNMENT_IN_BYTES) != mem, FATAL, RUNTIME) << "region not align by 256KB";

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uintptr_t begin = AlignUp(mem + sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
    uintptr_t end = mem + capacity;

    return new (ToVoidPtr(mem)) Region(space, space->GetHeap(), mem, begin, end, this);
}

void RegionFactory::FreeRegion(Region *region)
{
    auto size = region->GetCapacity();
    DecreaseAnnoMemoryUsage(size);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(ToVoidPtr(region->GetAllocateBase()), size, INVALID_VALUE, size);
#endif
    PoolManager::GetMmapMemPool()->FreePool(ToVoidPtr(region->GetAllocateBase()), size);
}

Area *RegionFactory::AllocateArea(size_t capacity)
{
    size_t headerSize = sizeof(Area);
    if (capacity < headerSize) {
        LOG_ECMA_MEM(FATAL) << "capacity must have a size not less than sizeof Area.";
        UNREACHABLE();
    }
    if (cachedArea_ != nullptr && capacity <= cachedArea_->GetSize()) {
        auto result = cachedArea_;
        cachedArea_ = nullptr;
        return result;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    void *mem = malloc(capacity);
    if (mem == nullptr) {
        LOG_ECMA_MEM(FATAL) << "malloc failed";
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(mem, capacity, 0, capacity);
#endif
    IncreaseNativeMemoryUsage(capacity);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uintptr_t begin = reinterpret_cast<uintptr_t>(mem) + headerSize;
    capacity -= headerSize;
    return new (mem) Area(begin, capacity);
}

void RegionFactory::FreeArea(Area *area)
{
    if (area == nullptr) {
        return;
    }
    if (cachedArea_ == nullptr && area->GetSize() <= MAX_CACHED_CHUNK_AREA_SIZE) {
        cachedArea_ = area;
        return;
    }
    auto size = area->GetSize() + sizeof(Area);
    DecreaseNativeMemoryUsage(size);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(area, size, INVALID_VALUE, size);
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(reinterpret_cast<std::byte *>(area));
}

void RegionFactory::Free(void *mem, size_t size)
{
    if (mem == nullptr) {
        return;
    }
    DecreaseNativeMemoryUsage(size);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(mem, size, INVALID_VALUE, size);
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}

void *RegionFactory::AllocateBuffer(size_t size)
{
    if (size == 0) {
        LOG_ECMA_MEM(FATAL) << "size must have a size bigger than 0";
        UNREACHABLE();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    void *ptr = malloc(size);
    if (ptr == nullptr) {
        LOG_ECMA_MEM(FATAL) << "malloc failed";
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(ptr, size, INVALID_VALUE, size);
#endif
    IncreaseNativeMemoryUsage(size);
    return ptr;
}

void RegionFactory::FreeBuffer(void *mem)
{
    if (mem == nullptr) {
        return;
    }
    DecreaseNativeMemoryUsage(malloc_usable_size(mem));
#if ECMASCRIPT_ENABLE_ZAP_MEM
    memset_s(mem, size, INVALID_VALUE, size);
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}

void RegionFactory::FreeBufferFunc(void *buffer, void* data)
{
    if (buffer == nullptr || data == nullptr) {
        return;
    }
    RegionFactory* factory = reinterpret_cast<RegionFactory*>(data);
    factory->FreeBuffer(buffer);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MM_REGION_FACTORY_H
