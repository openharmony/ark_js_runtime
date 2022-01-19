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

namespace panda::ecmascript::kungfu {
void ByteCodeCircuitBuilder::BytecodeToCircuit(const std::vector<uint8_t *> &pcArray, const panda_file::File &pf,
                                               const JSMethod *method)
{
    auto curPc = pcArray.front();
    auto prePc = curPc;
    std::map<uint8_t *, uint8_t *> byteCodeCurPrePc;
    std::vector<CfgInfo> bytecodeBlockInfos;
    auto startPc = curPc;
    bytecodeBlockInfos.emplace_back(startPc, SplitKind::START, std::vector<uint8_t *>(1, startPc));
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(curPc, prePc));
    for (size_t i = 1; i < pcArray.size() - 1; i++) {
        curPc = pcArray[i];
        byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(curPc, prePc));
        prePc = curPc;
        CollectBytecodeBlockInfo(curPc, bytecodeBlockInfos);
    }
    // handle empty
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(pcArray[pcArray.size() - 1], prePc));

    // collect try catch block info
    auto exceptionInfo = CollectTryCatchBlockInfo(pf, method, byteCodeCurPrePc, bytecodeBlockInfos);

    // Complete bytecode blcok Infomation
    CompleteBytecodeBlockInfo(byteCodeCurPrePc, bytecodeBlockInfos);

    // Building the basic block diagram of bytecode
    BuildBasicBlocks(method, exceptionInfo, bytecodeBlockInfos, byteCodeCurPrePc);
}

void ByteCodeCircuitBuilder::CollectBytecodeBlockInfo(uint8_t *pc, std::vector<CfgInfo> &bytecodeBlockInfos)
{
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8: {
            int8_t offset = READ_INST_8_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            // current basic block end
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::TWO, SplitKind::START,
                                                              std::vector<uint8_t *>(1, pc + ByteCodeOffset::TWO));
            // jump basic block start
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START,
                                                              std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JMP_IMM16: {
            int16_t offset = READ_INST_16_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::THREE, SplitKind::START,
                                                              std::vector<uint8_t *>(1, pc + ByteCodeOffset::THREE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START,
                                                              std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JMP_IMM32: {
            int32_t offset = READ_INST_32_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::FIVE, SplitKind::START,
                                                              std::vector<uint8_t *>(1, pc + ByteCodeOffset::FIVE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START,
                                                              std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + ByteCodeOffset::TWO);   // first successor
            int8_t offset = READ_INST_8_0();
            temp.emplace_back(pc + offset);  // second successor
            // condition branch current basic block end
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            // first branch basic block start
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::TWO, SplitKind::START,
                                std::vector<uint8_t *>(1, pc + ByteCodeOffset::TWO));
            // second branch basic block start
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + ByteCodeOffset::THREE);   // first successor
            int16_t offset = READ_INST_16_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp); // end
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::THREE, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + ByteCodeOffset::THREE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + ByteCodeOffset::TWO); // first successor
            int8_t offset = READ_INST_8_0();
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::TWO, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + ByteCodeOffset::TWO));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + ByteCodeOffset::THREE); // first successor
            int8_t offset = READ_INST_16_0();
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + ByteCodeOffset::THREE, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + ByteCodeOffset::THREE));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, std::vector<uint8_t *>(1, pc));
            break;
        }
        case EcmaOpcode::THROWDYN_PREF:
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8:
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF:
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF:
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, std::vector<uint8_t *>(1, pc));
        }
            break;
        default:
            break;
    }
}

