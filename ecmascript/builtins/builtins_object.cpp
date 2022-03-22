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

#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
// 19.1.1.1 Object ( [ value ] )
JSTaggedValue BuiltinsObject::ObjectConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();

    // 1.If NewTarget is neither undefined nor the active function, then
    //    a.Return OrdinaryCreateFromConstructor(NewTarget, "%ObjectPrototype%").
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (!newTarget->IsUndefined() && !(newTarget.GetTaggedValue() == constructor.GetTaggedValue())) {
        JSHandle<JSObject> obj =
            ecmaVm->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
        return obj.GetTaggedValue();
    }

    // 2.If value is null, undefined or not supplied, return ObjectCreate(%ObjectPrototype%).
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (value->IsNull() || value->IsUndefined()) {
        JSHandle<JSObject> obj = ecmaVm->GetFactory()->OrdinaryNewJSObjectCreate(env->GetObjectFunctionPrototype());
        return obj.GetTaggedValue();
    }

    // 3.Return ToObject(value).
    return JSTaggedValue::ToObject(thread, value).GetTaggedValue();
}

// 19.1.2.1 Object.assign ( target, ...sources )
JSTaggedValue BuiltinsObject::Assign(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Assign);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t numArgs = argv->GetArgsNumber();
    // 1.Let to be ToObject(target).
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    JSHandle<JSObject> toAssign = JSTaggedValue::ToObject(thread, target);
    // 2.ReturnIfAbrupt(to).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3.If only one argument was passed, return to.
    // 4.Let sources be the List of argument values starting with the second argument.
    // 5.For each element nextSource of sources, in ascending index order
    //   a.If nextSource is undefined or null, let keys be an empty List.
    //   b.Else,
    //     i.Let from be ToObject(nextSource).
    //     ii.Let keys be from.[[OwnPropertyKeys]]().
    //     iii.ReturnIfAbrupt(keys).
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 1; i < numArgs; i++) {
        JSHandle<JSTaggedValue> source = GetCallArg(argv, i);
        if (!source->IsNull() && !source->IsUndefined()) {
            JSHandle<JSObject> from = JSTaggedValue::ToObject(thread, source);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, JSHandle<JSTaggedValue>::Cast(from));
            // ReturnIfAbrupt(keys)
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            // c.Repeat for each element nextKey of keys in List order,
            //    i.Let desc be from.[[GetOwnProperty]](nextKey).
            //    ii.ReturnIfAbrupt(desc).
            //    iii.if desc is not undefined and desc.[[Enumerable]] is true, then
            //      1.Let propValue be Get(from, nextKey).
            //      2.ReturnIfAbrupt(propValue).
            //      3.Let status be Set(to, nextKey, propValue, true).
            //      4.ReturnIfAbrupt(status).
            uint32_t keysLen = keys->GetLength();
            for (uint32_t j = 0; j < keysLen; j++) {
                PropertyDescriptor desc(thread);
                key.Update(keys->Get(j));
                bool success = JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(from), key, desc);
                // ReturnIfAbrupt(desc)
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

                if (success && desc.IsEnumerable()) {
                    JSTaggedValue value =
                        FastRuntimeStub::FastGetPropertyByValue(thread, from.GetTaggedValue(), key.GetTaggedValue());
                    // ReturnIfAbrupt(prop_value)
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

                    FastRuntimeStub::FastSetPropertyByValue(thread, toAssign.GetTaggedValue(), key.GetTaggedValue(),
                                                            value);
                    //  ReturnIfAbrupt(status)
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                }
            }
        }
    }

    // 6.Return to.
    return toAssign.GetTaggedValue();
}

