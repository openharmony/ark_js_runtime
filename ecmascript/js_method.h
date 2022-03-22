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

#include "ecmascript/mem/c_string.h"
#include "include/method.h"
#include "libpandafile/file.h"

namespace panda {
class Class;
}

static constexpr uint32_t CALL_TYPE_MASK = 0xF;  // 0xF: the last 4 bits are used as callType
static constexpr size_t STORAGE_32_NUM = 4;
static constexpr size_t STORAGE_PTR_NUM = 3;

#define JS_METHOD_OFFSET_LIST(V)                                                                    \
    V(STORPTR, STOR32, STORAGE_32_NUM * sizeof(uint32_t), STORAGE_32_NUM * sizeof(uint32_t))        \
    V(PANDAFILE, STORPTR, STORAGE_PTR_NUM * sizeof(uint32_t), STORAGE_PTR_NUM * sizeof(uint64_t))   \
    V(FILEID, PANDAFILE, sizeof(uint32_t), sizeof(uint64_t))                                        \
    V(CODEID, FILEID, sizeof(uint32_t), sizeof(uint32_t))                                           \
    V(SHORTY, CODEID, sizeof(uint32_t), sizeof(uint32_t))                                           \
    V(PROFILINGDATA, SHORTY, sizeof(uint32_t), sizeof(uint64_t))                                    \
    V(CALLFIELD, PROFILINGDATA, sizeof(uint32_t), sizeof(uint64_t))                                 \
    V(BYTECODEARRAY, CALLFIELD, sizeof(uint64_t), sizeof(uint64_t))                                 \
    V(BYTECODEARRAYSIZE, BYTECODEARRAY, sizeof(uint32_t), sizeof(uint64_t))                         \
    V(SLOTSIZE, BYTECODEARRAYSIZE, sizeof(uint32_t), sizeof(uint32_t))                              \

static constexpr uint32_t JS_METHOD_STOR32_OFFSET_32 = 0U;
static constexpr uint32_t JS_METHOD_STOR32_OFFSET_64 = 0U;
#define JS_METHOD_OFFSET_MACRO(name, lastName, lastSize32, lastSize64)                                          \
    static constexpr uint32_t JS_METHOD_##name##_OFFSET_32 = JS_METHOD_##lastName##_OFFSET_32 + (lastSize32);   \
    static constexpr uint32_t JS_METHOD_##name##_OFFSET_64 = JS_METHOD_##lastName##_OFFSET_64 + (lastSize64);
JS_METHOD_OFFSET_LIST(JS_METHOD_OFFSET_MACRO)
#undef JS_METHOD_OFFSET_MACRO

namespace panda::ecmascript {
class JSPandaFile;
class JSMethod : public Method {
public:
    static constexpr uint8_t MAX_SLOT_SIZE = 0xFF;
    static constexpr uint32_t HOTNESS_COUNTER_OFFSET = 3 * sizeof(uint32_t);  // 3: the 3th field of method

    static JSMethod *Cast(Method *method)
    {
        return static_cast<JSMethod *>(method);
    }

    JSMethod(Class *klass, const JSPandaFile *jsPandaFile, panda_file::File::EntityId fileId,
                      panda_file::File::EntityId codeId, uint32_t accessFlags,
                      uint32_t numArgs, const uint16_t *shorty);
    JSMethod() = delete;
    ~JSMethod() = default;
    JSMethod(const JSMethod &) = delete;
    JSMethod(JSMethod &&) = delete;
    JSMethod &operator=(const JSMethod &) = delete;
    JSMethod &operator=(JSMethod &&) = delete;

    static constexpr uint32_t GetBytecodeArrayOffset()
    {
        return MEMBER_OFFSET(JSMethod, bytecodeArray_);
    }

    static constexpr uint32_t GetBytecodeArrayOffset(bool isArm32)
    {
        if (isArm32) {
            return JS_METHOD_BYTECODEARRAY_OFFSET_32;
        }
        return JS_METHOD_BYTECODEARRAY_OFFSET_64;
    }

    const uint8_t *GetBytecodeArray() const
    {
        return bytecodeArray_;
    }

    uint32_t GetBytecodeArraySize() const
    {
        return bytecodeArraySize_;
    }

    void SetBytecodeArray(const uint8_t *bc)
    {
        bytecodeArray_ = bc;
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

    static constexpr size_t VREGS_ARGS_NUM_BITS = 28; // 28: maximum 268,435,455
    using HaveThisBit = BitField<bool, 0, 1>;  // offset 0
    using HaveNewTargetBit = HaveThisBit::NextFlag;  // offset 1
    using HaveExtraBit = HaveNewTargetBit::NextFlag;  // offset 2
    using HaveFuncBit = HaveExtraBit::NextFlag;  // offset 3
    using NumVregsBits = HaveFuncBit::NextField<uint32_t, VREGS_ARGS_NUM_BITS>;  // offset 4-31
    using NumArgsBits = NumVregsBits::NextField<uint32_t, VREGS_ARGS_NUM_BITS>;  // offset 32-59
    using IsNativeBit = NumArgsBits::NextFlag;  // offset 60

    uint64_t GetCallField() const
    {
        return callField_;
    }

    static constexpr uint32_t GetCallFieldOffset(bool isArm32)
    {
        if (isArm32) {
            return JS_METHOD_CALLFIELD_OFFSET_32;
        }
        return JS_METHOD_CALLFIELD_OFFSET_64;
    }

    void SetNativeBit(bool isNative)
    {
        callField_ = IsNativeBit::Update(callField_, isNative);
    }

    CString ParseFunctionName() const;
    void InitializeCallField();

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

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

private:
    uint64_t callField_ {0};
    const JSPandaFile *jsPandaFile_ {nullptr};
    const uint8_t *bytecodeArray_ {nullptr};
    uint32_t bytecodeArraySize_ {0};
    uint8_t slotSize_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_METHOD_H
