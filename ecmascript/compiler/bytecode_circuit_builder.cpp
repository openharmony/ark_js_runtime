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

#include "bytecode_circuit_builder.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/ts_types/ts_loader.h"

namespace panda::ecmascript::kungfu {
void BytecodeCircuitBuilder::BytecodeToCircuit()
{
    auto curPc = pcArray_.front();
    auto prePc = curPc;
    std::map<uint8_t *, uint8_t *> byteCodeCurPrePc;
    std::vector<CfgInfo> bytecodeBlockInfos;
    auto startPc = curPc;
    bytecodeBlockInfos.emplace_back(startPc, SplitKind::START, std::vector<uint8_t *>(1, startPc));
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(curPc, prePc));
    for (size_t i = 1; i < pcArray_.size() - 1; i++) {
        curPc = pcArray_[i];
        byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(curPc, prePc));
        prePc = curPc;
        CollectBytecodeBlockInfo(curPc, bytecodeBlockInfos);
    }
    // handle empty
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(pcArray_[pcArray_.size() - 1], prePc));

    // collect try catch block info
    auto exceptionInfo = CollectTryCatchBlockInfo(byteCodeCurPrePc, bytecodeBlockInfos);

    // Complete bytecode block Information
    CompleteBytecodeBlockInfo(byteCodeCurPrePc, bytecodeBlockInfos);

    // Building the basic block diagram of bytecode
    BuildBasicBlocks(exceptionInfo, bytecodeBlockInfos, byteCodeCurPrePc);
}

void BytecodeCircuitBuilder::CollectBytecodeBlockInfo(uint8_t *pc, std::vector<CfgInfo> &bytecodeBlockInfos)
{
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8: {
            int8_t offset = static_cast<int8_t>(READ_INST_8_0());
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            // current basic block end
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::TWO, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::TWO));
            // jump basic block start
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JMP_IMM16: {
            int16_t offset = static_cast<int16_t>(READ_INST_16_0());
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::THREE, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::THREE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JMP_IMM32: {
            int32_t offset = static_cast<int32_t>(READ_INST_32_0());
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::FIVE, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::FIVE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + BytecodeOffset::TWO);   // first successor
            int8_t offset = static_cast<int8_t>(READ_INST_8_0());
            temp.emplace_back(pc + offset);  // second successor
            // condition branch current basic block end
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            // first branch basic block start
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::TWO, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::TWO));
            // second branch basic block start
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + BytecodeOffset::THREE);   // first successor
            int16_t offset = static_cast<int16_t>(READ_INST_16_0());
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp); // end
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::THREE, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::THREE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + BytecodeOffset::TWO); // first successor
            int8_t offset = static_cast<int8_t>(READ_INST_8_0());
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::TWO, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::TWO));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + BytecodeOffset::THREE); // first successor
            int8_t offset = static_cast<int8_t>(READ_INST_16_0());
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + BytecodeOffset::THREE, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + BytecodeOffset::THREE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF:
        case EcmaOpcode::THROWDYN_PREF:
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8:
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF:
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF:
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, std::vector<uint8_t *>(1, pc));
            break;
        }
        default:
            break;
    }
}

std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> BytecodeCircuitBuilder::CollectTryCatchBlockInfo(
    std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc, std::vector<CfgInfo> &bytecodeBlockInfos)
{
    // try contains many catch
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> byteCodeException;
    panda_file::MethodDataAccessor mda(*pf_, method_->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf_, mda.GetCodeId().value());
    cda.EnumerateTryBlocks([this, &byteCodeCurPrePc, &bytecodeBlockInfos, &byteCodeException](
            panda_file::CodeDataAccessor::TryBlock &try_block) {
        auto tryStartOffset = try_block.GetStartPc();
        auto tryEndOffset = try_block.GetStartPc() + try_block.GetLength();
        auto tryStartPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + tryStartOffset);
        auto tryEndPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + tryEndOffset);
        byteCodeException[std::make_pair(tryStartPc, tryEndPc)] = {};
        uint32_t pcOffset = panda_file::INVALID_OFFSET;
        try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
            pcOffset = catch_block.GetHandlerPc();
            auto catchBlockPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + pcOffset);
            // try block associate catch block
            byteCodeException[std::make_pair(tryStartPc, tryEndPc)].emplace_back(catchBlockPc);
            return true;
        });
        // Check whether the previous block of the try block exists.
        // If yes, add the current block; otherwise, create a new block.
        bool flag = false;
        for (size_t i = 0; i < bytecodeBlockInfos.size(); i++) {
            if (bytecodeBlockInfos[i].splitKind == SplitKind::START) {
                continue;
            }
            if (bytecodeBlockInfos[i].pc == byteCodeCurPrePc[tryStartPc]) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            // pre block
            bytecodeBlockInfos.emplace_back(byteCodeCurPrePc[tryStartPc], SplitKind::END,
                                            std::vector<uint8_t *>(1, tryStartPc));
        }
        // try block
        bytecodeBlockInfos.emplace_back(tryStartPc, SplitKind::START, std::vector<uint8_t *>(1, tryStartPc));
        flag = false;
        for (size_t i = 0; i < bytecodeBlockInfos.size(); i++) {
            if (bytecodeBlockInfos[i].splitKind == SplitKind::START) {
                continue;
            }
            if (bytecodeBlockInfos[i].pc == byteCodeCurPrePc[tryEndPc]) {
                auto &succs = bytecodeBlockInfos[i].succs;
                auto iter = std::find(succs.cbegin(), succs.cend(), bytecodeBlockInfos[i].pc);
                if (iter == succs.cend()) {
                    auto opcode = static_cast<EcmaOpcode>(*(bytecodeBlockInfos[i].pc));
                    switch (opcode) {
                        case EcmaOpcode::JMP_IMM8:
                        case EcmaOpcode::JMP_IMM16:
                        case EcmaOpcode::JMP_IMM32:
                        case EcmaOpcode::JEQZ_IMM8:
                        case EcmaOpcode::JEQZ_IMM16:
                        case EcmaOpcode::JNEZ_IMM8:
                        case EcmaOpcode::JNEZ_IMM16:
                        case EcmaOpcode::RETURN_DYN:
                        case EcmaOpcode::RETURNUNDEFINED_PREF:
                        case EcmaOpcode::THROWDYN_PREF: {
                            break;
                        }
                        default: {
                            succs.emplace_back(tryEndPc);
                            break;
                        }
                    }
                }
                flag = true;
                break;
            }
        }
        if (!flag) {
            bytecodeBlockInfos.emplace_back(byteCodeCurPrePc[tryEndPc], SplitKind::END,
                                            std::vector<uint8_t *>(1, tryEndPc));
        }
        bytecodeBlockInfos.emplace_back(tryEndPc, SplitKind::START, std::vector<uint8_t *>(1, tryEndPc)); // next block
        return true;
    });
    return byteCodeException;
}

