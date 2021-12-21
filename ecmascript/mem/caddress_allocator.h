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

#ifndef RUNTIME_ECMASCRIPT_C_ADDRESS_ALLOCATOR_H
#define RUNTIME_ECMASCRIPT_C_ADDRESS_ALLOCATOR_H

#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
template<typename T>
class CAddressAllocator {
public:
    // using by std allocator
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using const_pointer = const T *;
    using const_reference = const T &;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template<typename U>
    struct Rebind {
        using other = CAddressAllocator<U>;
    };

    template<typename U>
    using rebind = Rebind<U>;

    CAddressAllocator() = default;

    template<typename U>
    explicit CAddressAllocator(const CAddressAllocator<U> &other [[maybe_unused]])
    {
    }

    CAddressAllocator(const CAddressAllocator &) = default;
    CAddressAllocator &operator=(const CAddressAllocator &) = default;
    CAddressAllocator(CAddressAllocator &&other) noexcept = default;
    CAddressAllocator &operator=(CAddressAllocator &&other) noexcept = default;
    ~CAddressAllocator() = default;

    // NOLINTNEXTLINE(readability-identifier-naming)
    size_type max_size() const
    {
        return static_cast<size_type>(-1) / sizeof(T);
    }

    bool operator==([[maybe_unused]] CAddressAllocator const &other) const
    {
        return false;
    }

    bool operator!=([[maybe_unused]] CAddressAllocator const &other) const
    {
        return true;
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    pointer allocate(size_type n, [[maybe_unused]] const void *ptr = nullptr)
    {
        ASSERT(n <= max_size());
        return static_cast<T *>(Allocate(n * sizeof(T)));
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    void deallocate([[maybe_unused]] pointer p, [[maybe_unused]] size_type n)
    {
        Free(static_cast<void *>(p));
    }

    template<typename U, typename... Args>
    void construct(U *p, Args &&... args)  // NOLINT(readability-identifier-naming)
    {
        if (p == nullptr) {
            return;
        }
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

    [[nodiscard]] void *Allocate(size_t size)
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
        return ptr;
    }

    template<typename S, typename... Args>
    [[nodiscard]] S *New(Args &&... args)
    {
        auto p = reinterpret_cast<void *>(Allocate(sizeof(S)));
        new (p) S(std::forward<Args>(args)...);
        return reinterpret_cast<S *>(p);
    }

    template<class S>
    void Finalize(S *ptr)
    {
        ASSERT(ptr != nullptr);
        // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
        if constexpr (std::is_class_v<S>) {
            ptr->~S();
        }
        Free(ptr);
    }

    [[nodiscard]] T *AllocArray(size_t size)
    {
        return static_cast<T *>(Allocate(size * sizeof(T)));
    }

    void Delete(T *ptr)
    {
        if (ptr == nullptr) {
            return;
        }
        // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
        if constexpr (std::is_class_v<T>) {
            ptr->~T();
        }
        Free(ptr);
    }

    void Free(void *mem)
    {
        if (mem == nullptr) {
            return;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        free(mem);
    }
};
}  // namespace panda::ecmascript

#endif  // RUNTIME_ECMASCRIPT_C_ADDRESS_ALLOCATOR_H