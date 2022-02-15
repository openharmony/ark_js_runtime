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

#include "js_module_source_text.h"

#include "ecmascript/global_env.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
static constexpr auto PANDA_MAIN_FUNCTION = "_GLOBAL::func_main_0";

CVector<std::string> SourceTextModule::GetExportedNames(JSThread *thread,
                                                        JSHandle<TaggedArray> &exportStarSet)
{
    CVector<std::string> exportedNames;
    // 1. Let module be this Source Text Module Record.
    JSHandle<SourceTextModule> module(thread, this);
    // 2. If exportStarSet contains module, then
    if (exportStarSet->GetIdx(module.GetTaggedValue()) != TaggedArray::MAX_ARRAY_INDEX) {
        // a. Assert: We've reached the starting point of an import * circularity.
        // b. Return a new empty List.
        return exportedNames;
    }
    // 3. Append module to exportStarSet.
    array_size_t len = exportStarSet->GetLength();
    exportStarSet = TaggedArray::SetCapacity(thread, exportStarSet, len + 1);
    exportStarSet->Set(thread, len, module.GetTaggedValue());

    // 5. For each ExportEntry Record e in module.[[LocalExportEntries]], do
    JSTaggedValue entryValue = module->GetLocalExportEntries();
    if (!entryValue.IsUndefined()) {
        JSHandle<TaggedArray> localExportEntries(thread, entryValue);
        array_size_t localExportEntriesLen = localExportEntries->GetLength();
        for (array_size_t idx = 0; idx < localExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, localExportEntries->Get(idx));
            // a. Assert: module provides the direct binding for this export.
            // b. Append e.[[ExportName]] to exportedNames.
            std::string exportName =
                base::StringHelper::ToStdString(EcmaString::Cast(ee->GetExportName().GetHeapObject()));
            exportedNames.emplace_back(exportName);
        }
    }

    // 6. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    entryValue = module->GetIndirectExportEntries();
    if (!entryValue.IsUndefined()) {
        JSHandle<TaggedArray> indirectExportEntries(thread, entryValue);
        array_size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
        for (array_size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, indirectExportEntries->Get(idx));
            // a. Assert: module imports a specific binding for this export.
            // b. Append e.[[ExportName]] to exportedNames.
            std::string exportName =
                base::StringHelper::ToStdString(EcmaString::Cast(ee->GetExportName().GetHeapObject()));
            exportedNames.emplace_back(exportName);
        }
    }

    // 7. For each ExportEntry Record e in module.[[StarExportEntries]], do
    entryValue = module->GetStarExportEntries();
    if (!entryValue.IsUndefined()) {
        JSHandle<TaggedArray> starExportEntries(thread, entryValue);
        array_size_t starExportEntriesLen = starExportEntries->GetLength();
        for (array_size_t idx = 0; idx < starExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, starExportEntries->Get(idx));
            // a. Let requestedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
            JSHandle<JSTaggedValue> moduleRequest(thread, ee->GetModuleRequest());
            JSHandle<SourceTextModule> requestedModule =
                SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exportedNames);
            // b. Let starNames be ? requestedModule.GetExportedNames(exportStarSet).
            CVector<std::string> starNames = requestedModule->GetExportedNames(thread, exportStarSet);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exportedNames);
            // c. For each element n of starNames, do
            for (std::string &nn : starNames) {
                // i. If SameValue(n, "default") is false, then
                if (nn != "default") {
                    // 1. If n is not an element of exportedNames, then
                    if (std::find(exportedNames.begin(), exportedNames.end(), nn) == exportedNames.end()) {
                        // a. Append n to exportedNames.
                        exportedNames.emplace_back(nn);
                    }
                }
            }
        }
    }
    return exportedNames;
}

JSHandle<SourceTextModule> SourceTextModule::HostResolveImportedModule(JSThread *thread,
                                                                       const JSHandle<SourceTextModule> &module,
                                                                       const JSHandle<JSTaggedValue> &moduleRequest)
{
    std::string moduleFilename = base::StringHelper::ToStdString(EcmaString::Cast(moduleRequest->GetHeapObject()));
    ASSERT(module->GetEcmaModuleFilename().IsHeapObject());
    std::string baseFilename =
        base::StringHelper::ToStdString(EcmaString::Cast(module->GetEcmaModuleFilename().GetHeapObject()));
    int suffixEnd = moduleFilename.find_last_of('.');
    if (suffixEnd == -1) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(SourceTextModule, thread);
    }
    std::string moduleFullname;
    if (moduleFilename[0] == '/') { // absoluteFilePath
        moduleFullname = moduleFilename.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = baseFilename.find_last_of('/');
        if (pos == -1) {
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(SourceTextModule, thread);
        }
        moduleFullname = baseFilename.substr(0, pos + 1) + moduleFilename.substr(0, suffixEnd) + ".abc";
    }
    return thread->GetEcmaVM()->GetEcmaModuleManager()->HostResolveImportedModule(thread, moduleFullname);
}

