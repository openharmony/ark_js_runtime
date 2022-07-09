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
#include "js_cjs_module.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
void CjsModule::InitializeModule(JSThread *thread, JSHandle<CjsModule> &module,
                                 JSHandle<JSTaggedValue> &filename, JSHandle<JSTaggedValue> &dirname)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> dirKey(factory->NewFromASCII("path"));
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), dirKey.GetTaggedValue(),
                                 dirname.GetTaggedValue());
    JSHandle<JSTaggedValue> filenameKey(factory->NewFromASCII("filename"));
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), filenameKey.GetTaggedValue(),
                                 filename.GetTaggedValue());
    module->SetFilename(thread, filename.GetTaggedValue());
    module->SetPath(thread, dirname.GetTaggedValue());
    return;
}

JSHandle<JSTaggedValue> CjsModule::SearchFromModuleCache(JSThread *thread, JSHandle<JSTaggedValue> &filename)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> moduleObj(env->GetCjsModuleFunction());
    JSHandle<JSTaggedValue> cacheName = globalConst->GetHandledCjsCacheString();
    JSTaggedValue modCache = SlowRuntimeStub::LdObjByName(thread, moduleObj.GetTaggedValue(),
                                                          cacheName.GetTaggedValue(),
                                                          false,
                                                          JSTaggedValue::Undefined());
    JSHandle<CjsModuleCache> moduleCache = JSHandle<CjsModuleCache>(thread, modCache);
    if (moduleCache->ContainsModule(filename.GetTaggedValue())) {
        JSHandle<CjsModule> cachedModule = JSHandle<CjsModule>(thread,
                                                                   moduleCache->GetModule(filename.GetTaggedValue()));
        JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
        JSTaggedValue cachedExports = SlowRuntimeStub::LdObjByName(thread, cachedModule.GetTaggedValue(),
                                                                   exportsName.GetTaggedValue(),
                                                                   false,
                                                                   JSTaggedValue::Undefined());
        return JSHandle<JSTaggedValue>(thread, cachedExports);
    }

    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Hole());
}

void CjsModule::PutIntoCache(JSThread *thread, JSHandle<CjsModule> &module, JSHandle<JSTaggedValue> &filename)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> moduleObj(env->GetCjsModuleFunction());
    JSHandle<JSTaggedValue> cacheName = globalConst->GetHandledCjsCacheString();
    JSTaggedValue modCache = SlowRuntimeStub::LdObjByName(thread, moduleObj.GetTaggedValue(),
                                                          cacheName.GetTaggedValue(),
                                                          false,
                                                          JSTaggedValue::Undefined());
    JSHandle<CjsModuleCache> moduleCache = JSHandle<CjsModuleCache>(thread, modCache);
    JSHandle<JSTaggedValue> moduleHandle = JSHandle<JSTaggedValue>::Cast(module);
    JSHandle<CjsModuleCache> newCache = CjsModuleCache::PutIfAbsent(thread, moduleCache, filename,
                                                                    moduleHandle);
    SlowRuntimeStub::StObjByName(thread, moduleObj.GetTaggedValue(), cacheName.GetTaggedValue(),
                                 newCache.GetTaggedValue());
}

JSHandle<JSTaggedValue> CjsModule::Load(JSThread *thread, JSHandle<EcmaString> &request)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // Get local jsPandaFile's dirPath
    JSMutableHandle<JSTaggedValue> parent(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> dirname(thread, JSTaggedValue::Undefined());
    const JSPandaFile *jsPandaFile = EcmaInterpreter::GetNativeCallPandafile(thread);
    RequireManager::ResolveCurrentPath(thread, parent, dirname, jsPandaFile);
    // Get filename from Callback
    JSHandle<EcmaString> filenameStr = ResolveFilename(thread, dirname.GetTaggedValue(),
                                                        request.GetTaggedValue());

    JSHandle<JSTaggedValue> filename = JSHandle<JSTaggedValue>::Cast(filenameStr);

    // Search from Module.cache
    JSHandle<JSTaggedValue> maybeCachedExports = SearchFromModuleCache(thread, filename);
    if (!maybeCachedExports->IsHole()) {
        return maybeCachedExports;
    }

    // Don't get required exports from cache, execute required JSPandaFile.
    // module = new Module(), which belongs to required JSPandaFile.
    JSHandle<CjsModule> module = factory->NewCjsModule();
    InitializeModule(thread, module, filename, dirname);
    PutIntoCache(thread, module, filename);

    // Execute required JSPandaFile
    RequireExecution(thread, JSHandle<EcmaString>::Cast(filename));

    // Search from Module.cache after execution.
    JSHandle<JSTaggedValue> cachedExports = SearchFromModuleCache(thread, filename);
    if (cachedExports->IsHole()) {
        LOG_ECMA(ERROR) << "CJS REQUIRE FAIL : Can not obtain module, after executing required jsPandaFile";
        UNREACHABLE();
    }
    return cachedExports;
}

void CjsModule::RequireExecution(JSThread *thread, const JSHandle<EcmaString> &moduleFileName)
{
    CString moduleFilenameStr = ConvertToString(moduleFileName.GetTaggedValue());
    const JSPandaFile *jsPandaFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, moduleFilenameStr, JSPandaFile::ENTRY_MAIN_FUNCTION);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open jsPandaFile " << moduleFilenameStr << " error";
        UNREACHABLE();
    }
    JSPandaFileExecutor::Execute(thread, jsPandaFile);
}

JSHandle<EcmaString> CjsModule::ResolveFilename(JSThread *thread, JSTaggedValue dirname, JSTaggedValue request)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    ResolvePathCallback resolvePathCallback = thread->GetEcmaVM()->GetResolvePathCallback();
    if (resolvePathCallback == nullptr) {
        JSHandle<EcmaString> nativeRequireName = ResolveFilenameFromNative(thread, dirname, request);
        return nativeRequireName;
    }
    std::string modDirname = std::string(ConvertToString(EcmaString::Cast(dirname.GetTaggedObject())));
    std::string modFile = std::string(ConvertToString(EcmaString::Cast(request.GetTaggedObject())));
    std::string callbackRequireName = resolvePathCallback(modDirname, modFile);
    CString fullname = callbackRequireName.c_str();
    return factory->NewFromUtf8(fullname);
}

JSHandle<EcmaString> CjsModule::ResolveFilenameFromNative(JSThread *thread, JSTaggedValue dirname,
                                                          JSTaggedValue request)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    CString dirnameStr = ConvertToString(EcmaString::Cast(dirname.GetTaggedObject()));
    CString requestStr = ConvertToString(EcmaString::Cast(request.GetTaggedObject()));
    if (requestStr.find("./") == 0) {
        requestStr = requestStr.substr(2); // 2 : delete './'
    }
    int suffixEnd = static_cast<int>(requestStr.find_last_of('.'));
    if (suffixEnd == -1) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    }
    CString fullname;
    if (requestStr[0] == '/') { // absolute FilePath
        fullname = requestStr.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirnameStr.find_last_of('/'));
        if (pos == -1) {
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
        }
        fullname = dirnameStr.substr(0, pos + 1) + requestStr.substr(0, suffixEnd) + ".abc";
    }
    return factory->NewFromUtf8(fullname);
}

JSTaggedValue CjsModule::Require(JSThread *thread, JSHandle<EcmaString> &request,
                                 [[maybe_unused]]JSHandle<CjsModule> &parent,
                                 [[maybe_unused]]bool isMain)
{
    Load(thread, request);
    return JSTaggedValue::Undefined();
}
} // namespace panda::ecmascript