void BytecodeCircuitBuilder::CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc,
                                                       std::vector<CfgInfo> &bytecodeBlockInfos)
{
    std::sort(bytecodeBlockInfos.begin(), bytecodeBlockInfos.end());

    if (IsLogEnabled()) {
        PrintCollectBlockInfo(bytecodeBlockInfos);
    }

    // Deduplicate
    auto deduplicateIndex = std::unique(bytecodeBlockInfos.begin(), bytecodeBlockInfos.end());
    bytecodeBlockInfos.erase(deduplicateIndex, bytecodeBlockInfos.end());

    // Supplementary block information
    std::vector<uint8_t *> endBlockPc;
    std::vector<uint8_t *> startBlockPc;
    for (size_t i = 0; i < bytecodeBlockInfos.size() - 1; i++) {
        if (bytecodeBlockInfos[i].splitKind == bytecodeBlockInfos[i + 1].splitKind &&
            bytecodeBlockInfos[i].splitKind == SplitKind::START) {
            auto prePc = byteCodeCurPrePc[bytecodeBlockInfos[i + 1].pc];
            endBlockPc.emplace_back(prePc); // Previous instruction of current instruction
            endBlockPc.emplace_back(bytecodeBlockInfos[i + 1].pc); // current instruction
            continue;
        }
        if (bytecodeBlockInfos[i].splitKind == bytecodeBlockInfos[i + 1].splitKind &&
            bytecodeBlockInfos[i].splitKind == SplitKind::END) {
            auto tempPc = bytecodeBlockInfos[i].pc;
            auto findItem = std::find_if(byteCodeCurPrePc.cbegin(), byteCodeCurPrePc.cend(),
                                         [tempPc](const std::map<uint8_t *, uint8_t *>::value_type item) {
                                             return item.second == tempPc;
                                         });
            if (findItem != byteCodeCurPrePc.cend()) {
                startBlockPc.emplace_back((*findItem).first);
            }
        }
    }

    // Supplementary end block info
    for (auto iter = endBlockPc.cbegin(); iter != endBlockPc.cend(); iter += 2) { // 2: index
        bytecodeBlockInfos.emplace_back(*iter, SplitKind::END,
                                                          std::vector<uint8_t *>(1, *(iter + 1)));
    }
    // Supplementary start block info
    for (auto iter = startBlockPc.cbegin(); iter != startBlockPc.cend(); iter++) {
        bytecodeBlockInfos.emplace_back(*iter, SplitKind::START, std::vector<uint8_t *>(1, *iter));
    }

    // Deduplicate successor
    for (size_t i = 0; i < bytecodeBlockInfos.size(); i++) {
        if (bytecodeBlockInfos[i].splitKind == SplitKind::END) {
            std::set<uint8_t *> tempSet(bytecodeBlockInfos[i].succs.cbegin(),
                                        bytecodeBlockInfos[i].succs.cend());
            bytecodeBlockInfos[i].succs.assign(tempSet.cbegin(), tempSet.cend());
        }
    }

    std::sort(bytecodeBlockInfos.begin(), bytecodeBlockInfos.end());

    // handling jumps to an empty block
    auto endPc = bytecodeBlockInfos[bytecodeBlockInfos.size() - 1].pc;
    auto iter = --byteCodeCurPrePc.cend();
    if (endPc == iter->first) {
        bytecodeBlockInfos.emplace_back(endPc, SplitKind::END, std::vector<uint8_t *>(1, endPc));
    }
    // Deduplicate
    deduplicateIndex = std::unique(bytecodeBlockInfos.begin(), bytecodeBlockInfos.end());
    bytecodeBlockInfos.erase(deduplicateIndex, bytecodeBlockInfos.end());

    if (IsLogEnabled()) {
        PrintCollectBlockInfo(bytecodeBlockInfos);
    }
}

void BytecodeCircuitBuilder::BuildBasicBlocks(std::map<std::pair<uint8_t *, uint8_t *>,
                                                       std::vector<uint8_t *>> &exception,
                                              std::vector<CfgInfo> &bytecodeBlockInfo,
                                              [[maybe_unused]] std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc)
{
    std::map<uint8_t *, BytecodeRegion *> startPcToBB; // [start, bb]
    std::map<uint8_t *, BytecodeRegion *> endPcToBB; // [end, bb]
    graph_.resize(bytecodeBlockInfo.size() / 2); // 2 : half size
    // build basic block
    int blockId = 0;
    int index = 0;
    for (size_t i = 0; i < bytecodeBlockInfo.size() - 1; i += 2) { // 2:index
        auto startPc = bytecodeBlockInfo[i].pc;
        auto endPc = bytecodeBlockInfo[i + 1].pc;
        auto block = &graph_[index++];
        block->id = blockId++;
        block->start = startPc;
        block->end = endPc;
        block->preds = {};
        block->succs = {};
        startPcToBB[startPc] = block;
        endPcToBB[endPc] = block;
    }

    // add block associate
    for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
        if (bytecodeBlockInfo[i].splitKind == SplitKind::START) {
            continue;
        }
        auto curPc = bytecodeBlockInfo[i].pc;
        auto &successors = bytecodeBlockInfo[i].succs;
        for (size_t j = 0; j < successors.size(); j++) {
            if (successors[j] == curPc) {
                continue;
            }
            auto curBlock = endPcToBB[curPc];
            auto succsBlock = startPcToBB[successors[j]];
            curBlock->succs.emplace_back(succsBlock);
            succsBlock->preds.emplace_back(curBlock);
        }
    }

    // try catch block associate
    for (size_t i = 0; i < graph_.size(); i++) {
        const auto pc = graph_[i].start;
        auto it = exception.cbegin();
        for (; it != exception.cend(); it++) {
            if (pc < it->first.first || pc >= it->first.second) { // try block interval
                continue;
            }
            auto catchs = exception[it->first]; // catchs start pc
            for (size_t j = i + 1; j < graph_.size(); j++) {
                if (std::find(catchs.cbegin(), catchs.cend(), graph_[j].start) != catchs.cend()) {
                    graph_[i].catchs.insert(graph_[i].catchs.cbegin(), &graph_[j]);
                    graph_[i].succs.emplace_back(&graph_[j]);
                    graph_[j].preds.emplace_back(&graph_[i]);
                }
            }
        }
    }

    if (IsLogEnabled()) {
        PrintGraph();
    }
    ComputeDominatorTree();
}

void BytecodeCircuitBuilder::ComputeDominatorTree()
{
    // Construct graph backward order
    std::map<size_t, size_t> bbIdToDfsTimestamp; // (basicblock id, dfs order)
    size_t timestamp = 0;
    std::deque<size_t> pendingList;
    std::vector<size_t> visited(graph_.size(), 0);
    auto basicBlockId = graph_[0].id;
    pendingList.push_back(basicBlockId);
    while (!pendingList.empty()) {
        auto &curBlockId = pendingList.back();
        pendingList.pop_back();
        bbIdToDfsTimestamp[curBlockId] = timestamp++;
        for (auto &succBlock: graph_[curBlockId].succs) {
            if (visited[succBlock->id] == 0) {
                visited[succBlock->id] = 1;
                pendingList.push_back(succBlock->id);
            }
        }
    }

    RemoveDeadRegions(bbIdToDfsTimestamp);

    if (IsLogEnabled()) {
        // print cfg order
        for (auto iter : bbIdToDfsTimestamp) {
            COMPILER_LOG(INFO) << "BB_" << iter.first << " dfs timestamp is : " << iter.second;
        }
    }

    std::vector<size_t> immDom(graph_.size()); // immediate dominator
    std::vector<std::vector<size_t>> doms(graph_.size()); // dominators set
    doms[0] = {0};
    for (size_t i = 1; i < doms.size(); i++) {
        doms[i].resize(doms.size());
        std::iota(doms[i].begin(), doms[i].end(), 0);
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < doms.size(); i++) {
            if (graph_[i].isDead) {
                continue;
            }
            auto &curDom = doms[i];
            size_t curDomSize = curDom.size();
            curDom.resize(doms.size());
            std::iota(curDom.begin(), curDom.end(), 0);
            // traverse the predecessor nodes of the current node, Computing Dominators
            for (auto &preBlock : graph_[i].preds) {
                std::vector<size_t> tmp(curDom.size());
                auto preDom = doms[preBlock->id];
                auto it = std::set_intersection(curDom.begin(), curDom.end(), preDom.begin(), preDom.end(),
                                                tmp.begin());
                tmp.resize(it - tmp.cbegin());
                curDom = tmp;
            }
            auto it = std::find(curDom.cbegin(), curDom.cend(), i);
            if (it == curDom.cend()) {
                curDom.push_back(i);
                std::sort(curDom.begin(), curDom.end());
            }

            if (doms[i].size() != curDomSize) {
                changed = true;
            }
        }
    }

    if (IsLogEnabled()) {
        // print dominators set
        for (size_t i = 0; i < doms.size(); i++) {
            std::string log("block " + std::to_string(i) + " dominator blocks has: ");
            for (auto j: doms[i]) {
                log += std::to_string(j) + " , ";
            }
            COMPILER_LOG(INFO) << log;
        }
    }

    // compute immediate dominator
    immDom[0] = static_cast<size_t>(doms[0].front());
    for (size_t i = 1; i < doms.size(); i++) {
        if (graph_[i].isDead) {
            continue;
        }
        auto it = std::remove(doms[i].begin(), doms[i].end(), i);
        doms[i].resize(it - doms[i].cbegin());
        immDom[i] = static_cast<size_t>(*std::max_element(
            doms[i].cbegin(),
            doms[i].cend(),
            [this, &bbIdToDfsTimestamp](size_t lhs, size_t rhs) -> bool {
                auto lhsTimestamp = bbIdToDfsTimestamp.at(this->graph_[lhs].id);
                auto rhsTimestamp = bbIdToDfsTimestamp.at(this->graph_[rhs].id);
                return lhsTimestamp < rhsTimestamp;
            }));
    }

    if (IsLogEnabled()) {
        // print immediate dominator
        for (size_t i = 0; i < immDom.size(); i++) {
            COMPILER_LOG(INFO) << i << " immediate dominator: " << immDom[i];
        }
        PrintGraph();
    }

    BuildImmediateDominator(immDom);
}

