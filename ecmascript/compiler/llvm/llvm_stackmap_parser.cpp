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
#include "ecmascript/compiler/compiler_macros.h"
#include "ecmascript/frames.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/slots.h"

using namespace panda::ecmascript;

namespace panda::ecmascript::kungfu {
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

const CallSiteInfo* LLVMStackMapParser::GetCallSiteInfoByPc(uintptr_t callSiteAddr) const
{
    auto it = pc2CallSiteInfo_.find(callSiteAddr);
    if (it != pc2CallSiteInfo_.end()) {
        return &(it->second);
    }
    return nullptr;
}

const CallSiteInfo* LLVMStackMapParser::GetCallSiteInfoByPatchID(uint64_t patchPointId) const
{
    auto it = pid2CallSiteInfo_.find(patchPointId);
    if (it != pid2CallSiteInfo_.end()) {
        return &(it->second);
    }
    return nullptr;
}

void LLVMStackMapParser::PrintCallSiteInfo(const CallSiteInfo *infos, OptimizedLeaveFrame *frame) const
{
    int i = 0;
    uintptr_t address = 0;
    uintptr_t base = 0;
    uintptr_t derived = 0;
    for (auto &info: *infos) {
        if (info.first == FrameConstants::SP_DWARF_REG_NUM) {
            uintptr_t rsp = frame->GetCallSiteSp();
            address = rsp + info.second;
            LOG_ECMA(DEBUG) << std::dec << "SP_DWARF_REG_NUM:  info.second:" << info.second
                            << std::hex << "rsp :" << rsp;
        } else if (info.first == FrameConstants::FP_DWARF_REG_NUM) {
            ASSERT(frame->type != FrameType::ASM_LEAVE_FRAME);
            uintptr_t fp = frame->callsiteFp;
            address = fp + info.second;
            LOG_ECMA(DEBUG) << std::dec << "FP_DWARF_REG_NUM:  info.second:" << info.second
                            << std::hex << "rfp :" << fp;
        } else {
            LOG_ECMA(DEBUG) << "REG_NUM :  info.first:" << info.first;
            UNREACHABLE();
        }

        if (IsDeriveredPointer(i)) {
            derived = reinterpret_cast<uintptr_t>(address);
            if (base == derived) {
                LOG_ECMA(INFO) << std::hex << "visit base:" << base << " base Value: " <<
                    *reinterpret_cast<uintptr_t *>(base);
            } else {
                LOG_ECMA(INFO) << std::hex << "push base:" << base << " base Value: " <<
                    *reinterpret_cast<uintptr_t *>(base) << " derived:" << derived;
            }
        } else {
            base = reinterpret_cast<uintptr_t>(address);
        }
        i++;
    }
    i = 0;
}

void LLVMStackMapParser::PrintCallSiteInfo(const CallSiteInfo *infos, uintptr_t *fp) const
{
    int i = 0;
    uintptr_t address = 0;
    uintptr_t base = 0;
    uintptr_t derived = 0;

    uintptr_t callsiteFp = *fp;
    uintptr_t callsiteSp = *(reinterpret_cast<uintptr_t *>(callsiteFp) + FrameConstants::CALLSITE_SP_TO_FP_DELTA);

    for (auto &info: *infos) {
        if (info.first == FrameConstants::SP_DWARF_REG_NUM) {
            address = callsiteSp + info.second;
        } else if (info.first == FrameConstants::FP_DWARF_REG_NUM) {
            address = callsiteFp + info.second;
        } else {
            UNREACHABLE();
        }

        if (IsDeriveredPointer(i)) {
            derived = reinterpret_cast<uintptr_t>(address);
            if (base == derived) {
                LOG_ECMA(DEBUG) << std::hex << "visit base:" << base << " base Value: " <<
                    *reinterpret_cast<uintptr_t *>(base);
            } else {
                LOG_ECMA(DEBUG) << std::hex << "push base:" << base << " base Value: " <<
                    *reinterpret_cast<uintptr_t *>(base) << " derived:" << derived;
            }
        } else {
            base = reinterpret_cast<uintptr_t>(address);
        }
        i++;
    }
}

bool LLVMStackMapParser::IsDeriveredPointer(int callsitetime) const
{
    return callsitetime & 1;
}

bool LLVMStackMapParser::CollectStackMapSlots(uintptr_t callSiteAddr, uintptr_t frameFp,
    std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying) const
{
    const CallSiteInfo *infos = GetCallSiteInfoByPc(callSiteAddr);
    if (infos == nullptr) {
        return false;
    }
    uintptr_t *fp = reinterpret_cast<uintptr_t *>(frameFp);
    uintptr_t address = 0;
    uintptr_t base = 0;
    uintptr_t derived = 0;
    int i = 0;
#if ECMASCRIPT_ENABLE_COMPILER_LOG
    PrintCallSiteInfo(infos, fp);
#endif
    uintptr_t callsiteFp = *fp;
    uintptr_t callsiteSp;
    if (FrameHandler(reinterpret_cast<JSTaggedType *>(frameFp)).GetFrameType() == FrameType::ASM_LEAVE_FRAME) {
        callsiteSp = OptimizedLeaveFrame::GetFrameFromSp(reinterpret_cast<JSTaggedType *>(frameFp))->GetCallSiteSp();
    } else {
        callsiteSp = callsiteFp + FrameConstants::CALLSITE_SP_TO_FP_DELTA_OFFSET;
    }

    for (auto &info: *infos) {
        if (info.first == FrameConstants::SP_DWARF_REG_NUM) {
            address = callsiteSp + info.second;
        } else if (info.first == FrameConstants::FP_DWARF_REG_NUM) {
            address = callsiteFp + info.second;
        } else {
            UNREACHABLE();
        }

        if (IsDeriveredPointer(i)) {
            derived = reinterpret_cast<uintptr_t>(address);
            if (base == derived) {
                baseSet.emplace(base);
            } else {
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                if (!isVerifying) {
#endif
                    data->emplace(std::make_pair(base, derived),  *reinterpret_cast<uintptr_t *>(base));
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
                uint64_t  patchPointID = recordHead.PatchPointID;
                if (loc.location == LocationTy::Kind::INDIRECT) {
#if ECMASCRIPT_ENABLE_COMPILER_LOG
                    LOG_ECMA(DEBUG) << "DwarfRegNum:" << loc.DwarfRegNum << " loc.OffsetOrSmallConstant:" <<
                        loc.OffsetOrSmallConstant << "address:" << address << " instructionOffset:" <<
                        instructionOffset << " callsite:" << "  patchPointID :" << std::hex << patchPointID << callsite;
#endif
                    DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                    auto it = pc2CallSiteInfo_.find(callsite);
                    if (pc2CallSiteInfo_.find(callsite) == pc2CallSiteInfo_.end()) {
                        pc2CallSiteInfo_.insert(std::pair<uintptr_t, CallSiteInfo>(callsite, {info}));
                    } else {
                        it->second.emplace_back(info);
                    }

                    auto it2 = pid2CallSiteInfo_.find(patchPointID);
                    if (pid2CallSiteInfo_.find(patchPointID) == pid2CallSiteInfo_.end()) {
                        pid2CallSiteInfo_.insert(std::pair<uint64_t, CallSiteInfo>(patchPointID,
                            {info}));
                    } else {
                        it2->second.emplace_back(info);
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
    pc2CallSiteInfo_.clear();
    pid2CallSiteInfo_.clear();
    CalcCallSite();
    return true;
}
}  // namespace panda::ecmascript::kungfu
