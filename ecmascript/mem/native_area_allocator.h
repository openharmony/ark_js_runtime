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

#ifndef ECMASCRIPT_MEM_NATIVE_AREA_ALLOCATOR_H
#define ECMASCRIPT_MEM_NATIVE_AREA_ALLOCATOR_H

#include <atomic>

#include "ecmascript/common.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/area.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript {
class PUBLIC_API NativeAreaAllocator {
public:
    NativeAreaAllocator() = default;
    virtual ~NativeAreaAllocator()
    {
        if (cachedArea_ != nullptr) {
            FreeArea(cachedArea_);
            cachedArea_ = nullptr;
        }
    }

    Area *AllocateArea(size_t capacity);
    void FreeArea(Area *area);
    void Free(void *mem, size_t size);
    void *AllocateBuffer(size_t size);
    void FreeBuffer(void *mem);

    static void FreeBufferFunc(void* buffer, void* data);

    template<class T>
    static void FreeObjectFunc(void* buffer, void* data)
    {
        if (buffer == nullptr || data == nullptr) {
            return;
        }
        NativeAreaAllocator* allocator = reinterpret_cast<NativeAreaAllocator*>(data);
        allocator->Delete<T>(static_cast<T *>(buffer));
    }

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

    void *Allocate(size_t size)
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
        IncreaseNativeMemoryUsage(size);
        return ptr;
    }

    static inline Area *AllocateSpace(size_t capacity)
    {
        size_t headerSize = sizeof(Area);
        if (capacity < headerSize) {
            LOG_ECMA_MEM(FATAL) << "capacity must have a size not less than sizeof Area.";
            UNREACHABLE();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void *mem = malloc(capacity);
        if (mem == nullptr) {
            LOG_ECMA_MEM(FATAL) << "malloc failed";
            UNREACHABLE();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        uintptr_t begin = reinterpret_cast<uintptr_t>(mem) + headerSize;
        capacity -= headerSize;
        return new (mem) Area(begin, capacity);
    }

    static inline void FreeSpace(Area *area)
    {
        if (area == nullptr) {
            return;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        free(reinterpret_cast<std::byte *>(area));
    }

private:
    NO_COPY_SEMANTIC(NativeAreaAllocator);
    NO_MOVE_SEMANTIC(NativeAreaAllocator);

#if ECMASCRIPT_ENABLE_ZAP_MEM
    static constexpr int INVALID_VALUE = 0x7;
#endif
    Area *cachedArea_ {nullptr};
    std::atomic<size_t> nativeMemoryUsage_ {0};
    std::atomic<size_t> maxNativeMemoryUsage_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_NATIVE_AREA_ALLOCATOR_H