void BytecodeCircuitBuilder::BuildImmediateDominator(const std::vector<size_t> &immDom)
{
    graph_[0].iDominator = &graph_[0];
    for (size_t i = 1; i < immDom.size(); i++) {
        auto dominatedBlock = &graph_[i];
        if (dominatedBlock->isDead) {
            continue;
        }
        auto immDomBlock = &graph_[immDom[i]];
        dominatedBlock->iDominator = immDomBlock;
    }

    if (IsLogEnabled()) {
        for (auto block : graph_) {
            if (block.isDead) {
                continue;
            }
            COMPILER_LOG(INFO) << "current block " << block.id
                               << " immediate dominator block id: " << block.iDominator->id;
        }
    }

    for (auto &block : graph_) {
        if (block.isDead) {
            continue;
        }
        if (block.iDominator->id != block.id) {
            block.iDominator->immDomBlocks.emplace_back(&block);
        }
    }

    if (IsLogEnabled()) {
        for (auto &block : graph_) {
            if (block.isDead) {
                continue;
            }
            std::string log ("block " + std::to_string(block.id) + " dominate block has: ");
            for (size_t i = 0; i < block.immDomBlocks.size(); i++) {
                log += std::to_string(block.immDomBlocks[i]->id) + ",";
            }
            COMPILER_LOG(INFO) << log;
        }
    }

    ComputeDomFrontiers(immDom);
    InsertPhi();
    UpdateCFG();
    BuildCircuit();
}

void BytecodeCircuitBuilder::ComputeDomFrontiers(const std::vector<size_t> &immDom)
{
    std::vector<std::set<BytecodeRegion *>> domFrontiers(immDom.size());
    for (auto &bb : graph_) {
        if (bb.isDead) {
            continue;
        }
        if (bb.preds.size() < 2) { // 2: pred num
            continue;
        }
        for (size_t i = 0; i < bb.preds.size(); i++) {
            auto runner = bb.preds[i];
            while (runner->id != immDom[bb.id]) {
                domFrontiers[runner->id].insert(&bb);
                runner = &graph_[immDom[runner->id]];
            }
        }
    }

    for (size_t i = 0; i < domFrontiers.size(); i++) {
        for (auto iter = domFrontiers[i].cbegin(); iter != domFrontiers[i].cend(); iter++) {
            graph_[i].domFrontiers.emplace_back(*iter);
        }
    }

    if (IsLogEnabled()) {
        for (size_t i = 0; i < domFrontiers.size(); i++) {
            std::string log("basic block " + std::to_string(i) + " dominate Frontiers is: ");
            for (auto iter = domFrontiers[i].cbegin(); iter != domFrontiers[i].cend(); iter++) {
                log += std::to_string((*iter)->id) + ", ";
            }
            COMPILER_LOG(INFO) << log;
        }
    }
}

void BytecodeCircuitBuilder::RemoveDeadRegions(const std::map<size_t, size_t> &bbIdToDfsTimestamp)
{
    for (auto &block: graph_) {
        std::vector<BytecodeRegion *> newPreds;
        for (auto &bb : block.preds) {
            if (bbIdToDfsTimestamp.count(bb->id)) {
                newPreds.emplace_back(bb);
            }
        }
        block.preds = newPreds;
    }

    for (auto &block : graph_) {
        block.isDead = !bbIdToDfsTimestamp.count(block.id);
        if (block.isDead) {
            block.succs.clear();
        }
    }
}

