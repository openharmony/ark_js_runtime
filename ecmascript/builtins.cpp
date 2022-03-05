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

#include "ecmascript/builtins.h"

#ifdef PANDA_TARGET_WINDOWS
#include <shlwapi.h>
#ifdef ERROR
#undef ERROR
#endif
#ifdef GetObject
#undef GetObject
#endif
#endif
#include "ecmascript/base/error_type.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_generator.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_intl.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_map.h"
#include "ecmascript/builtins/builtins_math.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_plural_rules.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_relative_time_format.h"
#include "ecmascript/builtins/builtins_set.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/builtins/builtins_weak_set.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_api_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"
#include "ohos/init_data.h"

namespace panda::ecmascript {
using Number = builtins::BuiltinsNumber;
using Object = builtins::BuiltinsObject;
using Date = builtins::BuiltinsDate;
using Symbol = builtins::BuiltinsSymbol;
using Boolean = builtins::BuiltinsBoolean;
using BuiltinsMap = builtins::BuiltinsMap;
using BuiltinsSet = builtins::BuiltinsSet;
using BuiltinsWeakMap = builtins::BuiltinsWeakMap;
using BuiltinsWeakSet = builtins::BuiltinsWeakSet;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsTypedArray = builtins::BuiltinsTypedArray;
using BuiltinsIterator = builtins::BuiltinsIterator;

using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using ErrorType = base::ErrorType;
using Global = builtins::BuiltinsGlobal;
using BuiltinsString = builtins::BuiltinsString;
using StringIterator = builtins::BuiltinsStringIterator;
using RegExp = builtins::BuiltinsRegExp;
using Function = builtins::BuiltinsFunction;
using Math = builtins::BuiltinsMath;
using ArrayBuffer = builtins::BuiltinsArrayBuffer;
using Json = builtins::BuiltinsJson;
using Proxy = builtins::BuiltinsProxy;
using Reflect = builtins::BuiltinsReflect;
using AsyncFunction = builtins::BuiltinsAsyncFunction;
using GeneratorObject = builtins::BuiltinsGenerator;
using Promise = builtins::BuiltinsPromise;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
using ErrorType = base::ErrorType;
using DataView = builtins::BuiltinsDataView;
using Intl = builtins::BuiltinsIntl;
using Locale = builtins::BuiltinsLocale;
using DateTimeFormat = builtins::BuiltinsDateTimeFormat;
using RelativeTimeFormat = builtins::BuiltinsRelativeTimeFormat;
using NumberFormat = builtins::BuiltinsNumberFormat;
using Collator = builtins::BuiltinsCollator;
using PluralRules = builtins::BuiltinsPluralRules;
using ContainersPrivate = containers::ContainersPrivate;

bool GetAbsolutePath(const std::string &relativePath, std::string &absPath)
{
    if (relativePath.size() >= PATH_MAX) {
        return false;
    }
    char buffer[PATH_MAX] = {0};
#ifndef PANDA_TARGET_WINDOWS
    auto path = realpath(relativePath.c_str(), buffer);
    if (path == nullptr) {
        return false;
    }
    absPath = std::string(path);
    return true;
#else
    auto path = _fullpath(buffer, relativePath.c_str(), buffer.size() - 1);
    if (path == nullptr) {
        return false;
    }
    bool valid = PathCanonicalizeA(buffer, path);
    if (!valid) {
        return false;
    }
    absPath = std::string(buffer);
    return true;
#endif
}

void Builtins::Initialize(const JSHandle<GlobalEnv> &env, JSThread *thread)
{
    thread_ = thread;
    vm_ = thread->GetEcmaVM();
    factory_ = vm_->GetFactory();
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    // Object.prototype[dynclass]
    JSHandle<JSHClass> objPrototypeDynclass = factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);

    // Object.prototype
    JSHandle<JSObject> objFuncPrototype = factory_->NewJSObject(objPrototypeDynclass);
    JSHandle<JSTaggedValue> objFuncPrototypeVal(objFuncPrototype);

    // Object.prototype_or_dynclass
    JSHandle<JSHClass> objFuncDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);

    // GLobalObject.prototype_or_dynclass
    JSHandle<JSHClass> globalObjFuncDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_GLOBAL_OBJECT, 0);
    globalObjFuncDynclass->SetPrototype(thread_, objFuncPrototypeVal.GetTaggedValue());
    globalObjFuncDynclass->SetIsDictionaryMode(true);
    // Function.prototype_or_dynclass
    JSHandle<JSHClass> emptyFuncDynclass(
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, objFuncPrototypeVal));

    // PrimitiveRef.prototype_or_dynclass
    JSHandle<JSHClass> primRefObjDynclass =
        factory_->NewEcmaDynClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, objFuncPrototypeVal);

    // init global object
    JSHandle<JSObject> globalObject = factory_->NewNonMovableJSObject(globalObjFuncDynclass);
    JSHandle<JSHClass> newGlobalDynclass = JSHClass::Clone(thread_, globalObjFuncDynclass);
    globalObject->SetClass(newGlobalDynclass);
    env->SetJSGlobalObject(thread_, globalObject);

    // initialize Function, forbidden change order
    InitializeFunction(env, emptyFuncDynclass);

    JSHandle<JSObject> objFuncInstancePrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> objFuncInstancePrototypeValue(objFuncInstancePrototype);
    JSHandle<JSHClass> asyncFuncClass = factory_->CreateFunctionClass(
        FunctionKind::ASYNC_FUNCTION, JSAsyncFunction::SIZE, JSType::JS_ASYNC_FUNCTION, objFuncInstancePrototypeValue);
    env->SetAsyncFunctionClass(thread_, asyncFuncClass);

    JSHandle<JSHClass> asyncAwaitStatusFuncClass =
        factory_->CreateFunctionClass(FunctionKind::NORMAL_FUNCTION, JSAsyncAwaitStatusFunction::SIZE,
                                      JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, env->GetFunctionPrototype());
    env->SetAsyncAwaitStatusFunctionClass(thread_, asyncAwaitStatusFuncClass);

    JSHandle<JSHClass> promiseReactionFuncClass = factory_->NewEcmaDynClass(
        JSPromiseReactionsFunction::SIZE, JSType::JS_PROMISE_REACTIONS_FUNCTION, env->GetFunctionPrototype());
    promiseReactionFuncClass->SetCallable(true);
    promiseReactionFuncClass->SetExtensible(true);
    env->SetPromiseReactionFunctionClass(thread_, promiseReactionFuncClass);

    JSHandle<JSHClass> promiseExecutorFuncClass = factory_->NewEcmaDynClass(
        JSPromiseExecutorFunction::SIZE, JSType::JS_PROMISE_EXECUTOR_FUNCTION, env->GetFunctionPrototype());
    promiseExecutorFuncClass->SetCallable(true);
    promiseExecutorFuncClass->SetExtensible(true);
    env->SetPromiseExecutorFunctionClass(thread_, promiseExecutorFuncClass);

    JSHandle<JSHClass> promiseAllResolveElementFunctionClass =
        factory_->NewEcmaDynClass(JSPromiseAllResolveElementFunction::SIZE,
                                  JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, env->GetFunctionPrototype());
    promiseAllResolveElementFunctionClass->SetCallable(true);
    promiseAllResolveElementFunctionClass->SetExtensible(true);
    env->SetPromiseAllResolveElementFunctionClass(thread_, promiseAllResolveElementFunctionClass);

    JSHandle<JSHClass> proxyRevocFuncClass = factory_->NewEcmaDynClass(
        JSProxyRevocFunction::SIZE, JSType::JS_PROXY_REVOC_FUNCTION, env->GetFunctionPrototype());
    proxyRevocFuncClass->SetCallable(true);
    proxyRevocFuncClass->SetExtensible(true);
    env->SetProxyRevocFunctionClass(thread_, proxyRevocFuncClass);

    // Object = new Function()
    JSHandle<JSObject> objectFunction(
        NewBuiltinConstructor(env, objFuncPrototype, Object::ObjectConstructor, "Object", FunctionLength::ONE));
    objectFunction.GetObject<JSFunction>()->SetBuiltinsCtorMode();
    objectFunction.GetObject<JSFunction>()->SetFunctionPrototype(thread_, objFuncDynclass.GetTaggedValue());
    // initialize object method.
    env->SetObjectFunction(thread_, objectFunction);
    env->SetObjectFunctionPrototype(thread_, objFuncPrototype);

    JSHandle<JSHClass> functionClass = factory_->CreateFunctionClass(FunctionKind::BASE_CONSTRUCTOR, JSFunction::SIZE,
                                                                     JSType::JS_FUNCTION, env->GetFunctionPrototype());
    env->SetFunctionClassWithProto(thread_, functionClass);
    functionClass = factory_->CreateFunctionClass(FunctionKind::NORMAL_FUNCTION, JSFunction::SIZE, JSType::JS_FUNCTION,
                                                  env->GetFunctionPrototype());
    env->SetFunctionClassWithoutProto(thread_, functionClass);
    functionClass = factory_->CreateFunctionClass(FunctionKind::CLASS_CONSTRUCTOR, JSFunction::SIZE,
                                                  JSType::JS_FUNCTION, env->GetFunctionPrototype());
    env->SetFunctionClassWithoutName(thread_, functionClass);

    if (env == vm_->GetGlobalEnv()) {
        InitializeAllTypeError(env, objFuncDynclass);
        InitializeSymbol(env, primRefObjDynclass);
    } else {
        // error and symbol need to be shared when initialize realm
        InitializeAllTypeErrorWithRealm(env);
        InitializeSymbolWithRealm(env, primRefObjDynclass);
    }

    InitializeNumber(env, globalObject, primRefObjDynclass);
    InitializeDate(env, objFuncDynclass);
    InitializeObject(env, objFuncPrototype, objectFunction);
    InitializeBoolean(env, primRefObjDynclass);

    InitializeRegExp(env);
    InitializeSet(env, objFuncDynclass);
    InitializeMap(env, objFuncDynclass);
    InitializeWeakMap(env, objFuncDynclass);
    InitializeWeakSet(env, objFuncDynclass);
    InitializeArray(env, objFuncPrototypeVal);
    InitializeTypedArray(env, objFuncDynclass);
    InitializeString(env, primRefObjDynclass);
    InitializeArrayBuffer(env, objFuncDynclass);
    InitializeDataView(env, objFuncDynclass);

    JSHandle<JSHClass> argumentsDynclass = factory_->CreateJSArguments();
    env->SetArgumentsClass(thread_, argumentsDynclass);
    SetArgumentsSharedAccessor(env);

    InitializeGlobalObject(env, globalObject);
    InitializeMath(env, objFuncPrototypeVal);
    InitializeJson(env, objFuncPrototypeVal);
    InitializeIterator(env, objFuncDynclass);
    InitializeProxy(env);
    InitializeReflect(env, objFuncPrototypeVal);
    InitializeAsyncFunction(env, objFuncDynclass);
    InitializeGenerator(env, objFuncDynclass);
    InitializeGeneratorFunction(env, objFuncDynclass);
    InitializePromise(env, objFuncDynclass);
    InitializePromiseJob(env);

    // Initialize IcuData Path
    JSRuntimeOptions options = vm_->GetJSOptions();
    std::string icuPath = options.GetIcuDataPath();
    if (icuPath == "default") {
#ifndef PANDA_TARGET_WINDOWS
        SetHwIcuDirectory();
#endif
    } else {
        std::string absPath;
        if (GetAbsolutePath(icuPath, absPath)) {
            u_setDataDirectory(absPath.c_str());
        }
    }
    InitializeIntl(env, objFuncPrototypeVal);
    InitializeLocale(env);
    InitializeDateTimeFormat(env);
    InitializeNumberFormat(env);
    InitializeRelativeTimeFormat(env);
    InitializeCollator(env);
    InitializePluralRules(env);

    JSHandle<JSHClass> generatorFuncClass =
        factory_->CreateFunctionClass(FunctionKind::GENERATOR_FUNCTION, JSFunction::SIZE, JSType::JS_GENERATOR_FUNCTION,
                                      env->GetGeneratorFunctionPrototype());
    env->SetGeneratorFunctionClass(thread_, generatorFuncClass);
    env->SetObjectFunctionPrototypeClass(thread_, JSTaggedValue(objFuncPrototype->GetClass()));
    thread_->ResetGuardians();
}

void Builtins::InitializeGlobalObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    // Global object test
    SetFunction(env, globalObject, "print", Global::PrintEntrypoint, 0);
#if ECMASCRIPT_ENABLE_RUNTIME_STAT
    SetFunction(env, globalObject, "startRuntimeStat", Global::StartRuntimeStat, 0);
    SetFunction(env, globalObject, "stopRuntimeStat", Global::StopRuntimeStat, 0);
#endif

    JSRuntimeOptions options = vm_->GetJSOptions();
    if (options.IsEnableArkTools()) {
        JSHandle<JSTaggedValue> arkTools(InitializeArkTools(env));
        SetConstantObject(globalObject, "ArkTools", arkTools);
    }

#if ECMASCRIPT_ENABLE_ARK_CONTAINER
    // Set ArkPrivate
    JSHandle<JSTaggedValue> arkPrivate(InitializeArkPrivate(env));
    SetConstantObject(globalObject, "ArkPrivate", arkPrivate);
#endif

    // Global object function
    SetFunction(env, globalObject, "eval", Global::NotSupportEval, FunctionLength::ONE);
    SetFunction(env, globalObject, "isFinite", Global::IsFinite, FunctionLength::ONE);
    SetFunction(env, globalObject, "isNaN", Global::IsNaN, FunctionLength::ONE);
    SetFunction(env, globalObject, "decodeURI", Global::DecodeURI, FunctionLength::ONE);
    SetFunction(env, globalObject, "encodeURI", Global::EncodeURI, FunctionLength::ONE);
    SetFunction(env, globalObject, "decodeURIComponent", Global::DecodeURIComponent, FunctionLength::ONE);
    SetFunction(env, globalObject, "encodeURIComponent", Global::EncodeURIComponent, FunctionLength::ONE);

    // Global object property
    SetGlobalThis(globalObject, "globalThis", JSHandle<JSTaggedValue>::Cast(globalObject));
    SetConstant(globalObject, "Infinity", JSTaggedValue(base::POSITIVE_INFINITY));
    SetConstant(globalObject, "NaN", JSTaggedValue(base::NAN_VALUE));
    SetConstant(globalObject, "undefined", JSTaggedValue::Undefined());
}

void Builtins::InitializeFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &emptyFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Initialize Function.prototype
    JSMethod *invokeSelf =
        vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(Function::FunctionPrototypeInvokeSelf));
    JSHandle<JSFunction> funcFuncPrototype = factory_->NewJSFunctionByDynClass(invokeSelf, emptyFuncDynclass);
    // ecma 19.2.3 The value of the name property of the Function prototype object is the empty String.
    JSHandle<JSTaggedValue> emptyString(thread_->GlobalConstants()->GetHandledEmptyString());
    JSHandle<JSTaggedValue> undefinedString(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(funcFuncPrototype), emptyString, undefinedString);
    // ecma 19.2.3 The value of the length property of the Function prototype object is 0.
    JSFunction::SetFunctionLength(thread_, funcFuncPrototype, JSTaggedValue(FunctionLength::ZERO));

    JSHandle<JSTaggedValue> funcFuncPrototypeValue(funcFuncPrototype);
    // Function.prototype_or_dynclass
    JSHandle<JSHClass> funcFuncIntanceDynclass =
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, funcFuncPrototypeValue);
    funcFuncIntanceDynclass->SetConstructor(true);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(factory_->NewJSObject(funcFuncIntanceDynclass));
    function->SetBuiltinsCtorMode();

    // Function = new Function() (forbidden use NewBuiltinConstructor)
    JSMethod *ctor = vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(Function::FunctionConstructor));
    JSHandle<JSFunction> funcFunc =
        factory_->NewJSFunctionByDynClass(ctor, funcFuncIntanceDynclass, FunctionKind::BUILTIN_CONSTRUCTOR);

    auto funcFuncPrototypeObj = JSHandle<JSObject>(funcFuncPrototype);
    InitializeCtor(env, funcFuncPrototypeObj, funcFunc, "Function", FunctionLength::ONE);

    funcFunc->SetFunctionPrototype(thread_, funcFuncIntanceDynclass.GetTaggedValue());
    env->SetFunctionFunction(thread_, funcFunc);
    env->SetFunctionPrototype(thread_, funcFuncPrototype);

    JSHandle<JSHClass> normalFuncClass =
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetFunctionPrototype());
    env->SetNormalFunctionClass(thread_, normalFuncClass);

    JSHandle<JSHClass> jSIntlBoundFunctionClass =
        factory_->CreateFunctionClass(FunctionKind::NORMAL_FUNCTION, JSIntlBoundFunction::SIZE,
                                      JSType::JS_INTL_BOUND_FUNCTION, env->GetFunctionPrototype());
    env->SetJSIntlBoundFunctionClass(thread_, jSIntlBoundFunctionClass);

    JSHandle<JSHClass> constructorFunctionClass =
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetFunctionPrototype());
    constructorFunctionClass->SetConstructor(true);
    JSHandle<JSFunction> functionConstructor =
        JSHandle<JSFunction>::Cast(factory_->NewJSObject(constructorFunctionClass));
    functionConstructor->SetBuiltinsCtorMode();
    env->SetConstructorFunctionClass(thread_, constructorFunctionClass);

    StrictModeForbiddenAccessCallerArguments(env, funcFuncPrototypeObj);

    // Function.prototype method
    // 19.2.3.1 Function.prototype.apply ( thisArg, argArray )
    SetFunction(env, funcFuncPrototypeObj, "apply", Function::FunctionPrototypeApply, FunctionLength::TWO);
    // 19.2.3.2 Function.prototype.bind ( thisArg , ...args)
    SetFunction(env, funcFuncPrototypeObj, "bind", Function::FunctionPrototypeBind, FunctionLength::ONE);
    // 19.2.3.3 Function.prototype.call (thisArg , ...args)
    SetFunction(env, funcFuncPrototypeObj, "call", Function::FunctionPrototypeCall, FunctionLength::ONE);
    // 19.2.3.5 Function.prototype.toString ( )
    SetFunction(env, funcFuncPrototypeObj, thread_->GlobalConstants()->GetHandledToStringString(),
                Function::FunctionPrototypeToString, FunctionLength::ZERO);
}

