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

#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::builtins {
// ecma 19.2.1 Function (p1, p2, ... , pn, body)
JSTaggedValue BuiltinsFunction::FunctionConstructor(EcmaRuntimeCallInfo *argv)
{
    // not support
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "Not support eval. Forbidden using new Function()/Function().",
                                JSTaggedValue::Exception());
}

// ecma 19.2.3 The Function prototype object is itself a built-in function object.
//             When invoked, it accepts any arguments and returns undefined.
JSTaggedValue BuiltinsFunction::FunctionPrototypeInvokeSelf([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue::Undefined();
}
namespace {
static size_t MakeArgListWithHole(JSThread *thread, TaggedArray *argv, size_t length)
{
    if (length > argv->GetLength()) {
        length = argv->GetLength();
    }
    for (size_t index = 0; index < length; ++index) {
        JSTaggedValue value = argv->Get(thread, index);
        if (value.IsHole()) {
            argv->Set(thread, index, JSTaggedValue::Undefined());
        }
    }
    return length;
}

static std::pair<TaggedArray*, size_t> BuildArgumentsListFast(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &arrayObj)
{
    if (!arrayObj->HasStableElements(thread)) {
        return std::make_pair(nullptr, 0);
    }
    if (arrayObj->IsStableJSArguments(thread)) {
        JSHandle<JSArguments> argList = JSHandle<JSArguments>::Cast(arrayObj);
        TaggedArray *elements = TaggedArray::Cast(argList->GetElements().GetTaggedObject());
        auto env = thread->GetEcmaVM()->GetGlobalEnv();
        if (argList->GetClass() != env->GetArgumentsClass().GetObject<JSHClass>()) {
            return std::make_pair(nullptr, 0);
        }
        auto result = argList->GetPropertyInlinedProps(JSArguments::LENGTH_INLINE_PROPERTY_INDEX);
        if (!result.IsInt()) {
            return std::make_pair(nullptr, 0);
        }
        size_t length = static_cast<size_t>(result.GetInt());
        size_t res = MakeArgListWithHole(thread, elements, length);
        return std::make_pair(elements, res);
    } else if (arrayObj->IsStableJSArray(thread)) {
        JSHandle<JSArray> argList = JSHandle<JSArray>::Cast(arrayObj);
        TaggedArray *elements = TaggedArray::Cast(argList->GetElements().GetTaggedObject());
        size_t length = argList->GetArrayLength();
        size_t res = MakeArgListWithHole(thread, elements, length);
        return std::make_pair(elements, res);
    } else {
        UNREACHABLE();
    }
}
}  // anonymous namespace

// ecma 19.2.3.1 Function.prototype.apply (thisArg, argArray)
JSTaggedValue BuiltinsFunction::FunctionPrototypeApply(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeApply);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If IsCallable(func) is false, throw a TypeError exception.
    if (!GetThis(argv)->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "apply target is not callable", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> func = GetThis(argv);
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    // 2. If argArray is null or undefined, then
    if (GetCallArg(argv, 1)->IsUndefined()) {  // null will also get undefined
        // a. Return Call(func, thisArg).
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, 0);
        return JSFunction::Call(info);
    }
    // 3. Let argList be CreateListFromArrayLike(argArray).
    JSHandle<JSTaggedValue> arrayObj = GetCallArg(argv, 1);
    std::pair<TaggedArray*, size_t> argumentsList = BuildArgumentsListFast(thread, arrayObj);
    if (!argumentsList.first) {
        JSHandle<TaggedArray> argList = JSHandle<TaggedArray>::Cast(
            JSObject::CreateListFromArrayLike(thread, arrayObj));
        // 4. ReturnIfAbrupt(argList).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        const int32_t argsLength = static_cast<int32_t>(argList->GetLength());
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(argsLength, argList);
        return JSFunction::Call(info);
    }
    // 6. Return Call(func, thisArg, argList).
    const int32_t argsLength = static_cast<int32_t>(argumentsList.second);
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, argumentsList.first);
    return JSFunction::Call(info);
}

