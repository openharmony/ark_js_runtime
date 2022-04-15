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

#include "js_proxy.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
// ES6 9.5.15 ProxyCreate(target, handler)
JSHandle<JSProxy> JSProxy::ProxyCreate(JSThread *thread, const JSHandle<JSTaggedValue> &target,
                                       const JSHandle<JSTaggedValue> &handler)
{
    // 1. If Type(target) is not Object, throw a TypeError exception.
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "ProxyCreate: target is not Object",
                                    JSHandle<JSProxy>(thread, JSTaggedValue::Exception()));
    }

    // 2. If Type(handler) is not Object, throw a TypeError exception.
    if (!handler->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "ProxyCreate: handler is not Object",
                                    JSHandle<JSProxy>(thread, JSTaggedValue::Exception()));
    }
    // 3. Let P be ! MakeBasicObject(« [[ProxyHandler]], [[ProxyTarget]] »).
    // 6. If IsCallable(target) is true, then P.[[Call]] as specified in 9.5.12.

    // 8. Set the [[ProxyTarget]] internal slot of P to target.
    // 9. Set the [[ProxyHandler]] internal slot of P to handler.
    return thread->GetEcmaVM()->GetFactory()->NewJSProxy(target, handler);
}

// ES6 9.5.1 [[GetPrototypeOf]] ( )
JSTaggedValue JSProxy::GetPrototype(JSThread *thread, const JSHandle<JSProxy> &proxy)
{
    // 1. Let handler be the value of the [[ProxyHandler]] internal slot of O.
    JSHandle<JSTaggedValue> handler(thread, proxy->GetHandler());
    // 2. If handler is null, throw a TypeError exception.
    if (handler->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetPrototype: handler is null", JSTaggedValue::Exception());
    }
    // 3. Assert: Type(handler) is Object.
    ASSERT(handler->IsECMAObject());
    // 4. Let target be the value of the [[ProxyTarget]] internal slot of O.
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    // 5. Let trap be GetMethod(handler, "getPrototypeOf").
    JSHandle<JSTaggedValue> name(thread->GlobalConstants()->GetHandledGetPrototypeOfString());
    JSHandle<JSTaggedValue> trap = JSObject::GetMethod(thread, handler, name);
    // 6. ReturnIfAbrupt(trap).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. If trap is undefined, then Return target.[[GetPrototypeOf]]().
    if (trap->IsUndefined()) {
        return JSTaggedValue::GetPrototype(thread, targetHandle);
    }
    // 8. Let handlerProto be Call(trap, handler, «target»).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handler, undefined, 1);
    info.SetCallArg(targetHandle.GetTaggedValue());
    JSTaggedValue handlerProto = JSFunction::Call(&info);

    // 9. ReturnIfAbrupt(handlerProto).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 10. If Type(handlerProto) is neither Object nor Null, throw a TypeError exception.
    if (!handlerProto.IsECMAObject() && !handlerProto.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetPrototype: Type(handlerProto) is neither Object nor Null",
                                    JSTaggedValue::Exception());
    }
    // 11. Let extensibleTarget be IsExtensible(target).
    // 12. ReturnIfAbrupt(extensibleTarget).
    // 13. If extensibleTarget is true, return handlerProto.
    if (targetHandle->IsExtensible(thread)) {
        return handlerProto;
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 14. Let targetProto be target.[[GetPrototypeOf]]().
    JSTaggedValue targetProto = JSTaggedValue::GetPrototype(thread, targetHandle);
    // 15. ReturnIfAbrupt(targetProto).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 16. If SameValue(handlerProto, targetProto) is false, throw a TypeError exception.
    if (!JSTaggedValue::SameValue(handlerProto, targetProto)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetPrototype: SameValue(handlerProto, targetProto) is false",
                                    JSTaggedValue::Exception());
    }
    // 17. Return handlerProto.
    return handlerProto;
}

