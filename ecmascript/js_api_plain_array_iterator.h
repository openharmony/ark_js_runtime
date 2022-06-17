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

#ifndef ECMASCRIPT_JS_API_PLAIN_ARRAY_ITERATOR_H
#define ECMASCRIPT_JS_API_PLAIN_ARRAY_ITERATOR_H

#include "js_iterator.h"
#include "js_object.h"

namespace panda::ecmascript {
class JSAPIPlainArrayIterator : public JSObject {
public:
    static JSAPIPlainArrayIterator *Cast(TaggedObject *obj)
    {
        ASSERT(JSTaggedValue(obj).IsJSAPIPlainArrayIterator());
        return static_cast<JSAPIPlainArrayIterator *>(obj);
    }
    static JSTaggedValue Next(EcmaRuntimeCallInfo *argv);
    static constexpr size_t ITERATED_PLAIN_ARRAY_OFFSET = JSObject::SIZE;
    ACCESSORS(IteratedPlainArray, ITERATED_PLAIN_ARRAY_OFFSET, NEXT_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(NextIndex, int32_t, NEXT_INDEX_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t ITERATION_KIND_BITS = 2;  // 2 is define bit field
    FIRST_BIT_FIELD(BitField, IterationKind, IterationKind, ITERATION_KIND_BITS)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ITERATED_PLAIN_ARRAY_OFFSET, NEXT_INDEX_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_PLAIN_ARRAY_ITERATOR_H
