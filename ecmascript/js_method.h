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
#include "ecmascript/trampoline/asm_defines.h"
#include "libpandafile/file.h"

static constexpr uint32_t CALL_TYPE_MASK = 0xF;  // 0xF: the last 4 bits are used as callType

namespace panda::ecmascript {
class JSPandaFile;
struct PUBLIC_API JSMethod : public base::AlignedStruct<sizeof(uint64_t),
                                                        base::AlignedUint64,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedUint32,
                                                        base::AlignedUint32,
                                                        base::AlignedUint32,
                                                        base::AlignedUint8> {
    enum class Index : size_t {
        CALL_FIELD_INDEX = 0,
        NATIVE_POINTER_OR_BYTECODE_ARRAY_INDEX,
        JS_PANDA_FILE_INDEX,
        BYTECODE_ARRAY_SIZE_INDEX,
        HOTNESS_COUNTER_INDEX,
        METHOD_ID_INDEX,
        SLOT_SIZE_INDEX,
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

    uint32_t GetBytecodeArraySize() const
    {
        return bytecodeArraySize_;
    }

    void SetBytecodeArray(const uint8_t *bc)
    {
        nativePointerOrBytecodeArray_ = reinterpret_cast<const void *>(bc);
    }

    uint8_t GetSlotSize() const
    {
        return slotSize_;
    }

    void UpdateSlotSize (uint8_t size)
    {
        uint16_t end = GetSlotSize() + size;
        if (end >= MAX_SLOT_SIZE) {
            slotSize_ = MAX_SLOT_SIZE;
            return;
        }
        slotSize_ = static_cast<uint8_t>(end);
    }

    static size_t GetHotnessCounterOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::HOTNESS_COUNTER_INDEX)>(isArch32);
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

    inline uint32_t GetHotnessCounter() const
    {
        return hotnessCounter_;
    }

    inline NO_THREAD_SANITIZE void IncrementHotnessCounter()
    {
        ++hotnessCounter_;
    }

    NO_THREAD_SANITIZE void ResetHotnessCounter()
    {
        hotnessCounter_ = 0;
    }

    inline NO_THREAD_SANITIZE void SetHotnessCounter(size_t counter)
    {
        hotnessCounter_ = counter;
    }

    panda_file::File::EntityId GetMethodId() const
    {
        return methodId_;
    }

    uint32_t PUBLIC_API GetNumVregs() const;

    uint32_t GetCodeSize() const;

    panda_file::File::StringData GetName() const;

    const void* GetNativePointer() const
    {
        return nativePointerOrBytecodeArray_;
    }

    void SetNativePointer(const void *nativePointer)
    {
        nativePointerOrBytecodeArray_ = nativePointer;
    }

    const panda_file::File *GetPandaFile() const;

    alignas(EAS) uint64_t callField_ {0};
    // Native method decides this filed is NativePointer or BytecodeArray pointer.
    alignas(EAS) const void *nativePointerOrBytecodeArray_ {nullptr};
    alignas(EAS) const JSPandaFile *jsPandaFile_ {nullptr};
    alignas(EAS) uint32_t bytecodeArraySize_ {0};
    alignas(EAS) uint32_t hotnessCounter_;
    alignas(EAS) panda_file::File::EntityId methodId_;
    alignas(EAS) uint8_t slotSize_ {0};
};
static_assert(MEMBER_OFFSET(JSMethod, callField_) == ASM_JS_METHOD_CALLFIELD_OFFSET);
static_assert(MEMBER_OFFSET(JSMethod, nativePointerOrBytecodeArray_) == ASM_JS_METHOD_BYTECODEARRAY_OFFSET);
static_assert(MEMBER_OFFSET(JSMethod, hotnessCounter_) == ASM_JS_METHOD_HOTNESS_COUNTER_OFFSET);
static_assert(MEMBER_OFFSET(JSMethod, nativePointerOrBytecodeArray_) == ASM_JS_METHOD_NATIVE_POINTER_OFFSET);
STATIC_ASSERT_EQ_ARCH(sizeof(JSMethod), JSMethod::SizeArch32, JSMethod::SizeArch64);
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_METHOD_H