JSHandle<JSTaggedValue> SourceTextModule::ResolveExport(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &exportName,
                                                        CVector<std::pair<JSHandle<SourceTextModule>,
                                                        JSHandle<JSTaggedValue>>> &resolveSet)
{
    // 1. Let module be this Source Text Module Record.
    JSHandle<SourceTextModule> module(thread, this);
    // 2. For each Record { [[Module]], [[ExportName]] } r in resolveSet, do
    for (auto rr : resolveSet) {
        // a. If module and r.[[Module]] are the same Module Record and
        //    SameValue(exportName, r.[[ExportName]]) is true, then
        if (JSTaggedValue::SameValue(rr.first.GetTaggedValue(), module.GetTaggedValue()) &&
            JSTaggedValue::SameValue(rr.second, exportName)) {
            // i. Assert: This is a circular import request.
            // ii. Return null.
            return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
        }
    }
    // 3. Append the Record { [[Module]]: module, [[ExportName]]: exportName } to resolveSet.
    resolveSet.emplace_back(std::make_pair(module, exportName));
    // 4. For each ExportEntry Record e in module.[[LocalExportEntries]], do
    JSTaggedValue localExportEntriesTv = module->GetLocalExportEntries();
    if (!localExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> localExportEntries(thread, localExportEntriesTv);
        array_size_t localExportEntriesLen = localExportEntries->GetLength();
        for (array_size_t idx = 0; idx < localExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, localExportEntries->Get(idx));
            // a. If SameValue(exportName, e.[[ExportName]]) is true, then
            if (JSTaggedValue::SameValue(ee->GetExportName(), exportName.GetTaggedValue())) {
                // i. Assert: module provides the direct binding for this export.
                // ii. Return ResolvedBinding Record { [[Module]]: module, [[BindingName]]: e.[[LocalName]] }.
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSTaggedValue> localName(thread, ee->GetLocalName());
                return JSHandle<JSTaggedValue>::Cast(factory->NewResolvedBindingRecord(module, localName));
            }
        }
    }
    // 5. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    JSTaggedValue indirectExportEntriesTv = module->GetIndirectExportEntries();
    if (!indirectExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
        array_size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
        for (array_size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, indirectExportEntries->Get(idx));
            //  a. If SameValue(exportName, e.[[ExportName]]) is true, then
            if (JSTaggedValue::SameValue(exportName.GetTaggedValue(), ee->GetExportName())) {
                // i. Assert: module imports a specific binding for this export.
                // ii. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
                JSHandle<JSTaggedValue> moduleRequest(thread, ee->GetModuleRequest());
                JSHandle<SourceTextModule> requestedModule =
                    SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
                // iii. Return importedModule.ResolveExport(e.[[ImportName]], resolveSet).
                JSHandle<JSTaggedValue> importName(thread, ee->GetImportName());
                return requestedModule->ResolveExport(thread, importName, resolveSet);
            }
        }
    }

    auto globalConst = thread->GlobalConstants();
    // 6. If SameValue(exportName, "default") is true, then
    JSHandle<JSTaggedValue> defaultString(thread, globalConst->GetDefaultString());
    if (JSTaggedValue::SameValue(exportName, defaultString)) {
        // a. Assert: A default export was not explicitly defined by this module.
        // b. Return null.
        // c. NOTE: A default export cannot be provided by an export *.
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
    }
    // 7. Let starResolution be null.
    JSMutableHandle<JSTaggedValue> starResolution(thread, JSTaggedValue::Null());
    // 8. For each ExportEntry Record e in module.[[StarExportEntries]], do
    JSTaggedValue starExportEntriesTv = module->GetStarExportEntries();
    if (!starExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> starExportEntries(thread, starExportEntriesTv);
        array_size_t starExportEntriesLen = starExportEntries->GetLength();
        for (array_size_t idx = 0; idx < starExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, starExportEntries->Get(idx));
            // a. Let importedModule be ? HostResolveImportedModule(module, e.[[ModuleRequest]]).
            JSHandle<JSTaggedValue> moduleRequest(thread, ee->GetModuleRequest());
            JSHandle<SourceTextModule> importedModule =
                SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            // b. Let resolution be ? importedModule.ResolveExport(exportName, resolveSet).
            JSHandle<JSTaggedValue> resolution = importedModule->ResolveExport(thread, exportName, resolveSet);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            // c. If resolution is "ambiguous", return "ambiguous".
            if (resolution->IsString()) { // if resolution is string, resolution must be "ambiguous"
                return JSHandle<JSTaggedValue>(thread, globalConst->GetAmbiguousString());
            }
            // d. If resolution is not null, then
            if (!resolution->IsNull()) {
                // i. Assert: resolution is a ResolvedBinding Record.
                ASSERT(resolution->IsResolvedBinding());
                // ii. If starResolution is null, set starResolution to resolution.
                if (starResolution->IsNull()) {
                    starResolution.Update(resolution.GetTaggedValue());
                } else {
                    // 1. Assert: There is more than one * import that includes the requested name.
                    // 2. If resolution.[[Module]] and starResolution.[[Module]] are not the same Module Record or
                    // SameValue(
                    //    resolution.[[BindingName]], starResolution.[[BindingName]]) is false, return "ambiguous".
                    JSHandle<ResolvedBinding> resolutionBd = JSHandle<ResolvedBinding>::Cast(resolution);
                    JSHandle<ResolvedBinding> starResolutionBd = JSHandle<ResolvedBinding>::Cast(starResolution);
                    if ((!JSTaggedValue::SameValue(resolutionBd->GetModule(), starResolutionBd->GetModule())) ||
                        (!JSTaggedValue::SameValue(
                            resolutionBd->GetBindingName(), starResolutionBd->GetBindingName()))) {
                        return JSHandle<JSTaggedValue>(thread, globalConst->GetAmbiguousString());
                    }
                }
            }
        }
    }

    // 9. Return starResolution.
    return starResolution;
}

