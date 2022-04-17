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
    const panda_file::File *file = file_->GetPandaFile();
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> byteCodeException;
    panda_file::MethodDataAccessor mda(*file, method_->GetFileId());
    panda_file::CodeDataAccessor cda(*file, mda.GetCodeId().value());
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
                auto iter = std::find(succs.begin(), succs.end(), bytecodeBlockInfos[i].pc);
                if (iter == succs.end()) {
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
            auto findItem = std::find_if(byteCodeCurPrePc.begin(), byteCodeCurPrePc.end(),
                                         [tempPc](const std::map<uint8_t *, uint8_t *>::value_type item) {
                                             return item.second == tempPc;
                                         });
            if (findItem != byteCodeCurPrePc.end()) {
                startBlockPc.emplace_back((*findItem).first);
            }
        }
    }

    // Supplementary end block info
    for (auto iter = endBlockPc.begin(); iter != endBlockPc.end(); iter += 2) { // 2: index
        bytecodeBlockInfos.emplace_back(*iter, SplitKind::END,
                                                          std::vector<uint8_t *>(1, *(iter + 1)));
    }
    // Supplementary start block info
    for (auto iter = startBlockPc.begin(); iter != startBlockPc.end(); iter++) {
        bytecodeBlockInfos.emplace_back(*iter, SplitKind::START, std::vector<uint8_t *>(1, *iter));
    }

    // Deduplicate successor
    for (size_t i = 0; i < bytecodeBlockInfos.size(); i++) {
        if (bytecodeBlockInfos[i].splitKind == SplitKind::END) {
            std::set<uint8_t *> tempSet(bytecodeBlockInfos[i].succs.begin(),
                                        bytecodeBlockInfos[i].succs.end());
            bytecodeBlockInfos[i].succs.assign(tempSet.begin(), tempSet.end());
        }
    }

    std::sort(bytecodeBlockInfos.begin(), bytecodeBlockInfos.end());

    // handling jumps to an empty block
    auto endPc = bytecodeBlockInfos[bytecodeBlockInfos.size() - 1].pc;
    auto iter = --byteCodeCurPrePc.end();
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
    BytecodeGraph byteCodeGraph;
    auto &blocks = byteCodeGraph.graph;
    byteCodeGraph.method = method_;
    blocks.resize(bytecodeBlockInfo.size() / 2); // 2 : half size
    // build basic block
    int blockId = 0;
    int index = 0;
    for (size_t i = 0; i < bytecodeBlockInfo.size() - 1; i += 2) { // 2:index
        auto startPc = bytecodeBlockInfo[i].pc;
        auto endPc = bytecodeBlockInfo[i + 1].pc;
        auto block = &blocks[index++];
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
    for (size_t i = 0; i < blocks.size(); i++) {
        auto pc = blocks[i].start;
        auto it = exception.begin();
        for (; it != exception.end(); it++) {
            if (pc < it->first.first || pc >= it->first.second) { // try block interval
                continue;
            }
            auto catchs = exception[it->first]; // catchs start pc
            for (size_t j = i + 1; j < blocks.size(); j++) {
                if (std::find(catchs.begin(), catchs.end(), blocks[j].start) != catchs.end()) {
                    blocks[i].catchs.insert(blocks[i].catchs.begin(), &blocks[j]);
                    blocks[i].succs.emplace_back(&blocks[j]);
                    blocks[j].preds.emplace_back(&blocks[i]);
                }
            }
        }
    }

    for (size_t i = 0; i < blocks.size(); i++) {
        bbIdToBasicBlock_[blocks[i].id] = &blocks[i];
    }

    if (IsLogEnabled()) {
        PrintGraph(byteCodeGraph.graph);
    }
    ComputeDominatorTree(byteCodeGraph);
}

