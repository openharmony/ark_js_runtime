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

#ifndef ECMASCRIPT_MEM_ASSERT_SCOPE_H
#define ECMASCRIPT_MEM_ASSERT_SCOPE_H

#include <optional>

#include "libpandabase/macros.h"
#include "utils/bit_field.h"

namespace panda::ecmascript {
static thread_local size_t currentAssertData(~0);

using AssertGarbageCollectBit = panda::BitField<bool, 0, 1>;
using AssertHeapAllocBit = AssertGarbageCollectBit::NextFlag;

#ifndef NDEBUG
constexpr bool IS_ALLOW_CHECK = true;
#else
constexpr bool IS_ALLOW_CHECK = false;
#endif

enum class AssertType : uint8_t { GARBAGE_COLLECTION_ASSERT = 0, HEAP_ALLOC_ASSERT, LAST_ASSERT_TYPE };

template<AssertType type, bool isAllow, bool IsDebug = IS_ALLOW_CHECK>
class AssertScopeT {
public:
    static bool IsAllowed()
    {
        return true;
    }
};

template<AssertType type, bool isAllow>
class AssertScopeT<type, isAllow, true> {
public:
    AssertScopeT() : oldData_(currentAssertData)
    {
        switch (type) {
            case AssertType::GARBAGE_COLLECTION_ASSERT:
                currentAssertData = AssertGarbageCollectBit::Update(oldData_.value(), isAllow);
                break;
            case AssertType::HEAP_ALLOC_ASSERT:
                currentAssertData = AssertHeapAllocBit::Update(oldData_.value(), isAllow);
                break;
            default:
                break;
        }
    }

    ~AssertScopeT()
    {
        if (!oldData_.has_value()) {
            return;
        }

        currentAssertData = oldData_.value();
        oldData_.reset();
    }

    static bool IsAllowed()
    {
        switch (type) {
            case AssertType::GARBAGE_COLLECTION_ASSERT:
                return AssertGarbageCollectBit::Decode(currentAssertData);
            case AssertType::HEAP_ALLOC_ASSERT:
                return AssertHeapAllocBit::Decode(currentAssertData);
            default:
                return true;
        }
    }

    NO_COPY_SEMANTIC(AssertScopeT);
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(AssertScopeT);

private:
    std::optional<size_t> oldData_;
};

using DisallowGarbageCollection = AssertScopeT<AssertType::GARBAGE_COLLECTION_ASSERT, false, IS_ALLOW_CHECK>;
using AllowGarbageCollection = AssertScopeT<AssertType::GARBAGE_COLLECTION_ASSERT, true, IS_ALLOW_CHECK>;
using DisAllowHeapAlloc = AssertScopeT<AssertType::HEAP_ALLOC_ASSERT, false, IS_ALLOW_CHECK>;
using AllowHeapAlloc = AssertScopeT<AssertType::HEAP_ALLOC_ASSERT, true, IS_ALLOW_CHECK>;

#if (!defined NDEBUG) || (defined RUN_TEST)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_GARBAGE_COLLECTION [[maybe_unused]] DisallowGarbageCollection no_gc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_GARBAGE_COLLECTION [[maybe_unused]] AllowGarbageCollection allow_gc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISALLOW_HEAP_ALLOC [[maybe_unused]] DisAllowHeapAlloc no_heap_alloc
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ALLOW_HEAP_ALLOC [[maybe_unused]] AllowHeapAlloc allow_heap_alloc
#else
#define DISALLOW_GARBAGE_COLLECTION
#define ALLOW_GARBAGE_COLLECTION
#define DISALLOW_HEAP_ALLOC
#define ALLOW_HEAP_ALLOC
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_GC ASSERT_PRINT(AllowGarbageCollection::IsAllowed(), "disallow execute garbage collection.");

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_NO_HEAP_ALLOC (AllowHeapAlloc::IsAllowed(), "disallow execute heap alloc.");
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_ASSERT_SCOPE_H
