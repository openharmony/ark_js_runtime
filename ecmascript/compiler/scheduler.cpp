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

#include "ecmascript/compiler/scheduler.h"

#include <cmath>

#include "ecmascript/compiler/verifier.h"

namespace panda::ecmascript::kungfu {
using DominatorTreeInfo = std::tuple<std::vector<GateRef>, std::unordered_map<GateRef, size_t>,
    std::vector<size_t>>;
DominatorTreeInfo Scheduler::CalculateDominatorTree(const Circuit *circuit)
{
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::unordered_map<GateRef, size_t> dfsTimestamp;
    circuit->AdvanceTime();
    {
        size_t timestamp = 0;
        std::deque<GateRef> pendingList;
        auto startGate = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
        circuit->SetMark(startGate, MarkCode::VISITED);
        pendingList.push_back(startGate);
        while (!pendingList.empty()) {
            auto curGate = pendingList.back();
            dfsTimestamp[curGate] = timestamp++;
            pendingList.pop_back();
            bbGatesList.push_back(curGate);
            if (circuit->GetOpCode(curGate) != OpCode::LOOP_BACK) {
                for (const auto &succGate : circuit->GetOutVector(curGate)) {
                    if (circuit->GetOpCode(succGate).IsState() && circuit->GetMark(succGate) == MarkCode::NO_MARK) {
                        circuit->SetMark(succGate, MarkCode::VISITED);
                        pendingList.push_back(succGate);
                    }
                }
            }
        }
        for (size_t idx = 0; idx < bbGatesList.size(); idx++) {
            bbGatesAddrToIdx[bbGatesList[idx]] = idx;
        }
    }
    std::vector<size_t> immDom(bbGatesList.size());
    {
        std::vector<std::vector<size_t>> dom(bbGatesList.size());
        dom[0] = {0};
        for (size_t idx = 1; idx < dom.size(); idx++) {
            dom[idx].resize(dom.size());
            std::iota(dom[idx].begin(), dom[idx].end(), 0);
        }
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t idx = 1; idx < dom.size(); idx++) {
                auto &curDom = dom[idx];
                size_t origSize = curDom.size();
                curDom.resize(dom.size());
                std::iota(curDom.begin(), curDom.end(), 0);
                for (const auto &predGate : circuit->GetInVector(bbGatesList[idx])) {
                    if (bbGatesAddrToIdx.count(predGate) > 0) {
                        std::vector<size_t> tmp(curDom.size());
                        const auto &predDom = dom[bbGatesAddrToIdx[predGate]];
                        auto it = std::set_intersection(
                            curDom.begin(), curDom.end(), predDom.begin(), predDom.end(), tmp.begin());
                        tmp.resize(it - tmp.begin());
                        curDom = tmp;
                    }
                }
                auto it = std::find(curDom.begin(), curDom.end(), idx);
                if (it == curDom.end()) {
                    curDom.push_back(idx);
                    std::sort(curDom.begin(), curDom.end());
                }
                if (dom[idx].size() != origSize) {
                    changed = true;
                }
            }
        }
        immDom[0] = dom[0].front();
        for (size_t idx = 1; idx < dom.size(); idx++) {
            auto it = std::remove(dom[idx].begin(), dom[idx].end(), idx);
            dom[idx].resize(it - dom[idx].begin());
            immDom[idx] = *std::max_element(dom[idx].begin(), dom[idx].end(),
                [bbGatesList, dfsTimestamp](const size_t &lhs, const size_t &rhs) -> bool {
                    return dfsTimestamp.at(bbGatesList[lhs]) < dfsTimestamp.at(bbGatesList[rhs]);
                });
        }
    }
    return {bbGatesList, bbGatesAddrToIdx, immDom};
}