void Builtins::InitializeObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &objFuncPrototype,
                                const JSHandle<JSObject> &objFunc)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Object method.
    // 19.1.2.1Object.assign ( target, ...sources )
    SetFunction(env, objFunc, "assign", Object::Assign, FunctionLength::TWO);
    // 19.1.2.2Object.create ( O [ , Properties ] )
    SetFunction(env, objFunc, "create", Object::Create, FunctionLength::TWO);
    // 19.1.2.3Object.defineProperties ( O, Properties )
    SetFunction(env, objFunc, "defineProperties", Object::DefineProperties, FunctionLength::TWO);
    // 19.1.2.4Object.defineProperty ( O, P, Attributes )
    SetFunction(env, objFunc, "defineProperty", Object::DefineProperty, FunctionLength::THREE);
    // 19.1.2.5Object.freeze ( O )
    SetFunction(env, objFunc, "freeze", Object::Freeze, FunctionLength::ONE);
    // 19.1.2.6Object.getOwnPropertyDescriptor ( O, P )
    SetFunction(env, objFunc, "getOwnPropertyDescriptor", Object::GetOwnPropertyDesciptor, FunctionLength::TWO);
    // 19.1.2.7Object.getOwnPropertyNames ( O )
    SetFunction(env, objFunc, "getOwnPropertyNames", Object::GetOwnPropertyNames, FunctionLength::ONE);
    // 19.1.2.8Object.getOwnPropertySymbols ( O )
    SetFunction(env, objFunc, "getOwnPropertySymbols", Object::GetOwnPropertySymbols, FunctionLength::ONE);
    // 19.1.2.9Object.getPrototypeOf ( O )
    SetFunction(env, objFunc, "getPrototypeOf", Object::GetPrototypeOf, FunctionLength::ONE);
    // 19.1.2.10Object.is ( value1, value2 )
    SetFunction(env, objFunc, "is", Object::Is, 2);
    // 19.1.2.11Object.isExtensible ( O )
    SetFunction(env, objFunc, "isExtensible", Object::IsExtensible, FunctionLength::ONE);
    // 19.1.2.12Object.isFrozen ( O )
    SetFunction(env, objFunc, "isFrozen", Object::IsFrozen, FunctionLength::ONE);
    // 19.1.2.13Object.isSealed ( O )
    SetFunction(env, objFunc, "isSealed", Object::IsSealed, FunctionLength::ONE);
    // 19.1.2.14 Object.keys(O)
    SetFunction(env, objFunc, "keys", Object::Keys, FunctionLength::ONE);
    // 19.1.2.15 Object.preventExtensions(O)
    SetFunction(env, objFunc, "preventExtensions", Object::PreventExtensions, FunctionLength::ONE);
    // 19.1.2.17 Object.seal(O)
    SetFunction(env, objFunc, "seal", Object::Seal, FunctionLength::ONE);
    // 19.1.2.18 Object.setPrototypeOf(O, proto)
    SetFunction(env, objFunc, "setPrototypeOf", Object::SetPrototypeOf, FunctionLength::TWO);
    // 20.1.2.5 Object.entries ( O )
    SetFunction(env, objFunc, "entries", Object::Entries, FunctionLength::ONE);

    // Object.property method
    // 19.1.3.2 Object.prototype.hasOwnProperty(V)
    SetFunction(env, objFuncPrototype, "hasOwnProperty", Object::HasOwnProperty, FunctionLength::ONE);
    // 19.1.3.3 Object.prototype.isPrototypeOf(V)
    SetFunction(env, objFuncPrototype, "isPrototypeOf", Object::IsPrototypeOf, FunctionLength::ONE);
    // 19.1.3.4 Object.prototype.propertyIsEnumerable(V)
    SetFunction(env, objFuncPrototype, "propertyIsEnumerable", Object::PropertyIsEnumerable, FunctionLength::ONE);
    // 19.1.3.5 Object.prototype.toLocaleString([reserved1[, reserved2]])
    SetFunction(env, objFuncPrototype, "toLocaleString", Object::ToLocaleString, FunctionLength::ZERO);
    // 19.1.3.6 Object.prototype.toString()
    SetFunction(env, objFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Object::ToString,
                FunctionLength::ZERO);
    // 19.1.3.7 Object.prototype.valueOf()
    SetFunction(env, objFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(), Object::ValueOf,
                FunctionLength::ZERO);

    SetFunction(env, objFuncPrototype, "createRealm", Object::CreateRealm, FunctionLength::ZERO);

    // B.2.2.1 Object.prototype.__proto__
    JSHandle<JSTaggedValue> protoKey(factory_->NewFromCanBeCompressString("__proto__"));
    JSHandle<JSTaggedValue> protoGetter = CreateGetter(env, Object::ProtoGetter, "__proto__", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> protoSetter = CreateSetter(env, Object::ProtoSetter, "__proto__", FunctionLength::ONE);
    SetAccessor(objFuncPrototype, protoKey, protoGetter, protoSetter);
}

void Builtins::InitializeSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Symbol.prototype
    JSHandle<JSObject> symbolFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> symbolFuncPrototypeValue(symbolFuncPrototype);

    // Symbol.prototype_or_dynclass
    JSHandle<JSHClass> symbolFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, symbolFuncPrototypeValue);

    // Symbol = new Function()
    JSHandle<JSObject> symbolFunction(
        NewBuiltinConstructor(env, symbolFuncPrototype, Symbol::SymbolConstructor, "Symbol", FunctionLength::ZERO));
    JSHandle<JSFunction>(symbolFunction)->SetFunctionPrototype(thread_, symbolFuncInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(symbolFunction), true, false, true);
    JSObject::DefineOwnProperty(thread_, symbolFuncPrototype, constructorKey, descriptor);

    SetFunction(env, symbolFunction, "for", Symbol::For, FunctionLength::ONE);
    SetFunction(env, symbolFunction, "keyFor", Symbol::KeyFor, FunctionLength::ONE);

    // Symbol attribute
    JSHandle<JSTaggedValue> hasInstanceSymbol(factory_->NewWellKnownSymbolWithChar("Symbol.hasInstance"));
    SetNoneAttributeProperty(symbolFunction, "hasInstance", hasInstanceSymbol);
    JSHandle<JSTaggedValue> isConcatSpreadableSymbol(factory_->NewWellKnownSymbolWithChar("Symbol.isConcatSpreadable"));
    SetNoneAttributeProperty(symbolFunction, "isConcatSpreadable", isConcatSpreadableSymbol);
    JSHandle<JSTaggedValue> toStringTagSymbol(factory_->NewWellKnownSymbolWithChar("Symbol.toStringTag"));
    SetNoneAttributeProperty(symbolFunction, "toStringTag", toStringTagSymbol);
    JSHandle<JSTaggedValue> iteratorSymbol(factory_->NewPublicSymbolWithChar("Symbol.iterator"));
    SetNoneAttributeProperty(symbolFunction, "iterator", iteratorSymbol);
    JSHandle<JSTaggedValue> matchSymbol(factory_->NewPublicSymbolWithChar("Symbol.match"));
    SetNoneAttributeProperty(symbolFunction, "match", matchSymbol);
    JSHandle<JSTaggedValue> replaceSymbol(factory_->NewPublicSymbolWithChar("Symbol.replace"));
    SetNoneAttributeProperty(symbolFunction, "replace", replaceSymbol);
    JSHandle<JSTaggedValue> searchSymbol(factory_->NewPublicSymbolWithChar("Symbol.search"));
    SetNoneAttributeProperty(symbolFunction, "search", searchSymbol);
    JSHandle<JSTaggedValue> speciesSymbol(factory_->NewPublicSymbolWithChar("Symbol.species"));
    SetNoneAttributeProperty(symbolFunction, "species", speciesSymbol);
    JSHandle<JSTaggedValue> splitSymbol(factory_->NewPublicSymbolWithChar("Symbol.split"));
    SetNoneAttributeProperty(symbolFunction, "split", splitSymbol);
    JSHandle<JSTaggedValue> toPrimitiveSymbol(factory_->NewPublicSymbolWithChar("Symbol.toPrimitive"));
    SetNoneAttributeProperty(symbolFunction, "toPrimitive", toPrimitiveSymbol);
    JSHandle<JSTaggedValue> unscopablesSymbol(factory_->NewPublicSymbolWithChar("Symbol.unscopables"));
    SetNoneAttributeProperty(symbolFunction, "unscopables", unscopablesSymbol);

    // symbol.prototype.description
    PropertyDescriptor descriptionDesc(thread_);
    JSHandle<JSTaggedValue> getterKey(factory_->NewFromCanBeCompressString("description"));
    JSHandle<JSTaggedValue> getter(factory_->NewJSFunction(env, reinterpret_cast<void *>(Symbol::DescriptionGetter)));
    SetGetter(symbolFuncPrototype, getterKey, getter);

    // Setup symbol.prototype[@@toPrimitive]
    SetFunctionAtSymbol<JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE>(
        env, symbolFuncPrototype, toPrimitiveSymbol, "[Symbol.toPrimitive]", Symbol::ToPrimitive, FunctionLength::ONE);
    // install the Symbol.prototype methods
    SetFunction(env, symbolFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Symbol::ToString,
                FunctionLength::ZERO);
    SetFunction(env, symbolFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(), Symbol::ValueOf,
                FunctionLength::ZERO);

    env->SetSymbolFunction(thread_, symbolFunction);
    env->SetHasInstanceSymbol(thread_, hasInstanceSymbol);
    env->SetIsConcatSpreadableSymbol(thread_, isConcatSpreadableSymbol);
    env->SetToStringTagSymbol(thread_, toStringTagSymbol);
    env->SetIteratorSymbol(thread_, iteratorSymbol);
    env->SetMatchSymbol(thread_, matchSymbol);
    env->SetReplaceSymbol(thread_, replaceSymbol);
    env->SetSearchSymbol(thread_, searchSymbol);
    env->SetSpeciesSymbol(thread_, speciesSymbol);
    env->SetSplitSymbol(thread_, splitSymbol);
    env->SetToPrimitiveSymbol(thread_, toPrimitiveSymbol);
    env->SetUnscopablesSymbol(thread_, unscopablesSymbol);

    // Setup %SymbolPrototype%
    SetStringTagSymbol(env, symbolFuncPrototype, "Symbol");

    JSHandle<JSTaggedValue> holeySymbol(factory_->NewPrivateNameSymbolWithChar("holey"));
    env->SetHoleySymbol(thread_, holeySymbol.GetTaggedValue());
    JSHandle<JSTaggedValue> elementIcSymbol(factory_->NewPrivateNameSymbolWithChar("element-ic"));
    env->SetElementICSymbol(thread_, elementIcSymbol.GetTaggedValue());

    // ecma 19.2.3.6 Function.prototype[@@hasInstance] ( V )
    JSHandle<JSObject> funcFuncPrototypeObj = JSHandle<JSObject>(env->GetFunctionPrototype());
    SetFunctionAtSymbol<JSSymbol::SYMBOL_HAS_INSTANCE_TYPE>(
        env, funcFuncPrototypeObj, env->GetHasInstanceSymbol(), "[Symbol.hasInstance]",
        Function::FunctionPrototypeHasInstance, FunctionLength::ONE);
}

void Builtins::InitializeSymbolWithRealm(const JSHandle<GlobalEnv> &realm,
                                         const JSHandle<JSHClass> &objFuncInstanceDynclass)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    // Symbol.prototype
    JSHandle<JSObject> symbolFuncPrototype = factory_->NewJSObject(objFuncInstanceDynclass);
    JSHandle<JSTaggedValue> symbolFuncPrototypeValue(symbolFuncPrototype);

    // Symbol.prototype_or_dynclass
    JSHandle<JSHClass> symbolFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, symbolFuncPrototypeValue);

    // Symbol = new Function()
    JSHandle<JSObject> symbolFunction(
        NewBuiltinConstructor(realm, symbolFuncPrototype, Symbol::SymbolConstructor, "Symbol", FunctionLength::ZERO));
    JSHandle<JSFunction>(symbolFunction)->SetFunctionPrototype(thread_, symbolFuncInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = thread_->GlobalConstants()->GetHandledConstructorString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(symbolFunction), true, false, true);
    JSObject::DefineOwnProperty(thread_, symbolFuncPrototype, constructorKey, descriptor);

    SetFunction(realm, symbolFunction, "for", Symbol::For, FunctionLength::ONE);
    SetFunction(realm, symbolFunction, "keyFor", Symbol::KeyFor, FunctionLength::ONE);

    // Symbol attribute
    SetNoneAttributeProperty(symbolFunction, "hasInstance", env->GetHasInstanceSymbol());
    SetNoneAttributeProperty(symbolFunction, "isConcatSpreadable", env->GetIsConcatSpreadableSymbol());
    SetNoneAttributeProperty(symbolFunction, "toStringTag", env->GetToStringTagSymbol());
    SetNoneAttributeProperty(symbolFunction, "iterator", env->GetIteratorSymbol());
    SetNoneAttributeProperty(symbolFunction, "match", env->GetMatchSymbol());
    SetNoneAttributeProperty(symbolFunction, "replace", env->GetReplaceSymbol());
    SetNoneAttributeProperty(symbolFunction, "search", env->GetSearchSymbol());
    SetNoneAttributeProperty(symbolFunction, "species", env->GetSpeciesSymbol());
    SetNoneAttributeProperty(symbolFunction, "split", env->GetSplitSymbol());
    SetNoneAttributeProperty(symbolFunction, "toPrimitive", env->GetToPrimitiveSymbol());
    SetNoneAttributeProperty(symbolFunction, "unscopables", env->GetUnscopablesSymbol());

    // symbol.prototype.description
    PropertyDescriptor descriptionDesc(thread_);
    JSHandle<JSTaggedValue> getterKey(factory_->NewFromCanBeCompressString("description"));
    JSHandle<JSTaggedValue> getter(factory_->NewJSFunction(realm, reinterpret_cast<void *>(Symbol::DescriptionGetter)));
    SetGetter(symbolFuncPrototype, getterKey, getter);

    // Setup symbol.prototype[@@toPrimitive]
    SetFunctionAtSymbol<JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE>(realm, symbolFuncPrototype, env->GetToPrimitiveSymbol(),
                                                            "[Symbol.toPrimitive]", Symbol::ToPrimitive,
                                                            FunctionLength::ONE);
    // install the Symbol.prototype methods
    SetFunction(realm, symbolFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Symbol::ToString,
                FunctionLength::ZERO);
    SetFunction(realm, symbolFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(), Symbol::ValueOf,
                FunctionLength::ZERO);

    realm->SetSymbolFunction(thread_, symbolFunction);
    realm->SetHasInstanceSymbol(thread_, env->GetHasInstanceSymbol());
    realm->SetIsConcatSpreadableSymbol(thread_, env->GetIsConcatSpreadableSymbol());
    realm->SetToStringTagSymbol(thread_, env->GetToStringTagSymbol());
    realm->SetIteratorSymbol(thread_, env->GetIteratorSymbol());
    realm->SetMatchSymbol(thread_, env->GetMatchSymbol());
    realm->SetReplaceSymbol(thread_, env->GetReplaceSymbol());
    realm->SetSearchSymbol(thread_, env->GetSearchSymbol());
    realm->SetSpeciesSymbol(thread_, env->GetSpeciesSymbol());
    realm->SetSplitSymbol(thread_, env->GetSplitSymbol());
    realm->SetToPrimitiveSymbol(thread_, env->GetToPrimitiveSymbol());
    realm->SetUnscopablesSymbol(thread_, env->GetUnscopablesSymbol());

    // Setup %SymbolPrototype%
    SetStringTagSymbol(realm, symbolFuncPrototype, "Symbol");

    JSHandle<JSTaggedValue> holeySymbol(factory_->NewPrivateNameSymbolWithChar("holey"));
    realm->SetHoleySymbol(thread_, holeySymbol.GetTaggedValue());
    JSHandle<JSTaggedValue> elementIcSymbol(factory_->NewPrivateNameSymbolWithChar("element-ic"));
    realm->SetElementICSymbol(thread_, elementIcSymbol.GetTaggedValue());

    // ecma 19.2.3.6 Function.prototype[@@hasInstance] ( V )
    JSHandle<JSObject> funcFuncPrototypeObj = JSHandle<JSObject>(realm->GetFunctionPrototype());
    SetFunctionAtSymbol<JSSymbol::SYMBOL_HAS_INSTANCE_TYPE>(
        realm, funcFuncPrototypeObj, realm->GetHasInstanceSymbol(), "[Symbol.hasInstance]",
        Function::FunctionPrototypeHasInstance, FunctionLength::ONE);
}

