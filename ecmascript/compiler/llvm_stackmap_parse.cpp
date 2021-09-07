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

#include "ecmascript/compiler/llvm_stackmap_parse.h"
#include <iostream>
#include <fstream>
#include <string>

namespace kungfu {
std::string LocationTy::TypeToString(Kind location) const
{
    switch (location) {
        case Kind::REGISTER:
            return "Register	Reg	Value in a register";
        case Kind::DIRECT:
            return "Direct	Reg + Offset	Frame index value";
        case Kind::INDIRECT:
            return "Indirect	[Reg + Offset]	Spilled value";
        case Kind::CONSTANT:
            return "Constant	Offset	Small constant";
        case Kind::CONSTANTNDEX:
            return "ConstIndex	constants[Offset]	Large constant";
        default:
            return "no know location";
    }
}

bool LLVMStackMapParse::StackMapByAddr(uintptr_t funcAddr, DwarfRegAndOffsetType &info)
{
    for (auto it: callSiteInfos_) {
        if (it.first == funcAddr) {
            info = it.second;
            return true;
        }
    }
    return false;
}

void LLVMStackMapParse::CalcCallSite()
{
    uint64_t recordNum = 0;
    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t address =  llvmStackMap_.StkSizeRecords[i].functionAddress;
        std::cout << std::hex << "address " << address << std::endl;
        uint64_t recordCount = llvmStackMap_.StkSizeRecords[i].recordCount;
        std::cout << std::hex << "recordCount " << recordCount << std::endl;
        recordNum += recordCount;
        std::cout << std::hex << "recordNum " << recordNum << std::endl;
        struct StkMapRecordHeadTy recordHead = llvmStackMap_.StkMapRecord[recordNum - 1].head;
        uint32_t instructionOffset = recordHead.InstructionOffset;
        std::cout << std::hex << "instructionOffset " << instructionOffset << std::endl;
        uintptr_t callsite = address + instructionOffset;

        for (int j = 0; j < recordHead.NumLocations; j++) {
            struct LocationTy loc = llvmStackMap_.StkMapRecord[recordNum - 1].Locations[j];
            if (loc.location == LocationTy::Kind::INDIRECT) {
                DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                Fun2InfoType callSiteInfo {callsite, info};
                callSiteInfos_.push_back(callSiteInfo);
            }
        }
    }
}

bool LLVMStackMapParse::CalculateStackMap(const uint8_t *stackMapAddr)
{
    stackMapAddr_ = stackMapAddr;
    if (!stackMapAddr_) {
        std::cerr << "stackMapAddr_ nullptr error ! " << std::endl;
        return false;
    }
    dataInfo_ = std::make_unique<DataInfo>(stackMapAddr_);
    llvmStackMap_.head = dataInfo_->Read<struct Header>();
    uint32_t numFunctions, numConstants, numRecords;
    numFunctions = dataInfo_->Read<uint32_t>();
    numConstants = dataInfo_->Read<uint32_t>();
    numRecords = dataInfo_->Read<uint32_t>();
    for (uint32_t i = 0; i < numFunctions; i++) {
        auto stkRecord = dataInfo_->Read<struct StkSizeRecordTy>();
        llvmStackMap_.StkSizeRecords.push_back(stkRecord);
    }
    for (uint32_t i = 0; i < numConstants; i++) {
        auto val = dataInfo_->Read<struct ConstantsTy>();
        llvmStackMap_.Constants.push_back(val);
    }
    for (uint32_t i = 0; i < numRecords; i++) {
        struct StkMapRecordTy stkSizeRecord;
        auto head = dataInfo_->Read<struct StkMapRecordHeadTy>();
        stkSizeRecord.head = head;
        for (uint16_t j = 0; j < head.NumLocations; j++) {
            auto location = dataInfo_->Read<struct LocationTy>();
            stkSizeRecord.Locations.push_back(location);
        }
        uint16_t padding;
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            padding = dataInfo_->Read<uint16_t>();
        }
        uint32_t numLiveOuts = dataInfo_->Read<uint32_t>();
        if (numLiveOuts > 0) {
            for (uint32_t j = 0; j < numLiveOuts; j++) {
                auto liveOut = dataInfo_->Read<struct LiveOutsTy>();
                stkSizeRecord.LiveOuts.push_back(liveOut);
            }
        }
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            padding = dataInfo_->Read<uint16_t>();
        }
        llvmStackMap_.StkMapRecord.push_back(stkSizeRecord);
    }
    CalcCallSite();
    return true;
}
}  // namespace kungfu