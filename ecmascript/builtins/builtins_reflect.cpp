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

#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript::builtins {
// ecma 26.1.1 Reflect.apply (target, thisArgument, argumentsList)
JSTaggedValue BuiltinsReflect::ReflectApply(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, Apply);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If IsCallable(target) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.apply target is not callable", JSTaggedValue::Exception());
    }
    // 2. Let args be ? CreateListFromArrayLike(argumentsList).
    JSHandle<JSTaggedValue> thisArgument = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> argumentsList = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    JSHandle<JSTaggedValue> argOrAbrupt = JSObject::CreateListFromArrayLike(thread, argumentsList);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> args = JSHandle<TaggedArray>::Cast(argOrAbrupt);

    // 3. Perform PrepareForTailCall().
    // 4. Return ? Call(target, thisArgument, args).
    const int32_t argsLength = static_cast<int32_t>(args->GetLength());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, target, thisArgument, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, args);
    return JSFunction::Call(info);
}

// ecma 26.1.2 Reflect.construct (target, argumentsList [ , newTarget])
JSTaggedValue BuiltinsReflect::ReflectConstruct(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If IsConstructor(target) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.construct target is not constructor", JSTaggedValue::Exception());
    }
    // 2. If newTarget is not present, set newTarget to target.
    JSHandle<JSTaggedValue> newTarget =
        argv->GetArgsNumber() > 2 ? GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD) : target;  // 2: num args
    // 3. Else if IsConstructor(newTarget) is false, throw a TypeError exception.
    if (!newTarget->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.construct newTarget is present, but not constructor",
                                    JSTaggedValue::Exception());
    }
    // 4. Let args be ? CreateListFromArrayLike(argumentsList).
    JSHandle<JSTaggedValue> argumentsList = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> argOrAbrupt = JSObject::CreateListFromArrayLike(thread, argumentsList);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> args = JSHandle<TaggedArray>::Cast(argOrAbrupt);
    // 5. Return ? Construct(target, args, newTarget).
    const int32_t argsLength = static_cast<int32_t>(args->GetLength());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, target, undefined, newTarget, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, args);
    return JSFunction::Construct(info);
}

// ecma 26.1.3 Reflect.defineProperty (target, propertyKey, attributes)
JSTaggedValue BuiltinsReflect::ReflectDefineProperty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, DefineProperty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.defineProperty target is not object", JSTaggedValue::Exception());
    }
    // 2. Let key be ? ToPropertyKey(propertyKey).
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Let desc be ? ToPropertyDescriptor(attributes).
    JSHandle<JSTaggedValue> attributes = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    PropertyDescriptor desc(thread);
    JSObject::ToPropertyDescriptor(thread, attributes, desc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Return ? target.[[DefineOwnProperty]](key, desc).
    return GetTaggedBoolean(JSTaggedValue::DefineOwnProperty(thread, target, key, desc));
}

// ecma 21.1.4 Reflect.deleteProperty (target, propertyKey)
JSTaggedValue BuiltinsReflect::ReflectDeleteProperty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, DeleteProperty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.deleteProperty target is not object", JSTaggedValue::Exception());
    }
    // 2. Let key be ? ToPropertyKey(propertyKey).
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return ? target.[[Delete]](key).
    return GetTaggedBoolean(JSTaggedValue::DeleteProperty(thread, target, key));
}

// ecma 26.1.5 Reflect.get (target, propertyKey [ , receiver])
JSTaggedValue BuiltinsReflect::ReflectGet(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> val = GetCallArg(argv, 0);
    if (!val->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.get target is not object", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> target = JSHandle<JSObject>::Cast(val);
    // 2. Let key be ? ToPropertyKey(propertyKey).
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If receiver is not present, then
    //     a. Set receiver to target.
    // 4. Return ? target.[[Get]](key, receiver).
    if (argv->GetArgsNumber() == 2) {  // 2: 2 means that there are 2 args in total
        return JSObject::GetProperty(thread, target, key).GetValue().GetTaggedValue();
    }
    JSHandle<JSTaggedValue> receiver = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    return JSObject::GetProperty(thread, val, key, receiver).GetValue().GetTaggedValue();
}

// ecma 26.1.6 Reflect.getOwnPropertyDescriptor ( target, propertyKey )
JSTaggedValue BuiltinsReflect::ReflectGetOwnPropertyDescriptor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, GetOwnPropertyDescriptor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.getOwnPropertyDescriptor target is not object",
                                    JSTaggedValue::Exception());
    }
    // 2. Let key be ? ToPropertyKey(propertyKey).
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Let desc be ? target.[[GetOwnProperty]](key).
    PropertyDescriptor desc(thread);
    if (!JSTaggedValue::GetOwnProperty(thread, target, key, desc)) {
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 4. Return FromPropertyDescriptor(desc).
    JSHandle<JSTaggedValue> res = JSObject::FromPropertyDescriptor(thread, desc);
    return res.GetTaggedValue();
}