void Builtins::InitializeNumber(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject,
                                const JSHandle<JSHClass> &primRefObjDynclass)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Number.prototype
    JSHandle<JSTaggedValue> toObject(thread_, JSTaggedValue(FunctionLength::ZERO));
    JSHandle<JSObject> numFuncPrototype =
        JSHandle<JSObject>::Cast(factory_->NewJSPrimitiveRef(primRefObjDynclass, toObject));
    JSHandle<JSTaggedValue> numFuncPrototypeValue(numFuncPrototype);

    // Number.prototype_or_dynclass
    JSHandle<JSHClass> numFuncInstanceClass =
        factory_->NewEcmaDynClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, numFuncPrototypeValue);

    // Number = new Function()
    JSHandle<JSObject> numFunction(
        NewBuiltinConstructor(env, numFuncPrototype, Number::NumberConstructor, "Number", FunctionLength::ONE));
    numFunction.GetObject<JSFunction>()->SetFunctionPrototype(thread_, numFuncInstanceClass.GetTaggedValue());

    // Number.prototype method
    SetFunction(env, numFuncPrototype, "toExponential", Number::ToExponential, FunctionLength::ONE);
    SetFunction(env, numFuncPrototype, "toFixed", Number::ToFixed, FunctionLength::ONE);
    SetFunction(env, numFuncPrototype, "toLocaleString", Number::ToLocaleString, FunctionLength::ZERO);
    SetFunction(env, numFuncPrototype, "toPrecision", Number::ToPrecision, FunctionLength::ONE);
    SetFunction(env, numFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Number::ToString,
                FunctionLength::ONE);
    SetFunction(env, numFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(), Number::ValueOf,
                FunctionLength::ZERO);

    // Number method
    SetFunction(env, numFunction, "isFinite", Number::IsFinite, FunctionLength::ONE);
    SetFunction(env, numFunction, "isInteger", Number::IsInteger, FunctionLength::ONE);
    SetFunction(env, numFunction, "isNaN", Number::IsNaN, FunctionLength::ONE);
    SetFunction(env, numFunction, "isSafeInteger", Number::IsSafeInteger, FunctionLength::ONE);
    SetFuncToObjAndGlobal(env, globalObject, numFunction, "parseFloat", Number::ParseFloat, FunctionLength::ONE);
    SetFuncToObjAndGlobal(env, globalObject, numFunction, "parseInt", Number::ParseInt, FunctionLength::TWO);

    // Number constant
    const double epsilon = 2.220446049250313e-16;
    const double maxSafeInteger = 9007199254740991;
    const double maxValue = 1.7976931348623157e+308;
    const double minValue = 5e-324;
    const double positiveInfinity = std::numeric_limits<double>::infinity();
    SetConstant(numFunction, "MAX_VALUE", JSTaggedValue(maxValue));
    SetConstant(numFunction, "MIN_VALUE", JSTaggedValue(minValue));
    SetConstant(numFunction, "NaN", JSTaggedValue(NAN));
    SetConstant(numFunction, "NEGATIVE_INFINITY", JSTaggedValue(-positiveInfinity));
    SetConstant(numFunction, "POSITIVE_INFINITY", JSTaggedValue(positiveInfinity));
    SetConstant(numFunction, "MAX_SAFE_INTEGER", JSTaggedValue(maxSafeInteger));
    SetConstant(numFunction, "MIN_SAFE_INTEGER", JSTaggedValue(-maxSafeInteger));
    SetConstant(numFunction, "EPSILON", JSTaggedValue(epsilon));

    env->SetNumberFunction(thread_, numFunction);
}

void Builtins::InitializeDate(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const int utcLength = 7;
    // Date.prototype
    JSHandle<JSObject> dateFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> dateFuncPrototypeValue(dateFuncPrototype);

    // Date.prototype_or_dynclass
    JSHandle<JSHClass> dateFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSDate::SIZE, JSType::JS_DATE, dateFuncPrototypeValue);

    // Date = new Function()
    JSHandle<JSObject> dateFunction(
        NewBuiltinConstructor(env, dateFuncPrototype, Date::DateConstructor, "Date", FunctionLength::ONE));
    JSHandle<JSFunction>(dateFunction)->SetFunctionPrototype(thread_, dateFuncInstanceDynclass.GetTaggedValue());

    // Date.prototype method
    SetFunction(env, dateFuncPrototype, "getDate", Date::GetDate, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getDay", Date::GetDay, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getFullYear", Date::GetFullYear, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getHours", Date::GetHours, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getMilliseconds", Date::GetMilliseconds, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getMinutes", Date::GetMinutes, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getMonth", Date::GetMonth, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getSeconds", Date::GetSeconds, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getTime", Date::GetTime, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getTimezoneOffset", Date::GetTimezoneOffset, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCDate", Date::GetUTCDate, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCDay", Date::GetUTCDay, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCFullYear", Date::GetUTCFullYear, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCHours", Date::GetUTCHours, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCMilliseconds", Date::GetUTCMilliseconds, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCMinutes", Date::GetUTCMinutes, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCMonth", Date::GetUTCMonth, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "getUTCSeconds", Date::GetUTCSeconds, FunctionLength::ZERO);

    SetFunction(env, dateFuncPrototype, "setDate", Date::SetDate, FunctionLength::ONE);
    SetFunction(env, dateFuncPrototype, "setFullYear", Date::SetFullYear, FunctionLength::THREE);
    SetFunction(env, dateFuncPrototype, "setHours", Date::SetHours, FunctionLength::FOUR);
    SetFunction(env, dateFuncPrototype, "setMilliseconds", Date::SetMilliseconds, FunctionLength::ONE);
    SetFunction(env, dateFuncPrototype, "setMinutes", Date::SetMinutes, FunctionLength::THREE);
    SetFunction(env, dateFuncPrototype, "setMonth", Date::SetMonth, FunctionLength::TWO);
    SetFunction(env, dateFuncPrototype, "setSeconds", Date::SetSeconds, FunctionLength::TWO);
    SetFunction(env, dateFuncPrototype, "setTime", Date::SetTime, FunctionLength::ONE);
    SetFunction(env, dateFuncPrototype, "setUTCDate", Date::SetUTCDate, FunctionLength::ONE);
    SetFunction(env, dateFuncPrototype, "setUTCFullYear", Date::SetUTCFullYear, FunctionLength::THREE);
    SetFunction(env, dateFuncPrototype, "setUTCHours", Date::SetUTCHours, FunctionLength::FOUR);
    SetFunction(env, dateFuncPrototype, "setUTCMilliseconds", Date::SetUTCMilliseconds, FunctionLength::ONE);
    SetFunction(env, dateFuncPrototype, "setUTCMinutes", Date::SetUTCMinutes, FunctionLength::THREE);
    SetFunction(env, dateFuncPrototype, "setUTCMonth", Date::SetUTCMonth, FunctionLength::TWO);
    SetFunction(env, dateFuncPrototype, "setUTCSeconds", Date::SetUTCSeconds, FunctionLength::TWO);

    SetFunction(env, dateFuncPrototype, "toDateString", Date::ToDateString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "toISOString", Date::ToISOString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "toJSON", Date::ToJSON, FunctionLength::ONE);
    SetFunction(env, dateFuncPrototype, "toLocaleDateString", Date::ToLocaleDateString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "toLocaleString", Date::ToLocaleString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "toLocaleTimeString", Date::ToLocaleTimeString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Date::ToString,
                FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "toTimeString", Date::ToTimeString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, "toUTCString", Date::ToUTCString, FunctionLength::ZERO);
    SetFunction(env, dateFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(), Date::ValueOf,
                FunctionLength::ZERO);

    SetFunctionAtSymbol(env, dateFuncPrototype, env->GetToPrimitiveSymbol(), "[Symbol.toPrimitive]", Date::ToPrimitive,
                        FunctionLength::ONE);

    // Date method
    SetFunction(env, dateFunction, "now", Date::Now, FunctionLength::ZERO);
    SetFunction(env, dateFunction, "parse", Date::Parse, FunctionLength::ONE);
    SetFunction(env, dateFunction, "UTC", Date::UTC, utcLength);

    // Date.length
    SetConstant(dateFunction, "length", JSTaggedValue(utcLength));

    env->SetDateFunction(thread_, dateFunction);
}

void Builtins::InitializeBoolean(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &primRefObjDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Boolean.prototype
    JSHandle<JSTaggedValue> toObject(thread_, JSTaggedValue::False());
    JSHandle<JSObject> booleanFuncPrototype =
        JSHandle<JSObject>::Cast(factory_->NewJSPrimitiveRef(primRefObjDynclass, toObject));
    JSHandle<JSTaggedValue> booleanFuncPrototypeValue(booleanFuncPrototype);

    // Boolean.prototype_or_dynclass
    JSHandle<JSHClass> booleanFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, booleanFuncPrototypeValue);

    // new Boolean Function()
    JSHandle<JSFunction> booleanFunction(
        NewBuiltinConstructor(env, booleanFuncPrototype, Boolean::BooleanConstructor, "Boolean", FunctionLength::ONE));
    booleanFunction->SetFunctionPrototype(thread_, booleanFuncInstanceDynclass.GetTaggedValue());

    // Boolean.prototype method
    SetFunction(env, booleanFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                Boolean::BooleanPrototypeToString, FunctionLength::ZERO);
    SetFunction(env, booleanFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(),
                Boolean::BooleanPrototypeValueOf, FunctionLength::ZERO);

    env->SetBooleanFunction(thread_, booleanFunction);
}

void Builtins::InitializeProxy(const JSHandle<GlobalEnv> &env)
{
    JSHandle<JSObject> proxyFunction(InitializeExoticConstructor(env, Proxy::ProxyConstructor, "Proxy", 2));

    // Proxy method
    SetFunction(env, proxyFunction, "revocable", Proxy::Revocable, FunctionLength::TWO);
    env->SetProxyFunction(thread_, proxyFunction);
}

JSHandle<JSFunction> Builtins::InitializeExoticConstructor(const JSHandle<GlobalEnv> &env, EcmaEntrypoint ctorFunc,
                                                           const char *name, int length)
{
    JSHandle<JSFunction> ctor =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_PROXY_CONSTRUCTOR);

    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromString(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));

    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor);
    return ctor;
}

void Builtins::InitializeAsyncFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // AsyncFunction.prototype
    JSHandle<JSObject> asyncFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSObject::SetPrototype(thread_, asyncFuncPrototype, env->GetFunctionPrototype());
    JSHandle<JSTaggedValue> async_func_prototype_value(asyncFuncPrototype);

    // AsyncFunction.prototype_or_dynclass
    JSHandle<JSHClass> asyncFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSAsyncFunction::SIZE, JSType::JS_ASYNC_FUNCTION, async_func_prototype_value);

    // AsyncFunction = new Function()
    JSHandle<JSFunction> asyncFunction = NewBuiltinConstructor(
        env, asyncFuncPrototype, AsyncFunction::AsyncFunctionConstructor, "AsyncFunction", FunctionLength::ONE);
    JSObject::SetPrototype(thread_, JSHandle<JSObject>::Cast(asyncFunction), env->GetFunctionFunction());
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor asyncDesc(thread_, JSHandle<JSTaggedValue>::Cast(asyncFunction), false, false, true);
    JSObject::DefineOwnProperty(thread_, asyncFuncPrototype, constructorKey, asyncDesc);
    asyncFunction->SetProtoOrDynClass(thread_, asyncFuncInstanceDynclass.GetTaggedValue());

    // AsyncFunction.prototype property
    SetStringTagSymbol(env, asyncFuncPrototype, "AsyncFunction");
    env->SetAsyncFunction(thread_, asyncFunction);
    env->SetAsyncFunctionPrototype(thread_, asyncFuncPrototype);
}

void Builtins::InitializeAllTypeError(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    // Error.prototype
    JSHandle<JSObject> errorFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> errorFuncPrototypeValue(errorFuncPrototype);
    // Error.prototype_or_dynclass
    JSHandle<JSHClass> errorFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_ERROR, errorFuncPrototypeValue);
    // Error() = new Function()
    JSHandle<JSFunction> errorFunction(
        NewBuiltinConstructor(env, errorFuncPrototype, Error::ErrorConstructor, "Error", FunctionLength::ONE));
    errorFunction->SetFunctionPrototype(thread_, errorFuncInstanceDynclass.GetTaggedValue());

    // Error.prototype method
    SetFunction(env, errorFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Error::ToString,
                FunctionLength::ZERO);

    // Error.prototype Attribute
    SetAttribute(errorFuncPrototype, "name", "Error");
    SetAttribute(errorFuncPrototype, "message", "");
    env->SetErrorFunction(thread_, errorFunction);

    JSHandle<JSHClass> nativeErrorFuncClass =
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetErrorFunction());
    nativeErrorFuncClass->SetConstructor(true);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(factory_->NewJSObject(nativeErrorFuncClass));
    function->SetBuiltinsCtorMode();
    env->SetNativeErrorFunctionClass(thread_, nativeErrorFuncClass);

    JSHandle<JSHClass> errorNativeFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, errorFuncPrototypeValue);
    InitializeError(env, errorNativeFuncInstanceDynclass, JSType::JS_RANGE_ERROR);
    InitializeError(env, errorNativeFuncInstanceDynclass, JSType::JS_REFERENCE_ERROR);
    InitializeError(env, errorNativeFuncInstanceDynclass, JSType::JS_TYPE_ERROR);
    InitializeError(env, errorNativeFuncInstanceDynclass, JSType::JS_URI_ERROR);
    InitializeError(env, errorNativeFuncInstanceDynclass, JSType::JS_SYNTAX_ERROR);
    InitializeError(env, errorNativeFuncInstanceDynclass, JSType::JS_EVAL_ERROR);
}

void Builtins::InitializeAllTypeErrorWithRealm(const JSHandle<GlobalEnv> &realm) const
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    realm->SetErrorFunction(thread_, env->GetErrorFunction());
    realm->SetNativeErrorFunctionClass(thread_, env->GetNativeErrorFunctionClass());

    SetErrorWithRealm(realm, JSType::JS_RANGE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_REFERENCE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_TYPE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_URI_ERROR);
    SetErrorWithRealm(realm, JSType::JS_SYNTAX_ERROR);
    SetErrorWithRealm(realm, JSType::JS_EVAL_ERROR);
}

void Builtins::SetErrorWithRealm(const JSHandle<GlobalEnv> &realm, const JSType &errorTag) const
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread_, realm->GetGlobalObject());
    JSHandle<JSTaggedValue> nameString;
    JSHandle<JSTaggedValue> nativeErrorFunction;
    switch (errorTag) {
        case JSType::JS_RANGE_ERROR:
            nativeErrorFunction = env->GetRangeErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledRangeErrorString());
            realm->SetRangeErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_EVAL_ERROR:
            nativeErrorFunction = env->GetEvalErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledEvalErrorString());
            realm->SetEvalErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_REFERENCE_ERROR:
            nativeErrorFunction = env->GetReferenceErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledReferenceErrorString());
            realm->SetReferenceErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_TYPE_ERROR:
            nativeErrorFunction = env->GetTypeErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledTypeErrorString());
            realm->SetTypeErrorFunction(thread_, nativeErrorFunction);
            realm->SetThrowTypeError(thread_, env->GetThrowTypeError());
            break;
        case JSType::JS_URI_ERROR:
            nativeErrorFunction = env->GetURIErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledURIErrorString());
            realm->SetURIErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_SYNTAX_ERROR:
            nativeErrorFunction = env->GetSyntaxErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledSyntaxErrorString());
            realm->SetSyntaxErrorFunction(thread_, nativeErrorFunction);
            break;
        default:
            break;
    }
    PropertyDescriptor descriptor(thread_, nativeErrorFunction, true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor);
}

void Builtins::GeneralUpdateError(ErrorParameter *error, EcmaEntrypoint constructor, EcmaEntrypoint method,
                                  const char *name, JSType type) const
{
    error->nativeConstructor = constructor;
    error->nativeMethod = method;
    error->nativePropertyName = name;
    error->nativeJstype = type;
}

