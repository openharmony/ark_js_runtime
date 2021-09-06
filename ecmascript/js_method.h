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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_METHOD_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_METHOD_H

#include "include/method.h"
#include "libpandafile/file.h"

namespace panda {
class Class;
}

namespace panda::ecmascript {
class JSMethod : public Method {
public:
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
    JSMethod(const JSMethod &) = delete;
    JSMethod(JSMethod &&) = delete;
    JSMethod &operator=(const JSMethod &) = delete;
    JSMethod &operator=(JSMethod &&) = delete;

    static constexpr uint32_t GetBytecodeArrayOffset()
    {
        return MEMBER_OFFSET(JSMethod, bytecodeArray_);
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

private:
    const uint8_t *bytecodeArray_ {nullptr};
    uint32_t bytecodeArraySize_ {0};
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_METHOD_H