BytecodeInfo BytecodeCircuitBuilder::GetBytecodeInfo(const uint8_t *pc)
{
    BytecodeInfo info;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    info.opcode = opcode;
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4: {
            uint16_t vdst = READ_INST_4_0();
            uint16_t vsrc = READ_INST_4_1();
            info.vregOut.emplace_back(vdst);
            info.offset = BytecodeOffset::TWO;
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::MOV_DYN_V8_V8: {
            uint16_t vdst = READ_INST_8_0();
            uint16_t vsrc = READ_INST_8_1();
            info.vregOut.emplace_back(vdst);
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::MOV_DYN_V16_V16: {
            uint16_t vdst = READ_INST_16_0();
            uint16_t vsrc = READ_INST_16_2();
            info.vregOut.emplace_back(vdst);
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::LDA_STR_ID32: {
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            uint64_t imm = READ_INST_32_0();
            info.inputs.emplace_back(StringId(imm));
            break;
        }
        case EcmaOpcode::JMP_IMM8: {
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::JMP_IMM16: {
            info.offset = BytecodeOffset::THREE;
            break;
        }
        case EcmaOpcode::JMP_IMM32: {
            info.offset = BytecodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::JEQZ_IMM8: {
            info.accIn = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::JEQZ_IMM16: {
            info.accIn = true;
            info.offset = BytecodeOffset::THREE;
            break;
        }
        case EcmaOpcode::JNEZ_IMM8: {
            info.accIn = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::JNEZ_IMM16: {
            info.accIn = true;
            info.offset = BytecodeOffset::THREE;
            break;
        }
        case EcmaOpcode::LDA_DYN_V8: {
            uint16_t vsrc = READ_INST_8_0();
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            info.inputs.emplace_back(VirtualRegister(vsrc));
            break;
        }
        case EcmaOpcode::STA_DYN_V8: {
            uint16_t vdst = READ_INST_8_0();
            info.vregOut.emplace_back(vdst);
            info.accIn = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDAI_DYN_IMM32: {
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(Immediate(READ_INST_32_0()));
            break;
        }
        case EcmaOpcode::FLDAI_DYN_IMM64: {
            info.accOut = true;
            info.offset = BytecodeOffset::NINE;
            info.inputs.emplace_back(Immediate(READ_INST_64_0()));
            break;
        }
        case EcmaOpcode::CALLARG0DYN_PREF_V8: {
            uint32_t funcReg = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(funcReg));
            break;
        }
        case EcmaOpcode::CALLARG1DYN_PREF_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(funcReg));
            info.inputs.emplace_back(VirtualRegister(reg));
            break;
        }
        case EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg0 = READ_INST_8_2();
            uint32_t reg1 = READ_INST_8_3();
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(VirtualRegister(funcReg));
            info.inputs.emplace_back(VirtualRegister(reg0));
            info.inputs.emplace_back(VirtualRegister(reg1));
            break;
        }
        case EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg0 = READ_INST_8_2();
            uint32_t reg1 = READ_INST_8_3();
            uint32_t reg2 = READ_INST_8_4();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(VirtualRegister(funcReg));
            info.inputs.emplace_back(VirtualRegister(reg0));
            info.inputs.emplace_back(VirtualRegister(reg1));
            info.inputs.emplace_back(VirtualRegister(reg2));
            break;
        }
        case EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8: {
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1();
            info.inputs.emplace_back(VirtualRegister(funcReg));
            for (size_t i = 1; i <= actualNumArgs; i++) {
                info.inputs.emplace_back(VirtualRegister(funcReg + i));
            }
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::CALLSPREADDYN_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            info.inputs.emplace_back(VirtualRegister(v2));
            break;
        }
        case EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8: {
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1();
            info.inputs.emplace_back(VirtualRegister(funcReg));
            for (size_t i = 1; i <= actualNumArgs; i++) {
                info.inputs.emplace_back(VirtualRegister(funcReg + i));
            }
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::RETURN_DYN: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::ONE;
            break;
        }
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDNAN_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDINFINITY_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDGLOBALTHIS_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDUNDEFINED_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDNULL_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDSYMBOL_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDGLOBAL_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDTRUE_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDFALSE_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDLEXENVDYN_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::GETUNMAPPEDARGS_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONENTER_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::TONUMBER_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NEGDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NOTDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::INCDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DECDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::THROWDYN_PREF: {
            info.accIn = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::TYPEOFDYN_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::GETPROPITERATOR_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::RESUMEGENERATOR_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(vs));
            break;
        }
        case EcmaOpcode::GETRESUMEMODE_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(vs));
            break;
        }
        case EcmaOpcode::GETITERATOR_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF: {
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF: {
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWIFNOTOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ITERNEXT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CLOSEITERATOR_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ADD2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::SUB2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::MUL2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DIV2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::MOD2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::EQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NOTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LESSDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LESSEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::GREATERDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::GREATEREQDYN_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(vs));
            break;
        }
        case EcmaOpcode::SHL2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::SHR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ASHR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::AND2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::OR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::XOR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DELOBJPROP_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::DEFINEFUNCDYN_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(MethodId(READ_INST_16_1()));
            info.inputs.emplace_back(Immediate(READ_INST_16_3()));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DEFINENCFUNCDYN_PREF_ID16_IMM16_V8: {
            uint16_t methodId = READ_INST_16_1();
            uint16_t length = READ_INST_16_3();
            uint16_t v0 = READ_INST_8_5();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(MethodId(methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DEFINEMETHOD_PREF_ID16_IMM16_V8: {
            uint16_t methodId = READ_INST_16_1();
            uint16_t length = READ_INST_16_3();
            uint16_t v0 = READ_INST_8_5();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(MethodId(methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NEWOBJDYNRANGE_PREF_IMM16_V8: {
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            uint16_t range = READ_INST_16_1();
            uint16_t firstArgRegIdx = READ_INST_8_3();
            for (uint16_t i = 0; i < range; ++i) {
                info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + i));
            }
            break;
        }
        case EcmaOpcode::EXPDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::ISINDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::INSTANCEOFDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STRICTNOTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STRICTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM16_IMM16: {
            uint16_t level = READ_INST_16_1();
            uint16_t slot = READ_INST_16_3();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM8_IMM8: {
            uint16_t level = READ_INST_8_1();
            uint16_t slot = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM4_IMM4: {
            uint16_t level = READ_INST_4_2();
            uint16_t slot = READ_INST_4_3();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM16_IMM16_V8: {
            uint16_t level = READ_INST_16_1();
            uint16_t slot = READ_INST_16_3();
            uint16_t v0 = READ_INST_8_5();
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM8_IMM8_V8: {
            uint16_t level = READ_INST_8_1();
            uint16_t slot = READ_INST_8_2();
            uint16_t v0 = READ_INST_8_3();
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM4_IMM4_V8: {
            uint16_t level = READ_INST_4_2();
            uint16_t slot = READ_INST_4_3();
            uint16_t v0 = READ_INST_8_2();
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(level));
            info.inputs.emplace_back(Immediate(slot));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::NEWLEXENVDYN_PREF_IMM16: {
            uint16_t numVars = READ_INST_16_1();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(numVars));
            break;
        }
        case EcmaOpcode::NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16: {
            uint16_t numVars = READ_INST_16_1();
            uint16_t scopeId = READ_INST_16_3();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(Immediate(numVars));
            info.inputs.emplace_back(Immediate(scopeId));
            break;
        }
        case EcmaOpcode::POPLEXENVDYN_PREF: {
            info.accOut = false;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::CREATEITERRESULTOBJ_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::SUSPENDGENERATOR_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v2));
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONREJECT_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v2));
            break;
        }
        case EcmaOpcode::NEWOBJSPREADDYN_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::THROWUNDEFINEDIFHOLE_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8: {
            uint32_t stringId = READ_INST_32_1();
            uint32_t v0 = READ_INST_8_5();
            info.accIn = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CREATEEMPTYARRAY_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::CREATEEMPTYOBJECT_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_PREF_IMM16: {
            uint16_t imm = READ_INST_16_1();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(imm));
            break;
        }
        case EcmaOpcode::SETOBJECTWITHPROTO_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::CREATEARRAYWITHBUFFER_PREF_IMM16: {
            uint16_t imm = READ_INST_16_1();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(imm));
            break;
        }
        case EcmaOpcode::GETMODULENAMESPACE_PREF_ID32: {
            uint32_t stringId = READ_INST_32_1();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(stringId));
            break;
        }
        case EcmaOpcode::STMODULEVAR_PREF_ID32: {
            uint32_t stringId = READ_INST_32_1();
            info.accIn = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(stringId));
            break;
        }
        case EcmaOpcode::COPYMODULE_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDMODULEVAR_PREF_ID32_IMM8: {
            uint32_t stringId = READ_INST_32_1();
            uint8_t innerFlag = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(Immediate(innerFlag));
            break;
        }
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8: {
            uint32_t stringId = READ_INST_32_1();
            uint8_t flags = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(Immediate(flags));
            break;
        }
        case EcmaOpcode::GETTEMPLATEOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::GETNEXTPROPNAME_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::COPYDATAPROPERTIES_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t index = READ_INST_32_2();
            info.accIn = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.accIn = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8: {
            uint16_t numKeys = READ_INST_16_1();
            uint16_t v0 = READ_INST_8_3();
            uint16_t firstArgRegIdx = READ_INST_8_4();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(Immediate(numKeys));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(firstArgRegIdx));
            break;
        }
        case EcmaOpcode::DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8: {
            uint16_t methodId = READ_INST_16_1();
            uint16_t length = READ_INST_16_3();
            uint16_t v0 = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(MethodId(methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::DEFINEASYNCFUNC_PREF_ID16_IMM16_V8: {
            uint16_t methodId = READ_INST_16_1();
            uint16_t length = READ_INST_16_3();
            uint16_t v0 = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(MethodId(methodId));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDHOLE_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::COPYRESTARGS_PREF_IMM16: {
            uint16_t restIdx = READ_INST_16_1();
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(restIdx));
            break;
        }
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            uint16_t v3 = READ_INST_8_4();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            info.inputs.emplace_back(VirtualRegister(v2));
            info.inputs.emplace_back(VirtualRegister(v3));
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32: {
            uint16_t v0 = READ_INST_8_1();
            uint32_t idx = READ_INST_32_2();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(idx));
            break;
        }
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32: {
            uint16_t v0 = READ_INST_8_1();
            uint32_t index = READ_INST_32_2();
            info.accIn = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(Immediate(index));
            break;
        }
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.accIn = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.accIn = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32: {
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(READ_INST_32_1()));
            break;
        }
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32: {
            info.accIn = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(READ_INST_32_1()));
            break;
        }
        case EcmaOpcode::STCONSTTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(READ_INST_32_1()));
            break;
        }
        case EcmaOpcode::STLETTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(READ_INST_32_1()));
            break;
        }
        case EcmaOpcode::STCLASSTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(READ_INST_32_1()));
            break;
        }
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.accIn = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_PREF_ID32_V8: {
            uint32_t stringId = READ_INST_32_1();
            uint32_t v0 = READ_INST_8_5();
            info.accIn = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32: {
            uint32_t stringId = READ_INST_32_1();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(stringId));
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8: {
            uint32_t stringId = READ_INST_32_1();
            uint32_t v0 = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8: {
            uint32_t stringId = READ_INST_32_1();
            uint32_t v0 = READ_INST_8_5();
            info.accIn = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8: {
            uint32_t stringId = READ_INST_32_1();
            uint32_t v0 = READ_INST_8_5();
            info.accOut = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8: {
            uint32_t stringId = READ_INST_32_1();
            uint32_t v0 = READ_INST_8_5();
            info.accIn = true;
            info.offset = BytecodeOffset::SEVEN;
            info.inputs.emplace_back(StringId(stringId));
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STGLOBALVAR_PREF_ID32: {
            uint32_t stringId = READ_INST_32_1();
            info.accIn = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(stringId));
            break;
        }
        case EcmaOpcode::CREATEGENERATOROBJ_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::STARRAYSPREAD_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::GETITERATORNEXT_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
            uint16_t methodId = READ_INST_16_1();
            uint16_t imm = READ_INST_16_3();
            uint16_t length = READ_INST_16_5();
            uint16_t v0 = READ_INST_8_7();
            uint16_t v1 = READ_INST_8_8();
            info.accOut = true;
            info.offset = BytecodeOffset::TEN;
            info.inputs.emplace_back(MethodId(methodId));
            info.inputs.emplace_back(Immediate(imm));
            info.inputs.emplace_back(Immediate(length));
            info.inputs.emplace_back(VirtualRegister(v0));
            info.inputs.emplace_back(VirtualRegister(v1));
            break;
        }
        case EcmaOpcode::LDFUNCTION_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDBIGINT_PREF_ID32: {
            uint32_t stringId = READ_INST_32_1();
            info.accOut = true;
            info.offset = BytecodeOffset::SIX;
            info.inputs.emplace_back(StringId(stringId));
            break;
        }
        case EcmaOpcode::SUPERCALL_PREF_IMM16_V8: {
            uint16_t range = READ_INST_16_1();
            uint16_t v0 = READ_INST_8_3();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(Immediate(range));
            info.inputs.emplace_back(Immediate(v0));
            break;
        }
        case EcmaOpcode::SUPERCALLSPREAD_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::THREE;
            info.inputs.emplace_back(VirtualRegister(v0));
            break;
        }
        case EcmaOpcode::CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
            uint16_t imm = READ_INST_16_1();
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(imm));
            break;
        }
        case EcmaOpcode::THROWIFSUPERNOTCORRECTCALL_PREF_IMM16: {
            uint16_t imm = READ_INST_16_1();
            info.accIn = true;
            info.offset = BytecodeOffset::FOUR;
            info.inputs.emplace_back(Immediate(imm));
            break;
        }
        case EcmaOpcode::LDHOMEOBJECT_PREF: {
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::DEBUGGER_PREF: {
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::ISTRUE_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        case EcmaOpcode::ISFALSE_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = BytecodeOffset::TWO;
            break;
        }
        default: {
            COMPILER_LOG(ERROR) << "Error bytecode: " << opcode << ", pls check bytecode offset.";
            UNREACHABLE();
            break;
        }
    }
    return info;
}

