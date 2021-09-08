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

#include "ecmascript/compiler/verifier.h"

#include <unordered_set>

#include "ecmascript/compiler/scheduler.h"

namespace kungfu {
bool Verifier::RunDataIntegrityCheck(const Circuit *circuit)
{
    std::unordered_set<AddrShift> gatesSet;
    std::vector<AddrShift> gatesList;
    gatesList.push_back(0);
    gatesSet.insert(0);
    size_t out = sizeof(Gate);
    AddrShift prevGate = 0;
    while (true) {
        AddrShift gate = circuit->SaveGatePtr(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<const Out *>(circuit->LoadGatePtrConst(AddrShift(out)))->GetGateConst());
        if (gate < prevGate + static_cast<int64_t>(sizeof(Gate)) ||
            gate >= static_cast<int64_t>(circuit->GetCircuitDataSize())) {
            std::cerr << "[Verifier][Error] Circuit data is corrupted (bad next gate)" << std::endl;
            std::cerr << "at: " << std::dec << gate << std::endl;
            return false;
        }
        gatesList.push_back(gate);
        gatesSet.insert(gate);
        prevGate = gate;
        out += Gate::GetGateSize(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<const Out *>(circuit->LoadGatePtrConst(AddrShift(out)))->GetIndex() + 1);
        if (out == circuit->GetCircuitDataSize()) {
            break;
        }
        if (out > circuit->GetCircuitDataSize() || out < 0) {
            std::cerr << "[Verifier][Error] Circuit data is corrupted (out of bound access)" << std::endl;
            std::cerr << "at: " << std::dec << out << std::endl;
            return false;
        }
    }
    for (const auto &gate : gatesList) {
        for (size_t idx = 0; idx < circuit->LoadGatePtrConst(gate)->GetNumIns(); idx++) {
            const In *curIn = circuit->LoadGatePtrConst(gate)->GetInConst(idx);
            if (!(circuit->GetSpaceDataStartPtrConst() < curIn && curIn < circuit->GetSpaceDataEndPtrConst())) {
                std::cerr << "[Verifier][Error] Circuit data is corrupted (corrupted in list)" << std::endl;
                std::cerr << "id: " << std::dec << circuit->GetId(gate) << std::endl;
                return false;
            }
            if (gatesSet.count(circuit->SaveGatePtr(curIn->GetGateConst())) == 0) {
                std::cerr << "[Verifier][Error] Circuit data is corrupted (invalid in address)" << std::endl;
                std::cerr << "id: " << std::dec << circuit->GetId(gate) << std::endl;
                return false;
            }
        }
        {
            const Gate *curGate = circuit->LoadGatePtrConst(gate);
            if (!curGate->IsFirstOutNull()) {
                const Out *curOut = curGate->GetFirstOutConst();
                if (!(circuit->GetSpaceDataStartPtrConst() < curOut && curOut < circuit->GetSpaceDataEndPtrConst())) {
                    std::cerr << "[Verifier][Error] Circuit data is corrupted (corrupted out list)" << std::endl;
                    std::cerr << "id: " << std::dec << circuit->GetId(gate) << std::endl;
                    return false;
                }
                if (gatesSet.count(circuit->SaveGatePtr(curOut->GetGateConst())) == 0) {
                    std::cerr << "[Verifier][Error] Circuit data is corrupted (invalid out address)" << std::endl;
                    std::cerr << "id: " << std::dec << circuit->GetId(gate) << std::endl;
                    return false;
                }
                while (!curOut->IsNextOutNull()) {
                    curOut = curOut->GetNextOutConst();
                    if (!(circuit->GetSpaceDataStartPtrConst() < curOut &&
                        curOut < circuit->GetSpaceDataEndPtrConst())) {
                        std::cerr << "[Verifier][Error] Circuit data is corrupted (corrupted out list)" << std::endl;
                        std::cerr << "id: " << std::dec << circuit->GetId(gate) << std::endl;
                        return false;
                    }
                    if (gatesSet.count(circuit->SaveGatePtr(curOut->GetGateConst())) == 0) {
                        std::cerr << "[Verifier][Error] Circuit data is corrupted (invalid out address)" << std::endl;
                        std::cerr << "id: " << std::dec << circuit->GetId(gate) << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    std::cerr << "[Verifier][Pass] Circuit data integrity is verified" << std::endl;
    return true;
}

bool Verifier::RunStateGatesCheck(const Circuit *circuit, const std::vector<AddrShift> &bbGatesList)
{
    for (const auto &bbGate : bbGatesList) {
        if (!circuit->Verify(bbGate)) {
            return false;
        }
    }
    std::cerr << "[Verifier][Pass] State gates input list schema is verified" << std::endl;
    return true;
}

bool Verifier::RunCFGSoundnessCheck(const Circuit *circuit, const std::vector<AddrShift> &bbGatesList,
    const std::unordered_map<AddrShift, size_t> &bbGatesAddrToIdx)
{
    for (const auto &bbGate : bbGatesList) {
        for (const auto &predGate : circuit->GetInVector(bbGate)) {
            if (circuit->GetOpCode(predGate).IsState()) {
                if (bbGatesAddrToIdx.count(predGate) == 0) {
                    std::cerr << "[Verifier][Error] CFG is not sound" << std::endl;
                    std::cerr << "Proof:" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(predGate) << ") is pred of "
                              << "(id=" << circuit->GetId(bbGate) << ")" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(bbGate) << ") is reachable from entry" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(predGate) << ") is unreachable from entry" << std::endl;
                    return false;
                }
            }
        }
    }
    std::cerr << "[Verifier][Pass] CFG is sound" << std::endl;
    return true;
}

bool Verifier::RunCFGIsDAGCheck(const Circuit *circuit)
{
    circuit->AdvanceTime();
    std::function<bool(AddrShift)> dfs = [&](AddrShift cur) -> bool {
        if (circuit->GetOpCode(cur) == OpCode::LOOP_BACK) {
            return true;
        }
        circuit->SetMark(cur, MarkCode::VISITED);
        for (const auto &succ : circuit->GetOutVector(cur)) {
            if (circuit->GetOpCode(succ).IsState()) {
                if (circuit->GetMark(succ) == MarkCode::VISITED) {
                    std::cerr << "[Verifier][Error] CFG without loop back edges is not directed acyclic graph"
                              << std::endl;
                    std::cerr << "Proof:" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(succ) << ") is succ of "
                              << "(id=" << circuit->GetId(cur) << ")" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(cur) << ") is reachable from "
                              << "(id=" << circuit->GetId(succ) << ") without loop back edges" << std::endl;
                    return false;
                }
                if (circuit->GetMark(succ) == MarkCode::FINISHED) {
                    return true;
                }
                if (!dfs(succ)) {
                    return false;
                }
            }
        }
        circuit->SetMark(cur, MarkCode::FINISHED);
        return true;
    };
    auto root = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
    if (!dfs(root)) {
        return false;
    }
    std::cerr << "[Verifier][Pass] CFG without loop back edges is directed acyclic graph" << std::endl;
    return true;
}

bool Verifier::RunCFGReducibilityCheck(const Circuit *circuit, const std::vector<AddrShift> &bbGatesList,
    const std::unordered_map<AddrShift, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor)
{
    for (const auto &curGate : bbGatesList) {
        if (circuit->GetOpCode(curGate) == OpCode::LOOP_BACK) {
            for (const auto &succGate : circuit->GetOutVector(curGate)) {
                if (circuit->GetOpCode(succGate).IsState()) {
                    bool isDom = isAncestor(bbGatesAddrToIdx.at(succGate), bbGatesAddrToIdx.at(curGate));
                    if (!isDom) {
                        std::cerr << "[Verifier][Error] CFG is not reducible" << std::endl;
                        std::cerr << "Proof:" << std::endl;
                        std::cerr << "(id=" << circuit->GetId(succGate) << ") is loop back succ of "
                                  << "(id=" << circuit->GetId(curGate) << ")" << std::endl;
                        std::cerr << "(id=" << circuit->GetId(succGate) << ") does not dominate "
                                  << "(id=" << circuit->GetId(curGate) << ")" << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    std::cerr << "[Verifier][Pass] CFG is reducible" << std::endl;
    return true;
}

bool Verifier::RunFixedGatesCheck(const Circuit *circuit, const std::vector<AddrShift> &fixedGatesList)
{
    for (const auto &fixedGate : fixedGatesList) {
        if (!circuit->Verify(fixedGate)) {
            return false;
        }
    }
    std::cerr << "[Verifier][Pass] Fixed gates input list schema is verified" << std::endl;
    return true;
}

bool Verifier::RunFixedGatesRelationsCheck(const Circuit *circuit, const std::vector<AddrShift> &fixedGatesList,
    const std::unordered_map<AddrShift, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor)
{
    for (const auto &fixedGate : fixedGatesList) {
        size_t cnt = 0;
        for (const auto &predGate : circuit->GetInVector(fixedGate)) {
            if (circuit->GetOpCode(predGate).IsFixed() &&
                circuit->GetOpCode(circuit->GetIn(predGate, 0)) != OpCode::LOOP_BACK) {
                ASSERT(cnt > 0);
                auto a = bbGatesAddrToIdx.at(circuit->GetIn(predGate, 0));
                auto b = bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(fixedGate, 0), cnt - 1));
                if (!isAncestor(a, b)) {
                    std::cerr << "[Verifier][Error] Fixed gates relationship is not consistent" << std::endl;
                    std::cerr << "Proof:" << std::endl;
                    std::cerr << "Fixed gate (id=" << predGate << ") is pred of fixed gate (id=" << fixedGate << ")"
                              << std::endl;
                    std::cerr << "BB_" << bbGatesAddrToIdx.at(circuit->GetIn(predGate, 0)) << " does not dominate BB_"
                              << bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(fixedGate, 0), cnt - 1))
                              << std::endl;
                    return false;
                }
            }
            cnt++;
        }
    }
    std::cerr << "[Verifier][Pass] Fixed gates relationship is consistent" << std::endl;
    return true;
}

bool Verifier::RunFlowCyclesFind(const Circuit *circuit, std::vector<AddrShift> *schedulableGatesListPtr,
    const std::vector<AddrShift> &bbGatesList, const std::vector<AddrShift> &fixedGatesList)
{
    circuit->AdvanceTime();
    std::vector<AddrShift> startGateList;
    for (const auto &gate : bbGatesList) {
        for (const auto &predGate : circuit->GetInVector(gate)) {
            if (circuit->GetOpCode(predGate).IsSchedulable()) {
                if (circuit->GetMark(predGate) == MarkCode::EMPTY) {
                    startGateList.push_back(predGate);
                    circuit->SetMark(predGate, MarkCode::VISITED);
                }
            }
        }
    }
    for (const auto &gate : fixedGatesList) {
        for (const auto &predGate : circuit->GetInVector(gate)) {
            if (circuit->GetOpCode(predGate).IsSchedulable()) {
                if (circuit->GetMark(predGate) == MarkCode::EMPTY) {
                    startGateList.push_back(predGate);
                    circuit->SetMark(predGate, MarkCode::VISITED);
                }
            }
        }
    }
    circuit->AdvanceTime();
    std::vector<AddrShift> cycleGatesList;
    AddrShift meet = -1;
    std::function<bool(AddrShift)> dfs = [&](AddrShift cur) -> bool {
        circuit->SetMark(cur, MarkCode::VISITED);
        schedulableGatesListPtr->push_back(cur);
        size_t numIns = circuit->LoadGatePtrConst(cur)->GetNumIns();
        for (size_t idx = 0; idx < numIns; idx++) {
            const auto prev = circuit->GetIn(cur, idx);
            if (circuit->GetOpCode(prev).IsSchedulable()) {
                if (circuit->GetMark(prev) == MarkCode::VISITED) {
                    std::cerr << "[Verifier][Error] Found a data or depend flow cycle without passing selectors"
                              << std::endl;
                    std::cerr << "Proof:" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(prev) << ") is prev of "
                              << "(id=" << circuit->GetId(cur) << ")" << std::endl;
                    std::cerr << "(id=" << circuit->GetId(prev) << ") is reachable from "
                              << "(id=" << circuit->GetId(cur) << ") without passing selectors" << std::endl;
                    meet = prev;
                    cycleGatesList.push_back(cur);
                    return false;
                }
                if (circuit->GetMark(prev) != MarkCode::FINISHED) {
                    if (!dfs(prev)) {
                        if (meet != -1) {
                            cycleGatesList.push_back(cur);
                        }
                        if (meet == cur) {
                            meet = -1;
                        }
                        return false;
                    }
                }
            }
        }
        circuit->SetMark(cur, MarkCode::FINISHED);
        return true;
    };
    for (const auto &startGate : startGateList) {
        if (circuit->GetMark(startGate) == MarkCode::EMPTY) {
            if (!dfs(startGate)) {
                std::cerr << "Path:" << std::endl;
                for (const auto &cycleGate : cycleGatesList) {
                    circuit->Print(cycleGate);
                }
                return false;
            }
        }
    }
    std::cerr << "[Verifier][Pass] Every directed data or depend flow cycles in circuit contain selectors" << std::endl;
    return true;
}

bool Verifier::RunSchedulableGatesCheck(const Circuit *circuit, const std::vector<AddrShift> &schedulableGatesList)
{
    for (const auto &schedulableGate : schedulableGatesList) {
        if (!circuit->Verify(schedulableGate)) {
            return false;
        }
    }
    std::cerr << "[Verifier][Pass] Schedulable gates input list schema is verified" << std::endl;
    return true;
}

bool Verifier::RunPrologGatesCheck(const Circuit *circuit, const std::vector<AddrShift> &schedulableGatesList)
{
    for (const auto &schedulableGate : schedulableGatesList) {
        for (const auto &predGate : circuit->GetInVector(schedulableGate)) {
            if (circuit->GetOpCode(predGate).IsProlog()) {
                if (!circuit->Verify(predGate)) {
                    return false;
                }
            }
        }
    }
    std::cerr << "[Verifier][Pass] Prolog gates input list schema is verified" << std::endl;
    return true;
}

bool Verifier::RunSchedulingBoundsCheck(const Circuit *circuit, const std::vector<AddrShift> &schedulableGatesList,
    const std::unordered_map<AddrShift, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor,
    const std::function<size_t(size_t, size_t)> &lowestCommonAncestor)
{
    // check existence of scheduling upper bound
    std::unordered_map<AddrShift, size_t> upperBound;
    {
        auto result =
            Scheduler::CalculateSchedulingUpperBound(circuit, bbGatesAddrToIdx, isAncestor, schedulableGatesList);
        if (!result.has_value()) {
            return false;
        }
        upperBound = result.value();
        std::cerr << "[Verifier][Pass] Scheduling upper bounds of all schedulable gates exist" << std::endl;
    }
    // check existence of scheduling lower bound
    std::unordered_map<AddrShift, size_t> lowerBound;
    {
        auto result = Scheduler::CalculateSchedulingLowerBound(circuit, bbGatesAddrToIdx, lowestCommonAncestor);
        lowerBound = result.value();
        std::cerr << "[Verifier][Pass] Scheduling lower bounds of all schedulable gates exist" << std::endl;
    }
    // check consistency of lower bound and upper bound
    {
        ASSERT(upperBound.size() == lowerBound.size());
        for (const auto &item : lowerBound) {
            if (!isAncestor(upperBound.at(item.first), lowerBound.at(item.first))) {
                std::cerr << "[Verifier][Error] Bounds of gate (id=" << item.first << ") is not consistent"
                          << std::endl;
                std::cerr << "Proof:" << std::endl;
                std::cerr << "Upper bound is BB_" << upperBound.at(item.first) << std::endl;
                std::cerr << "Lower bound is BB_" << lowerBound.at(item.first) << std::endl;
            }
        }
        std::cerr << "[Verifier][Pass] Bounds of all schedulable gates are consistent" << std::endl;
    }
    return true;
}

std::vector<AddrShift> Verifier::FindFixedGates(const Circuit *circuit, const std::vector<AddrShift> &bbGatesList)
{
    std::vector<AddrShift> fixedGatesList;
    for (const auto &bbGate : bbGatesList) {
        for (const auto &succGate : circuit->GetOutVector(bbGate)) {
            if (circuit->GetOpCode(succGate).IsFixed()) {
                fixedGatesList.push_back(succGate);
            }
        }
    }
    return fixedGatesList;
}

bool Verifier::Run(const Circuit *circuit)
{
    if (!RunDataIntegrityCheck(circuit)) {
        return false;
    }
    std::vector<AddrShift> bbGatesList;
    std::unordered_map<AddrShift, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit);
    std::cerr << std::dec;
    if (!RunStateGatesCheck(circuit, bbGatesList)) {
        return false;
    }
    if (!RunCFGSoundnessCheck(circuit, bbGatesList, bbGatesAddrToIdx)) {
        return false;
    }
    if (!RunCFGIsDAGCheck(circuit)) {
        return false;
    }
    std::vector<std::vector<size_t>> sonList(bbGatesList.size());
    for (size_t idx = 1; idx < immDom.size(); idx++) {
        sonList[immDom[idx]].push_back(idx);
    }
    const size_t sizeLog = std::ceil(std::log2(static_cast<double>(bbGatesList.size())) + 1);
    std::vector<size_t> timeIn(bbGatesList.size());
    std::vector<size_t> timeOut(bbGatesList.size());
    std::vector<std::vector<size_t>> jumpUp;
    jumpUp.assign(bbGatesList.size(), std::vector<size_t>(sizeLog + 1));
    {
        size_t timestamp = 0;
        std::function<void(size_t, size_t)> dfs = [&](size_t cur, size_t prev) {
            timeIn[cur] = timestamp++;
            jumpUp[cur][0] = prev;
            for (size_t stepSize = 1; stepSize <= sizeLog; stepSize++) {
                jumpUp[cur][stepSize] = jumpUp[jumpUp[cur][stepSize - 1]][stepSize - 1];
            }
            for (const auto &succ : sonList[cur]) {
                dfs(succ, cur);
            }
            timeOut[cur] = timestamp++;
        };
        size_t root = 0;
        dfs(root, root);
    }
    auto isAncestor = [timeIn, timeOut](size_t nodeA, size_t nodeB) -> bool {
        return timeIn[nodeA] <= timeIn[nodeB] && timeOut[nodeA] >= timeOut[nodeB];
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
    if (!RunCFGReducibilityCheck(circuit, bbGatesList, bbGatesAddrToIdx, isAncestor)) {
        return false;
    }
    std::vector<AddrShift> fixedGatesList = FindFixedGates(circuit, bbGatesList);
    if (!RunFixedGatesCheck(circuit, fixedGatesList)) {
        return false;
    }
    if (!RunFixedGatesRelationsCheck(circuit, fixedGatesList, bbGatesAddrToIdx, isAncestor)) {
        return false;
    }
    std::vector<AddrShift> schedulableGatesList;
    if (!RunFlowCyclesFind(circuit, &schedulableGatesList, bbGatesList, fixedGatesList)) {
        return false;
    }
    if (!RunSchedulableGatesCheck(circuit, fixedGatesList)) {
        return false;
    }
    if (!RunPrologGatesCheck(circuit, fixedGatesList)) {
        return false;
    }
    if (!RunSchedulingBoundsCheck(circuit, schedulableGatesList, bbGatesAddrToIdx, isAncestor, lowestCommonAncestor)) {
        return false;
    }
    return true;
}
}  // namespace kungfu