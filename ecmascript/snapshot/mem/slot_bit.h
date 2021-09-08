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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SLOT_BIT_H
#define ECMASCRIPT_SNAPSHOT_MEM_SLOT_BIT_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/snapshot/mem/constants.h"

#include "utils/bit_field.h"

namespace panda::ecmascript {
class SlotBit final {
public:
    ~SlotBit() = default;
    explicit SlotBit() = delete;

    DEFAULT_COPY_SEMANTIC(SlotBit);
    DEFAULT_MOVE_SEMANTIC(SlotBit);

    explicit SlotBit(uint64_t value) : value_(value) {}

    using ObjectIndexBits = BitField<uint32_t, 0, OBJECT_INDEX_BIT_NUMBER>;
    using ObjectToStringBits = ObjectIndexBits::NextField<bool, OBJECT_TO_STRING_FLAG_NUMBER>;
    using ObjectInConstantsIndexBits = ObjectToStringBits::NextField<uint8_t, OBJECT_IN_CONSTANTS_INDEX_NUMBER>;
    using ObjectTypeBits = ObjectInConstantsIndexBits::NextField<uint8_t, OBJECT_TYPE_BIT_NUMBER>;
    using ObjectSizeBits = ObjectTypeBits::NextField<size_t, OBJECT_SIZE_BIT_NUMBER>;
    using ObjectSpecial = ObjectSizeBits::NextField<bool, OBJECT_SPECIAL>;
    using IsReferenceSlotBits = ObjectSpecial::NextField<uint32_t, IS_REFERENCE_SLOT_BIT_NUMBER>;

    void SetReferenceToString(bool flag)
    {
        ObjectToStringBits::Set<uint64_t>(flag, &value_);
    }

    void SetObjectInConstantsIndex(size_t index)
    {
        ObjectInConstantsIndexBits::Set<uint64_t>(index, &value_);
    }

    size_t GetObjectInConstantsIndex() const
    {
        return ObjectInConstantsIndexBits::Decode(value_);
    }

    bool IsReferenceToString() const
    {
        return ObjectToStringBits::Decode(value_);
    }

    void SetObjectType(uint8_t object_type)
    {
        ObjectTypeBits::Set<uint64_t>(object_type, &value_);
    }

    void SetObjectSize(size_t object_size)
    {
        ObjectSizeBits::Set<uint64_t>(object_size, &value_);
    }

    void SetObjectIndex(uint32_t object_index)
    {
        ObjectIndexBits::Set<uint64_t>(object_index, &value_);
    }

    uint64_t GetValue() const
    {
        return value_;
    }

    uint32_t GetObjectIndex() const
    {
        return ObjectIndexBits::Decode(value_);
    }

    uint8_t GetObjectType() const
    {
        return ObjectTypeBits::Decode(value_);
    }

    size_t GetObjectSize() const
    {
        return ObjectSizeBits::Decode(value_);
    }

    bool IsReferenceSlot() const
    {
        return IsReferenceSlotBits::Decode(value_) == 0;
    }

    bool IsSpecial() const
    {
        return ObjectSpecial::Decode(value_);
    }

    void SetObjectSpecial()
    {
        ObjectSpecial::Set<uint64_t>(true, &value_);
    }

    void ClearObjectSpecialFlag()
    {
        ObjectSpecial::Set<uint64_t>(false, &value_);
    }

private:
    uint64_t value_;
};

class SerializeHelper final {
public:
    static uint8_t GetObjectType(TaggedObject *objectHeader);

    static SlotBit AddObjectHeaderToData(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                         std::unordered_map<uint64_t, ecmascript::SlotBit> *data, size_t index = 0);
    static void AddTaggedObjectRangeToData(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                           std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_SLOT_BIT_H
