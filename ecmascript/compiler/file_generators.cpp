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
#include "file_generators.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "llvm_ir_builder.h"

namespace panda::ecmascript::kungfu {
void StubFileGenerator::CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name)
{
    uintptr_t codeBegin = asmModule_.GetCodeBufferOffset();
    auto asmCallSigns = asmModule_.GetCSigns();
    uint32_t funSize = 0;
    for (size_t i = 0; i < asmModule_.GetFunctionCount(); i++) {
        auto cs = asmCallSigns[i];
        auto entryOffset = asmModule_.GetFunction(cs->GetID());
        if (i < asmModule_.GetFunctionCount() - 1) {
            auto nextcs = asmCallSigns[i + 1];
            funSize = asmModule_.GetFunction(nextcs->GetID()) - entryOffset;
        } else {
            funSize = asmModule_.GetBufferSize() - entryOffset;
        }
        stubInfo_.AddStubEntry(cs->GetTargetKind(), cs->GetID(), entryOffset + codeBegin, 0, 0, funSize);
        ASSERT(!cs->GetName().empty());
        auto codeBuffer = modulePackage_[0].GetCodeBuffer();
        uintptr_t entry = codeBuffer + entryOffset + codeBegin;
        addr2name[entry] = cs->GetName();
    }
}

void StubFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, stubInfo_, i, GetLog());
        if (i == 0) {
            CollectAsmStubCodeInfo(addr2name);
        }
        auto des = modulePackage_[i].GetModuleSectionDes();
        stubInfo_.AddModuleDes(des);
    }
    DisassembleEachFunc(addr2name);
}

void AOTFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, aotInfo_, i, GetLog());
        auto des = modulePackage_[i].GetModuleSectionDes();
        aotInfo_.AddModuleDes(des, aotfileHashs_[i]);
    }
#ifndef NDEBUG
    DisassembleEachFunc(addr2name);
#endif
}

void StubFileGenerator::RunAsmAssembler()
{
    NativeAreaAllocator allocator;
    Chunk chunk(&allocator);
    asmModule_.Run(modulePackage_[0].GetCompilationConfig(), &chunk);

    auto buffer = asmModule_.GetBuffer();
    auto bufferSize = asmModule_.GetBufferSize();
    if (bufferSize == 0U) {
        return;
    }
    auto currentOffset = modulePackage_[0].GetCodeSize();
    auto codeBuffer = modulePackage_[0].AllocaCodeSection(bufferSize, "asm code");
    if (codeBuffer == nullptr) {
        LOG_FULL(FATAL) << "AllocaCodeSection failed";
        return;
    }
    if (memcpy_s(codeBuffer, bufferSize, buffer, bufferSize) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        return;
    }
    asmModule_.SetCodeBufferOffset(currentOffset);
}

void StubFileGenerator::SaveStubFile(const std::string &filename)
{
    RunLLVMAssembler();
    RunAsmAssembler();
    CollectCodeInfo();
    stubInfo_.Save(filename);
}

void AOTFileGenerator::SaveAOTFile(const std::string &filename)
{
    RunLLVMAssembler();
    CollectCodeInfo();
    aotInfo_.Save(filename);
    DestoryModule();
}

void AOTFileGenerator::GenerateSnapshotFile()
{
    TSLoader *tsLoader = vm_->GetTSLoader();
    Snapshot snapshot(vm_);
    CVector<JSTaggedType> constStringTable = tsLoader->GetConstStringTable();
    CVector<JSTaggedType> staticHClassTable = tsLoader->GetStaticHClassTable();
    CVector<JSTaggedType> tsLoaderSerializeTable(constStringTable);
    tsLoaderSerializeTable.insert(tsLoaderSerializeTable.end(), staticHClassTable.begin(), staticHClassTable.end());
    const CString snapshotPath(vm_->GetJSOptions().GetSnapshotOutputFile().c_str());
    snapshot.Serialize(reinterpret_cast<uintptr_t>(tsLoaderSerializeTable.data()), tsLoaderSerializeTable.size(),
                       snapshotPath);
}
}  // namespace panda::ecmascript::kungfu
