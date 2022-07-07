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

#ifndef ECMASCRIPT_REQUIRE_JS_REQUIRE_MANAGER_H
#define ECMASCRIPT_REQUIRE_JS_REQUIRE_MANAGER_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/require/js_cjs_exports.h"

namespace panda::ecmascript {
struct CJSInfo {
    JSHandle<CjsModule> moduleHdl;
    JSHandle<JSTaggedValue> requireHdl;
    JSHandle<CjsExports> exportsHdl;
    JSHandle<JSTaggedValue> filenameHdl;
    JSHandle<JSTaggedValue> dirnameHdl;
    CJSInfo(JSHandle<CjsModule> &module,
            JSHandle<JSTaggedValue> &require,
            JSHandle<CjsExports> &exports,
            JSHandle<JSTaggedValue> &filename,
            JSHandle<JSTaggedValue> &dirname) : moduleHdl(module), requireHdl(require), exportsHdl(exports),
                                                filenameHdl(filename), dirnameHdl(dirname) {}
};
class RequireManager {
public:

    static void ResolveCurrentPath(JSThread *thread, JSMutableHandle<JSTaggedValue> &dirPath,
                                   JSMutableHandle<JSTaggedValue> &file,
                                   const JSPandaFile *jsPandaFile);

    static void InitializeCommonJS(JSThread *thread, CJSInfo cjsInfo);

    static void CollectExecutedExp(JSThread *thread, CJSInfo cjsInfo);
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_REQUIRE_JS_REQUIRE_MANAGER_H