// ES6 9.5.2 [[SetPrototypeOf]] (V)
bool JSProxy::SetPrototype(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &proto)
{
    // 1. Assert: Either Type(V) is Object or Type(V) is Null.
    ASSERT(proto->IsECMAObject() || proto->IsNull());
    // 2. Let handler be the value of the [[ProxyHandler]] internal slot of O.
    JSTaggedValue handler = proxy->GetHandler();
    // 3. If handler is null, throw a TypeError exception.
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::SetPrototype: handler is null", false);
    }
    // 4. Assert: Type(handler) is Object.
    ASSERT(handler.IsECMAObject());
    // 5. Let target be the value of the [[ProxyTarget]] internal slot of O.
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    // 6. Let trap be GetMethod(handler, "setPrototypeOf").
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledSetPrototypeOfString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 7. If trap is undefined, then Return target.[[SetPrototypeOf]](V).
    if (trap->IsUndefined()) {
        return JSTaggedValue::SetPrototype(thread, targetHandle, proto);
    }
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    const size_t argsLength = 2;  // 2: target and proto
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(targetHandle.GetTaggedValue(), proto.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    // 9. Let booleanTrapResult be ToBoolean(Call(trap, handler, «target, V»)).
    // If booleanTrapResult is false, return false
    bool booleanTrapResult = trapResult.ToBoolean();
    if (!booleanTrapResult) {
        return false;
    }
    // 10. ReturnIfAbrupt(booleanTrapResult).
    // 11. Let extensibleTarget be IsExtensible(target).
    // 12. ReturnIfAbrupt(extensibleTarget).
    // 13. If extensibleTarget is true, return booleanTrapResult
    if (targetHandle->IsExtensible(thread)) {
        return booleanTrapResult;
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 14. Let targetProto be target.[[GetPrototypeOf]]().
    JSTaggedValue targetProto = JSTaggedValue::GetPrototype(thread, targetHandle);
    // 15. ReturnIfAbrupt(targetProto).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 16. If booleanTrapResult is true and SameValue(V, targetProto) is false, throw a TypeError exception.
    if (booleanTrapResult && !JSTaggedValue::SameValue(proto.GetTaggedValue(), targetProto)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::SetPrototype: TypeError of targetProto and Result", false);
    }
    // 17. Return handlerProto.
    return booleanTrapResult;
}

// ES6 9.5.3 [[IsExtensible]] ( )
bool JSProxy::IsExtensible(JSThread *thread, const JSHandle<JSProxy> &proxy)
{
    // 1. Let handler be the value of the [[ProxyHandler]] internal slot of O.
    JSTaggedValue handler = proxy->GetHandler();
    // 2. If handler is null, throw a TypeError exception.
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::IsExtensible: handler is null", false);
    }
    // 3. Assert: Type(handler) is Object.
    ASSERT(handler.IsECMAObject());
    // 4. Let target be the value of the [[ProxyTarget]] internal slot of O.
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    // 5. Let trap be GetMethod(handler, "isExtensible").
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledIsExtensibleString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 6. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 7. If trap is undefined, then Return target.[[IsExtensible]]().
    if (trap->IsUndefined()) {
        return targetHandle->IsExtensible(thread);
    }
    // 8. Let booleanTrapResult be ToBoolean(Call(trap, handler, «target»)).
    JSHandle<JSTaggedValue> newTgt(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, 1);
    info.SetCallArg(targetHandle.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    bool booleanTrapResult = trapResult.ToBoolean();
    // 9. ReturnIfAbrupt(booleanTrapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 10. Let targetResult be target.[[IsExtensible]]().
    // 11. ReturnIfAbrupt(targetResult).
    // 12. If SameValue(booleanTrapResult, targetResult) is false, throw a TypeError exception.
    // 13. Return booleanTrapResult.
    if (targetHandle->IsExtensible(thread) != booleanTrapResult) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::IsExtensible: TypeError of targetResult", false);
    }

    return booleanTrapResult;
}

// ES6 9.5.4 [[PreventExtensions]] ( )
bool JSProxy::PreventExtensions(JSThread *thread, const JSHandle<JSProxy> &proxy)
{
    // 1. Let handler be the value of the [[ProxyHandler]] internal slot of O.
    // 2. If handler is null, throw a TypeError exception.
    // 3. Assert: Type(handler) is Object.
    // 4. Let target be the value of the [[ProxyTarget]] internal slot of O.
    // 5. Let trap be GetMethod(handler, "preventExtensions").
    // 6. ReturnIfAbrupt(trap).
    // 7. If trap is undefined, then
    // a. Return target.[[PreventExtensions]]().
    // 8. Let booleanTrapResult be ToBoolean(Call(trap, handler, «target»)).
    // 9. ReturnIfAbrupt(booleanTrapResult).
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::PreventExtensions: handler is null", false);
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledPreventExtensionsString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 6. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    if (trap->IsUndefined()) {
        return JSTaggedValue::PreventExtensions(thread, targetHandle);
    }
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, 1);
    info.SetCallArg(targetHandle.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    bool booleanTrapResult = trapResult.ToBoolean();
    // 9. ReturnIfAbrupt(booleanTrapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 10. If booleanTrapResult is true, then
    // a. Let targetIsExtensible be target.[[IsExtensible]]().
    // b. ReturnIfAbrupt(targetIsExtensible).
    // c. If targetIsExtensible is true, throw a TypeError exception.
    if (booleanTrapResult && targetHandle->IsExtensible(thread)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::PreventExtensions: targetIsExtensible is true", false);
    }
    // 11. Return booleanTrapResult.
    return booleanTrapResult;
}