void BytecodeCircuitBuilder::InsertPhi()
{
    std::map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for (auto &bb : graph_) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetBytecodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            for (const auto &vreg: bytecodeInfo.vregOut) {
                defsitesInfo[vreg].insert(bb.id);
            }
        }
    }

    if (IsLogEnabled()) {
        for (const auto&[variable, defsites] : defsitesInfo) {
            std::string log("variable: " + std::to_string(variable) + " locate block have: ");
            for (auto id : defsites) {
                log += std::to_string(id) + " , ";
            }
            COMPILER_LOG(INFO) << log;
        }
    }

    for (const auto&[variable, defsites] : defsitesInfo) {
        std::queue<uint16_t> workList;
        for (auto blockId: defsites) {
            workList.push(blockId);
        }
        while (!workList.empty()) {
            auto currentId = workList.front();
            workList.pop();
            for (auto &block : graph_[currentId].domFrontiers) {
                if (!block->phi.count(variable)) {
                    block->phi.insert(variable);
                    if (!defsitesInfo[variable].count(block->id)) {
                        workList.push(block->id);
                    }
                }
            }
        }
    }

    if (IsLogEnabled()) {
        PrintGraph();
    }
}

// Update CFG's predecessor, successor and try catch associations
void BytecodeCircuitBuilder::UpdateCFG()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.preds.clear();
        bb.trys.clear();
        std::vector<BytecodeRegion *> newSuccs;
        for (const auto &succ: bb.succs) {
            if (std::count(bb.catchs.cbegin(), bb.catchs.cend(), succ)) {
                continue;
            }
            newSuccs.push_back(succ);
        }
        bb.succs = newSuccs;
    }
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        for (auto &succ: bb.succs) {
            succ->preds.push_back(&bb);
        }
        for (auto &catchBlock: bb.catchs) {
            catchBlock->trys.push_back(&bb);
        }
    }
}

// build circuit
void BytecodeCircuitBuilder::BuildCircuitArgs()
{
    const size_t numArgs = method_->GetNumArgs();
    const size_t actualNumArgs = GetActualNumArgs(numArgs);
    actualArgs_.resize(actualNumArgs);

    auto glueGate = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I64, 0,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                     GateType::NJSValue());
    actualArgs_.at(0) = glueGate;
    commonArgs_.at(0) = glueGate;
    auto argRoot = Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST));
    auto actualArgc = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I32, CommonArgIdx::ACTUAL_ARGC,
                                       {argRoot}, GateType::NJSValue());
    actualArgs_.at(CommonArgIdx::ACTUAL_ARGC) = actualArgc;
    commonArgs_.at(CommonArgIdx::ACTUAL_ARGC) = actualArgc;
    for (size_t argIdx = CommonArgIdx::FUNC; argIdx < CommonArgIdx::NUM_OF_ARGS; argIdx++) {
        auto argGate = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I64, argIdx, {argRoot},
                                        GateType::TaggedValue());
        actualArgs_.at(argIdx) = argGate;
        commonArgs_.at(argIdx) = argGate;
    }

    for (size_t argIdx = CommonArgIdx::NUM_OF_ARGS; argIdx < actualNumArgs; argIdx++) {
        actualArgs_.at(argIdx) = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I64, argIdx,
                                                  {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                                  GateType::TaggedValue());
    }
}

void BytecodeCircuitBuilder::CollectPredsInfo()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.numOfStatePreds = 0;
    }
    // get number of expanded state predicates of each block
    // one block-level try catch edge may correspond to multiple bytecode-level edges
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetBytecodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            if (bytecodeInfo.IsGeneral()) {
                if (!bb.catchs.empty()) {
                    bb.catchs.at(0)->numOfStatePreds++;
                }
            }
        }
        for (auto &succ: bb.succs) {
            succ->numOfStatePreds++;
        }
    }
    // collect loopback edges
    std::vector<VisitState> visitState(graph_.size(), VisitState::UNVISITED);
    std::function<void(size_t)> dfs = [&](size_t bbId) -> void {
        visitState[bbId] = VisitState::PENDING;
        auto it = this->graph_[bbId].succs.crbegin();
        while (it != this->graph_[bbId].succs.crend()) {
            auto succBlock = *it;
            it++;
            if (visitState[succBlock->id] == VisitState::UNVISITED) {
                dfs(succBlock->id);
            } else {
                if (visitState[succBlock->id] == VisitState::PENDING) {
                    this->graph_[succBlock->id].loopbackBlocks.insert(bbId);
                }
            }
        }
        visitState[bbId] = VisitState::VISITED;
    };
    dfs(graph_[0].id);
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.phiAcc = (bb.numOfStatePreds > 1) || (!bb.trys.empty());
        bb.numOfLoopBacks = bb.loopbackBlocks.size();
    }
}

void BytecodeCircuitBuilder::NewMerge(GateRef &state, GateRef &depend, size_t numOfIns)
{
    state = circuit_.NewGate(OpCode(OpCode::MERGE), numOfIns,
                             std::vector<GateRef>(numOfIns, Circuit::NullGate()),
                             GateType::Empty());
    depend = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR), numOfIns,
                              std::vector<GateRef>(numOfIns + 1, Circuit::NullGate()),
                              GateType::Empty());
    circuit_.NewIn(depend, 0, state);
}

void BytecodeCircuitBuilder::NewLoopBegin(BytecodeRegion &bb)
{
    NewMerge(bb.mergeForwardEdges, bb.depForward, bb.numOfStatePreds - bb.numOfLoopBacks);
    NewMerge(bb.mergeLoopBackEdges, bb.depLoopBack, bb.numOfLoopBacks);
    auto loopBack = circuit_.NewGate(OpCode(OpCode::LOOP_BACK), 0,
                                     {bb.mergeLoopBackEdges}, GateType::Empty());
    bb.stateStart = circuit_.NewGate(OpCode(OpCode::LOOP_BEGIN), 0,
                                     {bb.mergeForwardEdges, loopBack}, GateType::Empty());
    // 2: the number of depend inputs and it is in accord with LOOP_BEGIN
    bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR), 2,
                                      {bb.stateStart, bb.depForward, bb.depLoopBack},
                                      GateType::Empty());
}

void BytecodeCircuitBuilder::BuildBlockCircuitHead()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        if (bb.numOfStatePreds == 0) {
            bb.stateStart = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
            bb.dependStart = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        } else if (bb.numOfStatePreds == 1) {
            bb.stateStart = circuit_.NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0,
                                             {Circuit::NullGate()}, GateType::Empty());
            bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_RELAY), 0,
                                              {bb.stateStart, Circuit::NullGate()}, GateType::Empty());
        } else if (bb.numOfLoopBacks > 0) {
            NewLoopBegin(bb);
        } else {
            NewMerge(bb.stateStart, bb.dependStart, bb.numOfStatePreds);
        }
    }
}

std::vector<GateRef> BytecodeCircuitBuilder::CreateGateInList(const BytecodeInfo &info)
{
    size_t numValueInputs = (info.accIn ? 1 : 0) + info.inputs.size();
    const size_t length = 2; // 2: state and depend on input
    std::vector<GateRef> inList(length + numValueInputs, Circuit::NullGate());
    for (size_t i = 0; i < info.inputs.size(); i++) {
        const auto &input = info.inputs[i];
        if (std::holds_alternative<MethodId>(input)) {
            inList[i + length] = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I16,
                                                  std::get<MethodId>(input).GetId(),
                                                  {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                  GateType::NJSValue());
        } else if (std::holds_alternative<StringId>(input)) {
            size_t index = tsLoader_->GetStringIdx(constantPool_, std::get<StringId>(input).GetId());
            inList[i + length] = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I32, index,
                                                  {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                  GateType::NJSValue());
        } else if (std::holds_alternative<Immediate>(input)) {
            inList[i + length] = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                                  std::get<Immediate>(input).GetValue(),
                                                  {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                  GateType::NJSValue());
        } else {
            ASSERT(std::holds_alternative<VirtualRegister>(input));
            continue;
        }
    }
    return inList;
}

