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


namespace kungfu {
using OffsetType = int32_t;
using DwarfRegType = uint16_t;
using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
using DwarfRegAndOffsetTypeVector = std::vector<DwarfRegAndOffsetType>;
using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;
using DerivedData = panda::ecmascript::DerivedData;
using RootVisitor = panda::ecmascript::RootVisitor;
using RootRangeVisitor = panda::ecmascript::RootRangeVisitor;

struct Header {
    uint8_t  stackmapversion; // Stack Map Version (current version is 3)
    uint8_t  Reserved0; // Reserved (expected to be 0)
    uint16_t Reserved1; // Reserved (expected to be 0)
    void Print() const
    {
        LOG_ECMA(DEBUG) << "----- head ----";
        LOG_ECMA(DEBUG) << "   version:" << static_cast<int>(stackmapversion);
        LOG_ECMA(DEBUG) << "+++++ head ++++";
    }
};

#pragma pack(1)
struct StkSizeRecordTy {
    uintptr_t functionAddress;
    uint64_t stackSize;
    uint64_t recordCount;
    void Print() const
    {
        LOG_ECMA(DEBUG) << "               functionAddress:0x" << std::hex << functionAddress;
        LOG_ECMA(DEBUG) << "               stackSize:0x" << std::hex << stackSize;
        LOG_ECMA(DEBUG) << "               recordCount:" << std::hex << recordCount;
    }
};
#pragma pack()

struct ConstantsTy {
    uintptr_t LargeConstant;
    void Print() const
    {
        LOG_ECMA(DEBUG) << "               LargeConstant:0x" << std::hex << LargeConstant;
    }
};

struct StkMapRecordHeadTy {
    uint64_t PatchPointID;
    uint32_t InstructionOffset;
    uint16_t Reserved;
    uint16_t NumLocations;
    void Print() const
    {
        LOG_ECMA(DEBUG) << "               PatchPointID:0x" << std::hex << PatchPointID;
        LOG_ECMA(DEBUG) << "               instructionOffset:0x" << std::hex << InstructionOffset;
        LOG_ECMA(DEBUG) << "               Reserved:0x" << std::hex << Reserved;
        LOG_ECMA(DEBUG) << "               NumLocations:0x" << std::hex << NumLocations;
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
        LOG_ECMA(DEBUG)  << TypeToString(location);
        LOG_ECMA(DEBUG) << ", size:" << std::dec << LocationSize;
        LOG_ECMA(DEBUG) << "\tDwarfRegNum:" << DwarfRegNum;
        LOG_ECMA(DEBUG) << "\t OffsetOrSmallConstant:" << OffsetOrSmallConstant;
    }
};

struct LiveOutsTy {
    DwarfRegType DwarfRegNum;
    uint8_t Reserved;
    uint8_t SizeinBytes;
    void Print() const
    {
        LOG_ECMA(DEBUG) << "                  Dwarf RegNum:" << DwarfRegNum;
        LOG_ECMA(DEBUG) << "                  Reserved:" << Reserved;
        LOG_ECMA(DEBUG) << "                  SizeinBytes:" << SizeinBytes;
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
            LOG_ECMA(DEBUG) << "                   #" << std::dec << i << ":";
            Locations[i].Print();
        }
        size = LiveOuts.size();
        for (size_t i = 0; i < size; i++) {
            LOG_ECMA(DEBUG) << "               liveOuts[" << i << "] info:";
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
            LOG_ECMA(DEBUG) << "stkSizeRecord[" << i << "] info:";
            StkSizeRecords[i].Print();
        }
        for (size_t i = 0; i < Constants.size(); i++) {
            LOG_ECMA(DEBUG) << "constants[" << i << "] info:";
            Constants[i].Print();
        }
        for (size_t i = 0; i < StkMapRecord.size(); i++) {
            LOG_ECMA(DEBUG) << "StkMapRecord[" << i << "] info:";
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
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr);
    bool PUBLIC_API CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr, uintptr_t deviceCodeSectionAddr);
    void PUBLIC_API Print() const
    {
        llvmStackMap_.Print();
    }
    const DwarfRegAndOffsetTypeVector *StackMapByAddr(uintptr_t funcAddr) const;
    bool StackMapByFuncAddrFp(uintptr_t callSiteAddr, uintptr_t frameFp, const RootVisitor &v0,
                              const RootRangeVisitor &v1, panda::ecmascript::ChunkVector<DerivedData> *data,
                              [[maybe_unused]] bool isVerifying) const;
private:
    LLVMStackMapParser()
    {
        stackMapAddr_ = nullptr;
        callSiteInfos_.clear();
        dataInfo_ = nullptr;
    }
    ~LLVMStackMapParser()
    {
        if (stackMapAddr_) {
            stackMapAddr_.release();
        }
        callSiteInfos_.clear();
        dataInfo_ = nullptr;
    }
    void CalcCallSite();
    std::unique_ptr<uint8_t[]> stackMapAddr_;
    struct LLVMStackMap llvmStackMap_;
    std::unordered_map<uintptr_t, DwarfRegAndOffsetTypeVector> callSiteInfos_;
    [[maybe_unused]] std::unique_ptr<DataInfo> dataInfo_;
};
} // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_LLVMSTACKPARSE_H