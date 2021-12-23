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

#ifndef ECMASCRIPT_COMPILER_SCHEDULER_H
#define ECMASCRIPT_COMPILER_SCHEDULER_H

#include <algorithm>
#include <deque>
#include <functional>
#include <numeric>
#include <unordered_map>

#include "ecmascript/compiler/circuit.h"

namespace panda::ecmascript::kungfu {
using ControlFlowGraph = std::vector<std::vector<GateRef>>;
class Scheduler {
public:
    static std::tuple<std::vector<GateRef>, std::unordered_map<GateRef, size_t>, std::vector<size_t>>
    CalculateDominatorTree(const Circuit *circuit);
    static ControlFlowGraph Run(const Circuit *circuit);
    static std::optional<std::unordered_map<GateRef, size_t>> CalculateSchedulingUpperBound(const Circuit *circuit,
        const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
        const std::function<bool(size_t, size_t)> &isAncestor, const std::vector<GateRef> &schedulableGatesList);
    static std::optional<std::unordered_map<GateRef, size_t>> CalculateSchedulingLowerBound(const Circuit *circuit,
        const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
        const std::function<size_t(size_t, size_t)> &lowestCommonAncestor, std::vector<GateRef> *order = nullptr);
    static void Print(const ControlFlowGraph *cfg, const Circuit *circuit);
};
};  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_SCHEDULER_H
