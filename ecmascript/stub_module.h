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
    void SetCode(MachineCode *code)\
    {
        code_ = code;
    }
    void SetStubEntry(int index, Address offset)
    {
        fastStubEntries_[index] = offset;
    }
    JSTaggedValue GetCode()
    {
        return JSTaggedValue(code_);
    }

private:
    std::array<Address, kungfu::FAST_STUB_MAXCOUNT> fastStubEntries_ {-1};
    MachineCode *code_ {nullptr};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUB_MODULE_H