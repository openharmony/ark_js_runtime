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

#ifndef ECMASCRIPT_JS_STRING_ITERATOR_H
#define ECMASCRIPT_JS_STRING_ITERATOR_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda {
namespace ecmascript {
class JSStringIterator : public JSObject {
public:
    CAST_CHECK(JSStringIterator, IsStringIterator);

    static JSHandle<JSStringIterator> CreateStringIterator(const JSThread *thread, const JSHandle<EcmaString> &string);

    static constexpr size_t ITERATED_STRING_OFFSET = JSObject::SIZE;
    ACCESSORS(IteratedString, ITERATED_STRING_OFFSET, STRING_ITERATOR_NEXT_INDEX_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(StringIteratorNextIndex, uint32_t, STRING_ITERATOR_NEXT_INDEX_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ITERATED_STRING_OFFSET, STRING_ITERATOR_NEXT_INDEX_OFFSET)
    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JS_STRING_ITERATOR_H
