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

#include "ecmascript/llvm_stackmap_parser.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/frames.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"

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
    for (auto &pc2CallSiteInfo: pc2CallSiteInfoVec_) {
        auto it = pc2CallSiteInfo.find(callSiteAddr);
        if (it != pc2CallSiteInfo.end()) {
            return &(it->second);
        }
    }
    return nullptr;
}

void LLVMStackMapParser::PrintCallSiteSlotAddr(const CallSiteInfo& callsiteInfo, uintptr_t callSiteSp,
    uintptr_t callsiteFp) const
{
    bool flag = (callsiteInfo.size() % 2 != 0);
    size_t j = flag ? 1 : 0; // skip first element when size is odd number
    for (; j < callsiteInfo.size(); j += 2) { // 2: base and derived
        const DwarfRegAndOffsetType baseInfo = callsiteInfo[j];
        const DwarfRegAndOffsetType derivedInfo = callsiteInfo[j + 1];
        COMPILER_LOG(DEBUG) << std::hex << " callSiteSp:0x" << callSiteSp << " callsiteFp:" << callsiteFp;
        COMPILER_LOG(DEBUG) << std::dec << "base DWARF_REG:" << baseInfo.first
                    << " OFFSET:" << baseInfo.second;
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        if (base != derived) {
            COMPILER_LOG(DEBUG) << std::dec << "derived DWARF_REG:" << derivedInfo.first
                << " OFFSET:" << derivedInfo.second;
        }
    }
}
void LLVMStackMapParser::PrintCallSiteInfo(const CallSiteInfo *infos, OptimizedLeaveFrame *frame) const
{
    uintptr_t callSiteSp = frame->GetCallSiteSp();
    uintptr_t callsiteFp = frame->callsiteFp;
    ASSERT(infos != nullptr);
    PrintCallSiteSlotAddr(*infos, callSiteSp, callsiteFp);
}

void LLVMStackMapParser::PrintCallSiteInfo(const CallSiteInfo *infos, uintptr_t callsiteFp, uintptr_t callSiteSp) const
{
    if (!IsLogEnabled()) {
        return;
    }

    CallSiteInfo callsiteInfo = *infos;
    PrintCallSiteSlotAddr(*infos, callSiteSp, callsiteFp);
}

uintptr_t LLVMStackMapParser::GetStackSlotAddress(const DwarfRegAndOffsetType info,
    uintptr_t callSiteSp, uintptr_t callsiteFp) const
{
    uintptr_t address = 0;
    if (info.first == GCStackMapRegisters::SP) {
        address = callSiteSp + info.second;
    } else if (info.first == GCStackMapRegisters::FP) {
        address = callsiteFp + info.second;
    } else {
        UNREACHABLE();
    }
    return address;
}

void LLVMStackMapParser::CollectBaseAndDerivedPointers(const CallSiteInfo* infos, std::set<uintptr_t> &baseSet,
    ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying,
    uintptr_t callsiteFp, uintptr_t callSiteSp) const
{
    bool flag = (infos->size() % 2 != 0);
    size_t j = flag ? 1 : 0; // skip first element when size is odd number
    for (; j < infos->size(); j += 2) { // 2: base and derived
        const DwarfRegAndOffsetType& baseInfo = infos->at(j);
        const DwarfRegAndOffsetType& derivedInfo = infos->at(j + 1);
        uintptr_t base = GetStackSlotAddress(baseInfo, callSiteSp, callsiteFp);
        uintptr_t derived = GetStackSlotAddress(derivedInfo, callSiteSp, callsiteFp);
        baseSet.emplace(base);
        if (base != derived) {
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                if (!isVerifying) {
#endif
                    data->emplace(std::make_pair(base, derived),  *reinterpret_cast<uintptr_t *>(base));
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
                }
#endif
        }
    }
}

