/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object-inl.h"
#include "ecmascript/module/js_module_manager.h"

namespace panda::ecmascript {
bool JSPandaFileExecutor::ExecuteFromFile(JSThread *thread, const CString &filename, std::string_view entryPoint,
                                          const std::vector<std::string> &args)
{
    const JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(filename);
    if (jsPandaFile == nullptr) {
        return false;
    }

    bool isModule = jsPandaFile->IsModule();
    if (isModule) {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        EcmaVM *vm = thread->GetEcmaVM();
        ModuleManager *moduleManager = vm->GetModuleManager();
        JSHandle<SourceTextModule> moduleRecord = moduleManager->HostResolveImportedModule(filename);
        SourceTextModule::Instantiate(thread, moduleRecord);
        if (thread->HasPendingException()) {
            auto exception = thread->GetException();
            vm->HandleUncaughtException(exception.GetTaggedObject());
            return false;
        }
        SourceTextModule::Evaluate(thread, moduleRecord);
        return true;
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entryPoint, args);
}

bool JSPandaFileExecutor::ExecuteFromBuffer(JSThread *thread, const void *buffer, size_t size,
                                            std::string_view entryPoint, const std::vector<std::string> &args,
                                            const CString &filename)
{
    const JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(filename, buffer, size);
    if (jsPandaFile == nullptr) {
        return false;
    }
    bool isModule = jsPandaFile->IsModule();
    if (isModule) {
        ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();
        moduleManager->AddResolveImportedModule(jsPandaFile, filename);
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile, entryPoint, args);
}

bool JSPandaFileExecutor::Execute(JSThread *thread, const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                                  const std::vector<std::string> &args)
{
    // Get ClassName and MethodName
    size_t pos = entryPoint.find_last_of("::");
    if (pos == std::string_view::npos) {
        LOG_ECMA(ERROR) << "EntryPoint:" << entryPoint << " is illegal";
        return false;
    }
    CString methodName(entryPoint.substr(pos + 1));
    // For Ark application startup
    EcmaVM *vm = thread->GetEcmaVM();
    vm->InvokeEcmaEntrypoint(jsPandaFile, methodName, args);
    return true;
}
}  // namespace panda::ecmascript
