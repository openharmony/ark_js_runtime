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
#include "js_require_manager.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ecma_string.h"

namespace panda::ecmascript {
void RequireManager::ResolveCurrentPath(JSThread *thread,
                                        JSMutableHandle<JSTaggedValue> &dirPath,
                                        JSMutableHandle<JSTaggedValue> &fileName,
                                        const JSPandaFile *jsPandaFile)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString fullName = CString(jsPandaFile->GetPandaFile()->GetFilename());
    // find last '/'
    int foundPos = static_cast<int>(fullName.find_last_of("/\\"));
    if (foundPos == -1) {
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
    CString dirPathStr = fullName.substr(0, foundPos + 1);
    JSHandle<EcmaString> dirPathName = factory->NewFromUtf8(dirPathStr);
    dirPath.Update(dirPathName.GetTaggedValue());

    // Get filename from JSPandaFile
    JSHandle<EcmaString> cbFileName = factory->NewFromUtf8(fullName);
    fileName.Update(cbFileName.GetTaggedValue());
}

void RequireManager::InitializeCommonJS(JSThread *thread, CJSInfo cjsInfo)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<CjsModule> module = cjsInfo.moduleHdl;
    JSHandle<JSTaggedValue> require = cjsInfo.requireHdl;
    JSHandle<CjsExports> exports = cjsInfo.exportsHdl;
    JSHandle<JSTaggedValue> filename = cjsInfo.filenameHdl;
    JSHandle<JSTaggedValue> dirname = cjsInfo.dirnameHdl;

    // Set module.exports ---> exports
    JSHandle<JSTaggedValue> exportsKey = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsKey.GetTaggedValue(),
                                 exports.GetTaggedValue());
    // initialize module
    CjsModule::InitializeModule(thread, module, filename, dirname);

    // Set this.module ---> this.require.parent
    JSHandle<JSTaggedValue> parentKey(factory->NewFromASCII("parent"));
    SlowRuntimeStub::StObjByName(thread, require.GetTaggedValue(), parentKey.GetTaggedValue(),
                                 module.GetTaggedValue());
    // cache ----> Set Module._cache
    JSHandle<JSTaggedValue> cacheKey = globalConst->GetHandledCjsCacheString();
    JSHandle<JSTaggedValue> moduleObj(thread->GetEcmaVM()->GetGlobalEnv()->GetCjsModuleFunction());
    JSTaggedValue modCache =
        SlowRuntimeStub::LdObjByName(thread, moduleObj.GetTaggedValue(),
                                     cacheKey.GetTaggedValue(), false, JSTaggedValue::Undefined());
    JSHandle<CjsModuleCache> moduleCache = JSHandle<CjsModuleCache>(thread, modCache);
    JSHandle<CjsModuleCache> newCache = CjsModuleCache::PutIfAbsent(thread, moduleCache,
                                                                    JSHandle<JSTaggedValue>::Cast(filename),
                                                                    JSHandle<JSTaggedValue>(module));
    SlowRuntimeStub::StObjByName(thread, moduleObj.GetTaggedValue(), cacheKey.GetTaggedValue(),
                                 newCache.GetTaggedValue());
}

void RequireManager::CollectExecutedExp(JSThread *thread, CJSInfo cjsInfo)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<CjsModule> module = cjsInfo.moduleHdl;
    JSHandle<JSTaggedValue> filename = cjsInfo.filenameHdl;

    // get Module from global env
    JSHandle<JSTaggedValue> moduleObj(thread->GetEcmaVM()->GetGlobalEnv()->GetCjsModuleFunction());
    JSHandle<JSTaggedValue> cacheKey = globalConst->GetHandledCjsCacheString();

    JSTaggedValue executedCacheVal = SlowRuntimeStub::LdObjByName(thread, moduleObj.GetTaggedValue(),
                                                                  cacheKey.GetTaggedValue(),
                                                                  false, JSTaggedValue::Undefined());
    JSHandle<CjsModuleCache> executedCache = JSHandle<CjsModuleCache>(thread, executedCacheVal);
    JSHandle<CjsModuleCache> resetCache = CjsModuleCache::ResetModule(thread, executedCache,
                                                                      filename,
                                                                      JSHandle<JSTaggedValue>::Cast(module));
    SlowRuntimeStub::StObjByName(thread, moduleObj.GetTaggedValue(), cacheKey.GetTaggedValue(),
                                 resetCache.GetTaggedValue());
}
} // namespace panda::ecmascript