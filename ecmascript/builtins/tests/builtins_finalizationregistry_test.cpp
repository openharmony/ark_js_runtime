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

#include "ecmascript/jobs/micro_job_queue.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;
static JSTaggedValue testArgv = JSTaggedValue(0);

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
            testArgv = JSTaggedValue(10);  // number of 10
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

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    return JSTaggedValue(BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo.get()));
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

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsECMAObject());
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry>  jsfinalizationRegistry(thread, result);
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

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(testArgv, JSTaggedValue(0));
}

// finalizationRegistry.Register(target, heldValue [ , unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Register2)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry> jsfinalizationRegistry(thread, result);
    JSTaggedValue target =
	    factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc).GetTaggedValue();

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsfinalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, target);
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10));
    ecmaRuntimeCallInfo->SetCallArg(2, target);
    target = JSTaggedValue::Undefined();

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo.get());
    [[maybe_unused]] JSTaggedValue target22 =
	    factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc).GetTaggedValue();
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    ASSERT_EQ(testArgv, JSTaggedValue(0));
}

// finalizationRegistry.Unregister(unregisterToken ])
HWTEST_F_L0(BuiltinsFinalizationRegistryTest, Unregister)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();

    JSTaggedValue result = CreateFinalizationRegistryConstructor(thread);
    JSHandle<JSFinalizationRegistry>  jsfinalizationRegistry(thread, result);
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

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo.get());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsfinalizationRegistry.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    BuiltinsFinalizationRegistry::Register(ecmaRuntimeCallInfo1.get());
    ASSERT_EQ(testArgv, JSTaggedValue(0));
}
}  // namespace panda::test