// ecma 19.2.3.2 Function.prototype.bind (thisArg , ...args)
JSTaggedValue BuiltinsFunction::FunctionPrototypeBind(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeBind);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let Target be the this value.
    JSHandle<JSTaggedValue> target = GetThis(argv);
    // 2. If IsCallable(Target) is false, throw a TypeError exception.
    if (!target->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "bind target is not callable", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 0);
    int32_t argsLength = 0;
    if (argv->GetArgsNumber() > 1) {
        argsLength = argv->GetArgsNumber() - 1;
    }

    // 3. Let args be a new (possibly empty) List consisting of all of the argument
    //    values provided after thisArg in order.
    JSHandle<TaggedArray> argsArray = factory->NewTaggedArray(argsLength);
    for (int32_t index = 0; index < argsLength; ++index) {
        argsArray->Set(thread, index, GetCallArg(argv, index + 1));
    }
    // 4. Let F be BoundFunctionCreate(Target, thisArg, args).
    JSHandle<JSFunctionBase> targetFunction = JSHandle<JSFunctionBase>::Cast(target);
    JSHandle<JSBoundFunction> boundFunction = factory->NewJSBoundFunction(targetFunction, thisArg, argsArray);
    // 5. ReturnIfAbrupt(F)
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. Let targetHasLength be HasOwnProperty(Target, "length").
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    bool targetHasLength =
        JSTaggedValue::HasOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(targetFunction), lengthKey);
    // 7. ReturnIfAbrupt(targetHasLength).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    double lengthValue = 0.0;
    // 8. If targetHasLength is true, then
    if (targetHasLength) {
        // a. Let targetLen be Get(Target, "length").
        JSHandle<JSTaggedValue> targetLen = JSObject::GetProperty(thread, target, lengthKey).GetValue();
        // b. ReturnIfAbrupt(targetLen).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // c. If Type(targetLen) is not Number, let L be 0.
        // d. Else,
        //    i. Let targetLen be ToInteger(targetLen).
        //    ii. Let L be the larger of 0 and the result of targetLen minus the number of elements of args.
        if (targetLen->IsNumber()) {
            // argv include thisArg
            lengthValue =
                std::max(0.0, JSTaggedValue::ToNumber(thread, targetLen).GetNumber() - static_cast<double>(argsLength));
        }
    }
    // 9. Else let L be 0.

    // 10. Let status be DefinePropertyOrThrow(F, "length", PropertyDescriptor {[[Value]]: L,
    //     [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: true}).
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(lengthValue)), false, false, true);
    [[maybe_unused]] bool status =
        JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(boundFunction), lengthKey, desc);
    // 11. Assert: status is not an abrupt completion.
    ASSERT_PRINT(status, "DefinePropertyOrThrow failed");

    // 12. Let targetName be Get(Target, "name").
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    JSHandle<JSTaggedValue> targetName = JSObject::GetProperty(thread, target, nameKey).GetValue();
    // 13. ReturnIfAbrupt(targetName).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> boundName = thread->GlobalConstants()->GetHandledBoundString();
    // 14. If Type(targetName) is not String, let targetName be the empty string.
    // 15. Perform SetFunctionName(F, targetName, "bound").
    if (!targetName->IsString()) {
        JSHandle<JSTaggedValue> emptyString(factory->GetEmptyString());
        status = JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(boundFunction), emptyString, boundName);
    } else {
        status = JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(boundFunction), targetName, boundName);
    }
    // Assert: status is not an abrupt completion.
    ASSERT_PRINT(status, "SetFunctionName failed");

    // 16. Return F.
    return boundFunction.GetTaggedValue();
}

// ecma 19.2.3.3 Function.prototype.call (thisArg , ...args)
JSTaggedValue BuiltinsFunction::FunctionPrototypeCall(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeCall);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If IsCallable(func) is false, throw a TypeError exception.
    if (!GetThis(argv)->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "call target is not callable", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> func = GetThis(argv);
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 0);
    int32_t argsLength = 0;
    if (argv->GetArgsNumber() > 1) {
        argsLength = argv->GetArgsNumber() - 1;
    }
    // 2. Let argList be an empty List.
    // 3. If this method was called with more than one argument then in left to right order,
    //    starting with the second argument, append each argument as the last element of argList.
    // 5. Return Call(func, thisArg, argList).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(argsLength, 0, argv, 1);
    return JSFunction::Call(info);
}

// ecma 19.2.3.5 Function.prototype.toString ()
JSTaggedValue BuiltinsFunction::FunctionPrototypeToString(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeToString);
    // not implement due to that runtime can not get JS Source Code now.
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    if (!thisValue->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "function.toString() target is not callable", JSTaggedValue::Exception());
    }
    return GetTaggedString(thread, "Not support function.toString() due to Runtime can not obtain Source Code yet.");
}

// ecma 19.2.3.6 Function.prototype[@@hasInstance] (V)
JSTaggedValue BuiltinsFunction::FunctionPrototypeHasInstance(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Function, PrototypeHasInstance);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    // 1. Let F be the this value.
    JSHandle<JSTaggedValue> thisValue = GetThis(argv);
    // 2. Return OrdinaryHasInstance(F, V).
    JSHandle<JSTaggedValue> arg = GetCallArg(argv, 0);
    return JSFunction::OrdinaryHasInstance(argv->GetThread(), thisValue, arg) ? GetTaggedBoolean(true)
                                                                              : GetTaggedBoolean(false);
}
}  // namespace panda::ecmascript::builtins