// ES6 9.5.5 [[GetOwnProperty]] (P)
bool JSProxy::GetOwnProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                             PropertyDescriptor &desc)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT(JSTaggedValue::IsPropertyKey(key));
    // 2. Let handler be the value of the [[ProxyHandler]] internal slot of O.
    // 3. If handler is null, throw a TypeError exception.
    // 4. Assert: Type(handler) is Object.
    // 5. Let target be the value of the [[ProxyTarget]] internal slot of O.
    // 6. Let trap be GetMethod(handler, "getOwnPropertyDescriptor").
    // 7. ReturnIfAbrupt(trap).
    // 8. If trap is undefined, then
    // a. Return target.[[GetOwnProperty]](P).
    // 9. Let trapResultObj be Call(trap, handler, «target, P»).
    // 10. ReturnIfAbrupt(trapResultObj).
    // 11. If Type(trapResultObj) is neither Object nor Undefined, throw a TypeError exception
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: handler is null", false);
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledGetOwnPropertyDescriptorString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    if (trap->IsUndefined()) {
        return JSTaggedValue::GetOwnProperty(thread, targetHandle, key, desc);
    }
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    const size_t argsLength = 2;  // 2: target and key
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(targetHandle.GetTaggedValue(), key.GetTaggedValue());
    JSTaggedValue trapResultObj = JSFunction::Call(&info);

    JSHandle<JSTaggedValue> resultHandle(thread, trapResultObj);

    // 11. If Type(trapResultObj) is neither Object nor Undefined, throw a TypeError exception.
    if (!trapResultObj.IsECMAObject() && !trapResultObj.IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: TypeError of trapResultObj", false);
    }
    // 12. Let targetDesc be target.[[GetOwnProperty]](P).
    PropertyDescriptor targetDesc(thread);
    bool found = JSTaggedValue::GetOwnProperty(thread, targetHandle, key, targetDesc);
    // 13. ReturnIfAbrupt(targetDesc).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 14. If trapResultObj is undefined, then
    if (resultHandle->IsUndefined()) {
        // a. If targetDesc is undefined, return undefined.
        if (!found) {
            return false;
        }
        // b. If targetDesc.[[Configurable]] is false, throw a TypeError exception.
        if (!targetDesc.IsConfigurable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: targetDesc.[[Configurable]] is false", false);
        }
        // c. Let extensibleTarget be IsExtensible(target).
        // d. ReturnIfAbrupt(extensibleTarget).
        // e. Assert: Type(extensibleTarget) is Boolean.
        // f. If extensibleTarget is false, throw a TypeError exception.
        if (!targetHandle->IsExtensible(thread)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: extensibleTarget is false", false);
        }
        // g. Return undefined.
        return false;
    }
    // 15. Let extensibleTarget be IsExtensible(target).
    // 16. ReturnIfAbrupt(extensibleTarget).
    // 17. Let resultDesc be ToPropertyDescriptor(trapResultObj).
    PropertyDescriptor &resultDesc = desc;
    JSObject::ToPropertyDescriptor(thread, resultHandle, resultDesc);
    // 18. ReturnIfAbrupt(resultDesc)
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 19. Call CompletePropertyDescriptor(resultDesc).
    PropertyDescriptor::CompletePropertyDescriptor(thread, resultDesc);
    // 20. Let valid be IsCompatiblePropertyDescriptor (extensibleTarget, resultDesc, targetDesc).
    bool valid = JSObject::IsCompatiblePropertyDescriptor(targetHandle->IsExtensible(thread), resultDesc, targetDesc);
    if (!valid) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: TypeError of valid", false);
    }
    // 22. If resultDesc.[[Configurable]] is false, then
    if (!resultDesc.IsConfigurable()) {
        // a. If targetDesc is undefined or targetDesc.[[Configurable]] is true, then
        if (!found || targetDesc.IsConfigurable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: TypeError of targetDesc configurable", false);
        }
        // b. If resultDesc has a [[Writable]] field and resultDesc.[[Writable]] is false, then
        //    If targetDesc.[[Writable]] is true, throw a TypeError exception.
        if (resultDesc.HasWritable() && !resultDesc.IsWritable() && targetDesc.IsWritable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetOwnProperty: TypeError of targetDesc writable", false);
        }
        // b. If resultDesc has a [[Writable]] field and resultDesc.[[Writable]] is false, then
        //    If targetDesc.[[Writable]] is true, throw a TypeError exception.
        if (resultDesc.HasWritable() && !resultDesc.IsWritable() && targetDesc.IsWritable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "", false);
        }
    }
    // 23. Return resultDesc.
    return true;
}