// Runtime Semantics
JSTaggedValue BuiltinsObject::ObjectDefineProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                     const JSHandle<JSTaggedValue> &prop)
{
    BUILTINS_API_TRACE(thread, Object, DefineProperties);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1.If Type(O) is not Object, throw a TypeError exception.
    if (!obj->IsECMAObject()) {
        // throw a TypeError exception
        THROW_TYPE_ERROR_AND_RETURN(thread, "is not an object", JSTaggedValue::Exception());
    }

    // 2.Let props be ToObject(Properties).
    JSHandle<JSObject> props = JSTaggedValue::ToObject(thread, prop);

    // 3.ReturnIfAbrupt(props).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4.Let keys be props.[[OwnPropertyKeys]]().
    JSHandle<TaggedArray> handleKeys = JSTaggedValue::GetOwnPropertyKeys(thread, JSHandle<JSTaggedValue>::Cast(props));

    // 5.ReturnIfAbrupt(keys).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6.Let descriptors be an empty List.
    // new an empty array and append
    uint32_t length = handleKeys->GetLength();
    [[maybe_unused]] JSHandle<TaggedArray> descriptors =
        factory->NewTaggedArray(2 * length);  // 2: 2 means two element list

    // 7.Repeat for each element nextKey of keys in List order,
    //   a.Let propDesc be props.[[GetOwnProperty]](nextKey).
    //   b.ReturnIfAbrupt(propDesc).
    //   c.If propDesc is not undefined and propDesc.[[Enumerable]] is true, then
    //     i.Let descObj be Get( props, nextKey).
    //     ii.ReturnIfAbrupt(descObj).
    //     iii.Let desc be ToPropertyDescriptor(descObj).
    //     iv.ReturnIfAbrupt(desc).
    //     v.Append the pair (a two element List) consisting of nextKey and desc to the end of descriptors.
    JSMutableHandle<JSTaggedValue> handleKey(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; i++) {
        PropertyDescriptor propDesc(thread);
        handleKey.Update(handleKeys->Get(i));

        bool success = JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(props), handleKey, propDesc);
        // ReturnIfAbrupt(propDesc)
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        if (success && propDesc.IsEnumerable()) {
            JSHandle<JSTaggedValue> descObj =
                JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(props), handleKey).GetValue();
            // ReturnIfAbrupt(descObj)
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            PropertyDescriptor desc(thread);
            JSObject::ToPropertyDescriptor(thread, descObj, desc);

            // ReturnIfAbrupt(desc)
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            // 8.For each pair from descriptors in list order,
            //   a.Let P be the first element of pair.
            //   b.Let desc be the second element of pair.
            //   c.Let status be DefinePropertyOrThrow(O,P, desc).
            //   d.ReturnIfAbrupt(status).
            [[maybe_unused]] bool setSuccess = JSTaggedValue::DefinePropertyOrThrow(thread, obj, handleKey, desc);

            // ReturnIfAbrupt(status)
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }

    // 9.Return O.
    return obj.GetTaggedValue();
}

// 19.1.2.2 Object.create ( O [ , Properties ] )
JSTaggedValue BuiltinsObject::Create(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Create);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.If Type(O) is neither Object nor Null, throw a TypeError exception.
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsECMAObject() && !obj->IsNull()) {
        // throw a TypeError exception
        THROW_TYPE_ERROR_AND_RETURN(thread, "Create: O is neither Object nor Null", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> properties = GetCallArg(argv, 1);

    // 2.Let obj be ObjectCreate(O).
    JSHandle<JSObject> objCreate = thread->GetEcmaVM()->GetFactory()->OrdinaryNewJSObjectCreate(obj);

    // 3.If the argument Properties is present and not undefined, then
    //   a.Return ObjectDefineProperties(obj, Properties).
    if (!properties->IsUndefined()) {
        return ObjectDefineProperties(thread, JSHandle<JSTaggedValue>::Cast(objCreate), properties);
    }

    // 4.Return obj.
    return objCreate.GetTaggedValue();
}

// 19.1.2.3 Object.defineProperties ( O, Properties )
JSTaggedValue BuiltinsObject::DefineProperties(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, DefineProperties);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.Return ObjectDefineProperties(O, Properties).
    return ObjectDefineProperties(thread, GetCallArg(argv, 0), GetCallArg(argv, 1));
}

