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
#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_MACHINE_CODE_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_MACHINE_CODE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/compiler/call_signature.h"
#include "libpandabase/macros.h"


namespace panda::ecmascript {
class MachineCode : public TaggedObject {
public:
    NO_COPY_SEMANTIC(MachineCode);
    NO_MOVE_SEMANTIC(MachineCode);
    static MachineCode *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsMachineCodeObject());
        return static_cast<MachineCode *>(object);
    }

    static constexpr size_t INS_SIZE_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(InstructionSizeInBytes, uint32_t, INS_SIZE_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t DATA_OFFSET = SIZE;

    DECL_DUMP()

    uintptr_t GetDataOffsetAddress()
    {
        return reinterpret_cast<uintptr_t>(this) + DATA_OFFSET;
    }

    void SetData(const uint8_t *stackMapData, size_t codeLength)
    {
        if (stackMapData == nullptr) {
            LOG_ECMA_MEM(ERROR) << "data is null in creating new code object";
            return;
        }
        if (memcpy_s(reinterpret_cast<void *>(this->GetDataOffsetAddress()),
            this->GetInstructionSizeInBytes(), stackMapData, codeLength) != EOK) {
            LOG_ECMA_MEM(ERROR) << "memcpy fail in creating new code object ";
            return;
        }
    }

    void VisitRangeSlot([[maybe_unused]] const EcmaObjectRangeVisitor &v)
    {
        // left blank deliberately,only need to visit TaggedObject type object.
    }

    void VisitObjects([[maybe_unused]] const EcmaObjectRangeVisitor &visitor) const
    {
        // left blank deliberately,only need to visit TaggedObject type object.
    }

    size_t GetMachineCodeObjectSize()
    {
        return SIZE + this->GetInstructionSizeInBytes();
    }
};

class PUBLIC_API AotCodeInfo {
public:
    AotCodeInfo() {};
    ~AotCodeInfo() = default;

    void SerializeForStub(const std::string &filename);
    void Serialize(const std::string &filename);
    bool DeserializeForStub(JSThread *thread, const std::string &filename);
    bool Deserialize(EcmaVM *vm, const std::string &filename);
    bool VerifyFilePath(const CString &filePath) const;

    struct StubDes {
        uint64_t codeAddr_;
        CallSignature::TargetKind kind_;
        uint32_t indexInKind_;
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

        bool IsAsmStub() const
        {
            return (kind_ == kungfu::CallSignature::TargetKind::ASM_STUB);
        }
    };

    const StubDes& GetStubDes(int index) const
    {
        return stubEntries_[index];
    }

    const std::vector<StubDes>& GetStubs() const
    {
        return stubEntries_;
    }

    void Iterate(const RootVisitor &v);

    void SetCode(MachineCode *code)
    {
        code_ = JSTaggedValue(code);
    }

    JSTaggedValue GetCode()
    {
        return code_;
    }

    void SetCodePtr(uintptr_t codePtr)
    {
        codePtr_ = codePtr;
    }

    uintptr_t GetCodePtr()
    {
        return codePtr_;
    }

    void SetAOTFuncEntry(std::string str, uint64_t offset, uint32_t methodId)
    {
        auto it = make_pair(str, methodId);
        aotFuncEntryOffsets_[it] = offset;
    }

    uint64_t GetAOTFuncEntry(const std::string &name)
    {
        ASSERT(!code_.IsHole());
        for (auto it : aotFuncEntryOffsets_) {
            if (it.first.first == name) {
                return aotFuncEntryOffsets_[it.first];
            }
        }
        return reinterpret_cast<uint64_t>(nullptr);
    }

    uint64_t GetAOTFuncEntry(uint32_t id)
    {
        ASSERT(!code_.IsHole());
        for (auto it : aotFuncEntryOffsets_) {
            if (it.first.second == id) {
                return aotFuncEntryOffsets_[it.first];
            }
        }
        return reinterpret_cast<uint64_t>(nullptr);
    }

    void SetHostCodeSectionAddr(uint64_t addr)
    {
        hostCodeSectionAddr_ = addr;
    }

    uint64_t GetHostCodeSectionAddr() const
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

    void SetStackMapAddr(uint64_t addr)
    {
        stackMapAddr_ = addr;
    }

    uint64_t GetStackMapAddr() const
    {
        return stackMapAddr_;
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
        des.indexInKind_ = static_cast<uint32_t>(indexInKind);
        des.codeAddr_ = offset;
        stubEntries_.emplace_back(des);
    }
private:
    uint64_t stubNum_ {0};
    std::vector<StubDes> stubEntries_ {};
    std::map<std::pair<std::string, uint32_t>, uint64_t> aotFuncEntryOffsets_ {};
    uint64_t hostCodeSectionAddr_  {0};
    uintptr_t devicesCodeSectionAddr_ {0};
    // NOTE: code object is non movable(code space) currently.
    // The thought use of code->GetDataOffsetAddress() as data base rely on this premise.
    // A stable technique or mechanism for code object against other GC situation is future work.
    JSTaggedValue code_ {JSTaggedValue::Hole()};
    uintptr_t stackMapAddr_ {0};
    uintptr_t codePtr_ {0};
    uint32_t codeSize_ {0};
    uint32_t stackMapSize_ {0};
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_MACHINE_CODE_H
