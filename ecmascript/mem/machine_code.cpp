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
#include "machine_code.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
void AotCodeInfo::SerializeForStub(const std::string &filename)
{
    if (!VerifyFilePath(filename.c_str())) {
        LOG_ECMA(FATAL) << "file path illegal !";
        UNREACHABLE();
    }
    std::ofstream modulefile(filename.c_str(), std::ofstream::binary);
    SetStubNum(stubEntries_.size());
    /* write stub entries offset  */
    modulefile.write(reinterpret_cast<char *>(&stubNum_), sizeof(stubNum_));
    modulefile.write(reinterpret_cast<char *>(stubEntries_.data()), sizeof(StubDes) * stubNum_);
    /* write host code section start addr */
    modulefile.write(reinterpret_cast<char *>(&hostCodeSectionAddr_), sizeof(hostCodeSectionAddr_));
    /* write code length & code buff */
    modulefile.write(reinterpret_cast<char *>(&codeSize_), sizeof(codeSize_));
    modulefile.write(reinterpret_cast<char *>(codePtr_), codeSize_);
    /* write stackmap buff */
    modulefile.write(reinterpret_cast<char *>(&stackMapSize_), sizeof(stackMapSize_));
    modulefile.write(reinterpret_cast<char *>(stackMapAddr_), stackMapSize_);
    modulefile.close();
}