int32_t SourceTextModule::Instantiate(JSThread *thread)
{
    // 1. Let module be this Source Text Module Record.
    JSHandle<SourceTextModule> module(thread, this);
    // 2. Assert: module.[[Status]] is not "instantiating" or "evaluating".
    ModuleStatus status = module->GetStatus();
    ASSERT(status != ModuleStatus::INSTANTIATING && status != ModuleStatus::EVALUATING);
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    // 4. Let result be InnerModuleInstantiation(module, stack, 0).
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int32_t result = SourceTextModule::InnerModuleInstantiation(thread, moduleRecord, stack, 0);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        // a. For each module m in stack, do
        for (auto mm : stack) {
            // i. Assert: m.[[Status]] is "instantiating".
            [[maybe_unused]] ModuleStatus mmStatus = mm->GetStatus();
            ASSERT(mmStatus == ModuleStatus::INSTANTIATING);
            // ii. Set m.[[Status]] to "uninstantiated".
            mm->SetStatus(ModuleStatus::UNINSTANTIATED);
            // iii. Set m.[[Environment]] to undefined.
            // iv. Set m.[[DFSIndex]] to undefined.
            mm->SetDFSIndex(SourceTextModule::UNDEFINED_INDEX);
            // v. Set m.[[DFSAncestorIndex]] to undefined.
            mm->SetDFSAncestorIndex(SourceTextModule::UNDEFINED_INDEX);
        }
        // b. Assert: module.[[Status]] is "uninstantiated".
        status = module->GetStatus();
        ASSERT(status == ModuleStatus::UNINSTANTIATED);
        // c. return result
        return result;
    }
    // 6. Assert: module.[[Status]] is "instantiated" or "evaluated".
    status = module->GetStatus();
    ASSERT(status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATED);
    // 7. Assert: stack is empty.
    ASSERT(stack.empty());
    // 8. Return undefined.
    return SourceTextModule::UNDEFINED_INDEX;
}

