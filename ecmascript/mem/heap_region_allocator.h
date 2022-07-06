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

#ifndef ECMASCRIPT_MEM_HEAP_REGION_ALLOCATOR_H
#define ECMASCRIPT_MEM_HEAP_REGION_ALLOCATOR_H

#include <atomic>

#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
class JSThread;
class Region;
class Space;

class HeapRegionAllocator {
public:
    HeapRegionAllocator() = default;
    virtual ~HeapRegionAllocator() = default;

    Region *AllocateAlignedRegion(Space *space, size_t capacity, JSThread* thread);
    void FreeRegion(Region *region);

    void IncreaseAnnoMemoryUsage(size_t bytes)
    {
        size_t current = annoMemoryUsage_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        size_t max = maxAnnoMemoryUsage_.load(std::memory_order_relaxed);
        while (current > max && !maxAnnoMemoryUsage_.compare_exchange_weak(max, current, std::memory_order_relaxed)) {
        }
    }

    void DecreaseAnnoMemoryUsage(size_t bytes)
    {
        annoMemoryUsage_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    size_t GetAnnoMemoryUsage() const
    {
        return annoMemoryUsage_.load(std::memory_order_relaxed);
    }

    size_t GetMaxAnnoMemoryUsage() const
    {
        return maxAnnoMemoryUsage_.load(std::memory_order_relaxed);
    }

private:
    NO_COPY_SEMANTIC(HeapRegionAllocator);
    NO_MOVE_SEMANTIC(HeapRegionAllocator);

    std::atomic<size_t> annoMemoryUsage_ {0};
    std::atomic<size_t> maxAnnoMemoryUsage_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_REGION_ALLOCATOR_H
