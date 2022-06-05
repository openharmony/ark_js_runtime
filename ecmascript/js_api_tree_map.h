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

#ifndef ECMASCRIPT_JS_API_TREE_MAP_H
#define ECMASCRIPT_JS_API_TREE_MAP_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
/**
 * Provide the object of non ECMA standard jsapi container.
 * JSAPITreeMap provides ordered maps.
 * */
class JSAPITreeMap : public JSObject {
public:
    static JSAPITreeMap *Cast(ObjectHeader *object)
    {
        return static_cast<JSAPITreeMap *>(object);
    }

    static void Set(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key,
                    const JSHandle<JSTaggedValue> &value);

    static void Clear(const JSThread *thread, const JSHandle<JSAPITreeMap> &map);

    static JSTaggedValue Get(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key);

    static JSTaggedValue Delete(JSThread *thread, const JSHandle<JSAPITreeMap> &map,
                                const JSHandle<JSTaggedValue> &key);

    static bool HasKey(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key);
    bool HasValue(JSThread *thread, const JSHandle<JSTaggedValue> &value) const;

    static bool Replace(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key,
                        const JSHandle<JSTaggedValue> &value);

    int GetSize() const;

    JSTaggedValue GetKey(int entry) const;

    JSTaggedValue GetValue(int entry) const;

    static constexpr size_t TREE_MAP_OFFSET = JSObject::SIZE;
    ACCESSORS(TreeMap, TREE_MAP_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, TREE_MAP_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_TREE_MAP_H
