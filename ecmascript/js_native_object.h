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

#ifndef ECMASCRIPT_JSNATIVEOBJECT_H
#define ECMASCRIPT_JSNATIVEOBJECT_H

#include "js_object.h"
#include "js_native_pointer.h"

namespace panda::ecmascript {
class JSNativeObject : JSObject {
public:
    static JSNativeObject *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSNativeObject());
        return static_cast<JSNativeObject *>(object);
    }

    inline void SetDeleter(DeleteEntryPoint deleter)
    {
        if (!GetJSNativePointer().IsJSNativePointer()) {
            return;
        }
        JSNativePointer::Cast(GetJSNativePointer().GetHeapObject())->SetDeleter(deleter);
    }

    inline void SetData(void *data)
    {
        if (!GetJSNativePointer().IsJSNativePointer()) {
            return;
        }
        JSNativePointer::Cast(GetJSNativePointer().GetHeapObject())->SetData(data);
    }

    inline const void *GetData() const
    {
        if (!GetJSNativePointer().IsJSNativePointer()) {
            return nullptr;
        }
        return JSNativePointer::Cast(GetJSNativePointer().GetHeapObject())->GetData();
    }

    inline void Destroy()
    {
        if (!GetJSNativePointer().IsJSNativePointer()) {
            return;
        }
        JSNativePointer::Cast(GetJSNativePointer().GetHeapObject())->Destroy();
    }

    inline void *GetExternalPointer() const
    {
        if (!GetJSNativePointer().IsJSNativePointer()) {
            return nullptr;
        }
        return JSNativePointer::Cast(GetJSNativePointer().GetHeapObject())->GetExternalPointer();
    }

    static constexpr size_t OBJECT_OFFSET = JSObject::SIZE;
    ACCESSORS(JSNativePointer, OBJECT_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, OBJECT_OFFSET, SIZE)
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSNATIVEOBJECT_H
