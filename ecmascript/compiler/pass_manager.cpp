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
#include "ecmascript/compiler/pass_manager.h"

#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/ts_types/ts_loader.h"

namespace panda::ecmascript::kungfu {
bool PassManager::Compile(const std::string &fileName, AOTFileGenerator &generator)
{
    BytecodeTranslationInfo translationInfo;
    [[maybe_unused]] EcmaHandleScope handleScope(vm_->GetJSThread());
    bool res = CollectInfoOfPandaFile(fileName, entry_, &translationInfo);
    if (!res) {
        COMPILER_LOG(ERROR) << "Cannot execute panda file '" << fileName << "'";
        return false;
    }
    auto aotModule = new LLVMModule("aot_" + fileName, triple_);
    auto aotModuleAssembler = new LLVMAssembler(aotModule->GetModule(), LOptions(optLevel_, true));
    CompilationConfig cmpCfg(triple_);
    TSLoader *tsLoader = vm_->GetTSLoader();

    bool enableLog = log_->IsAlwaysEnabled();

    for (size_t i = 0; i < translationInfo.methodPcInfos.size(); i++) {
        const JSMethod *method = translationInfo.methodPcInfos[i].method;
        const std::string methodName(method->GetMethodName());
        if (!log_->IsAlwaysEnabled() && !log_->IsAlwaysDisabled()) {  // neither "all" nor "none"
            enableLog = log_->IncludesMethod(fileName, methodName);
        }

        if (enableLog) {
            COMPILER_LOG(INFO) << "\033[34m" << "aot method [" << fileName << ":"
                               << methodName << "] log:" << "\033[0m";
        }

        BytecodeCircuitBuilder builder(translationInfo, i, tsLoader, enableLog);
        builder.BytecodeToCircuit();
        PassData data(builder.GetCircuit());
        PassRunner<PassData> pipeline(&data, enableLog);
        pipeline.RunPass<TypeInferPass>(&builder, tsLoader);
        pipeline.RunPass<TypeLoweringPass>(&builder, &cmpCfg, tsLoader);
        pipeline.RunPass<SlowPathLoweringPass>(&builder, &cmpCfg);
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>(aotModule, method);
    }

    generator.AddModule(aotModule, aotModuleAssembler, translationInfo.jsPandaFile);
    return true;
}

bool PassManager::CollectInfoOfPandaFile(const std::string &fileName, std::string_view entryPoint,
                                         BytecodeTranslationInfo *translateInfo)
{
    if (translateInfo == nullptr) {
        return false;
    }
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadAotInfoFromPf(fileName.c_str(), entryPoint,
                                                             &(translateInfo->methodPcInfos));
    if (jsPandaFile == nullptr) {
        return false;
    }
    translateInfo->jsPandaFile = jsPandaFile;

    if (jsPandaFile->HasTSTypes()) {
        TSLoader *tsLoader = vm_->GetTSLoader();
        tsLoader->DecodeTSTypes(jsPandaFile);
    } else {
        COMPILER_LOG(INFO) << fileName << " has no type info";
    }

    auto program = PandaFileTranslator::GenerateProgram(vm_, jsPandaFile);
    JSHandle<JSFunction> mainFunc(vm_->GetJSThread(), program->GetMainFunction());
    JSHandle<JSTaggedValue> constPool(vm_->GetJSThread(), mainFunc->GetConstantPool());
    translateInfo->constantPool = constPool;
    return true;
}
} // namespace panda::ecmascript::kungfu
