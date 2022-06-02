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

#ifndef ECMASCRIPT_COMPILER_PASS_MANAGER_H
#define ECMASCRIPT_COMPILER_PASS_MANAGER_H

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript::kungfu {
class PassManager {
public:
    PassManager(EcmaVM* vm, std::string entry, std::string &triple,
        size_t optLevel, AotLog *log) : vm_(vm), entry_(entry),
                                        triple_(triple), optLevel_(optLevel), log_(log) {};
    PassManager() = default;
    bool CollectInfoOfPandaFile(const std::string &filename, std::string_view entryPoint,
                                BytecodeTranslationInfo *translateInfo);
    bool Compile(const std::string &fileName, AOTFileGenerator &generator);
    void GenerateSnapshotFile();

private:
    EcmaVM* vm_ {nullptr};
    std::string entry_ {};
    std::string triple_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    AotLog *log_ {nullptr};
};
}
#endif