void Builtins::InitializeError(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass,
                               const JSType &errorTag) const
{
    // NativeError.prototype
    JSHandle<JSObject> nativeErrorFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> nativeErrorFuncPrototypeValue(nativeErrorFuncPrototype);

    ErrorParameter errorParameter{RangeError::RangeErrorConstructor, RangeError::ToString, "RangeError",
                                  JSType::JS_RANGE_ERROR};
    switch (errorTag) {
        case JSType::JS_RANGE_ERROR:
            GeneralUpdateError(&errorParameter, RangeError::RangeErrorConstructor, RangeError::ToString, "RangeError",
                               JSType::JS_RANGE_ERROR);
            break;
        case JSType::JS_EVAL_ERROR:
            GeneralUpdateError(&errorParameter, EvalError::EvalErrorConstructor, EvalError::ToString, "EvalError",
                               JSType::JS_EVAL_ERROR);
            break;
        case JSType::JS_REFERENCE_ERROR:
            GeneralUpdateError(&errorParameter, ReferenceError::ReferenceErrorConstructor, ReferenceError::ToString,
                               "ReferenceError", JSType::JS_REFERENCE_ERROR);
            break;
        case JSType::JS_TYPE_ERROR:
            GeneralUpdateError(&errorParameter, TypeError::TypeErrorConstructor, TypeError::ToString, "TypeError",
                               JSType::JS_TYPE_ERROR);
            break;
        case JSType::JS_URI_ERROR:
            GeneralUpdateError(&errorParameter, URIError::URIErrorConstructor, URIError::ToString, "URIError",
                               JSType::JS_URI_ERROR);
            break;
        case JSType::JS_SYNTAX_ERROR:
            GeneralUpdateError(&errorParameter, SyntaxError::SyntaxErrorConstructor, SyntaxError::ToString,
                               "SyntaxError", JSType::JS_SYNTAX_ERROR);
            break;
        default:
            break;
    }

    // NativeError.prototype_or_dynclass
    JSHandle<JSHClass> nativeErrorFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, errorParameter.nativeJstype, nativeErrorFuncPrototypeValue);

    // NativeError() = new Error()
    JSHandle<JSFunction> nativeErrorFunction =
        factory_->NewJSNativeErrorFunction(env, reinterpret_cast<void *>(errorParameter.nativeConstructor));
    InitializeCtor(env, nativeErrorFuncPrototype, nativeErrorFunction, errorParameter.nativePropertyName,
                   FunctionLength::ONE);

    nativeErrorFunction->SetFunctionPrototype(thread_, nativeErrorFuncInstanceDynclass.GetTaggedValue());

    // NativeError.prototype method
    SetFunction(env, nativeErrorFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                errorParameter.nativeMethod, FunctionLength::ZERO);

    // Error.prototype Attribute
    SetAttribute(nativeErrorFuncPrototype, "name", errorParameter.nativePropertyName);
    SetAttribute(nativeErrorFuncPrototype, "message", "");

    if (errorTag == JSType::JS_RANGE_ERROR) {
        env->SetRangeErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_REFERENCE_ERROR) {
        env->SetReferenceErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_TYPE_ERROR) {
        env->SetTypeErrorFunction(thread_, nativeErrorFunction);
        JSHandle<JSFunction> throwTypeErrorFunction =
            factory_->NewJSFunction(env, reinterpret_cast<void *>(TypeError::ThrowTypeError));
        JSFunction::SetFunctionLength(thread_, throwTypeErrorFunction, JSTaggedValue(1), false);
        JSObject::PreventExtensions(thread_, JSHandle<JSObject>::Cast(throwTypeErrorFunction));
        env->SetThrowTypeError(thread_, throwTypeErrorFunction);
    } else if (errorTag == JSType::JS_URI_ERROR) {
        env->SetURIErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_SYNTAX_ERROR) {
        env->SetSyntaxErrorFunction(thread_, nativeErrorFunction);
    } else {
        env->SetEvalErrorFunction(thread_, nativeErrorFunction);
    }
}  // namespace panda::ecmascript

void Builtins::InitializeCtor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                              const JSHandle<JSFunction> &ctor, const char *name, int length) const
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromString(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor1(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, prototype, constructorKey, descriptor1);

    /* set "prototype" in constructor */
    ctor->SetFunctionPrototype(thread_, prototype.GetTaggedValue());

    if (!JSTaggedValue::SameValue(nameString, thread_->GlobalConstants()->GetHandledAsyncFunctionString())) {
        JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
        PropertyDescriptor descriptor2(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor2);
    }
}

void Builtins::InitializeSet(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Set.prototype
    JSHandle<JSObject> setFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> setFuncPrototypeValue(setFuncPrototype);
    // Set.prototype_or_dynclass
    JSHandle<JSHClass> setFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSSet::SIZE, JSType::JS_SET, setFuncPrototypeValue);
    // Set() = new Function()
    JSHandle<JSTaggedValue> setFunction(
        NewBuiltinConstructor(env, setFuncPrototype, BuiltinsSet::SetConstructor, "Set", FunctionLength::ZERO));
    JSHandle<JSFunction>(setFunction)->SetFunctionPrototype(thread_, setFuncInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(setFuncPrototype), constructorKey, setFunction);
    // set.prototype.add()
    SetFunction(env, setFuncPrototype, "add", BuiltinsSet::Add, FunctionLength::ONE);
    // set.prototype.clear()
    SetFunction(env, setFuncPrototype, "clear", BuiltinsSet::Clear, FunctionLength::ZERO);
    // set.prototype.delete()
    SetFunction(env, setFuncPrototype, "delete", BuiltinsSet::Delete, FunctionLength::ONE);
    // set.prototype.has()
    SetFunction(env, setFuncPrototype, "has", BuiltinsSet::Has, FunctionLength::ONE);
    // set.prototype.forEach()
    SetFunction(env, setFuncPrototype, "forEach", BuiltinsSet::ForEach, FunctionLength::ONE);
    // set.prototype.entries()
    SetFunction(env, setFuncPrototype, "entries", BuiltinsSet::Entries, FunctionLength::ZERO);
    // set.prototype.keys()
    SetFunction(env, setFuncPrototype, "values", BuiltinsSet::Values, FunctionLength::ZERO);
    // set.prototype.values()
    JSHandle<JSTaggedValue> keys(factory_->NewFromCanBeCompressString("keys"));
    JSHandle<JSTaggedValue> values(factory_->NewFromCanBeCompressString("values"));
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(setFuncPrototype), values);
    PropertyDescriptor descriptor(thread_, valuesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, setFuncPrototype, keys, descriptor);

    // @@ToStringTag
    SetStringTagSymbol(env, setFuncPrototype, "Set");

    // 23.1.3.10get Set.prototype.size
    JSHandle<JSTaggedValue> sizeGetter = CreateGetter(env, BuiltinsSet::GetSize, "size", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> sizeKey(factory_->NewFromCanBeCompressString("size"));
    SetGetter(setFuncPrototype, sizeKey, sizeGetter);

    // 23.1.2.2get Set [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsSet::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(setFunction), speciesSymbol, speciesGetter);

    // %SetPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSObject::DefineOwnProperty(thread_, setFuncPrototype, iteratorSymbol, descriptor);

    env->SetBuiltinsSetFunction(thread_, setFunction);
}

void Builtins::InitializeMap(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Map.prototype
    JSHandle<JSObject> mapFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> mapFuncPrototypeValue(mapFuncPrototype);
    // Map.prototype_or_dynclass
    JSHandle<JSHClass> mapFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSMap::SIZE, JSType::JS_MAP, mapFuncPrototypeValue);
    // Map() = new Function()
    JSHandle<JSTaggedValue> mapFunction(
        NewBuiltinConstructor(env, mapFuncPrototype, BuiltinsMap::MapConstructor, "Map", FunctionLength::ZERO));
    // Map().prototype = Map.Prototype & Map.prototype.constructor = Map()
    JSFunction::Cast(mapFunction->GetTaggedObject())
        ->SetFunctionPrototype(thread_, mapFuncInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(mapFuncPrototype), constructorKey, mapFunction);
    // map.prototype.set()
    SetFunction(env, mapFuncPrototype, globalConst->GetHandledSetString(), BuiltinsMap::Set, FunctionLength::TWO);
    // map.prototype.clear()
    SetFunction(env, mapFuncPrototype, "clear", BuiltinsMap::Clear, FunctionLength::ZERO);
    // map.prototype.delete()
    SetFunction(env, mapFuncPrototype, "delete", BuiltinsMap::Delete, FunctionLength::ONE);
    // map.prototype.has()
    SetFunction(env, mapFuncPrototype, "has", BuiltinsMap::Has, FunctionLength::ONE);
    // map.prototype.get()
    SetFunction(env, mapFuncPrototype, thread_->GlobalConstants()->GetHandledGetString(), BuiltinsMap::Get,
                FunctionLength::ONE);
    // map.prototype.forEach()
    SetFunction(env, mapFuncPrototype, "forEach", BuiltinsMap::ForEach, FunctionLength::ONE);
    // map.prototype.keys()
    SetFunction(env, mapFuncPrototype, "keys", BuiltinsMap::Keys, FunctionLength::ZERO);
    // map.prototype.values()
    SetFunction(env, mapFuncPrototype, "values", BuiltinsMap::Values, FunctionLength::ZERO);
    // map.prototype.entries()
    SetFunction(env, mapFuncPrototype, "entries", BuiltinsMap::Entries, FunctionLength::ZERO);
    // @@ToStringTag
    SetStringTagSymbol(env, mapFuncPrototype, "Map");

    // 23.1.3.10get Map.prototype.size
    JSHandle<JSTaggedValue> sizeGetter = CreateGetter(env, BuiltinsMap::GetSize, "size", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> sizeKey(factory_->NewFromCanBeCompressString("size"));
    SetGetter(mapFuncPrototype, sizeKey, sizeGetter);

    // 23.1.2.2get Map [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsMap::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(mapFunction), speciesSymbol, speciesGetter);

    // %MapPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> entries(factory_->NewFromCanBeCompressString("entries"));
    JSHandle<JSTaggedValue> entriesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(mapFuncPrototype), entries);
    PropertyDescriptor descriptor(thread_, entriesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, mapFuncPrototype, iteratorSymbol, descriptor);

    env->SetBuiltinsMapFunction(thread_, mapFunction);
    env->SetMapPrototype(thread_, mapFuncPrototype);
}

void Builtins::InitializeWeakMap(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // WeakMap.prototype
    JSHandle<JSObject> weakMapFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> weakMapFuncPrototypeValue(weakMapFuncPrototype);
    // WeakMap.prototype_or_dynclass
    JSHandle<JSHClass> weakMapFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSWeakMap::SIZE, JSType::JS_WEAK_MAP, weakMapFuncPrototypeValue);
    // WeakMap() = new Function()
    JSHandle<JSTaggedValue> weakMapFunction(NewBuiltinConstructor(
        env, weakMapFuncPrototype, BuiltinsWeakMap::WeakMapConstructor, "WeakMap", FunctionLength::ZERO));
    // WeakMap().prototype = WeakMap.Prototype & WeakMap.prototype.constructor = WeakMap()
    JSFunction::Cast(weakMapFunction->GetTaggedObject())
        ->SetProtoOrDynClass(thread_, weakMapFuncInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(weakMapFuncPrototype), constructorKey, weakMapFunction);
    // weakmap.prototype.set()
    SetFunction(env, weakMapFuncPrototype, globalConst->GetHandledSetString(), BuiltinsWeakMap::Set,
                FunctionLength::TWO);
    // weakmap.prototype.delete()
    SetFunction(env, weakMapFuncPrototype, "delete", BuiltinsWeakMap::Delete, FunctionLength::ONE);
    // weakmap.prototype.has()
    SetFunction(env, weakMapFuncPrototype, "has", BuiltinsWeakMap::Has, FunctionLength::ONE);
    // weakmap.prototype.get()
    SetFunction(env, weakMapFuncPrototype, thread_->GlobalConstants()->GetHandledGetString(), BuiltinsWeakMap::Get,
                FunctionLength::ONE);
    // @@ToStringTag
    SetStringTagSymbol(env, weakMapFuncPrototype, "WeakMap");

    env->SetBuiltinsWeakMapFunction(thread_, weakMapFunction);
}

void Builtins::InitializeWeakSet(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Set.prototype
    JSHandle<JSObject> weakSetFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> weakSetFuncPrototypeValue(weakSetFuncPrototype);
    // Set.prototype_or_dynclass
    JSHandle<JSHClass> weakSetFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSWeakSet::SIZE, JSType::JS_WEAK_SET, weakSetFuncPrototypeValue);
    // Set() = new Function()
    JSHandle<JSTaggedValue> weakSetFunction(NewBuiltinConstructor(
        env, weakSetFuncPrototype, BuiltinsWeakSet::WeakSetConstructor, "WeakSet", FunctionLength::ZERO));
    JSHandle<JSFunction>(weakSetFunction)->SetProtoOrDynClass(thread_, weakSetFuncInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(weakSetFuncPrototype), constructorKey, weakSetFunction);
    // set.prototype.add()
    SetFunction(env, weakSetFuncPrototype, "add", BuiltinsWeakSet::Add, FunctionLength::ONE);
    // set.prototype.delete()
    SetFunction(env, weakSetFuncPrototype, "delete", BuiltinsWeakSet::Delete, FunctionLength::ONE);
    // set.prototype.has()
    SetFunction(env, weakSetFuncPrototype, "has", BuiltinsWeakSet::Has, FunctionLength::ONE);

    // @@ToStringTag
    SetStringTagSymbol(env, weakSetFuncPrototype, "WeakSet");

    env->SetBuiltinsWeakSetFunction(thread_, weakSetFunction);
}