int32_t SourceTextModule::InnerModuleInstantiation(JSThread *thread,
                                                   JSHandle<ModuleRecord> moduleRecord,
                                                   CVector<JSHandle<SourceTextModule>> &stack,
                                                   int32_t index)
{
    // 1. If module is not a Source Text Module Record, then
    if (!moduleRecord.GetTaggedValue().IsSourceTextModule()) {
        //  a. Perform ? module.Instantiate().
        ModuleRecord::Instantiate(thread, JSHandle<JSTaggedValue>::Cast(moduleRecord));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        //  b. Return index.
        return index;
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    // 2. If module.[[Status]] is "instantiating", "instantiated", or "evaluated", then Return index.
    ModuleStatus status = module->GetStatus();
    if (status == ModuleStatus::INSTANTIATING ||
        status == ModuleStatus::INSTANTIATED ||
        status == ModuleStatus::EVALUATED) {
        return index;
    }
    // 3. Assert: module.[[Status]] is "uninstantiated".
    ASSERT(status == ModuleStatus::UNINSTANTIATED);
    // 4. Set module.[[Status]] to "instantiating".
    module->SetStatus(ModuleStatus::INSTANTIATING);
    // 5. Set module.[[DFSIndex]] to index.
    module->SetDFSIndex(index);
    // 6. Set module.[[DFSAncestorIndex]] to index.
    module->SetDFSAncestorIndex(index);
    // 7. Set index to index + 1.
    index++;
    // 8. Append module to stack.
    stack.emplace_back(module);
    // 9. For each String required that is an element of module.[[RequestedModules]], do
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    array_size_t requestedModulesLen = requestedModules->GetLength();
    for (array_size_t idx = 0; idx < requestedModulesLen; idx++) {
        JSHandle<JSTaggedValue> required(thread, requestedModules->Get(idx));
        // a. Let requiredModule be ? HostResolveImportedModule(module, required).
        JSHandle<SourceTextModule> requiredModule =
            SourceTextModule::HostResolveImportedModule(thread, module, required);
        // b. Set index to ? InnerModuleInstantiation(requiredModule, stack, index).
        JSHandle<ModuleRecord> requiredModuleRecord = JSHandle<ModuleRecord>::Cast(requiredModule);
        index = SourceTextModule::InnerModuleInstantiation(thread, requiredModuleRecord, stack, index);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        // c. Assert: requiredModule.[[Status]] is either "instantiating", "instantiated", or "evaluated".
        ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
        ASSERT((requiredModuleStatus == ModuleStatus::INSTANTIATING ||
               requiredModuleStatus == ModuleStatus::INSTANTIATED || requiredModuleStatus == ModuleStatus::EVALUATED));
        // d. Assert: requiredModule.[[Status]] is "instantiating" if and only if requiredModule is in stack.
        // e. If requiredModule.[[Status]] is "instantiating", then
        if (requiredModuleStatus == ModuleStatus::INSTANTIATING) {
            // d. Assert: requiredModule.[[Status]] is "instantiating" if and only if requiredModule is in stack.
            ASSERT(std::find(stack.begin(), stack.end(), requiredModule) != stack.end());
            // i. Assert: requiredModule is a Source Text Module Record.
            // ii. Set module.[[DFSAncestorIndex]] to min(
            //    module.[[DFSAncestorIndex]], requiredModule.[[DFSAncestorIndex]]).
            int32_t dfsAncIdx = std::min(module->GetDFSAncestorIndex(), requiredModule->GetDFSAncestorIndex());
            module->SetDFSAncestorIndex(dfsAncIdx);
        }
    }
    // 10. Perform ? ModuleDeclarationEnvironmentSetup(module).
    SourceTextModule::ModuleDeclarationEnvironmentSetup(thread, module);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
    // 11. Assert: module occurs exactly once in stack.
    // 12. Assert: module.[[DFSAncestorIndex]] is less than or equal to module.[[DFSIndex]].
    int32_t dfsAncIdx = module->GetDFSAncestorIndex();
    int32_t dfsIdx = module->GetDFSIndex();
    ASSERT(dfsAncIdx <= dfsIdx);
    // 13. If module.[[DFSAncestorIndex]] equals module.[[DFSIndex]], then
    if (dfsAncIdx == dfsIdx) {
        // a. Let done be false.
        bool done = false;
        // b. Repeat, while done is false,
        while (!done) {
            // i. Let requiredModule be the last element in stack.
            JSHandle<SourceTextModule> requiredModule = stack.back();
            // ii. Remove the last element of stack.
            stack.pop_back();
            // iii. Set requiredModule.[[Status]] to "instantiated".
            requiredModule->SetStatus(ModuleStatus::INSTANTIATED);
            // iv. If requiredModule and module are the same Module Record, set done to true.
            if (JSTaggedValue::SameValue(module.GetTaggedValue(), requiredModule.GetTaggedValue())) {
                done = true;
            }
        }
    }
    return index;
}

void SourceTextModule::ModuleDeclarationEnvironmentSetup(JSThread *thread,
                                                         JSHandle<SourceTextModule> module)
{
    auto globalConst = thread->GlobalConstants();
    // 1. For each ExportEntry Record e in module.[[IndirectExportEntries]], do
    JSTaggedValue indirectExportEntriesTv = module->GetIndirectExportEntries();
    if (!indirectExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
        array_size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
        for (array_size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, indirectExportEntries->Get(idx));
            //  a. Let resolution be ? module.ResolveExport(e.[[ExportName]], « »).
            JSHandle<JSTaggedValue> exportName(thread, ee->GetExportName());
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
            JSHandle<JSTaggedValue> resolution = module->ResolveExport(thread, exportName, resolveSet);
            //  b. If resolution is null or "ambiguous", throw a SyntaxError exception.
            if (resolution->IsNull() || resolution->IsString()) {
                THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, "");
            }
            //  c. Assert: resolution is a ResolvedBinding Record.
            ASSERT(resolution->IsResolvedBinding());
        }
    }

    // 2. Assert: All named exports from module are resolvable.
    // 3. Let realm be module.[[Realm]].
    // 4. Assert: realm is not undefined.
    // 5. Let env be NewModuleEnvironment(realm.[[GlobalEnv]]).
    JSHandle<LinkedHashMap> map = LinkedHashMap::Create(thread);
    // 6. Set module.[[Environment]] to env.
    module->SetEnvironment(thread, map);
    // 7. Let envRec be env's EnvironmentRecord.
    JSMutableHandle<JSTaggedValue> envRec(thread, module->GetEnvironment());
    ASSERT(!envRec->IsUndefined());
    // 8. For each ImportEntry Record in in module.[[ImportEntries]], do
    JSTaggedValue ImportEntriesTv = module->GetImportEntries();
    if (!ImportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> importEntries(thread, ImportEntriesTv);
        array_size_t importEntriesLen = importEntries->GetLength();
        for (array_size_t idx = 0; idx < importEntriesLen; idx++) {
            JSHandle<ImportEntry> in(thread, importEntries->Get(idx));
            // a. Let importedModule be ! HostResolveImportedModule(module, in.[[ModuleRequest]]).
            JSHandle<JSTaggedValue> moduleRequest(thread, in->GetModuleRequest());
            JSHandle<SourceTextModule> importedModule =
                SourceTextModule::HostResolveImportedModule(thread, module, moduleRequest);
            // c. If in.[[ImportName]] is "*", then
            JSHandle<JSTaggedValue> importName(thread, in->GetImportName());
            JSHandle<JSTaggedValue> starString(thread, globalConst->GetStarString());
            if (JSTaggedValue::SameValue(importName, starString)) {
                // i. Let namespace be ? GetModuleNamespace(importedModule).
                JSHandle<JSTaggedValue> moduleNamespace = SourceTextModule::GetModuleNamespace(thread, importedModule);
                // ii. Perform ! envRec.CreateImmutableBinding(in.[[LocalName]], true).
                // iii. Call envRec.InitializeBinding(in.[[LocalName]], namespace).
                JSHandle<LinkedHashMap> mapHandle = JSHandle<LinkedHashMap>::Cast(envRec);
                JSHandle<JSTaggedValue> localName(thread, in->GetLocalName());
                JSHandle<LinkedHashMap> newMap = LinkedHashMap::Set(thread, mapHandle, localName, moduleNamespace);
                envRec.Update(newMap);
            } else {
                // i. Let resolution be ? importedModule.ResolveExport(in.[[ImportName]], « »).
                CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
                JSHandle<JSTaggedValue> resolution = importedModule->ResolveExport(thread, importName, resolveSet);
                // ii. If resolution is null or "ambiguous", throw a SyntaxError exception.
                if (resolution->IsNull() || resolution->IsString()) {
                    THROW_ERROR(thread, ErrorType::SYNTAX_ERROR, "");
                }
                // iii. Call envRec.CreateImportBinding(
                //    in.[[LocalName]], resolution.[[Module]], resolution.[[BindingName]]).
                JSHandle<LinkedHashMap> mapHandle = JSHandle<LinkedHashMap>::Cast(envRec);
                JSHandle<JSTaggedValue> localName(thread, in->GetLocalName());
                JSHandle<LinkedHashMap> newMap = LinkedHashMap::Set(thread, mapHandle, localName, resolution);
                envRec.Update(newMap);
            }
        }
    }
    module->SetEnvironment(thread, envRec);
}

