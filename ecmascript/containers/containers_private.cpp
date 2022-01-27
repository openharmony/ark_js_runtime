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

#include "ecmascript/containers/containers_private.h"

#include "containers_arraylist.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_arraylist.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersPrivate::Load(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> argv = GetCallArg(msg, 0);
    JSHandle<JSObject> thisValue(GetThis(msg));

    uint32_t tag = 0;
    if (!JSTaggedValue::ToElementIndex(argv.GetTaggedValue(), &tag) || tag >= ContainerTag::END) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Incorrect input parameters", JSTaggedValue::Exception());
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue res = JSTaggedValue::Undefined();
    switch (tag) {
        case ContainerTag::ArrayList: {
            JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("ArrayListConstructor"));
            JSTaggedValue value =
                FastRuntimeStub::GetPropertyByName<true>(thread, thisValue.GetTaggedValue(), key.GetTaggedValue());
            if (value != JSTaggedValue::Undefined()) {
                res = value;
            } else {
                JSHandle<JSTaggedValue> arrayList = InitializeArrayList(thread);
                SetFrozenConstructor(thread, thisValue, "ArrayListConstructor", arrayList);
                res = arrayList.GetTaggedValue();
            }
            break;
        }
        case ContainerTag::Queue:
        case ContainerTag::END:
            break;
        default:
            UNREACHABLE();
    }

    return res;
}

JSHandle<JSFunction> ContainersPrivate::NewContainerConstructor(JSThread *thread, const JSHandle<JSObject> &prototype,
                                                                EcmaEntrypoint ctorFunc, const char *name, int length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> ctor =
        factory->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);

    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSFunction::SetFunctionLength(thread, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory->NewFromCanBeCompressString(name));
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor1(thread, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread, prototype, constructorKey, descriptor1);

    /* set "prototype" in constructor */
    ctor->SetFunctionPrototype(thread, prototype.GetTaggedValue());

    return ctor;
}

void ContainersPrivate::SetFrozenFunction(JSThread *thread, const JSHandle<JSObject> &obj, const char *key,
                                          EcmaEntrypoint func, int length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> keyString(factory->NewFromCanBeCompressString(key));
    JSHandle<JSFunction> function = NewFunction(thread, keyString, func, length);
    PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>(function), false, false, false);
    JSObject::DefineOwnProperty(thread, obj, keyString, descriptor);
}

void ContainersPrivate::SetFrozenConstructor(JSThread *thread, const JSHandle<JSObject> &obj, const char *keyChar,
                                             JSHandle<JSTaggedValue> &value)
{
    JSObject::PreventExtensions(thread, obj);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(keyChar));
    PropertyDescriptor descriptor(thread, value, false, false, false);
    JSObject::DefineOwnProperty(thread, obj, key, descriptor);
}

JSHandle<JSFunction> ContainersPrivate::NewFunction(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                                    EcmaEntrypoint func, int length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function =
        factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(), reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSFunction::SetFunctionName(thread, baseFunction, key, thread->GlobalConstants()->GetHandledUndefined());
    return function;
}

JSHandle<JSTaggedValue> ContainersPrivate::CreateGetter(JSThread *thread, EcmaEntrypoint func, const char *name,
                                                        int length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function = factory->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> funcName(factory->NewFromString(name));
    JSHandle<JSTaggedValue> prefix = thread->GlobalConstants()->GetHandledGetString();
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(function), funcName, prefix);
    return JSHandle<JSTaggedValue>(function);
}

void ContainersPrivate::SetGetter(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  const JSHandle<JSTaggedValue> &getter)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, getter);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, false);
    JSObject::AddAccessor(thread, JSHandle<JSTaggedValue>::Cast(obj), key, accessor, attr);
}

void ContainersPrivate::SetFunctionAtSymbol(JSThread *thread, const JSHandle<GlobalEnv> &env,
                                            const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &symbol,
                                            const char *name, EcmaEntrypoint func, int length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function = factory->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory->NewFromString(name));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSFunction::SetFunctionName(thread, baseFunction, nameString, thread->GlobalConstants()->GetHandledUndefined());

    PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(function), false, false, false);
    JSObject::DefineOwnProperty(thread, obj, symbol, descriptor);
}

void ContainersPrivate::SetStringTagSymbol(JSThread *thread, const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSObject> &obj, const char *key)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> tag(factory->NewFromString(key));
    JSHandle<JSTaggedValue> symbol = env->GetToStringTagSymbol();
    PropertyDescriptor desc(thread, tag, false, false, false);
    JSObject::DefineOwnProperty(thread, obj, symbol, desc);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeArrayList(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // ArrayList.prototype
    JSHandle<JSObject> arrayListFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> arrayListFuncPrototypeValue(arrayListFuncPrototype);
    // ArrayList.prototype_or_dynclass
    JSHandle<JSHClass> arrayListInstanceDynclass =
        factory->NewEcmaDynClass(JSArrayList::SIZE, JSType::JS_ARRAY_LIST, arrayListFuncPrototypeValue);
    // ArrayList() = new Function()
    JSHandle<JSTaggedValue> arrayListFunction(NewContainerConstructor(
        thread, arrayListFuncPrototype, ContainersArrayList::ArrayListConstructor, "ArrayList", FuncLength::ZERO));
    JSHandle<JSFunction>(arrayListFunction)->SetFunctionPrototype(thread, arrayListInstanceDynclass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(arrayListFuncPrototype), constructorKey, arrayListFunction);

    // ArrayList.prototype.add()
    SetFrozenFunction(thread, arrayListFuncPrototype, "add", ContainersArrayList::Add, FuncLength::ONE);

    return arrayListFunction;
}
}  // namespace panda::ecmascript::containers
