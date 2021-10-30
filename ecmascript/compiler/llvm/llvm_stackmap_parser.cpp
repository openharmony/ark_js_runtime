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

#include "llvm_stackmap_parser.h"
#include <iostream>
#include <fstream>
#include <string>

namespace kungfu {
std::string LocationTy::TypeToString(Kind loc) const
{
    switch (loc) {
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

bool LLVMStackMapParser::StackMapByAddr(uintptr_t funcAddr, DwarfRegAndOffsetTypeVector &infos)
{
    bool found = false;
    for (auto it: callSiteInfos_) {
        LOG_ECMA(INFO) << __FUNCTION__ << std::hex <<  " addr:" << it.first << std::endl;
        if (it.first == funcAddr) {
            DwarfRegAndOffsetType info = it.second;
            LOG_ECMA(INFO) << __FUNCTION__ << " info <" << info.first << " ," << info.second << " >" << std::endl;
            infos.push_back(info);
            found = true;
        }
    }
    return found;
}

bool LLVMStackMapParser::StackMapByFuncAddrFp(uintptr_t funcAddr, uintptr_t frameFp,
    std::set<uintptr_t> &slotAddrs)
{
    DwarfRegAndOffsetTypeVector infos;
    if (!StackMapByAddr(funcAddr, infos)) {
        return false;
    }
    uintptr_t *fp = reinterpret_cast<uintptr_t *>(frameFp);
    uintptr_t **address = nullptr;
    for (auto &info: infos) {
        if (info.first == SP_DWARF_REG_NUM) {
            uintptr_t *rsp = fp + SP_OFFSET;
            address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uintptr_t>(rsp) + info.second);
        } else if (info.first == FP_DWARF_REG_NUM) {
            fp = reinterpret_cast<uintptr_t *>(*fp);
            address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(fp) + info.second);
        } else {
            address = nullptr;
            abort();
        }
        LOG_ECMA(INFO) << std::hex << "stackMap ref addr:" << address;
        LOG_ECMA(INFO) << "  value:" << *address;
        LOG_ECMA(INFO) << " *value :" << **address << std::endl;
        slotAddrs.insert(reinterpret_cast<uintptr_t>(address));
    }
    return true;
}

void LLVMStackMapParser::CalcCallSite()
{
    uint64_t recordNum = 0;
    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t address =  llvmStackMap_.StkSizeRecords[i].functionAddress;
        uint64_t recordCount = llvmStackMap_.StkSizeRecords[i].recordCount;
        for (uint64_t k = 0; k < recordCount; k++) {
            struct StkMapRecordHeadTy recordHead = llvmStackMap_.StkMapRecord[recordNum + k].head;
            for (int j = 0; j < recordHead.NumLocations; j++) {
                struct LocationTy loc = llvmStackMap_.StkMapRecord[recordNum + k].Locations[j];
                uint32_t instructionOffset = recordHead.InstructionOffset;
                uintptr_t callsite = address + instructionOffset;
                if (loc.location == LocationTy::Kind::INDIRECT) {
                    DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                    Fun2InfoType callSiteInfo {callsite, info};
                    callSiteInfos_.push_back(callSiteInfo);
                }
            }
        }
        recordNum += recordCount;
    }
}

bool LLVMStackMapParser::CalculateStackMap(const uint8_t *stackMapAddr)
{
    stackMapAddr_ = stackMapAddr;
    if (!stackMapAddr_) {
        LOG_ECMA(ERROR) << "stackMapAddr_ nullptr error ! " << std::endl;
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

bool LLVMStackMapParser::CalculateStackMap(const uint8_t *stackMapAddr,
    uintptr_t hostCodeSectionAddr, uintptr_t deviceCodeSectionAddr)
{
    bool ret = CalculateStackMap(stackMapAddr);
    if (!ret) {
        return ret;
    }
    // update functionAddress from host side to device side
    LOG_ECMA(INFO) << "stackmap calculate update funcitonaddress " << std::endl;
    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t hostAddr = llvmStackMap_.StkSizeRecords[i].functionAddress;
        uintptr_t deviceAddr = hostAddr - hostCodeSectionAddr + deviceCodeSectionAddr;
        llvmStackMap_.StkSizeRecords[i].functionAddress = deviceAddr;
        LOG_ECMA(INFO) << std::dec << i << "th function " << std::hex << hostAddr << " ---> " << deviceAddr
            << std::endl;
    }
    callSiteInfos_.clear();
    CalcCallSite();
    return true;
}
}  // namespace kungfu
