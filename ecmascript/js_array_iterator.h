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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_ARRAY_ITERATOR_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_ARRAY_ITERATOR_H

#include "js_iterator.h"
#include "js_object.h"

namespace panda::ecmascript {
class JSArrayIterator : public JSObject {
public:
    static JSArrayIterator *Cast(ObjectHeader *obj)
    {
        ASSERT(JSTaggedValue(obj).IsJSArrayIterator());
        return static_cast<JSArrayIterator *>(obj);
    }
    static JSTaggedValue Next(EcmaRuntimeCallInfo *argv);

    static constexpr size_t ITERATED_ARRAY_OFFSET = JSObject::SIZE;
    ACCESSORS(IteratedArray, ITERATED_ARRAY_OFFSET, NEXT_INDEX_OFFSET)
    ACCESSORS(NextIndex, NEXT_INDEX_OFFSET, ITERATION_KIND_OFFSET)
    ACCESSORS(IterationKind, ITERATION_KIND_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ITERATED_ARRAY_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_ARRAY_ITERATOR_H
