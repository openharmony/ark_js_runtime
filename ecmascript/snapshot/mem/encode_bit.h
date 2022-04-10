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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_ENCODE_BIT_H
#define ECMASCRIPT_SNAPSHOT_MEM_ENCODE_BIT_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/snapshot/mem/constants.h"

#include "utils/bit_field.h"

/*
 *                        EncodeBit: use uint64_t value to encode TaggedObject when serialize
 *
 *     | 0000...000 |     | 0 |     | 0000...000 |     | 00000000 |     | 0 |      | 0000...000 |       | 00...0 |
 *   16bit:is reference  special   10bit:native pointer  obj type       string   18bit:obj offset    10bit:region index
 *                                       index
 */

namespace panda::ecmascript {
class EncodeBit final {
public:
    ~EncodeBit() = default;
    explicit EncodeBit() = delete;

    DEFAULT_COPY_SEMANTIC(EncodeBit);
    DEFAULT_MOVE_SEMANTIC(EncodeBit);

    explicit EncodeBit(uint64_t value) : value_(value) {}

    using RegionIndexBits = BitField<uint16_t, 0, Constants::REGION_INDEX_BIT_NUMBER>;
    using ObjectOffsetInRegionBits = RegionIndexBits::NextField<uint32_t, Constants::OBJECT_OFFSET_IN_REGION_NUMBER>;
    using ObjectToStringBits = ObjectOffsetInRegionBits::NextField<bool, Constants::OBJECT_TO_STRING_FLAG_NUMBER>;
    using ObjectTypeBits = ObjectToStringBits::NextField<uint8_t, Constants::OBJECT_TYPE_BIT_NUMBER>;
    using ObjectSpecialBits = ObjectTypeBits::NextField<bool, Constants::OBJECT_SPECIAL>;
    using NativePointerIndexBits = ObjectSpecialBits::NextField<uint16_t, Constants::NATIVE_POINTER_INDEX_BIT_NUMBER>;
    using IsReferenceBits = NativePointerIndexBits::NextField<uint32_t, Constants::IS_REFERENCE_BIT_NUMBER>;

    void SetRegionIndex(uint16_t region_index)
    {
        RegionIndexBits::Set<uint64_t>(region_index, &value_);
    }

    void SetObjectOffsetInRegion(uint32_t object_offset)
    {
        ObjectOffsetInRegionBits::Set<uint64_t>(object_offset, &value_);
    }

    void SetReferenceToString(bool flag)
    {
        ObjectToStringBits::Set<uint64_t>(flag, &value_);
    }

    bool IsReferenceToString() const
    {
        return ObjectToStringBits::Decode(value_);
    }

    void SetObjectType(uint8_t object_type)
    {
        ObjectTypeBits::Set<uint64_t>(object_type, &value_);
    }

    void SetNativePointerIndex(uint16_t native_pointer_index)
    {
        NativePointerIndexBits::Set<uint64_t>(native_pointer_index, &value_);
    }

    uint64_t GetValue() const
    {
        return value_;
    }

    uint16_t GetRegionIndex() const
    {
        return RegionIndexBits::Decode(value_);
    }

    uint32_t GetObjectOffsetInRegion() const
    {
        return ObjectOffsetInRegionBits::Decode(value_);
    }

    uint8_t GetObjectType() const
    {
        return ObjectTypeBits::Decode(value_);
    }

    uint16_t GetNativePointerIndex() const
    {
        return NativePointerIndexBits::Decode(value_);
    }

    bool IsReference() const
    {
        return IsReferenceBits::Decode(value_) == 0;
    }

    bool IsSpecial() const
    {
        return ObjectSpecialBits::Decode(value_);
    }

    void SetObjectSpecial()
    {
        ObjectSpecialBits::Set<uint64_t>(true, &value_);
    }

    void ClearObjectSpecialFlag()
    {
        ObjectSpecialBits::Set<uint64_t>(false, &value_);
    }

private:
    uint64_t value_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_ENCODE_BIT_H
