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

#ifndef ECMASCRIPT_JS_WEAK_REF_H
#define ECMASCRIPT_JS_WEAK_REF_H

#include "ecmascript/js_object.h"

namespace panda::ecmascript {
class JSWeakRef : public JSObject {
public:
    static JSWeakRef *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSWeakRef());
        return static_cast<JSWeakRef *>(object);
    }

    // 26.1.4.1 WeakRefDeref ( weakRef )
    static JSTaggedValue WeakRefDeref(JSThread *thread, const JSHandle<JSWeakRef> &weakRef)
    {
        // 1. Let target be weakRef.[[WeakRefTarget]].
        // 2. If target is not empty, then
        //     a. Perform ! AddToKeptObjects(target).
        //     b. Return target.
        // 3. Return undefined.
        JSHandle<JSTaggedValue> target(thread, weakRef->GetFromWeak());
        if (!target->IsUndefined()) {
            thread->GetEcmaVM()->GetHeap()->AddToKeptObjects(target);
        }
        return target.GetTaggedValue();
    }

    void SetToWeak(JSTaggedValue value)
    {
        JSTaggedValue weakObj = JSTaggedValue(value.CreateAndGetWeakRef());
        ASSERT(weakObj.IsWeak());
        SetWeakObject(weakObj);
    }

    JSTaggedValue GetFromWeak() const
    {
        JSTaggedValue weakObj = GetWeakObject();
        if (!weakObj.IsUndefined()) {
            return JSTaggedValue(weakObj.GetWeakReferent());
        }
        return weakObj;
    }
    static constexpr size_t WEAK_OBJECT_OFFSET = JSObject::SIZE;
    ACCESSORS(WeakObject, WEAK_OBJECT_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, WEAK_OBJECT_OFFSET, SIZE)
    DECL_DUMP()
};
} // namespace
#endif // ECMASCRIPT_JS_WEAK_REF_H