// 19.1.2.4 Object.defineProperty ( O, P, Attributes )
JSTaggedValue BuiltinsObject::DefineProperty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, DefineProperty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1.If Type(O) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsECMAObject()) {
        // throw a TypeError
        THROW_TYPE_ERROR_AND_RETURN(thread, "DefineProperty: O is not Object", JSTaggedValue::Exception());
    }

    // 2.Let key be ToPropertyKey(P).
    JSHandle<JSTaggedValue> prop = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, prop);

    // 3.ReturnIfAbrupt(key).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4.Let desc be ToPropertyDescriptor(Attributes).
    PropertyDescriptor desc(thread);
    JSObject::ToPropertyDescriptor(thread, GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD), desc);

    // 5.ReturnIfAbrupt(desc).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6.Let success be DefinePropertyOrThrow(O,key, desc).
    [[maybe_unused]] bool success = JSTaggedValue::DefinePropertyOrThrow(thread, obj, key, desc);

    // 7.ReturnIfAbrupt(success).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8.Return O.
    return obj.GetTaggedValue();
}

// 19.1.2.5 Object.freeze ( O )
JSTaggedValue BuiltinsObject::Freeze(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Freeze);

    // 1.If Type(O) is not Object, return O.
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsECMAObject()) {
        return obj.GetTaggedValue();
    }

    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 2.Let status be SetIntegrityLevel( O, "frozen").
    bool status = JSObject::SetIntegrityLevel(thread, JSHandle<JSObject>(obj), IntegrityLevel::FROZEN);

    // 3.ReturnIfAbrupt(status).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4.If status is false, throw a TypeError exception.
    if (!status) {
        // throw a TypeError exception
        THROW_TYPE_ERROR_AND_RETURN(thread, "Freeze: freeze failed", JSTaggedValue::Exception());
    }

    // 5.Return O.
    return obj.GetTaggedValue();
}

// 19.1.2.6 Object.getOwnPropertyDescriptor ( O, P )
JSTaggedValue BuiltinsObject::GetOwnPropertyDescriptor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, GetOwnPropertyDescriptor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1.Let obj be ToObject(O).
    JSHandle<JSTaggedValue> func = GetCallArg(argv, 0);
    JSHandle<JSObject> handle = JSTaggedValue::ToObject(thread, func);

    // 2.ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3.Let key be ToPropertyKey(P).
    JSHandle<JSTaggedValue> prop = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, prop);

    // 4.ReturnIfAbrupt(key).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5.Let desc be obj.[[GetOwnProperty]](key).
    PropertyDescriptor desc(thread);
    JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(handle), key, desc);

    // 6.ReturnIfAbrupt(desc).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7.Return FromPropertyDescriptor(desc).
    JSHandle<JSTaggedValue> res = JSObject::FromPropertyDescriptor(thread, desc);
    return res.GetTaggedValue();
}

