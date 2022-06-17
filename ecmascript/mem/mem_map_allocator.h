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

#ifndef ECMASCRIPT_MEM_MEM_MAP_ALLOCATOR_H
#define ECMASCRIPT_MEM_MEM_MAP_ALLOCATOR_H

#include <deque>
#include <map>

#include "ecmascript/mem/mem.h"
#include "os/mutex.h"

#if !(defined PANDA_TARGET_MACOS || defined PANDA_TARGET_WINDOWS)
#include <sys/prctl.h>
#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#endif

#ifndef PR_SET_VMA_ANON_NAME
#define PR_SET_VMA_ANON_NAME 0
#endif
#endif // PANDA_TARGET_UNIX

#ifdef PANDA_TARGET_WINDOWS
#include <windows.h>

#ifdef ERROR
#undef ERROR
#endif

#ifdef GetObject
#undef GetObject
#endif

#ifdef STRICT
#undef STRICT
#endif

#ifdef VOID
#undef VOID
#endif
#endif

namespace panda::ecmascript {
class MemMap {
public:
    MemMap() : mem_(nullptr), size_(0) {}
    MemMap(void *mem, size_t size) : mem_(mem), size_(size) {};

    inline void *GetMem()
    {
        return mem_;
    }

    inline size_t GetSize()
    {
        return size_;
    }
private:
    void *mem_;
    size_t size_;
};

// Regular region with length of DEFAULT_REGION_SIZE(256kb)
class MemMapPool {
public:
    explicit MemMapPool() = default;
    ~MemMapPool() = default;

    void Finalize()
    {
        os::memory::LockHolder lock(lock_);
        for (auto &it : memMapVector_) {
#ifdef PANDA_TARGET_UNIX
            munmap(it.GetMem(), it.GetSize());
#else
            UnmapViewOfFile(it.GetMem());
#endif
        }
        memMapVector_.clear();
        memMapCache_.clear();
    }

    NO_COPY_SEMANTIC(MemMapPool);
    NO_MOVE_SEMANTIC(MemMapPool);

    MemMap GetMemFromCache([[maybe_unused]]size_t size)
    {
        ASSERT(size == REGULAR_MMAP_SIZE);
        os::memory::LockHolder lock(lock_);
        if (!memMapCache_.empty()) {
            MemMap mem = memMapCache_.front();
            memMapCache_.pop_front();
            return mem;
        }
        return MemMap();
    }

    void AddMemToCache(void *mem, size_t size)
    {
        ASSERT(size == REGULAR_MMAP_SIZE);
        os::memory::LockHolder lock(lock_);
        memMapCache_.emplace_back(mem, size);
    }

    MemMap SplitMemToCache(MemMap memMap)
    {
        os::memory::LockHolder lock(lock_);
        auto remainderMem = reinterpret_cast<uintptr_t>(memMap.GetMem()) + REGULAR_MMAP_SIZE;
        size_t remainderSize = AlignDown(memMap.GetSize() - REGULAR_MMAP_SIZE, REGULAR_MMAP_SIZE);
        size_t count = remainderSize / REGULAR_MMAP_SIZE;
        while (count-- > 0) {
            memMapCache_.emplace_back(reinterpret_cast<void *>(remainderMem), REGULAR_MMAP_SIZE);
            remainderMem = remainderMem + REGULAR_MMAP_SIZE;
        }
        return MemMap(memMap.GetMem(), REGULAR_MMAP_SIZE);
    }

    void InsertMemMap(MemMap memMap)
    {
        os::memory::LockHolder lock(lock_);
        memMapVector_.emplace_back(memMap);
    }

private:
    static constexpr size_t REGULAR_MMAP_SIZE = 256_KB;
    os::memory::Mutex lock_;
    std::deque<MemMap> memMapCache_;
    std::vector<MemMap> memMapVector_;
};

// Non regular region with length of DEFAULT_REGION_SIZE(256kb) multiple
class MemMapFreeList {
public:
    MemMapFreeList() = default;
    ~MemMapFreeList() = default;

    void Initialize(MemMap memMap)
    {
        memMap_ = memMap;
        freeList_.insert(std::pair<size_t, MemMap>(memMap.GetSize(), memMap));
    }

    void Finalize()
    {
#ifdef PANDA_TARGET_UNIX
        munmap(memMap_.GetMem(), memMap_.GetSize());
#else
        UnmapViewOfFile(memMap_.GetMem());
#endif
        freeList_.clear();
    }

    NO_COPY_SEMANTIC(MemMapFreeList);
    NO_MOVE_SEMANTIC(MemMapFreeList);

    MemMap GetMemFromList(size_t size)
    {
        os::memory::LockHolder lock(lock_);
        auto iterate = freeList_.lower_bound(size);
        if (iterate == freeList_.end()) {
            return MemMap();
        }
        freeList_.erase(iterate);
        MemMap memMap = iterate->second;
        size_t remainderSize = memMap.GetSize() - size;
        if (remainderSize >= DEFAULT_REGION_SIZE) {
            auto next = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(memMap.GetMem()) + size);
            freeList_.insert(std::pair<size_t, MemMap>(remainderSize, MemMap(next, remainderSize)));
        }
        return MemMap(memMap.GetMem(), size);
    }

    void AddMemToList(MemMap memMap)
    {
        os::memory::LockHolder lock(lock_);
        freeList_.insert(std::pair<size_t, MemMap>(memMap.GetSize(), memMap));
    }

private:
    os::memory::Mutex lock_;
    MemMap memMap_;
    std::multimap<size_t, MemMap> freeList_;
};

class MemMapAllocator {
public:
    MemMapAllocator() = default;
    ~MemMapAllocator() = default;

    NO_COPY_SEMANTIC(MemMapAllocator);
    NO_MOVE_SEMANTIC(MemMapAllocator);

    void Initialize(size_t alignment)
    {
        AdapterSuitablePoolCapacity();
        memMapTotalSize_ = 0;
        MemMap memMap = PageMap(capacity_ / 2, alignment);
        PageRelease(memMap.GetMem(), memMap.GetSize());
        memMapFreeList_.Initialize(memMap);
    }

    void Finalize()
    {
        memMapTotalSize_ = 0;
        capacity_ = 0;
        memMapFreeList_.Finalize();
        memMapPool_.Finalize();
    }

    static MemMapAllocator *GetInstance()
    {
        static MemMapAllocator vmAllocator_;
        return &vmAllocator_;
    }

    MemMap Allocate(size_t size, size_t alignment, bool isRegular);

    void Free(void *mem, size_t size, bool isRegular);

private:
    static constexpr size_t REGULAR_REGION_MMAP_SIZE = 4_MB;

    MemMap PageMap(size_t size, size_t alignment);

    void PageRelease([[maybe_unused]]void *mem, [[maybe_unused]]size_t size)
    {
#ifdef PANDA_TARGET_UNIX
        madvise(mem, size, MADV_DONTNEED);
#endif
    }

    void PageTag([[maybe_unused]]void *mem, [[maybe_unused]]size_t size, [[maybe_unused]]bool remove = false)
    {
#if !(defined PANDA_TARGET_MACOS || defined PANDA_TARGET_WINDOWS)
        if (remove) {
            prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, mem, size, nullptr);
        } else {
            prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, mem, size, "ArkJS Heap");
        }
#endif
    }

    void AdapterSuitablePoolCapacity();

    MemMapPool memMapPool_;
    MemMapFreeList memMapFreeList_;
    std::atomic_size_t memMapTotalSize_ {0};
    size_t capacity_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MEM_MAP_ALLOCATOR_H