std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> ByteCodeCircuitBuilder::CollectTryCatchBlockInfo(
    const panda_file::File &file, const JSMethod *method, std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
    std::vector<CfgInfo> &bytecodeBlockInfos)
{
    // try contains many catch
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> byteCodeException;
    panda_file::MethodDataAccessor mda(file, method->GetFileId());
    panda_file::CodeDataAccessor cda(file, mda.GetCodeId().value());
    cda.EnumerateTryBlocks([method, &byteCodeCurPrePc, &bytecodeBlockInfos, &byteCodeException](
            panda_file::CodeDataAccessor::TryBlock &try_block) {
        auto tryStartOffset = try_block.GetStartPc();
        auto tryEndOffset = try_block.GetStartPc() + try_block.GetLength();
        auto tryStartPc = const_cast<uint8_t *>(method->GetBytecodeArray() + tryStartOffset);
        auto tryEndPc = const_cast<uint8_t *>(method->GetBytecodeArray() + tryEndOffset);
        byteCodeException[std::make_pair(tryStartPc, tryEndPc)] = {};
        uint32_t pcOffset = panda_file::INVALID_OFFSET;
        try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
            pcOffset = catch_block.GetHandlerPc();
            auto catchBlockPc = const_cast<uint8_t *>(method->GetBytecodeArray() + pcOffset);
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
                auto &succs = bytecodeBlockInfos[i].succs; // 2 : get successors
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
        bytecodeBlockInfos.emplace_back(tryEndPc, SplitKind::START,std::vector<uint8_t *>(1, tryEndPc)); // next block
        return true;
    });
    return byteCodeException;
}