void Builtins::InitializeMath(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> mathDynclass = factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> mathObject = factory_->NewJSObject(mathDynclass);
    SetFunction(env, mathObject, "abs", Math::Abs, FunctionLength::ONE);
    SetFunction(env, mathObject, "acos", Math::Acos, FunctionLength::ONE);
    SetFunction(env, mathObject, "acosh", Math::Acosh, FunctionLength::ONE);
    SetFunction(env, mathObject, "asin", Math::Asin, FunctionLength::ONE);
    SetFunction(env, mathObject, "asinh", Math::Asinh, FunctionLength::ONE);
    SetFunction(env, mathObject, "atan", Math::Atan, FunctionLength::ONE);
    SetFunction(env, mathObject, "atanh", Math::Atanh, FunctionLength::ONE);
    SetFunction(env, mathObject, "atan2", Math::Atan2, FunctionLength::TWO);
    SetFunction(env, mathObject, "cbrt", Math::Cbrt, FunctionLength::ONE);
    SetFunction(env, mathObject, "ceil", Math::Ceil, FunctionLength::ONE);
    SetFunction(env, mathObject, "clz32", Math::Clz32, FunctionLength::ONE);
    SetFunction(env, mathObject, "cos", Math::Cos, FunctionLength::ONE);
    SetFunction(env, mathObject, "cosh", Math::Cosh, FunctionLength::ONE);
    SetFunction(env, mathObject, "exp", Math::Exp, FunctionLength::ONE);
    SetFunction(env, mathObject, "expm1", Math::Expm1, FunctionLength::ONE);
    SetFunction(env, mathObject, "floor", Math::Floor, FunctionLength::ONE);
    SetFunction(env, mathObject, "fround", Math::Fround, FunctionLength::ONE);
    SetFunction(env, mathObject, "hypot", Math::Hypot, FunctionLength::TWO);
    SetFunction(env, mathObject, "imul", Math::Imul, FunctionLength::TWO);
    SetFunction(env, mathObject, "log", Math::Log, FunctionLength::ONE);
    SetFunction(env, mathObject, "log1p", Math::Log1p, FunctionLength::ONE);
    SetFunction(env, mathObject, "log10", Math::Log10, FunctionLength::ONE);
    SetFunction(env, mathObject, "log2", Math::Log2, FunctionLength::ONE);
    SetFunction(env, mathObject, "max", Math::Max, FunctionLength::TWO);
    SetFunction(env, mathObject, "min", Math::Min, FunctionLength::TWO);
    SetFunction(env, mathObject, "pow", Math::Pow, FunctionLength::TWO);
    SetFunction(env, mathObject, "random", Math::Random, FunctionLength::ZERO);
    SetFunction(env, mathObject, "round", Math::Round, FunctionLength::ONE);
    SetFunction(env, mathObject, "sign", Math::Sign, FunctionLength::ONE);
    SetFunction(env, mathObject, "sin", Math::Sin, FunctionLength::ONE);
    SetFunction(env, mathObject, "sinh", Math::Sinh, FunctionLength::ONE);
    SetFunction(env, mathObject, "sqrt", Math::Sqrt, FunctionLength::ONE);
    SetFunction(env, mathObject, "tan", Math::Tan, FunctionLength::ONE);
    SetFunction(env, mathObject, "tanh", Math::Tanh, FunctionLength::ONE);
    SetFunction(env, mathObject, "trunc", Math::Trunc, FunctionLength::ONE);

    SetConstant(mathObject, "E", JSTaggedValue(Math::E));
    SetConstant(mathObject, "LN10", JSTaggedValue(Math::LN10));
    SetConstant(mathObject, "LN2", JSTaggedValue(Math::LN2));
    SetConstant(mathObject, "LOG10E", JSTaggedValue(Math::LOG10E));
    SetConstant(mathObject, "LOG2E", JSTaggedValue(Math::LOG2E));
    SetConstant(mathObject, "PI", JSTaggedValue(Math::PI));
    SetConstant(mathObject, "SQRT1_2", JSTaggedValue(Math::SQRT1_2));
    SetConstant(mathObject, "SQRT2", JSTaggedValue(Math::SQRT2));

    JSHandle<JSTaggedValue> mathString(factory_->NewFromCanBeCompressString("Math"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor mathDesc(thread_, JSHandle<JSTaggedValue>::Cast(mathObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, mathString, mathDesc);
    // @@ToStringTag
    SetStringTagSymbol(env, mathObject, "Math");
    env->SetMathFunction(thread_, mathObject);
}

void Builtins::InitializeJson(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> jsonDynclass = factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> jsonObject = factory_->NewJSObject(jsonDynclass);

    SetFunction(env, jsonObject, "parse", Json::Parse, FunctionLength::TWO);
    SetFunction(env, jsonObject, "stringify", Json::Stringify, FunctionLength::THREE);

    PropertyDescriptor jsonDesc(thread_, JSHandle<JSTaggedValue>::Cast(jsonObject), true, false, true);
    JSHandle<JSTaggedValue> jsonString(factory_->NewFromCanBeCompressString("JSON"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSObject::DefineOwnProperty(thread_, globalObject, jsonString, jsonDesc);
    // @@ToStringTag
    SetStringTagSymbol(env, jsonObject, "JSON");
    env->SetJsonFunction(thread_, jsonObject);
}

void Builtins::InitializeString(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &primRefObjDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // String.prototype
    JSHandle<JSTaggedValue> toObject(factory_->GetEmptyString());
    JSHandle<JSObject> stringFuncPrototype =
        JSHandle<JSObject>::Cast(factory_->NewJSPrimitiveRef(primRefObjDynclass, toObject));
    JSHandle<JSTaggedValue> stringFuncPrototypeValue(stringFuncPrototype);

    // String.prototype_or_dynclass
    JSHandle<JSHClass> stringFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, stringFuncPrototypeValue);

    // String = new Function()
    JSHandle<JSObject> stringFunction(NewBuiltinConstructor(env, stringFuncPrototype, BuiltinsString::StringConstructor,
                                                            "String", FunctionLength::ONE));
    stringFunction.GetObject<JSFunction>()->SetFunctionPrototype(thread_, stringFuncInstanceDynclass.GetTaggedValue());

    // String.prototype method
    SetFunction(env, stringFuncPrototype, "charAt", BuiltinsString::CharAt, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "charCodeAt", BuiltinsString::CharCodeAt, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "codePointAt", BuiltinsString::CodePointAt, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "concat", BuiltinsString::Concat, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "endsWith", BuiltinsString::EndsWith, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "includes", BuiltinsString::Includes, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "indexOf", BuiltinsString::IndexOf, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "lastIndexOf", BuiltinsString::LastIndexOf, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "localeCompare", BuiltinsString::LocaleCompare, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "match", BuiltinsString::Match, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "repeat", BuiltinsString::Repeat, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "normalize", BuiltinsString::Normalize, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, "replace", BuiltinsString::Replace, FunctionLength::TWO);
    SetFunction(env, stringFuncPrototype, "search", BuiltinsString::Search, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "slice", BuiltinsString::Slice, FunctionLength::TWO);
    SetFunction(env, stringFuncPrototype, "split", BuiltinsString::Split, FunctionLength::TWO);
    SetFunction(env, stringFuncPrototype, "startsWith", BuiltinsString::StartsWith, FunctionLength::ONE);
    SetFunction(env, stringFuncPrototype, "substring", BuiltinsString::Substring, FunctionLength::TWO);
    SetFunction(env, stringFuncPrototype, "substr", BuiltinsString::SubStr, FunctionLength::TWO);
    SetFunction(env, stringFuncPrototype, "toLocaleLowerCase", BuiltinsString::ToLocaleLowerCase, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, "toLocaleUpperCase", BuiltinsString::ToLocaleUpperCase, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, "toLowerCase", BuiltinsString::ToLowerCase, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                BuiltinsString::ToString, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, "toUpperCase", BuiltinsString::ToUpperCase, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, "trim", BuiltinsString::Trim, FunctionLength::ZERO);
    SetFunction(env, stringFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(),
                BuiltinsString::ValueOf, FunctionLength::ZERO);
    SetFunctionAtSymbol(env, stringFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        BuiltinsString::GetStringIterator, FunctionLength::ZERO);

    // String method
    SetFunction(env, stringFunction, "fromCharCode", BuiltinsString::FromCharCode, FunctionLength::ONE);
    SetFunction(env, stringFunction, "fromCodePoint", BuiltinsString::FromCodePoint, FunctionLength::ONE);
    SetFunction(env, stringFunction, "raw", BuiltinsString::Raw, FunctionLength::ONE);

    // String.prototype.length
    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, BuiltinsString::GetLength, "length", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromCanBeCompressString("length"));
    SetGetter(stringFuncPrototype, lengthKey, lengthGetter);

    env->SetStringFunction(thread_, stringFunction);
}

void Builtins::InitializeStringIterator(const JSHandle<GlobalEnv> &env,
                                        const JSHandle<JSHClass> &iteratorFuncDynclass) const
{
    // StringIterator.prototype
    JSHandle<JSObject> strIterPrototype(factory_->NewJSObject(iteratorFuncDynclass));

    // StringIterator.prototype_or_dynclass
    JSHandle<JSHClass> strIterFuncInstanceDynclass = factory_->NewEcmaDynClass(
        JSStringIterator::SIZE, JSType::JS_STRING_ITERATOR, JSHandle<JSTaggedValue>(strIterPrototype));

    JSHandle<JSFunction> strIterFunction(
        factory_->NewJSFunction(env, static_cast<void *>(nullptr), FunctionKind::BASE_CONSTRUCTOR));
    strIterFunction->SetFunctionPrototype(thread_, strIterFuncInstanceDynclass.GetTaggedValue());

    SetFunction(env, strIterPrototype, "next", StringIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, strIterPrototype, "String Iterator");

    env->SetStringIterator(thread_, strIterFunction);
    env->SetStringIteratorPrototype(thread_, strIterPrototype);
}

void Builtins::InitializeIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Iterator.prototype
    JSHandle<JSObject> iteratorPrototype = factory_->NewJSObject(objFuncDynclass);
    // Iterator.prototype.next()
    SetFunction(env, iteratorPrototype, "next", BuiltinsIterator::Next, FunctionLength::ONE);
    // Iterator.prototype.return()
    SetFunction(env, iteratorPrototype, "return", BuiltinsIterator::Return, FunctionLength::ONE);
    // Iterator.prototype.throw()
    SetFunction(env, iteratorPrototype, "throw", BuiltinsIterator::Throw, FunctionLength::ONE);
    // %IteratorPrototype% [ @@iterator ]
    SetFunctionAtSymbol(env, iteratorPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        BuiltinsIterator::GetIteratorObj, FunctionLength::ZERO);
    env->SetIteratorPrototype(thread_, iteratorPrototype);

    // Iterator.dynclass
    JSHandle<JSHClass> iteratorFuncDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_ITERATOR, JSHandle<JSTaggedValue>(iteratorPrototype));

    InitializeForinIterator(env, iteratorFuncDynclass);
    InitializeSetIterator(env, iteratorFuncDynclass);
    InitializeMapIterator(env, iteratorFuncDynclass);
    InitializeArrayIterator(env, iteratorFuncDynclass);
    InitializeStringIterator(env, iteratorFuncDynclass);
}

void Builtins::InitializeForinIterator(const JSHandle<GlobalEnv> &env,
                                       const JSHandle<JSHClass> &iteratorFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Iterator.prototype
    JSHandle<JSObject> forinIteratorPrototype = factory_->NewJSObject(iteratorFuncDynclass);
    JSHandle<JSHClass> dynclass = factory_->NewEcmaDynClass(JSForInIterator::SIZE, JSType::JS_FORIN_ITERATOR,
                                                            JSHandle<JSTaggedValue>(forinIteratorPrototype));

    // Iterator.prototype.next()
    SetFunction(env, forinIteratorPrototype, "next", JSForInIterator::Next, FunctionLength::ONE);
    env->SetForinIteratorPrototype(thread_, forinIteratorPrototype);
    env->SetForinIteratorClass(thread_, dynclass);
}

void Builtins::InitializeSetIterator(const JSHandle<GlobalEnv> &env,
                                     const JSHandle<JSHClass> &iteratorFuncDynclass) const
{
    // SetIterator.prototype
    JSHandle<JSObject> setIteratorPrototype(factory_->NewJSObject(iteratorFuncDynclass));
    // Iterator.prototype.next()
    SetFunction(env, setIteratorPrototype, "next", JSSetIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, setIteratorPrototype, "Set Iterator");
    env->SetSetIteratorPrototype(thread_, setIteratorPrototype);
}

void Builtins::InitializeMapIterator(const JSHandle<GlobalEnv> &env,
                                     const JSHandle<JSHClass> &iteratorFuncDynclass) const
{
    // MapIterator.prototype
    JSHandle<JSObject> mapIteratorPrototype(factory_->NewJSObject(iteratorFuncDynclass));
    // Iterator.prototype.next()
    SetFunction(env, mapIteratorPrototype, "next", JSMapIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, mapIteratorPrototype, "Map Iterator");
    env->SetMapIteratorPrototype(thread_, mapIteratorPrototype);
}
void Builtins::InitializeArrayIterator(const JSHandle<GlobalEnv> &env,
                                       const JSHandle<JSHClass> &iteratorFuncDynclass) const
{
    // ArrayIterator.prototype
    JSHandle<JSObject> arrayIteratorPrototype(factory_->NewJSObject(iteratorFuncDynclass));
    // Iterator.prototype.next()
    SetFunction(env, arrayIteratorPrototype, "next", JSArrayIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, arrayIteratorPrototype, "Array Iterator");
    env->SetArrayIteratorPrototype(thread_, arrayIteratorPrototype);
}

void Builtins::InitializeRegExp(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // RegExp.prototype
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> regPrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> regPrototypeValue(regPrototype);

    // RegExp.prototype_or_dynclass
    JSHandle<JSHClass> regexpFuncInstanceDynclass = factory_->CreateJSRegExpInstanceClass(regPrototypeValue);

    // RegExp = new Function()
    JSHandle<JSObject> regexpFunction(
        NewBuiltinConstructor(env, regPrototype, RegExp::RegExpConstructor, "RegExp", FunctionLength::TWO));

    JSHandle<JSFunction>(regexpFunction)->SetFunctionPrototype(thread_, regexpFuncInstanceDynclass.GetTaggedValue());

    // RegExp.prototype method
    SetFunction(env, regPrototype, "exec", RegExp::Exec, FunctionLength::ONE);
    SetFunction(env, regPrototype, "test", RegExp::Test, FunctionLength::ONE);
    SetFunction(env, regPrototype, thread_->GlobalConstants()->GetHandledToStringString(), RegExp::ToString,
                FunctionLength::ZERO);

    JSHandle<JSTaggedValue> flagsGetter = CreateGetter(env, RegExp::GetFlags, "flags", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> flagsKey(factory_->NewFromCanBeCompressString("flags"));
    SetGetter(regPrototype, flagsKey, flagsGetter);

    JSHandle<JSTaggedValue> sourceGetter = CreateGetter(env, RegExp::GetSource, "source", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> sourceKey(factory_->NewFromCanBeCompressString("source"));
    SetGetter(regPrototype, sourceKey, sourceGetter);

    JSHandle<JSTaggedValue> globalGetter = CreateGetter(env, RegExp::GetGlobal, "global", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> globalKey(factory_->NewFromCanBeCompressString("global"));
    SetGetter(regPrototype, globalKey, globalGetter);

    JSHandle<JSTaggedValue> ignoreCaseGetter =
        CreateGetter(env, RegExp::GetIgnoreCase, "ignoreCase", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> ignoreCaseKey(factory_->NewFromCanBeCompressString("ignoreCase"));
    SetGetter(regPrototype, ignoreCaseKey, ignoreCaseGetter);

    JSHandle<JSTaggedValue> multilineGetter =
        CreateGetter(env, RegExp::GetMultiline, "multiline", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> multilineKey(factory_->NewFromCanBeCompressString("multiline"));
    SetGetter(regPrototype, multilineKey, multilineGetter);

    JSHandle<JSTaggedValue> dotAllGetter = CreateGetter(env, RegExp::GetDotAll, "dotAll", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> dotAllKey(factory_->NewFromCanBeCompressString("dotAll"));
    SetGetter(regPrototype, dotAllKey, dotAllGetter);

    JSHandle<JSTaggedValue> stickyGetter = CreateGetter(env, RegExp::GetSticky, "sticky", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> stickyKey(factory_->NewFromCanBeCompressString("sticky"));
    SetGetter(regPrototype, stickyKey, stickyGetter);

    JSHandle<JSTaggedValue> unicodeGetter = CreateGetter(env, RegExp::GetUnicode, "unicode", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> unicodeKey(factory_->NewFromCanBeCompressString("unicode"));
    SetGetter(regPrototype, unicodeKey, unicodeGetter);

    // Set RegExp [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsMap::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(regexpFunction), speciesSymbol, speciesGetter);

    // Set RegExp.prototype[@@split]
    SetFunctionAtSymbol(env, regPrototype, env->GetSplitSymbol(), "[Symbol.split]", RegExp::Split, FunctionLength::TWO);
    // Set RegExp.prototype[@@search]
    SetFunctionAtSymbol(env, regPrototype, env->GetSearchSymbol(), "[Symbol.search]", RegExp::Search,
                        FunctionLength::ONE);
    // Set RegExp.prototype[@@match]
    SetFunctionAtSymbol(env, regPrototype, env->GetMatchSymbol(), "[Symbol.match]", RegExp::Match, FunctionLength::ONE);
    // Set RegExp.prototype[@@replace]
    SetFunctionAtSymbol(env, regPrototype, env->GetReplaceSymbol(), "[Symbol.replace]", RegExp::Replace,
                        FunctionLength::TWO);

    env->SetRegExpFunction(thread_, regexpFunction);
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    globalConst->SetConstant(ConstantIndex::JS_REGEXP_CLASS_INDEX, regexpFuncInstanceDynclass.GetTaggedValue());
}

void Builtins::InitializeArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Arraybase.prototype
    JSHandle<JSHClass> arrBaseFuncInstanceDynclass = factory_->CreateJSArrayInstanceClass(objFuncPrototypeVal);

    // Array.prototype
    JSHandle<JSObject> arrFuncPrototype = factory_->NewJSObject(arrBaseFuncInstanceDynclass);
    JSHandle<JSArray>::Cast(arrFuncPrototype)->SetLength(thread_, JSTaggedValue(FunctionLength::ZERO));
    auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
    JSArray::Cast(*arrFuncPrototype)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
    JSHandle<JSTaggedValue> arrFuncPrototypeValue(arrFuncPrototype);

    //  Array.prototype_or_dynclass
    JSHandle<JSHClass> arrFuncInstanceDynclass = factory_->CreateJSArrayInstanceClass(arrFuncPrototypeValue);

    // Array = new Function()
    JSHandle<JSObject> arrayFunction(
        NewBuiltinConstructor(env, arrFuncPrototype, BuiltinsArray::ArrayConstructor, "Array", FunctionLength::ONE));
    JSHandle<JSFunction> arrayFuncFunction(arrayFunction);

    // Set the [[Realm]] internal slot of F to the running execution context's Realm
    JSHandle<LexicalEnv> lexicalEnv = factory_->NewLexicalEnv(0);
    lexicalEnv->SetParentEnv(thread_, env.GetTaggedValue());
    arrayFuncFunction->SetLexicalEnv(thread_, lexicalEnv.GetTaggedValue());

    arrayFuncFunction->SetFunctionPrototype(thread_, arrFuncInstanceDynclass.GetTaggedValue());

    // Array.prototype method
    SetFunction(env, arrFuncPrototype, "concat", BuiltinsArray::Concat, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "copyWithin", BuiltinsArray::CopyWithin, FunctionLength::TWO);
    SetFunction(env, arrFuncPrototype, "entries", BuiltinsArray::Entries, FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, "every", BuiltinsArray::Every, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "fill", BuiltinsArray::Fill, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "filter", BuiltinsArray::Filter, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "find", BuiltinsArray::Find, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "findIndex", BuiltinsArray::FindIndex, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "forEach", BuiltinsArray::ForEach, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "indexOf", BuiltinsArray::IndexOf, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "join", BuiltinsArray::Join, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "keys", BuiltinsArray::Keys, FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, "lastIndexOf", BuiltinsArray::LastIndexOf, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "map", BuiltinsArray::Map, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "pop", BuiltinsArray::Pop, FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, "push", BuiltinsArray::Push, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "reduce", BuiltinsArray::Reduce, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "reduceRight", BuiltinsArray::ReduceRight, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "reverse", BuiltinsArray::Reverse, FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, "shift", BuiltinsArray::Shift, FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, "slice", BuiltinsArray::Slice, FunctionLength::TWO);
    SetFunction(env, arrFuncPrototype, "some", BuiltinsArray::Some, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "sort", BuiltinsArray::Sort, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "splice", BuiltinsArray::Splice, FunctionLength::TWO);
    SetFunction(env, arrFuncPrototype, thread_->GlobalConstants()->GetHandledToLocaleStringString(),
                BuiltinsArray::ToLocaleString, FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), BuiltinsArray::ToString,
                FunctionLength::ZERO);
    SetFunction(env, arrFuncPrototype, "unshift", BuiltinsArray::Unshift, FunctionLength::ONE);
    SetFunction(env, arrFuncPrototype, "values", BuiltinsArray::Values, FunctionLength::ZERO);

    // %ArrayPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> values(factory_->NewFromCanBeCompressString("values"));
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(arrFuncPrototype), values);
    PropertyDescriptor iteartorDesc(thread_, valuesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, arrFuncPrototype, iteratorSymbol, iteartorDesc);

    // Array method
    SetFunction(env, arrayFunction, "from", BuiltinsArray::From, FunctionLength::ONE);
    SetFunction(env, arrayFunction, "isArray", BuiltinsArray::IsArray, FunctionLength::ONE);
    SetFunction(env, arrayFunction, "of", BuiltinsArray::Of, FunctionLength::ZERO);

    // 22.1.2.5 get %Array% [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsArray::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(arrayFunction), speciesSymbol, speciesGetter);

    const int arrProtoLen = 0;
    JSHandle<JSTaggedValue> key_string = thread_->GlobalConstants()->GetHandledLengthString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(thread_, JSTaggedValue(arrProtoLen)), true, false,
                                  false);
    JSObject::DefineOwnProperty(thread_, arrFuncPrototype, key_string, descriptor);

    JSHandle<JSTaggedValue> valuesKey(factory_->NewFromCanBeCompressString("values"));
    PropertyDescriptor desc(thread_);
    JSObject::GetOwnProperty(thread_, arrFuncPrototype, valuesKey, desc);

    // Array.prototype [ @@unscopables ]
    JSHandle<JSTaggedValue> unscopablesSymbol = env->GetUnscopablesSymbol();
    JSHandle<JSTaggedValue> unscopablesGetter =
        CreateGetter(env, BuiltinsArray::Unscopables, "[Symbol.unscopables]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(arrFuncPrototype), unscopablesSymbol, unscopablesGetter);

    env->SetArrayProtoValuesFunction(thread_, desc.GetValue());
    env->SetArrayFunction(thread_, arrayFunction);
    env->SetArrayPrototype(thread_, arrFuncPrototype);
}

void Builtins::InitializeTypedArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // TypedArray.prototype
    JSHandle<JSObject> typedArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> typedArrFuncPrototypeValue(typedArrFuncPrototype);

    // TypedArray.prototype_or_dynclass
    JSHandle<JSHClass> typedArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_TYPED_ARRAY, typedArrFuncPrototypeValue);

    // TypedArray = new Function()
    JSHandle<JSObject> typedArrayFunction(NewBuiltinConstructor(
        env, typedArrFuncPrototype, BuiltinsTypedArray::TypedArrayBaseConstructor, "TypedArray", FunctionLength::ZERO));

    JSHandle<JSFunction>(typedArrayFunction)
        ->SetProtoOrDynClass(thread_, typedArrFuncInstanceDynclass.GetTaggedValue());

    // TypedArray.prototype method
    SetFunction(env, typedArrFuncPrototype, "copyWithin", BuiltinsTypedArray::CopyWithin, FunctionLength::TWO);
    SetFunction(env, typedArrFuncPrototype, "entries", BuiltinsTypedArray::Entries, FunctionLength::ZERO);
    SetFunction(env, typedArrFuncPrototype, "every", BuiltinsTypedArray::Every, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "fill", BuiltinsTypedArray::Fill, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "filter", BuiltinsTypedArray::Filter, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "find", BuiltinsTypedArray::Find, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "findIndex", BuiltinsTypedArray::FindIndex, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "forEach", BuiltinsTypedArray::ForEach, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "indexOf", BuiltinsTypedArray::IndexOf, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "join", BuiltinsTypedArray::Join, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "keys", BuiltinsTypedArray::Keys, FunctionLength::ZERO);
    SetFunction(env, typedArrFuncPrototype, "lastIndexOf", BuiltinsTypedArray::LastIndexOf, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "map", BuiltinsTypedArray::Map, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "reduce", BuiltinsTypedArray::Reduce, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "reduceRight", BuiltinsTypedArray::ReduceRight, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "reverse", BuiltinsTypedArray::Reverse, FunctionLength::ZERO);
    SetFunction(env, typedArrFuncPrototype, "set", BuiltinsTypedArray::Set, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "slice", BuiltinsTypedArray::Slice, FunctionLength::TWO);
    SetFunction(env, typedArrFuncPrototype, "some", BuiltinsTypedArray::Some, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "sort", BuiltinsTypedArray::Sort, FunctionLength::ONE);
    SetFunction(env, typedArrFuncPrototype, "subarray", BuiltinsTypedArray::Subarray, FunctionLength::TWO);
    SetFunction(env, typedArrFuncPrototype, thread_->GlobalConstants()->GetHandledToLocaleStringString(),
                BuiltinsTypedArray::ToLocaleString, FunctionLength::ZERO);
    SetFunction(env, typedArrFuncPrototype, "values", BuiltinsTypedArray::Values, FunctionLength::ZERO);

    JSHandle<JSTaggedValue> bufferGetter =
        CreateGetter(env, BuiltinsTypedArray::GetBuffer, "buffer", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> bufferKey(factory_->NewFromCanBeCompressString("buffer"));
    SetGetter(typedArrFuncPrototype, bufferKey, bufferGetter);

    JSHandle<JSTaggedValue> byteLengthGetter =
        CreateGetter(env, BuiltinsTypedArray::GetByteLength, "byteLength", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> byteLengthKey(factory_->NewFromCanBeCompressString("byteLength"));
    SetGetter(typedArrFuncPrototype, byteLengthKey, byteLengthGetter);

    JSHandle<JSTaggedValue> byteOffsetGetter =
        CreateGetter(env, BuiltinsTypedArray::GetByteOffset, "byteOffset", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> byteOffsetKey(factory_->NewFromCanBeCompressString("byteOffset"));
    SetGetter(typedArrFuncPrototype, byteOffsetKey, byteOffsetGetter);

    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, BuiltinsTypedArray::GetLength, "length", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromCanBeCompressString("length"));
    SetGetter(typedArrFuncPrototype, lengthKey, lengthGetter);

    // %TypedArray%.prototype.toString()
    JSHandle<JSTaggedValue> arrFuncPrototype = env->GetArrayPrototype();
    JSHandle<JSTaggedValue> toStringFunc =
        JSObject::GetMethod(thread_, arrFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString());
    PropertyDescriptor toStringDesc(thread_, toStringFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, typedArrFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                                toStringDesc);

    // %TypedArray%.prototype [ @@iterator ] ( )
    JSHandle<JSTaggedValue> values(factory_->NewFromCanBeCompressString("values"));
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(typedArrFuncPrototype), values);
    PropertyDescriptor iteartorDesc(thread_, valuesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, typedArrFuncPrototype, iteratorSymbol, iteartorDesc);

    // 22.2.3.31 get %TypedArray%.prototype [ @@toStringTag ]
    JSHandle<JSTaggedValue> toStringTagSymbol = env->GetToStringTagSymbol();
    JSHandle<JSTaggedValue> toStringTagGetter =
        CreateGetter(env, BuiltinsTypedArray::ToStringTag, "[Symbol.toStringTag]", FunctionLength::ZERO);
    SetGetter(typedArrFuncPrototype, toStringTagSymbol, toStringTagGetter);

    // TypedArray method
    SetFunction(env, typedArrayFunction, "from", BuiltinsTypedArray::From, FunctionLength::ONE);
    SetFunction(env, typedArrayFunction, "of", BuiltinsTypedArray::Of, FunctionLength::ZERO);

    // 22.2.2.4 get %TypedArray% [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsTypedArray::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(typedArrayFunction), speciesSymbol, speciesGetter);

    env->SetTypedArrayFunction(thread_, typedArrayFunction.GetTaggedValue());
    env->SetTypedArrayPrototype(thread_, typedArrFuncPrototype);

    JSHandle<JSHClass> specificTypedArrayFuncClass =
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetTypedArrayFunction());
    specificTypedArrayFuncClass->SetConstructor(true);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(factory_->NewJSObject(specificTypedArrayFuncClass));
    function->SetBuiltinsCtorMode();
    env->SetSpecificTypedArrayFunctionClass(thread_, specificTypedArrayFuncClass);

    InitializeInt8Array(env, typedArrFuncInstanceDynclass);
    InitializeUint8Array(env, typedArrFuncInstanceDynclass);
    InitializeUint8ClampedArray(env, typedArrFuncInstanceDynclass);
    InitializeInt16Array(env, typedArrFuncInstanceDynclass);
    InitializeUint16Array(env, typedArrFuncInstanceDynclass);
    InitializeInt32Array(env, typedArrFuncInstanceDynclass);
    InitializeUint32Array(env, typedArrFuncInstanceDynclass);
    InitializeFloat32Array(env, typedArrFuncInstanceDynclass);
    InitializeFloat64Array(env, typedArrFuncInstanceDynclass);
}

void Builtins::InitializeInt8Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Int8Array.prototype
    JSHandle<JSObject> int8ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> int8ArrFuncPrototypeValue(int8ArrFuncPrototype);

    // Int8Array.prototype_or_dynclass
    JSHandle<JSHClass> int8ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_INT8_ARRAY, int8ArrFuncPrototypeValue);

    // Int8Array = new Function()
    JSHandle<JSFunction> int8ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Int8ArrayConstructor));
    InitializeCtor(env, int8ArrFuncPrototype, int8ArrayFunction, "Int8Array", FunctionLength::THREE);

    int8ArrayFunction->SetProtoOrDynClass(thread_, int8ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 1;
    SetConstant(int8ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(int8ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetInt8ArrayFunction(thread_, int8ArrayFunction);
}

void Builtins::InitializeUint8Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Uint8Array.prototype
    JSHandle<JSObject> uint8ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> uint8ArrFuncPrototypeValue(uint8ArrFuncPrototype);

    // Uint8Array.prototype_or_dynclass
    JSHandle<JSHClass> uint8ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_UINT8_ARRAY, uint8ArrFuncPrototypeValue);

    // Uint8Array = new Function()
    JSHandle<JSFunction> uint8ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Uint8ArrayConstructor));
    InitializeCtor(env, uint8ArrFuncPrototype, uint8ArrayFunction, "Uint8Array", FunctionLength::THREE);

    uint8ArrayFunction->SetProtoOrDynClass(thread_, uint8ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 1;
    SetConstant(uint8ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(uint8ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetUint8ArrayFunction(thread_, uint8ArrayFunction);
}

void Builtins::InitializeUint8ClampedArray(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Uint8ClampedArray.prototype
    JSHandle<JSObject> uint8ClampedArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> uint8ClampedArrFuncPrototypeValue(uint8ClampedArrFuncPrototype);

    // Uint8ClampedArray.prototype_or_dynclass
    JSHandle<JSHClass> uint8ClampedArrFuncInstanceDynclass =
        factory_->NewEcmaDynClass(panda::ecmascript::JSTypedArray::SIZE, JSType::JS_UINT8_CLAMPED_ARRAY,
                                  uint8ClampedArrFuncPrototypeValue);

    // Uint8ClampedArray = new Function()
    JSHandle<JSFunction> uint8ClampedArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Uint8ClampedArrayConstructor));
    InitializeCtor(env, uint8ClampedArrFuncPrototype, uint8ClampedArrayFunction, "Uint8ClampedArray",
                   FunctionLength::THREE);

    uint8ClampedArrayFunction->SetProtoOrDynClass(thread_, uint8ClampedArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 1;
    SetConstant(uint8ClampedArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(uint8ClampedArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetUint8ClampedArrayFunction(thread_, uint8ClampedArrayFunction);
}

void Builtins::InitializeInt16Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Int16Array.prototype
    JSHandle<JSObject> int16ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> int16ArrFuncPrototypeValue(int16ArrFuncPrototype);

    // Int16Array.prototype_or_dynclass
    JSHandle<JSHClass> int16ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_INT16_ARRAY, int16ArrFuncPrototypeValue);

    // Int16Array = new Function()
    JSHandle<JSFunction> int16ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Int16ArrayConstructor));
    InitializeCtor(env, int16ArrFuncPrototype, int16ArrayFunction, "Int16Array", FunctionLength::THREE);

    int16ArrayFunction->SetProtoOrDynClass(thread_, int16ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 2;
    SetConstant(int16ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(int16ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetInt16ArrayFunction(thread_, int16ArrayFunction);
}

void Builtins::InitializeUint16Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Uint16Array.prototype
    JSHandle<JSObject> uint16ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> uint16ArrFuncPrototypeValue(uint16ArrFuncPrototype);

    // Uint16Array.prototype_or_dynclass
    JSHandle<JSHClass> uint16ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_UINT16_ARRAY, uint16ArrFuncPrototypeValue);

    // Uint16Array = new Function()
    JSHandle<JSFunction> uint16ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Uint16ArrayConstructor));
    InitializeCtor(env, uint16ArrFuncPrototype, uint16ArrayFunction, "Uint16Array", FunctionLength::THREE);

    uint16ArrayFunction->SetProtoOrDynClass(thread_, uint16ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 2;
    SetConstant(uint16ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(uint16ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetUint16ArrayFunction(thread_, uint16ArrayFunction);
}

void Builtins::InitializeInt32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Int32Array.prototype
    JSHandle<JSObject> int32ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> int32ArrFuncPrototypeValue(int32ArrFuncPrototype);

    // Int32Array.prototype_or_dynclass
    JSHandle<JSHClass> int32ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_INT32_ARRAY, int32ArrFuncPrototypeValue);

    // Int32Array = new Function()
    JSHandle<JSFunction> int32ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Int32ArrayConstructor));
    InitializeCtor(env, int32ArrFuncPrototype, int32ArrayFunction, "Int32Array", FunctionLength::THREE);

    int32ArrayFunction->SetProtoOrDynClass(thread_, int32ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 4;
    SetConstant(int32ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(int32ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetInt32ArrayFunction(thread_, int32ArrayFunction);
}

void Builtins::InitializeUint32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Uint32Array.prototype
    JSHandle<JSObject> uint32ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> uint32ArrFuncPrototypeValue(uint32ArrFuncPrototype);

    // Uint32Array.prototype_or_dynclass
    JSHandle<JSHClass> uint32ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_UINT32_ARRAY, uint32ArrFuncPrototypeValue);

    // Uint32Array = new Function()
    JSHandle<JSFunction> uint32ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Uint32ArrayConstructor));
    InitializeCtor(env, uint32ArrFuncPrototype, uint32ArrayFunction, "Uint32Array", FunctionLength::THREE);

    uint32ArrayFunction->SetProtoOrDynClass(thread_, uint32ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 4;
    SetConstant(uint32ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(uint32ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetUint32ArrayFunction(thread_, uint32ArrayFunction);
}

void Builtins::InitializeFloat32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Float32Array.prototype
    JSHandle<JSObject> float32ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> float32ArrFuncPrototypeValue(float32ArrFuncPrototype);

    // Float32Array.prototype_or_dynclass
    JSHandle<JSHClass> float32ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_FLOAT32_ARRAY, float32ArrFuncPrototypeValue);

    // Float32Array = new Function()
    JSHandle<JSFunction> float32ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Float32ArrayConstructor));
    InitializeCtor(env, float32ArrFuncPrototype, float32ArrayFunction, "Float32Array", FunctionLength::THREE);

    float32ArrayFunction->SetProtoOrDynClass(thread_, float32ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 4;
    SetConstant(float32ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(float32ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetFloat32ArrayFunction(thread_, float32ArrayFunction);
}

void Builtins::InitializeFloat64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Float64Array.prototype
    JSHandle<JSObject> float64ArrFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> float64ArrFuncPrototypeValue(float64ArrFuncPrototype);

    // Float64Array.prototype_or_dynclass
    JSHandle<JSHClass> float64ArrFuncInstanceDynclass = factory_->NewEcmaDynClass(
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_FLOAT64_ARRAY, float64ArrFuncPrototypeValue);

    // Float64Array = new Function()
    JSHandle<JSFunction> float64ArrayFunction = factory_->NewSpecificTypedArrayFunction(
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Float64ArrayConstructor));
    InitializeCtor(env, float64ArrFuncPrototype, float64ArrayFunction, "Float64Array", FunctionLength::THREE);

    float64ArrayFunction->SetProtoOrDynClass(thread_, float64ArrFuncInstanceDynclass.GetTaggedValue());

    const int bytesPerElement = 8;
    SetConstant(float64ArrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    SetConstant(JSHandle<JSObject>(float64ArrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));
    env->SetFloat64ArrayFunction(thread_, float64ArrayFunction);
}

void Builtins::InitializeArrayBuffer(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // ArrayBuffer.prototype
    JSHandle<JSObject> arrayBufferFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> arrayBufferFuncPrototypeValue(arrayBufferFuncPrototype);

    //  ArrayBuffer.prototype_or_dynclass
    JSHandle<JSHClass> arrayBufferFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSArrayBuffer::SIZE, JSType::JS_ARRAY_BUFFER, arrayBufferFuncPrototypeValue);

    // ArrayBuffer = new Function()
    JSHandle<JSObject> arrayBufferFunction(NewBuiltinConstructor(
        env, arrayBufferFuncPrototype, ArrayBuffer::ArrayBufferConstructor, "ArrayBuffer", FunctionLength::ONE));

    JSHandle<JSFunction>(arrayBufferFunction)
        ->SetFunctionPrototype(thread_, arrayBufferFuncInstanceDynclass.GetTaggedValue());

    // ArrayBuffer prototype method
    SetFunction(env, arrayBufferFuncPrototype, "slice", ArrayBuffer::Slice, FunctionLength::TWO);

    // ArrayBuffer method
    SetFunction(env, arrayBufferFunction, "isView", ArrayBuffer::IsView, FunctionLength::ONE);

    // 24.1.3.3 get ArrayBuffer[@@species]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, ArrayBuffer::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(arrayBufferFunction), speciesSymbol, speciesGetter);

    // 24.1.4.1 get ArrayBuffer.prototype.byteLength
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ArrayBuffer::GetByteLength, "byteLength", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromCanBeCompressString("byteLength"));
    SetGetter(arrayBufferFuncPrototype, lengthKey, lengthGetter);

    // 24.1.4.4 ArrayBuffer.prototype[@@toStringTag]
    SetStringTagSymbol(env, arrayBufferFuncPrototype, "ArrayBuffer");

    env->SetArrayBufferFunction(thread_, arrayBufferFunction.GetTaggedValue());
}

