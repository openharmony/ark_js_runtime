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

#include "ecmascript/builtins/builtins_weak_ref.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsWeakRef::WeakRefConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, WeakRef, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    // 2. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "target is not object", JSTaggedValue::Exception());
    }
    // 3. Let weakRef be ? OrdinaryCreateFromConstructor(NewTarget, "%WeakRef.prototype%", « [[WeakRefTarget]] »).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Perform ! AddToKeptObjects(target).
    thread->GetEcmaVM()->GetHeap()->AddToKeptObjects(target);
    // 5. Set weakRef.[[WeakRefTarget]] to target.
    // 6. Return weakRef.
    JSHandle<JSWeakRef> weakRef = JSHandle<JSWeakRef>::Cast(obj);
    weakRef->SetToWeak(target.GetTaggedValue());
    return weakRef.GetTaggedValue();
}

JSTaggedValue BuiltinsWeakRef::Deref(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, WeakRef, Deref);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let weakRef be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(weakRef, [[WeakRefTarget]]).
    if (!thisValue->IsJSWeakRef()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "thisValue is not object or does not have an internalSlot internal slot",
                                    JSTaggedValue::Exception());
    }
    // 3. Return ! WeakRefDeref(weakRef).
    JSHandle<JSWeakRef> weakRef = JSHandle<JSWeakRef>::Cast(thisValue);
    return JSWeakRef::WeakRefDeref(thread, weakRef);
}
} // namespace panda::ecmascript::builtins
