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

#include <cmath>
#include <unordered_set>

#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ecma_macros.h"

namespace panda::ecmascript::kungfu {
bool Verifier::RunDataIntegrityCheck(const Circuit *circuit)
{
    std::unordered_set<GateRef> gatesSet;
    std::vector<GateRef> gatesList;
    gatesList.push_back(0);
    gatesSet.insert(0);
    size_t out = sizeof(Gate);
    GateRef prevGate = 0;
    while (true) {
        GateRef gate = circuit->SaveGatePtr(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<const Out *>(circuit->LoadGatePtrConst(GateRef(out)))->GetGateConst());
        if (gate < prevGate + static_cast<int64_t>(sizeof(Gate)) ||
            gate >= static_cast<int64_t>(circuit->GetCircuitDataSize())) {
            LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (bad next gate)";
            LOG_COMPILER(ERROR) << "at: " << std::dec << gate;
            return false;
        }
        gatesList.push_back(gate);
        gatesSet.insert(gate);
        prevGate = gate;
        out += Gate::GetGateSize(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<const Out *>(circuit->LoadGatePtrConst(GateRef(out)))->GetIndex() + 1);
        if (out == circuit->GetCircuitDataSize()) {
            break;
        }
        if (out > circuit->GetCircuitDataSize() || out < 0) {
            LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (out of bound access)";
            LOG_COMPILER(ERROR) << "at: " << std::dec << out;
            return false;
        }
    }
    for (const auto &gate : gatesList) {
        for (size_t idx = 0; idx < circuit->LoadGatePtrConst(gate)->GetNumIns(); idx++) {
            const In *curIn = circuit->LoadGatePtrConst(gate)->GetInConst(idx);
            if (!(circuit->GetSpaceDataStartPtrConst() < curIn && curIn < circuit->GetSpaceDataEndPtrConst())) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (corrupted in list)";
                LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                return false;
            }
            if (gatesSet.count(circuit->SaveGatePtr(curIn->GetGateConst())) == 0) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (invalid in address)";
                LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                return false;
            }
        }
        {
            const Gate *curGate = circuit->LoadGatePtrConst(gate);
            if (!curGate->IsFirstOutNull()) {
                const Out *curOut = curGate->GetFirstOutConst();
                if (!(circuit->GetSpaceDataStartPtrConst() < curOut && curOut < circuit->GetSpaceDataEndPtrConst())) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (corrupted out list)";
                    LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                    return false;
                }
                if (gatesSet.count(circuit->SaveGatePtr(curOut->GetGateConst())) == 0) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (invalid out address)";
                    LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                    return false;
                }
                while (!curOut->IsNextOutNull()) {
                    curOut = curOut->GetNextOutConst();
                    if (!(circuit->GetSpaceDataStartPtrConst() < curOut &&
                        curOut < circuit->GetSpaceDataEndPtrConst())) {
                        LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (corrupted out list)";
                        LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                        return false;
                    }
                    if (gatesSet.count(circuit->SaveGatePtr(curOut->GetGateConst())) == 0) {
                        LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (invalid out address)";
                        LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Verifier::RunStateGatesCheck(const Circuit *circuit, const std::vector<GateRef> &bbGatesList)
{
    for (const auto &bbGate : bbGatesList) {
        if (!circuit->Verify(bbGate)) {
            return false;
        }
    }
    return true;
}

bool Verifier::RunCFGSoundnessCheck(const Circuit *circuit, const std::vector<GateRef> &bbGatesList,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx)
{
    for (const auto &bbGate : bbGatesList) {
        GateAccessor gateAcc(const_cast<Circuit *>(circuit));
        for (const auto &predGate : gateAcc.ConstIns(bbGate)) {
            if (circuit->GetOpCode(predGate).IsState() || circuit->GetOpCode(predGate) == OpCode::STATE_ENTRY) {
                if (bbGatesAddrToIdx.count(predGate) == 0) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] CFG is not sound";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(predGate) << ") is pred of "
                              << "(id=" << circuit->GetId(bbGate) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(bbGate) << ") is reachable from entry";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(predGate) << ") is unreachable from entry";
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verifier::RunCFGIsDAGCheck(const Circuit *circuit)
{
    circuit->AdvanceTime();
    std::function<bool(GateRef)> dfs = [&](GateRef cur) -> bool {
        if (circuit->GetOpCode(cur) == OpCode::LOOP_BACK) {
            return true;
        }
        circuit->SetMark(cur, MarkCode::VISITED);
        GateAccessor gateAcc(const_cast<Circuit *>(circuit));
        auto uses = gateAcc.ConstUses(cur);
        for (auto use = uses.begin(); use != uses.end(); ++use) {
            if (circuit->GetOpCode(*use).IsState() && use.GetIndex() < circuit->GetOpCode(*use).GetStateCount(
                circuit->LoadGatePtrConst(*use)->GetBitField())) {
                if (circuit->GetMark(*use) == MarkCode::VISITED) {
                    LOG_COMPILER(ERROR) <<
                        "[Verifier][Error] CFG without loop back edges is not a directed acyclic graph";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(*use) << ") is succ of "
                              << "(id=" << circuit->GetId(cur) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(cur) << ") is reachable from "
                              << "(id=" << circuit->GetId(*use) << ") without loop back edges";
                    return false;
                }
                if (circuit->GetMark(*use) == MarkCode::FINISHED) {
                    continue;
                }
                if (!dfs(*use)) {
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
    return true;
}

bool Verifier::RunCFGReducibilityCheck(const Circuit *circuit, const std::vector<GateRef> &bbGatesList,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor)
{
    for (const auto &curGate : bbGatesList) {
        if (circuit->GetOpCode(curGate) == OpCode::LOOP_BACK) {
            GateAccessor gateAcc(const_cast<Circuit *>(circuit));
            auto uses = gateAcc.ConstUses(curGate);
            for (auto use = uses.begin(); use != uses.end(); use++) {
                if (use.GetIndex() >= circuit->LoadGatePtrConst(*use)->GetStateCount()) {
                    continue;
                }
                ASSERT(circuit->LoadGatePtrConst(*use)->GetOpCode().IsState());
                bool isDom = isAncestor(bbGatesAddrToIdx.at(*use), bbGatesAddrToIdx.at(curGate));
                if (!isDom) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] CFG is not reducible";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(*use) << ") is loop back succ of "
                              << "(id=" << circuit->GetId(curGate) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(*use) << ") does not dominate "
                              << "(id=" << circuit->GetId(curGate) << ")";
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verifier::RunFixedGatesCheck(const Circuit *circuit, const std::vector<GateRef> &fixedGatesList)
{
    for (const auto &fixedGate : fixedGatesList) {
        if (!circuit->Verify(fixedGate)) {
            return false;
        }
    }
    return true;
}

bool Verifier::RunFixedGatesRelationsCheck(const Circuit *circuit, const std::vector<GateRef> &fixedGatesList,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor)
{
    for (const auto &fixedGate : fixedGatesList) {
        size_t cnt = 0;
        for (const auto &predGate : circuit->GetInVector(fixedGate)) {
            if (circuit->GetOpCode(predGate).IsFixed() &&
                (circuit->GetOpCode(circuit->GetIn(fixedGate, 0)) == OpCode::LOOP_BEGIN && cnt == 2)) {
                ASSERT(cnt > 0);
                auto a = bbGatesAddrToIdx.at(circuit->GetIn(predGate, 0));
                auto b = bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(fixedGate, 0),
                    static_cast<size_t>(cnt - 1)));
                if (!isAncestor(a, b)) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] Fixed gates relationship is not consistent";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "Fixed gate (id="
                                        << circuit->GetId(predGate)
                                        << ") is pred of fixed gate (id="
                                        << circuit->GetId(fixedGate) << ")";
                    LOG_COMPILER(ERROR) << "BB_" << bbGatesAddrToIdx.at(circuit->GetIn(predGate, 0))
                                        << " does not dominate BB_"
                                        << bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(fixedGate, 0),
                                            static_cast<size_t>(cnt - 1)));
                    return false;
                }
            }
            cnt++;
        }
    }
    return true;
}

bool Verifier::RunFlowCyclesFind(const Circuit *circuit, std::vector<GateRef> *schedulableGatesListPtr,
    const std::vector<GateRef> &bbGatesList, const std::vector<GateRef> &fixedGatesList)
{
    circuit->AdvanceTime();
    std::vector<GateRef> startGateList;
    for (const auto &gate : bbGatesList) {
        for (const auto &predGate : circuit->GetInVector(gate)) {
            if (circuit->GetOpCode(predGate).IsSchedulable()) {
                if (circuit->GetMark(predGate) == MarkCode::NO_MARK) {
                    startGateList.push_back(predGate);
                    circuit->SetMark(predGate, MarkCode::VISITED);
                }
            }
        }
    }
    for (const auto &gate : fixedGatesList) {
        for (const auto &predGate : circuit->GetInVector(gate)) {
            if (circuit->GetOpCode(predGate).IsSchedulable()) {
                if (circuit->GetMark(predGate) == MarkCode::NO_MARK) {
                    startGateList.push_back(predGate);
                    circuit->SetMark(predGate, MarkCode::VISITED);
                }
            }
        }
    }
    circuit->AdvanceTime();
    std::vector<GateRef> cycleGatesList;
    GateRef meet = -1;
    std::function<bool(GateRef)> dfs = [&](GateRef cur) -> bool {
        circuit->SetMark(cur, MarkCode::VISITED);
        schedulableGatesListPtr->push_back(cur);
        size_t numIns = circuit->LoadGatePtrConst(cur)->GetNumIns();
        for (size_t idx = 0; idx < numIns; idx++) {
            const auto prev = circuit->GetIn(cur, idx);
            if (circuit->GetOpCode(prev).IsSchedulable()) {
                if (circuit->GetMark(prev) == MarkCode::VISITED) {
                    LOG_COMPILER(ERROR) <<
                        "[Verifier][Error] Found a data or depend flow cycle without passing selectors";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(prev) << ") is prev of "
                              << "(id=" << circuit->GetId(cur) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(prev) << ") is reachable from "
                              << "(id=" << circuit->GetId(cur) << ") without passing selectors";
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
        if (circuit->GetMark(startGate) == MarkCode::NO_MARK) {
            if (!dfs(startGate)) {
                LOG_COMPILER(ERROR) << "Path:";
                for (const auto &cycleGate : cycleGatesList) {
                    circuit->Print(cycleGate);
                }
                return false;
            }
        }
    }
    return true;
}

bool Verifier::RunSchedulableGatesCheck(const Circuit *circuit, const std::vector<GateRef> &schedulableGatesList)
{
    for (const auto &schedulableGate : schedulableGatesList) {
        if (!circuit->Verify(schedulableGate)) {
            return false;
        }
    }
    return true;
}

bool Verifier::RunPrologGatesCheck(const Circuit *circuit, const std::vector<GateRef> &schedulableGatesList)
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
    return true;
}

bool Verifier::RunSchedulingBoundsCheck(const Circuit *circuit, const std::vector<GateRef> &schedulableGatesList,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor,
    const std::function<size_t(size_t, size_t)> &lowestCommonAncestor)
{
    // check existence of scheduling upper bound
    std::unordered_map<GateRef, size_t> upperBound;
    {
        auto result =
            Scheduler::CalculateSchedulingUpperBound(circuit, bbGatesAddrToIdx, isAncestor, schedulableGatesList);
        if (!result.has_value()) {
            return false;
        }
        upperBound = result.value();
    }
    // check existence of scheduling lower bound
    std::unordered_map<GateRef, size_t> lowerBound;
    {
        auto result = Scheduler::CalculateSchedulingLowerBound(circuit, bbGatesAddrToIdx, lowestCommonAncestor);
        lowerBound = result.value();
    }
    // check consistency of lower bound and upper bound
    {
        ASSERT(upperBound.size() == lowerBound.size());
        for (const auto &item : lowerBound) {
            if (!isAncestor(upperBound.at(item.first), lowerBound.at(item.first))) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Bounds of gate (id=" << item.first << ") is not consistent";
                LOG_COMPILER(ERROR) << "Proof:";
                LOG_COMPILER(ERROR) << "Upper bound is BB_" << upperBound.at(item.first);
                LOG_COMPILER(ERROR) << "Lower bound is BB_" << lowerBound.at(item.first);
            }
        }
    }
    return true;
}

std::vector<GateRef> Verifier::FindFixedGates(const Circuit *circuit, const std::vector<GateRef> &bbGatesList)
{
    std::vector<GateRef> fixedGatesList;
    for (const auto &bbGate : bbGatesList) {
        for (const auto &succGate : circuit->GetOutVector(bbGate)) {
            if (circuit->GetOpCode(succGate).IsFixed()) {
                fixedGatesList.push_back(succGate);
            }
        }
    }
    return fixedGatesList;
}

bool Verifier::Run(const Circuit *circuit, bool enableLog)
{
    if (!RunDataIntegrityCheck(circuit)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] Circuit data integrity verifier failed";
        }
        return false;
    }
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit);
    if (!RunStateGatesCheck(circuit, bbGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunStateGatesCheck failed";
        }
        return false;
    }
    if (!RunCFGSoundnessCheck(circuit, bbGatesList, bbGatesAddrToIdx)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunCFGSoundnessCheck failed";
        }
        return false;
    }
    if (!RunCFGIsDAGCheck(circuit)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunCFGIsDAGCheck failed";
        }
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
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunCFGReducibilityCheck failed";
        }
        return false;
    }
    std::vector<GateRef> fixedGatesList = FindFixedGates(circuit, bbGatesList);
    if (!RunFixedGatesCheck(circuit, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunFixedGatesCheck failed";
        }
        return false;
    }
    if (!RunFixedGatesRelationsCheck(circuit, fixedGatesList, bbGatesAddrToIdx, isAncestor)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunFixedGatesRelationsCheck failed";
        }
        return false;
    }
    std::vector<GateRef> schedulableGatesList;
    if (!RunFlowCyclesFind(circuit, &schedulableGatesList, bbGatesList, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunFlowCyclesFind failed";
        }
        return false;
    }
    if (!RunSchedulableGatesCheck(circuit, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunSchedulableGatesCheck failed";
        }
        return false;
    }
    if (!RunPrologGatesCheck(circuit, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunPrologGatesCheck failed";
        }
        return false;
    }
    if (!RunSchedulingBoundsCheck(circuit, schedulableGatesList, bbGatesAddrToIdx, isAncestor, lowestCommonAncestor)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunSchedulingBoundsCheck failed";
        }
        return false;
    }

    if (enableLog) {
        LOG_COMPILER(INFO) << "[Verifier][Pass] Verifier success";
    }

    return true;
}
}  // namespace panda::ecmascript::kungfu