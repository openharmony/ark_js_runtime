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

#ifndef ECMASCRIPT_COMPILER_LLVM_LLVMSTACKPARSE_H
#define ECMASCRIPT_COMPILER_LLVM_LLVMSTACKPARSE_H

#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "ecmascript/common.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript::kungfu {
using OffsetType = int32_t;
using DwarfRegType = uint16_t;
using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
using CallSiteInfo = std::vector<DwarfRegAndOffsetType>;
using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;

struct Header {
    uint8_t  stackmapversion; // Stack Map Version (current version is 3)
    uint8_t  Reserved0; // Reserved (expected to be 0)
    uint16_t Reserved1; // Reserved (expected to be 0)
    void Print() const
    {
        COMPILER_LOG(DEBUG) << "----- head ----";
        COMPILER_LOG(DEBUG) << "   version:" << static_cast<int>(stackmapversion);
        COMPILER_LOG(DEBUG) << "+++++ head ++++";
    }
};

#pragma pack(1)
struct StkSizeRecordTy {
    uintptr_t functionAddress;
    uint64_t stackSize;
    uint64_t recordCount;
    void Print() const
    {
        COMPILER_LOG(DEBUG) << "               functionAddress:0x" << std::hex << functionAddress;
        COMPILER_LOG(DEBUG) << "               stackSize:0x" << std::hex << stackSize;
        COMPILER_LOG(DEBUG) << "               recordCount:" << std::hex << recordCount;
    }
};
#pragma pack()

struct ConstantsTy {
    uintptr_t LargeConstant;
    void Print() const
    {
        COMPILER_LOG(DEBUG) << "               LargeConstant:0x" << std::hex << LargeConstant;
    }
};

struct StkMapRecordHeadTy {
    uint64_t PatchPointID;
    uint32_t InstructionOffset;
    uint16_t Reserved;
    uint16_t NumLocations;
    void Print() const
    {
        COMPILER_LOG(DEBUG) << "               PatchPointID:0x" << std::hex << PatchPointID;
        COMPILER_LOG(DEBUG) << "               instructionOffset:0x" << std::hex << InstructionOffset;
        COMPILER_LOG(DEBUG) << "               Reserved:0x" << std::hex << Reserved;
        COMPILER_LOG(DEBUG) << "               NumLocations:0x" << std::hex << NumLocations;
    }
};

struct  LocationTy {
    enum class Kind: uint8_t {
        REGISTER = 1,
        DIRECT = 2,
        INDIRECT = 3,
        CONSTANT = 4,
        CONSTANTNDEX = 5,
    };
    Kind location;
    uint8_t Reserved_0;
    uint16_t LocationSize;
    uint16_t DwarfRegNum;
    uint16_t Reserved_1;
    OffsetType OffsetOrSmallConstant;

    std::string PUBLIC_API TypeToString(Kind loc) const;

    void Print() const
    {
        COMPILER_LOG(DEBUG)  << TypeToString(location);
        COMPILER_LOG(DEBUG) << ", size:" << std::dec << LocationSize;
        COMPILER_LOG(DEBUG) << "\tDwarfRegNum:" << DwarfRegNum;
        COMPILER_LOG(DEBUG) << "\t OffsetOrSmallConstant:" << OffsetOrSmallConstant;
    }
};

struct LiveOutsTy {
    DwarfRegType DwarfRegNum;
    uint8_t Reserved;
    uint8_t SizeinBytes;
    void Print() const
    {
        COMPILER_LOG(DEBUG) << "                  Dwarf RegNum:" << DwarfRegNum;
        COMPILER_LOG(DEBUG) << "                  Reserved:" << Reserved;
        COMPILER_LOG(DEBUG) << "                  SizeinBytes:" << SizeinBytes;
    }
};