// Runtime Semantics
JSTaggedValue BuiltinsObject::GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &object,
                                                 const KeyType &type)
{
    BUILTINS_API_TRACE(thread, Object, GetOwnPropertyKeys);
    // 1.Let obj be ToObject(O).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, object);

    // 2.ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3.Let keys be obj.[[OwnPropertyKeys]]().
    JSHandle<TaggedArray> handleKeys = JSTaggedValue::GetOwnPropertyKeys(thread, JSHandle<JSTaggedValue>::Cast(obj));

    // 4.ReturnIfAbrupt(keys).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5.Let nameList be a new empty List.
    // new an empty array and append
    uint32_t length = handleKeys->GetLength();
    JSHandle<TaggedArray> nameList = factory->NewTaggedArray(length);

    // 6.Repeat for each element nextKey of keys in List order,
    uint32_t copyLength = 0;
    switch (type) {
        case KeyType::STRING_TYPE: {
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue key = handleKeys->Get(i);
                if (key.IsString()) {
                    nameList->Set(thread, copyLength, key);
                    copyLength++;
                }
            }
            break;
        }
        case KeyType::SYMBOL_TYPE: {
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue key = handleKeys->Get(i);
                if (key.IsSymbol()) {
                    nameList->Set(thread, copyLength, key);
                    copyLength++;
                }
            }
            break;
        }
        default:
            break;
    }

    // 7.Return CreateArrayFromList(nameList).
    JSHandle<TaggedArray> resultList = factory->CopyArray(nameList, length, copyLength);
    JSHandle<JSArray> resultArray = JSArray::CreateArrayFromList(thread, resultList);
    return resultArray.GetTaggedValue();
}

// 19.1.2.7 Object.getOwnPropertyNames ( O )
JSTaggedValue BuiltinsObject::GetOwnPropertyNames(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, GetOwnPropertyNames);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    KeyType type = KeyType::STRING_TYPE;

    // 1.Return GetOwnPropertyKeys(O, String).
    return GetOwnPropertyKeys(argv->GetThread(), obj, type);
}

// 19.1.2.8 Object.getOwnPropertySymbols ( O )
JSTaggedValue BuiltinsObject::GetOwnPropertySymbols(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, GetOwnPropertySymbols);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    KeyType type = KeyType::SYMBOL_TYPE;

    // 1.Return GetOwnPropertyKeys(O, Symbol).
    return GetOwnPropertyKeys(argv->GetThread(), obj, type);
}

// 19.1.2.9 Object.getPrototypeOf ( O )
JSTaggedValue BuiltinsObject::GetPrototypeOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, GetPrototypeOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1.Let obj be ToObject(O).
    JSHandle<JSTaggedValue> func = GetCallArg(argv, 0);

    JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, func);

    // 2.ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3.Return obj.[[GetPrototypeOf]]().
    return obj->GetPrototype(thread);
}

// 19.1.2.10 Object.is ( value1, value2 )
JSTaggedValue BuiltinsObject::Is(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Is);

    // 1.Return SameValue(value1, value2).
    bool result = JSTaggedValue::SameValue(GetCallArg(argv, 0), GetCallArg(argv, 1));
    return GetTaggedBoolean(result);
}

// 19.1.2.11 Object.isExtensible ( O )
JSTaggedValue BuiltinsObject::IsExtensible(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    // 1.If Type(O) is not Object, return false.
    JSTaggedValue obj = GetCallArg(argv, 0).GetTaggedValue();
    if (!obj.IsObject()) {
        return GetTaggedBoolean(false);
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 2.Return IsExtensible(O).
    return GetTaggedBoolean(obj.IsExtensible(thread));
}

// 19.1.2.12 Object.isFrozen ( O )
JSTaggedValue BuiltinsObject::IsFrozen(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    // 1.If Type(O) is not Object, return true.
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsECMAObject()) {
        return GetTaggedBoolean(true);
    }

    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 2.Return TestIntegrityLevel(O, "frozen").
    bool status = JSObject::TestIntegrityLevel(thread, JSHandle<JSObject>(obj), IntegrityLevel::FROZEN);
    return GetTaggedBoolean(status);
}

// 19.1.2.13 Object.isSealed ( O )
JSTaggedValue BuiltinsObject::IsSealed(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);

    // 1.If Type(O) is not Object, return true.
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsECMAObject()) {
        return GetTaggedBoolean(true);
    }

    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 2.Return TestIntegrityLevel(O, "sealed").
    bool status = JSObject::TestIntegrityLevel(thread, JSHandle<JSObject>(obj), IntegrityLevel::SEALED);
    return GetTaggedBoolean(status);
}

