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

#include "ecmascript/mem/native_area_allocator.h"

#ifndef PANDA_TARGET_MACOS
#include <malloc.h>
#else
#include <malloc/malloc.h>
#endif
#include "os/mem.h"

namespace panda::ecmascript {
Area *NativeAreaAllocator::AllocateArea(size_t capacity)
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
        LOG_ECMA_MEM(FATAL) << "malloc failed,  current alloc size = " << capacity 
                            << ", total allocated size = " << nativeMemoryUsage_.load(std::memory_order_relaxed);
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(mem, capacity, 0, capacity) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    IncreaseNativeMemoryUsage(capacity);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uintptr_t begin = reinterpret_cast<uintptr_t>(mem) + headerSize;
    capacity -= headerSize;
    return new (mem) Area(begin, capacity);
}

void NativeAreaAllocator::FreeArea(Area *area)
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
    if (memset_s(area, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(reinterpret_cast<std::byte *>(area));
}

void NativeAreaAllocator::Free(void *mem, size_t size)
{
    if (mem == nullptr) {
        return;
    }
    DecreaseNativeMemoryUsage(size);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(mem, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}

void *NativeAreaAllocator::AllocateBuffer(size_t size)
{
    if (size == 0) {
        LOG_ECMA_MEM(FATAL) << "size must have a size bigger than 0";
        UNREACHABLE();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    void *ptr = malloc(size);
    if (ptr == nullptr) {
        LOG_ECMA_MEM(FATAL) << "malloc failed, current alloc size = " << size 
                            << ", total allocated size = " << nativeMemoryUsage_.load(std::memory_order_relaxed);
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ptr, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    IncreaseNativeMemoryUsage(size);
    return ptr;
}

void NativeAreaAllocator::FreeBuffer(void *mem)
{
    if (mem == nullptr) {
        return;
    }
#if defined(PANDA_TARGET_WINDOWS)
    DecreaseNativeMemoryUsage(_msize(mem));
#elif defined(PANDA_TARGET_MACOS)
    DecreaseNativeMemoryUsage(malloc_size(mem));
#else
    DecreaseNativeMemoryUsage(malloc_usable_size(mem));
#endif

#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(mem, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}

void NativeAreaAllocator::FreeBufferFunc(void *buffer, void* data)
{
    if (buffer == nullptr || data == nullptr) {
        return;
    }
    NativeAreaAllocator* allocator = reinterpret_cast<NativeAreaAllocator*>(data);
    allocator->FreeBuffer(buffer);
}
}  // namespace panda::ecmascript