void BytecodeCircuitBuilder::ComputeDominatorTree(BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    // Construct graph backward order
    std::map<size_t, size_t> bbIdToDfsTimestamp; // (basicblock id, dfs order)
    size_t timestamp = 0;
    std::deque<size_t> pendingList;
    std::vector<size_t> visited(graph.size(), 0);
    auto basicBlockId = graph[0].id;
    pendingList.push_back(basicBlockId);
    while (!pendingList.empty()) {
        auto &curBlockId = pendingList.back();
        pendingList.pop_back();
        bbIdToDfsTimestamp[curBlockId] = timestamp++;
        for (auto &succBlock: graph[curBlockId].succs) {
            if (visited[succBlock->id] == 0) {
                visited[succBlock->id] = 1;
                pendingList.push_back(succBlock->id);
            }
        }
    }

    RemoveDeadRegions(bbIdToDfsTimestamp, byteCodeGraph);

    if (IsLogEnabled()) {
        // print cfg order
        for (auto iter : bbIdToDfsTimestamp) {
            COMPILER_LOG(INFO) << "BB_" << iter.first << " depth is : " << iter.second;
        }
    }

    std::vector<int32_t> immDom(graph.size()); // immediate dominator
    std::vector<std::vector<size_t>> doms(graph.size()); // dominators set
    doms[0] = {0};
    for (size_t i = 1; i < doms.size(); i++) {
        doms[i].resize(doms.size());
        std::iota(doms[i].begin(), doms[i].end(), 0);
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < doms.size(); i++) {
            if (graph[i].isDead) {
                continue;
            }
            auto &curDom = doms[i];
            size_t curDomSize = curDom.size();
            curDom.resize(doms.size());
            std::iota(curDom.begin(), curDom.end(), 0);
            // traverse the predecessor nodes of the current node, Computing Dominators
            for (auto &preBlock : graph[i].preds) {
                std::vector<size_t> tmp(curDom.size());
                auto preDom = doms[preBlock->id];
                auto it = std::set_intersection(curDom.begin(), curDom.end(), preDom.begin(), preDom.end(),
                                                tmp.begin());
                tmp.resize(it - tmp.begin());
                curDom = tmp;
            }
            auto it = std::find(curDom.begin(), curDom.end(), i);
            if (it == curDom.end()) {
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
    immDom[0] = static_cast<int32_t>(doms[0].front());
    for (size_t i = 1; i < doms.size(); i++) {
        if (graph[i].isDead) {
            continue;
        }
        auto it = std::remove(doms[i].begin(), doms[i].end(), i);
        doms[i].resize(it - doms[i].begin());
        immDom[i] = static_cast<int32_t>(*std::max_element(
            doms[i].begin(), doms[i].end(), [graph, bbIdToDfsTimestamp](size_t lhs, size_t rhs) -> bool {
                return bbIdToDfsTimestamp.at(graph[lhs].id) < bbIdToDfsTimestamp.at(graph[rhs].id);
            }));
    }

    if (IsLogEnabled()) {
        // print immediate dominator
        for (size_t i = 0; i < immDom.size(); i++) {
            COMPILER_LOG(INFO) << i << " immediate dominator: " << immDom[i];
        }
        PrintGraph(graph);
    }

    BuildImmediateDominator(immDom, byteCodeGraph);
}

void BytecodeCircuitBuilder::BuildImmediateDominator(std::vector<int32_t> &immDom, BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;

    graph[0].iDominator = &graph[0];
    for (size_t i = 1; i < immDom.size(); i++) {
        auto dominatedBlock = bbIdToBasicBlock_.at(i);
        if (dominatedBlock->isDead) {
            continue;
        }
        auto immDomBlock = bbIdToBasicBlock_.at(immDom[i]);
        dominatedBlock->iDominator = immDomBlock;
    }

    if (IsLogEnabled()) {
        for (auto block : graph) {
            if (block.isDead) {
                continue;
            }
            COMPILER_LOG(INFO) << "current block " << block.id
                               << " immediate dominator block id: " << block.iDominator->id;
        }
    }

    for (auto &block : graph) {
        if (block.isDead) {
            continue;
        }
        if (block.iDominator->id != block.id) {
            block.iDominator->immDomBlocks.emplace_back(&block);
        }
    }

    if (IsLogEnabled()) {
        for (auto &block : graph) {
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

    ComputeDomFrontiers(immDom, byteCodeGraph);
    InsertPhi(byteCodeGraph);
    UpdateCFG(byteCodeGraph);
    BuildCircuit(byteCodeGraph);
}

void BytecodeCircuitBuilder::ComputeDomFrontiers(std::vector<int32_t> &immDom, BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::vector<std::set<BytecodeRegion *>> domFrontiers(immDom.size());
    for (auto &bb : graph) {
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
                runner = bbIdToBasicBlock_.at(immDom[runner->id]);
            }
        }
    }

    for (size_t i = 0; i < domFrontiers.size(); i++) {
        for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
            graph[i].domFrontiers.emplace_back(*iter);
        }
    }

    if (IsLogEnabled()) {
        for (size_t i = 0; i < domFrontiers.size(); i++) {
            std::string log("basic block " + std::to_string(i) + " dominate Frontiers is: ");
            for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
                log += std::to_string((*iter)->id) + ", ";
            }
            COMPILER_LOG(INFO) << log;
        }
    }
}

