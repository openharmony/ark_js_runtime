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

#ifndef RUNTIME_ECMASCRIPT_CHUNK_ALLOCATOR_H
#define RUNTIME_ECMASCRIPT_CHUNK_ALLOCATOR_H

#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
template<typename T>
class ChunkAllocator {
public:
    // used for std allocator
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using const_pointer = const T *;
    using const_reference = const T &;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template<typename U>
    struct Rebind {
        using other = ChunkAllocator<U>;
    };

    template<typename U>
    using rebind = Rebind<U>;

    explicit ChunkAllocator(Chunk *chunk) : chunk_(chunk) {}

    template<typename U>
    ChunkAllocator(const ChunkAllocator<U> &other) : chunk_(other.chunk_)
    {
    }
    template<typename U>
    friend class ChunkAllocator;

    ChunkAllocator(const ChunkAllocator &) = default;
    ChunkAllocator &operator=(const ChunkAllocator &) = default;
    ChunkAllocator(ChunkAllocator &&other) noexcept
    {
        chunk_ = other.chunk_;
        other.chunk_ = nullptr;
    }
    ChunkAllocator &operator=(ChunkAllocator &&other) noexcept
    {
        chunk_ = other.chunk_;
        other.chunk_ = nullptr;
        return *this;
    }
    ~ChunkAllocator() = default;

    // NOLINTNEXTLINE(readability-identifier-naming)
    size_type max_size() const
    {
        return static_cast<size_type>(-1) / sizeof(T);
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    pointer address(reference x) const
    {
        return &x;
    }
    // NOLINTNEXTLINE(readability-identifier-naming)
    const_pointer address(const_reference x) const
    {
        return &x;
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    pointer allocate(size_type n, [[maybe_unused]] const void *ptr = nullptr)
    {
        ASSERT(n <= max_size());
        return chunk_->NewArray<T>(n);
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    void deallocate([[maybe_unused]] pointer p, [[maybe_unused]] size_type n) {}

    template<typename U, typename... Args>
    void construct(U *p, Args &&... args)  // NOLINT(readability-identifier-naming)
    {
        ::new (static_cast<void *>(p)) U(std::forward<Args>(args)...);
    }
    template<typename U>
    void destroy(U *p)  // NOLINT(readability-identifier-naming)
    {
        if (p == nullptr) {
            return;
        }
        p->~U();
    }

    bool operator==(ChunkAllocator const &other) const
    {
        return chunk_ == other.chunk_;
    }
    bool operator!=(ChunkAllocator const &other) const
    {
        return chunk_ != other.chunk_;
    }

    [[nodiscard]] void *Alloc(size_t size)
    {
        return chunk_->NewArray<uint8_t>(size);
    }

    [[nodiscard]] T *AllocArray(size_t size)
    {
        return chunk_->NewArray<T>(size);
    }

    void Delete(T *ptr)
    {
        if (ptr == nullptr) {
            LOG_ECMA_MEM(FATAL) << "free nullptr";
            UNREACHABLE();
        }
        // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
        if constexpr (std::is_class_v<T>) {
            ptr->~T();
        }
        Free(ptr);
    }

    void Free([[maybe_unused]] void *mem) {}

    Chunk *chunk()
    {
        return chunk_;
    }

private:
    Chunk *chunk_;
};
}  // namespace panda::ecmascript

#endif  // RUNTIME_ECMASCRIPT_CHUNK_ALLOCATOR_H
