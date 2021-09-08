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

#ifndef ECMASCRIPT_JSMAP_H
#define ECMASCRIPT_JSMAP_H

#include <limits>
#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSMap : public JSObject {
public:
    static JSMap *Cast(ObjectHeader *object)
    {
        return static_cast<JSMap *>(object);
    }
    static bool Delete(const JSThread *thread, const JSHandle<JSMap> &map, const JSHandle<JSTaggedValue> &key);

    static void Set(JSThread *thread, const JSHandle<JSMap> &map, const JSHandle<JSTaggedValue> &key,
                    const JSHandle<JSTaggedValue> &value);
    static void Clear(const JSThread *thread, const JSHandle<JSMap> &map);

    bool Has(JSTaggedValue key) const;

    JSTaggedValue Get(JSTaggedValue key) const;

    int GetSize() const;

    JSTaggedValue GetKey(int entry) const;

    JSTaggedValue GetValue(int entry) const;

    static constexpr size_t LINKED_MAP_OFFSET = JSObject::SIZE;
    ACCESSORS(LinkedMap, LINKED_MAP_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LINKED_MAP_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSMAP_H