void BytecodeCircuitBuilder::RemoveDeadRegions(const std::map<size_t, size_t> &bbIdToDfsTimestamp,
                                               BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    for (auto &block: graph) {
        std::vector<BytecodeRegion *> newPreds;
        for (auto &bb : block.preds) {
            if (bbIdToDfsTimestamp.count(bb->id)) {
                newPreds.emplace_back(bb);
            }
        }
        block.preds = newPreds;
    }

    for (auto &block : graph) {
        block.isDead = !bbIdToDfsTimestamp.count(block.id);
        if (block.isDead) {
            block.succs.clear();
        }
    }
}

BytecodeInfo BytecodeCircuitBuilder::GetBytecodeInfo(uint8_t *pc)
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

            info.inputs.emplace_back(Immediate(actualNumArgs));
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

            info.inputs.emplace_back(Immediate(actualNumArgs));
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
            uint16_t firstArgRegIdx = READ_INST_8_3();
            info.accOut = true;
            info.offset = BytecodeOffset::FIVE;
            info.inputs.emplace_back(Immediate(READ_INST_16_1()));
            info.inputs.emplace_back(Immediate(firstArgRegIdx));
            info.inputs.emplace_back(VirtualRegister(firstArgRegIdx));
            info.inputs.emplace_back(VirtualRegister(firstArgRegIdx + 1));
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

void BytecodeCircuitBuilder::InsertPhi(BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for (auto &bb : graph) {
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
            for (auto &block : graph[currentId].domFrontiers) {
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
        PrintGraph(graph);
    }
}

// Update CFG's predecessor, successor and try catch associations
void BytecodeCircuitBuilder::UpdateCFG(BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.preds.clear();
        bb.trys.clear();
        std::vector<BytecodeRegion *> newSuccs;
        for (const auto &succ: bb.succs) {
            if (std::count(bb.catchs.begin(), bb.catchs.end(), succ)) {
                continue;
            }
            newSuccs.push_back(succ);
        }
        bb.succs = newSuccs;
    }
    for (auto &bb: graph) {
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

bool BytecodeCircuitBuilder::IsJump(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
            return true;
        default:
            return false;
    }
}

bool BytecodeCircuitBuilder::IsCondJump(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
            return true;
        default:
            return false;
    }
}

bool BytecodeCircuitBuilder::IsMov(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_DYN_V8_V8:
        case EcmaOpcode::MOV_DYN_V16_V16:
        case EcmaOpcode::LDA_DYN_V8:
        case EcmaOpcode::STA_DYN_V8:
            return true;
        default:
            return false;
    }
}

bool BytecodeCircuitBuilder::IsReturn(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF:
            return true;
        default:
            return false;
    }
}

bool BytecodeCircuitBuilder::IsThrow(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::THROWDYN_PREF:
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8:
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF:
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF:
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF:
            return true;
        default:
            return false;
    }
}

bool BytecodeCircuitBuilder::IsDiscarded(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::COPYMODULE_PREF_V8:
        case EcmaOpcode::DEBUGGER_PREF:
            return true;
        default:
            return false;
    }
}

bool BytecodeCircuitBuilder::IsGeneral(EcmaOpcode opcode)
{
    return !IsMov(opcode) && !IsJump(opcode) && !IsReturn(opcode) && !IsSetConstant(opcode) && !IsDiscarded(opcode);
}

bool BytecodeCircuitBuilder::IsSetConstant(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::LDNAN_PREF:
        case EcmaOpcode::LDINFINITY_PREF:
        case EcmaOpcode::LDUNDEFINED_PREF:
        case EcmaOpcode::LDNULL_PREF:
        case EcmaOpcode::LDTRUE_PREF:
        case EcmaOpcode::LDFALSE_PREF:
        case EcmaOpcode::LDHOLE_PREF:
        case EcmaOpcode::LDAI_DYN_IMM32:
        case EcmaOpcode::FLDAI_DYN_IMM64:
        case EcmaOpcode::LDFUNCTION_PREF:
            return true;
        default:
            return false;
    }
}