bool LLVMStackMapParser::CollectGCSlots(uintptr_t callSiteAddr, uintptr_t callsiteFp,
    std::set<uintptr_t> &baseSet, ChunkMap<DerivedDataKey, uintptr_t> *data, [[maybe_unused]] bool isVerifying,
    uintptr_t callSiteSp) const
{
    const CallSiteInfo *infos = GetCallSiteInfoByPc(callSiteAddr);
    if (infos == nullptr) {
        return false;
    }
    ASSERT(callsiteFp != callSiteSp);
    CollectBaseAndDerivedPointers(infos, baseSet, data, isVerifying, callsiteFp, callSiteSp);

    if (IsLogEnabled()) {
        PrintCallSiteInfo(infos, callsiteFp, callSiteSp);
    }
    return true;
}

void LLVMStackMapParser::CalcCallSite()
{
    uint64_t recordNum = 0;
    Pc2CallSiteInfo pc2CallSiteInfo;
    Pc2ConstInfo pc2ConstInfo;
    Pc2DeoptBundle deoptbundles;
    auto calStkMapRecordFunc =
        [this, &recordNum, &pc2CallSiteInfo, &pc2ConstInfo, &deoptbundles](uintptr_t address, uint32_t recordId) {
        struct StkMapRecordHeadTy recordHead = llvmStackMap_.StkMapRecord[recordNum + recordId].head;
        int lastDeoptIndex = -1;
        for (int j = 0; j < recordHead.NumLocations; j++) {
            struct LocationTy loc = llvmStackMap_.StkMapRecord[recordNum + recordId].Locations[j];
            uint32_t instructionOffset = recordHead.InstructionOffset;
            uintptr_t callsite = address + instructionOffset;
            uint64_t  patchPointID = recordHead.PatchPointID;
            if (j == LocationTy::CONSTANT_DEOPT_CNT_INDEX) {
                ASSERT(loc.location == LocationTy::Kind::CONSTANT);
                lastDeoptIndex = loc.OffsetOrSmallConstant + LocationTy::CONSTANT_DEOPT_CNT_INDEX;
            }
            if (loc.location == LocationTy::Kind::INDIRECT) {
                COMPILER_OPTIONAL_LOG(DEBUG) << "DwarfRegNum:" << loc.DwarfRegNum << " loc.OffsetOrSmallConstant:"
                    << loc.OffsetOrSmallConstant << "address:" << address << " instructionOffset:" <<
                    instructionOffset << " callsite:" << "  patchPointID :" << std::hex << patchPointID <<
                    callsite;
                DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                ASSERT(loc.DwarfRegNum == GCStackMapRegisters::SP || loc.DwarfRegNum == GCStackMapRegisters::FP);
                auto it = pc2CallSiteInfo.find(callsite);
                if (j > lastDeoptIndex) {
                    if (pc2CallSiteInfo.find(callsite) == pc2CallSiteInfo.end()) {
                        pc2CallSiteInfo.insert(std::pair<uintptr_t, CallSiteInfo>(callsite, {info}));
                    } else {
                        it->second.emplace_back(info);
                    }
                } else if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX) {
                    deoptbundles[callsite].push_back(info);
                }
            } else if (loc.location == LocationTy::Kind::CONSTANT) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    pc2ConstInfo[callsite].push_back(loc.OffsetOrSmallConstant);
                    deoptbundles[callsite].push_back(loc.OffsetOrSmallConstant);
                }
            } else if (loc.location == LocationTy::Kind::DIRECT) {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    ASSERT(loc.DwarfRegNum == GCStackMapRegisters::SP || loc.DwarfRegNum == GCStackMapRegisters::FP);
                    DwarfRegAndOffsetType info(loc.DwarfRegNum, loc.OffsetOrSmallConstant);
                    deoptbundles[callsite].push_back(info);
                }
            } else {
                if (j >= LocationTy::CONSTANT_FIRST_ELEMENT_INDEX && j <= lastDeoptIndex) {
                    LargeInt v = static_cast<LargeInt>(llvmStackMap_.
                        Constants[loc.OffsetOrSmallConstant].LargeConstant);
                    deoptbundles[callsite].push_back(v);
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
    pc2CallSiteInfoVec_.emplace_back(pc2CallSiteInfo);
    pc2ConstInfoVec_.emplace_back(pc2ConstInfo);
    pc2DeoptBundleVec_.emplace_back(deoptbundles);
}

bool LLVMStackMapParser::CalculateStackMap(std::unique_ptr<uint8_t []> stackMapAddr)
{
    if (!stackMapAddr) {
        COMPILER_LOG(ERROR) << "stackMapAddr nullptr error ! ";
        return false;
    }
    dataInfo_ = std::make_unique<DataInfo>(std::move(stackMapAddr));
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
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            dataInfo_->Read<uint16_t>();
        }
        uint32_t numLiveOuts = dataInfo_->Read<uint32_t>();
        if (numLiveOuts > 0) {
            for (uint32_t j = 0; j < numLiveOuts; j++) {
                auto liveOut = dataInfo_->Read<struct LiveOutsTy>();
                stkSizeRecord.LiveOuts.push_back(liveOut);
            }
        }
        while (dataInfo_->GetOffset() & 7) { // 7: 8 byte align
            dataInfo_->Read<uint16_t>();
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
    COMPILER_OPTIONAL_LOG(DEBUG) << "stackmap calculate update funcitonaddress ";

    for (size_t i = 0; i < llvmStackMap_.StkSizeRecords.size(); i++) {
        uintptr_t hostAddr = llvmStackMap_.StkSizeRecords[i].functionAddress;
        uintptr_t deviceAddr = hostAddr - hostCodeSectionAddr + deviceCodeSectionAddr;
        llvmStackMap_.StkSizeRecords[i].functionAddress = deviceAddr;
        COMPILER_OPTIONAL_LOG(DEBUG) << std::dec << i << "th function " << std::hex << hostAddr << " ---> "
                                     << deviceAddr;
    }
    CalcCallSite();
    return true;
}

void LLVMStackMapParser::CalculateFuncFpDelta(Func2FpDelta info)
{
    bool find = std::find(fun2FpDelta_.begin(), fun2FpDelta_.end(), info) == fun2FpDelta_.end();
    if (!info.empty() && find) {
        fun2FpDelta_.emplace_back(info);
    }
    for (auto &it: info) {
        funAddr_.insert(it.first);
    }
}

int LLVMStackMapParser::FindFpDelta(uintptr_t funcAddr, uintptr_t callsitePc) const
{
    int delta = 0;
    // next optimization can be performed via sorted/map.
    for (auto &info: fun2FpDelta_) {
        if (info.find(funcAddr) != info.end()) {
            delta = info.at(funcAddr).first;
            uint32_t funcSize = info.at(funcAddr).second;
            if (callsitePc <= funcAddr + funcSize && callsitePc >= funcAddr) {
                return delta;
            }
        }
    }
    return delta;
}

int LLVMStackMapParser::GetFuncFpDelta(uintptr_t callsitePc) const
{
    int delta = 0;
    auto itupper = funAddr_.upper_bound(callsitePc);
    if (itupper != funAddr_.end()) { // find first element >= callsitePc
        --itupper;
        // callsitePC may jscall or entry, thus not existed in funAddr_
        if ((itupper == funAddr_.end()) || (*itupper > callsitePc)) {
            return delta;
        }
        delta = FindFpDelta(*itupper, callsitePc);
    } else {
        auto rit = funAddr_.crbegin(); // find last element
        // callsitePC may jscall or entry, thus not existed in funAddr_
        if ((rit == funAddr_.crend()) || (*rit > callsitePc)) {
            return delta;
        }
        delta = FindFpDelta(*rit, callsitePc);
    }
    return delta;
}
}  // namespace panda::ecmascript::kungfu
