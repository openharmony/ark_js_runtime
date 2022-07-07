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

#ifndef ECMASCRIPT_REQUIRE_JS_EXPORTS_H
#define ECMASCRIPT_REQUIRE_JS_EXPORTS_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class CjsExports final : public JSObject {
public:
    CAST_CHECK(CjsExports, IsCjsExports);

    static constexpr size_t JS_CJS_EXPORTS_OFFSET = JSObject::SIZE;
    ACCESSORS(Exports, JS_CJS_EXPORTS_OFFSET, SIZE)

    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, JS_CJS_EXPORTS_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_REQUIRE_JS_MODULE_NAMESPACE_H
