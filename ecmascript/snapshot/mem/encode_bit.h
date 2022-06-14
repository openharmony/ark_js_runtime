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
 *     |0000...000|      |0000...00|      |0|       |0|     |00000000|     |0|      |0000...000|       |00...0|
 *   16bit:is reference  9bit:unused   builtins  special     obj type     string   18bit:obj offset  10bit:region index
 *
 */

namespace panda::ecmascript {
class EncodeBit final {
public:
    ~EncodeBit() = default;
    explicit EncodeBit() = delete;

    DEFAULT_COPY_SEMANTIC(EncodeBit);
    DEFAULT_MOVE_SEMANTIC(EncodeBit);

    explicit EncodeBit(uint64_t value) : value_(value) {}

    // encode bit
    static constexpr int REGION_INDEX_BIT_NUMBER = 10;         // region index
    static constexpr int OBJECT_OFFSET_IN_REGION_NUMBER = 18;  // object offset in current region
    static constexpr int OBJECT_TO_STRING_FLAG_NUMBER = 1;     // 1 : reference to string
    static constexpr int OBJECT_TYPE_BIT_NUMBER = 8;           // js_type
    static constexpr int OBJECT_SPECIAL = 1;                   // special
    static constexpr int GLOBAL_CONST_OR_BUILTINS = 1;         // is global const or builtins object
    static constexpr int UNUSED_BIT_NUMBER = 9;                // unused bit number
    static constexpr int IS_REFERENCE_BIT_NUMBER = 16;         // [0x0000] is reference

    using RegionIndexBits = BitField<size_t, 0, REGION_INDEX_BIT_NUMBER>;
    using ObjectOffsetInRegionBits = RegionIndexBits::NextField<size_t, OBJECT_OFFSET_IN_REGION_NUMBER>;
    using ObjectToStringBits = ObjectOffsetInRegionBits::NextField<bool, OBJECT_TO_STRING_FLAG_NUMBER>;
    using ObjectTypeBits = ObjectToStringBits::NextField<size_t, OBJECT_TYPE_BIT_NUMBER>;
    using ObjectSpecialBits = ObjectTypeBits::NextField<bool, OBJECT_SPECIAL>;
    using GlobalConstOrBuiltinsBits = ObjectSpecialBits::NextField<bool, GLOBAL_CONST_OR_BUILTINS>;
    using UnusedBits = GlobalConstOrBuiltinsBits::NextField<size_t, UNUSED_BIT_NUMBER>;
    using IsReferenceBits = UnusedBits::NextField<size_t, IS_REFERENCE_BIT_NUMBER>;

    void SetRegionIndex(size_t region_index)
    {
        RegionIndexBits::Set<uint64_t>(region_index, &value_);
    }

    void SetObjectOffsetInRegion(size_t object_offset)
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

    void SetObjectType(size_t object_type)
    {
        ObjectTypeBits::Set<uint64_t>(object_type, &value_);
    }

    uint64_t GetValue() const
    {
        return value_;
    }

    size_t GetRegionIndex() const
    {
        return RegionIndexBits::Decode(value_);
    }

    size_t GetObjectOffsetInRegion() const
    {
        return ObjectOffsetInRegionBits::Decode(value_);
    }

    size_t GetNativePointerOrObjectIndex() const
    {
        return (ObjectOffsetInRegionBits::Decode(value_) << REGION_INDEX_BIT_NUMBER) + RegionIndexBits::Decode(value_);
    }

    size_t GetObjectType() const
    {
        return ObjectTypeBits::Decode(value_);
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

    bool IsGlobalConstOrBuiltins() const
    {
        return GlobalConstOrBuiltinsBits::Decode(value_);
    }

    void SetGlobalConstOrBuiltins()
    {
        GlobalConstOrBuiltinsBits::Set<uint64_t>(true, &value_);
    }

    // low 28 bits are used to record object location(region index and object offset), besides, it's
    // used to record string index, global const and builtins object index, native pointer index
    void SetNativePointerOrObjectIndex(size_t index)
    {
        ASSERT(index < Constants::MAX_OBJECT_INDEX);
        ObjectOffsetInRegionBits::Set<uint64_t>(index >> REGION_INDEX_BIT_NUMBER, &value_);
        RegionIndexBits::Set<uint64_t>(index & Constants::REGION_INDEX_MASK, &value_);
    }

    void ClearObjectSpecialFlag()
    {
        ObjectSpecialBits::Set<uint64_t>(false, &value_);
    }

private:
    uint64_t value_;
};
static_assert(EncodeBit::REGION_INDEX_BIT_NUMBER + EncodeBit::OBJECT_OFFSET_IN_REGION_NUMBER +
              EncodeBit::OBJECT_TO_STRING_FLAG_NUMBER + EncodeBit::OBJECT_TYPE_BIT_NUMBER + EncodeBit::OBJECT_SPECIAL +
              EncodeBit::GLOBAL_CONST_OR_BUILTINS + EncodeBit::UNUSED_BIT_NUMBER +
              EncodeBit::IS_REFERENCE_BIT_NUMBER == Constants::UINT_64_BITS_COUNT);
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_ENCODE_BIT_H
