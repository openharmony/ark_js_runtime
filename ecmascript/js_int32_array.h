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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_INT32_ARRAY_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_INT32_ARRAY_H

#include <limits>
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSInt32Array : public JSObject {
public:
    static JSInt32Array *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSInt32Array());
        return static_cast<JSInt32Array *>(object);
    }

    static constexpr size_t VIEWED_ARRAY_BUFFER_OFFSET = JSObject::SIZE;
    ACCESSORS(ViewedArrayBuffer, VIEWED_ARRAY_BUFFER_OFFSET, TYPED_ARRAY_NAME_OFFSET)
    ACCESSORS(TypedArrayName, TYPED_ARRAY_NAME_OFFSET, BYTE_LENGTH_OFFSET)
    ACCESSORS(ByteLength, BYTE_LENGTH_OFFSET, BYTE_OFFSET_OFFSET)
    ACCESSORS(ByteOffset, BYTE_OFFSET_OFFSET, ARRAY_LENGTH_OFFSET)
    ACCESSORS(ArrayLength, ARRAY_LENGTH_OFFSET, SIZE)
    DECL_DUMP()

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, VIEWED_ARRAY_BUFFER_OFFSET, SIZE)
};  // namespace panda::ecmascript
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_INT32_ARRAY_H