void BytecodeCircuitBuilder::SetBlockPred(BytecodeRegion &bbNext, const GateRef &state,
                                          const GateRef &depend, bool isLoopBack)
{
    if (bbNext.numOfLoopBacks == 0) {
        circuit_.NewIn(bbNext.stateStart, bbNext.statePredIndex, state);
        circuit_.NewIn(bbNext.dependStart, bbNext.statePredIndex + 1, depend);
    } else {
        if (isLoopBack) {
            circuit_.NewIn(bbNext.mergeLoopBackEdges, bbNext.loopBackIndex, state);
            circuit_.NewIn(bbNext.depLoopBack, bbNext.loopBackIndex + 1, depend);
            bbNext.loopBackIndex++;
            ASSERT(bbNext.loopBackIndex <= bbNext.numOfLoopBacks);
        } else {
            circuit_.NewIn(bbNext.mergeForwardEdges, bbNext.forwardIndex, state);
            circuit_.NewIn(bbNext.depForward, bbNext.forwardIndex + 1, depend);
            bbNext.forwardIndex++;
            ASSERT(bbNext.forwardIndex <= bbNext.numOfStatePreds - bbNext.numOfLoopBacks);
        }
    }
    bbNext.statePredIndex++;
    ASSERT(bbNext.statePredIndex <= bbNext.numOfStatePreds);
}

GateRef BytecodeCircuitBuilder::NewConst(const BytecodeInfo &info)
{
    auto opcode = static_cast<EcmaOpcode>(info.opcode);
    GateRef gate = 0;
    switch (opcode) {
        case EcmaOpcode::LDNAN_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::F64,
                                    base::NumberHelper::GetNaN(),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDINFINITY_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::F64,
                                    base::NumberHelper::GetPositiveInfinity(),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDUNDEFINED_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_UNDEFINED,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDNULL_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_NULL,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDTRUE_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_TRUE,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDFALSE_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_FALSE,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDHOLE_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_HOLE,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedNPointer());
            break;
        case EcmaOpcode::LDAI_DYN_IMM32:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                    std::get<Immediate>(info.inputs[0]).ToJSTaggedValueInt(),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::FLDAI_DYN_IMM64:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                    std::get<Immediate>(info.inputs.at(0)).ToJSTaggedValueDouble(),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TaggedValue());
            break;
        case EcmaOpcode::LDFUNCTION_PREF:
            gate = GetCommonArgByIndex(CommonArgIdx::FUNC);
            break;
        default:
            UNREACHABLE();
    }
    return gate;
}

void BytecodeCircuitBuilder::NewJSGate(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend)
{
    auto bytecodeInfo = GetBytecodeInfo(pc);
    size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.inputs.size();
    GateRef gate = 0;
    std::vector<GateRef> inList = CreateGateInList(bytecodeInfo);
    if (!bytecodeInfo.vregOut.empty() || bytecodeInfo.accOut) {
        gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::I64, numValueInputs,
                                inList, GateType::AnyType());
    } else {
        gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::NOVALUE, numValueInputs,
                                inList, GateType::Empty());
    }
    circuit_.NewIn(gate, 0, state);
    circuit_.NewIn(gate, 1, depend);
    auto ifSuccess = circuit_.NewGate(OpCode(OpCode::IF_SUCCESS), 0, {gate}, GateType::Empty());
    auto ifException = circuit_.NewGate(OpCode(OpCode::IF_EXCEPTION), 0, {gate}, GateType::Empty());
    if (!bb.catchs.empty()) {
        auto &bbNext = bb.catchs.at(0);
        auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
        SetBlockPred(*bbNext, ifException, gate, isLoopBack);
        bbNext->expandedPreds.push_back(
            {bb.id, pc, true}
        );
    } else {
        auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                         JSTaggedValue::VALUE_EXCEPTION,
                                         {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                         GateType::AnyType());
        circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                         {ifException, gate, constant,
                         Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                         GateType::AnyType());
    }
    jsgateToBytecode_[gate] = {bb.id, pc};
    if (bytecodeInfo.IsThrow()) {
        auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                         JSTaggedValue::VALUE_HOLE,
                                         {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                         GateType::AnyType());
        circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                         {ifSuccess, gate, constant,
                         Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                         GateType::AnyType());
        return;
    }
    state = ifSuccess;
    depend = gate;
    if (pc == bb.end) {
        auto &bbNext = graph_[bb.id + 1];
        auto isLoopBack = bbNext.loopbackBlocks.count(bb.id);
        SetBlockPred(bbNext, state, depend, isLoopBack);
        bbNext.expandedPreds.push_back(
            {bb.id, pc, false}
        );
    }
}

void BytecodeCircuitBuilder::NewJump(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend)
{
    auto bytecodeInfo = GetBytecodeInfo(pc);
    size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.inputs.size();
    if (bytecodeInfo.IsCondJump()) {
        GateRef gate = 0;
        gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::NOVALUE, numValueInputs,
                                std::vector<GateRef>(2 + numValueInputs, // 2: state and depend input
                                                     Circuit::NullGate()),
                                GateType::Empty());
        circuit_.NewIn(gate, 0, state);
        circuit_.NewIn(gate, 1, depend);
        auto ifTrue = circuit_.NewGate(OpCode(OpCode::IF_TRUE), 0, {gate}, GateType::Empty());
        auto ifFalse = circuit_.NewGate(OpCode(OpCode::IF_FALSE), 0, {gate}, GateType::Empty());
        ASSERT(bb.succs.size() == 2); // 2 : 2 num of successors
        uint32_t bitSet = 0;
        for (auto &bbNext: bb.succs) {
            if (bbNext->id == bb.id + 1) {
                auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
                SetBlockPred(*bbNext, ifFalse, gate, isLoopBack);
                bbNext->expandedPreds.push_back(
                    {bb.id, pc, false}
                );
                bitSet |= 1;
            } else {
                auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
                SetBlockPred(*bbNext, ifTrue, gate, isLoopBack);
                bbNext->expandedPreds.push_back(
                    {bb.id, pc, false}
                );
                bitSet |= 2; // 2:verify
            }
        }
        ASSERT(bitSet == 3); // 3:Verify the number of successor blocks
        jsgateToBytecode_[gate] = {bb.id, pc};
    } else {
        ASSERT(bb.succs.size() == 1);
        auto &bbNext = bb.succs.at(0);
        auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
        SetBlockPred(*bbNext, state, depend, isLoopBack);
        bbNext->expandedPreds.push_back(
            {bb.id, pc, false}
        );
    }
}

void BytecodeCircuitBuilder::NewReturn(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend)
{
    ASSERT(bb.succs.empty());
    auto bytecodeInfo = GetBytecodeInfo(pc);
    if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURN_DYN) {
        // handle return.dyn bytecode
        auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     { state, depend, Circuit::NullGate(),
                                     Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST)) },
                                     GateType::Empty());
        jsgateToBytecode_[gate] = {bb.id, pc};
    } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURNUNDEFINED_PREF) {
        // handle returnundefined bytecode
        auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                         JSTaggedValue::VALUE_UNDEFINED,
                                         { Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST)) },
                                         GateType::AnyType());
        auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     { state, depend, constant,
                                     Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST)) },
                                     GateType::Empty());
        jsgateToBytecode_[gate] = {bb.id, pc};
    }
}

