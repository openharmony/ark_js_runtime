/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "ecmascript/file_loader.h"

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/snapshot/mem/snapshot.h"

namespace panda::ecmascript {
void StubModulePackInfo::Save(const std::string &filename)
{
    if (!VerifyFilePath(filename, true)) {
        return;
    }

    std::ofstream moduleFile(filename.c_str(), std::ofstream::binary);
    SetStubNum(entries_.size());
    /* write stub entries offset  */
    moduleFile.write(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    moduleFile.write(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    uint32_t moduleNum = GetCodeUnitsNum();
    moduleFile.write(reinterpret_cast<char *>(&moduleNum), sizeof(moduleNum_));
    moduleFile.write(reinterpret_cast<char *>(&totalCodeSize_), sizeof(totalCodeSize_));
    for (size_t i = 0; i < moduleNum; i++) {
        moduleFile.write(reinterpret_cast<char *>(&(des_[i].hostCodeSectionAddr_)),
            sizeof(des_[i].hostCodeSectionAddr_));
        moduleFile.write(reinterpret_cast<char *>(&(des_[i].codeSize_)),
            sizeof(des_[i].codeSize_));
        moduleFile.write(reinterpret_cast<char *>(des_[i].GetHostCodeSecAddr()),
            des_[i].GetCodeSize());
        moduleFile.write(reinterpret_cast<char *>(&(des_[i].stackMapSize_)),
            sizeof(des_[i].stackMapSize_));
        moduleFile.write(reinterpret_cast<char *>(des_[i].GetStackMapSecAddr()),
            des_[i].GetStackMapSize());
    }
    moduleFile.close();
}

bool StubModulePackInfo::Load(EcmaVM *vm, const std::string &filename)
{
    //  now MachineCode is non movable, code and stackmap sperately is saved to MachineCode
    // by calling NewMachineCodeObject.
    //  then MachineCode will support movable, code is saved to MachineCode and stackmap is saved
    // to different heap which will be freed when stackmap is parsed by EcmaVM is started.
    if (!VerifyFilePath(filename)) {
        COMPILER_LOG(ERROR) << "Can not load stub file from default path [ "  << filename << " ], "
            << "please execute ark_stub_compiler with options --stub-file manually.";
        return false;
    }
    std::ifstream moduleFile(filename.c_str(), std::ofstream::binary);
    if (!moduleFile.good()) {
        moduleFile.close();
        return false;
    }
    moduleFile.read(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    entries_.resize(entryNum_);
    moduleFile.read(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    moduleFile.read(reinterpret_cast<char *>(&moduleNum_), sizeof(moduleNum_));
    des_.resize(moduleNum_);
    uint32_t totalCodeSize = 0;
    moduleFile.read(reinterpret_cast<char *>(&totalCodeSize), sizeof(totalCodeSize_));
    auto factory = vm->GetFactory();
    auto codeHandle = factory->NewMachineCodeObject(totalCodeSize, nullptr);
    SetCode(codeHandle);
    uint32_t curUnitOffset = 0;
    for (size_t i = 0; i < moduleNum_; i++) {
        moduleFile.read(reinterpret_cast<char *>(&(des_[i].hostCodeSectionAddr_)),
            sizeof(des_[i].hostCodeSectionAddr_));
        moduleFile.read(reinterpret_cast<char *>(&(des_[i].codeSize_)),
            sizeof(des_[i].codeSize_));
        uint32_t codeSize = des_[i].GetCodeSize();
        // startAddr of current code unit on device side
        uintptr_t startAddr = codeHandle->GetDataOffsetAddress() + static_cast<uintptr_t>(curUnitOffset);
        moduleFile.read(reinterpret_cast<char *>(startAddr), codeSize);
        curUnitOffset += codeSize;
        des_[i].SetDeviceCodeSecAddr(startAddr);
        moduleFile.read(reinterpret_cast<char *>(&(des_[i].stackMapSize_)),
            sizeof(des_[i].stackMapSize_));
        uint32_t stackmapSize = des_[i].GetStackMapSize();
        std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
        moduleFile.read(reinterpret_cast<char *>(stackmapPtr.get()), stackmapSize);
        if (stackmapSize != 0) {
            bool enableLog = vm->GetJSOptions().WasSetlogCompiledMethods();
            kungfu::LLVMStackMapParser::GetInstance(enableLog).CalculateStackMap(std::move(stackmapPtr),
                des_[i].GetHostCodeSecAddr(), startAddr);
        }
    }
    for (auto &funcEntryDes : GetStubs()) {
        auto codeAddr = funcEntryDes.codeAddr_; // offset
        auto moduleIndex = funcEntryDes.moduleIndex_;
        auto startAddr = des_[moduleIndex].GetDeviceCodeSecAddr();
        auto delta = funcEntryDes.fpDeltaPrevFramSp_;
        uintptr_t funAddr = startAddr + codeAddr;
        kungfu::Func2FpDelta fun2fpDelta;
        auto funSize = funcEntryDes.funcSize_;
        fun2fpDelta[funAddr] = std::make_pair(delta, funSize);
        kungfu::LLVMStackMapParser::GetInstance().CalculateFuncFpDelta(fun2fpDelta);
    }
    for (size_t i = 0; i < entries_.size(); i++) {
        auto des = des_[entries_[i].moduleIndex_];
        entries_[i].codeAddr_ += des.GetDeviceCodeSecAddr();
    }
    moduleFile.close();
    return true;
}

void AOTModulePackInfo::Save(const std::string &filename)
{
    if (!VerifyFilePath(filename, true)) {
        return;
    }
    std::ofstream moduleFile(filename.c_str(), std::ofstream::binary);
    SetStubNum(entries_.size());
    moduleFile.write(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    moduleFile.write(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    uint32_t moduleNum = GetCodeUnitsNum();
    moduleFile.write(reinterpret_cast<char *>(&moduleNum), sizeof(moduleNum_));
    moduleFile.write(reinterpret_cast<char *>(&totalCodeSize_), sizeof(totalCodeSize_));
    moduleFile.write(reinterpret_cast<char *>(aotFileHashs_.data()), sizeof(uint32_t) * moduleNum);
    for (size_t i = 0; i < moduleNum; i++) {
        moduleFile.write(reinterpret_cast<char *>(&(des_[i].hostCodeSectionAddr_)),
            sizeof(des_[i].hostCodeSectionAddr_));
        moduleFile.write(reinterpret_cast<char *>(&(des_[i].codeSize_)),
            sizeof(des_[i].codeSize_));
        moduleFile.write(reinterpret_cast<char *>(des_[i].GetHostCodeSecAddr()),
            des_[i].GetCodeSize());
        moduleFile.write(reinterpret_cast<char *>(&(des_[i].stackMapSize_)),
            sizeof(des_[i].stackMapSize_));
        moduleFile.write(reinterpret_cast<char *>(des_[i].GetStackMapSecAddr()),
            des_[i].GetStackMapSize());
    }
    moduleFile.close();
}

bool AOTModulePackInfo::Load(EcmaVM *vm, const std::string &filename)
{
    if (!VerifyFilePath(filename)) {
        return false;
    }
    std::ifstream moduleFile(filename.c_str(), std::ofstream::binary);
    if (!moduleFile.good()) {
        moduleFile.close();
        return false;
    }
    moduleFile.read(reinterpret_cast<char *>(&entryNum_), sizeof(entryNum_));
    entries_.resize(entryNum_);
    moduleFile.read(reinterpret_cast<char *>(entries_.data()), sizeof(FuncEntryDes) * entryNum_);
    moduleFile.read(reinterpret_cast<char *>(&moduleNum_), sizeof(moduleNum_));
    des_.resize(moduleNum_);
    aotFileHashs_.resize(moduleNum_);
    uint32_t totalCodeSize = 0;
    moduleFile.read(reinterpret_cast<char *>(&totalCodeSize), sizeof(totalCodeSize_));
    [[maybe_unused]] EcmaHandleScope handleScope(vm->GetAssociatedJSThread());
    auto factory = vm->GetFactory();
    auto codeHandle = factory->NewMachineCodeObject(totalCodeSize, nullptr);
    SetCode(codeHandle);
    moduleFile.read(reinterpret_cast<char *>(aotFileHashs_.data()), sizeof(uint32_t) * moduleNum_);
    uint32_t curUnitOffset = 0;
    for (size_t i = 0; i < moduleNum_; i++) {
        moduleFile.read(reinterpret_cast<char *>(&(des_[i].hostCodeSectionAddr_)),
            sizeof(des_[i].hostCodeSectionAddr_));
        moduleFile.read(reinterpret_cast<char *>(&(des_[i].codeSize_)), sizeof(des_[i].codeSize_));
        uint32_t codeSize = des_[i].GetCodeSize();
        uintptr_t startAddr = codeHandle->GetDataOffsetAddress() + static_cast<uintptr_t>(curUnitOffset);
        moduleFile.read(reinterpret_cast<char *>(startAddr), codeSize);
        curUnitOffset += codeSize;
        des_[i].SetDeviceCodeSecAddr(startAddr);
        moduleFile.read(reinterpret_cast<char *>(&(des_[i].stackMapSize_)), sizeof(des_[i].stackMapSize_));
        uint32_t stackmapSize = des_[i].GetStackMapSize();
        std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
        moduleFile.read(reinterpret_cast<char *>(stackmapPtr.get()), stackmapSize);
        if (stackmapSize != 0) {
            bool enableLog = vm->GetJSOptions().WasSetlogCompiledMethods();
            kungfu::LLVMStackMapParser::GetInstance(enableLog).CalculateStackMap(std::move(stackmapPtr),
                des_[i].GetHostCodeSecAddr(), startAddr);
        }
    }
    for (auto &funcEntryDes : GetStubs()) {
        auto codeAddr = funcEntryDes.codeAddr_; // offset
        auto moduleIndex = funcEntryDes.moduleIndex_;
        auto delta = funcEntryDes.fpDeltaPrevFramSp_;
        auto funSize = funcEntryDes.funcSize_;
        auto startAddr = des_[moduleIndex].GetDeviceCodeSecAddr();
        uintptr_t funAddr = startAddr + codeAddr;
        kungfu::Func2FpDelta fun2fpDelta;
        fun2fpDelta[funAddr] = std::make_pair(delta, funSize);
        kungfu::LLVMStackMapParser::GetInstance().CalculateFuncFpDelta(fun2fpDelta);
    }

    for (size_t i = 0; i < entries_.size(); i++) {
        auto des = des_[entries_[i].moduleIndex_];
        entries_[i].codeAddr_ += des.GetDeviceCodeSecAddr();
        auto curFileHash = aotFileHashs_[entries_[i].moduleIndex_];
        auto curMethodId = entries_[i].indexInKind_;
        vm->SetAOTFuncEntry(curFileHash, curMethodId, entries_[i].codeAddr_);
    }
    moduleFile.close();
    return true;
}

void ModulePackInfo::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&machineCodeObj_)));
}

bool ModulePackInfo::VerifyFilePath([[maybe_unused]] const std::string &filePath,
    [[maybe_unused]] bool toGenerate) const
{
#ifndef PANDA_TARGET_WINDOWS
    if (filePath.size() > PATH_MAX) {
        return false;
    }

    std::vector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (toGenerate && errno == ENOENT) {
        return true;
    }
    if (result == nullptr) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

void FileLoader::LoadStubFile(const std::string &fileName)
{
    if (!stubPackInfo_.Load(vm_, fileName)) {
        return;
    }
    auto stubs = stubPackInfo_.GetStubs();
    InitializeStubEntries(stubs);
}

void FileLoader::LoadAOTFile(const std::string &fileName)
{
    AOTModulePackInfo aotPackInfo_;
    if (!aotPackInfo_.Load(vm_, fileName)) {
        return;
    }
    AddAOTPackInfo(aotPackInfo_);
}

void FileLoader::TryLoadSnapshotFile()
{
    const CString snapshotPath(vm_->GetJSOptions().GetSnapshotOutputFile().c_str());
    Snapshot snapshot(vm_);
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
    snapshot.Deserialize(SnapshotType::TS_LOADER, snapshotPath);
#endif
}

void FileLoader::UpdateJSMethods(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile)
{
    // get generated const pool in execution phase
    auto thread = vm_->GetAssociatedJSThread();
    JSHandle<JSTaggedValue> constPool(thread, mainFunc->GetConstantPool());
    ConstantPool *curPool = ConstantPool::Cast(constPool->GetTaggedObject());

    // get main func method
    auto mainFuncMethodId = jsPandaFile->GetMainMethodIndex();
    auto fileHash = jsPandaFile->GetPandaFile()->GetFilenameHash();
    auto mainEntry = GetAOTFuncEntry(fileHash, mainFuncMethodId);
    // 1 : default para number
    JSMethod *mainMethod = factory_->NewMethodForAOTFunction(reinterpret_cast<void *>(mainEntry), 1);
    mainFunc->SetMethod(mainMethod);
    mainFunc->SetCodeEntry(reinterpret_cast<uintptr_t>(mainEntry));

    const CUnorderedMap<uint32_t, uint64_t> &constpoolMap = jsPandaFile->GetConstpoolMap();
    for (const auto &it : constpoolMap) {
        ConstPoolValue value(it.second);
        if (value.GetConstpoolType() == ConstPoolType::BASE_FUNCTION ||
            value.GetConstpoolType() == ConstPoolType::NC_FUNCTION ||
            value.GetConstpoolType() == ConstPoolType::GENERATOR_FUNCTION ||
            value.GetConstpoolType() == ConstPoolType::ASYNC_FUNCTION ||
            value.GetConstpoolType() == ConstPoolType::METHOD) {
                auto id = value.GetConstpoolIndex();
                auto codeEntry = GetAOTFuncEntry(fileHash, it.first);
                JSMethod *curMethod = factory_->NewMethodForAOTFunction(reinterpret_cast<void *>(codeEntry), 1);
                auto curFunction = JSFunction::Cast(curPool->GetObjectFromCache(id).GetTaggedObject());
                curFunction->SetMethod(curMethod);
                curFunction->SetCodeEntry(reinterpret_cast<uintptr_t>(codeEntry));
        }
    }
}

void FileLoader::AdjustBCStubAndDebuggerStubEntries(JSThread *thread,
    const std::vector<ModulePackInfo::FuncEntryDes> &stubs,
    const AsmInterParsedOption &asmInterOpt)
{
    auto defaultBCStubDes = stubs[CommonStubCSigns::NUM_OF_STUBS + BytecodeStubCSigns::SingleStepDebugging];
    auto defaultNonexistentBCStubDes = stubs[CommonStubCSigns::NUM_OF_STUBS + BytecodeStubCSigns::HandleOverflow];
    auto defaultBCDebuggerStubDes = stubs[CommonStubCSigns::NUM_OF_STUBS + BytecodeStubCSigns::BCDebuggerEntry];
    auto defaultBCDebuggerExceptionStubDes =
        stubs[CommonStubCSigns::NUM_OF_STUBS + BytecodeStubCSigns::BCDebuggerExceptionEntry];
    thread->SetUnrealizedBCStubEntry(defaultBCStubDes.codeAddr_);
    thread->SetNonExistedBCStubEntry(defaultNonexistentBCStubDes.codeAddr_);
#define UNDEF_STUB(name)    \
    thread->SetBCStubEntry(BytecodeStubCSigns::ID_##name, defaultBCStubDes.codeAddr_);
    INTERPRETER_IGNORED_BC_STUB_LIST(UNDEF_STUB)
#undef UNDEF_STUB
    for (int i = asmInterOpt.handleStart; i <= asmInterOpt.handleEnd && i >= 0; i++) {
        thread->SetBCStubEntry(static_cast<size_t>(i), defaultBCStubDes.codeAddr_);
    }
    // bc debugger stub entries
    thread->SetNonExistedBCDebugStubEntry(defaultNonexistentBCStubDes.codeAddr_);
    for (size_t i = 0; i < BCStubEntries::EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
        if (i == BytecodeStubCSigns::ID_ExceptionHandler) {
            thread->SetBCDebugStubEntry(i, defaultBCDebuggerExceptionStubDes.codeAddr_);
            continue;
        }
        thread->SetBCDebugStubEntry(i, defaultBCDebuggerStubDes.codeAddr_);
    }
}

void FileLoader::InitializeStubEntries(const std::vector<AOTModulePackInfo::FuncEntryDes>& stubs)
{
    auto thread = vm_->GetAssociatedJSThread();
    for (size_t i = 0; i < stubs.size(); i++) {
        auto des = stubs[i];
        if (des.IsCommonStub()) {
            thread->SetFastStubEntry(des.indexInKind_, des.codeAddr_);
        } else if (des.IsBCStub()) {
            // bc helper handler use to adjust bc stub, not init bc stub
            if (des.IsBCHandlerStub()) {
                thread->SetBCStubEntry(des.indexInKind_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
                std::cout << "bytecode: " << GetEcmaOpcodeStr(static_cast<EcmaOpcode>(des.indexInKind_))
                    << " addr:" << des.codeAddr_ << std::endl;
#endif
            }
        } else {
            thread->RegisterRTInterface(des.indexInKind_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
                std::cout << "runtime index: " << des.indexInKind_ << " addr:" << des.codeAddr_ << std::endl;
#endif
        }
    }
    AsmInterParsedOption asmInterOpt = vm_->GetJSOptions().GetAsmInterParsedOption();
    AdjustBCStubAndDebuggerStubEntries(thread, stubs, asmInterOpt);
}
}