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

#ifndef ECMASCRIPT_MEM_REGION_FACTORY_H
#define ECMASCRIPT_MEM_REGION_FACTORY_H

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
    void *Allocate(size_t size);
    void Free(void *mem, size_t size);
    void *AllocateBuffer(size_t size);
    void FreeBuffer(void *mem);

    static void FreeBufferFunc(void* buffer, void* data);

    // implemented by AllocateBuffer
    template<typename T, typename... Args>
    std::enable_if_t<!std::is_array_v<T>, T *> New(Args &&... args)
    {
        void *p = AllocateBuffer(sizeof(T));
        if (UNLIKELY(p == nullptr)) {
            return nullptr;
        }
        new (p) T(std::forward<Args>(args)...);  // NOLINT(bugprone-throw-keyword-missing)
        return reinterpret_cast<T *>(p);
    }

    template<class T>
    void Delete(T *ptr)
    {
        if (ptr == nullptr) {
            return;
        }
        // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
        if constexpr (std::is_class_v<T>) {
            ptr->~T();
        }
        FreeBuffer(ptr);
    }

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

    void IncreaseNativeMemoryUsage(size_t bytes)
    {
        size_t current = nativeMemoryUsage_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        size_t max = maxNativeMemoryUsage_.load(std::memory_order_relaxed);
        while (current > max && !maxNativeMemoryUsage_.compare_exchange_weak(max, current, std::memory_order_relaxed)) {
        }
    }

    void DecreaseNativeMemoryUsage(size_t bytes)
    {
        nativeMemoryUsage_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    size_t GetNativeMemoryUsage() const
    {
        return nativeMemoryUsage_.load(std::memory_order_relaxed);
    }

    size_t GetMaxNativeMemoryUsage() const
    {
        return maxNativeMemoryUsage_.load(std::memory_order_relaxed);
    }

private:
    NO_COPY_SEMANTIC(RegionFactory);
    NO_MOVE_SEMANTIC(RegionFactory);

    Area *cachedArea_{nullptr};
    std::atomic<size_t> annoMemoryUsage_{0};
    std::atomic<size_t> maxAnnoMemoryUsage_{0};
    std::atomic<size_t> nativeMemoryUsage_{0};
    std::atomic<size_t> maxNativeMemoryUsage_{0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_REGION_FACTORY_H
