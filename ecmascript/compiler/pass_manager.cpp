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

#include "pass_manager.h"
#include "pass.h"

namespace panda::ecmascript::kungfu {
bool PassManager::Compile(std::string fileName)
{
    std::vector<BytecodeTranslationInfo> infoList;
    const panda_file::File *file = nullptr;
    auto res = vm_->CollectInfoOfPandaFile(fileName, entry_, infoList, file);
    if (!res) {
        std::cerr << "Cannot execute panda file '" << fileName << "' with entry '" << entry_ << "'" << std::endl;
        return false;
    }

    for (auto &info : infoList) {
        BytecodeCircuitBuilder builder;
        builder.BytecodeToCircuit(info.pcArray, *info.file, info.method);
        PassData data(builder.GetCircuit());
        PassRunner<PassData> pipeline(&data);
        pipeline.RunPass<SlowPathLoweringPass>(&builder);
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>();
    }
    return true;
}
}