std::vector<std::vector<GateRef>> Scheduler::Run(const Circuit *circuit)
{
#ifndef NDEBUG
    if (!Verifier::Run(circuit)) {
        UNREACHABLE();
    }
#endif
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit);
    std::vector<std::vector<GateRef>> result(bbGatesList.size());
    for (size_t idx = 0; idx < bbGatesList.size(); idx++) {
        result[idx].push_back(bbGatesList[idx]);
    }
    // assuming CFG is always reducible
    std::vector<std::vector<size_t>> sonList(result.size());
    for (size_t idx = 1; idx < immDom.size(); idx++) {
        sonList[immDom[idx]].push_back(idx);
    }
    const size_t sizeLog = std::ceil(std::log2(static_cast<double>(result.size())) + 1);
    std::vector<size_t> timeIn(result.size());
    std::vector<size_t> timeOut(result.size());
    std::vector<std::vector<size_t>> jumpUp;
    jumpUp.assign(result.size(), std::vector<size_t>(sizeLog + 1));
    {
        size_t timestamp = 0;
        std::function<void(size_t, size_t)> dfs = [&](size_t cur, size_t prev) {
            timeIn[cur] = timestamp;
            timestamp++;
            jumpUp[cur][0] = prev;
            for (size_t stepSize = 1; stepSize <= sizeLog; stepSize++) {
                jumpUp[cur][stepSize] = jumpUp[jumpUp[cur][stepSize - 1]][stepSize - 1];
            }
            for (const auto &succ : sonList[cur]) {
                dfs(succ, cur);
            }
            timeOut[cur] = timestamp;
            timestamp++;
        };
        size_t root = 0;
        dfs(root, root);
    }
    auto isAncestor = [&](size_t nodeA, size_t nodeB) -> bool {
        return (timeIn[nodeA] <= timeIn[nodeB]) && (timeOut[nodeA] >= timeOut[nodeB]);
    };
    auto lowestCommonAncestor = [&](size_t nodeA, size_t nodeB) -> size_t {
        if (isAncestor(nodeA, nodeB)) {
            return nodeA;
        }
        if (isAncestor(nodeB, nodeA)) {
            return nodeB;
        }
        for (size_t stepSize = sizeLog + 1; stepSize > 0; stepSize--) {
            if (!isAncestor(jumpUp[nodeA][stepSize - 1], nodeB)) {
                nodeA = jumpUp[nodeA][stepSize - 1];
            }
        }
        return jumpUp[nodeA][0];
    };
    {
        std::vector<GateRef> order;
        auto lowerBound =
            Scheduler::CalculateSchedulingLowerBound(circuit, bbGatesAddrToIdx, lowestCommonAncestor, &order).value();
        for (const auto &schedulableGate : order) {
            result[lowerBound.at(schedulableGate)].push_back(schedulableGate);
        }
        auto argList = circuit->GetOutVector(Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST)));
        std::sort(argList.begin(), argList.end(), [&](const GateRef &lhs, const GateRef &rhs) -> bool {
            return circuit->GetBitField(lhs) > circuit->GetBitField(rhs);
        });
        for (const auto &arg : argList) {
            result.front().push_back(arg);
        }
        for (const auto &bbGate : bbGatesList) {
            for (const auto &succGate : circuit->GetOutVector(bbGate)) {
                if (circuit->GetOpCode(succGate).IsFixed()) {
                    result[bbGatesAddrToIdx.at(circuit->GetIn(succGate, 0))].push_back(succGate);
                }
            }
        }
    }
    return result;
}

std::optional<std::unordered_map<GateRef, size_t>> Scheduler::CalculateSchedulingUpperBound(const Circuit *circuit,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor, const std::vector<GateRef> &schedulableGatesList)
{
    std::unordered_map<GateRef, size_t> upperBound;
    std::function<std::optional<size_t>(GateRef)> dfs = [&](GateRef curGate) -> std::optional<size_t> {
        if (upperBound.count(curGate) > 0) {
            return upperBound[curGate];
        }
        if (circuit->GetOpCode(curGate).IsProlog() || circuit->GetOpCode(curGate).IsRoot()) {
            return 0;
        }
        if (circuit->GetOpCode(curGate).IsFixed()) {
            return bbGatesAddrToIdx.at(circuit->GetIn(curGate, 0));
        }
        if (circuit->GetOpCode(curGate).IsState()) {
            return bbGatesAddrToIdx.at(curGate);
        }
        // then cur is schedulable
        size_t curUpperBound = 0;
        for (const auto &predGate : circuit->GetInVector(curGate)) {
            auto predResult = dfs(predGate);
            if (!predResult.has_value()) {
                return std::nullopt;
            }
            auto predUpperBound = predResult.value();
            if (!isAncestor(curUpperBound, predUpperBound) && !isAncestor(predUpperBound, curUpperBound)) {
                std::cerr << "[Verifier][Error] Scheduling upper bound of gate (id="
                          << circuit->LoadGatePtrConst(curGate)->GetId() << ") does not exist"
                          << std::endl;
                return std::nullopt;
            }
            if (isAncestor(curUpperBound, predUpperBound)) {
                curUpperBound = predUpperBound;
            }
        }
        return (upperBound[curGate] = curUpperBound);
    };
    for (const auto &schedulableGate : schedulableGatesList) {
        if (upperBound.count(schedulableGate) == 0) {
            if (!dfs(schedulableGate).has_value()) {
                return std::nullopt;
            }
        }
    }
    return upperBound;
}