// ES6 9.5.6 [[DefineOwnProperty]] (P, Desc)
bool JSProxy::DefineOwnProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                                const PropertyDescriptor &desc)
{
    // step 1 ~ 10 are almost same as GetOwnProperty
    ASSERT(JSTaggedValue::IsPropertyKey(key));
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DefineOwnProperty: handler is Null", false);
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledDefinePropertyString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (trap->IsUndefined()) {
        return JSTaggedValue::DefineOwnProperty(thread, targetHandle, key, desc);
    }

    // 9. Let descObj be FromPropertyDescriptor(Desc).
    JSHandle<JSTaggedValue> descObj = JSObject::FromPropertyDescriptor(thread, desc);
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    const size_t argsLength = 3;  // 3: target, key and desc
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(targetHandle.GetTaggedValue(), key.GetTaggedValue(), descObj.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    bool booleanTrapResult = trapResult.ToBoolean();
    // 11. ReturnIfAbrupt(booleanTrapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!booleanTrapResult) {
        return false;
    }
    // 13. Let targetDesc be target.[[GetOwnProperty]](P).
    PropertyDescriptor targetDesc(thread);
    bool found = JSTaggedValue::GetOwnProperty(thread, targetHandle, key, targetDesc);
    // 14. ReturnIfAbrupt(targetDesc).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 15. Let extensibleTarget be IsExtensible(target).
    // 16. ReturnIfAbrupt(extensibleTarget).
    // 17. If Desc has a [[Configurable]] field and if Desc.[[Configurable]] is false, then Let settingConfigFalse be
    // true.
    // 18. Else let settingConfigFalse be false.
    bool settingConfigFalse = false;
    if (desc.HasConfigurable() && !desc.IsConfigurable()) {
        settingConfigFalse = true;
    }
    // 19. If targetDesc is undefined, then
    if (!found) {
        // a. If extensibleTarget is false, throw a TypeError exception.
        if (!targetHandle->IsExtensible(thread)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DefineOwnProperty: extensibleTarget is false", false);
        }
        // b. If settingConfigFalse is true, throw a TypeError exception.
        if (settingConfigFalse) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DefineOwnProperty: settingConfigFalse is true", false);
        }
    } else {
        // a. If IsCompatiblePropertyDescriptor(extensibleTarget, Desc , targetDesc) is false, throw a TypeError
        // exception.
        if (!JSObject::IsCompatiblePropertyDescriptor(targetHandle->IsExtensible(thread), desc, targetDesc)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DefineOwnProperty: CompatiblePropertyDescriptor err", false);
        }
        // b. If settingConfigFalse is true and targetDesc.[[Configurable]] is true, throw a TypeError exception.
        if (settingConfigFalse && targetDesc.IsConfigurable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DefineOwnProperty: TypeError of settingConfigFalse", false);
        }
        // c. If IsDataDescriptor(targetDesc) is true, targetDesc.[[Configurable]] is false, and targetDesc.[[Writable]]
        // is true, then If Desc has a [[Writable]] field and Desc.[[Writable]] is false, throw a TypeError exception.
        if (targetDesc.IsDataDescriptor() && !targetDesc.IsConfigurable() && targetDesc.IsWritable() &&
            desc.HasWritable() && !desc.IsWritable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DefineOwnProperty: TypeError of DataDescriptor", false);
        }
        // c. If IsDataDescriptor(targetDesc) is true, targetDesc.[[Configurable]] is false, and targetDesc.[[Writable]]
        // is true, then If Desc has a [[Writable]] field and Desc.[[Writable]] is false, throw a TypeError exception.
        if (targetDesc.IsDataDescriptor() && !targetDesc.IsConfigurable() && targetDesc.IsWritable() &&
            desc.HasWritable() && !desc.IsWritable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "", false);
        }
    }
    // 21. Return true.
    return true;
}

// ES6 9.5.7 [[HasProperty]] (P)
bool JSProxy::HasProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key)
{
    // step 1 ~ 10 are almost same as GetOwnProperty
    ASSERT(JSTaggedValue::IsPropertyKey(key));
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::HasProperty: handler is Null", false);
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledHasString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (trap->IsUndefined()) {
        return JSTaggedValue::HasProperty(thread, targetHandle, key);
    }

    // 9. Let booleanTrapResult be ToBoolean(Call(trap, handler, «target, P»)).
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());

    const size_t argsLength = 2;  // 2: target and key
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(targetHandle.GetTaggedValue(), key.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    bool booleanTrapResult = trapResult.ToBoolean();
    // 10. ReturnIfAbrupt(booleanTrapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 11. If booleanTrapResult is false, then
    if (!booleanTrapResult) {
        // a. Let targetDesc be target.[[GetOwnProperty]](P).
        PropertyDescriptor targetDesc(thread);
        bool found = JSTaggedValue::GetOwnProperty(thread, targetHandle, key, targetDesc);
        // b. ReturnIfAbrupt(targetDesc).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        // c. If targetDesc is not undefined, then
        if (found) {
            // i. If targetDesc.[[Configurable]] is false, throw a TypeError exception.
            if (!targetDesc.IsConfigurable()) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::HasProperty: TypeError of targetDesc", false);
            }
            // ii. Let extensibleTarget be IsExtensible(target).
            // iii. ReturnIfAbrupt(extensibleTarget).
            // iv. If extensibleTarget is false, throw a TypeError exception.
            if (!targetHandle->IsExtensible(thread)) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::HasProperty: extensibleTarget is false", false);
            }
        }
    }
    return booleanTrapResult;
}