JSHandle<JSTaggedValue> SourceTextModule::GetModuleNamespace(JSThread *thread, JSHandle<SourceTextModule> module)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Assert: module is an instance of a concrete subclass of Module Record.
    // 2. Assert: module.[[Status]] is not "uninstantiated".
    ModuleStatus status = module->GetStatus();
    ASSERT(status != ModuleStatus::UNINSTANTIATED);
    // 3. Assert: If module.[[Status]] is "evaluated", module.[[EvaluationError]] is undefined.
    if (status == ModuleStatus::EVALUATED) {
        ASSERT(module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX);
    }
    // 4. Let namespace be module.[[Namespace]].
    JSMutableHandle<JSTaggedValue> moduleNamespace(thread, module->GetNamespace());
    // If namespace is undefined, then
    if (moduleNamespace->IsUndefined()) {
        // a. Let exportedNames be ? module.GetExportedNames(« »).
        JSHandle<TaggedArray> exportStarSet = factory->EmptyArray();
        CVector<std::string> exportedNames = module->GetExportedNames(thread, exportStarSet);
        // b. Let unambiguousNames be a new empty List.
        JSHandle<TaggedArray> unambiguousNames = factory->NewTaggedArray(exportedNames.size());
        // c. For each name that is an element of exportedNames, do
        array_size_t idx = 0;
        for (std::string &name : exportedNames) {
            // i. Let resolution be ? module.ResolveExport(name, « »).
            CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
            JSHandle<JSTaggedValue> nameHandle = JSHandle<JSTaggedValue>::Cast(factory->NewFromStdString(name));
            JSHandle<JSTaggedValue> resolution = module->ResolveExport(thread, nameHandle, resolveSet);
            // ii. If resolution is a ResolvedBinding Record, append name to unambiguousNames.
            if (resolution->IsResolvedBinding()) {
                unambiguousNames->Set(thread, idx, nameHandle);
                idx++;
            }
        }
        JSHandle<TaggedArray> fixUnambiguousNames = TaggedArray::SetCapacity(thread, unambiguousNames, idx);
        JSHandle<JSTaggedValue> moduleTagged = JSHandle<JSTaggedValue>::Cast(module);
        JSHandle<ModuleNamespace> np =
            ModuleNamespace::ModuleNamespaceCreate(thread, moduleTagged, fixUnambiguousNames);
        moduleNamespace.Update(np.GetTaggedValue());
    }
    return moduleNamespace;
}

