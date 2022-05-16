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

#ifndef ECMASCRIPT_JS_REGEXP_ITERATOR_H
#define ECMASCRIPT_JS_REGEXP_ITERATOR_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_iterator.h"

namespace panda::ecmascript {
class JSRegExpIterator : public JSObject {
public:
    CAST_CHECK(JSRegExpIterator, IsJSRegExpIterator);

    // 22.2.7.1 CreateRegExpStringIterator ( R, S, global, fullUnicode )
    static JSHandle<JSTaggedValue> CreateRegExpStringIterator(JSThread *thread, const JSHandle<JSTaggedValue> &matcher,
                                                              const JSHandle<EcmaString> &inputStr, bool global,
                                                              bool fullUnicode);

    static JSTaggedValue Next(EcmaRuntimeCallInfo *argv);

    static constexpr size_t ITERATED_REGEXP_OFFSET = JSObject::SIZE;
    ACCESSORS(IteratingRegExp, ITERATED_REGEXP_OFFSET, ITERATED_STRING_OFFSET);
    ACCESSORS(IteratedString, ITERATED_STRING_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t REGEXP_GLOBAL_BITS = 1;
    static constexpr size_t REGEXP_UNICODE_BITS = 1;
    static constexpr size_t REGEXP_DONE_BITS = 1;
    FIRST_BIT_FIELD(BitField, Global, bool, REGEXP_GLOBAL_BITS)
    NEXT_BIT_FIELD(BitField, Unicode, bool, REGEXP_UNICODE_BITS, Global)
    NEXT_BIT_FIELD(BitField, Done, bool, REGEXP_DONE_BITS, Unicode)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ITERATED_REGEXP_OFFSET, BIT_FIELD_OFFSET)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_REGEXP_ITERATOR_H
