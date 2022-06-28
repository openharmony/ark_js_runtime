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

#include "ecmascript/builtins/builtins_finalization_registry.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
// 26.2.1.1
JSTaggedValue BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, FinalizationRegistry, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    // 2. If IsCallable(cleanupCallback) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> cleanupCallback = GetCallArg(argv, 0);
    if (!cleanupCallback->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "cleanupCallback not Callable", JSTaggedValue::Exception());
    }
    // 3. Let finalizationRegistry be ? OrdinaryCreateFromConstructor(NewTarget, "%FinalizationRegistry.prototype%",
    // « [[Realm]], [[CleanupCallback]], [[Cells]] »).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSFinalizationRegistry> finalization = JSHandle<JSFinalizationRegistry>::Cast(obj);
    // 4. Let fn be the active function object.
    // 5. Set finalizationRegistry.[[Realm]] to fn.[[Realm]].
    // 6. Set finalizationRegistry.[[CleanupCallback]] to cleanupCallback.
    finalization->SetCleanupCallback(thread, cleanupCallback);
    // 7. Set finalizationRegistry.[[Cells]] to a new empty List.
    JSHandle<CellRecordVector> noUnregister(CellRecordVector::Create(thread));
    JSHandle<LinkedHashMap> maybeUnregister = LinkedHashMap::Create(thread);
    finalization->SetNoUnregister(thread, noUnregister);
    finalization->SetMaybeUnregister(thread, maybeUnregister);
    JSHandle<JSTaggedValue> objValue(finalization);
    // 8. Return finalizationRegistry.
    return finalization.GetTaggedValue();
}
// 26.2.3.2
JSTaggedValue BuiltinsFinalizationRegistry::Register(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, FinalizationRegistry, Register);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> heldValue  = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> unregisterToken = GetCallArg(argv, 2);  // 2 : unregisterToken storage location
    // 1. Let finalizationRegistry be the this value.
    JSHandle<JSTaggedValue> finalizationRegistry = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(finalizationRegistry, [[Cells]]).
    if (!finalizationRegistry->IsJSFinalizationRegistry()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "thisValue is not object or does not have an internalSlot internal slot",
                                    JSTaggedValue::Exception());
    }
    // 3. If Type(target) is not Object, throw a TypeError exception.
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "target is not object", JSTaggedValue::Exception());
    }
    // 4. If SameValue(target, heldValue) is true, throw a TypeError exception.
    if (JSTaggedValue::SameValue(target, heldValue)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "target and heldValue should not be equal", JSTaggedValue::Exception());
    }
    // 5. If Type(unregisterToken) is not Object, then
    //     a. If unregisterToken is not undefined, throw a TypeError exception.
    //     b. Set unregisterToken to empty.
    if (!unregisterToken->IsECMAObject() && !unregisterToken->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "unregisterToken should be object", JSTaggedValue::Exception());
    }
    // 6. Let cell be the Record { [[WeakRefTarget]]: target,
    //                             [[HeldValue]]: heldValue, [[UnregisterToken]]: unregisterToken }.
    // 7. Append cell to finalizationRegistry.[[Cells]].
    JSHandle<JSFinalizationRegistry> finRegHandle(finalizationRegistry);
    JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finRegHandle);
    // 8. Return undefined.
    return JSTaggedValue::Undefined();
}
// 26.2.3.3
JSTaggedValue BuiltinsFinalizationRegistry::Unregister(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, FinalizationRegistry, Unregister);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> unregisterToken = GetCallArg(argv, 0);
    // 1. Let finalizationRegistry be the this value.
    JSHandle<JSTaggedValue> finalizationRegistry = GetThis(argv);
    // 2. Perform ? RequireInternalSlot(finalizationRegistry, [[Cells]]).
    if (!finalizationRegistry->IsJSFinalizationRegistry()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "thisValue is not object or does not have an internalSlot internal slot",
                                    JSTaggedValue::Exception());
    }
    // 3. If Type(unregisterToken) is not Object, throw a TypeError exception.
    if (!unregisterToken->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "unregisterToken should be object", JSTaggedValue::Exception());
    }
    // 4. Let removed be false.
    // 5. For each Record { [[WeakRefTarget]], [[HeldValue]], [[UnregisterToken]] } cell of
    // finalizationRegistry.[[Cells]], do
    //     a. If cell.[[UnregisterToken]] is not empty and SameValue(cell.[[UnregisterToken]], unregisterToken)
    //     is true, then
    //         i. Remove cell from finalizationRegistry.[[Cells]].
    //         ii. Set removed to true.
    JSHandle<JSFinalizationRegistry> finRegHandle(finalizationRegistry);
    bool removed = JSFinalizationRegistry::Unregister(thread, unregisterToken, finRegHandle);
    // 6. Return removed.
    return JSTaggedValue(removed);
}
} // namespace panda::ecmascript::builtins