// 19.1.2.14 Object.keys(O)
JSTaggedValue BuiltinsObject::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let obj be ToObject(O).
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);

    JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, msg);

    // 2. ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let nameList be EnumerableOwnNames(obj).
    JSHandle<TaggedArray> nameList = JSObject::EnumerableOwnNames(thread, obj);

    // 4. ReturnIfAbrupt(nameList).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return CreateArrayFromList(nameList).
    JSHandle<JSArray> result = JSArray::CreateArrayFromList(thread, nameList);
    return result.GetTaggedValue();
}

// 19.1.2.15 Object.preventExtensions(O)
JSTaggedValue BuiltinsObject::PreventExtensions(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, PreventExtensions);
    // 1. If Type(O) is not Object, return O.
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsECMAObject()) {
        return obj.GetTaggedValue();
    }
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    // 2. Let status be O.[[PreventExtensions]]().
    bool status = JSTaggedValue::PreventExtensions(argv->GetThread(), obj);

    // 3. ReturnIfAbrupt(status).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());

    // 4. If status is false, throw a TypeError exception.
    if (!status) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "PreventExtensions: preventExtensions failed",
                                    JSTaggedValue::Exception());
    }

    // 5. Return O.
    return obj.GetTaggedValue();
}
// 19.1.2.16 Object.prototype

// 19.1.2.17 Object.seal(O)
JSTaggedValue BuiltinsObject::Seal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, Seal);

    // 1. If Type(O) is not Object, return O.
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    if (!msg->IsECMAObject()) {
        return msg.GetTaggedValue();
    }

    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 2. Let status be SetIntegrityLevel(O, "sealed").
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, msg);
    bool status = JSObject::SetIntegrityLevel(thread, object, IntegrityLevel::SEALED);

    // 3. ReturnIfAbrupt(status).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. If status is false, throw a TypeError exception.
    if (!status) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "Seal: seal failed", JSTaggedValue::Exception());
    }

    // 5. Return O.
    return object.GetTaggedValue();
}

// 19.1.2.18 Object.setPrototypeOf(O, proto)
JSTaggedValue BuiltinsObject::SetPrototypeOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, SetPrototypeOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be RequireObjectCoercible(O).
    JSHandle<JSTaggedValue> object = JSTaggedValue::RequireObjectCoercible(thread, GetCallArg(argv, 0));

    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If Type(proto) is neither Object nor Null, throw a TypeError exception.
    JSHandle<JSTaggedValue> proto = GetCallArg(argv, 1);
    if (!proto->IsNull() && !proto->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "SetPrototypeOf: proto is neither Object nor Null",
                                    JSTaggedValue::Exception());
    }

    // 4. If Type(O) is not Object, return O.
    if (!object->IsECMAObject()) {
        return object.GetTaggedValue();
    }

    // 5. Let status be O.[[SetPrototypeOf]](proto).
    bool status = JSTaggedValue::SetPrototype(thread, object, proto);

    // 6. ReturnIfAbrupt(status).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. If status is false, throw a TypeError exception.
    if (!status) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "SetPrototypeOf: prototype set failed", JSTaggedValue::Exception());
    }

    // 8. Return O.
    return object.GetTaggedValue();
}

// 19.1.3.1 Object.prototype.constructor

// 19.1.3.2 Object.prototype.hasOwnProperty(V)
JSTaggedValue BuiltinsObject::HasOwnProperty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, HasOwnProperty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let P be ToPropertyKey(V).
    JSHandle<JSTaggedValue> prop = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> property = JSTaggedValue::ToPropertyKey(thread, prop);

    // 2. ReturnIfAbrupt(P).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let O be ToObject(this value).
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, GetThis(argv));

    // 4. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Return HasOwnProperty(O, P).
    bool res = JSTaggedValue::HasOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(object), property);
    return GetTaggedBoolean(res);
}

