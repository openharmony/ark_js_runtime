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

#ifndef ECMASCRIPT_JSGLOBALOBJECT_H
#define ECMASCRIPT_JSGLOBALOBJECT_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSGlobalObject : public JSObject {
public:
    static JSGlobalObject *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsECMAObject());
        return static_cast<JSGlobalObject *>(object);
    }

    static constexpr size_t SIZE = JSObject::SIZE;

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, SIZE, SIZE)
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSGLOBALOBJECT_H