// ecma 21.1.7 Reflect.getPrototypeOf (target)
JSTaggedValue BuiltinsReflect::ReflectGetPrototypeOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, GetPrototypeOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> val = GetCallArg(argv, 0);
    if (!val->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.getPrototypeOf target is not object", JSTaggedValue::Exception());
    }
    // 2. Return ? target.[[GetPrototypeOf]]().
    return JSTaggedValue::GetPrototype(thread, val);
}

// ecma 26.1.8 Reflect.has (target, propertyKey)
JSTaggedValue BuiltinsReflect::ReflectHas(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.has target is not object", JSTaggedValue::Exception());
    }
    // 2. Let key be ? ToPropertyKey(propertyKey).
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return ? target.[[HasProperty]](key).
    return GetTaggedBoolean(JSTaggedValue::HasProperty(thread, target, key));
}

// ecma 26.1.9  Reflect.isExtensible (target)
JSTaggedValue BuiltinsReflect::ReflectIsExtensible(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.isExtensible target is not object", JSTaggedValue::Exception());
    }
    // 2. Return ? target.[[IsExtensible]]().
    return GetTaggedBoolean(target->IsExtensible(thread));
}

// ecma 26.1.10 Reflect.ownKeys (target)
JSTaggedValue BuiltinsReflect::ReflectOwnKeys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, OwnKeys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.ownKeys target is not object", JSTaggedValue::Exception());
    }
    // 2. Let keys be ? target.[[OwnPropertyKeys]]().
    JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, target);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayFromList(keys).
    JSHandle<JSArray> result = JSArray::CreateArrayFromList(thread, keys);
    return result.GetTaggedValue();
}

// ecma 26.1.11 Reflect.preventExtensions (target)
JSTaggedValue BuiltinsReflect::ReflectPreventExtensions(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, PreventExtensions);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.preventExtensions target is not object",
                                    JSTaggedValue::Exception());
    }
    // 2. Return ? target.[[PreventExtensions]]().
    return GetTaggedBoolean(JSTaggedValue::PreventExtensions(thread, target));
}

// ecma 26.1.12 Reflect.set (target, propertyKey, V [ , receiver])
JSTaggedValue BuiltinsReflect::ReflectSet(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> targetVal = GetCallArg(argv, 0);
    if (!targetVal->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.get target is not object", JSTaggedValue::Exception());
    }
    // 2. Let key be ? ToPropertyKey(propertyKey).
    JSHandle<JSTaggedValue> key = JSTaggedValue::ToPropertyKey(thread, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    // 3. If receiver is not present, then
    //     a. Set receiver to target.
    // 4. Return ? target.[[Set]](key, receiver).
    if (argv->GetArgsNumber() == 3) {  // 3: 3 means that there are three args in total
        return GetTaggedBoolean(JSTaggedValue::SetProperty(thread, targetVal, key, value));
    }
    JSHandle<JSTaggedValue> receiver = GetCallArg(argv, 3);  // 3: 3 means the third arg
    return GetTaggedBoolean(JSTaggedValue::SetProperty(thread, targetVal, key, value, receiver));
}

// ecma 26.1.13  Reflect.setPrototypeOf (target, proto)
JSTaggedValue BuiltinsReflect::ReflectSetPrototypeOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Reflect, SetPrototypeOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. If Type(target) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Reflect.setPrototypeOf target is not object", JSTaggedValue::Exception());
    }
    // 2. If Type(proto) is not Object and proto is not null, throw a TypeError exception.
    JSHandle<JSTaggedValue> proto = GetCallArg(argv, 1);
    if (!proto->IsECMAObject() && !proto->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "SetPrototypeOf: proto is neither Object nor Null",
                                    JSTaggedValue::Exception());
    }
    // 3. Return ? target.[[SetPrototypeOf]](proto).
    return GetTaggedBoolean(JSTaggedValue::SetPrototype(thread, target, proto));
}
}  // namespace panda::ecmascript::builtins
