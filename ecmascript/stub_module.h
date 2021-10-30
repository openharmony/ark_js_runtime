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
using Address = uintptr_t;
class PUBLIC_API StubModule {
public:
    Address GetStubEntry(int index)
    {
        return code_->GetDataOffsetAddress() + fastStubEntries_[index];
    }
    void Save(const std::string &filename);
    void Load(JSThread *thread, const std::string &filename);
    void SetCode(MachineCode *code)
    {
        code_ = code;
    }
    void SetData(MachineCode *data)
    {
        data_ = data;
    }
    void SetStubEntry(int index, Address offset)
    {
        fastStubEntries_[index] = offset;
    }
    void SetHostCodeSectionAddr(Address addr)
    {
        hostCodeSectionAddr_ = addr;
    }
    Address GetHostCodeSectionAddr()
    {
        return hostCodeSectionAddr_;
    }
    void SetDeviceCodeSectionAddr(uintptr_t addr)
    {
        devicesCodeSectionAddr_ = addr;
    }
    uintptr_t GetDeviceCodeSectionAddr() const
    {
        return devicesCodeSectionAddr_;
    }
    JSTaggedValue GetCode()
    {
        return JSTaggedValue(code_);
    }
    JSTaggedValue GetData()
    {
        return JSTaggedValue(data_);
    }
    void SetStackMapAddr(Address addr)
    {
        stackMapAddr_ = addr;
    }
    Address GetStackMapAddr()
    {
        return stackMapAddr_;
    }
    void SetStackMapSize(int len)
    {
        stackMapSize_ = len;
    }
    int GetStackMapSize()
    {
        return stackMapSize_;
    }

private:
    std::array<Address, kungfu::FAST_STUB_MAXCOUNT> fastStubEntries_ {-1};
    Address hostCodeSectionAddr_ = 0;
    uintptr_t devicesCodeSectionAddr_ = 0;
    MachineCode *code_ {nullptr};
    MachineCode *data_ {nullptr};
    Address stackMapAddr_  = 0;
    int stackMapSize_ = 0;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUB_MODULE_H