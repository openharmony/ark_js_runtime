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

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "pass.h"

namespace panda::ecmascript::kungfu {
bool PassManager::Compile(std::string fileName)
{
    BytecodeTranslationInfo translationInfo;
    [[maybe_unused]] EcmaHandleScope handleScope(vm_->GetJSThread());
    bool res = CollectInfoOfPandaFile(fileName, &translationInfo);
    if (!res) {
        std::cerr << "Cannot execute panda file '" << fileName << "' with entry '" << entry_ << "'" << std::endl;
        return false;
    }

    for (size_t i = 0; i < translationInfo.methodPcInfos.size(); i++) {
        BytecodeCircuitBuilder builder(vm_, translationInfo, i);
        builder.BytecodeToCircuit();
        PassData data(builder.GetCircuit());
        PassRunner<PassData> pipeline(&data);
        pipeline.RunPass<SlowPathLoweringPass>(&builder);
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>();
    }
    return true;
}

bool PassManager::CollectInfoOfPandaFile(const std::string &filename, BytecodeTranslationInfo *translateInfo)
{
    if (translateInfo == nullptr) {
        return false;
    }
    const JSPandaFile *jsPandaFile =
        vm_->GetJSPandaFileManager()->LoadAotInfoFromPf(filename, &(translateInfo->methodPcInfos));
    if (jsPandaFile == nullptr) {
        return false;
    }
    translateInfo->jsPandaFile = jsPandaFile;

    TSLoader *tsLoader = vm_->GetTSLoader();
    tsLoader->DecodeTSTypes(*jsPandaFile->GetPandaFile());

    PandaFileTranslator translator(vm_, jsPandaFile);
    auto program = translator.GenerateProgram();
    JSHandle<JSFunction> mainFunc(vm_->GetJSThread(), program->GetMainFunction());
    JSHandle<JSTaggedValue> constPool(vm_->GetJSThread(), mainFunc->GetConstantPool());
    translateInfo->constantPool = constPool;
    return true;
}
}