void BytecodeCircuitBuilder::NewByteCode(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend)
{
    ASSERT(pc != nullptr);
    auto bytecodeInfo = GetBytecodeInfo(pc);
    if (bytecodeInfo.IsSetConstant()) {
        // handle bytecode command to get constants
        GateRef gate = NewConst(bytecodeInfo);
        jsgateToBytecode_[gate] = {bb.id, pc};
    } else if (bytecodeInfo.IsGeneral()) {
        // handle general ecma.* bytecodes
        NewJSGate(bb, pc, state, depend);
    } else if (bytecodeInfo.IsJump()) {
        // handle conditional jump and unconditional jump bytecodes
        NewJump(bb, pc, state, depend);
    } else if (bytecodeInfo.IsReturn()) {
        // handle return.dyn and returnundefined bytecodes
        NewReturn(bb, pc, state, depend);
    } else if (bytecodeInfo.IsMov()) {
        // handle mov.dyn lda.dyn sta.dyn bytecodes
        if (pc == bb.end) {
            auto &bbNext = graph_[bb.id + 1];
            auto isLoopBack = bbNext.loopbackBlocks.count(bb.id);
            SetBlockPred(bbNext, state, depend, isLoopBack);
            bbNext.expandedPreds.push_back(
                {bb.id, pc, false}
            );
        }
    } else if (bytecodeInfo.IsDiscarded()) {
        return;
    } else {
        UNREACHABLE();
    }
}

void BytecodeCircuitBuilder::BuildSubCircuit()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        auto stateCur = bb.stateStart;
        auto dependCur = bb.dependStart;
        ASSERT(stateCur != Circuit::NullGate());
        ASSERT(dependCur != Circuit::NullGate());
        if (!bb.trys.empty()) {
            dependCur = circuit_.NewGate(OpCode(OpCode::GET_EXCEPTION), 0, {dependCur}, GateType::Empty());
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto pcPrev = pc;
            auto bytecodeInfo = GetBytecodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            NewByteCode(bb, pcPrev, stateCur, dependCur);
            if (bytecodeInfo.IsJump() || bytecodeInfo.IsThrow()) {
                break;
            }
        }
    }
}

GateRef BytecodeCircuitBuilder::NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc)
{
    GateRef gate = 0;
    if (bb.numOfLoopBacks == 0) {
        gate = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64,
                                bb.numOfStatePreds,
                                std::vector<GateRef>(
                                    1 + bb.numOfStatePreds, Circuit::NullGate()),
                                GateType::AnyType());
        circuit_.NewIn(gate, 0, bb.stateStart);
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
            circuit_.NewIn(gate, i + 1, RenameVariable(predId, predPc, reg, acc));
        }
    } else {
        auto loopBackValue = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64,
                                              bb.numOfLoopBacks,
                                              std::vector<GateRef>(
                                                  1 + bb.numOfLoopBacks, Circuit::NullGate()),
                                              GateType::AnyType());
        circuit_.NewIn(loopBackValue, 0, bb.mergeLoopBackEdges);
        bb.loopBackIndex = 1; // 1: start index of value inputs
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
            if (bb.loopbackBlocks.count(predId)) {
                circuit_.NewIn(loopBackValue, bb.loopBackIndex++,
                               RenameVariable(predId, predPc, reg, acc));
            }
        }
        auto forwardValue = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64,
                                             bb.numOfStatePreds - bb.numOfLoopBacks,
                                             std::vector<GateRef>(
                                                 1 + bb.numOfStatePreds - bb.numOfLoopBacks,
                                                 Circuit::NullGate()),
                                             GateType::AnyType());
        circuit_.NewIn(forwardValue, 0, bb.mergeForwardEdges);
        bb.forwardIndex = 1; // 1: start index of value inputs
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
            if (!bb.loopbackBlocks.count(predId)) {
                circuit_.NewIn(forwardValue, bb.forwardIndex++,
                               RenameVariable(predId, predPc, reg, acc));
            }
        }
        // 2: the number of value inputs and it is in accord with LOOP_BEGIN
        gate = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64, 2,
                                {bb.stateStart, forwardValue, loopBackValue},
                                GateType::AnyType());
    }
    return gate;
}

GateType BytecodeCircuitBuilder::GetRealGateType(const uint16_t reg, const GateType gateType)
{
    if (file_->HasTSTypes()) {
        auto curType = GateType(tsLoader_->GetGTFromPandaFile(*pf_, reg, method_));
        auto type = curType.IsAnyType() ? gateType : curType;
        return type;
    }

    return GateType::AnyType();
}

// recursive variables renaming algorithm
GateRef BytecodeCircuitBuilder::RenameVariable(const size_t bbId,
    const uint8_t *end, const uint16_t reg, const bool acc, const GateType gateType)
{
    ASSERT(end != nullptr);
    const size_t offsetArgs = method_->GetNumVregs();
    auto tmpReg = reg;
    auto tsType = GetRealGateType(tmpReg, gateType);
    // find def-site in bytecodes of basic block
    auto ans = Circuit::NullGate();
    auto &bb = graph_.at(bbId);
    std::vector<uint8_t *> instList;
    {
        auto pcIter = bb.start;
        while (pcIter <= end) {
            instList.push_back(pcIter);
            auto curInfo = GetBytecodeInfo(pcIter);
            pcIter += curInfo.offset;
        }
    }
    std::reverse(instList.begin(), instList.end());
    auto tmpAcc = acc;
    for (auto pcIter: instList) { // upper bound
        auto curInfo = GetBytecodeInfo(pcIter);
        // original bc use acc as input && current bc use acc as output
        bool isTransByAcc = tmpAcc && curInfo.accOut;
        // 0 : the index in vreg-out list
        bool isTransByVreg = (!tmpAcc && curInfo.IsOut(tmpReg, 0));
        if (isTransByAcc || isTransByVreg) {
            if (curInfo.IsMov()) {
                tmpAcc = curInfo.accIn;
                if (!curInfo.inputs.empty()) {
                    ASSERT(!tmpAcc);
                    ASSERT(curInfo.inputs.size() == 1);
                    tmpReg = std::get<VirtualRegister>(curInfo.inputs.at(0)).GetId();
                    tsType = GetRealGateType(tmpReg, tsType);
                }
            } else {
                ans = byteCodeToJSGate_.at(pcIter);
                break;
            }
        }
    }
    // find GET_EXCEPTION gate if this is a catch block
    if (ans == Circuit::NullGate() && tmpAcc) {
        if (!bb.trys.empty()) {
            const auto &outList = circuit_.GetOutVector(bb.dependStart);
            ASSERT(outList.size() == 1);
            const auto &getExceptionGate = outList.at(0);
            ASSERT(circuit_.GetOpCode(getExceptionGate) == OpCode::GET_EXCEPTION);
            ans = getExceptionGate;
        }
    }
    // find def-site in value selectors of vregs
    if (ans == Circuit::NullGate() && !tmpAcc && bb.phi.count(tmpReg)) {
        if (!bb.vregToValSelectorGate.count(tmpReg)) {
            bb.vregToValSelectorGate[tmpReg] = NewPhi(bb, tmpReg, tmpAcc);
        }
        ans = bb.vregToValSelectorGate.at(tmpReg);
    }
    // find def-site in value selectors of acc
    if (ans == Circuit::NullGate() && tmpAcc && bb.phiAcc) {
        if (bb.valueSelectorAccGate == Circuit::NullGate()) {
            bb.valueSelectorAccGate = NewPhi(bb, tmpReg, tmpAcc);
        }
        ans = bb.valueSelectorAccGate;
    }
    if (ans == Circuit::NullGate() && bbId == 0) { // entry block
        // find def-site in function args
        ASSERT(!tmpAcc && tmpReg >= offsetArgs && tmpReg < offsetArgs + actualArgs_.size());
        auto index = GetFunctionArgIndex(tmpReg, offsetArgs);
        ans = actualArgs_.at(index);
        circuit_.LoadGatePtr(ans)->SetGateType(static_cast<GateType>(tsType));
        return ans;
    }
    if (ans == Circuit::NullGate()) {
        // recursively find def-site in dominator block
        return RenameVariable(bb.iDominator->id, bb.iDominator->end, tmpReg, tmpAcc, tsType);
    } else {
        // def-site already found
        circuit_.LoadGatePtr(ans)->SetGateType(static_cast<GateType>(tsType));
        return ans;
    }
}

