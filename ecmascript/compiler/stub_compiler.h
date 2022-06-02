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

#ifndef ECMASCRIPT_COMPILER_STUB_COMPILER_H
#define ECMASCRIPT_COMPILER_STUB_COMPILER_H

#include <string>

#include "compiler_log.h"
#include "llvm_ir_builder.h"

namespace panda::ecmascript::kungfu {
class Stub;
class StubCompiler {
public:
    explicit StubCompiler(const CompilerLog *log) : log_(log) {}

    ~StubCompiler() = default;

    bool BuildStubModuleAndSave(const std::string &triple, const std::string &stubFile, size_t optLevel);

    const CompilerLog *GetLog() const
    {
        return log_;
    }
private:
    void RunPipeline(LLVMModule *module);
    const CompilerLog *log_ {nullptr};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_COMPILER_H