void ByteCodeCircuitBuilder::CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc,
                                                       std::vector<CfgInfo> &bytecodeBlockInfos)
{
    std::sort(bytecodeBlockInfos.begin(), bytecodeBlockInfos.end());

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintCollectBlockInfo(bytecodeBlockInfos);
#endif

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
    for (auto iter = endBlockPc.begin(); iter != endBlockPc.end(); iter += 2) {
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

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintCollectBlockInfo(bytecodeBlockInfos);
#endif
}

void ByteCodeCircuitBuilder::BuildBasicBlocks(const JSMethod *method,
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
    std::vector<CfgInfo> &bytecodeBlockInfo, std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc)
{

    std::map<uint8_t *, ByteCodeRegion *> startPcToBB; // [start, bb]
    std::map<uint8_t *, ByteCodeRegion *> endPcToBB; // [end, bb]
    ByteCodeGraph byteCodeGraph;
    auto &blocks = byteCodeGraph.graph;
    byteCodeGraph.method = method;
    blocks.resize(bytecodeBlockInfo.size() / 2); // 2 : half size
    // build basic block
    int blockId = 0;
    int index = 0;
    for (size_t i = 0; i < bytecodeBlockInfo.size() - 1; i += 2) {
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

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintGraph(byteCodeGraph.graph);
#endif
    ComputeDominatorTree(byteCodeGraph);
}

void ByteCodeCircuitBuilder::ComputeDominatorTree(ByteCodeGraph &byteCodeGraph)
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

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    // print cfg order
    for (auto iter : bbIdToDfsTimestamp) {
        std::cout << "BB_" << iter.first << " depth is : " << iter.second << std::endl;
    }
#endif
    std::vector<size_t> immDom(graph.size()); // immediate dominator
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
                auto it = std::set_intersection(
                        curDom.begin(), curDom.end(), preDom.begin(), preDom.end(), tmp.begin());
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

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    // print dominators set
    for (size_t i = 0; i < doms.size(); i++) {
        std::cout << "block " << i << " dominator blocks has: ";
        for (auto j: doms[i]) {
            std::cout << j << " , ";
        }
        std::cout << std::endl;
    }
#endif

    // compute immediate dominator
    immDom[0] = doms[0].front();
    for (size_t i = 1; i < doms.size(); i++) {
        if (graph[i].isDead) {
            continue;
        }
        auto it = std::remove(doms[i].begin(), doms[i].end(), i);
        doms[i].resize(it - doms[i].begin());
        immDom[i] = *std::max_element(
            doms[i].begin(), doms[i].end(), [graph, bbIdToDfsTimestamp](size_t lhs, size_t rhs) -> bool {
                return bbIdToDfsTimestamp.at(graph[lhs].id) < bbIdToDfsTimestamp.at(graph[rhs].id);
            });
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    // print immediate dominator
    for (size_t i = 0; i < immDom.size(); i++) {
        std::cout << i << " immediate dominator: " << immDom[i] << std::endl;
    }
    PrintGraph(graph);
#endif
    BuildImmediateDominator(immDom, byteCodeGraph);
}

void ByteCodeCircuitBuilder::BuildImmediateDominator(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph)
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

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (auto block : graph) {
        if (block.isDead) {
            continue;
        }
        std::cout << "current block " << block.id
                  << " immediate dominator block id: " << block.iDominator->id << std::endl;
    }
#endif

    for (auto &block : graph) {
        if (block.isDead) {
            continue;
        }
        if (block.iDominator->id != block.id) {
            block.iDominator->immDomBlocks.emplace_back(&block);
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (auto &block : graph) {
        if (block.isDead) {
            continue;
        }
        std::cout << "block " << block.id << " dominate block has: ";
        for (size_t i = 0; i < block.immDomBlocks.size(); i++) {
            std::cout << block.immDomBlocks[i]->id << ",";
        }
        std::cout << std::endl;
    }
#endif
    ComputeDomFrontiers(immDom, byteCodeGraph);
    InsertPhi(byteCodeGraph);
    UpdateCFG(byteCodeGraph);
    BuildCircuit(byteCodeGraph);
}

void ByteCodeCircuitBuilder::ComputeDomFrontiers(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::vector<std::set<ByteCodeRegion *>> domFrontiers(immDom.size());
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        if (bb.preds.size() < 2) {
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

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (size_t i = 0; i < domFrontiers.size(); i++) {
        std::cout << "basic block " << i << " dominate Frontiers is: ";
        for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
            std::cout << (*iter)->id << " , ";
        }
        std::cout << std::endl;
    }
#endif
}

void ByteCodeCircuitBuilder::RemoveDeadRegions(const std::map<size_t, size_t> &bbIdToDfsTimestamp,
                                            ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    for (auto &block: graph) {
        std::vector<ByteCodeRegion *> newPreds;
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

ByteCodeInfo ByteCodeCircuitBuilder::GetByteCodeInfo(uint8_t *pc)
{
    ByteCodeInfo info;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    info.opcode = opcode;
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4: {
            uint16_t vdst = READ_INST_4_0();
            uint16_t vsrc = READ_INST_4_1();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::MOV_DYN_V8_V8: {
            uint16_t vdst = READ_INST_8_0();
            uint16_t vsrc = READ_INST_8_1();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::MOV_DYN_V16_V16: {
            uint16_t vdst = READ_INST_16_0();
            uint16_t vsrc = READ_INST_16_2();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::LDA_STR_ID32: {
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::JMP_IMM8: {
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::JMP_IMM16: {
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::JMP_IMM32: {
            info.offset = 5;
            break;
        }
        case EcmaOpcode::JEQZ_IMM8: {
            info.accIn = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::JEQZ_IMM16: {
            info.accIn = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::JNEZ_IMM8: {
            info.accIn = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::JNEZ_IMM16: {
            info.accIn = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::LDA_DYN_V8: {
            uint16_t vsrc = READ_INST_8_0();
            info.vregIn.emplace_back(vsrc);
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::STA_DYN_V8: {
            uint16_t vdst = READ_INST_8_0();
            info.vregOut.emplace_back(vdst);
            info.accIn = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDAI_DYN_IMM32: {
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::FLDAI_DYN_IMM64: {
            info.accOut = true;
            info.offset = ByteCodeOffset::NINE;
            break;
        }
        case EcmaOpcode::CALLARG0DYN_PREF_V8: {
            uint32_t funcReg = READ_INST_8_1();
            info.vregIn.emplace_back(funcReg);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::CALLARG1DYN_PREF_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_2();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_3();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_4();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8: {
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1() - 1;
            size_t copyArgs = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS - 2;
            info.vregIn.emplace_back(funcReg);
            for (size_t i = 1; i <= copyArgs; i++) {
                info.vregIn.emplace_back(funcReg + i);
            }
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::CALLSPREADDYN_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.vregIn.emplace_back(v2);
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8: {
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1();
            size_t copyArgs = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS - 2;
            info.vregIn.emplace_back(funcReg);
            for (size_t i = 1; i <= copyArgs; i++) {
                info.vregIn.emplace_back(funcReg + i);
            }
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::RETURN_DYN: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::ONE;
            break;
        }
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDNAN_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDINFINITY_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDGLOBALTHIS_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDUNDEFINED_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDNULL_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDSYMBOL_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDGLOBAL_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDTRUE_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDFALSE_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::LDLEXENVDYN_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::GETUNMAPPEDARGS_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONENTER_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::TONUMBER_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::NEGDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::NOTDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::INCDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::DECDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::THROWDYN_PREF: {
            info.accIn = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::TYPEOFDYN_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::GETPROPITERATOR_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::RESUMEGENERATOR_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::GETRESUMEMODE_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::GETITERATOR_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF: {
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF: {
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWIFNOTOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::ITERNEXT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::CLOSEITERATOR_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::ADD2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::SUB2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::MUL2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::DIV2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::MOD2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::EQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::NOTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::LESSDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::LESSEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::GREATERDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::GREATEREQDYN_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::SHL2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::SHR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::ASHR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::AND2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::OR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::XOR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::DELOBJPROP_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::DEFINEFUNCDYN_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::DEFINENCFUNCDYN_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::DEFINEMETHOD_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::NEWOBJDYNRANGE_PREF_IMM16_V8: {
            uint16_t firstArgRegIdx = READ_INST_8_3();
            info.vregIn.emplace_back(firstArgRegIdx);
            info.vregIn.emplace_back(firstArgRegIdx + 1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::EXPDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::ISINDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::INSTANCEOFDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::STRICTNOTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::STRICTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM16_IMM16: {
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM8_IMM8: {
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM4_IMM4: {
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM8_IMM8_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM4_IMM4_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::NEWLEXENVDYN_PREF_IMM16: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::POPLEXENVDYN_PREF: {
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::CREATEITERRESULTOBJ_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::SUSPENDGENERATOR_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v2);
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONREJECT_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v2);
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::NEWOBJSPREADDYN_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::THROWUNDEFINEDIFHOLE_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::CREATEEMPTYARRAY_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::CREATEEMPTYOBJECT_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_PREF_IMM16: {
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::SETOBJECTWITHPROTO_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::CREATEARRAYWITHBUFFER_PREF_IMM16: {
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::IMPORTMODULE_PREF_ID32: {
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::STMODULEVAR_PREF_ID32: {
            info.accIn = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::COPYMODULE_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::LDMODVARBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8: {
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::GETTEMPLATEOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::GETNEXTPROPNAME_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::COPYDATAPROPERTIES_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32: {
            uint32_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8: {
            uint16_t v0 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::DEFINEASYNCFUNC_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::LDHOLE_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::COPYRESTARGS_PREF_IMM16: {
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            uint16_t v3 = READ_INST_8_4();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.vregIn.emplace_back(v2);
            info.vregIn.emplace_back(v3);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32: {
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32: {
            info.accIn = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::STCONSTTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::STLETTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::STCLASSTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32: {
            info.accOut = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = ByteCodeOffset::SEVEN;
            break;
        }
        case EcmaOpcode::STGLOBALVAR_PREF_ID32: {
            info.accIn = true;
            info.offset = ByteCodeOffset::SIX;
            break;
        }
        case EcmaOpcode::CREATEGENERATOROBJ_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::STARRAYSPREAD_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::GETITERATORNEXT_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
            uint16_t v0 = READ_INST_8_7();
            uint16_t v1 = READ_INST_8_8();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = ByteCodeOffset::TEN;
            break;
        }
        case EcmaOpcode::LDFUNCTION_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::SUPERCALL_PREF_IMM16_V8: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::FIVE;
            break;
        }
        case EcmaOpcode::SUPERCALLSPREAD_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::THREE;
            break;
        }
        case EcmaOpcode::CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::THROWIFSUPERNOTCORRECTCALL_PREF_IMM16: {
            info.accIn = true;
            info.offset = ByteCodeOffset::FOUR;
            break;
        }
        case EcmaOpcode::LDHOMEOBJECT_PREF: {
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::DEBUGGER_PREF: {
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::ISTRUE_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        case EcmaOpcode::ISFALSE_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = ByteCodeOffset::TWO;
            break;
        }
        default: {
            std::cout << opcode << std::endl;
            abort();
            break;
        }
    }
    return info;
}

void ByteCodeCircuitBuilder::InsertPhi(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            for (const auto &vreg: bytecodeInfo.vregOut) {
                defsitesInfo[vreg].insert(bb.id);
            }
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (const auto&[variable, defsites] : defsitesInfo) {
        std::cout << "variable: " << variable << " locate block have: ";
        for (auto id : defsites) {
            std::cout << id << " , ";
        }
        std::cout << std::endl;
    }
#endif

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
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintGraph(graph);
#endif
}

// Update CFG's predecessor, successor and try catch associations
void ByteCodeCircuitBuilder::UpdateCFG(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.preds.clear();
        bb.trys.clear();
        std::vector<ByteCodeRegion *> newSuccs;
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

bool ByteCodeCircuitBuilder::IsJump(EcmaOpcode opcode)
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

bool ByteCodeCircuitBuilder::IsCondJump(EcmaOpcode opcode)
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

bool ByteCodeCircuitBuilder::IsMov(EcmaOpcode opcode)
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

bool ByteCodeCircuitBuilder::IsReturn(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF:
            return true;
        default:
            return false;
    }
}

bool ByteCodeCircuitBuilder::IsThrow(EcmaOpcode opcode)
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

bool ByteCodeCircuitBuilder::IsGeneral(EcmaOpcode opcode)
{
    return !IsMov(opcode) && !IsJump(opcode) && !IsReturn(opcode);
}

void ByteCodeCircuitBuilder::BuildCircuit(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintBBInfo(graph);
#endif

    // create arg gates array
    const size_t numArgs = byteCodeGraph.method->GetNumArgs();
    const size_t offsetArgs = byteCodeGraph.method->GetNumVregs();
    std::vector<GateRef> argGates(numArgs);
    for (size_t argIdx = 0; argIdx < numArgs; argIdx++) {
        argGates.at(argIdx) = circuit_.NewGate(OpCode(OpCode::ARG), ValueCode::INT64, argIdx,
                                               {Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST))},
                                               TypeCode::NOTYPE);
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
            auto bytecodeInfo = GetByteCodeInfo(pc);
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
                                             {Circuit::NullGate()}, TypeCode::NOTYPE);
            bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_RELAY), 0,
                                              {bb.stateStart, Circuit::NullGate()}, TypeCode::NOTYPE);
        } else {
            bb.stateStart = circuit_.NewGate(OpCode(OpCode::MERGE), bb.numOfStatePreds,
                                             std::vector<GateRef>(bb.numOfStatePreds, Circuit::NullGate()),
                                             TypeCode::NOTYPE);
            bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR), bb.numOfStatePreds,
                                              std::vector<GateRef>(bb.numOfStatePreds + 1, Circuit::NullGate()),
                                              TypeCode::NOTYPE);
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
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto pcPrev = pc;
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.vregIn.size();
            if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle general ecma.* bytecodes
                auto gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), numValueInputs,
                                             std::vector<GateRef>(2 + numValueInputs, Circuit::NullGate()),
                                             TypeCode::NOTYPE);
                circuit_.NewIn(gate, 0, stateCur);
                circuit_.NewIn(gate, 1, dependCur);
                auto ifSuccess = circuit_.NewGate(OpCode(OpCode::IF_SUCCESS), 0,
                                                  {gate},
                                                  TypeCode::NOTYPE);
                auto ifException = circuit_.NewGate(OpCode(OpCode::IF_EXCEPTION), 0,
                                                    {gate},
                                                    TypeCode::NOTYPE);
                if (!bb.catchs.empty()) {
                    auto bbNext = bb.catchs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, ifException);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, gate);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back({bb.id, pcPrev, true});
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                } else {
                    circuit_.NewGate(OpCode(OpCode::THROW), 0,
                                     {ifException, gate, gate,
                                      Circuit::GetCircuitRoot(OpCode(OpCode::THROW_LIST))},
                                     TypeCode::NOTYPE);
                }
                jsgateToByteCode_[gate] = {bb.id, pcPrev};
                if (IsThrow(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                    circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     {ifSuccess, gate, TaggedValue::VALUE_HOLE,
                                      Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                     TypeCode::NOTYPE);
                    break;
                }
                stateCur = ifSuccess;
                dependCur = gate;
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, dependCur);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                }
            } else if (IsJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle conditional jump and unconditional jump bytecodes
                if (IsCondJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                    auto gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), numValueInputs,
                                                 std::vector<GateRef>(2 + numValueInputs, Circuit::NullGate()),
                                                 TypeCode::NOTYPE);
                    circuit_.NewIn(gate, 0, stateCur);
                    circuit_.NewIn(gate, 1, dependCur);
                    auto ifTrue = circuit_.NewGate(OpCode(OpCode::IF_TRUE), 0, {gate}, TypeCode::NOTYPE);
                    auto ifFalse = circuit_.NewGate(OpCode(OpCode::IF_FALSE), 0, {gate}, TypeCode::NOTYPE);
                    ASSERT(bb.succs.size() == 2);
                    int bitSet = 0;
                    for (auto &bbNext: bb.succs) {
                        if (bbNext->id == bb.id + 1) {
                            circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, ifFalse);
                            circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, gate);
                            bbNext->statePredIndex++;
                            bbNext->expandedPreds.push_back({bb.id, pcPrev, false});
                            ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                            bitSet |= 1;
                        } else {
                            circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, ifTrue);
                            circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, gate);
                            bbNext->statePredIndex++;
                            bbNext->expandedPreds.push_back({bb.id, pcPrev, false});
                            ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                            bitSet |= 2;
                        }
                    }
                    ASSERT(bitSet == 3);
                    jsgateToByteCode_[gate] = {bb.id, pcPrev};
                    break;
                } else {
                    ASSERT(bb.succs.size() == 1);
                    auto bbNext = bb.succs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, dependCur);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                    break;
                }
            } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURN_DYN) {
                // handle return.dyn bytecode
                ASSERT(bb.succs.empty());
                auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                             {stateCur, dependCur, Circuit::NullGate(),
                                              Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                             TypeCode::NOTYPE);
                jsgateToByteCode_[gate] = {bb.id, pcPrev};
                break;
            } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURNUNDEFINED_PREF) {
                // handle returnundefined bytecode
                ASSERT(bb.succs.empty());
                auto constant = circuit_.NewGate(OpCode(OpCode::CONSTANT), ValueCode::INT64,
                                                 TaggedValue::VALUE_UNDEFINED,
                                                 {Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))},
                                                 TypeCode::NOTYPE);
                auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                             {stateCur, dependCur, constant,
                                              Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                                              TypeCode::NOTYPE);
                jsgateToByteCode_[gate] = {bb.id, pcPrev};
                break;
            } else if (IsMov(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                // handle mov.dyn lda.dyn sta.dyn bytecodes
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit_.NewIn(bbNext->stateStart, bbNext->statePredIndex, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->statePredIndex + 1, dependCur);
                    bbNext->statePredIndex++;
                    bbNext->expandedPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->statePredIndex <= bbNext->numOfStatePreds);
                }
            } else {
                abort();
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
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintByteCodeInfo(graph);
#endif
    for (const auto &[key, value]: jsgateToByteCode_) {
        byteCodeToJSGate_[value.second] = key;
    }

    // resolve def-site of virtual regs and set all value inputs
    for (auto gate: circuit_.GetAllGates()) {
        auto numInsArray = circuit_.GetOpCode(gate).GetOpCodeNumInsArray(circuit_.GetBitField(gate));
        auto it = jsgateToByteCode_.find(gate);
        if (it == jsgateToByteCode_.end()) {
            continue;
        }
        const auto &[id, pc] = it->second;
        auto bytecodeInfo = GetByteCodeInfo(pc);
        [[maybe_unused]] size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.vregIn.size();
        [[maybe_unused]] size_t numValueOutputs = (bytecodeInfo.accOut ? 1 : 0) + bytecodeInfo.vregOut.size();
        ASSERT(numValueInputs == numInsArray[2]); // 2: input value num
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
                            auto curInfo = GetByteCodeInfo(pcIter);
                            pcIter += curInfo.offset;
                        }
                    }
                    std::reverse(instList.begin(), instList.end());
                    for (auto pcIter: instList) {
                        auto curInfo = GetByteCodeInfo(pcIter);
                        if (acc) {
                            if (curInfo.accOut) {
                                if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                                    acc = curInfo.accIn;
                                    if (!curInfo.vregIn.empty()) {
                                        ASSERT(!acc);
                                        ASSERT(curInfo.vregIn.size() == 1);
                                        reg = curInfo.vregIn.at(0);
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
                                    if (!curInfo.vregIn.empty()) {
                                        ASSERT(!acc);
                                        ASSERT(curInfo.vregIn.size() == 1);
                                        reg = curInfo.vregIn.at(0);
                                    }
                                } else {
                                    ans = byteCodeToJSGate_.at(pcIter);
                                    break;
                                }
                            }
                        }
                    }
                    // find def-site in value selectors of vregs
                    if (ans == Circuit::NullGate() && !acc && bb.phi.count(reg)) {
                        if (!bb.vregToValSelectorGate.count(reg)) {
                            auto gate = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), ValueCode::INT64,
                                                         bb.numOfStatePreds,
                                                         std::vector<GateRef>(
                                                                 1 + bb.numOfStatePreds, Circuit::NullGate()),
                                                         TypeCode::NOTYPE);
                            bb.vregToValSelectorGate[reg] = gate;
                            circuit_.NewIn(gate, 0, bb.stateStart);
                            for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
                                auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
                                circuit_.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                            }
                        }
                        ans = bb.vregToValSelectorGate.at(reg);
                    }
                    // find def-site in value selectors of acc
                    if (ans == Circuit::NullGate() && acc && bb.phiAcc) {
                        if (bb.valueSelectorAccGate == Circuit::NullGate()) {
                            auto gate = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), ValueCode::INT64,
                                                         bb.numOfStatePreds,
                                                         std::vector<GateRef>(
                                                             1 + bb.numOfStatePreds, Circuit::NullGate()),
                                                         TypeCode::NOTYPE);
                            bb.valueSelectorAccGate = gate;
                            circuit_.NewIn(gate, 0, bb.stateStart);
                            bool hasException = false;
                            bool hasNonException = false;
                            for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
                                auto &[predId, predPc, isException] = bb.expandedPreds.at(i);
                                if (isException) {
                                    hasException = true;
                                } else {
                                    hasNonException = true;
                                }
                                if (isException) {
                                    // exception handler will set acc
                                    auto ifExceptionGate = circuit_.GetIn(bb.stateStart, i);
                                    ASSERT(circuit_.GetOpCode(ifExceptionGate) == OpCode::IF_EXCEPTION);
                                    circuit_.NewIn(gate, i + 1, circuit_.GetIn(ifExceptionGate, 0));
                                } else {
                                    circuit_.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                                }
                            }
                            // catch block should have only exception entries
                            // normal block should have only normal entries
                            ASSERT(!hasException || !hasNonException);
                        }
                        ans = bb.valueSelectorAccGate;
                    }
                    if (ans == Circuit::NullGate() && bbId == 0) { // entry block
                        // find def-site in function args
                        ASSERT(!acc && reg >= offsetArgs && reg < offsetArgs + argGates.size());
                        return argGates.at(reg - offsetArgs);
                    }
                    if (ans == Circuit::NullGate()) {
                        // recursively find def-site in dominator block
                        return defSiteOfReg(bb.iDominator->id, bb.iDominator->end, reg, acc);
                    } else {
                        // def-site already found
                        return ans;
                    }
                };
        for (size_t valueIdx = 0; valueIdx < numInsArray[2]; valueIdx++) { // 2: input value num
            auto inIdx = valueIdx + numInsArray[0] + numInsArray[1];
            if (!circuit_.IsInGateNull(gate, inIdx)) {
                continue;
            }
            if (valueIdx < bytecodeInfo.vregIn.size()) {
                circuit_.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, bytecodeInfo.vregIn.at(valueIdx), false));
            } else {
                circuit_.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, 0, true));
            }
        }
    }
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    circuit_.PrintAllGates(*this);
#endif
}