void Builtins::InitializeReflect(const JSHandle<GlobalEnv> &env,
                                 const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> reflectDynclass =
        factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> reflectObject = factory_->NewJSObject(reflectDynclass);

    SetFunction(env, reflectObject, "apply", Reflect::ReflectApply, FunctionLength::THREE);
    SetFunction(env, reflectObject, "construct", Reflect::ReflectConstruct, FunctionLength::TWO);
    SetFunction(env, reflectObject, "defineProperty", Reflect::ReflectDefineProperty, FunctionLength::THREE);
    SetFunction(env, reflectObject, "deleteProperty", Reflect::ReflectDeleteProperty, FunctionLength::TWO);
    SetFunction(env, reflectObject, "get", Reflect::ReflectGet, FunctionLength::TWO);
    SetFunction(env, reflectObject, "getOwnPropertyDescriptor", Reflect::ReflectGetOwnPropertyDescriptor,
                FunctionLength::TWO);
    SetFunction(env, reflectObject, "getPrototypeOf", Reflect::ReflectGetPrototypeOf, FunctionLength::ONE);
    SetFunction(env, reflectObject, "has", Reflect::ReflectHas, FunctionLength::TWO);
    SetFunction(env, reflectObject, "isExtensible", Reflect::ReflectIsExtensible, FunctionLength::ONE);
    SetFunction(env, reflectObject, "ownKeys", Reflect::ReflectOwnKeys, FunctionLength::ONE);
    SetFunction(env, reflectObject, "preventExtensions", Reflect::ReflectPreventExtensions, FunctionLength::ONE);
    SetFunction(env, reflectObject, "set", Reflect::ReflectSet, FunctionLength::THREE);
    SetFunction(env, reflectObject, "setPrototypeOf", Reflect::ReflectSetPrototypeOf, FunctionLength::TWO);

    JSHandle<JSTaggedValue> reflectString(factory_->NewFromCanBeCompressString("Reflect"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor reflectDesc(thread_, JSHandle<JSTaggedValue>::Cast(reflectObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, reflectString, reflectDesc);

    // @@ToStringTag
    SetStringTagSymbol(env, reflectObject, "Reflect");

    env->SetReflectFunction(thread_, reflectObject.GetTaggedValue());
}

void Builtins::InitializePromise(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &promiseFuncDynclass)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Promise.prototype
    JSHandle<JSObject> promiseFuncPrototype = factory_->NewJSObject(promiseFuncDynclass);
    JSHandle<JSTaggedValue> promiseFuncPrototypeValue(promiseFuncPrototype);
    // Promise.prototype_or_dynclass
    JSHandle<JSHClass> promiseFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSPromise::SIZE, JSType::JS_PROMISE, promiseFuncPrototypeValue);
    // Promise() = new Function()
    JSHandle<JSObject> promiseFunction(
        NewBuiltinConstructor(env, promiseFuncPrototype, Promise::PromiseConstructor, "Promise", FunctionLength::ONE));
    JSHandle<JSFunction>(promiseFunction)->SetFunctionPrototype(thread_, promiseFuncInstanceDynclass.GetTaggedValue());

    // Promise method
    SetFunction(env, promiseFunction, "all", Promise::All, FunctionLength::ONE);
    SetFunction(env, promiseFunction, "race", Promise::Race, FunctionLength::ONE);
    SetFunction(env, promiseFunction, "resolve", Promise::Resolve, FunctionLength::ONE);
    SetFunction(env, promiseFunction, "reject", Promise::Reject, FunctionLength::ONE);

    // promise.prototype method
    SetFunction(env, promiseFuncPrototype, "catch", Promise::Catch, FunctionLength::ONE);
    SetFunction(env, promiseFuncPrototype, "then", Promise::Then, FunctionLength::TWO);

    // Promise.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, promiseFuncPrototype, "Promise");

    // Set Promise [@@species]
    JSHandle<JSTaggedValue> speciesSymbol(env->GetSpeciesSymbol());
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, Promise::GetSpecies, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(promiseFunction, speciesSymbol, speciesGetter);

    env->SetPromiseFunction(thread_, promiseFunction);
}

void Builtins::InitializePromiseJob(const JSHandle<GlobalEnv> &env)
{
    JSHandle<JSTaggedValue> keyString(thread_->GlobalConstants()->GetHandledEmptyString());
    auto func = NewFunction(env, keyString, BuiltinsPromiseJob::PromiseReactionJob, FunctionLength::TWO);
    env->SetPromiseReactionJob(thread_, func);
    func = NewFunction(env, keyString, BuiltinsPromiseJob::PromiseResolveThenableJob, FunctionLength::THREE);
    env->SetPromiseResolveThenableJob(thread_, func);
}

void Builtins::InitializeDataView(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // ArrayBuffer.prototype
    JSHandle<JSObject> dataViewFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> dataViewFuncPrototypeValue(dataViewFuncPrototype);

    //  ArrayBuffer.prototype_or_dynclass
    JSHandle<JSHClass> dataViewFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSDataView::SIZE, JSType::JS_DATA_VIEW, dataViewFuncPrototypeValue);

    // ArrayBuffer = new Function()
    JSHandle<JSObject> dataViewFunction(NewBuiltinConstructor(env, dataViewFuncPrototype, DataView::DataViewConstructor,
                                                              "DataView", FunctionLength::ONE));

    JSHandle<JSFunction>(dataViewFunction)->SetProtoOrDynClass(thread_, dataViewFuncInstanceDynclass.GetTaggedValue());
    // DataView.prototype method
    SetFunction(env, dataViewFuncPrototype, "getFloat32", DataView::GetFloat32, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getFloat64", DataView::GetFloat64, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getInt8", DataView::GetInt8, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getInt16", DataView::GetInt16, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getInt32", DataView::GetInt32, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getUint8", DataView::GetUint8, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getUint16", DataView::GetUint16, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "getUint32", DataView::GetUint32, FunctionLength::ONE);
    SetFunction(env, dataViewFuncPrototype, "setFloat32", DataView::SetFloat32, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setFloat64", DataView::SetFloat64, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setInt8", DataView::SetInt8, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setInt16", DataView::SetInt16, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setInt32", DataView::SetInt32, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setUint8", DataView::SetUint8, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setUint16", DataView::SetUint16, FunctionLength::TWO);
    SetFunction(env, dataViewFuncPrototype, "setUint32", DataView::SetUint32, FunctionLength::TWO);

    // 24.2.4.1 get DataView.prototype.buffer
    JSHandle<JSTaggedValue> bufferGetter = CreateGetter(env, DataView::GetBuffer, "buffer", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> bufferKey(factory_->NewFromCanBeCompressString("buffer"));
    SetGetter(dataViewFuncPrototype, bufferKey, bufferGetter);

    // 24.2.4.2 get DataView.prototype.byteLength
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, DataView::GetByteLength, "byteLength", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromCanBeCompressString("byteLength"));
    SetGetter(dataViewFuncPrototype, lengthKey, lengthGetter);

    // 24.2.4.3 get DataView.prototype.byteOffset
    JSHandle<JSTaggedValue> offsetGetter = CreateGetter(env, DataView::GetOffset, "byteOffset", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> offsetKey(factory_->NewFromCanBeCompressString("byteOffset"));
    SetGetter(dataViewFuncPrototype, offsetKey, offsetGetter);

    // 24.2.4.21 DataView.prototype[ @@toStringTag ]
    SetStringTagSymbol(env, dataViewFuncPrototype, "DataView");
    env->SetDataViewFunction(thread_, dataViewFunction.GetTaggedValue());
}

JSHandle<JSFunction> Builtins::NewBuiltinConstructor(const JSHandle<GlobalEnv> &env,
                                                     const JSHandle<JSObject> &prototype, EcmaEntrypoint ctorFunc,
                                                     const char *name, int length) const
{
    JSHandle<JSFunction> ctor =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);
    InitializeCtor(env, prototype, ctor, name, length);
    return ctor;
}