GateRef BytecodeCircuitBuilder::SetGateConstant(const BytecodeInfo &info)
{
    auto opcode = static_cast<EcmaOpcode>(info.opcode);
    GateRef gate = 0;
    // ts loader
    panda::ecmascript::TSLoader* tsLoader = vm_->GetTSLoader();
    uint64_t tsType = 0;
    switch (opcode) {
        case EcmaOpcode::LDNAN_PREF:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_NUMBER).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::F64,
                                    bit_cast<int64_t>(panda::ecmascript::base::NAN_VALUE),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDINFINITY_PREF:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_NUMBER).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::F64,
                                    bit_cast<int64_t>(panda::ecmascript::base::POSITIVE_INFINITY),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDUNDEFINED_PREF:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_UNDEFINED).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_UNDEFINED,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDNULL_PREF:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_NULL).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_NULL,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDTRUE_PREF:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_BOOLEAN).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_TRUE,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDFALSE_PREF:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_BOOLEAN).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_FALSE,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDHOLE_PREF:
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64, JSTaggedValue::VALUE_HOLE,
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    GateType::TAGGED_NPOINTER);
            break;
        case EcmaOpcode::LDAI_DYN_IMM32:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_NUMBER).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                    std::get<Immediate>(info.inputs[0]).GetValue(),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::FLDAI_DYN_IMM64:
            tsType = tsLoader->GetPrimitiveGT(TSTypeKind::TS_NUMBER).GetGlobalTSTypeRef();
            gate = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::F64,
                                    std::get<Immediate>(info.inputs.at(0)).GetValue(),
                                    {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                    static_cast<GateType>(tsType));
            break;
        case EcmaOpcode::LDFUNCTION_PREF:
            gate = GetCommonArgByIndex(CommonArgIdx::FUNC);
            break;
        default:
            UNREACHABLE();
    }
    return gate;
}