bool AotCodeInfo::DeserializeForStub(JSThread *thread, const std::string &filename)
{
    //  now MachineCode is non movable, code and stackmap sperately is saved to MachineCode
    // by calling NewMachineCodeObject.
    //  then MachineCode will support movable, code is saved to MachineCode and stackmap is saved
    // to different heap which will be freed when stackmap is parsed by EcmaVM is started.
    if (!VerifyFilePath(filename.c_str())) {
        LOG_ECMA(FATAL) << "file path illegal !";
        UNREACHABLE();
    }
    std::ifstream modulefile(filename.c_str(), std::ofstream::binary);
    if (!modulefile.good()) {
        modulefile.close();
        return false;
    }
    modulefile.read(reinterpret_cast<char *>(&stubNum_), sizeof(stubNum_));
    stubEntries_.resize(stubNum_);
    modulefile.read(reinterpret_cast<char *>(stubEntries_.data()), sizeof(StubDes) * stubNum_);
    /* read  host code section start addr  */
    modulefile.read(reinterpret_cast<char *>(&hostCodeSectionAddr_), sizeof(hostCodeSectionAddr_));
    uint32_t codeSize = 0;
    modulefile.read(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto codeHandle = factory->NewMachineCodeObject(codeSize, nullptr);
    modulefile.read(reinterpret_cast<char *>(codeHandle->GetDataOffsetAddress()), codeSize);
    SetCode(*codeHandle);
    SetDeviceCodeSectionAddr(codeHandle->GetDataOffsetAddress());
    /* read stackmap */
    uint32_t stackmapSize;
    modulefile.read(reinterpret_cast<char *>(&stackmapSize), sizeof(stackmapSize));
    SetStackMapSize(stackmapSize);
    std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
    modulefile.read(reinterpret_cast<char *>(stackmapPtr.get()), stackmapSize);
    if (stackmapSize != 0) {
        kungfu::LLVMStackMapParser::GetInstance().CalculateStackMap(std::move(stackmapPtr),
            hostCodeSectionAddr_, devicesCodeSectionAddr_);
    }
    for (size_t i = 0; i < stubEntries_.size(); i++) {
        stubEntries_[i].codeAddr_ += codeHandle->GetDataOffsetAddress();
    }
    modulefile.close();
    return true;
}

void AotCodeInfo::Serialize(const std::string &filename)
{
    if (!VerifyFilePath(filename.c_str())) {
        LOG_ECMA(FATAL) << "file path illegal !";
        UNREACHABLE();
    }
    std::ofstream moduleFile(filename.c_str(), std::ofstream::binary);
    uint32_t funcNum = aotFuncEntryOffsets_.size();
    moduleFile.write(reinterpret_cast<char *>(&funcNum), sizeof(funcNum));
    /* write AOT func entries offset  */
    for (auto &it : aotFuncEntryOffsets_) {
        uint32_t curFuncNameSize = it.first.size();
        moduleFile.write(reinterpret_cast<char *>(&curFuncNameSize), sizeof(curFuncNameSize));
        moduleFile.write(const_cast<char *>(it.first.data()), curFuncNameSize);
        moduleFile.write(reinterpret_cast<char *>(&it.second), sizeof(uint64_t));
    }
    /* write host code section start addr */
    moduleFile.write(reinterpret_cast<char *>(&hostCodeSectionAddr_), sizeof(hostCodeSectionAddr_));
    /* write code length & code buff */
    moduleFile.write(reinterpret_cast<char *>(&codeSize_), sizeof(codeSize_));
    moduleFile.write(reinterpret_cast<char *>(codePtr_), codeSize_);
    /* write stackmap buff */
    moduleFile.write(reinterpret_cast<char *>(&stackMapSize_), sizeof(stackMapSize_));
    moduleFile.write(reinterpret_cast<char *>(stackMapAddr_), stackMapSize_);
    moduleFile.close();
}

bool AotCodeInfo::Deserialize(EcmaVM *vm, const std::string &filename)
{
    if (!VerifyFilePath(filename.c_str())) {
        LOG_ECMA(FATAL) << "file path illegal !";
        UNREACHABLE();
    }
    std::ifstream moduleFile(filename.c_str(), std::ofstream::binary);
    if (!moduleFile.good()) {
        moduleFile.close();
        return false;
    }
    uint32_t funcNum = 0;
    moduleFile.read(reinterpret_cast<char *>(&funcNum), sizeof(funcNum));
    uint32_t curfuncNameSize = 0;
    std::string curFuncName;
    uint64_t curFuncOffset = 0;
    for (uint32_t i = 0; i < funcNum; i++) {
        moduleFile.read(reinterpret_cast<char *>(&curfuncNameSize), sizeof(uint32_t));
        curFuncName.resize(curfuncNameSize);
        moduleFile.read(reinterpret_cast<char *>(curFuncName.data()), curfuncNameSize);
        moduleFile.read(reinterpret_cast<char *>(&curFuncOffset), sizeof(uint64_t));
        aotFuncEntryOffsets_.insert(make_pair(curFuncName, curFuncOffset));
    }
    /* read host code section start addr  */
    moduleFile.read(reinterpret_cast<char *>(&hostCodeSectionAddr_), sizeof(hostCodeSectionAddr_));
    uint32_t codeSize = 0;
    moduleFile.read(reinterpret_cast<char *>(&codeSize), sizeof(codeSize));
    auto factory = vm->GetFactory();
    auto codeHandle = factory->NewMachineCodeObject(codeSize, nullptr);
    moduleFile.read(reinterpret_cast<char *>(codeHandle->GetDataOffsetAddress()), codeSize);
    SetCode(*codeHandle);
    SetDeviceCodeSectionAddr(codeHandle->GetDataOffsetAddress());
    /* read stackmap */
    int stackmapSize;
    moduleFile.read(reinterpret_cast<char *>(&stackmapSize), sizeof(stackmapSize));
    SetStackMapSize(stackmapSize);
    std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
    moduleFile.read(reinterpret_cast<char *>(stackmapPtr.get()), stackmapSize);
    if (stackmapSize != 0) {
        kungfu::LLVMStackMapParser::GetInstance().CalculateStackMap(std::move(stackmapPtr),
            hostCodeSectionAddr_, devicesCodeSectionAddr_);
    }
    for (auto &aotEntry : aotFuncEntryOffsets_) {
        aotEntry.second += codeHandle->GetDataOffsetAddress();
    }
    moduleFile.close();
    return true;
}

void AotCodeInfo::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&code_)));
}

bool AotCodeInfo::VerifyFilePath(const CString &filePath) const
{
#ifndef PANDA_TARGET_WINDOWS
    if (filePath.size() > PATH_MAX) {
        return false;
    }

    CVector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (result == nullptr) {
        return false;
    }
    return true;
#else
    return false;
#endif
}
} // panda::ecmascript