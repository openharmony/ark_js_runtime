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

#ifndef ECMASCRIPT_JS_METHOD_H
#define ECMASCRIPT_JS_METHOD_H

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_string.h"
#include "libpandafile/file.h"

static constexpr uint32_t CALL_TYPE_MASK = 0xF;  // 0xF: the last 4 bits are used as callType

namespace panda::ecmascript {
class JSPandaFile;
struct PUBLIC_API JSMethod : public base::AlignedStruct<sizeof(uint64_t),
                                                        base::AlignedUint64,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedUint64> {
    enum class Index : size_t {
        CALL_FIELD_INDEX = 0,
        NATIVE_POINTER_OR_BYTECODE_ARRAY_INDEX,
        JS_PANDA_FILE_INDEX,
        LITERAL_INFO_INDEX,
        NUM_OF_MEMBERS
    };

    static_assert(static_cast<size_t>(Index::NUM_OF_MEMBERS) == NumOfTypes);

    static constexpr uint8_t MAX_SLOT_SIZE = 0xFF;

    JSMethod(const JSPandaFile *jsPandaFile, panda_file::File::EntityId fileId);
    JSMethod() = delete;
    ~JSMethod() = default;
    JSMethod(const JSMethod &) = delete;
    JSMethod(JSMethod &&) = delete;
    JSMethod &operator=(const JSMethod &) = delete;
    JSMethod &operator=(JSMethod &&) = delete;

    static size_t GetBytecodeArrayOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::NATIVE_POINTER_OR_BYTECODE_ARRAY_INDEX)>(isArch32);
    }

    const uint8_t *GetBytecodeArray() const
    {
        return reinterpret_cast<const uint8_t *>(nativePointerOrBytecodeArray_);
    }

    void SetBytecodeArray(const uint8_t *bc)
    {
        nativePointerOrBytecodeArray_ = reinterpret_cast<const void *>(bc);
    }

    static constexpr size_t VREGS_ARGS_NUM_BITS = 28; // 28: maximum 268,435,455
    using HaveThisBit = BitField<bool, 0, 1>;  // offset 0
    using HaveNewTargetBit = HaveThisBit::NextFlag;  // offset 1
    using HaveExtraBit = HaveNewTargetBit::NextFlag;  // offset 2
    using HaveFuncBit = HaveExtraBit::NextFlag;  // offset 3
    using NumVregsBits = HaveFuncBit::NextField<uint32_t, VREGS_ARGS_NUM_BITS>;  // offset 4-31
    using NumArgsBits = NumVregsBits::NextField<uint32_t, VREGS_ARGS_NUM_BITS>;  // offset 32-59
    using IsNativeBit = NumArgsBits::NextFlag;  // offset 60
    using IsAotCodeBit = IsNativeBit::NextFlag; // offset 61

    uint64_t GetCallField() const
    {
        return callField_;
    }

    static size_t GetCallFieldOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CALL_FIELD_INDEX)>(isArch32);
    }

    void SetNumArgsWithCallField(uint32_t numargs)
    {
        callField_ = NumArgsBits::Update(callField_, numargs);
    }

    void SetNativeBit(bool isNative)
    {
        callField_ = IsNativeBit::Update(callField_, isNative);
    }

    void SetAotCodeBit(bool isCompiled)
    {
        callField_ = IsAotCodeBit::Update(callField_, isCompiled);
    }

    CString PUBLIC_API ParseFunctionName() const;

    void InitializeCallField(uint32_t numVregs, uint32_t numArgs);

    bool HaveThisWithCallField() const
    {
        return HaveThisBit::Decode(callField_);
    }

    bool HaveNewTargetWithCallField() const
    {
        return HaveNewTargetBit::Decode(callField_);
    }

    bool HaveExtraWithCallField() const
    {
        return HaveExtraBit::Decode(callField_);
    }

    bool HaveFuncWithCallField() const
    {
        return HaveFuncBit::Decode(callField_);
    }

    bool IsNativeWithCallField() const
    {
        return IsNativeBit::Decode(callField_);
    }

    bool OnlyHaveThisWithCallField() const
    {
        return (callField_ & CALL_TYPE_MASK) == 1;  // 1: the first bit of callFiled is HaveThisBit
    }

    bool OnlyHaveNewTagetAndThisWithCallField() const
    {
        return (callField_ & CALL_TYPE_MASK) == 0b11;  // the first two bit of callFiled is `This` and `NewTarget`
    }

    uint32_t GetNumVregsWithCallField() const
    {
        return NumVregsBits::Decode(callField_);
    }

    uint32_t GetNumArgsWithCallField() const
    {
        return NumArgsBits::Decode(callField_);
    }

    uint32_t GetNumArgs() const
    {
        return GetNumArgsWithCallField() + HaveFuncWithCallField() +
            HaveNewTargetWithCallField() + HaveThisWithCallField();
    }

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    const char * PUBLIC_API GetMethodName() const;

    static constexpr size_t METHOD_ARGS_NUM_BYTES = 8;
    static constexpr size_t METHOD_ARGS_NUM_BITS = 16;
    static constexpr size_t METHOD_ARGS_METHODID_BITS = 32;
    using HotnessCounterBits = BitField<int16_t, 0, METHOD_ARGS_NUM_BITS>; // offset 0-15
    using MethodIdBits = HotnessCounterBits::NextField<uint32_t, METHOD_ARGS_METHODID_BITS>; // offset 16-47
    using SlotSizeBits = MethodIdBits::NextField<uint8_t, METHOD_ARGS_NUM_BYTES>; // offset 48-55

    uint32_t GetBytecodeArraySize() const;

    inline int16_t GetHotnessCounter() const
    {
        return HotnessCounterBits::Decode(literalInfo_);
    }

    static size_t GetHotnessCounterOffset(bool isArch32)
    {
        // hotness counter is encoded in a js method field, the first uint16_t in a uint64_t.
        return GetOffset<static_cast<size_t>(Index::LITERAL_INFO_INDEX)>(isArch32);
    }

    inline NO_THREAD_SANITIZE void IncreaseHotnessCounter()
    {
        auto hotnessCounter = HotnessCounterBits::Decode(literalInfo_);
        literalInfo_ = HotnessCounterBits::Update(literalInfo_, ++hotnessCounter);
    }

    NO_THREAD_SANITIZE void ResetHotnessCounter()
    {
        literalInfo_ = HotnessCounterBits::Update(literalInfo_, 0);
    }

    inline NO_THREAD_SANITIZE void SetHotnessCounter(int16_t counter)
    {
        literalInfo_ = HotnessCounterBits::Update(literalInfo_, counter);
    }

    panda_file::File::EntityId GetMethodId() const
    {
        return panda_file::File::EntityId(MethodIdBits::Decode(literalInfo_));
    }

    void SetMethodId(panda_file::File::EntityId methodId)
    {
        literalInfo_ = MethodIdBits::Update(literalInfo_, methodId.GetOffset());
    }

    uint8_t GetSlotSize() const
    {
        return SlotSizeBits::Decode(literalInfo_);;
    }

    void UpdateSlotSize(uint8_t size)
    {
        uint16_t end = GetSlotSize() + size;
        if (end >= MAX_SLOT_SIZE) {
            literalInfo_ = SlotSizeBits::Update(literalInfo_, MAX_SLOT_SIZE);
            return;
        }
        literalInfo_ = SlotSizeBits::Update(literalInfo_, static_cast<uint8_t>(end));
    }

    uint32_t PUBLIC_API GetNumVregs() const;

    uint32_t GetCodeSize() const;

    uint32_t GetCodeSize(panda_file::File::EntityId methodId) const;

    panda_file::File::StringData GetName() const;

    const void* GetNativePointer() const
    {
        return nativePointerOrBytecodeArray_;
    }

    void SetNativePointer(const void *nativePointer)
    {
        nativePointerOrBytecodeArray_ = nativePointer;
    }

    static constexpr size_t GetNativePointerOffset()
    {
        return MEMBER_OFFSET(JSMethod, nativePointerOrBytecodeArray_);
    }

    const panda_file::File *GetPandaFile() const;

    uint64_t GetLiteralInfo() const
    {
        return literalInfo_;
    }

    alignas(EAS) uint64_t callField_ {0};
    // Native method decides this filed is NativePointer or BytecodeArray pointer.
    alignas(EAS) const void *nativePointerOrBytecodeArray_ {nullptr};
    alignas(EAS) const JSPandaFile *jsPandaFile_ {nullptr};
    // hotnessCounter, methodId and slotSize are encoded in literalInfo_.
    alignas(EAS) uint64_t literalInfo_ {0};
};
STATIC_ASSERT_EQ_ARCH(sizeof(JSMethod), JSMethod::SizeArch32, JSMethod::SizeArch64);
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_METHOD_H
