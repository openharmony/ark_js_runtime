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

#include "ecmascript/ecma_vm.h"
#include "bytecode_circuit_builder.h"

namespace panda::ecmascript::kungfu {
class PassManager {
public:
    PassManager(EcmaVM* vm, std::string entry) : vm_(vm), entry_(entry) {}
    PassManager() = default;
    bool Compile(std::string fileName);
    bool CollectInfoOfPandaFile(const std::string &filename, BytecodeTranslationInfo *translateInfo);

private:
    EcmaVM* vm_;
    std::string entry_;
};
}
#endif