struct StkMapRecordTy {
    struct StkMapRecordHeadTy head;
    std::vector<struct LocationTy> Locations;
    std::vector<struct LiveOutsTy> LiveOuts;
    void Print() const
    {
        head.Print();
        auto size = Locations.size();
        for (size_t i = 0; i < size; i++) {
            COMPILER_LOG(DEBUG) << "                   #" << std::dec << i << ":";
            Locations[i].Print();
        }
        size = LiveOuts.size();
        for (size_t i = 0; i < size; i++) {
            COMPILER_LOG(DEBUG) << "               liveOuts[" << i << "] info:";
        }
    }
};

class DataInfo {
public:
    explicit DataInfo(std::unique_ptr<uint8_t[]> data): data_(std::move(data)), offset_(0) {}
    ~DataInfo()
    {
        data_.reset();
        offset_ = 0;
    }
    template<class T>
    T Read()
    {
        T t = *reinterpret_cast<const T*>(data_.get() + offset_);
        offset_ += sizeof(T);
        return t;
    }
    unsigned int GetOffset() const
    {
        return offset_;
    }
private:
    std::unique_ptr<uint8_t[]> data_ {nullptr};
    unsigned int offset_ {0};
};

struct LLVMStackMap {
    struct Header head;
    std::vector<struct StkSizeRecordTy> StkSizeRecords;
    std::vector<struct ConstantsTy> Constants;
    std::vector<struct StkMapRecordTy> StkMapRecord;
    void Print() const
    {
        head.Print();
        for (size_t i = 0; i < StkSizeRecords.size(); i++) {
            COMPILER_LOG(DEBUG) << "stkSizeRecord[" << i << "] info:";
            StkSizeRecords[i].Print();
        }
        for (size_t i = 0; i < Constants.size(); i++) {
            COMPILER_LOG(DEBUG) << "constants[" << i << "] info:";
            Constants[i].Print();
        }
        for (size_t i = 0; i < StkMapRecord.size(); i++) {
            COMPILER_LOG(DEBUG) << "StkMapRecord[" << i << "] info:";
            StkMapRecord[i].Print();
        }
    }
};

class LLVMStackMapParser {
public:
    static LLVMStackMapParser& GetInstance(bool enableLog = false)
    {
        static LLVMStackMapParser instance(enableLog);
        return instance;
    }
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr);
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr, uintptr_t deviceCodeSectionAddr);
    void PUBLIC_API Print() const
    {
        if (IsLogEnabled()) {
            llvmStackMap_.Print();
        }
    }
    const CallSiteInfo *GetCallSiteInfoByPc(uintptr_t funcAddr) const;
    bool CollectStackMapSlots(uintptr_t callSiteAddr, uintptr_t frameFp,
                            std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data,
                            [[maybe_unused]] bool isVerifying) const;

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

private:
    explicit LLVMStackMapParser(bool enableLog)
    {
        stackMapAddr_ = nullptr;
        pc2CallSiteInfo_.clear();
        pid2CallSiteInfo_.clear();
        dataInfo_ = nullptr;
        enableLog_ = enableLog;
    }
    ~LLVMStackMapParser()
    {
        if (stackMapAddr_) {
            stackMapAddr_.release();
        }
        pc2CallSiteInfo_.clear();
        pid2CallSiteInfo_.clear();
        dataInfo_ = nullptr;
    }
    void CalcCallSite();
    bool IsDeriveredPointer(int callsitetime) const;
    const CallSiteInfo* GetCallSiteInfoByPatchID(uint64_t patchPointId) const;
    void PrintCallSiteInfo(const CallSiteInfo *infos, OptimizedLeaveFrame *frame) const;
    void PrintCallSiteInfo(const CallSiteInfo *infos, uintptr_t *fp) const;

    std::unique_ptr<uint8_t[]> stackMapAddr_;
    struct LLVMStackMap llvmStackMap_;
    std::unordered_map<uintptr_t, CallSiteInfo> pc2CallSiteInfo_;
    std::unordered_map<uint64_t, CallSiteInfo> pid2CallSiteInfo_;
    [[maybe_unused]] std::unique_ptr<DataInfo> dataInfo_;
    bool enableLog_ {false};
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_LLVMSTACKPARSE_H