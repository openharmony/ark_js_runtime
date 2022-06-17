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

#ifndef ECMASCRIPT_JS_API_ARRAYLIST_ITERATOR_H
#define ECMASCRIPT_JS_API_ARRAYLIST_ITERATOR_H

#include "js_iterator.h"
#include "js_object.h"

namespace panda::ecmascript {
/**
 * It is used to provide iterators for non ECMA standard jsapi containers.
 * JSAPIArrayListIterator provides ordered iterators.
 * */
class JSAPIArrayListIterator : public JSObject {
public:
    static JSAPIArrayListIterator *Cast(TaggedObject *obj)
    {
        ASSERT(JSTaggedValue(obj).IsJSAPIArrayListIterator());
        return static_cast<JSAPIArrayListIterator *>(obj);
    }
    static JSTaggedValue Next(EcmaRuntimeCallInfo *argv);

    static constexpr size_t ITERATED_ARRAYLIST_OFFSET = JSObject::SIZE;
    ACCESSORS(IteratedArrayList, ITERATED_ARRAYLIST_OFFSET, NEXT_INDEX_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(NextIndex, uint32_t, NEXT_INDEX_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ITERATED_ARRAYLIST_OFFSET, NEXT_INDEX_OFFSET)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_ARRAYLIST_ITERATOR_H
