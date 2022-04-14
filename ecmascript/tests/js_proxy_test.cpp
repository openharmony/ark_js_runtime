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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JSProxyTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(JSProxyTest, ProxyCreate)
{
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(dynclass), dynclass));

    JSHandle<JSTaggedValue> key(thread->GetEcmaVM()->GetFactory()->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, targetHandle, key, value);
    EXPECT_EQ(JSObject::GetProperty(thread, targetHandle, key).GetValue()->GetInt(), 1);

    JSHandle<JSTaggedValue> handlerHandle(
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle, key).GetValue()->GetInt(), 1);
    PropertyDescriptor desc(thread);
    JSProxy::GetOwnProperty(thread, proxyHandle, key, desc);
    EXPECT_EQ(desc.GetValue()->GetInt(), 1);
}

// ES6 9.5.8 [[Get]] (P, Receiver)
// Called by the following function
JSTaggedValue HandlerGetProperty([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(10); // 10 : test case
}

HWTEST_F_L0(JSProxyTest, GetProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "get"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, targetHandle, key, value);
    EXPECT_EQ(JSObject::GetProperty(thread, targetHandle, key).GetValue()->GetInt(), 1);

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle, key).GetValue()->GetInt(), 1);

    // 2. handler has "get"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> getKey = thread->GlobalConstants()->GetHandledGetString();
    JSHandle<JSTaggedValue> getHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerGetProperty)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), getKey, getHandle);

    JSHandle<JSProxy> proxyHandle2(JSProxy::ProxyCreate(thread, targetHandle, handlerHandle));
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("y"));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle2, key2).GetValue()->GetInt(), 10);
}

// ES6 9.5.5 [[GetOwnProperty]] (P)
// Called by the following function
JSTaggedValue HandlerGetOwnProperty([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::Undefined());
}

HWTEST_F_L0(JSProxyTest, GetOwnProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "get"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, targetHandle, key, value);
    EXPECT_EQ(JSObject::GetProperty(thread, targetHandle, key).GetValue()->GetInt(), 1);

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    PropertyDescriptor desc(thread);
    JSProxy::GetOwnProperty(thread, proxyHandle, key, desc);
    EXPECT_EQ(desc.GetValue()->GetInt(), 1);

    // 2. handler has "get"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> defineKey = thread->GlobalConstants()->GetHandledGetOwnPropertyDescriptorString();
    JSHandle<JSTaggedValue> defineHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerGetOwnProperty)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), defineKey, defineHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("y"));
    PropertyDescriptor desc2(thread);
    EXPECT_FALSE(JSProxy::GetOwnProperty(thread, proxyHandle2, key2, desc2));
}

// ES6 9.5.9 [[Set]] ( P, V, Receiver)
JSTaggedValue HandlerSetProperty([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

HWTEST_F_L0(JSProxyTest, SetProperty)
{
    // 1. handler has no "get"
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "get"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    EXPECT_TRUE(JSProxy::SetProperty(thread, proxyHandle, key, value));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle, key).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSObject::GetProperty(thread, targetHandle, key).GetValue()->GetInt(), 1);

    // 2. handler has "set"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> setKey = thread->GlobalConstants()->GetHandledSetString();
    JSHandle<JSTaggedValue> setHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerSetProperty)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), setKey, setHandle);

    JSHandle<JSProxy> proxyHandle2(JSProxy::ProxyCreate(thread, targetHandle, handlerHandle));
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(10));
    EXPECT_FALSE(JSProxy::SetProperty(thread, proxyHandle2, key, value2));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle2, key).GetValue()->GetInt(), 1);
}

// ES6 9.5.6 [[DefineOwnProperty]] (P, Desc)
JSTaggedValue HandlerDefineOwnProperty([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

HWTEST_F_L0(JSProxyTest, DefineOwnProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "defineProperty"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    EXPECT_TRUE(JSProxy::DefineOwnProperty(thread, proxyHandle, key, desc));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle, key).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSObject::GetProperty(thread, targetHandle, key).GetValue()->GetInt(), 1);

    // 2. handler has "defineProperty"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> setKey = thread->GlobalConstants()->GetHandledDefinePropertyString();
    JSHandle<JSTaggedValue> setHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerDefineOwnProperty)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), setKey, setHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(10)));
    EXPECT_FALSE(JSProxy::DefineOwnProperty(thread, proxyHandle, key, desc2));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle2, key).GetValue()->GetInt(), 1);
}