// ES6 9.5.8 [[Get]] (P, Receiver)
OperationResult JSProxy::GetProperty(JSThread *thread, const JSHandle<JSProxy> &proxy,
                                     const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    // step 1 ~ 10 are almost same as GetOwnProperty
    ASSERT(JSTaggedValue::IsPropertyKey(key));
    JSTaggedValue handler = proxy->GetHandler();
    JSHandle<JSTaggedValue> exceptionHandle(thread, JSTaggedValue::Exception());
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::GetProperty: handler is Null",
                                    OperationResult(thread, exceptionHandle.GetTaggedValue(), PropertyMetaData(false)));
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledGetString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(
        thread, OperationResult(thread, exceptionHandle.GetTaggedValue(), PropertyMetaData(false)));

    if (trap->IsUndefined()) {
        return JSTaggedValue::GetProperty(thread, targetHandle, key, receiver);
    }
    // 9. Let trapResult be Call(trap, handler, «target, P, Receiver»).
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    const size_t argsLength = 3;  // 3: «target, P, Receiver»
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(targetHandle.GetTaggedValue(), key.GetTaggedValue(), receiver.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);
    JSHandle<JSTaggedValue> resultHandle(thread, trapResult);

    // 10. ReturnIfAbrupt(trapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(
        thread, OperationResult(thread, exceptionHandle.GetTaggedValue(), PropertyMetaData(false)));

    // 11. Let targetDesc be target.[[GetOwnProperty]](P).
    PropertyDescriptor targetDesc(thread);
    bool found = JSTaggedValue::GetOwnProperty(thread, targetHandle, key, targetDesc);
    // 12. ReturnIfAbrupt(targetDesc).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(
        thread, OperationResult(thread, exceptionHandle.GetTaggedValue(), PropertyMetaData(false)));

    // 13. If targetDesc is not undefined, then
    if (found) {
        // a. If IsDataDescriptor(targetDesc) and targetDesc.[[Configurable]] is false and targetDesc.[[Writable]] is
        // false, then
        if (targetDesc.IsDataDescriptor() && !targetDesc.IsConfigurable() && !targetDesc.IsWritable()) {
            // i. If SameValue(trapResult, targetDesc.[[Value]]) is false, throw a TypeError exception.
            if (!JSTaggedValue::SameValue(resultHandle.GetTaggedValue(), targetDesc.GetValue().GetTaggedValue())) {
                THROW_TYPE_ERROR_AND_RETURN(
                    thread, "JSProxy::GetProperty: TypeError of trapResult",
                    OperationResult(thread, exceptionHandle.GetTaggedValue(), PropertyMetaData(false)));
            }
        }
        // b. If IsAccessorDescriptor(targetDesc) and targetDesc.[[Configurable]] is false and targetDesc.[[Get]] is
        // undefined, then
        if (targetDesc.IsAccessorDescriptor() && !targetDesc.IsConfigurable() &&
            targetDesc.GetGetter()->IsUndefined()) {
            // i. If trapResult is not undefined, throw a TypeError exception.
            if (!resultHandle.GetTaggedValue().IsUndefined()) {
                THROW_TYPE_ERROR_AND_RETURN(
                    thread, "JSProxy::GetProperty: trapResult is not undefined",
                    OperationResult(thread, exceptionHandle.GetTaggedValue(), PropertyMetaData(false)));
            }
        }
    }
    // 14. Return trapResult.
    return OperationResult(thread, resultHandle.GetTaggedValue(), PropertyMetaData(true));
}

// ES6 9.5.9 [[Set]] ( P, V, Receiver)
bool JSProxy::SetProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key,
                          const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver, bool mayThrow)
{
    // step 1 ~ 10 are almost same as GetOwnProperty
    ASSERT(JSTaggedValue::IsPropertyKey(key));
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::SetProperty: handler is Null", false);
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledSetString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (trap->IsUndefined()) {
        return JSTaggedValue::SetProperty(thread, targetHandle, key, value, receiver, mayThrow);
    }

    // 9. Let booleanTrapResult be ToBoolean(Call(trap, handler, «target, P, V, Receiver»))
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    const size_t argsLength = 4;  // 4: «target, P, V, Receiver»
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(
        targetHandle.GetTaggedValue(), key.GetTaggedValue(), value.GetTaggedValue(), receiver.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    bool booleanTrapResult = trapResult.ToBoolean();
    // 11. ReturnIfAbrupt(booleanTrapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!booleanTrapResult) {
        return false;
    }
    // 13. Let targetDesc be target.[[GetOwnProperty]](P).
    PropertyDescriptor targetDesc(thread);
    bool found = JSTaggedValue::GetOwnProperty(thread, targetHandle, key, targetDesc);
    // 14. If targetDesc is not undefined, then
    if (found) {
        // a. If IsDataDescriptor(targetDesc) and targetDesc.[[Configurable]] is false and targetDesc.[[Writable]] is
        // false, then
        if (targetDesc.IsDataDescriptor() && !targetDesc.IsConfigurable() && !targetDesc.IsWritable()) {
            // i. If SameValue(trapResult, targetDesc.[[Value]]) is false, throw a TypeError exception.
            if (!JSTaggedValue::SameValue(value, targetDesc.GetValue())) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::SetProperty: TypeError of trapResult", false);
            }
        }
        // b. If IsAccessorDescriptor(targetDesc) and targetDesc.[[Configurable]] is false, then
        // i. If targetDesc.[[Set]] is undefined, throw a TypeError exception.
        if (targetDesc.IsAccessorDescriptor() && !targetDesc.IsConfigurable() &&
            targetDesc.GetSetter()->IsUndefined()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::SetProperty: TypeError of AccessorDescriptor", false);
        }
    }
    return true;
}