int32_t SourceTextModule::Evaluate(JSThread *thread)
{
    // 1. Let module be this Source Text Module Record.
    JSHandle<SourceTextModule> module(thread, this);
    // 2. Assert: module.[[Status]] is "instantiated" or "evaluated".
    ModuleStatus status = module->GetStatus();
    ASSERT((status == ModuleStatus::INSTANTIATED || status == ModuleStatus::EVALUATED));
    // 3. Let stack be a new empty List.
    CVector<JSHandle<SourceTextModule>> stack;
    // 4. Let result be InnerModuleEvaluation(module, stack, 0)
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    int32_t result = SourceTextModule::InnerModuleEvaluation(thread, moduleRecord, stack, 0);
    // 5. If result is an abrupt completion, then
    if (thread->HasPendingException()) {
        // a. For each module m in stack, do
        for (auto mm : stack) {
            // i. Assert: m.[[Status]] is "evaluating".
            [[maybe_unused]] ModuleStatus mmStatus = mm->GetStatus();
            ASSERT(mmStatus == ModuleStatus::EVALUATING);
            // ii. Set m.[[Status]] to "evaluated".
            mm->SetStatus(ModuleStatus::EVALUATED);
            // iii. Set m.[[EvaluationError]] to result.
            mm->SetEvaluationError(result);
        }
        // b. Assert: module.[[Status]] is "evaluated" and module.[[EvaluationError]] is result.
        status = module->GetStatus();
        ASSERT(status == ModuleStatus::EVALUATED && module->GetEvaluationError() == result);
        // c. return result
        return result;
    }
    // 6. Assert: module.[[Status]] is "evaluated" and module.[[EvaluationError]] is undefined.
    status = module->GetStatus();
    ASSERT(status == ModuleStatus::EVALUATED && module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX);
    // 7. Assert: stack is empty.
    ASSERT(stack.empty());
    // 8. Return undefined.
    return SourceTextModule::UNDEFINED_INDEX;
}

int32_t SourceTextModule::InnerModuleEvaluation(JSThread *thread,
                                                JSHandle<ModuleRecord> moduleRecord,
                                                CVector<JSHandle<SourceTextModule>> &stack,
                                                int32_t index)
{
    // 1. If module is not a Source Text Module Record, then
    if (!moduleRecord.GetTaggedValue().IsSourceTextModule()) {
        //  a. Perform ? module.Instantiate().
        ModuleRecord::Instantiate(thread, JSHandle<JSTaggedValue>::Cast(moduleRecord));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        //  b. Return index.
        return index;
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(moduleRecord);
    // 2.If module.[[Status]] is "evaluated", then
    ModuleStatus status = module->GetStatus();
    if (status == ModuleStatus::EVALUATED) {
        // a. If module.[[EvaluationError]] is undefined, return index.
        if (module->GetEvaluationError() == SourceTextModule::UNDEFINED_INDEX) {
            return index;
        }
        // Otherwise return module.[[EvaluationError]].
        return module->GetEvaluationError();
    }
    // 3. If module.[[Status]] is "evaluating", return index.
    if (status == ModuleStatus::EVALUATING) {
        return index;
    }
    // 4. Assert: module.[[Status]] is "instantiated".
    ASSERT(status == ModuleStatus::INSTANTIATED);
    // 5. Set module.[[Status]] to "evaluating".
    module->SetStatus(ModuleStatus::EVALUATING);
    // 6. Set module.[[DFSIndex]] to index.
    module->SetDFSIndex(index);
    // 7. Set module.[[DFSAncestorIndex]] to index.
    module->SetDFSAncestorIndex(index);
    // 8. Set index to index + 1.
    index++;
    // 9. Append module to stack.
    stack.emplace_back(module);
    // 10. For each String required that is an element of module.[[RequestedModules]], do
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules());
    array_size_t requestedModulesLen = requestedModules->GetLength();
    for (array_size_t idx = 0; idx < requestedModulesLen; idx++) {
        JSHandle<JSTaggedValue> required(thread, requestedModules->Get(idx));
        // a. Let requiredModule be ! HostResolveImportedModule(module, required).
        JSHandle<SourceTextModule> requiredModule =
            SourceTextModule::HostResolveImportedModule(thread, module, required);
        // c. Set index to ? InnerModuleEvaluation(requiredModule, stack, index).
        JSHandle<ModuleRecord> requiredModuleRecord = JSHandle<ModuleRecord>::Cast(requiredModule);
        index = SourceTextModule::InnerModuleEvaluation(thread, requiredModuleRecord, stack, index);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
        // d. Assert: requiredModule.[[Status]] is either "evaluating" or "evaluated".
        ModuleStatus requiredModuleStatus = requiredModule->GetStatus();
        ASSERT((requiredModuleStatus == ModuleStatus::EVALUATING || requiredModuleStatus == ModuleStatus::EVALUATED));
        // e. Assert: requiredModule.[[Status]] is "evaluating" if and only if requiredModule is in stack.
        if (requiredModuleStatus == ModuleStatus::EVALUATING) {
            ASSERT(std::find(stack.begin(), stack.end(), requiredModule) != stack.end());
        }
        // f. If requiredModule.[[Status]] is "evaluating", then
        if (requiredModuleStatus == ModuleStatus::EVALUATING) {
            // i. Assert: requiredModule is a Source Text Module Record.
            // ii. Set module.[[DFSAncestorIndex]] to min(
            //    module.[[DFSAncestorIndex]], requiredModule.[[DFSAncestorIndex]]).
            int32_t dfsAncIdx = std::min(module->GetDFSAncestorIndex(), requiredModule->GetDFSAncestorIndex());
            module->SetDFSAncestorIndex(dfsAncIdx);
        }
    }
    // 11. Perform ? ModuleExecution(module).
    SourceTextModule::ModuleExecution(thread, module);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, index);
    // 12. Assert: module occurs exactly once in stack.
    // 13. Assert: module.[[DFSAncestorIndex]] is less than or equal to module.[[DFSIndex]].
    int32_t dfsAncIdx = module->GetDFSAncestorIndex();
    int32_t dfsIdx = module->GetDFSIndex();
    ASSERT(dfsAncIdx <= dfsIdx);
    // 14. If module.[[DFSAncestorIndex]] equals module.[[DFSIndex]], then
    if (dfsAncIdx == dfsIdx) {
        // a. Let done be false.
        bool done = false;
        // b. Repeat, while done is false,
        while (!done) {
            // i. Let requiredModule be the last element in stack.
            JSHandle<SourceTextModule> requiredModule = stack.back();
            // ii. Remove the last element of stack.
            stack.pop_back();
            // iii. Set requiredModule.[[Status]] to "evaluated".
            requiredModule->SetStatus(ModuleStatus::EVALUATED);
            // iv. If requiredModule and module are the same Module Record, set done to true.
            if (JSTaggedValue::SameValue(module.GetTaggedValue(), requiredModule.GetTaggedValue())) {
                done = true;
            }
        }
    }
    return index;
}

