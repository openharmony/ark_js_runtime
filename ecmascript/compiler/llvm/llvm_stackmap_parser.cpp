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

#include <fstream>
#include <iostream>
#include <string>
#include "ecmascript/compiler/compiler_macros.h"
#include "ecmascript/frames.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/slots.h"

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

const DwarfRegAndOffsetTypeVector* LLVMStackMapParser::StackMapByAddr(uintptr_t callSiteAddr) const
{
    auto it = callSiteInfos_.find(callSiteAddr);
    if (it != callSiteInfos_.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool LLVMStackMapParser::StackMapByFuncAddrFp(uintptr_t callSiteAddr, uintptr_t frameFp,
                                              const RootVisitor &v0, const RootRangeVisitor &v1,
                                              panda::ecmascript::ChunkVector<DerivedData> *data,
                                              [[maybe_unused]] bool isVerifying) const
{
    const DwarfRegAndOffsetTypeVector *infos = StackMapByAddr(callSiteAddr);
    if (infos == nullptr) {
        return false;
    }
    uintptr_t *fp = reinterpret_cast<uintptr_t *>(frameFp);
    uintptr_t address = 0;
    uintptr_t base = 0;
    uintptr_t derived = 0;
    int i = 0;
    for (auto &info: *infos) {
        if (info.first == panda::ecmascript::FrameConst::SP_DWARF_REG_NUM) {
#ifdef PANDA_TARGET_ARM64
            uintptr_t *curFp = reinterpret_cast<uintptr_t *>(*fp);
            uintptr_t *rsp = reinterpret_cast<uintptr_t *>(*(curFp + panda::ecmascript::FrameConst::SP_OFFSET));
#else
            uintptr_t *rsp = fp + panda::ecmascript::FrameConst::SP_OFFSET;
#endif
            address = reinterpret_cast<uintptr_t>(rsp) + info.second;
#if ECMASCRIPT_ENABLE_COMPILER_LOG
            LOG_ECMA(DEBUG) << "SP_DWARF_REG_NUM:  info.second:" << info.second << " rbp offset:" <<
                reinterpret_cast<uintptr_t>(*fp) - address << "rsp :" << rsp;
#endif
        } else if (info.first == panda::ecmascript::FrameConst::FP_DWARF_REG_NUM) {
            uintptr_t tmpFp = *fp;
            address = tmpFp + info.second;
#if ECMASCRIPT_ENABLE_COMPILER_LOG
            LOG_ECMA(DEBUG) << "FP_DWARF_REG_NUM:  info.second:" << info.second;
#endif
        } else {
#if ECMASCRIPT_ENABLE_COMPILER_LOG
            LOG_ECMA(DEBUG) << "REG_NUM :  info.first:" << info.first;
#endif
            abort();
        }

        if (i & 0x1) {
            derived = reinterpret_cast<uintptr_t>(address);
            if (base == derived) {
#if ECMASCRIPT_ENABLE_COMPILER_LOG
                LOG_ECMA(DEBUG) << std::hex << "visit base:" << base << " base Value: " <<
                    *reinterpret_cast<uintptr_t *>(base);
#endif
                v0(panda::ecmascript::Root::ROOT_FRAME, panda::ecmascript::ObjectSlot(base));
            } else {
#if ECMASCRIPT_ENABLE_COMPILER_LOG
                LOG_ECMA(DEBUG) << std::hex << "push base:" << base << " base Value: " <<
                    *reinterpret_cast<uintptr_t *>(base) << " derived:" << derived;
#endif
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                if (!isVerifying) {
#endif
                    data->emplace_back(std::make_tuple(base, *reinterpret_cast<uintptr_t *>(base), derived));
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                }
#endif
            }
        } else {
            base = reinterpret_cast<uintptr_t>(address);
        }
        i++;
    }
    return true;
}

void LLVMStackMapParser::CalcCallSite()
{
    uint64_t recordNum = 0;
    auto calStkMapRecordFunc = [this, &recordNum](uintptr_t address, int recordId) {
        struct StkMapRecordHeadTy recordHead = llvmStackMap_.StkMapRecord[recordNum + recordId].head;
        for (int j = 0; j < recordHead.NumLocations; j++) {
                struct LocationTy loc = llvmStackMap_.StkMapRecord[recordNum + recordId].Locations[j];
                uint32_t instructionOffset = recordHead.InstructionOffset;
                uintptr_t callsite = address + instructionOffset;
                if (loc.location == LocationTy::Kind::INDIRECT) {
#if ECMASCRIPT_ENABLE_COMPILER_LOG
                    LOG_ECMA(DEBUG) << "DwarfRegNum:" << loc.DwarfRegNum << " loc.OffsetOrSmallConstant:" <<
                        loc.OffsetOrSmallConstant << "address:" << address << " instructionOffset:" <<
                        instructionOffset << " callsite:" << callsite;
#endif
                    DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                    auto it = callSiteInfos_.find(callsite);
                    if (callSiteInfos_.find(callsite) == callSiteInfos_.end()) {
                        callSiteInfos_.insert(std::pair<uintptr_t, DwarfRegAndOffsetTypeVector>(callsite, {info}));
                    } else {
                        it->second.emplace_back(info);
                    }
                }
        }
    };
    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t address =  llvmStackMap_.StkSizeRecords[i].functionAddress;
        uint64_t recordCount = llvmStackMap_.StkSizeRecords[i].recordCount;
        for (uint64_t k = 0; k < recordCount; k++) {
            calStkMapRecordFunc(address, k);
        }
        recordNum += recordCount;
    }
}

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr)
{
    stackMapAddr_ = std::move(stackMapAddr);
    if (!stackMapAddr_) {
        LOG_ECMA(ERROR) << "stackMapAddr_ nullptr error ! " << std::endl;
        return false;
    }
    dataInfo_ = std::make_unique<DataInfo>(std::move(stackMapAddr_));
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

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr,
    uintptr_t hostCodeSectionAddr, uintptr_t deviceCodeSectionAddr)
{
    bool ret = CalculateStackMap(std::move(stackMapAddr));
    if (!ret) {
        return ret;
    }
    // update functionAddress from host side to device side
#if ECMASCRIPT_ENABLE_COMPILER_LOG
    LOG_ECMA(DEBUG) << "stackmap calculate update funcitonaddress ";
#endif
    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t hostAddr = llvmStackMap_.StkSizeRecords[i].functionAddress;
        uintptr_t deviceAddr = hostAddr - hostCodeSectionAddr + deviceCodeSectionAddr;
        llvmStackMap_.StkSizeRecords[i].functionAddress = deviceAddr;
#if ECMASCRIPT_ENABLE_COMPILER_LOG
        LOG_ECMA(DEBUG) << std::dec << i << "th function " << std::hex << hostAddr << " ---> " << deviceAddr;
#endif
    }
    callSiteInfos_.clear();
    CalcCallSite();
    return true;
}
}  // namespace kungfu