// ES6 9.5.10 [[Delete]] (P)
bool JSProxy::DeleteProperty(JSThread *thread, const JSHandle<JSProxy> &proxy, const JSHandle<JSTaggedValue> &key)
{
    // step 1 ~ 13 are almost same as GetOwnProperty
    ASSERT(JSTaggedValue::IsPropertyKey(key));
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DeleteProperty: handler is Null", false);
    }
    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());
    JSHandle<JSTaggedValue> name = thread->GlobalConstants()->GetHandledDeletePropertyString();
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, JSHandle<JSTaggedValue>(thread, handler), name));
    // 7. ReturnIfAbrupt(trap).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (trap->IsUndefined()) {
        return JSTaggedValue::DeleteProperty(thread, targetHandle, key);
    }

    // 9. Let booleanTrapResult be ToBoolean(Call(trap, handler, «target, P»)).
    JSHandle<JSTaggedValue> newTgt(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handlerTag(thread, proxy->GetHandler());
    const size_t argsLength = 2;  // 2: target and key
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerTag, undefined, argsLength);
    info.SetCallArg(targetHandle.GetTaggedValue(), key.GetTaggedValue());
    JSTaggedValue trapResult = JSFunction::Call(&info);

    bool booleanTrapResult = trapResult.ToBoolean();
    // 11. ReturnIfAbrupt(booleanTrapResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!booleanTrapResult) {
        return false;
    }
    // 13. Let targetDesc be target.[[GetOwnProperty]](P).
    PropertyDescriptor targetDesc(thread);
    bool found = JSTaggedValue::GetOwnProperty(thread, targetHandle, key, targetDesc);
    // 14. If targetDesc is undefined, return true.
    if (!found) {
        return true;
    }
    // 15. If targetDesc.[[Configurable]] is false, throw a TypeError exception.
    if (!targetDesc.IsConfigurable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DeleteProperty: targetDesc is not Configurable", false);
    }
    if (!targetHandle->IsExtensible(thread)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "JSProxy::DeleteProperty: targetHandle is not Extensible", false);
    }
    if (!targetHandle->IsExtensible(thread)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "", false);
    }
    // 16. Return true.
    return true;
}

