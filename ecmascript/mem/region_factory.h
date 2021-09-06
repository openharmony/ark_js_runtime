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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_REGION_FACTORY_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_REGION_FACTORY_H

#include <atomic>

#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/area.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript {
class Region;
class Space;
class Area;

class RegionFactory {
public:
    RegionFactory() = default;
    virtual ~RegionFactory()
    {
        if (cachedArea_ != nullptr) {
            FreeArea(cachedArea_);
            cachedArea_ = nullptr;
        }
    }

    Region *AllocateAlignedRegion(Space *space, size_t capacity);
    void FreeRegion(Region *region);
    Area *AllocateArea(size_t capacity);
    void FreeArea(Area *area);
    void *AllocateWithMMap(size_t size);
    void FreeWithMMap(void *mem, size_t size);
    void *Allocate(size_t size);
    void Free(void *mem, size_t size);
    void *AllocateBuffer(size_t size);
    void FreeBuffer(void *mem);

    static void FreeBufferFunc(void* buffer, void* data);

    // implemented by AllocateBuffer
    template <typename T, typename... Args>
    std::enable_if_t<!std::is_array_v<T>, T *> New(Args &&... args)
    {
        void *p = AllocateBuffer(sizeof(T));
        if (UNLIKELY(p == nullptr)) {
            return nullptr;
        }
        new (p) T(std::forward<Args>(args)...);  // NOLINT(bugprone-throw-keyword-missing)
        return reinterpret_cast<T *>(p);
    }

    void IncreaseMemoryUsage(size_t bytes)
    {
        size_t current = memoryUsage_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        size_t max = maxMemoryUsage_.load(std::memory_order_relaxed);
        while (current > max && !maxMemoryUsage_.compare_exchange_weak(max, current, std::memory_order_relaxed)) {
        }
    }

    void DecreaseMemoryUsage(size_t bytes)
    {
        memoryUsage_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    size_t GetCurrentMemoryUsage() const
    {
        return memoryUsage_.load(std::memory_order_relaxed);
    }

    size_t GetMaxMemoryUsage() const
    {
        return maxMemoryUsage_.load(std::memory_order_relaxed);
    }

private:
    NO_COPY_SEMANTIC(RegionFactory);
    NO_MOVE_SEMANTIC(RegionFactory);

    Area *cachedArea_{nullptr};
    std::atomic<size_t> memoryUsage_{0};
    std::atomic<size_t> maxMemoryUsage_{0};
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_REGION_FACTORY_H
