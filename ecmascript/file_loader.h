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
#ifndef ECMASCRIPT_COMPILER_FILE_LOADER_H
#define ECMASCRIPT_COMPILER_FILE_LOADER_H

#include "ecmascript/js_function.h"
#include "ecmascript/js_runtime_options.h"

namespace panda::ecmascript {
class JSpandafile;
class JSThread;

struct ModuleSectionDes {
    uint64_t hostCodeSectionAddr_ {0};
    uint64_t stackMapSectionAddr_ {0};
    uint32_t codeSize_ {0};
    uint32_t stackMapSize_ {0};
    uintptr_t deviceCodeSectionAddr_ {0};
    ModuleSectionDes() = default;
    ModuleSectionDes(uint64_t codeSecBegin, uint64_t stackmapSecBegin, uint32_t codeSize,
        uint32_t stackMapSize) : hostCodeSectionAddr_(codeSecBegin), stackMapSectionAddr_(stackmapSecBegin),
                                 codeSize_(codeSize), stackMapSize_(stackMapSize) {}
    void SetHostCodeSecAddr(uint64_t addr)
    {
        hostCodeSectionAddr_ = addr;
    }

    uint64_t GetHostCodeSecAddr() const
    {
        return hostCodeSectionAddr_;
    }

    void SetStackMapSecAddr(uint64_t addr)
    {
        stackMapSectionAddr_ = addr;
    }

    uint64_t GetStackMapSecAddr() const
    {
        return stackMapSectionAddr_;
    }

    void SetStackMapSize(uint32_t len)
    {
        stackMapSize_ = len;
    }

    uint32_t GetStackMapSize() const
    {
        return stackMapSize_;
    }

    void SetCodeSize(uint32_t len)
    {
        codeSize_ = len;
    }

    uint32_t GetCodeSize() const
    {
        return codeSize_;
    }

    void SetDeviceCodeSecAddr(uintptr_t addr)
    {
        deviceCodeSectionAddr_ = addr;
    }

    uintptr_t GetDeviceCodeSecAddr() const
    {
        return deviceCodeSectionAddr_;
    }
};

class PUBLIC_API ModulePackInfo {
public:
    using CallSignature = kungfu::CallSignature;
    ModulePackInfo() = default;
    virtual ~ModulePackInfo() = default;
    bool VerifyFilePath([[maybe_unused]] const std::string &filePath, [[maybe_unused]] bool toGenerate = false) const;

    struct FuncEntryDes {
        uint64_t codeAddr_;
        CallSignature::TargetKind kind_;
        uint32_t indexInKind_;
        uint32_t moduleIndex_;
        int fpDeltaPrevFramSp_;
        uint32_t funcSize_;
        bool IsStub() const
        {
            return CallSignature::TargetKind::STUB_BEGIN <= kind_ && kind_ < CallSignature::TargetKind::STUB_END;
        }

        bool IsBCStub() const
        {
            return CallSignature::TargetKind::BCHANDLER_BEGIN <= kind_ &&
                   kind_ < CallSignature::TargetKind::BCHANDLER_END;
        }

        bool IsBCHandlerStub() const
        {
            return (kind_ == CallSignature::TargetKind::BYTECODE_HANDLER);
        }

        bool IsCommonStub() const
        {
            return (kind_ == CallSignature::TargetKind::COMMON_STUB);
        }
    };

    const FuncEntryDes& GetStubDes(int index) const
    {
        return entries_[index];
    }

    const std::vector<FuncEntryDes>& GetStubs() const
    {
        return entries_;
    }

    void Iterate(const RootVisitor &v);

    void SetCode(JSHandle<MachineCode> code)
    {
        machineCodeObj_ = code.GetTaggedValue();
    }

    JSHandle<MachineCode> GetCode()
    {
        return JSHandle<MachineCode>(reinterpret_cast<uintptr_t>(&machineCodeObj_));
    }

    bool isCodeHole()
    {
        return machineCodeObj_.IsHole();
    }

    uint32_t GetStubNum() const
    {
        return entryNum_;
    }

    void SetStubNum(uint32_t n)
    {
        entryNum_ = n;
    }

