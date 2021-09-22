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
#include <set>
#include <vector>

namespace kungfu {
using OffsetType = int32_t;
using DwarfRegType = uint16_t;
using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;

struct Header {
    uint8_t  stackmapversion; // Stack Map Version (current version is 3)
    uint8_t  Reserved0; // Reserved (expected to be 0)
    uint16_t Reserved1; // Reserved (expected to be 0)
    void Print() const
    {
        std::cout << "----- head ----" << std::endl;
        std::cout << "   version:" << stackmapversion << std::endl;
        std::cout << "+++++ head ++++" << std::endl;
    }
};

struct StkSizeRecordTy {
    uint64_t functionAddress;
    uint64_t stackSize;
    uint64_t recordCount;
    void Print() const
    {
            std::cout << "               functionAddress:0x" << std::hex << functionAddress << std::endl;
            std::cout << "               stackSize:" << std::dec << stackSize << std::endl;
            std::cout << "               recordCount:" << std::dec << recordCount << std::endl;
    }
};

struct ConstantsTy {
    uint64_t LargeConstant;
    void Print() const
    {
        std::cout << "               LargeConstant:" << LargeConstant << std::endl;
    }
};

struct StkMapRecordHeadTy {
    uint64_t PatchPointID;
    uint32_t InstructionOffset;
    uint16_t Reserved;
    uint16_t NumLocations;
    void Print() const
    {
        std::cout << "               PatchPointID:" << std::hex << PatchPointID << std::endl;
        std::cout << "               instructionOffset:" << std::hex << InstructionOffset << std::endl;
        std::cout << "               Reserved:" << Reserved << std::endl;
        std::cout << "               NumLocations:" << NumLocations << std::endl;
    }
};

struct LocationTy {
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

    std::string TypeToString(Kind location) const;

    void Print() const
    {
        std::cout  << TypeToString(location);
        std::cout << ", size:" << std::dec << LocationSize;
        std::cout << "\tDwarfRegNum:" << DwarfRegNum;
        std::cout << "\t OffsetOrSmallConstant:" << OffsetOrSmallConstant << std::endl;
    }
};

struct LiveOutsTy {
    DwarfRegType DwarfRegNum;
    uint8_t Reserved;
    uint8_t SizeinBytes;
    void Print() const
    {
        std::cout << "                  Dwarf RegNum:" << DwarfRegNum << std::endl;
        std::cout << "                  Reserved:" << Reserved << std::endl;
        std::cout << "                  SizeinBytes:" << SizeinBytes << std::endl;
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
            std::cout << "                   #" << std::dec << i << ":";
            Locations[i].Print();
        }
        size = LiveOuts.size();
        for (size_t i = 0; i < size; i++) {
            std::cout << "               liveOuts[" << i << "] info:" << std::endl;
        }
    }
};

class DataInfo {
public:
    explicit DataInfo(const uint8_t* data): data_(data), offset(0) {}
    ~DataInfo()
    {
        data_ = nullptr;
        offset = 0;
    }
    template<class T>
    T Read()
    {
        T t = *reinterpret_cast<const T*>(data_ + offset);
        offset += sizeof(T);
        return t;
    }
    unsigned int GetOffset() const
    {
        return offset;
    }
private:
    const uint8_t* data_;
    unsigned int offset;
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
            std::cout << "stkSizeRecord[" << i << "] info:" << std::endl;
            StkSizeRecords[i].Print();
        }
        for (size_t i = 0; i < Constants.size(); i++) {
            std::cout << "constants[" << i << "] info:" << std::endl;
            Constants[i].Print();
        }
        for (size_t i = 0; i < StkMapRecord.size(); i++) {
            std::cout << "StkMapRecord[" << i << "] info:" << std::endl;
            StkMapRecord[i].Print();
        }
    }
};

class LLVMStackMapParse {
public:
    static LLVMStackMapParse& GetInstance()
    {
        static LLVMStackMapParse instance;
        return instance;
    }
    bool CalculateStackMap(const uint8_t *stackMapAddr);
    void Print() const
    {
        llvmStackMap_.Print();
    }
    bool StackMapByAddr(uintptr_t funcAddr, DwarfRegAndOffsetType &info);
private:
    LLVMStackMapParse()
    {
        stackMapAddr_ = nullptr;
        callSiteInfos_.clear();
        dataInfo_ = nullptr;
    }
    ~LLVMStackMapParse()
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