void BytecodeCircuitBuilder::BuildCircuit(BytecodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;

    if (IsLogEnabled()) {
        PrintBBInfo(graph);
    }

    // create arg gates array
    const size_t numArgs = byteCodeGraph.method->GetNumArgs();
    const size_t offsetArgs = byteCodeGraph.method->GetNumVregs();
    const size_t actualNumArgs = GetActualNumArgs(numArgs);
    std::vector<GateRef> argGates(actualNumArgs);

    auto glueGate = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I64, 0,
                                     {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                     GateType::NJS_VALUE);
    argGates.at(0) = glueGate;
    commonArgs_.at(0) = glueGate;
    auto argRoot = Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST));
    auto actualArgc = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I32, CommonArgIdx::ACTUAL_ARGC,
                                       {argRoot}, GateType::NJS_VALUE);
    argGates.at(CommonArgIdx::ACTUAL_ARGC) = actualArgc;
    commonArgs_.at(CommonArgIdx::ACTUAL_ARGC) = actualArgc;
    for (size_t argIdx = CommonArgIdx::FUNC; argIdx < CommonArgIdx::NUM_OF_ARGS; argIdx++) {
        auto argGate = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I64, argIdx, {argRoot},
                                        GateType::TAGGED_VALUE);
        argGates.at(argIdx) = argGate;
        commonArgs_.at(argIdx) = argGate;
    }

    for (size_t argIdx = CommonArgIdx::NUM_OF_ARGS; argIdx < actualNumArgs; argIdx++) {
        argGates.at(argIdx) = circuit_.NewGate(OpCode(OpCode::ARG), MachineType::I64, argIdx,
                                               {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                               GateType::TAGGED_VALUE);
    }
    // get number of expanded state predicates of each block
    // one block-level try catch edge may correspond to multiple bytecode-level edges
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.numOfStatePreds = 0;
    }
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetBytecodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                if (!bb.catchs.empty()) {
                    bb.catchs.at(0)->numOfStatePreds++;
                }
            }
        }
        for (auto &succ: bb.succs) {
            succ->numOfStatePreds++;
        }
    }

    // build head of each block
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        if (bb.numOfStatePreds == 0) {
            bb.stateStart = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
            bb.dependStart = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        } else if (bb.numOfStatePreds == 1) {
            bb.stateStart = circuit_.NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0,
                                             {Circuit::NullGate()}, GateType::EMPTY);
            bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_RELAY), 0,
                                              {bb.stateStart, Circuit::NullGate()}, GateType::EMPTY);
        } else {
            bb.stateStart = circuit_.NewGate(OpCode(OpCode::MERGE), bb.numOfStatePreds,
                                             std::vector<GateRef>(bb.numOfStatePreds, Circuit::NullGate()),
                                             GateType::EMPTY);
            bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR), bb.numOfStatePreds,
                                              std::vector<GateRef>(bb.numOfStatePreds + 1, Circuit::NullGate()),
                                              GateType::EMPTY);
            circuit_.NewIn(bb.dependStart, 0, bb.stateStart);
        }
    }
    // build states sub-circuit of each block
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto stateCur = bb.stateStart;
        auto dependCur = bb.dependStart;
        ASSERT(stateCur != Circuit::NullGate());
        ASSERT(dependCur != Circuit::NullGate());
        if (!bb.trys.empty()) {
            dependCur = circuit_.NewGate(OpCode(OpCode::GET_EXCEPTION), 0, {dependCur}, GateType::EMPTY);
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto pcPrev = pc;
            auto bytecodeInfo = GetBytecodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.inputs.size();
            if (IsSetConstant(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle bytecode command to get constants
                GateRef gate = SetGateConstant(bytecodeInfo);
                jsgateToBytecode_[gate] = {bb.id, pcPrev};
            } else if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle general ecma.* bytecodes
                GateRef gate = 0;
                const size_t length = 2; // 2: state and depend on input
                std::vector<GateRef> inList(length + numValueInputs, Circuit::NullGate());
                for (size_t i = 0; i < bytecodeInfo.inputs.size(); i++) {
                    const auto &input = bytecodeInfo.inputs[i];
                    if (std::holds_alternative<MethodId>(input)) {
                        inList[i + length] = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I16,
                                                              std::get<MethodId>(input).GetId(),
                                                              {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                              GateType::NJS_VALUE);
                    } else if (std::holds_alternative<StringId>(input)) {
                        auto tsLoader = vm_->GetTSLoader();
                        JSHandle<ConstantPool> newConstPool(vm_->GetJSThread(), constantPool_.GetTaggedValue());
                        auto string = newConstPool->GetObjectFromCache(std::get<StringId>(input).GetId());
                        size_t index = tsLoader->AddConstString(string);
                        inList[i + length] = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I32, index,
                                                              {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                              GateType::NJS_VALUE);
                    } else if (std::holds_alternative<Immediate>(input)) {
                        inList[i + length] = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                                              std::get<Immediate>(input).GetValue(),
                                                              {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                              GateType::NJS_VALUE);
                    } else {
                        ASSERT(std::holds_alternative<VirtualRegister>(input));
                        continue;
                    }
                }
                if (!bytecodeInfo.vregOut.empty() || bytecodeInfo.accOut) {
                    gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::I64, numValueInputs,
                                            inList, GateType::JS_ANY);
                } else {
                    gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::NOVALUE, numValueInputs,
                                            inList, GateType::EMPTY);
                }
                circuit_.NewIn(gate, 0, stateCur);
                circuit_.NewIn(gate, 1, dependCur);
                auto ifSuccess = circuit_.NewGate(OpCode(OpCode::IF_SUCCESS), 0, {gate}, GateType::EMPTY);
                auto ifException = circuit_.NewGate(OpCode(OpCode::IF_EXCEPTION), 0, {gate}, GateType::EMPTY);
                if (!bb.catchs.empty()) {
                    auto bbNext = bb.catchs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, ifException);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, gate);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back(
                        {bb.id, pcPrev, true}
                    );
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                } else {
                    auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                                     JSTaggedValue::VALUE_EXCEPTION,
                                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                     GateType::JS_ANY);
                    circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     {ifException, gate, constant,
                                      Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                     GateType::JS_ANY);
                }
                jsgateToBytecode_[gate] = {bb.id, pcPrev};
                if (IsThrow(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                    auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                                     JSTaggedValue::VALUE_HOLE,
                                                     {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                     GateType::JS_ANY);
                    circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     {ifSuccess, gate, constant,
                                      Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                     GateType::JS_ANY);
                    break;
                }
                stateCur = ifSuccess;
                dependCur = gate;
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, dependCur);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back(
                        {bb.id, pcPrev, false}
                    );
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                }
            } else if (IsJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle conditional jump and unconditional jump bytecodes
                if (IsCondJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                    GateRef gate = 0;
                    gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::NOVALUE, numValueInputs,
                                            std::vector<GateRef>(2 + numValueInputs, // 2: state and depend input
                                                                 Circuit::NullGate()),
                                            GateType::EMPTY);
                    circuit_.NewIn(gate, 0, stateCur);
                    circuit_.NewIn(gate, 1, dependCur);
                    auto ifTrue = circuit_.NewGate(OpCode(OpCode::IF_TRUE), 0, {gate}, GateType::EMPTY);
                    auto ifFalse = circuit_.NewGate(OpCode(OpCode::IF_FALSE), 0, {gate}, GateType::EMPTY);
                    ASSERT(bb.succs.size() == 2); // 2 : 2 num of successors
                    uint32_t bitSet = 0;
                    for (auto &bbNext: bb.succs) {
                        if (bbNext->id == bb.id + 1) {
                            circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, ifFalse);
                            circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, gate);
                            bbNext->statePredIndex++;
                            bbNext->expandedPreds.push_back(
                                {bb.id, pcPrev, false}
                            );
                            ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                            bitSet |= 1;
                        } else {
                            circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, ifTrue);
                            circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, gate);
                            bbNext->statePredIndex++;
                            bbNext->expandedPreds.push_back(
                                {bb.id, pcPrev, false}
                            );
                            ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                            bitSet |= 2; // 2:verify
                        }
                    }
                    ASSERT(bitSet == 3); // 3:Verify the number of successor blocks
                    jsgateToBytecode_[gate] = {bb.id, pcPrev};
                    break;
                } else {
                    ASSERT(bb.succs.size() == 1);
                    auto bbNext = bb.succs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, dependCur);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back(
                        {bb.id, pcPrev, false}
                    );
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                    break;
                }
            } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURN_DYN) {
                // handle return.dyn bytecode
                ASSERT(bb.succs.empty());
                auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                             { stateCur, dependCur, Circuit::NullGate(),
                                              Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST)) },
                                             GateType::EMPTY);
                jsgateToBytecode_[gate] = {bb.id, pcPrev};
                break;
            } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURNUNDEFINED_PREF) {
                // handle returnundefined bytecode
                ASSERT(bb.succs.empty());
                auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), MachineType::I64,
                                                 TaggedValue::VALUE_UNDEFINED,
                                                 { Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST)) },
                                                 GateType::JS_ANY);
                auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                             { stateCur, dependCur, constant,
                                              Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST)) },
                                             GateType::EMPTY);
                jsgateToBytecode_[gate] = {bb.id, pcPrev};
                break;
            } else if (IsMov(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle mov.dyn lda.dyn sta.dyn bytecodes
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, dependCur);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back(
                        {bb.id, pcPrev, false}
                    );
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                }
            } else if (IsDiscarded(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                continue;
            } else {
                UNREACHABLE();
            }
        }
    }
    // verification of soundness of CFG
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        ASSERT(bb.statePredIndex == bb.numOfStatePreds);
    }
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.phiAcc = (bb.numOfStatePreds > 1) || (!bb.trys.empty());
    }

    if (IsLogEnabled()) {
        PrintBytecodeInfo(graph);
    }

    for (const auto &[key, value]: jsgateToBytecode_) {
        byteCodeToJSGate_[value.second] = key;
    }

    // ts loader
    panda::ecmascript::TSLoader* tsLoader = vm_->GetTSLoader();

    // resolve def-site of virtual regs and set all value inputs
    for (auto gate: circuit_.GetAllGates()) {
        auto valueCount = circuit_.GetOpCode(gate).GetInValueCount(circuit_.GetBitField(gate));
        auto it = jsgateToBytecode_.find(gate);
        if (it == jsgateToBytecode_.end()) {
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
        // recursive variables renaming algorithm
        std::function<GateRef(size_t, const uint8_t *, uint16_t, bool)> defSiteOfReg =
                [&](size_t bbId, const uint8_t *end, uint16_t reg, bool acc) -> GateRef {
                    // find def-site in bytecodes of basic block
                    auto ans = Circuit::NullGate();
                    auto &bb = graph.at(bbId);
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
                    for (auto pcIter: instList) { // upper bound
                        auto curInfo = GetBytecodeInfo(pcIter);
                        if (acc) {
                            if (curInfo.accOut) {
                                if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                                    acc = curInfo.accIn;
                                    if (!curInfo.inputs.empty()) {
                                        ASSERT(!acc);
                                        ASSERT(curInfo.inputs.size() == 1);
                                        reg = std::get<VirtualRegister>(curInfo.inputs.at(0)).GetId();
                                    }
                                } else {
                                    ans = byteCodeToJSGate_.at(pcIter);
                                    break;
                                }
                            }
                        } else {
                            if (!curInfo.vregOut.empty() && curInfo.vregOut.at(0) == reg) {
                                if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                                    acc = curInfo.accIn;
                                    if (!curInfo.inputs.empty()) {
                                        ASSERT(!acc);
                                        ASSERT(curInfo.inputs.size() == 1);
                                        reg = std::get<VirtualRegister>(curInfo.inputs.at(0)).GetId();
                                    }
                                } else {
                                    ans = byteCodeToJSGate_.at(pcIter);
                                    break;
                                }
                            }
                        }
                    }
                    // find GET_EXCEPTION gate if this is a catch block
                    if (ans == Circuit::NullGate() && acc) {
                        if (!bb.trys.empty()) {
                            const auto &outList = circuit_.GetOutVector(bb.dependStart);
                            ASSERT(outList.size() == 1);
                            const auto &getExceptionGate = outList.at(0);
                            ASSERT(circuit_.GetOpCode(getExceptionGate) == OpCode::GET_EXCEPTION);
                            ans = getExceptionGate;
                        }
                    }
                    // find def-site in value selectors of vregs
                    if (ans == Circuit::NullGate() && !acc && bb.phi.count(reg)) {
                        if (!bb.vregToValSelectorGate.count(reg)) {
                            auto gate = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64,
                                                         bb.numOfStatePreds,
                                                         std::vector<GateRef>(
                                                                 1 + bb.numOfStatePreds, Circuit::NullGate()),
                                                         GateType::JS_ANY);
                            bb.vregToValSelectorGate[reg] = gate;
                            circuit_.NewIn(gate, 0, bb.stateStart);
                            for (int32_t i = 0; i < bb.numOfStatePreds; ++i) {
                                auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
                                circuit_.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                            }
                        }
                        ans = bb.vregToValSelectorGate.at(reg);
                    }
                    // find def-site in value selectors of acc
                    if (ans == Circuit::NullGate() && acc && bb.phiAcc) {
                        if (bb.valueSelectorAccGate == Circuit::NullGate()) {
                            auto gate = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64,
                                                         bb.numOfStatePreds,
                                                         std::vector<GateRef>(
                                                             1 + bb.numOfStatePreds, Circuit::NullGate()),
                                                         GateType::JS_ANY);
                            bb.valueSelectorAccGate = gate;
                            circuit_.NewIn(gate, 0, bb.stateStart);
                            for (int32_t i = 0; i < bb.numOfStatePreds; ++i) {
                                auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
                                circuit_.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                            }
                        }
                        ans = bb.valueSelectorAccGate;
                    }
                    if (ans == Circuit::NullGate() && bbId == 0) { // entry block
                        // find def-site in function args
                        ASSERT(!acc && reg >= offsetArgs && reg < offsetArgs + argGates.size());
                        const panda_file::File *pf = file_->GetPandaFile();
                        auto argVreg = reg - offsetArgs;
                        auto tsType = tsLoader->GetGTFromPandaFile(*pf, argVreg, method_).GetGlobalTSTypeRef();
                        auto index = GetFunctionArgIndex(reg, offsetArgs);
                        circuit_.LoadGatePtr(ans)->SetGateType(static_cast<GateType>(tsType));
                        return argGates.at(index);
                    }
                    if (ans == Circuit::NullGate()) {
                        // recursively find def-site in dominator block
                        return defSiteOfReg(bb.iDominator->id, bb.iDominator->end, reg, acc);
                    } else {
                        // def-site already found
                        const panda_file::File *pf = file_->GetPandaFile();
                        auto tsType = tsLoader->GetGTFromPandaFile(*pf, reg, method_).GetGlobalTSTypeRef();
                        circuit_.LoadGatePtr(ans)->SetGateType(static_cast<GateType>(tsType));
                        return ans;
                    }
                };
        auto stateCount = circuit_.GetOpCode(gate).GetStateCount(circuit_.GetBitField(gate));
        auto dependCount = circuit_.GetOpCode(gate).GetDependCount(circuit_.GetBitField(gate));
        for (size_t valueIdx = 0; valueIdx < valueCount; valueIdx++) {
            auto inIdx = valueIdx + stateCount + dependCount;
            if (!circuit_.IsInGateNull(gate, inIdx)) {
                continue;
            }
            if (valueIdx < bytecodeInfo.inputs.size()) {
                circuit_.NewIn(gate, inIdx,
                               defSiteOfReg(id, pc - 1,
                                            std::get<VirtualRegister>(bytecodeInfo.inputs.at(valueIdx)).GetId(),
                                            false));
            } else {
                circuit_.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, 0, true));
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
    for (auto iter = bytecodeBlockInfos.begin(); iter != bytecodeBlockInfos.end(); iter++) {
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

void BytecodeCircuitBuilder::PrintGraph(std::vector<BytecodeRegion> &graph)
{
    for (size_t i = 0; i < graph.size(); i++) {
        if (graph[i].isDead) {
            COMPILER_LOG(INFO) << "BB_" << graph[i].id << ":                               ;predsId= invalid BB";
            COMPILER_LOG(INFO) << "curStartPc: " << reinterpret_cast<uintptr_t>(graph[i].start) <<
                      " curEndPc: " << reinterpret_cast<uintptr_t>(graph[i].end);
            continue;
        }
        std::string log("BB_" + std::to_string(graph[i].id) + ":                               ;predsId= ");
        for (size_t k = 0; k < graph[i].preds.size(); ++k) {
            log += std::to_string(graph[i].preds[k]->id) + ", ";
        }
        COMPILER_LOG(INFO) << log;
        COMPILER_LOG(INFO) << "curStartPc: " << reinterpret_cast<uintptr_t>(graph[i].start) <<
                  " curEndPc: " << reinterpret_cast<uintptr_t>(graph[i].end);

        for (size_t j = 0; j < graph[i].preds.size(); j++) {
            COMPILER_LOG(INFO) << "predsStartPc: " << reinterpret_cast<uintptr_t>(graph[i].preds[j]->start) <<
                      " predsEndPc: " << reinterpret_cast<uintptr_t>(graph[i].preds[j]->end);
        }

        for (size_t j = 0; j < graph[i].succs.size(); j++) {
            COMPILER_LOG(INFO) << "succesStartPc: " << reinterpret_cast<uintptr_t>(graph[i].succs[j]->start) <<
                      " succesEndPc: " << reinterpret_cast<uintptr_t>(graph[i].succs[j]->end);
        }

        std::string log1("succesId: ");
        for (size_t j = 0; j < graph[i].succs.size(); j++) {
            log1 += std::to_string(graph[i].succs[j]->id) + ", ";
        }
        COMPILER_LOG(INFO) << log1;

        for (size_t j = 0; j < graph[i].catchs.size(); j++) {
            COMPILER_LOG(INFO) << "catchStartPc: " << reinterpret_cast<uintptr_t>(graph[i].catchs[j]->start) <<
                      " catchEndPc: " << reinterpret_cast<uintptr_t>(graph[i].catchs[j]->end);
        }

        for (size_t j = 0; j < graph[i].immDomBlocks.size(); j++) {
            COMPILER_LOG(INFO) << "dominate block id: " << graph[i].immDomBlocks[j]->id << " startPc: " <<
                      reinterpret_cast<uintptr_t>(graph[i].immDomBlocks[j]->start) << " endPc: " <<
                      reinterpret_cast<uintptr_t>(graph[i].immDomBlocks[j]->end);
        }

        if (graph[i].iDominator) {
            COMPILER_LOG(INFO) << "current block " << graph[i].id <<
                      " immediate dominator is " << graph[i].iDominator->id;
        }

        std::string log2("current block " + std::to_string(graph[i].id) + " dominance Frontiers: ");
        for (const auto &frontier: graph[i].domFrontiers) {
            log2 += std::to_string(frontier->id) + " , ";
        }
        COMPILER_LOG(INFO) << log2;

        std::string log3("current block " + std::to_string(graph[i].id) + " phi variable: ");
        for (auto variable: graph[i].phi) {
            log3 += std::to_string(variable) + " , ";
        }
        COMPILER_LOG(INFO) << log3;
        COMPILER_LOG(INFO) << "-------------------------------------------------------";
    }
}

void BytecodeCircuitBuilder::PrintBytecodeInfo(std::vector<BytecodeRegion> &graph)
{
    for (auto &bb: graph) {
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

void BytecodeCircuitBuilder::PrintBBInfo(std::vector<BytecodeRegion> &graph)
{
    for (auto &bb: graph) {
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