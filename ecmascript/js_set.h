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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JSSET_H
#define PANDA_RUNTIME_ECMASCRIPT_JSSET_H

#include <limits>
#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSSet : public JSObject {
public:
    static JSSet *Cast(ObjectHeader *object)
    {
        return static_cast<JSSet *>(object);
    }

    static bool Delete(const JSThread *thread, const JSHandle<JSSet> &set, const JSHandle<JSTaggedValue> &value);

    static void Add(JSThread *thread, const JSHandle<JSSet> &set, const JSHandle<JSTaggedValue> &value);

    static void Clear(const JSThread *thread, const JSHandle<JSSet> &set);

    bool Has(JSTaggedValue value) const;

    int GetSize() const;

    JSTaggedValue GetValue(int entry) const;

    static constexpr size_t LINKED_SET_OFFSET = JSObject::SIZE;
    ACCESSORS(LinkedSet, LINKED_SET_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LINKED_SET_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JSSET_H