void SourceTextModule::ModuleExecution(JSThread *thread, JSHandle<SourceTextModule> module)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSTaggedValue moduleFileName = module->GetEcmaModuleFilename();
    ASSERT(moduleFileName.IsString());
    std::string moduleFilenameStr = base::StringHelper::ToStdString(EcmaString::Cast(moduleFileName.GetHeapObject()));

    std::vector<std::string> argv;
    vm->ModuleExecution(moduleFilenameStr, PANDA_MAIN_FUNCTION, module.GetTaggedValue(), argv);
}

void SourceTextModule::AddImportEntry(JSThread *thread, JSHandle<ImportEntry> importEntry)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module(thread, this);
    JSTaggedValue importEntries = module->GetImportEntries();
    if (importEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(1);
        array->Set(thread, 0, importEntry.GetTaggedValue());
        module->SetImportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, importEntries);
        array_size_t len = entries->GetLength();
        JSHandle<TaggedArray> newEntries = TaggedArray::SetCapacity(thread, entries, len + 1);
        newEntries->Set(thread, len, importEntry.GetTaggedValue());
        module->SetImportEntries(thread, newEntries);
    }
}

void SourceTextModule::AddLocalExportEntry(JSThread *thread, JSHandle<ExportEntry> exportEntry)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module(thread, this);
    JSTaggedValue localExportEntries = module->GetLocalExportEntries();
    if (localExportEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(1);
        array->Set(thread, 0, exportEntry.GetTaggedValue());
        module->SetLocalExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, localExportEntries);
        array_size_t len = entries->GetLength();
        JSHandle<TaggedArray> newEntries = TaggedArray::SetCapacity(thread, entries, len + 1);
        newEntries->Set(thread, len, exportEntry.GetTaggedValue());
        module->SetLocalExportEntries(thread, newEntries);
    }
}

void SourceTextModule::AddIndirectExportEntry(JSThread *thread, JSHandle<ExportEntry> exportEntry)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module(thread, this);
    JSTaggedValue indirectExportEntries = module->GetIndirectExportEntries();
    if (indirectExportEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(1);
        array->Set(thread, 0, exportEntry.GetTaggedValue());
        module->SetIndirectExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, indirectExportEntries);
        array_size_t len = entries->GetLength();
        JSHandle<TaggedArray> newEntries = TaggedArray::SetCapacity(thread, entries, len + 1);
        newEntries->Set(thread, len, exportEntry.GetTaggedValue());
        module->SetIndirectExportEntries(thread, newEntries);
    }
}

void SourceTextModule::AddStarExportEntry(JSThread *thread, JSHandle<ExportEntry> exportEntry)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module(thread, this);
    JSTaggedValue starExportEntries = module->GetStarExportEntries();
    if (starExportEntries.IsUndefined()) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(1);
        array->Set(thread, 0, exportEntry.GetTaggedValue());
        module->SetStarExportEntries(thread, array);
    } else {
        JSHandle<TaggedArray> entries(thread, starExportEntries);
        array_size_t len = entries->GetLength();
        JSHandle<TaggedArray> newEntries = TaggedArray::SetCapacity(thread, entries, len + 1);
        newEntries->Set(thread, len, exportEntry.GetTaggedValue());
        module->SetStarExportEntries(thread, newEntries);
    }
}