// ES6 9.5.12 [[OwnPropertyKeys]] ()
JSHandle<TaggedArray> JSProxy::OwnPropertyKeys(JSThread *thread, const JSHandle<JSProxy> &proxy)
{
    // step 1 ~ 4 get ProxyHandler and ProxyTarget
    JSTaggedValue handler = proxy->GetHandler();
    if (handler.IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "OwnPropertyKeys: handler is null",
                                    JSHandle<TaggedArray>(thread, JSTaggedValue::Exception()));
    }

    ASSERT(handler.IsECMAObject());
    JSHandle<JSTaggedValue> targetHandle(thread, proxy->GetTarget());

    // 5.Let trap be GetMethod(handler, "ownKeys").
    JSHandle<JSTaggedValue> key = thread->GlobalConstants()->GetHandledOwnKeysString();
    JSHandle<JSTaggedValue> handlerHandle(thread, handler);
    JSHandle<JSTaggedValue> trap(JSObject::GetMethod(thread, handlerHandle, key));

    // 6.ReturnIfAbrupt(trap).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

    // 7.If trap is undefined, then
    //    a.Return target.[[OwnPropertyKeys]]().
    if (trap->IsUndefined()) {
        return JSTaggedValue::GetOwnPropertyKeys(thread, targetHandle);
    }

    // 8.Let trapResultArray be Call(trap, handler, «target»).
    JSHandle<JSFunction> tagFunc(targetHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, trap, handlerHandle, undefined, 1);
    info.SetCallArg(targetHandle.GetTaggedValue());
    JSTaggedValue res = JSFunction::Call(&info);
    JSHandle<JSTaggedValue> trap_res_arr(thread, res);

    // 9.Let trapResult be CreateListFromArrayLike(trapResultArray, «String, Symbol»).
    // 10.ReturnIfAbrupt(trapResult)
    // If trapResult contains any duplicate entries, throw a TypeError exception.
    JSHandle<TaggedArray> trapRes(
        JSObject::CreateListFromArrayLike<ElementTypes::STRING_AND_SYMBOL>(thread, trap_res_arr));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

    if (trapRes->HasDuplicateEntry()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "OwnPropertyKeys: contains duplicate entries",
                                    JSHandle<TaggedArray>(thread, JSTaggedValue::Exception()));
    }

    // 11.Let extensibleTarget be IsExtensible(target).
    bool extensibleTarget = targetHandle->IsExtensible(thread);

    // 12.ReturnIfAbrupt(extensibleTarget).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

    // 13.Let targetKeys be target.[[OwnPropertyKeys]]().
    JSHandle<TaggedArray> targetKeys = JSTaggedValue::GetOwnPropertyKeys(thread, targetHandle);

    // 14.ReturnIfAbrupt(targetKeys).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

    // 15.Assert: targetKeys is a List containing only String and Symbol values.
    // 16.Let targetConfigurableKeys be an empty List.
    // 17.Let targetNonconfigurableKeys be an empty List.
    // 18.Repeat, for each element key of targetKeys,
    //     a.Let desc be target.[[GetOwnProperty]](key).
    //     b.ReturnIfAbrupt(desc).
    //     c.If desc is not undefined and desc.[[Configurable]] is false, then
    //        i.Append key as an element of targetNonconfigurableKeys.
    //     d.Else,
    //        i.Append key as an element of targetConfigurableKeys.
    uint32_t length = targetKeys->GetLength();
    JSHandle<TaggedArray> tgtCfigKeys = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length);
    JSHandle<TaggedArray> tgtNoCfigKeys = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length);

    uint32_t cfigLength = 0;
    uint32_t noCfigLength = 0;
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> targetKey(thread, targetKeys->Get(i));
        ASSERT(targetKey->IsStringOrSymbol());

        PropertyDescriptor desc(thread);
        JSTaggedValue::GetOwnProperty(thread, targetHandle, targetKey, desc);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

        if (!desc.IsEmpty() && !desc.IsConfigurable()) {
            tgtNoCfigKeys->Set(thread, noCfigLength, targetKey);
            noCfigLength++;
        } else {
            tgtCfigKeys->Set(thread, cfigLength, targetKey);
            cfigLength++;
        }
    }

    // 19.If extensibleTarget is true and targetNonconfigurableKeys is empty, then
    //     a.Return trapResult.
    if (extensibleTarget && (cfigLength == 0)) {
        return trapRes;
    }

    // 20.Let uncheckedResultKeys be a new List which is a copy of trapResult.
    JSHandle<TaggedArray> uncheckFesKeys =
        thread->GetEcmaVM()->GetFactory()->CopyArray(trapRes, trapRes->GetLength(), trapRes->GetLength());
    uint32_t uncheckLength = uncheckFesKeys->GetLength();

    // 21.Repeat, for each key that is an element of targetNonconfigurableKeys,
    //     a.If key is not an element of uncheckedResultKeys, throw a TypeError exception.
    //     b.Remove key from uncheckedResultKeys
    for (uint32_t i = 0; i < noCfigLength; i++) {
        uint32_t idx = uncheckFesKeys->GetIdx(tgtNoCfigKeys->Get(i));
        if (idx == TaggedArray::MAX_ARRAY_INDEX) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "OwnPropertyKeys: key is not an element of uncheckedResultKeys",
                                        JSHandle<TaggedArray>(thread, JSTaggedValue::Exception()));
        }
        uncheckFesKeys->Set(thread, idx, JSTaggedValue::Hole());
        uncheckLength--;
    }

    // 22.If extensibleTarget is true, return trapResult.
    if (extensibleTarget) {
        return trapRes;
    }

    // 23.Repeat, for each key that is an element of targetConfigurableKeys,
    //     a.If key is not an element of uncheckedResultKeys, throw a TypeError exception.
    //     b.Remove key from uncheckedResultKeys
    for (uint32_t i = 0; i < cfigLength; i++) {
        uint32_t idx = uncheckFesKeys->GetIdx(tgtCfigKeys->Get(i));
        if (idx == TaggedArray::MAX_ARRAY_INDEX) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "OwnPropertyKeys: key is not an element of uncheckedResultKeys",
                                        JSHandle<TaggedArray>(thread, JSTaggedValue::Exception()));
        }
        uncheckFesKeys->Set(thread, idx, JSTaggedValue::Hole());
        uncheckLength--;
    }

    // 24.If uncheckedResultKeys is not empty, throw a TypeError exception.
    if (uncheckLength != 0) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "OwnPropertyKeys: uncheckedResultKeys is not empty",
                                    JSHandle<TaggedArray>(thread, JSTaggedValue::Exception()));
    }

    // 25.Return trapResult.
    return trapRes;
}

