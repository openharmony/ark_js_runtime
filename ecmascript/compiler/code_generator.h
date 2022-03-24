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
#include "ecmascript/js_method.h"
#include "stub.h"

namespace panda::ecmascript::kungfu {
using ControlFlowGraph = std::vector<std::vector<GateRef>>;
class CodeGeneratorImpl {
public:
    CodeGeneratorImpl() = default;
    virtual ~CodeGeneratorImpl() = default;
    virtual void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
        const CompilationConfig *cfg) = 0;
    virtual void GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
        const JSMethod *method) = 0;
};

class CodeGenerator {
public:
    explicit CodeGenerator(std::unique_ptr<CodeGeneratorImpl> &impl) : impl_(std::move(impl)) {}
    ~CodeGenerator() = default;
    void RunForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index, const CompilationConfig *cfg)
    {
        impl_->GenerateCodeForStub(circuit, graph, index, cfg);
    }
    void Run(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg, const JSMethod *method)
    {
        impl_->GenerateCode(circuit, graph, cfg, method);
    }
private:
    std::unique_ptr<CodeGeneratorImpl> impl_{nullptr};
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_CODE_GENERATOR_H