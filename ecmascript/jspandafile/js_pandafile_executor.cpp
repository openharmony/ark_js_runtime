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
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/js_module_manager.h"

namespace panda::ecmascript {
Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromFile(JSThread *thread, const CString &filename,
                                                                   std::string_view entryPoint)
{
    const JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename, entryPoint);
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
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
            return JSTaggedValue::Undefined();
        }
        SourceTextModule::Evaluate(thread, moduleRecord);
        return JSTaggedValue::Undefined();
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::ExecuteFromBuffer(
    JSThread *thread, const void *buffer, size_t size, std::string_view entryPoint, const CString &filename)
{
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, filename, entryPoint, buffer, size);
    if (jsPandaFile == nullptr) {
        return Unexpected(false);
    }
    bool isModule = jsPandaFile->IsModule();
    if (isModule) {
        ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();
        moduleManager->AddResolveImportedModule(jsPandaFile, filename);
    }
    return JSPandaFileExecutor::Execute(thread, jsPandaFile);
}

Expected<JSTaggedValue, bool> JSPandaFileExecutor::Execute(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    // For Ark application startup
    EcmaVM *vm = thread->GetEcmaVM();
    Expected<JSTaggedValue, bool> result = vm->InvokeEcmaEntrypoint(jsPandaFile);
    return result;
}
}  // namespace panda::ecmascript
