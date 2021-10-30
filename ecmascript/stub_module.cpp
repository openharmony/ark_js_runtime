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
#include "ecmascript/object_factory.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"

namespace panda::ecmascript {
void StubModule::Save(const std::string &filename)
{
    if (code_ != nullptr) {
        std::ofstream modulefile(filename.c_str(), std::ofstream::binary);
        /* write stub entries offset  */
        modulefile.write(reinterpret_cast<char *>(fastStubEntries_.data()),
                         sizeof(uintptr_t) * (kungfu::FAST_STUB_MAXCOUNT));
        int codeSize = code_->GetInstructionSizeInBytes().GetInt();
        /* write host code section start addr */
        modulefile.write(reinterpret_cast<char *>(&hostCodeSectionAddr_), sizeof(hostCodeSectionAddr_));
        /* write stackmap offset */
        int stackmapOffset = sizeof(uintptr_t) * (kungfu::FAST_STUB_MAXCOUNT) + 2*sizeof(int)
            + codeSize;
        modulefile.write(reinterpret_cast<char *>(&stackmapOffset),
                         sizeof(int));
        /* write code length & code buff */
        modulefile.write(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));
        modulefile.write(reinterpret_cast<char *>(code_->GetDataOffsetAddress()), codeSize);
        /* write stackmap buff */
        int stackmapSize = GetStackMapSize();
#ifndef NDEBUG
        LOG_ECMA(INFO) << "stackmap host addr:" << GetStackMapAddr() << " stackmapSize:" << stackmapSize << std::endl;
#endif
        modulefile.write(reinterpret_cast<char *>(&stackmapSize), sizeof(stackmapSize));
        modulefile.write(reinterpret_cast<char *>(GetStackMapAddr()), stackmapSize);

        uint8_t *ptr = reinterpret_cast<uint8_t *>(GetStackMapAddr());
        kungfu::LLVMStackMapParser::GetInstance().CalculateStackMap(ptr);

        modulefile.close();
    }
}

void StubModule::Load(JSThread *thread, const std::string &filename)
{
    std::ifstream modulefile(filename.c_str(), std::ofstream::binary);
    modulefile.read(reinterpret_cast<char *>(fastStubEntries_.data()),
        sizeof(uintptr_t) * (kungfu::FAST_STUB_MAXCOUNT));
    /* read  host code section start addr  */
    modulefile.read(reinterpret_cast<char *>(&hostCodeSectionAddr_), sizeof(hostCodeSectionAddr_));
    int stackmapOffset;
    modulefile.read(reinterpret_cast<char *>(&stackmapOffset), sizeof(stackmapOffset));
    int codeSize = 0;
    modulefile.read(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto codeHandle = factory->NewMachineCodeObject(codeSize, nullptr);
    modulefile.read(reinterpret_cast<char *>(codeHandle->GetDataOffsetAddress()), codeSize);
    SetCode(*codeHandle);
#ifndef NDEBUG
    LOG_ECMA(INFO) << "codeSection adress for device:" << std::hex << codeHandle->GetDataOffsetAddress() <<
        " codeSize:" << codeSize << std::endl;
#endif
    SetDeviceCodeSectionAddr(codeHandle->GetDataOffsetAddress());
    /* read stackmap */

    int stackmapSize;
    modulefile.read(reinterpret_cast<char *>(&stackmapSize), sizeof(stackmapSize));
    SetStackMapSize(stackmapSize);

    auto dataHandle = factory->NewMachineCodeObject(stackmapSize, nullptr);
    modulefile.read(reinterpret_cast<char *>(dataHandle->GetDataOffsetAddress()), stackmapSize);
    SetData(*dataHandle);
#ifndef NDEBUG
    LOG_ECMA(INFO) << "stackmapSize adress for device:" << std::hex << dataHandle->GetDataOffsetAddress()
        << " stackmapSize:"<< stackmapSize << std::endl;
#endif
    SetStackMapAddr(reinterpret_cast<Address>(dataHandle->GetDataOffsetAddress()));
    modulefile.close();
}
}  // namespace panda::ecmascript