JSTaggedValue SourceTextModule::GetModuleValue(JSThread *thread, JSTaggedValue key, bool isThrow)
{
    JSTaggedValue dictionary = GetNameDictionary();
    if (dictionary.IsUndefined()) {
        if (isThrow) {
            THROW_REFERENCE_ERROR_AND_RETURN(thread, "module environment is undefined", JSTaggedValue::Exception());
        }
        return JSTaggedValue::Hole();
    }

    NameDictionary *dict = NameDictionary::Cast(dictionary.GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return dict->GetValue(entry);
    }

    JSTaggedValue importEntriesTv = GetImportEntries();
    if (!importEntriesTv.IsUndefined()) {
        TaggedArray *importEntries = TaggedArray::Cast(importEntriesTv.GetTaggedObject());
        array_size_t importEntriesLen = importEntries->GetLength();
        for (array_size_t idx = 0; idx < importEntriesLen; idx++) {
            ImportEntry *ee = ImportEntry::Cast(importEntries->Get(idx).GetTaggedObject());
            if (JSTaggedValue::SameValue(ee->GetLocalName(), key)) {
                JSTaggedValue importName = ee->GetImportName();
                entry = dict->FindEntry(importName);
                if (entry != -1) {
                    return dict->GetValue(entry);
                }
            }
        }
    }

    JSTaggedValue exportEntriesTv = GetLocalExportEntries();
    if (!exportEntriesTv.IsUndefined()) {
        TaggedArray *exportEntries = TaggedArray::Cast(exportEntriesTv.GetTaggedObject());
        array_size_t exportEntriesLen = exportEntries->GetLength();
        for (array_size_t idx = 0; idx < exportEntriesLen; idx++) {
            ExportEntry *ee = ExportEntry::Cast(exportEntries->Get(idx).GetTaggedObject());
            if (JSTaggedValue::SameValue(ee->GetLocalName(), key)) {
                JSTaggedValue exportName = ee->GetExportName();
                entry = dict->FindEntry(exportName);
                if (entry != -1) {
                    return dict->GetValue(entry);
                }
            }
        }
    }

    return JSTaggedValue::Hole();
}

void SourceTextModule::StoreModuleValue(JSThread *thread, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
{
    JSHandle<SourceTextModule> module(thread, this);
    JSMutableHandle<JSTaggedValue> data(thread, module->GetNameDictionary());
    if (data->IsUndefined()) {
        data.Update(NameDictionary::Create(thread, DEAULT_DICTIONART_CAPACITY));
    }

    JSMutableHandle<NameDictionary> dataDict(data);
    JSTaggedValue localExportEntriesTv = module->GetLocalExportEntries();
    if (!localExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> localExportEntries(thread, localExportEntriesTv);
        array_size_t localExportEntriesLen = localExportEntries->GetLength();
        for (array_size_t idx = 0; idx < localExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, localExportEntries->Get(idx));
            if (JSTaggedValue::SameValue(ee->GetLocalName(), key.GetTaggedValue())) {
                JSHandle<JSTaggedValue> exportName(thread, ee->GetExportName());
                dataDict.Update(NameDictionary::Put(thread, dataDict, exportName, value,
                                                    PropertyAttributes::Default()));
            }
        }
    }
    JSTaggedValue indirectExportEntriesTv = module->GetIndirectExportEntries();
    if (!indirectExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
        array_size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
        for (array_size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, indirectExportEntries->Get(idx));
            if (JSTaggedValue::SameValue(ee->GetImportName(), key.GetTaggedValue())) {
                JSHandle<JSTaggedValue> exportName(thread, ee->GetExportName());
                dataDict.Update(NameDictionary::Put(thread, dataDict, exportName, value,
                                                    PropertyAttributes::Default()));
            }
        }
    }
    module->SetNameDictionary(thread, dataDict);
}

JSTaggedValue SourceTextModule::FindExportName(JSThread *thread, JSHandle<JSTaggedValue> localName)
{
    JSHandle<SourceTextModule> module(thread, this);
    JSTaggedValue localExportEntriesTv = module->GetLocalExportEntries();
    if (!localExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> localExportEntries(thread, localExportEntriesTv);
        array_size_t localExportEntriesLen = localExportEntries->GetLength();
        for (array_size_t idx = 0; idx < localExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, localExportEntries->Get(idx));
            if (JSTaggedValue::SameValue(ee->GetLocalName(), localName.GetTaggedValue())) {
                return ee->GetExportName();
            }
        }
    }
    JSTaggedValue indirectExportEntriesTv = module->GetIndirectExportEntries();
    if (!indirectExportEntriesTv.IsUndefined()) {
        JSHandle<TaggedArray> indirectExportEntries(thread, indirectExportEntriesTv);
        array_size_t indirectExportEntriesLen = indirectExportEntries->GetLength();
        for (array_size_t idx = 0; idx < indirectExportEntriesLen; idx++) {
            JSHandle<ExportEntry> ee(thread, indirectExportEntries->Get(idx));
            if (JSTaggedValue::SameValue(ee->GetLocalName(), localName.GetTaggedValue())) {
                return ee->GetExportName();
            }
        }
    }
    return JSTaggedValue::Undefined();
}
} // namespace panda::ecmascript