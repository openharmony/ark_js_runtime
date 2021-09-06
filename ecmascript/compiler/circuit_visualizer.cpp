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
#include "ecmascript/compiler/circuit_visualizer.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"

namespace kungfu {
#ifndef NDEBUG
void CircuitVisualizer::DumpDFG() {}

void CircuitVisualizer::PrintGateVisit(const Gate *node, std::ostream &os, int depth, int indentation)
{
    for (int i = 0; i < indentation; ++i) {
        os << "---";
    }
    if (node != nullptr) {
        PrintSingleGate(node, os);
    } else {
        os << "(NULL)";
        return;
    }
    os << std::endl;
    if (depth <= 0) {
        return;
    }

    if (!node->IsFirstOutNull()) {
        Out *curOut = node->GetFirstOut();
        PrintGateVisit(curOut->GetGate(), os, depth - 1, indentation + 1);
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOut();
            PrintGateVisit(curOut->GetGate(), os, depth - 1, indentation + 1);
        }
    }
}

void CircuitVisualizer::PrintGate(const Gate *node, int depth)
{
    PrintGateVisit(node, std::cout, depth, 0);
    std::flush(std::cout);
}

void CircuitVisualizer::PrintSingleGate(const Gate *node, std::ostream &os)
{
    ASSERT(node != nullptr);
    (void)os;
    node->Print();
}

#endif
}  // namespace kungfu
