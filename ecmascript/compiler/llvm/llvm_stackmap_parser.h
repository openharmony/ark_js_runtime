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
#include <vector>
#include <set>
#include "ecmascript/common.h"
#include "ecmascript/ecma_macros.h"

#ifdef PANDA_TARGET_AMD64
#define SP_DWARF_REG_NUM  7
#define FP_DWARF_REG_NUM  6
#define SP_OFFSET      2
#else
#define SP_DWARF_REG_NUM  0
#define FP_DWARF_REG_NUM  0
#define SP_OFFSET      0
#endif

namespace kungfu {
using OffsetType = int32_t;
using DwarfRegType = uint16_t;
using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
using DwarfRegAndOffsetTypeVector = std::vector<DwarfRegAndOffsetType>;
using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;

struct Header {
    uint8_t  stackmapversion; // Stack Map Version (current version is 3)
    uint8_t  Reserved0; // Reserved (expected to be 0)
    uint16_t Reserved1; // Reserved (expected to be 0)
    void Print() const
    {
        LOG_ECMA(INFO) << "----- head ----" << std::endl;
        LOG_ECMA(INFO) << "   version:" << static_cast<int>(stackmapversion) << std::endl;
        LOG_ECMA(INFO) << "+++++ head ++++" << std::endl;
    }
};

struct StkSizeRecordTy {
    uint64_t functionAddress;
    uint64_t stackSize;
    uint64_t recordCount;
    void Print() const
    {
            LOG_ECMA(INFO) << "               functionAddress:0x" << std::hex << functionAddress << std::endl;
            LOG_ECMA(INFO) << "               stackSize:" << std::dec << stackSize << std::endl;
            LOG_ECMA(INFO) << "               recordCount:" << std::dec << recordCount << std::endl;
    }
};

struct ConstantsTy {
    uint64_t LargeConstant;
    void Print() const
    {
        LOG_ECMA(INFO) << "               LargeConstant:" << LargeConstant << std::endl;
    }
};

struct StkMapRecordHeadTy {
    uint64_t PatchPointID;
    uint32_t InstructionOffset;
    uint16_t Reserved;
    uint16_t NumLocations;
    void Print() const
    {
        LOG_ECMA(INFO) << "               PatchPointID:" << std::hex << PatchPointID << std::endl;
        LOG_ECMA(INFO) << "               instructionOffset:" << std::hex << InstructionOffset << std::endl;
        LOG_ECMA(INFO) << "               Reserved:" << Reserved << std::endl;
        LOG_ECMA(INFO) << "               NumLocations:" << NumLocations << std::endl;
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
        LOG_ECMA(INFO)  << TypeToString(location);
        LOG_ECMA(INFO) << ", size:" << std::dec << LocationSize;
        LOG_ECMA(INFO) << "\tDwarfRegNum:" << DwarfRegNum;
        LOG_ECMA(INFO) << "\t OffsetOrSmallConstant:" << OffsetOrSmallConstant << std::endl;
    }
};

struct LiveOutsTy {
    DwarfRegType DwarfRegNum;
    uint8_t Reserved;
    uint8_t SizeinBytes;
    void Print() const
    {
        LOG_ECMA(INFO) << "                  Dwarf RegNum:" << DwarfRegNum << std::endl;
        LOG_ECMA(INFO) << "                  Reserved:" << Reserved << std::endl;
        LOG_ECMA(INFO) << "                  SizeinBytes:" << SizeinBytes << std::endl;
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
            LOG_ECMA(INFO) << "                   #" << std::dec << i << ":";
            Locations[i].Print();
        }
        size = LiveOuts.size();
        for (size_t i = 0; i < size; i++) {
            LOG_ECMA(INFO) << "               liveOuts[" << i << "] info:" << std::endl;
        }
    }
};

class DataInfo {
public:
    explicit DataInfo(const uint8_t *data): data_(data), offset_(0) {}
    ~DataInfo()
    {
        data_ = nullptr;
        offset_ = 0;
    }
    template<class T>
    T Read()
    {
        T t = *reinterpret_cast<const T*>(data_ + offset_);
        offset_ += sizeof(T);
        return t;
    }
    unsigned int GetOffset() const
    {
        return offset_;
    }
private:
    const uint8_t *data_;
    unsigned int offset_;
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
            LOG_ECMA(INFO) << "stkSizeRecord[" << i << "] info:" << std::endl;
            StkSizeRecords[i].Print();
        }
        for (size_t i = 0; i < Constants.size(); i++) {
            LOG_ECMA(INFO) << "constants[" << i << "] info:" << std::endl;
            Constants[i].Print();
        }
        for (size_t i = 0; i < StkMapRecord.size(); i++) {
            LOG_ECMA(INFO) << "StkMapRecord[" << i << "] info:" << std::endl;
            StkMapRecord[i].Print();
        }
    }
};

class LLVMStackMapParser {
public:
    static LLVMStackMapParser& GetInstance()
    {
        static LLVMStackMapParser instance;
        return instance;
    }
    bool PUBLIC_API CalculateStackMap(const uint8_t *stackMapAddr);
    bool PUBLIC_API CalculateStackMap(const uint8_t *stackMapAddr,
        uintptr_t hostCodeSectionAddr, uintptr_t deviceCodeSectionAddr);
    void PUBLIC_API Print() const
    {
        llvmStackMap_.Print();
    }
    bool StackMapByAddr(uintptr_t funcAddr, DwarfRegAndOffsetTypeVector &infos);
    bool StackMapByFuncAddrFp(uintptr_t funcAddr, uintptr_t frameFp,
                                                std::set<uintptr_t> &slotAddrs);
private:
    LLVMStackMapParser()
    {
        stackMapAddr_ = nullptr;
        callSiteInfos_.clear();
        dataInfo_ = nullptr;
    }
    ~LLVMStackMapParser()
    {
        stackMapAddr_ = nullptr;
        callSiteInfos_.clear();
        dataInfo_ = nullptr;
    }
    void CalcCallSite();
    const uint8_t *stackMapAddr_;
    struct LLVMStackMap llvmStackMap_;
    std::vector<Fun2InfoType> callSiteInfos_;
    [[maybe_unused]] std::unique_ptr<DataInfo> dataInfo_;
};
} // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_LLVMSTACKPARSE_H