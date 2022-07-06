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

#include "ecmascript/mem/mem_map_allocator.h"
#if defined(PANDA_TARGET_WINDOWS)
#include <io.h>
#include <sysinfoapi.h>
#elif defined(PANDA_TARGET_MACOS)
#include "sys/sysctl.h"
#else
#include "sys/sysinfo.h"
#endif
#ifdef PANDA_TARGET_WINDOWS

void *mmap(size_t size, int fd, off_t offset)
{
    HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    HANDLE extra = CreateFileMapping(handle, nullptr, PAGE_READWRITE,
                                     (DWORD) ((uint64_t) size >> 32),
                                     (DWORD) (size & 0xffffffff),
                                     nullptr);
    if (extra == nullptr) {
        return nullptr;
    }

    void *data = MapViewOfFile(extra, FILE_MAP_WRITE | FILE_MAP_READ,
                               (DWORD) ((uint64_t) offset >> 32),
                               (DWORD) (offset & 0xffffffff),
                               size);
    CloseHandle(extra);
    return data;
}
#endif

namespace panda::ecmascript {
MemMap MemMapAllocator::Allocate(size_t size, size_t alignment, bool isRegular)
{
    if (UNLIKELY(memMapTotalSize_ + size > capacity_)) {
        LOG_GC(ERROR) << "memory map overflow";
        return MemMap();
    }
    MemMap mem;
    if (isRegular) {
        mem = memMapPool_.GetMemFromCache(size);
        if (mem.GetMem() != nullptr) {
            memMapTotalSize_ += size;
            PageTag(mem.GetMem(), size);
            return mem;
        }
        mem = PageMap(REGULAR_REGION_MMAP_SIZE, alignment);
        memMapPool_.InsertMemMap(mem);
        mem = memMapPool_.SplitMemToCache(mem);
    } else {
        mem = memMapFreeList_.GetMemFromList(size);
    }
    if (mem.GetMem() != nullptr) {
        PageTag(mem.GetMem(), mem.GetSize());
        memMapTotalSize_ += mem.GetSize();
    }
    return mem;
}

void MemMapAllocator::Free(void *mem, size_t size, bool isRegular)
{
    memMapTotalSize_ -= size;
    PageRelease(mem, size);
    if (isRegular) {
        memMapPool_.AddMemToCache(mem, size);
    } else {
        memMapFreeList_.AddMemToList(MemMap(mem, size));
    }
}

MemMap MemMapAllocator::PageMap(size_t size, size_t alignment)
{
    size_t allocSize = size + alignment;
#ifdef PANDA_TARGET_UNIX
    void *result = mmap(0, allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
    void *result = mmap(allocSize, -1, 0);
#endif
    LOG_ECMA_IF(result == nullptr, FATAL) << "mmap fail";
    auto alignResult = AlignUp(reinterpret_cast<uintptr_t>(result), alignment);
#ifdef PANDA_TARGET_UNIX
    size_t leftSize = alignResult - reinterpret_cast<uintptr_t>(result);
    size_t rightSize = alignment - leftSize;
    void *alignEndResult = reinterpret_cast<void *>(alignResult + size);
    munmap(result, leftSize);
    munmap(alignEndResult, rightSize);
#endif
    return MemMap(reinterpret_cast<void *>(alignResult), size);
}

void MemMapAllocator::AdapterSuitablePoolCapacity()
{
#ifdef PANDA_TARGET_WINDOWS
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&status);
    DWORDLONG physSize = status.ullTotalPhys;
#elif PANDA_TARGET_MACOS
    static constexpr int MIB_LENGTH = 2;
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    int64_t size = 0;
    size_t bufferLength = sizeof(size);
    if (sysctl(mib, MIB_LENGTH, &size, &bufferLength, NULL, 0) != 0) {
        LOG_GC(FATAL) << "sysctl error";
    }
    size_t physSize = static_cast<size_t>(size);
#else
    auto pages = sysconf(_SC_PHYS_PAGES);
    auto pageSize = sysconf(_SC_PAGE_SIZE);
    size_t physSize = pages * pageSize;
#endif
    capacity_ = std::max<size_t>(physSize / PHY_SIZE_MULTIPLE, MIN_MEM_POOL_CAPACITY);
    if (capacity_ > LARGE_POOL_SIZE) {
        capacity_ = std::max<size_t>(capacity_, STANDARD_POOL_SIZE);
    } else if (capacity_ >= MEDIUM_POOL_SIZE) {
        capacity_ = std::min<size_t>(capacity_, STANDARD_POOL_SIZE);
    } else if (capacity_ >= LOW_POOL_SIZE) {
        capacity_ = std::max<size_t>(capacity_, 128_MB);
    }
    LOG_GC(INFO) << "Ark Auto adapter memory pool capacity:" << capacity_;
}
}  // namespace panda::ecmascript