// 19.1.3.3 Object.prototype.isPrototypeOf(V)
JSTaggedValue BuiltinsObject::IsPrototypeOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, IsPrototypeOf);
    JSThread *thread = argv->GetThread();
    // 1. If Type(V) is not Object, return false.
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    if (!msg->IsECMAObject()) {
        return GetTaggedBoolean(false);
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 2. Let O be ToObject(this value).
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, GetThis(argv));
    // 3. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. Repeat
    //    a. Let V be V.[[GetPrototypeOf]]().
    //    b. If V is null, return false
    //    c. If SameValue(O, V) is true, return true.
    JSTaggedValue msgValue = msg.GetTaggedValue();
    while (!msgValue.IsNull()) {
        if (JSTaggedValue::SameValue(object.GetTaggedValue(), msgValue)) {
            return GetTaggedBoolean(true);
        }
        msgValue = JSObject::Cast(msgValue)->GetPrototype(thread);
    }
    return GetTaggedBoolean(false);
}

// 19.1.3.4 Object.prototype.propertyIsEnumerable(V)
JSTaggedValue BuiltinsObject::PropertyIsEnumerable(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    // 1. Let P be ToPropertyKey(V).
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> property = JSTaggedValue::ToPropertyKey(thread, msg);

    // 2. ReturnIfAbrupt(P).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let O be ToObject(this value).
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, GetThis(argv));
    // 4. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let desc be O.[[GetOwnProperty]](P).
    PropertyDescriptor desc(thread);
    JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(object), property, desc);

    // 6. ReturnIfAbrupt(desc).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. If desc is undefined, return false.
    if (desc.IsEmpty()) {
        return GetTaggedBoolean(false);
    }

    // 8. Return the value of desc.[[Enumerable]].
    return GetTaggedBoolean(desc.IsEnumerable());
}

// 19.1.3.5 Object.prototype.toLocaleString([reserved1[, reserved2]])
JSTaggedValue BuiltinsObject::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, ToLocaleString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> object = GetThis(argv);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());

    // 2. Return Invoke(O, "toString").
    JSHandle<JSTaggedValue> calleeKey = thread->GlobalConstants()->GetHandledToStringString();

    JSHandle<TaggedArray> argsList = GetArgsArray(argv);
    ecmascript::InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgList(*argsList);
    return JSFunction::Invoke(thread, object, calleeKey, argsList->GetLength(), arguments->GetArgv());
}

JSTaggedValue BuiltinsObject::GetBuiltinTag(JSThread *thread, const JSHandle<JSObject> &object)
{
    BUILTINS_API_TRACE(thread, Object, GetBuiltinTag);
    // 4. Let isArray be IsArray(O).
    bool isArray = object->IsJSArray();
    // 5. ReturnIfAbrupt(isArray).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> builtinTag = factory->NewFromCanBeCompressString("Object");
    // 6. If isArray is true, let builtinTag be "Array".
    if (isArray) {
        builtinTag = factory->NewFromCanBeCompressString("Array");
    } else if (object->IsJSPrimitiveRef()) {
        // 7. Else, if O is an exotic String object, let builtinTag be "String".
        JSPrimitiveRef *primitiveRef = JSPrimitiveRef::Cast(*object);
        if (primitiveRef->IsString()) {
            builtinTag = factory->NewFromCanBeCompressString("String");
        } else if (primitiveRef->IsBoolean()) {
            // 11. Else, if O has a [[BooleanData]] internal slot, let builtinTag be "Boolean".
            builtinTag = factory->NewFromCanBeCompressString("Boolean");
        } else if (primitiveRef->IsNumber()) {
            // 12. Else, if O has a [[NumberData]] internal slot, let builtinTag be "Number".
            builtinTag = factory->NewFromCanBeCompressString("Number");
        }
    } else if (object->IsArguments()) {
        builtinTag = factory->NewFromCanBeCompressString("Arguments");
    } else if (object->IsCallable()) {
        builtinTag = factory->NewFromCanBeCompressString("Function");
    } else if (object->IsJSError()) {
        builtinTag = factory->NewFromCanBeCompressString("Error");
    } else if (object->IsDate()) {
        builtinTag = factory->NewFromCanBeCompressString("Date");
    } else if (object->IsJSRegExp()) {
        builtinTag = factory->NewFromCanBeCompressString("RegExp");
    }
    // 15. Else, let builtinTag be "Object".
    return builtinTag.GetTaggedValue();
}