void ByteCodeCircuitBuilder::PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos)
{
    for (auto iter = bytecodeBlockInfos.begin(); iter != bytecodeBlockInfos.end(); iter++) {
        std::cout << "offset: " << static_cast<const void *>(iter->pc) << " splitKind: " <<
                  static_cast<int32_t>(iter->splitKind) << " successor are: ";
        auto &vec = iter->succs;
        for (size_t i = 0; i < vec.size(); i++) {
            std::cout << static_cast<const void *>(vec[i]) << " , ";
        }
        std::cout << "" << std::endl;
    }
    std::cout << "-----------------------------------------------------------------------" << std::endl;
}

void ByteCodeCircuitBuilder::PrintGraph(std::vector<ByteCodeRegion> &graph)
{
    for (size_t i = 0; i < graph.size(); i++) {
        if (graph[i].isDead) {
            std::cout << "BB_" << graph[i].id << ":                               ;predsId= invalid BB" << std::endl;
            std::cout << "curStartPc: " << static_cast<const void *>(graph[i].start) <<
                      " curEndPc: " << static_cast<const void *>(graph[i].end) << std::endl;
            continue;
        }
        std::cout << "BB_" << graph[i].id << ":                               ;predsId= ";
        for (size_t k = 0; k < graph[i].preds.size(); ++k) {
            std::cout << graph[i].preds[k]->id << ", ";
        }
        std::cout << "" << std::endl;
        std::cout << "curStartPc: " << static_cast<const void *>(graph[i].start) <<
                  " curEndPc: " << static_cast<const void *>(graph[i].end) << std::endl;

        for (size_t j = 0; j < graph[i].preds.size(); j++) {
            std::cout << "predsStartPc: " << static_cast<const void *>(graph[i].preds[j]->start) <<
                      " predsEndPc: " << static_cast<const void *>(graph[i].preds[j]->end) << std::endl;
        }

        for (size_t j = 0; j < graph[i].succs.size(); j++) {
            std::cout << "succesStartPc: " << static_cast<const void *>(graph[i].succs[j]->start) <<
                      " succesEndPc: " << static_cast<const void *>(graph[i].succs[j]->end) << std::endl;
        }

        std::cout << "succesId: ";
        for (size_t j = 0; j < graph[i].succs.size(); j++) {
            std::cout << graph[i].succs[j]->id << ", ";
        }
        std::cout << "" << std::endl;


        for (size_t j = 0; j < graph[i].catchs.size(); j++) {
            std::cout << "catchStartPc: " << static_cast<const void *>(graph[i].catchs[j]->start) <<
                      " catchEndPc: " << static_cast<const void *>(graph[i].catchs[j]->end) << std::endl;
        }

        for (size_t j = 0; j < graph[i].immDomBlocks.size(); j++) {
            std::cout << "dominate block id: " << graph[i].immDomBlocks[j]->id << " startPc: " <<
                      static_cast<const void *>(graph[i].immDomBlocks[j]->start) << " endPc: " <<
                      static_cast<const void *>(graph[i].immDomBlocks[j]->end) << std::endl;
        }

        if (graph[i].iDominator) {
            std::cout << "current block " << graph[i].id <<
                      " immediate dominator is " << graph[i].iDominator->id << std::endl;
        }

        std::cout << "current block " << graph[i].id << " dominace Frontiers: ";
        for (const auto &frontier: graph[i].domFrontiers) {
            std::cout << frontier->id << " , ";
        }
        std::cout << std::endl;

        std::cout << "current block " << graph[i].id << " phi variable: ";
        for (auto variable: graph[i].phi) {
            std::cout << variable << " , ";
        }
        std::cout << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
    }
}

