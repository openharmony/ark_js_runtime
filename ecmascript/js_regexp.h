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

#ifndef ECMASCRIPT_JSREGEXP_H
#define ECMASCRIPT_JSREGEXP_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/ecma_macros.h"

namespace panda::ecmascript {
class JSRegExp : public JSObject {
public:
    CAST_CHECK(JSRegExp, IsJSRegExp);

    static constexpr size_t REGEXP_BYTE_CODE_OFFSET = JSObject::SIZE;
    ACCESSORS(ByteCodeBuffer, REGEXP_BYTE_CODE_OFFSET, ORIGINAL_SOURCE_OFFSET)
    ACCESSORS(OriginalSource, ORIGINAL_SOURCE_OFFSET, ORIGINAL_FLAGS_OFFSET)
    ACCESSORS(OriginalFlags, ORIGINAL_FLAGS_OFFSET, GROUP_NAME_OFFSET)
    ACCESSORS(GroupName, GROUP_NAME_OFFSET, LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, REGEXP_BYTE_CODE_OFFSET, LENGTH_OFFSET)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPRIMITIVEREF_H
