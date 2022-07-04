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

#include "ecmascript/builtins/builtins_finalization_registry.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;
static int testValue = 0;

namespace panda::test {
class BuiltinsFinalizationRegistryTest : public testing::Test {
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

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue cleanupCallback()
        {
            ++testValue;
            return JSTaggedValue::Undefined();
        }
    };
};

JSTaggedValue CreateFinalizationRegistryConstructor(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> finalizationRegistry(env->GetBuiltinsFinalizationRegistryFunction());
    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(
        env, reinterpret_cast<void *>(BuiltinsFinalizationRegistryTest::TestClass::cleanupCallback));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*finalizationRegistry), 6);
    ecmaRuntimeCallInfo->SetFunction(finalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue res = BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return res;
}

// new FinalizationRegistry (cleanupCallback)
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, FinalizationRegistryConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> finalizationRegistry(env->GetBuiltinsFinalizationRegistryFunction());
    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::cleanupCallback));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*finalizationRegistry), 6);
    ecmaRuntimeCallInfo->SetFunction(finalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
}

// finalizationRegistry.Register(target, heldValue)
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register0)
{
    testValue = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("1"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, target, key, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
    ASSERT_EQ(testValue, 0);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register1)
{
    testValue = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("1"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, target, key, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
    ecmaRuntimeCallInfo->SetCallArg(2, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
    ASSERT_EQ(testValue, 0);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register2)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, target);
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo->SetCallArg(2, target);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 1);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register3)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);

    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        auto obj1 =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        target1 = obj1.GetTaggedValue();
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, target);
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo->SetCallArg(2, target);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo1->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo1->SetCallArg(0, target1);
        ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo1->SetCallArg(2, target1);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev1);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register4)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    JSTaggedValue result1 = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry1(thread, result1);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        auto obj1 =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        target1 = obj1.GetTaggedValue();
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, target);
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo->SetCallArg(2, target);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo1->SetThis(jsfinalizationRegistry1.GetTaggedValue());
        ecmaRuntimeCallInfo1->SetCallArg(0, target1);
        ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo1->SetCallArg(2, target1);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev1);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register5)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    JSTaggedValue target1 = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        auto obj1 =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        target1 = obj1.GetTaggedValue();
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, target);
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo->SetCallArg(2, target);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo1->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo1->SetCallArg(0, target1);
        ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo1->SetCallArg(2, target);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev1);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 2);
}

// finalizationRegistry.Unregister(unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Unregister1)
{
    testValue = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("1"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, target, key, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
    ecmaRuntimeCallInfo->SetCallArg(2, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsfinalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    BuiltinsFinalizationRegistry::Unregister(ecmaRuntimeCallInfo1);
    ASSERT_EQ(testValue, 0);
}

HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Unregister2)
{
    testValue = 0;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    vm->SetEnableForceGC(false);
    JSTaggedValue target = JSTaggedValue::Undefined();
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = obj.GetTaggedValue();
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, target);
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
        ecmaRuntimeCallInfo->SetCallArg(2, target);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo);

        auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo1->SetThis(jsfinalizationRegistry.GetTaggedValue());
        ecmaRuntimeCallInfo1->SetCallArg(0, target);

        BuiltinsFinalizationRegistry::Unregister(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    ASSERT_EQ(testValue, 0);
}
}  // namespace panda::test