JSTaggedValue HandlerDeleteProperty([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

// ES6 9.5.10 [[Delete]] (P)
HWTEST_F_L0(JSProxyTest, DeleteProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "deleteProperty"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    EXPECT_TRUE(JSProxy::DefineOwnProperty(thread, proxyHandle, key, desc));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle, key).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSObject::GetProperty(thread, targetHandle, key).GetValue()->GetInt(), 1);
    EXPECT_TRUE(JSProxy::DeleteProperty(thread, proxyHandle, key));
    PropertyDescriptor resDesc(thread);
    JSProxy::GetOwnProperty(thread, proxyHandle, key, resDesc);
    EXPECT_TRUE(JSTaggedValue::SameValue(resDesc.GetValue().GetTaggedValue(), JSTaggedValue::Undefined()));

    // 2. handler has "deleteProperty"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledDeletePropertyString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerDeleteProperty)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    EXPECT_TRUE(JSProxy::DefineOwnProperty(thread, proxyHandle2, key, desc2));
    EXPECT_EQ(JSProxy::GetProperty(thread, proxyHandle2, key).GetValue()->GetInt(), 1);
    EXPECT_FALSE(JSProxy::DeleteProperty(thread, proxyHandle2, key));
}

JSTaggedValue HandlerGetPrototype([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::Null());
}

// ES6 9.5.1 [[GetPrototypeOf]] ( )
HWTEST_F_L0(JSProxyTest, GetPrototypeOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "GetPrototypeOf"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> proto(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());
    JSObject::SetPrototype(thread, JSHandle<JSObject>(targetHandle), proto);
    EXPECT_TRUE(
        JSTaggedValue::SameValue(JSTaggedValue::GetPrototype(thread, targetHandle), proto.GetTaggedValue()));

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    EXPECT_TRUE(JSTaggedValue::SameValue(JSProxy::GetPrototype(thread, proxyHandle), proto.GetTaggedValue()));

    // 2. handler has "GetPrototypeOf"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledGetPrototypeOfString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerGetPrototype)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    EXPECT_TRUE(JSTaggedValue::SameValue(JSProxy::GetPrototype(thread, proxyHandle2), JSTaggedValue::Null()));
}