void BytecodeCircuitBuilder::BuildCircuit()
{
    if (IsLogEnabled()) {
        PrintBBInfo();
    }

    // create arg gates array
    BuildCircuitArgs();
    CollectPredsInfo();
    BuildBlockCircuitHead();
    // build states sub-circuit of each block
    BuildSubCircuit();
    // verification of soundness of CFG
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        ASSERT(bb.statePredIndex == bb.numOfStatePreds);
        ASSERT(bb.loopBackIndex == bb.numOfLoopBacks);
        if (bb.numOfLoopBacks) {
            ASSERT(bb.forwardIndex == bb.numOfStatePreds - bb.numOfLoopBacks);
        }
    }

    if (IsLogEnabled()) {
        PrintBytecodeInfo();
    }

    for (const auto &[key, value]: jsgateToBytecode_) {
        byteCodeToJSGate_[value.second] = key;
    }
    // resolve def-site of virtual regs and set all value inputs
    for (auto gate: circuit_.GetAllGates()) {
        auto valueCount = circuit_.GetOpCode(gate).GetInValueCount(circuit_.GetBitField(gate));
        auto it = jsgateToBytecode_.find(gate);
        if (it == jsgateToBytecode_.cend()) {
            continue;
        }
        if (circuit_.LoadGatePtrConst(gate)->GetOpCode() == OpCode::CONSTANT) {
            continue;
        }
        const auto &[id, pc] = it->second;
        auto bytecodeInfo = GetBytecodeInfo(pc);
        [[maybe_unused]] size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.inputs.size();
        [[maybe_unused]] size_t numValueOutputs = (bytecodeInfo.accOut ? 1 : 0) + bytecodeInfo.vregOut.size();
        ASSERT(numValueInputs == valueCount);
        ASSERT(numValueOutputs <= 1);
        auto stateCount = circuit_.GetOpCode(gate).GetStateCount(circuit_.GetBitField(gate));
        auto dependCount = circuit_.GetOpCode(gate).GetDependCount(circuit_.GetBitField(gate));
        for (size_t valueIdx = 0; valueIdx < valueCount; valueIdx++) {
            auto inIdx = valueIdx + stateCount + dependCount;
            if (!circuit_.IsInGateNull(gate, inIdx)) {
                continue;
            }
            if (valueIdx < bytecodeInfo.inputs.size()) {
                circuit_.NewIn(gate, inIdx,
                               RenameVariable(id, pc - 1,
                                              std::get<VirtualRegister>(bytecodeInfo.inputs.at(valueIdx)).GetId(),
                                              false));
            } else {
                circuit_.NewIn(gate, inIdx, RenameVariable(id, pc - 1, 0, true));
            }
        }
    }

    if (IsLogEnabled()) {
        circuit_.PrintAllGates(*this);
    }
}

size_t BytecodeCircuitBuilder::GetFunctionArgIndex(size_t currentVreg, size_t numVregs) const
{
    return (currentVreg - numVregs + CommonArgIdx::NUM_OF_ARGS);
}

void BytecodeCircuitBuilder::PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos)
{
    for (auto iter = bytecodeBlockInfos.cbegin(); iter != bytecodeBlockInfos.cend(); iter++) {
        std::string log("offset: " + std::to_string(reinterpret_cast<uintptr_t>(iter->pc)) + " splitKind: " +
            std::to_string(static_cast<int32_t>(iter->splitKind)) + " successor are: ");
        auto &vec = iter->succs;
        for (size_t i = 0; i < vec.size(); i++) {
            log += std::to_string(reinterpret_cast<uintptr_t>(vec[i])) + " , ";
        }
        COMPILER_LOG(INFO) << log;
    }
    COMPILER_LOG(INFO) << "-----------------------------------------------------------------------";
}

void BytecodeCircuitBuilder::PrintGraph()
{
    for (size_t i = 0; i < graph_.size(); i++) {
        if (graph_[i].isDead) {
            COMPILER_LOG(INFO) << "BB_" << graph_[i].id << ":                               ;predsId= invalid BB";
            COMPILER_LOG(INFO) << "curStartPc: " << reinterpret_cast<uintptr_t>(graph_[i].start) <<
                      " curEndPc: " << reinterpret_cast<uintptr_t>(graph_[i].end);
            continue;
        }
        std::string log("BB_" + std::to_string(graph_[i].id) + ":                               ;predsId= ");
        for (size_t k = 0; k < graph_[i].preds.size(); ++k) {
            log += std::to_string(graph_[i].preds[k]->id) + ", ";
        }
        COMPILER_LOG(INFO) << log;
        COMPILER_LOG(INFO) << "curStartPc: " << reinterpret_cast<uintptr_t>(graph_[i].start) <<
                  " curEndPc: " << reinterpret_cast<uintptr_t>(graph_[i].end);

        for (size_t j = 0; j < graph_[i].preds.size(); j++) {
            COMPILER_LOG(INFO) << "predsStartPc: " << reinterpret_cast<uintptr_t>(graph_[i].preds[j]->start) <<
                      " predsEndPc: " << reinterpret_cast<uintptr_t>(graph_[i].preds[j]->end);
        }

        for (size_t j = 0; j < graph_[i].succs.size(); j++) {
            COMPILER_LOG(INFO) << "succesStartPc: " << reinterpret_cast<uintptr_t>(graph_[i].succs[j]->start) <<
                      " succesEndPc: " << reinterpret_cast<uintptr_t>(graph_[i].succs[j]->end);
        }

        std::string log1("succesId: ");
        for (size_t j = 0; j < graph_[i].succs.size(); j++) {
            log1 += std::to_string(graph_[i].succs[j]->id) + ", ";
        }
        COMPILER_LOG(INFO) << log1;

        for (size_t j = 0; j < graph_[i].catchs.size(); j++) {
            COMPILER_LOG(INFO) << "catchStartPc: " << reinterpret_cast<uintptr_t>(graph_[i].catchs[j]->start) <<
                      " catchEndPc: " << reinterpret_cast<uintptr_t>(graph_[i].catchs[j]->end);
        }

        for (size_t j = 0; j < graph_[i].immDomBlocks.size(); j++) {
            COMPILER_LOG(INFO) << "dominate block id: " << graph_[i].immDomBlocks[j]->id << " startPc: " <<
                      reinterpret_cast<uintptr_t>(graph_[i].immDomBlocks[j]->start) << " endPc: " <<
                      reinterpret_cast<uintptr_t>(graph_[i].immDomBlocks[j]->end);
        }

        if (graph_[i].iDominator) {
            COMPILER_LOG(INFO) << "current block " << graph_[i].id <<
                      " immediate dominator is " << graph_[i].iDominator->id;
        }

        std::string log2("current block " + std::to_string(graph_[i].id) + " dominance Frontiers: ");
        for (const auto &frontier: graph_[i].domFrontiers) {
            log2 += std::to_string(frontier->id) + " , ";
        }
        COMPILER_LOG(INFO) << log2;

        std::string log3("current block " + std::to_string(graph_[i].id) + " phi variable: ");
        for (auto variable: graph_[i].phi) {
            log3 += std::to_string(variable) + " , ";
        }
        COMPILER_LOG(INFO) << log3;
        COMPILER_LOG(INFO) << "-------------------------------------------------------";
    }
}

void BytecodeCircuitBuilder::PrintBytecodeInfo()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        COMPILER_LOG(INFO) << "BB_" << bb.id << ": ";
        while (pc <= bb.end) {
            std::string log;
            auto curInfo = GetBytecodeInfo(pc);
            log += "Inst_" + GetEcmaOpcodeStr(static_cast<EcmaOpcode>(*pc)) + ": " + "In=[";
            if (curInfo.accIn) {
                log += "acc,";
            }
            for (const auto &in: curInfo.inputs) {
                if (std::holds_alternative<VirtualRegister>(in)) {
                    log += std::to_string(std::get<VirtualRegister>(in).GetId()) + ",";
                }
            }
            log += "] Out=[";
            if (curInfo.accOut) {
                log += "acc,";
            }
            for (const auto &out: curInfo.vregOut) {
                log +=  std::to_string(out) + ",";
            }
            log += "]";
            COMPILER_LOG(INFO) << log;
            pc += curInfo.offset;
        }
    }
}

void BytecodeCircuitBuilder::PrintBBInfo()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        COMPILER_LOG(INFO) << "------------------------";
        COMPILER_LOG(INFO) << "block: " << bb.id;
        std::string log("preds: ");
        for (auto pred: bb.preds) {
            log += std::to_string(pred->id) + " , ";
        }
        COMPILER_LOG(INFO) << log;
        std::string log1("succs: ");
        for (auto succ: bb.succs) {
            log1 += std::to_string(succ->id) + " , ";
        }
        COMPILER_LOG(INFO) << log1;
        std::string log2("catchs: ");
        for (auto catchBlock: bb.catchs) {
            log2 += std::to_string(catchBlock->id) + " , ";
        }
        COMPILER_LOG(INFO) << log2;
        std::string log3("trys: ");
        for (auto tryBlock: bb.trys) {
            log3 += std::to_string(tryBlock->id) + " , ";
        }
        COMPILER_LOG(INFO) << log3;
    }
}
}  // namespace panda::ecmascript::kungfu