void ByteCodeCircuitBuilder::PrintByteCodeInfo(std::vector<ByteCodeRegion> &graph)
{
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        std::cout << "BB_" << bb.id << ": " << std::endl;
        while (pc <= bb.end) {
            auto curInfo = GetByteCodeInfo(pc);
            std::cout << "Inst_" << static_cast<int>(curInfo.opcode) << ": ";
            std::cout << "In=[";
            if (curInfo.accIn) {
                std::cout << "acc" << ",";
            }
            for (const auto &in: curInfo.vregIn) {
                std::cout << in << ",";
            }
            std::cout << "] Out=[";
            if (curInfo.accOut) {
                std::cout << "acc" << ",";
            }
            for (const auto &out: curInfo.vregOut) {
                std::cout << out << ",";
            }
            std::cout << "]";
            std::cout << std::endl;
            pc += curInfo.offset;
        }
    }
}

void ByteCodeCircuitBuilder::PrintBBInfo(std::vector<ByteCodeRegion> &graph)
{
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        std::cout << "------------------------" << std::endl;
        std::cout << "block: " << bb.id << std::endl;
        std::cout << "preds: ";
        for (auto pred: bb.preds) {
            std::cout << pred->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "succs: ";
        for (auto succ: bb.succs) {
            std::cout << succ->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "catchs: ";
        for (auto catchBlock: bb.catchs) {
            std::cout << catchBlock->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "trys: ";
        for (auto tryBlock: bb.trys) {
            std::cout << tryBlock->id << " , ";
        }
        std::cout << std::endl;
    }
}
}  // namespace panda::ecmascript::kungfu