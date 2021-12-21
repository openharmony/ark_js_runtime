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

#ifndef ECMASCRIPT_JS_FORIN_ITERATOR_H
#define ECMASCRIPT_JS_FORIN_ITERATOR_H

#include <utility>
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array-inl.h"
#include "js_object.h"

namespace panda::ecmascript {
class JSForInIterator : public JSObject {
public:
    CAST_CHECK(JSForInIterator, IsForinIterator);

    static std::pair<JSTaggedValue, bool> NextInternal(JSThread *thread, const JSHandle<JSForInIterator> &it);

    static JSTaggedValue Next(EcmaRuntimeCallInfo *msg);

    static bool CheckObjProto(const JSThread *thread, const JSHandle<JSForInIterator> &it);

    static void FastGetAllEnumKeys(const JSThread *thread, const JSHandle<JSForInIterator> &it,
                                   const JSHandle<JSTaggedValue> &object);

    static void SlowGetAllEnumKeys(JSThread *thread, const JSHandle<JSForInIterator> &it,
                                   const JSHandle<JSTaggedValue> &object);

    static constexpr size_t OBJECT_OFFSET = JSObject::SIZE;

    ACCESSORS(Object, OBJECT_OFFSET, WAS_VISITED_OFFSET)

    ACCESSORS(WasVisited, WAS_VISITED_OFFSET, VISITED_KEYS_OFFSET)

    ACCESSORS(VisitedKeys, VISITED_KEYS_OFFSET, REMAINING_KEYS_OFFSET)

    ACCESSORS(RemainingKeys, REMAINING_KEYS_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, OBJECT_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_FORIN_ITERATOR_H
