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

#include "libpandabase/mem/mem.h"
#include "ecmascript/mem/tagged_object.h"
#include "libpandabase/utils/logger.h"
#include "mem/gc/bitmap.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, bugprone-lambda-function-name)
#define LOG_ECMA_MEM(type) LOG(type, ECMASCRIPT) << __func__ << " Line:" << __LINE__ << " "

namespace panda::ecmascript {
enum class MemAlignment : uint8_t {
    MEM_ALIGN_OBJECT = 8,
    MEM_ALIGN_REGION = 16,
};

static constexpr size_t DEFAULT_SEMI_SPACE_SIZE = 1024 * 1024;
static constexpr size_t MIN_AllOC_LIMIT_GROWING_STEP = 2 * 1024 * 1024;

static constexpr size_t SEMI_SPACE_CAPACITY = 2 * 1024 * 1024;
static constexpr size_t MAX_SEMI_SPACE_CAPACITY = 16 * 1024 * 1024;
static constexpr size_t SEMI_SPACE_TRIGGER_CONCURRENT_MARK = 1.5 * 1024 * 1024;
static constexpr size_t SEMI_SPACE_OVERSHOOT_SIZE = 2 * 1024 * 1024;

static constexpr size_t OLD_SPACE_LIMIT_BEGIN = 256 * 1024 * 1024;
static constexpr size_t GLOBAL_SPACE_LIMIT_BEGIN = 512 * 1024 * 1024;
static constexpr size_t MIN_GROWING_STEP = 16 * 1024 * 1024;

static constexpr size_t DEFAULT_OLD_SPACE_SIZE = 2 * 1024 * 1024;
static constexpr size_t MAX_OLD_SPACE_SIZE = 512 * 1024 * 1024;

static constexpr size_t DEFAULT_NON_MOVABLE_SPACE_SIZE = 2 * 1024 * 1024;
static constexpr size_t MAX_NON_MOVABLE_SPACE_SIZE = 256 * 1024 * 1024;
static constexpr size_t DEFAULT_NON_MOVABLE_SPACE_LIMIT = 4 * 1024 * 1024;

static constexpr size_t REGION_SIZE_LOG2 = 18U;
static constexpr size_t DEFAULT_SNAPSHOT_SPACE_SIZE = 1U << REGION_SIZE_LOG2;
static constexpr size_t MAX_SNAPSHOT_SPACE_SIZE = 8 * 1024 * 1024;

static constexpr size_t DEFAULT_MACHINE_CODE_SPACE_SIZE = 256 * 1024;
static constexpr size_t DEFAULT_MACHINE_CODE_SPACE_LIMIT = 1024 * 1024;
static constexpr size_t MAX_MACHINE_CODE_SPACE_SIZE = 8 * 1024 * 1024;

static constexpr size_t MAX_HEAP_SIZE = 512 * 1024 * 1024;
static constexpr size_t HALF_MAX_HEAP_SIZE = MAX_HEAP_SIZE / 2;
static constexpr size_t DEFAULT_HEAP_SIZE = 5 * 1024 * 1024;

static constexpr size_t DEFAULT_REGION_SIZE = 1U << REGION_SIZE_LOG2;
static constexpr size_t DEFAULT_REGION_MASK = DEFAULT_REGION_SIZE - 1;

static constexpr size_t DEFAULT_MARK_STACK_SIZE = 4 * 1024;

static constexpr double MIN_OBJECT_SURVIVAL_RATE = 0.75;

// Objects which are larger than half of the region size are huge objects.
// Regular objects will be allocated on regular regions and migrated on spaces.
// They will never be moved to huge object space. So we take half of a regular
// region as the border of regular objects.
static constexpr size_t MAX_32BIT_OBJECT_SPACE_SIZE = 1 * 1024 * 1024 * 1024;
static constexpr size_t MAX_REGULAR_HEAP_OBJECT_SIZE = DEFAULT_REGION_SIZE * 2 / 3;
static constexpr size_t MAX_HUGE_OBJECT_SIZE = 256 * 1024 * 1024;
static constexpr size_t MAX_HUGE_OBJECT_SPACE_SIZE = 256 * 1024 * 1024;
static constexpr size_t LARGE_BITMAP_MIN_SIZE = static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)
                                                << mem::Bitmap::LOG_BITSPERWORD;
// internal allocator
static constexpr size_t CHUNK_ALIGN_SIZE = 4 * 1024;
static constexpr size_t MIN_CHUNK_AREA_SIZE = 4 * 1024;
static constexpr size_t MAX_CACHED_CHUNK_AREA_SIZE = 16 * 1024;
static constexpr size_t MAX_CHUNK_AREA_SIZE = 1 * 1024 * 1024;

static constexpr uintptr_t PANDA_32BITS_HEAP_START_ADDRESS_256 = 256_KB;

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
