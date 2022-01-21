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

static constexpr uint32_t NORMAL_CALL_TYPE = 0;  // 0: normal (without this, newTarget, extra, func)
static constexpr uint32_t HAVE_THIS_BIT = 1;  // 1: the last bit means this
static constexpr uint32_t HAVE_NEWTARGET_BIT = 2;  // 2: the 2nd to last bit means newTarget
static constexpr uint32_t HAVE_EXTRA_BIT = 4;  // 4: the 3rd to last bit means extra
static constexpr uint32_t HAVE_FUNC_BIT = 8;  // 8: the 4th to last bit means func (for old version, UINT32_MAX)
static constexpr size_t STORAGE_32_NUM = 4;
static constexpr size_t STORAGE_PTR_NUM = 3;


#define JS_METHOD_OFFSET_LIST(V)                                                                    \
    V(STORPTR, STOR32, STORAGE_32_NUM * sizeof(uint32_t), STORAGE_32_NUM * sizeof(uint32_t))        \
    V(PANDAFILE, STORPTR, STORAGE_PTR_NUM * sizeof(uint32_t), STORAGE_PTR_NUM * sizeof(uint64_t))   \
    V(FILEID, PANDAFILE, sizeof(uint32_t), sizeof(uint64_t))                                        \
    V(CODEID, FILEID, sizeof(uint32_t), sizeof(uint32_t))                                           \
    V(SHORTY, CODEID, sizeof(uint32_t), sizeof(uint32_t))                                           \
    V(PROFILINGDATA, SHORTY, sizeof(uint32_t), sizeof(uint64_t))                                    \
    V(BYTECODEARRAY, PROFILINGDATA, sizeof(uint32_t), sizeof(uint64_t))                             \
    V(BYTECODEARRAYSIZE, BYTECODEARRAY, sizeof(uint32_t), sizeof(uint64_t))                         \
    V(SLOTSIZE, BYTECODEARRAYSIZE, sizeof(uint32_t), sizeof(uint32_t))                              \
    V(CALLTYPE, SLOTSIZE, sizeof(uint8_t), sizeof(uint8_t))                                         \

static constexpr uint32_t JS_METHOD_STOR32_OFFSET_32 = 0U;
static constexpr uint32_t JS_METHOD_STOR32_OFFSET_64 = 0U;
#define JS_METHOD_OFFSET_MACRO(name, lastName, lastSize32, lastSize64)                                          \
    static constexpr uint32_t JS_METHOD_##name##_OFFSET_32 = JS_METHOD_##lastName##_OFFSET_32 + (lastSize32);   \
    static constexpr uint32_t JS_METHOD_##name##_OFFSET_64 = JS_METHOD_##lastName##_OFFSET_64 + (lastSize64);
JS_METHOD_OFFSET_LIST(JS_METHOD_OFFSET_MACRO)
#undef JS_METHOD_OFFSET_MACRO

namespace panda::ecmascript {
class JSMethod : public Method {
public:
    static constexpr uint8_t MAX_SLOT_SIZE = 0xFF;
    static constexpr uint32_t HOTNESS_COUNTER_OFFSET = 3 * sizeof(uint32_t);  // 3: the 3th field of method

    static JSMethod *Cast(Method *method)
    {
        return static_cast<JSMethod *>(method);
    }

    explicit JSMethod(Class *klass, const panda_file::File *pf, panda_file::File::EntityId fileId,
                      panda_file::File::EntityId codeId, uint32_t accessFlags, uint32_t numArgs, const uint16_t *shorty)
        : Method(klass, pf, fileId, codeId, accessFlags, numArgs, shorty)
    {
        bytecodeArray_ = JSMethod::GetInstructions();
        bytecodeArraySize_ = JSMethod::GetCodeSize();
    }

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

    uint32_t GetCallType() const
    {
        return callType_;
    }

    CString ParseFunctionName() const;
    void SetCallTypeFromAnnotation();

private:
    const uint8_t *bytecodeArray_ {nullptr};
    uint32_t bytecodeArraySize_ {0};
    uint8_t slotSize_ {0};
    uint32_t callType_ {UINT32_MAX};  // UINT32_MAX means not found
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_METHOD_H
