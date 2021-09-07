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

#ifndef ECMASCRIPT_MEM_MARK_WORD_H
#define ECMASCRIPT_MEM_MARK_WORD_H

#include <cstdint>
#include <atomic>

#include "utils/bit_field.h"
#include "libpandabase/mem/mem.h"

namespace panda {
namespace ecmascript {
class TaggedObject;
class JSHClass;

using MarkWordType = uint64_t;

class MarkWord {
public:
    // ForwardingAddress mark, this object has been moved and the address points to the newly allocated space.
    static constexpr MarkWordType TAG_MARK_BIT = 0x02ULL;

    explicit MarkWord(TaggedObject *header)
    {
        value_ = reinterpret_cast<volatile std::atomic<MarkWordType> *>(header)->load(std::memory_order_acquire);
    }
    ~MarkWord() = default;
    NO_COPY_SEMANTIC(MarkWord);
    NO_MOVE_SEMANTIC(MarkWord);

    bool IsForwardingAddress()
    {
        return (value_ & TAG_MARK_BIT) != 0;
    }

    TaggedObject *ToForwardingAddress()
    {
        return reinterpret_cast<TaggedObject *>(value_ & (~TAG_MARK_BIT));
    }

    static MarkWordType FromForwardingAddress(MarkWordType forwardAddress)
    {
        return forwardAddress | TAG_MARK_BIT;
    }

    MarkWordType GetValue() const
    {
        return value_;
    }

    JSHClass *GetJSHClass() const
    {
        return reinterpret_cast<JSHClass *>(value_ & (~TAG_MARK_BIT));
    }

private:
    MarkWordType value_{0};
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_MARK_WORD_H