JSHandle<JSFunction> Builtins::NewFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &key,
                                           EcmaEntrypoint func, int length) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSHandle<JSTaggedValue> handleUndefine(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, baseFunction, key, handleUndefine);
    return function;
}

void Builtins::SetFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, const char *key,
                           EcmaEntrypoint func, int length) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    SetFunction(env, obj, keyString, func, length);
}

void Builtins::SetFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                           const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, int length) const
{
    JSHandle<JSFunction> function(NewFunction(env, key, func, length));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, key, descriptor);
}

void Builtins::SetFrozenFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, const char *key,
                                 EcmaEntrypoint func, int length) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    JSHandle<JSFunction> function = NewFunction(env, keyString, func, length);
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(function), false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

template<int flag>
void Builtins::SetFunctionAtSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                   const JSHandle<JSTaggedValue> &symbol, const char *name, EcmaEntrypoint func,
                                   int length) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromString(name));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSHandle<JSTaggedValue> handleUndefine(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, baseFunction, nameString, handleUndefine);
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (flag == JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE) {
        PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), false, false, true);
        JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
        return;
    } else if constexpr (flag == JSSymbol::SYMBOL_HAS_INSTANCE_TYPE) {  // NOLINTE(readability-braces-around-statements)
        // ecma 19.2.3.6 Function.prototype[@@hasInstance] has the attributes
        // { [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: false }.
        PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), false, false, false);
        JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
        return;
    }
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
}

void Builtins::SetStringTagSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, const char *key) const
{
    JSHandle<JSTaggedValue> tag(factory_->NewFromString(key));
    JSHandle<JSTaggedValue> symbol = env->GetToStringTagSymbol();
    PropertyDescriptor desc(thread_, tag, false, false, true);
    JSObject::DefineOwnProperty(thread_, obj, symbol, desc);
}

JSHandle<JSTaggedValue> Builtins::CreateGetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func, const char *name,
                                               int length) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> funcName(factory_->NewFromString(name));
    JSHandle<JSTaggedValue> prefix = thread_->GlobalConstants()->GetHandledGetString();
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(function), funcName, prefix);
    return JSHandle<JSTaggedValue>(function);
}

JSHandle<JSTaggedValue> Builtins::CreateSetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func, const char *name,
                                               int length)
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> funcName(factory_->NewFromString(name));
    JSHandle<JSTaggedValue> prefix = thread_->GlobalConstants()->GetHandledSetString();
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(function), funcName, prefix);
    return JSHandle<JSTaggedValue>(function);
}

void Builtins::SetConstant(const JSHandle<JSObject> &obj, const char *key, JSTaggedValue value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(thread_, value), false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetConstantObject(const JSHandle<JSObject> &obj, const char *key, JSHandle<JSTaggedValue> &value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    PropertyDescriptor descriptor(thread_, value, false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetGlobalThis(const JSHandle<JSObject> &obj, const char *key, const JSHandle<JSTaggedValue> &globalValue)
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    PropertyDescriptor descriptor(thread_, globalValue, true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetAttribute(const JSHandle<JSObject> &obj, const char *key, const char *value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(factory_->NewFromString(value)), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetNoneAttributeProperty(const JSHandle<JSObject> &obj, const char *key,
                                        const JSHandle<JSTaggedValue> &value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    PropertyDescriptor des(thread_, value, false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, des);
}

void Builtins::SetFuncToObjAndGlobal(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject,
                                     const JSHandle<JSObject> &obj, const char *key, EcmaEntrypoint func, int length)
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> keyString(factory_->NewFromString(key));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSHandle<JSTaggedValue> handleUndefine(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, baseFunction, keyString, handleUndefine);
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
    JSObject::DefineOwnProperty(thread_, globalObject, keyString, descriptor);
}

void Builtins::StrictModeForbiddenAccessCallerArguments(const JSHandle<GlobalEnv> &env,
                                                        const JSHandle<JSObject> &prototype) const
{
    JSHandle<JSFunction> function =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(JSFunction::AccessCallerArgumentsThrowTypeError));

    JSHandle<JSTaggedValue> caller(factory_->NewFromCanBeCompressString("caller"));
    SetAccessor(prototype, caller, JSHandle<JSTaggedValue>::Cast(function), JSHandle<JSTaggedValue>::Cast(function));

    JSHandle<JSTaggedValue> arguments(factory_->NewFromCanBeCompressString("arguments"));
    SetAccessor(prototype, arguments, JSHandle<JSTaggedValue>::Cast(function), JSHandle<JSTaggedValue>::Cast(function));
}

void Builtins::InitializeGeneratorFunction(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSObject> generatorFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSHandle<JSTaggedValue> generatorFuncPrototypeValue(generatorFuncPrototype);

    // 26.3.3.1 GeneratorFunction.prototype.constructor
    // GeneratorFunction.prototype_or_dynclass
    JSHandle<JSHClass> generatorFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_GENERATOR_FUNCTION, generatorFuncPrototypeValue);
    generatorFuncInstanceDynclass->SetCallable(true);
    generatorFuncInstanceDynclass->SetExtensible(true);
    // GeneratorFunction = new GeneratorFunction()
    JSHandle<JSFunction> generatorFunction =
        NewBuiltinConstructor(env, generatorFuncPrototype, GeneratorObject::GeneratorFunctionConstructor,
                              "GeneratorFunction", FunctionLength::ONE);
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor generatorDesc(thread_, JSHandle<JSTaggedValue>::Cast(generatorFunction), false, false, true);
    JSObject::DefineOwnProperty(thread_, generatorFuncPrototype, constructorKey, generatorDesc);
    generatorFunction->SetProtoOrDynClass(thread_, generatorFuncInstanceDynclass.GetTaggedValue());
    env->SetGeneratorFunctionFunction(thread_, generatorFunction);

    // 26.3.3.2 GeneratorFunction.prototype.prototype -> Generator prototype object.
    PropertyDescriptor descriptor(thread_, env->GetGeneratorPrototype(), false, false, true);
    JSObject::DefineOwnProperty(thread_, generatorFuncPrototype, globalConst->GetHandledPrototypeString(), descriptor);

    // 26.3.3.3 GeneratorFunction.prototype[@@toStringTag]
    SetStringTagSymbol(env, generatorFuncPrototype, "GeneratorFunction");

    // GeneratorFunction prototype __proto__ -> Function.
    JSObject::SetPrototype(thread_, generatorFuncPrototype, env->GetFunctionPrototype());

    // 26.5.1.1 Generator.prototype.constructor -> %GeneratorFunction.prototype%.
    PropertyDescriptor generatorObjDesc(thread_, generatorFuncPrototypeValue, false, false, true);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>(env->GetInitialGenerator()),
                                globalConst->GetHandledConstructorString(), generatorObjDesc);

    // Generator instances prototype -> GeneratorFunction.prototype.prototype
    PropertyDescriptor generatorObjProtoDesc(thread_, generatorFuncPrototypeValue, true, false, false);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>(env->GetInitialGenerator()),
                                globalConst->GetHandledPrototypeString(), generatorObjProtoDesc);

    env->SetGeneratorFunctionPrototype(thread_, generatorFuncPrototype);
}

void Builtins::InitializeGenerator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSObject> generatorFuncPrototype = factory_->NewJSObject(objFuncDynclass);

    // GeneratorObject.prototype method
    // 26.5.1.2 Generator.prototype.next(value)
    SetFunction(env, generatorFuncPrototype, "next", GeneratorObject::GeneratorPrototypeNext, FunctionLength::ONE);
    // 26.5.1.3 Generator.prototype.return(value)
    SetFunction(env, generatorFuncPrototype, "return", GeneratorObject::GeneratorPrototypeReturn, FunctionLength::ONE);
    // 26.5.1.4 Generator.prototype.throw(exception)
    SetFunction(env, generatorFuncPrototype, "throw", GeneratorObject::GeneratorPrototypeThrow, FunctionLength::ONE);

    // 26.5.1.5 Generator.prototype[@@toStringTag]
    SetStringTagSymbol(env, generatorFuncPrototype, "Generator");

    // Generator with constructor, symbolTag, next/return/throw etc.
    PropertyDescriptor descriptor(thread_, env->GetIteratorPrototype(), true, false, false);
    JSObject::DefineOwnProperty(thread_, generatorFuncPrototype, globalConst->GetHandledPrototypeString(), descriptor);
    env->SetGeneratorPrototype(thread_, generatorFuncPrototype);
    JSObject::SetPrototype(thread_, generatorFuncPrototype, env->GetIteratorPrototype());

    // Generator {}
    JSHandle<JSObject> initialGeneratorFuncPrototype = factory_->NewJSObject(objFuncDynclass);
    JSObject::SetPrototype(thread_, initialGeneratorFuncPrototype, JSHandle<JSTaggedValue>(generatorFuncPrototype));
    env->SetInitialGenerator(thread_, initialGeneratorFuncPrototype);
}

void Builtins::SetArgumentsSharedAccessor(const JSHandle<GlobalEnv> &env)
{
    JSHandle<JSTaggedValue> throwFunction = env->GetThrowTypeError();

    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, throwFunction);
    accessor->SetSetter(thread_, throwFunction);
    env->SetArgumentsCallerAccessor(thread_, accessor);

    accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, throwFunction);
    accessor->SetSetter(thread_, throwFunction);
    env->SetArgumentsCalleeAccessor(thread_, accessor);
}

