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

#include "aot_file_manager.h"
#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "pass.h"

namespace panda::ecmascript::kungfu {
bool PassManager::Compile(const std::string &fileName, const std::string &triple,
                          const std::string &outputFileName)
{
    BytecodeTranslationInfo translationInfo;
    [[maybe_unused]] EcmaHandleScope handleScope(vm_->GetJSThread());
    bool res = CollectInfoOfPandaFile(fileName, &translationInfo);
    if (!res) {
        std::cerr << "Cannot execute panda file '" << fileName << "'" << std::endl;
        return false;
    }
    LLVMModule aotModule("aot_file", triple);

    for (size_t i = 0; i < translationInfo.methodPcInfos.size(); i++) {
        BytecodeCircuitBuilder builder(vm_, translationInfo, i);
        builder.BytecodeToCircuit();
        PassData data(builder.GetCircuit());
        PassRunner<PassData> pipeline(&data);
        pipeline.RunPass<SlowPathLoweringPass>(&builder);
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>(&aotModule, translationInfo.methodPcInfos[i].method);
    }

    AotFileManager manager(&aotModule);
    manager.SaveAOTFile(outputFileName);
    return true;
}

bool PassManager::CollectInfoOfPandaFile(const std::string &fileName, BytecodeTranslationInfo *translateInfo)
{
    if (translateInfo == nullptr) {
        return false;
    }
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadAotInfoFromPf(fileName.c_str(), &(translateInfo->methodPcInfos));
    if (jsPandaFile == nullptr) {
        return false;
    }
    translateInfo->jsPandaFile = jsPandaFile;

    TSLoader *tsLoader = vm_->GetTSLoader();
    tsLoader->DecodeTSTypes(jsPandaFile);

    auto program = PandaFileTranslator::GenerateProgram(vm_, jsPandaFile);
    JSHandle<JSFunction> mainFunc(vm_->GetJSThread(), program->GetMainFunction());
    JSHandle<JSTaggedValue> constPool(vm_->GetJSThread(), mainFunc->GetConstantPool());
    translateInfo->constantPool = constPool;
    return true;
}
} // namespace panda::ecmascript::kungfu
