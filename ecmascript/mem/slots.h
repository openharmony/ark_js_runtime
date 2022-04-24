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

#ifndef ECMASCRIPT_MEM_SLOTS_H
#define ECMASCRIPT_MEM_SLOTS_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
enum class SlotStatus : bool {
    KEEP_SLOT,
    CLEAR_SLOT,
};

class ObjectSlot {
public:
    explicit ObjectSlot(uintptr_t slotAddr) : slotAddress_(slotAddr) {}
    ~ObjectSlot() = default;

    DEFAULT_COPY_SEMANTIC(ObjectSlot);
    DEFAULT_MOVE_SEMANTIC(ObjectSlot);

    void Update(TaggedObject *header)
    {
        Update(static_cast<JSTaggedType>(ToUintPtr(header)));
    }

    void Update(JSTaggedType value)
    {
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        *reinterpret_cast<JSTaggedType *>(slotAddress_) = value;
    }

    TaggedObject *GetTaggedObjectHeader() const
    {
        return reinterpret_cast<TaggedObject *>(GetTaggedType());
    }

    JSTaggedType GetTaggedType() const
    {
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        return *reinterpret_cast<JSTaggedType *>(slotAddress_);
    }

    ObjectSlot &operator++()
    {
        slotAddress_ += sizeof(JSTaggedType);
        return *this;
    }

    // NOLINTNEXTLINE(cert-dcl21-cpp)
    ObjectSlot operator++(int)
    {
        ObjectSlot ret = *this;
        slotAddress_ += sizeof(JSTaggedType);
        return ret;
    }

    uintptr_t SlotAddress() const
    {
        return slotAddress_;
    }

    bool operator<(const ObjectSlot &other) const
    {
        return slotAddress_ < other.slotAddress_;
    }
    bool operator<=(const ObjectSlot &other) const
    {
        return slotAddress_ <= other.slotAddress_;
    }
    bool operator>(const ObjectSlot &other) const
    {
        return slotAddress_ > other.slotAddress_;
    }
    bool operator>=(const ObjectSlot &other) const
    {
        return slotAddress_ >= other.slotAddress_;
    }
    bool operator==(const ObjectSlot &other) const
    {
        return slotAddress_ == other.slotAddress_;
    }
    bool operator!=(const ObjectSlot &other) const
    {
        return slotAddress_ != other.slotAddress_;
    }

private:
    uintptr_t slotAddress_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SLOTS_H