void Builtins::SetAccessor(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &getter, const JSHandle<JSTaggedValue> &setter) const
{
    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, getter);
    accessor->SetSetter(thread_, setter);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, true);
    JSObject::AddAccessor(thread_, JSHandle<JSTaggedValue>::Cast(obj), key, accessor, attr);
}

void Builtins::SetGetter(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                         const JSHandle<JSTaggedValue> &getter) const
{
    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, getter);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, true);
    JSObject::AddAccessor(thread_, JSHandle<JSTaggedValue>::Cast(obj), key, accessor, attr);
}

JSHandle<JSFunction> Builtins::NewIntlConstructor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                                                  EcmaEntrypoint ctorFunc, const char *name, int length)
{
    JSHandle<JSFunction> ctor =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);
    InitializeIntlCtor(env, prototype, ctor, name, length);
    return ctor;
}

void Builtins::InitializeIntlCtor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                                  const JSHandle<JSFunction> &ctor, const char *name, int length)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromString(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor1(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, prototype, constructorKey, descriptor1);

    // set "prototype" in constructor.
    ctor->SetFunctionPrototype(thread_, prototype.GetTaggedValue());

    if (!JSTaggedValue::SameValue(nameString, thread_->GlobalConstants()->GetHandledAsyncFunctionString())) {
        JSHandle<JSObject> intlObject(thread_, env->GetIntlFunction().GetTaggedValue());
        PropertyDescriptor descriptor2(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
        JSObject::DefineOwnProperty(thread_, intlObject, nameString, descriptor2);
    }
}

void Builtins::InitializeIntl(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeValue)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> intlDynclass = factory_->NewEcmaDynClass(JSObject::SIZE, JSType::JS_INTL, objFuncPrototypeValue);
    JSHandle<JSObject> intlObject = factory_->NewJSObject(intlDynclass);

    JSHandle<JSTaggedValue> initIntlSymbol(factory_->NewPublicSymbolWithChar("Symbol.IntlLegacyConstructedSymbol"));
    SetNoneAttributeProperty(intlObject, "fallbackSymbol", initIntlSymbol);

    SetFunction(env, intlObject, "getCanonicalLocales", Intl::GetCanonicalLocales, FunctionLength::ONE);

    // initial value of the "Intl" property of the global object.
    JSHandle<JSTaggedValue> intlString(factory_->NewFromString("Intl"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor intlDesc(thread_, JSHandle<JSTaggedValue>::Cast(intlObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, intlString, intlDesc);

    SetStringTagSymbol(env, intlObject, "Intl");

    env->SetIntlFunction(thread_, intlObject);
}

void Builtins::InitializeDateTimeFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // DateTimeFormat.prototype
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> dtfPrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> dtfPrototypeValue(dtfPrototype);

    // DateTimeFormat.prototype_or_dynclass
    JSHandle<JSHClass> dtfFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSDateTimeFormat::SIZE, JSType::JS_DATE_TIME_FORMAT, dtfPrototypeValue);

    // DateTimeFormat = new Function()
    // 13.4.1 Intl.DateTimeFormat.prototype.constructor
    JSHandle<JSObject> dtfFunction(NewIntlConstructor(env, dtfPrototype, DateTimeFormat::DateTimeFormatConstructor,
                                                      "DateTimeFormat", FunctionLength::ZERO));
    JSHandle<JSFunction>(dtfFunction)->SetFunctionPrototype(thread_, JSTaggedValue(*dtfFuncInstanceDynclass));

    // 13.3.2 Intl.DateTimeFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, dtfFunction, "supportedLocalesOf", DateTimeFormat::SupportedLocalesOf, FunctionLength::ONE);

    // DateTimeFormat.prototype method
    // 13.4.2 Intl.DateTimeFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, dtfPrototype, "Intl.DateTimeFormat");
    env->SetDateTimeFormatFunction(thread_, dtfFunction);

    // 13.4.3 get Intl.DateTimeFormat.prototype.format
    JSHandle<JSTaggedValue> formatGetter = CreateGetter(env, DateTimeFormat::Format, "format", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> formatSetter(thread_, JSTaggedValue::Undefined());
    SetAccessor(dtfPrototype, thread_->GlobalConstants()->GetHandledFormatString(), formatGetter, formatSetter);

    // 13.4.4 Intl.DateTimeFormat.prototype.formatToParts ( date )
    SetFunction(env, dtfPrototype, "formatToParts", DateTimeFormat::FormatToParts, FunctionLength::ONE);

    // 13.4.5 Intl.DateTimeFormat.prototype.resolvedOptions ()
    SetFunction(env, dtfPrototype, "resolvedOptions", DateTimeFormat::ResolvedOptions, FunctionLength::ZERO);

    SetFunction(env, dtfPrototype, "formatRange", DateTimeFormat::FormatRange, FunctionLength::TWO);

    SetFunction(env, dtfPrototype, "formatRangeToParts", DateTimeFormat::FormatRangeToParts, FunctionLength::TWO);
}

void Builtins::InitializeRelativeTimeFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // RelativeTimeFormat.prototype
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> rtfPrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> rtfPrototypeValue(rtfPrototype);

    // RelativeTimeFormat.prototype_or_dynclass
    JSHandle<JSHClass> rtfFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSRelativeTimeFormat::SIZE, JSType::JS_RELATIVE_TIME_FORMAT, rtfPrototypeValue);

    // RelativeTimeFormat = new Function()
    // 14.2.1 Intl.RelativeTimeFormat.prototype.constructor
    JSHandle<JSObject> rtfFunction(NewIntlConstructor(env, rtfPrototype,
                                                      RelativeTimeFormat::RelativeTimeFormatConstructor,
                                                      "RelativeTimeFormat", FunctionLength::ZERO));
    JSHandle<JSFunction>(rtfFunction)->SetFunctionPrototype(thread_, JSTaggedValue(*rtfFuncInstanceDynclass));

    // 14.3.2 Intl.RelativeTimeFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, rtfFunction, "supportedLocalesOf", RelativeTimeFormat::SupportedLocalesOf, FunctionLength::ONE);

    // RelativeTimeFormat.prototype method
    // 14.4.2 Intl.RelativeTimeFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, rtfPrototype, "Intl.RelativeTimeFormat");
    env->SetRelativeTimeFormatFunction(thread_, rtfFunction);

    // 14.4.3 get Intl.RelativeTimeFormat.prototype.format
    SetFunction(env, rtfPrototype, "format", RelativeTimeFormat::Format, FunctionLength::TWO);

    // 14.4.4  Intl.RelativeTimeFormat.prototype.formatToParts( value, unit )
    SetFunction(env, rtfPrototype, "formatToParts", RelativeTimeFormat::FormatToParts, FunctionLength::TWO);

    // 14.4.5 Intl.RelativeTimeFormat.prototype.resolvedOptions ()
    SetFunction(env, rtfPrototype, "resolvedOptions", RelativeTimeFormat::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeNumberFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // NumberFormat.prototype
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> nfPrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> nfPrototypeValue(nfPrototype);

    // NumberFormat.prototype_or_dynclass
    JSHandle<JSHClass> nfFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSNumberFormat::SIZE, JSType::JS_NUMBER_FORMAT, nfPrototypeValue);

    // NumberFormat = new Function()
    // 12.4.1 Intl.NumberFormat.prototype.constructor
    JSHandle<JSObject> nfFunction(NewIntlConstructor(env, nfPrototype, NumberFormat::NumberFormatConstructor,
                                                     "NumberFormat", FunctionLength::ZERO));
    JSHandle<JSFunction>(nfFunction)->SetFunctionPrototype(thread_, JSTaggedValue(*nfFuncInstanceDynclass));

    // 12.3.2 Intl.NumberFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, nfFunction, "supportedLocalesOf", NumberFormat::SupportedLocalesOf, FunctionLength::ONE);

    // NumberFormat.prototype method
    // 12.4.2 Intl.NumberFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, nfPrototype, "Intl.NumberFormat");
    env->SetNumberFormatFunction(thread_, nfFunction);

    // 12.4.3 get Intl.NumberFormat.prototype.format
    JSHandle<JSTaggedValue> formatGetter = CreateGetter(env, NumberFormat::Format, "format", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> formatSetter(thread_, JSTaggedValue::Undefined());
    SetAccessor(nfPrototype, thread_->GlobalConstants()->GetHandledFormatString(), formatGetter, formatSetter);

    // 12.4.4 Intl.NumberFormat.prototype.formatToParts ( date )
    SetFunction(env, nfPrototype, "formatToParts", NumberFormat::FormatToParts, FunctionLength::ONE);

    // 12.4.5 Intl.NumberFormat.prototype.resolvedOptions ()
    SetFunction(env, nfPrototype, "resolvedOptions", NumberFormat::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeLocale(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Locale.prototype
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> localePrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> localePrototypeValue(localePrototype);

    // Locale.prototype_or_dynclass
    JSHandle<JSHClass> localeFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSLocale::SIZE, JSType::JS_LOCALE, localePrototypeValue);

    // Locale = new Function()
    JSHandle<JSObject> localeFunction(
        NewIntlConstructor(env, localePrototype, Locale::LocaleConstructor, "Locale", FunctionLength::ONE));
    JSHandle<JSFunction>(localeFunction)->SetFunctionPrototype(thread_, JSTaggedValue(*localeFuncInstanceDynclass));

    // Locale.prototype method
    SetFunction(env, localePrototype, "maximize", Locale::Maximize, FunctionLength::ZERO);
    SetFunction(env, localePrototype, "minimize", Locale::Minimize, FunctionLength::ZERO);
    SetFunction(env, localePrototype, "toString", Locale::ToString, FunctionLength::ZERO);

    JSHandle<JSTaggedValue> baseNameGetter = CreateGetter(env, Locale::GetBaseName, "baseName", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledBaseNameString(), baseNameGetter);

    JSHandle<JSTaggedValue> calendarGetter = CreateGetter(env, Locale::GetCalendar, "calendar", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledCalendarString(), calendarGetter);

    JSHandle<JSTaggedValue> caseFirstGetter =
        CreateGetter(env, Locale::GetCaseFirst, "caseFirst", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledCaseFirstString(), caseFirstGetter);

    JSHandle<JSTaggedValue> collationGetter =
        CreateGetter(env, Locale::GetCollation, "collation", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledCollationString(), collationGetter);

    JSHandle<JSTaggedValue> hourCycleGetter =
        CreateGetter(env, Locale::GetHourCycle, "hourCycle", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledHourCycleString(), hourCycleGetter);

    JSHandle<JSTaggedValue> numericGetter = CreateGetter(env, Locale::GetNumeric, "numeric", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledNumericString(), numericGetter);

    JSHandle<JSTaggedValue> numberingSystemGetter =
        CreateGetter(env, Locale::GetNumberingSystem, "numberingSystem", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledNumberingSystemString(), numberingSystemGetter);

    JSHandle<JSTaggedValue> languageGetter = CreateGetter(env, Locale::GetLanguage, "language", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledLanguageString(), languageGetter);

    JSHandle<JSTaggedValue> scriptGetter = CreateGetter(env, Locale::GetScript, "script", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledScriptString(), scriptGetter);

    JSHandle<JSTaggedValue> regionGetter = CreateGetter(env, Locale::GetRegion, "region", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledRegionString(), regionGetter);

    // 10.3.2 Intl.Locale.prototype[ @@toStringTag ]
    SetStringTagSymbol(env, localePrototype, "Intl.Locale");
    env->SetLocaleFunction(thread_, localeFunction);
}

void Builtins::InitializeCollator(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Collator.prototype
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> collatorPrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> collatorPrototypeValue(collatorPrototype);

    // Collator.prototype_or_dynclass
    JSHandle<JSHClass> collatorFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSCollator::SIZE, JSType::JS_COLLATOR, collatorPrototypeValue);

    // Collator = new Function()
    // 11.1.2 Intl.Collator.prototype.constructor
    JSHandle<JSObject> collatorFunction(
        NewIntlConstructor(env, collatorPrototype, Collator::CollatorConstructor, "Collator", FunctionLength::ZERO));
    JSHandle<JSFunction>(collatorFunction)->SetFunctionPrototype(thread_, JSTaggedValue(*collatorFuncInstanceDynclass));

    // 11.2.2 Intl.Collator.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, collatorFunction, "supportedLocalesOf", Collator::SupportedLocalesOf, FunctionLength::ONE);

    // Collator.prototype method
    // 11.3.2 Intl.Collator.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, collatorPrototype, "Intl.Collator");
    env->SetCollatorFunction(thread_, collatorFunction);

    // 11.3.3 get Intl.Collator.prototype.compare
    JSHandle<JSTaggedValue> compareGetter = CreateGetter(env, Collator::Compare, "compare", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> compareSetter(thread_, JSTaggedValue::Undefined());
    SetAccessor(collatorPrototype, thread_->GlobalConstants()->GetHandledCompareString(), compareGetter, compareSetter);

    // 11.3.4 Intl.Collator.prototype.resolvedOptions ()
    SetFunction(env, collatorPrototype, "resolvedOptions", Collator::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializePluralRules(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // PluralRules.prototype
    JSHandle<JSTaggedValue> objFun(env->GetObjectFunction());
    JSHandle<JSObject> prPrototype = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> prPrototypeValue(prPrototype);

    // PluralRules.prototype_or_dynclass
    JSHandle<JSHClass> prFuncInstanceDynclass =
        factory_->NewEcmaDynClass(JSPluralRules::SIZE, JSType::JS_PLURAL_RULES, prPrototypeValue);

    // PluralRules = new Function()
    // 15.2.1 Intl.PluralRules.prototype.constructor
    JSHandle<JSObject> prFunction(
        NewIntlConstructor(env, prPrototype, PluralRules::PluralRulesConstructor, "PluralRules", FunctionLength::ZERO));
    JSHandle<JSFunction>(prFunction)->SetFunctionPrototype(thread_, JSTaggedValue(*prFuncInstanceDynclass));

    // 15.3.2 Intl.PluralRules.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, prFunction, "supportedLocalesOf", PluralRules::SupportedLocalesOf, FunctionLength::ONE);

    // PluralRules.prototype method
    // 15.4.2 Intl.PluralRules.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, prPrototype, "Intl.PluralRules");
    env->SetPluralRulesFunction(thread_, prFunction);

    // 15.4.3 get Intl.PluralRules.prototype.select
    SetFunction(env, prPrototype, "select", PluralRules::Select, FunctionLength::ONE);

    // 15.4.5 Intl.PluralRules.prototype.resolvedOptions ()
    SetFunction(env, prPrototype, "resolvedOptions", PluralRules::ResolvedOptions, FunctionLength::ZERO);
}

JSHandle<JSObject> Builtins::InitializeArkTools(const JSHandle<GlobalEnv> &env) const
{
    JSHandle<JSObject> tools = factory_->NewEmptyJSObject();
    SetFunction(env, tools, "print", builtins::BuiltinsArkTools::ObjectDump, FunctionLength::ZERO);
    return tools;
}

JSHandle<JSObject> Builtins::InitializeArkPrivate(const JSHandle<GlobalEnv> &env) const
{
    JSHandle<JSObject> arkPrivate = factory_->NewEmptyJSObject();
    SetFrozenFunction(env, arkPrivate, "Load", ContainersPrivate::Load, FunctionLength::ZERO);

    // It is used to provide non ECMA standard jsapi containers.
    SetConstant(arkPrivate, "ArrayList", JSTaggedValue(static_cast<int>(containers::ContainerTag::ArrayList)));
    SetConstant(arkPrivate, "Queue", JSTaggedValue(static_cast<int>(containers::ContainerTag::Queue)));
    SetConstant(arkPrivate, "Deque", JSTaggedValue(static_cast<int>(containers::ContainerTag::Deque)));
    SetConstant(arkPrivate, "Stack", JSTaggedValue(static_cast<int>(containers::ContainerTag::Stack)));
    SetConstant(arkPrivate, "Vector", JSTaggedValue(static_cast<int>(containers::ContainerTag::Vector)));
    SetConstant(arkPrivate, "List", JSTaggedValue(static_cast<int>(containers::ContainerTag::List)));
    SetConstant(arkPrivate, "LinkedList", JSTaggedValue(static_cast<int>(containers::ContainerTag::LinkedList)));
    SetConstant(arkPrivate, "TreeMap", JSTaggedValue(static_cast<int>(containers::ContainerTag::TreeMap)));
    SetConstant(arkPrivate, "TreeSet", JSTaggedValue(static_cast<int>(containers::ContainerTag::TreeSet)));
    SetConstant(arkPrivate, "HashMap", JSTaggedValue(static_cast<int>(containers::ContainerTag::HashMap)));
    SetConstant(arkPrivate, "HashSet", JSTaggedValue(static_cast<int>(containers::ContainerTag::HashSet)));
    SetConstant(arkPrivate, "LightWeightMap",
                JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightMap)));
    SetConstant(arkPrivate, "LightWeightSet",
                JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightSet)));
    SetConstant(arkPrivate, "PlainArray", JSTaggedValue(static_cast<int>(containers::ContainerTag::PlainArray)));
    return arkPrivate;
}
}  // namespace panda::ecmascript
