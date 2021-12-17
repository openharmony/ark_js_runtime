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

#ifndef ECMASCRIPT_COMPILER_CODE_GENERATOR_H
#define ECMASCRIPT_COMPILER_CODE_GENERATOR_H

#include "circuit.h"

namespace kungfu {
using ControlFlowGraph = std::vector<std::vector<GateRef>>;
class CodeGeneratorImpl {
public:
    CodeGeneratorImpl() = default;
    virtual ~CodeGeneratorImpl() = default;
    virtual void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, int index) = 0;
};

class CodeGenerator {
public:
    explicit CodeGenerator(std::unique_ptr<CodeGeneratorImpl> &impl, const char* triple) : impl_(std::move(impl)) {}
    ~CodeGenerator() = default;
    void Run(Circuit *circuit, const ControlFlowGraph &graph, int index)
    {
        impl_->GenerateCodeForStub(circuit, graph, index);
    }

private:
    std::unique_ptr<CodeGeneratorImpl> impl_{nullptr};
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_CODE_GENERATOR_H