// ES6 9.5.13 [[Call]] (thisArgument, argumentsList)
JSTaggedValue JSProxy::CallInternal(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    JSHandle<JSProxy> proxy(info->GetFunction());
    // step 1 ~ 4 get ProxyHandler and ProxyTarget
    JSHandle<JSTaggedValue> handler(thread, proxy->GetHandler());
    if (handler->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Call: handler is null", JSTaggedValue::Exception());
    }
    ASSERT(handler->IsECMAObject());
    JSHandle<JSTaggedValue> target(thread, proxy->GetTarget());

    // 5.Let trap be GetMethod(handler, "apply").
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledApplyString());
    JSHandle<JSTaggedValue> method = JSObject::GetMethod(thread, handler, key);

    // 6.ReturnIfAbrupt(trap).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7.If trap is undefined, then
    //   a.Return Call(target, thisArgument, argumentsList).
    if (method->IsUndefined()) {
        info->SetFunction(target.GetTaggedValue());
        return JSFunction::Call(info);
    }
    // 8.Let argArray be CreateArrayFromList(argumentsList).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t argc = info->GetArgsNumber();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(argc);
    for (size_t index = 0; index < argc; ++index) {
        taggedArray->Set(thread, index, info->GetCallArg(index));
    }
    JSHandle<JSArray> arrHandle = JSArray::CreateArrayFromList(thread, taggedArray);

    // 9.Return Call(trap, handler, «target, thisArgument, argArray»).
    JSHandle<JSTaggedValue> thisArg = info->GetThis();
    const size_t argsLength = 3;  // 3: «target, thisArgument, argArray»
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, method, handler, undefined, argsLength);
    runtimeInfo.SetCallArg(target.GetTaggedValue(), thisArg.GetTaggedValue(), arrHandle.GetTaggedValue());
    return JSFunction::Call(&runtimeInfo);
}

// ES6 9.5.14 [[Construct]] ( argumentsList, newTarget)
JSTaggedValue JSProxy::ConstructInternal(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    // step 1 ~ 4 get ProxyHandler and ProxyTarget
    JSHandle<JSProxy> proxy(info->GetFunction());
    JSHandle<JSTaggedValue> handler(thread, proxy->GetHandler());
    if (handler->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor: handler is null", JSTaggedValue::Exception());
    }
    ASSERT(handler->IsECMAObject());
    JSHandle<JSTaggedValue> target(thread, proxy->GetTarget());

    // 5.Let trap be GetMethod(handler, "construct").
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledProxyConstructString());
    JSHandle<JSTaggedValue> method = JSObject::GetMethod(thread, handler, key);

    // 6.ReturnIfAbrupt(trap).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7.If trap is undefined, then
    //   a.Assert: target has a [[Construct]] internal method.
    //   b.Return Construct(target, argumentsList, newTarget).
    if (method->IsUndefined()) {
        ASSERT(target->IsConstructor());
        info->SetFunction(target.GetTaggedValue());
        return JSFunction::Construct(info);
    }

    // 8.Let argArray be CreateArrayFromList(argumentsList).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t argc = info->GetArgsNumber();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(argc);
    for (size_t index = 0; index < argc; ++index) {
        taggedArray->Set(thread, index, info->GetCallArg(index));
    }
    JSHandle<JSArray> arrHandle = JSArray::CreateArrayFromList(thread, taggedArray);

    // step 8 ~ 9 Call(trap, handler, «target, argArray, newTarget »).
    JSHandle<JSTaggedValue> newTarget(info->GetNewTarget());
    const size_t argsLength = 3;  // 3: «target, argArray, newTarget »
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, method, handler, undefined, argsLength);
    runtimeInfo.SetCallArg(target.GetTaggedValue(), arrHandle.GetTaggedValue(), newTarget.GetTaggedValue());
    JSTaggedValue newObj = JSFunction::Call(&runtimeInfo);

    // 10.ReturnIfAbrupt(newObj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11.If Type(newObj) is not Object, throw a TypeError exception.
    if (!newObj.IsObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new object is not object", JSTaggedValue::Exception());
    }
    // 12.Return newObj.
    return newObj;
}

bool JSProxy::IsArray(JSThread *thread) const
{
    if (GetHandler().IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "", false);
    }
    return GetTarget().IsArray(thread);
}

JSHandle<JSTaggedValue> JSProxy::GetSourceTarget(JSThread *thread) const
{
    JSMutableHandle<JSProxy> proxy(thread, JSTaggedValue(this));
    JSMutableHandle<JSTaggedValue> target(thread, proxy->GetTarget());
    while (target->IsJSProxy()) {
        proxy.Update(target.GetTaggedValue());
        target.Update(proxy->GetTarget());
    }
    return target;
}
}  // namespace panda::ecmascript