std::optional<std::unordered_map<GateRef, size_t>> Scheduler::CalculateSchedulingLowerBound(const Circuit *circuit,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<size_t(size_t, size_t)> &lowestCommonAncestor, std::vector<GateRef> *order)
{
    std::unordered_map<GateRef, size_t> lowerBound;
    std::unordered_map<GateRef, size_t> useCount;
    std::deque<GateRef> pendingList;
    std::vector<GateRef> bbAndFixedGatesList;
    for (const auto &item : bbGatesAddrToIdx) {
        bbAndFixedGatesList.push_back(item.first);
        for (const auto &succGate : circuit->GetOutVector(item.first)) {
            if (circuit->GetOpCode(succGate).IsFixed()) {
                bbAndFixedGatesList.push_back(succGate);
            }
        }
    }
    std::function<void(GateRef)> dfsVisit = [&](GateRef curGate) {
        for (const auto &prevGate : circuit->GetInVector(curGate)) {
            if (circuit->GetOpCode(prevGate).IsSchedulable()) {
                useCount[prevGate]++;
                if (useCount[prevGate] == 1) {
                    dfsVisit(prevGate);
                }
            }
        }
    };
    for (const auto &gate : bbAndFixedGatesList) {
        dfsVisit(gate);
    }
    std::function<void(GateRef)> dfsFinish = [&](GateRef curGate) {
        size_t cnt = 0;
        for (const auto &prevGate : circuit->GetInVector(curGate)) {
            if (circuit->GetOpCode(prevGate).IsSchedulable()) {
                useCount[prevGate]--;
                size_t curLowerBound;
                if (circuit->GetOpCode(curGate).IsState()) {  // cur_opcode would not be STATE_ENTRY
                    curLowerBound = bbGatesAddrToIdx.at(curGate);
                } else if (circuit->GetOpCode(curGate).IsFixed()) {
                    ASSERT(cnt > 0);
                    curLowerBound = bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(curGate, 0), cnt - 1));
                } else {
                    curLowerBound = lowerBound.at(curGate);
                }
                if (lowerBound.count(prevGate) == 0) {
                    lowerBound[prevGate] = curLowerBound;
                } else {
                    lowerBound[prevGate] = lowestCommonAncestor(lowerBound[prevGate], curLowerBound);
                }
                if (useCount[prevGate] == 0) {
                    if (order != nullptr) {
                        order->push_back(prevGate);
                    }
                    dfsFinish(prevGate);
                }
            }
            cnt++;
        }
    };
    for (const auto &gate : bbAndFixedGatesList) {
        dfsFinish(gate);
    }
    return lowerBound;
}

void Scheduler::Print(const std::vector<std::vector<GateRef>> *cfg, const Circuit *circuit)
{
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit);
    std::cout << "==========================================================================" << std::endl;
    for (size_t bbIdx = 0; bbIdx < cfg->size(); bbIdx++) {
        std::cout << "BB_" << bbIdx << "_" << circuit->GetOpCode((*cfg)[bbIdx].front()).Str() << ":"
                  << "  immDom=" << immDom[bbIdx];
        std::cout << "  pred=[";
        bool isFirst = true;
        for (const auto &predStates : circuit->GetInVector((*cfg)[bbIdx].front())) {
            if (circuit->GetOpCode(predStates).IsState() || circuit->GetOpCode(predStates) == OpCode::STATE_ENTRY) {
                std::cout << (isFirst ? "" : " ") << bbGatesAddrToIdx.at(predStates);
                isFirst = false;
            }
        }
        std::cout << "]  succ=[";
        isFirst = true;
        for (const auto &succStates : circuit->GetOutVector((*cfg)[bbIdx].front())) {
            if (circuit->GetOpCode(succStates).IsState() || circuit->GetOpCode(succStates) == OpCode::STATE_ENTRY) {
                std::cout << (isFirst ? "" : " ") << bbGatesAddrToIdx.at(succStates);
                isFirst = false;
            }
        }
        std::cout << "]";
        std::cout << std::endl;
        for (size_t instIdx = (*cfg)[bbIdx].size(); instIdx > 0; instIdx--) {
            circuit->Print((*cfg)[bbIdx][instIdx - 1]);
        }
    }
    std::cout << "==========================================================================" << std::endl;
}
}  // namespace panda::ecmascript::kungfu