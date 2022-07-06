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

#ifndef ECMASCRIPT_MEM_MEM_H
#define ECMASCRIPT_MEM_MEM_H

#include <cstdint>

#ifndef PANDA_TARGET_WINDOWS
#include <sys/mman.h>
#endif

#include "ecmascript/ecma_param_configuration.h"
#include "ecmascript/mem/tagged_object.h"
#include "libpandabase/mem/mem.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, bugprone-lambda-function-name)
#define LOG_ECMA_MEM(level) LOG_GC(level) << __func__ << ":" << __LINE__ << " "

namespace panda::ecmascript {
enum class MemAlignment : uint8_t {
    MEM_ALIGN_OBJECT = 8,
    MEM_ALIGN_REGION = 16,
};

enum class MemAlignmentLog2 : uint8_t {
    MEM_ALIGN_OBJECT_LOG2 = 3,
    MEM_ALIGN_REGION_LOG2 = 4,
};

static constexpr size_t LARGE_POOL_SIZE = 480_MB;
static constexpr size_t MEDIUM_POOL_SIZE = 256_MB;
static constexpr size_t LOW_POOL_SIZE = 64_MB;
static constexpr size_t MIN_MEM_POOL_CAPACITY = 64_MB;
static constexpr size_t PHY_SIZE_MULTIPLE = 4;
static constexpr size_t WORKER_NUM = 7;
static constexpr size_t STANDARD_POOL_SIZE = WORKER_NUM * DEFAULT_WORKER_HEAP_SIZE + DEFAULT_HEAP_SIZE;

static constexpr size_t MIN_OLD_SPACE_LIMIT = 2_MB;

static constexpr size_t REGION_SIZE_LOG2 = 18U;

static constexpr size_t MIN_HEAP_SIZE = 5_MB;

static constexpr size_t DEFAULT_REGION_SIZE = 1U << REGION_SIZE_LOG2;
static constexpr size_t DEFAULT_REGION_MASK = DEFAULT_REGION_SIZE - 1;

static constexpr size_t DEFAULT_MARK_STACK_SIZE = 4_KB;

static constexpr double MIN_OBJECT_SURVIVAL_RATE = 0.75;

// Objects which are larger than half of the region size are huge objects.
// Regular objects will be allocated on regular regions and migrated on spaces.
// They will never be moved to huge object space. So we take half of a regular
// region as the border of regular objects.
static constexpr size_t MAX_32BIT_OBJECT_SPACE_SIZE = 1_GB;
static constexpr size_t MAX_REGULAR_HEAP_OBJECT_SIZE = DEFAULT_REGION_SIZE * 2 / 3;
// internal allocator
static constexpr size_t CHUNK_ALIGN_SIZE = 4_KB;
static constexpr size_t MIN_CHUNK_AREA_SIZE = 4_KB;
static constexpr size_t MAX_CACHED_CHUNK_AREA_SIZE = 16_KB;
static constexpr size_t MAX_CHUNK_AREA_SIZE = 1 * 1024_KB;

using TaggedType = uint64_t;
static constexpr uint32_t TAGGED_TYPE_SIZE = sizeof(TaggedType);
static constexpr uint32_t TAGGED_TYPE_SIZE_LOG = panda::helpers::math::GetIntLog2(TAGGED_TYPE_SIZE);

template<typename T>
constexpr inline bool IsAligned(T value, size_t alignment)
{
    return (value & (alignment - 1U)) == 0;
}

template<typename T>
inline T AlignDown(T x, size_t alignment)
{
    ASSERT(std::is_integral<T>::value);
    // alignment must be a power of two.
    ASSERT(alignment != 0 && ((alignment & (alignment - 1U)) == 0));
    return x & ~(alignment - 1U);
}

template<typename T>
inline T AlignUp(T x, size_t alignment)
{
    ASSERT(std::is_integral<T>::value);
    return AlignDown<T>(static_cast<T>(x + alignment - 1U), alignment);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_MEM_H
