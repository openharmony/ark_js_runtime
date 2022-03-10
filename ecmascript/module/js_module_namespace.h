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

#ifndef ECMASCRIPT_MODULE_JS_MODULE_NAMESPACE_H
#define ECMASCRIPT_MODULE_JS_MODULE_NAMESPACE_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class ModuleNamespace final : public JSObject {
public:
    CAST_CHECK(ModuleNamespace, IsModuleNamespace);

    // 9.4.6.11ModuleNamespaceCreate ( module, exports )
    static JSHandle<ModuleNamespace> ModuleNamespaceCreate(JSThread *thread, const JSHandle<JSTaggedValue> &module,
                                                           const JSHandle<TaggedArray> &exports);
    // 9.4.6.1[[SetPrototypeOf]]
    static bool SetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                             const JSHandle<JSTaggedValue> &proto);
    // 9.4.6.2[[IsExtensible]]
    static bool IsExtensible();
    // 9.4.6.3[[PreventExtensions]]
    static bool PreventExtensions();
    // 9.4.6.4[[GetOwnProperty]]
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);
    // 9.4.6.5[[DefineOwnProperty]] ( P, Desc )
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                  const JSHandle<JSTaggedValue> &key, PropertyDescriptor desc);
    // 9.4.6.6[[HasProperty]]
    static bool HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key);
    // 9.4.6.7[[Get]] ( P, Receiver )
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key);
    // 9.4.6.8[[Set]] ( P, V, Receiver )
    static bool SetProperty(JSThread *thread, bool mayThrow);
    // 9.4.6.9[[Delete]] ( P )
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &key);
    // 9.4.6.10[[OwnPropertyKeys]]
    static JSHandle<TaggedArray> OwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &proxy);

    bool ValidateKeysAvailable(JSThread *thread, const JSHandle<TaggedArray> &exports);

    static constexpr size_t MODULE_OFFSET = JSObject::SIZE;
    ACCESSORS(Module, MODULE_OFFSET, EXPORTS_OFFSET)
    ACCESSORS(Exports, EXPORTS_OFFSET, SIZE)

    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, MODULE_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_MODULE_NAMESPACE_H
