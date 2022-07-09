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

#ifndef ECMASCRIPT_REQUIRE_CJS_REQUIRE_H
#define ECMASCRIPT_REQUIRE_CJS_REQUIRE_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript {
class CjsRequire final : public JSObject {
public:
    CAST_CHECK(CjsRequire, IsCjsRequire);

    // Instantiate member
    static constexpr size_t JS_CJS_REQUIRE_OFFSET = JSObject::SIZE;
    ACCESSORS(Cache, JS_CJS_REQUIRE_OFFSET, CACHE_OFFSET)
    ACCESSORS(Parent, CACHE_OFFSET, SIZE)

    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, JS_CJS_REQUIRE_OFFSET, SIZE)

    static JSTaggedValue Main(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Resolve(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_REQUIRE_CJS_REQUIRE_H