    void AddStubEntry(CallSignature::TargetKind kind, int indexInKind, uint64_t offset,
    uint32_t moduleIndex, int delta, uint32_t size)
    {
        FuncEntryDes des;
        des.kind_ = kind;
        des.indexInKind_ = static_cast<uint32_t>(indexInKind);
        des.codeAddr_ = offset;
        des.moduleIndex_ = moduleIndex;
        des.fpDeltaPrevFramSp_ = delta;
        des.funcSize_ = size;
        entries_.emplace_back(des);
    }

    size_t GetCodeUnitsNum()
    {
        return des_.size();
    }
protected:
    uint32_t entryNum_ {0};
    uint32_t moduleNum_ {0};
    uint32_t totalCodeSize_ {0};
    std::vector<FuncEntryDes> entries_ {};
    std::vector<ModuleSectionDes> des_ {};
    // NOTE: code object is non movable(code space) currently.
    // The thought use of code->GetDataOffsetAddress() as data base rely on this assumption.
    // A stable technique or mechanism for code object against other GC situation is future work.
    JSTaggedValue machineCodeObj_ {JSTaggedValue::Hole()};
};

class PUBLIC_API AOTModulePackInfo : public ModulePackInfo {
public:
    AOTModulePackInfo() = default;
    ~AOTModulePackInfo() override = default;
    void Save(const std::string &filename);
    bool Load(EcmaVM *vm, const std::string &filename);
    void AddModuleDes(ModuleSectionDes moduleDes, uint32_t hash)
    {
        des_.emplace_back(moduleDes);
        totalCodeSize_ += moduleDes.codeSize_;
        aotFileHashs_.emplace_back(hash);
    }

private:
    std::vector<uint32_t> aotFileHashs_ {};
};

class PUBLIC_API StubModulePackInfo : public ModulePackInfo {
public:
    StubModulePackInfo() = default;
    ~StubModulePackInfo() override = default;
    void Save(const std::string &filename);
    bool Load(EcmaVM *vm, const std::string &filename);

    void AddModuleDes(ModuleSectionDes moduleDes)
    {
        des_.emplace_back(moduleDes);
        totalCodeSize_ += moduleDes.codeSize_;
    }
};

class FileLoader {
using CommonStubCSigns = kungfu::CommonStubCSigns;
using BytecodeStubCSigns = kungfu::BytecodeStubCSigns;
public:
    explicit FileLoader(EcmaVM *vm) : vm_(vm), factory_(vm->GetFactory()) {}
    ~FileLoader() = default;
    void LoadStubFile(const std::string &fileName);
    void LoadAOTFile(const std::string &fileName);
    void AddAOTPackInfo(AOTModulePackInfo packInfo)
    {
        aotPackInfos_.emplace_back(packInfo);
    }
    void Iterate(const RootVisitor &v)
    {
        if (!stubPackInfo_.isCodeHole()) {
            stubPackInfo_.Iterate(v);
        }
        if (aotPackInfos_.size() != 0) {
            for (auto &packInfo : aotPackInfos_) {
                packInfo.Iterate(v);
            }
        }
    }

    void SetAOTFuncEntry(uint32_t hash, uint32_t methodId, uint64_t funcEntry)
    {
        hashToEntryMap_[hash][methodId] = funcEntry;
    }

    uintptr_t GetAOTFuncEntry(uint32_t hash, uint32_t methodId)
    {
        if (hashToEntryMap_.find(hash) == hashToEntryMap_.end()) {
            return reinterpret_cast<uint64_t>(nullptr);
        } else if (hashToEntryMap_[hash].find(methodId) == hashToEntryMap_[hash].end()) {
            return reinterpret_cast<uint64_t>(nullptr);
        }
        return static_cast<uintptr_t>(hashToEntryMap_[hash][methodId]);
    }

    void UpdateJSMethods(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile);
    void TryLoadSnapshotFile();
private:
    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    StubModulePackInfo stubPackInfo_ {};
    std::vector<AOTModulePackInfo> aotPackInfos_ {};
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint64_t>> hashToEntryMap_ {};

    void InitializeStubEntries(const std::vector<AOTModulePackInfo::FuncEntryDes>& stubs);
    void AdjustBCStubAndDebuggerStubEntries(JSThread *thread, const std::vector<ModulePackInfo::FuncEntryDes> &stubs,
        const AsmInterParsedOption &asmInterOpt);
};
}
#endif // ECMASCRIPT_COMPILER_FILE_LOADER_H