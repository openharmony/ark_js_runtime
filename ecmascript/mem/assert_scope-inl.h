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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_ASSERT_SCOPE_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_ASSERT_SCOPE_INL_H

#include "assert_scope.h"

namespace panda::ecmascript {
// Thread-local storage for assert data. Default all asserts to "allow".
// NOLINTNEXTLINE(hicpp-signed-bitwise)
static thread_local size_t currentAssertData(~0);

template <AssertType type, bool isAllow>
AssertScopeT<type, isAllow, true>::AssertScopeT() : oldData_(currentAssertData)
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

template <AssertType type, bool isAllow>
AssertScopeT<type, isAllow, true>::~AssertScopeT()
{
    if (!oldData_.has_value()) {
        return;
    }

    currentAssertData = oldData_.value();
    oldData_.reset();
}

// static
template <AssertType type, bool isAllow>
bool AssertScopeT<type, isAllow, true>::IsAllowed()
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
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_ASSERT_SCOPE_INL_H