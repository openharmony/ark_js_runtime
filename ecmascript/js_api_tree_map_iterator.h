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

#ifndef ECMASCRIPT_JS_API_TREE_MAP_ITERATOR_H
#define ECMASCRIPT_JS_API_TREE_MAP_ITERATOR_H

#include "js_iterator.h"
#include "js_object.h"

namespace panda::ecmascript {
/**
 * It is used to provide iterators for non ECMA standard jsapi containers.
 * JSAPITreeMapIterator provides ordered iterators.
 * */
class JSAPITreeMapIterator : public JSObject {
public:
    static JSAPITreeMapIterator *Cast(ObjectHeader *obj)
    {
        ASSERT(JSTaggedValue(obj).IsJSAPITreeMapIterator());
        return static_cast<JSAPITreeMapIterator *>(obj);
    }
    static JSHandle<JSTaggedValue> CreateTreeMapIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                         IterationKind kind);

    static JSTaggedValue Next(EcmaRuntimeCallInfo *argv);

    static constexpr size_t ITERATED_MAP_OFFSET = JSObject::SIZE;
    ACCESSORS(IteratedMap, ITERATED_MAP_OFFSET, ITERATED_ENTRIES_OFFSET);
    ACCESSORS(Entries, ITERATED_ENTRIES_OFFSET, NEXT_INDEX_OFFSET);
    ACCESSORS(NextIndex, NEXT_INDEX_OFFSET, ITERATION_KIND_OFFSET);
    ACCESSORS(IterationKind, ITERATION_KIND_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ITERATED_MAP_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_TREE_MAP_ITERATOR_H
