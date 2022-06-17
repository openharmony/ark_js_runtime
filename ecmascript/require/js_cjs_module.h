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

#ifndef ECMASCRIPT_REQUIRE_CJS_MODULE_H
#define ECMASCRIPT_REQUIRE_CJS_MODULE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript {
enum class CjsModuleStatus : uint8_t { UNLOAD = 0x01, LOADED};
class JSCjsModule final : public JSObject {
public:
    CAST_CHECK(JSCjsModule, IsJSCjsModule);

    // Instantiate member
    static constexpr size_t JS_CJS_MODULE_OFFSET = JSObject::SIZE;
    ACCESSORS(Id, JS_CJS_MODULE_OFFSET, ID_OFFSET)
    ACCESSORS(Path, ID_OFFSET, PATH_OFFSET)
    ACCESSORS(Exports, PATH_OFFSET, EXPORTS_OFFSET)
    ACCESSORS(Filename, EXPORTS_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_SIZE) // define BitField
    DEFINE_ALIGN_SIZE(LAST_SIZE);
    // ACCESSORS(Loaded, FILENAME_OFFSET, LOADED_OFFSET)
    // ACCESSORS(Children, LOADED_OFFSET, SIZE)

    static constexpr size_t STATUS_BITS = 2;
    FIRST_BIT_FIELD(BitField, Status, CjsModuleStatus, STATUS_BITS)

    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, JS_CJS_MODULE_OFFSET, BIT_FIELD_OFFSET)

    static void InitializeModule(JSThread *thread, JSHandle<JSCjsModule> &module,
                                              JSHandle<JSTaggedValue> &filename, JSHandle<JSTaggedValue> &dirname);

    static JSHandle<JSTaggedValue> SearchFromModuleCache(JSThread *thread, JSHandle<JSTaggedValue> &filename);

    static void PutIntoCache(JSThread *thread, JSHandle<JSCjsModule> &module, JSHandle<JSTaggedValue> &filename);

    static JSHandle<JSTaggedValue> Load(JSThread *thread, JSHandle<EcmaString> &request);

    static JSTaggedValue Require(JSThread *thread, JSHandle<EcmaString> &request, JSHandle<JSCjsModule> &parent,
                                 bool isMain);

    static JSHandle<EcmaString> ResolveFilename(JSThread *thread, JSTaggedValue dirname, JSTaggedValue filename);

    static JSHandle<EcmaString> ResolveFilenameFromNative(JSThread *thread, JSTaggedValue dirname,
                                                          JSTaggedValue request);

    static void RequireExecution(JSThread *thread, const JSHandle<EcmaString> &moduleFileName);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_REQUIRE_CJS_MODULE_H
