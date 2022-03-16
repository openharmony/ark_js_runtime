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
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/machine_code.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
using panda::ecmascript::kungfu::CallSignature;
class PUBLIC_API StubModule {
public:
    struct StubDes {
        CallSignature::TargetKind kind_;
        uint32_t indexInKind_;
        uint64_t codeAddr_;
        bool IsStub() const
        {
            return kungfu::CallSignature::TargetKind::STUB_BEGIN <= kind_ &&
                kind_ < kungfu::CallSignature::TargetKind::STUB_END;
        }

        bool IsBCHandler() const
        {
            return kungfu::CallSignature::TargetKind::BCHANDLER_BEGIN <= kind_ &&
                kind_ < kungfu::CallSignature::TargetKind::BCHANDLER_END;
        }

        bool IsCommonStub() const
        {
            return (kind_ == kungfu::CallSignature::TargetKind::COMMON_STUB);
        }
    };

    const StubDes& GetStubDes(int index) const
    {
        return stubEntries_[index];
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

    uint64_t GetStubNum() const
    {
        return stubNum_;
    }

    void SetStubNum(uint64_t n)
    {
        stubNum_ = n;
    }

    void AddStubEntry(CallSignature::TargetKind kind, int indexInKind, uint64_t offset)
    {
        StubDes des;
        des.kind_ = kind;
        des.indexInKind_ = indexInKind;
        des.codeAddr_ = offset;
        stubEntries_.emplace_back(des);
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
    const std::vector<StubDes>& GetStubs() const
    {
        return stubEntries_;
    }

private:
    uint64_t stubNum_ {0};
    std::vector<StubDes> stubEntries_ {};
    uint64_t hostCodeSectionAddr_ {0};
    uint64_t devicesCodeSectionAddr_ {0};
    MachineCode *code_ {nullptr};
    uint64_t stackMapAddr_ {0};
    uint64_t codePtr_ {0};
    int stackMapSize_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUB_MODULE_H
