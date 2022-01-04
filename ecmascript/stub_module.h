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

#ifndef ECMASCRIPT_STUB_MODULE_H
#define ECMASCRIPT_STUB_MODULE_H

#include "ecmascript/common.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/machine_code.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class PUBLIC_API StubModule {
public:
    uint64_t GetStubEntry(int index)
    {
        return code_->GetDataOffsetAddress() + stubEntries_[index];
    }
    void Save(const std::string &filename);
    void Load(JSThread *thread, const std::string &filename);
    void SetCode(MachineCode *code)
    {
        code_ = code;
    }

    void SetCodePtr(uint64_t codePtr)
    {
        codePtr_ = codePtr;
    }

    uint64_t GetCodePtr()
    {
        return codePtr_;
    }

    void SetStubEntry(int index, uint64_t offset)
    {
        stubEntries_[index] = offset;
    }
    void SetHostCodeSectionAddr(uint64_t addr)
    {
        hostCodeSectionAddr_ = addr;
    }
    uint64_t GetHostCodeSectionAddr() const
    {
        return hostCodeSectionAddr_;
    }
    void SetDeviceCodeSectionAddr(uint64_t addr)
    {
        devicesCodeSectionAddr_ = addr;
    }
    uint64_t GetDeviceCodeSectionAddr() const
    {
        return devicesCodeSectionAddr_;
    }
    JSTaggedValue GetCode()
    {
        return JSTaggedValue(code_);
    }
    void SetStackMapAddr(uint64_t addr)
    {
        stackMapAddr_ = addr;
    }
    uint64_t GetStackMapAddr() const
    {
        return stackMapAddr_;
    }
    void SetStackMapSize(int len)
    {
        stackMapSize_ = len;
    }

    int GetStackMapSize() const
    {
        return stackMapSize_;
    }

private:
    std::array<uint64_t, kungfu::ALL_STUB_MAXCOUNT> stubEntries_ {-1};
    uint64_t hostCodeSectionAddr_  {0};
    uint64_t devicesCodeSectionAddr_ {0};
    MachineCode *code_ {nullptr};
    uint64_t stackMapAddr_ {0};
    uint64_t codePtr_{0};
    int stackMapSize_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUB_MODULE_H
