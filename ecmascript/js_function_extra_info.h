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

#ifndef ECMASCRIPT_JS_FUNCTION_INFO_H
#define ECMASCRIPT_JS_FUNCTION_INFO_H

#include "utils/bit_field.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "js_tagged_value-inl.h"
#include "js_function_kind.h"

namespace panda::ecmascript {
class JSFunctionExtraInfo : public TaggedObject {
public:
    static JSFunctionExtraInfo *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSFunctionExtraInfo());
        return static_cast<JSFunctionExtraInfo *>(object);
    }

    static constexpr size_t CALL_BACK_OFFSET = TaggedObjectSize();
    ACCESSORS(Callback, CALL_BACK_OFFSET, DATA_OFFSET);
    ACCESSORS(Data, DATA_OFFSET, SIZE);

    DECL_VISIT_OBJECT(CALL_BACK_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif
