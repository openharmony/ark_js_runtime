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

#include "stub_module.h"

namespace panda::ecmascript {
void StubModule::Save(const std::string &filename)
{
    if (code_ != nullptr) {
        std::ofstream modulefile(filename.c_str(), std::ofstream::binary);
        /* write stub entries offset  */
        modulefile.write(reinterpret_cast<char *>(fastStubEntries_.data()),
                         sizeof(uintptr_t) * kungfu::FAST_STUB_MAXCOUNT);
        /* write code length & code buff */
        int codeSize = code_->GetInstructionSizeInBytes().GetInt();
        modulefile.write(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));
        modulefile.write(reinterpret_cast<char *>(code_->GetDataOffsetAddress()), codeSize);
        modulefile.close();
    }
}

void StubModule::Load(JSThread *thread, const std::string &filename)
{
    std::ifstream modulefile(filename.c_str(), std::ofstream::binary);
    modulefile.read(reinterpret_cast<char *>(fastStubEntries_.data()), sizeof(uintptr_t) * kungfu::FAST_STUB_MAXCOUNT);
    int codeSize = 0;
    modulefile.read(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto codeHandle = factory->NewMachineCodeObject(codeSize, nullptr);
    modulefile.read(reinterpret_cast<char *>(codeHandle->GetDataOffsetAddress()), codeSize);
    SetCode(*codeHandle);
    modulefile.close();
}
}  // namespace panda::ecmascript