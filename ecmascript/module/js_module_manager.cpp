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

#include "ecmascript/module/js_module_manager.h"

#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_array.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
ModuleManager::ModuleManager(EcmaVM *vm) : vm_(vm)
{
    resolvedModules_ = NameDictionary::Create(vm_->GetJSThread(), DEAULT_DICTIONART_CAPACITY).GetTaggedValue();
}

JSTaggedValue ModuleManager::GetCurrentModule()
{
    FrameHandler frameHandler(vm_->GetJSThread());
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    return JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule();
}

JSTaggedValue ModuleManager::GetModuleValueInner(JSTaggedValue key)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_ECMA(FATAL) << "GetModuleValueInner currentModule failed";
    }
    return SourceTextModule::Cast(currentModule.GetHeapObject())->GetModuleValue(vm_->GetJSThread(), key, false);
}

JSTaggedValue ModuleManager::GetModuleValueOutter(JSTaggedValue key)
{
    JSThread *thread = vm_->GetJSThread();
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_ECMA(FATAL) << "GetModuleValueOutter currentModule failed";
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetHeapObject())->GetEnvironment();
    ASSERT(!moduleEnvironment.IsUndefined());
    JSTaggedValue resolvedBinding = LinkedHashMap::Cast(moduleEnvironment.GetTaggedObject())->Get(key);
    if (resolvedBinding.IsUndefined()) {
        return thread->GlobalConstants()->GetUndefined();
    }
    ASSERT(resolvedBinding.IsResolvedBinding());
    ResolvedBinding *binding = ResolvedBinding::Cast(resolvedBinding.GetTaggedObject());
    JSTaggedValue resolvedModule = binding->GetModule();
    ASSERT(resolvedModule.IsSourceTextModule());
    return SourceTextModule::Cast(resolvedModule.GetHeapObject())->GetModuleValue(thread, binding->GetBindingName(),
                                                                                  false);
}

void ModuleManager::StoreModuleValue(JSTaggedValue key, JSTaggedValue value)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<SourceTextModule> currentModule(thread, GetCurrentModule());
    if (currentModule.GetTaggedValue().IsUndefined()) {
        LOG_ECMA(FATAL) << "StoreModuleValue currentModule failed";
    }
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    currentModule->StoreModuleValue(thread, keyHandle, valueHandle);
}

JSHandle<SourceTextModule> ModuleManager::HostGetImportedModule(const CString &referencingModule)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> referencingHandle =
        JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(referencingModule));
    int entry =
        NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencingHandle.GetTaggedValue());
    LOG_IF(entry == -1, FATAL, ECMASCRIPT) << "cannot get module: " << referencingModule;

    return JSHandle<SourceTextModule>(vm_->GetJSThread(),
                                      NameDictionary::Cast(resolvedModules_.GetTaggedObject())->GetValue(entry));
}

JSHandle<SourceTextModule> ModuleManager::HostResolveImportedModule(const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> referencingHandle =
        JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(referencingModule));
    int entry =
        NameDictionary::Cast(resolvedModules_.GetTaggedObject())->FindEntry(referencingHandle.GetTaggedValue());
    if (entry != -1) {
        return JSHandle<SourceTextModule>(
            thread, NameDictionary::Cast(resolvedModules_.GetTaggedObject())->GetValue(entry));
    }

    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(referencingModule, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open jsPandaFile " << referencingModule << " error";
        UNREACHABLE();
    }
    JSHandle<JSTaggedValue> moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, referencingModule);
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
    return JSHandle<SourceTextModule>::Cast(moduleRecord);
}

void ModuleManager::AddResolveImportedModule(const JSPandaFile *jsPandaFile, const CString &referencingModule)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> moduleRecord = ModuleDataExtractor::ParseModule(thread, jsPandaFile, referencingModule);
    JSHandle<JSTaggedValue> referencingHandle =
        JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(referencingModule));
    JSHandle<NameDictionary> dict(thread, resolvedModules_);
    resolvedModules_ =
        NameDictionary::Put(thread, dict, referencingHandle, moduleRecord, PropertyAttributes::Default())
        .GetTaggedValue();
}

JSTaggedValue ModuleManager::GetModuleNamespace(JSTaggedValue localName)
{
    JSTaggedValue currentModule = GetCurrentModule();
    if (currentModule.IsUndefined()) {
        LOG_ECMA(FATAL) << "GetModuleNamespace currentModule failed";
    }
    JSTaggedValue moduleEnvironment = SourceTextModule::Cast(currentModule.GetHeapObject())->GetEnvironment();
    ASSERT(!moduleEnvironment.IsUndefined());
    JSTaggedValue moduleNamespace = LinkedHashMap::Cast(moduleEnvironment.GetTaggedObject())->Get(localName);
    if (moduleNamespace.IsUndefined()) {
        return vm_->GetJSThread()->GlobalConstants()->GetUndefined();
    }
    ASSERT(moduleNamespace.IsModuleNamespace());
    return moduleNamespace;
}

void ModuleManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&resolvedModules_)));
}
} // namespace panda::ecmascript
