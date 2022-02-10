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

#ifndef ECMASCRIPT_JS_API_TREE_SET_H
#define ECMASCRIPT_JS_API_TREE_SET_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
/**
 * Provide the object of non ECMA standard jsapi container.
 * JSAPITreeSet provides ordered sets.
 * */
class JSAPITreeSet : public JSObject {
public:
    static JSAPITreeSet *Cast(ObjectHeader *object)
    {
        return static_cast<JSAPITreeSet *>(object);
    }

    static void Add(JSThread *thread, const JSHandle<JSAPITreeSet> &set, const JSHandle<JSTaggedValue> &value);

    static void Clear(const JSThread *thread, const JSHandle<JSAPITreeSet> &set);

    static bool Delete(JSThread *thread, const JSHandle<JSAPITreeSet> &set, const JSHandle<JSTaggedValue> &key);

    static bool Has(JSThread *thread, const JSHandle<JSAPITreeSet> &set, const JSHandle<JSTaggedValue> &key);

    static JSTaggedValue PopFirst(JSThread *thread, const JSHandle<JSAPITreeSet> &set);
    static JSTaggedValue PopLast(JSThread *thread, const JSHandle<JSAPITreeSet> &set);

    int GetSize() const;

    JSTaggedValue GetKey(int entry) const;

    JSTaggedValue GetValue(int entry) const;

    static constexpr size_t TREE_SET_OFFSET = JSObject::SIZE;
    ACCESSORS(TreeSet, TREE_SET_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, TREE_SET_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_TREE_SET_H_
