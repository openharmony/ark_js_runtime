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

#ifndef ECMASCRIPT_JS_WEAK_CONTAINER_H
#define ECMASCRIPT_JS_WEAK_CONTAINER_H

#include <limits>
#include "ecmascript/js_object.h"

namespace panda::ecmascript {
class JSWeakMap : public JSObject {
public:
    static JSWeakMap *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSWeakMap());
        return static_cast<JSWeakMap *>(object);
    }

    static bool Delete(JSThread *thread, const JSHandle<JSWeakMap> &map, const JSHandle<JSTaggedValue> &key);

    static void Set(JSThread *thread, const JSHandle<JSWeakMap> &map, const JSHandle<JSTaggedValue> &key,
                    const JSHandle<JSTaggedValue> &value);

    bool Has(JSTaggedValue key) const;

    JSTaggedValue Get(JSTaggedValue key) const;

    int GetSize() const;

    static constexpr size_t LINKED_MAP_OFFSET = JSObject::SIZE;
    ACCESSORS(LinkedMap, LINKED_MAP_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LINKED_MAP_OFFSET, SIZE)
    DECL_DUMP()
};

class JSWeakSet : public JSObject {
public:
    static JSWeakSet *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSWeakSet());
        return static_cast<JSWeakSet *>(object);
    }
    static bool Delete(JSThread *thread, const JSHandle<JSWeakSet> &set, const JSHandle<JSTaggedValue> &value);

    static void Add(JSThread *thread, const JSHandle<JSWeakSet> &set, const JSHandle<JSTaggedValue> &value);

    bool Has(JSTaggedValue value) const;

    int GetSize() const;

    static constexpr size_t LINKED_SET_OFFSET = JSObject::SIZE;
    ACCESSORS(LinkedSet, LINKED_SET_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LINKED_SET_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_WEAK_CONTAINER_H