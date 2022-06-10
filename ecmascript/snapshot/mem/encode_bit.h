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
 *     |0000...000|      |0000...000|      |0|       |0|     |0000000|     |0|      |0000...000|       |00...0|
 *   16bit:is reference  10bit:native or  builtins  special   obj type    string   18bit:obj offset  10bit:region index
 *                        global index
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
    static constexpr int OBJECT_TYPE_BIT_NUMBER = 7;           // js_type
    static constexpr int OBJECT_SPECIAL = 1;                   // special
    static constexpr int GLOBAL_ENV_CONST = 1;                 // global object which has initialized before snapshot
    static constexpr int NATIVE_OR_GLOBAL_INDEX_NUMBER = 10;   // native pointer or global object index
    static constexpr int IS_REFERENCE_BIT_NUMBER = 16;         // [0x0000] is reference

    using RegionIndexBits = BitField<size_t, 0, REGION_INDEX_BIT_NUMBER>;
    using ObjectOffsetInRegionBits = RegionIndexBits::NextField<size_t, OBJECT_OFFSET_IN_REGION_NUMBER>;
    using ObjectToStringBits = ObjectOffsetInRegionBits::NextField<bool, OBJECT_TO_STRING_FLAG_NUMBER>;
    using ObjectTypeBits = ObjectToStringBits::NextField<size_t, OBJECT_TYPE_BIT_NUMBER>;
    using ObjectSpecialBits = ObjectTypeBits::NextField<bool, OBJECT_SPECIAL>;
    using GlobalEnvConstBits = ObjectSpecialBits::NextField<bool, GLOBAL_ENV_CONST>;
    using NativeOrGlobalIndexBits = GlobalEnvConstBits::NextField<size_t, NATIVE_OR_GLOBAL_INDEX_NUMBER>;
    using IsReferenceBits = NativeOrGlobalIndexBits::NextField<size_t, IS_REFERENCE_BIT_NUMBER>;

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

    void SetNativeOrGlobalIndex(size_t native_or_global_index)
    {
        NativeOrGlobalIndexBits::Set<uint64_t>(native_or_global_index, &value_);
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

    size_t GetStringIndex() const
    {
        return (ObjectOffsetInRegionBits::Decode(value_) << REGION_INDEX_BIT_NUMBER) + RegionIndexBits::Decode(value_);
    }

    size_t GetObjectType() const
    {
        return ObjectTypeBits::Decode(value_);
    }

    size_t GetNativeOrGlobalIndex() const
    {
        return NativeOrGlobalIndexBits::Decode(value_);
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

    bool IsGlobalEnvConst() const
    {
        return GlobalEnvConstBits::Decode(value_);
    }

    void SetGlobalEnvConst()
    {
        GlobalEnvConstBits::Set<uint64_t>(true, &value_);
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
              EncodeBit::GLOBAL_ENV_CONST + EncodeBit::NATIVE_OR_GLOBAL_INDEX_NUMBER +
              EncodeBit::IS_REFERENCE_BIT_NUMBER == Constants::UINT_64_BITS_COUNT);
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_ENCODE_BIT_H