JSTaggedValue HandlerSetPrototype([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

// ES6 9.5.2 [[SetPrototypeOf]] (V)
HWTEST_F_L0(JSProxyTest, SetPrototypeOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "SetPrototypeOf"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> proto(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    JSProxy::SetPrototype(thread, proxyHandle, proto);
    EXPECT_TRUE(
        JSTaggedValue::SameValue(JSTaggedValue::GetPrototype(thread, targetHandle), proto.GetTaggedValue()));

    // 2. handler has "SetPrototypeOf"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledSetPrototypeOfString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerSetPrototype)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    EXPECT_FALSE(JSProxy::SetPrototype(thread, proxyHandle2, proto));
}

JSTaggedValue HandlerIsExtensible([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

// ES6 9.5.3 [[IsExtensible]] ( )
HWTEST_F_L0(JSProxyTest, IsExtensible)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "IsExtensible"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    bool status1 = JSProxy::IsExtensible(thread, proxyHandle);
    bool status2 = JSHandle<JSObject>::Cast(targetHandle)->IsExtensible();
    EXPECT_TRUE(status1 == status2);

    // 2. handler has "IsExtensible"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledIsExtensibleString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerIsExtensible)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    EXPECT_FALSE(JSProxy::IsExtensible(thread, proxyHandle2));
}

JSTaggedValue HandlerPreventExtensions([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

// ES6 9.5.4 [[PreventExtensions]] ( )
HWTEST_F_L0(JSProxyTest, PreventExtensions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "PreventExtensions"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    bool status1 = JSProxy::PreventExtensions(thread, proxyHandle);
    EXPECT_TRUE(status1);
    bool status2 = JSHandle<JSObject>::Cast(targetHandle)->IsExtensible();
    EXPECT_FALSE(status2);

    // 2. handler has "PreventExtensions"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledPreventExtensionsString();
    JSHandle<JSTaggedValue> funcHandle(
        factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerPreventExtensions)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    EXPECT_FALSE(JSProxy::PreventExtensions(thread, proxyHandle2));
}

JSTaggedValue HandlerHasProperty([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}

// ES6 9.5.7 [[HasProperty]] (P)
HWTEST_F_L0(JSProxyTest, HasProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "HasProperty"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::DefineOwnProperty(thread, JSHandle<JSObject>::Cast(targetHandle), key, desc);

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    EXPECT_TRUE(JSProxy::HasProperty(thread, proxyHandle, key));

    // 2. handler has "HasProperty"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledHasString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerHasProperty)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    EXPECT_FALSE(JSProxy::HasProperty(thread, proxyHandle2, key));
}

JSTaggedValue HandlerOwnPropertyKeys([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    auto thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArray> arr = factory->NewJSArray();
    return JSTaggedValue(arr.GetTaggedValue());
}

// ES6 9.5.12 [[OwnPropertyKeys]] ()
HWTEST_F_L0(JSProxyTest, OwnPropertyKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "OwnPropertyKeys"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::DefineOwnProperty(thread, JSHandle<JSObject>::Cast(targetHandle), key, desc);

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);
    JSHandle<TaggedArray> res = JSProxy::OwnPropertyKeys(thread, proxyHandle);

    EXPECT_TRUE(JSTaggedValue::SameValue(res->Get(0), key.GetTaggedValue()));

    // 2. handler has "OwnPropertyKeys"
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledOwnKeysString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerOwnPropertyKeys)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    JSHandle<TaggedArray> res2 = JSProxy::OwnPropertyKeys(thread, proxyHandle2);
    EXPECT_TRUE(res2->GetLength() == 0U || !JSTaggedValue::SameValue(res2->Get(0), key.GetTaggedValue()));
}

JSTaggedValue HandlerCall([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::False());
}
JSTaggedValue HandlerFunction([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue(JSTaggedValue::True());
}

// ES6 9.5.13 [[Call]] (thisArgument, argumentsList)
HWTEST_F_L0(JSProxyTest, Call)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. handler has no "Call"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerFunction)));
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);
    
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(proxyHandle),
        JSHandle<JSTaggedValue>(proxyHandle), undefined, 0);
    JSTaggedValue res = JSProxy::CallInternal(&info);
    EXPECT_TRUE(JSTaggedValue::SameValue(res, JSTaggedValue::True()));

    // 2. handler has "Call"
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledApplyString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerCall)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);

    EcmaRuntimeCallInfo runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(proxyHandle2),
        JSHandle<JSTaggedValue>(proxyHandle2), undefined, 0);
    JSTaggedValue res2 = JSProxy::CallInternal(&runtimeInfo);
    EXPECT_TRUE(JSTaggedValue::SameValue(res2, JSTaggedValue::False()));
}

JSTaggedValue HandlerConstruct([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    auto thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2))); // 2 : test case
    JSObject::DefineOwnProperty(argv->GetThread(), JSHandle<JSObject>::Cast(obj), key, desc);
    return JSTaggedValue(obj.GetTaggedValue());
}
JSTaggedValue HandlerConFunc([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    auto thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::DefineOwnProperty(argv->GetThread(), JSHandle<JSObject>::Cast(obj), key, desc);
    return JSTaggedValue(obj.GetTaggedValue());
}

// ES6 9.5.14 [[Construct]] ( argumentsList, newTarget)
HWTEST_F_L0(JSProxyTest, Construct)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. handler has no "Construct"
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerConFunc)));
    JSHandle<JSFunction>::Cast(targetHandle)->GetJSHClass()->SetConstructor(true);
    EXPECT_TRUE(targetHandle->IsECMAObject());

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(proxyHandle), handlerHandle, undefined, 0);
    JSTaggedValue res = JSProxy::ConstructInternal(&info);
    JSHandle<JSTaggedValue> taggedRes(thread, res);
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    EXPECT_EQ(JSObject::GetProperty(thread, taggedRes, key).GetValue()->GetInt(), 1);

    // 2. handler has "Construct"
    JSHandle<JSTaggedValue> funcKey = thread->GlobalConstants()->GetHandledProxyConstructString();
    JSHandle<JSTaggedValue> funcHandle(factory->NewJSFunction(env, reinterpret_cast<void *>(HandlerConstruct)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handlerHandle), funcKey, funcHandle);

    JSHandle<JSProxy> proxyHandle2 = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle2 != nullptr);
    EcmaRuntimeCallInfo runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(proxyHandle2), targetHandle, undefined, 0);
    JSTaggedValue res2 = JSProxy::ConstructInternal(&runtimeInfo);
    JSHandle<JSTaggedValue> taggedRes2(thread, res2);
    EXPECT_EQ(JSObject::GetProperty(thread, taggedRes2, key).GetValue()->GetInt(), 2);
}
}  // namespace panda::test
