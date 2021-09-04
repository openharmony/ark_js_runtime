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

#ifndef PANDA_RUNTIME_ECMASCRIPT_COMPILER_CIRCUIT_VISUALIZER_H
#define PANDA_RUNTIME_ECMASCRIPT_COMPILER_CIRCUIT_VISUALIZER_H

#include <ostream>
#include <fstream>
#include <string>

namespace kungfu {
#ifndef NDEBUG
// This class dump graph in graphviz format
// Option: --compiler-graph-visualizer

class CircuitVisualizer {
public:
    explicit CircuitVisualizer(Circuit *graph) : graph_(graph) {}

    void CreateDumpFile(const char *file_name);
    void DumpDFG();
    void PrintGate(Gate *node, int depth);
    ~CircuitVisualizer() = default;
private:
    void PrintGateVisit(Gate *node, std::ostream &os, int depth, int indentation = 0);
    void PrintSingleGate(Gate *node, std::ostream &os);
    Circuit *graph_;
    std::ofstream dump_output_{nullptr};
    const char *pass_name_{nullptr};
    std::map<Gate *, uint32_t> node_info_;
};
#endif
}  // namespace kungfu

#endif