// 19.1.3.6 Object.prototype.toString()
JSTaggedValue BuiltinsObject::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If the this value is undefined, return "[object Undefined]".

    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (msg->IsUndefined()) {
        return GetTaggedString(thread, "[object Undefined]");
    }
    // 2. If the this value is null, return "[object Null]".
    if (msg->IsNull()) {
        return GetTaggedString(thread, "[object Null]");
    }

    // 3. Let O be ToObject(this value).
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> builtinTag(thread, GetBuiltinTag(thread, object));

    // 16. Let tag be Get (O, @@toStringTag).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    auto factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> tag = JSTaggedValue::GetProperty(thread, msg, env->GetToStringTagSymbol()).GetValue();

    // 17. ReturnIfAbrupt(tag).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 18. If Type(tag) is not String, let tag be builtinTag.
    if (!tag->IsString()) {
        tag = builtinTag;
    }

    // 19. Return the String that is the result of concatenating "[object ", tag, and "]".
    JSHandle<EcmaString> leftString(factory->NewFromCanBeCompressString("[object "));
    JSHandle<EcmaString> rightString(factory->NewFromCanBeCompressString("]"));

    JSHandle<EcmaString> newLeftStringHandle =
        factory->ConcatFromString(leftString, JSTaggedValue::ToString(thread, tag));
    auto result = factory->ConcatFromString(newLeftStringHandle, rightString);
    return result.GetTaggedValue();
}

// 19.1.3.7 Object.prototype.valueOf()
JSTaggedValue BuiltinsObject::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, ValueOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Return ToObject(this value).
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return object.GetTaggedValue();
}
// B.2.2.1 Object.prototype.__proto__
JSTaggedValue BuiltinsObject::ProtoGetter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, ProtoGetter);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1.Let obj be ToObject(this value).
    JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, GetThis(argv));

    // 2.ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3.Return obj.[[GetPrototypeOf]]().
    return obj->GetPrototype(thread);
}

JSTaggedValue BuiltinsObject::ProtoSetter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, ProtoSetter);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> obj = JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv));

    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If Type(proto) is neither Object nor Null, return undefined..
    JSHandle<JSTaggedValue> proto = GetCallArg(argv, 0);
    if (!proto->IsNull() && !proto->IsECMAObject()) {
        return JSTaggedValue::Undefined();
    }

    // 4. If Type(O) is not Object, return undefined.
    if (!obj->IsECMAObject()) {
        return JSTaggedValue::Undefined();
    }

    // 5. Let status be O.[[SetPrototypeOf]](proto).
    bool status = JSTaggedValue::SetPrototype(thread, obj, proto);

    // 6. ReturnIfAbrupt(status).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. If status is false, throw a TypeError exception.
    if (!status) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "ProtoSetter: proto set failed", JSTaggedValue::Exception());
    }

    // 8. Return O.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsObject::CreateRealm(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSRealm> realm = factory->NewJSRealm();
    return realm.GetTaggedValue();
}

JSTaggedValue BuiltinsObject::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Object, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let obj be ? ToObject(O).
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let nameList be ? EnumerableOwnPropertyNames(obj, key+value).
    JSHandle<TaggedArray> nameList = JSObject::EnumerableOwnPropertyNames(thread, object, PropertyKind::KEY_VALUE);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayFromList(nameList).
    return JSArray::CreateArrayFromList(thread, nameList).GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
