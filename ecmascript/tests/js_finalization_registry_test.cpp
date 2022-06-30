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
#include "ecmascript/global_env.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

static int testValue = 0;

namespace panda::test {
class JSFinalizationRegistryTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue callbackTest()
        {
            testValue++;
            return JSTaggedValue::Undefined();
        }
    };
};

static JSHandle<JSTaggedValue> CreateFinalizationRegistry(JSThread *thread)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSFunction> finaRegFunc(env->GetBuiltinsFinalizationRegistryFunction());
    JSHandle<JSFunction> callbackFunc = factory->NewJSFunction(env, reinterpret_cast<void *>(
        JSFinalizationRegistryTest::TestClass::callbackTest));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*finaRegFunc), 6);
    ecmaRuntimeCallInfo->SetFunction(finaRegFunc.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, callbackFunc.GetTaggedValue());

    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue finalizationRegistry =
        BuiltinsFinalizationRegistry::FinalizationRegistryConstructor(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> finalizationRegistryHandle(thread, finalizationRegistry);
    TestHelper::TearDownFrame(thread, prev);
    return finalizationRegistryHandle;
}

/**
 * @tc.name: Register
 * @tc.desc: Register the target object to the JSFinalizationRegistry object, and call the callback method after the
 *           target object is garbage collected. And a unregistration token, which is passed to the unregister method
 *           when the finalizer is no longer needed. The held value, which is used to represent that object when
 *           cleaning it up in the finalizer.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSFinalizationRegistryTest, Register_001)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, target, key, value);
    JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
    JSHandle<JSTaggedValue> unregisterToken(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
    JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());

    // If unregisterToken is undefined, use vector to store
    JSHandle<CellRecord> cellRecord = factory->NewCellRecord();
    cellRecord->SetToWeakRefTarget(target.GetTaggedValue());
    cellRecord->SetHeldValue(thread, heldValue);
    JSHandle<JSTaggedValue> cell(cellRecord);
    JSHandle<CellRecordVector> expectNoUnregister(thread, finaRegObj->GetNoUnregister());
    expectNoUnregister = CellRecordVector::Append(thread, expectNoUnregister, cell);

    JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
    JSHandle<JSTaggedValue> noUnregister(thread, finaRegObj->GetNoUnregister());
    JSHandle<JSTaggedValue> finRegLists = env->GetFinRegLists();
    EXPECT_EQ(finRegLists.GetTaggedValue().GetRawData(),
        JSHandle<JSTaggedValue>::Cast(finaRegObj).GetTaggedValue().GetRawData());
    EXPECT_EQ(expectNoUnregister.GetTaggedValue().GetRawData(), noUnregister.GetTaggedValue().GetRawData());
    EXPECT_EQ(testValue, 0);
}

HWTEST_F_L0(JSFinalizationRegistryTest, Register_002)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSObject::SetProperty(thread, target, key, value);
    JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
    JSHandle<JSTaggedValue> unregisterToken = target;
    JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
    JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());

    // If unregisterToken is not undefined, use hash map to store to facilitate subsequent delete operations
    JSHandle<CellRecord> cellRecord = factory->NewCellRecord();
    cellRecord->SetToWeakRefTarget(target.GetTaggedValue());
    cellRecord->SetHeldValue(thread, heldValue);
    JSHandle<JSTaggedValue> cell(cellRecord);
    JSHandle<CellRecordVector> array = JSHandle<CellRecordVector>(CellRecordVector::Create(thread));
    array = CellRecordVector::Append(thread, array, cell);
    JSHandle<JSTaggedValue> arrayValue(array);
    JSHandle<LinkedHashMap> expectMaybeUnregister(thread, finaRegObj->GetMaybeUnregister());
    expectMaybeUnregister = LinkedHashMap::SetWeakRef(thread, expectMaybeUnregister, unregisterToken, arrayValue);

    JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
    JSHandle<JSTaggedValue> maybeUnregister(thread, finaRegObj->GetMaybeUnregister());
    EXPECT_EQ(expectMaybeUnregister.GetTaggedValue().GetRawData(), maybeUnregister.GetTaggedValue().GetRawData());

    JSHandle<JSTaggedValue> finRegLists = env->GetFinRegLists();
    EXPECT_EQ(finRegLists.GetTaggedValue().GetRawData(),
        JSHandle<JSTaggedValue>::Cast(finaRegObj).GetTaggedValue().GetRawData());
    EXPECT_EQ(testValue, 0);
}

HWTEST_F_L0(JSFinalizationRegistryTest, Register_003)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    vm->SetEnableForceGC(false);
    JSHandle<JSTaggedValue> target(thread, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
        JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());

        JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
        EXPECT_EQ(testValue, 0);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    EXPECT_EQ(testValue, 1);
}

HWTEST_F_L0(JSFinalizationRegistryTest, Register_004)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    vm->SetEnableForceGC(false);
    JSHandle<JSTaggedValue> target1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> target2(thread, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target1 = JSHandle<JSTaggedValue>::Cast(obj);
        target2 = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
        JSHandle<JSFinalizationRegistry> finaRegObj1(thread, constructor.GetTaggedValue());
        JSHandle<JSFinalizationRegistry> finaRegObj2(thread, constructor.GetTaggedValue());

        JSFinalizationRegistry::Register(thread, target1, heldValue, unregisterToken, finaRegObj1);
        JSFinalizationRegistry::Register(thread, target2, heldValue, unregisterToken, finaRegObj2);
        EXPECT_EQ(testValue, 0);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    EXPECT_EQ(testValue, 2);
}

/**
 * @tc.name: Unregister
 * @tc.desc: Unregister the JSFinalizationRegistry object from the finalization registry lists.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSFinalizationRegistryTest, Unregister_001)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    vm->SetEnableForceGC(false);
    JSHandle<JSTaggedValue> target(thread, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken = target;
        JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
        JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());

        JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
        JSFinalizationRegistry::Unregister(thread, target, finaRegObj);
        EXPECT_EQ(testValue, 0);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    EXPECT_EQ(testValue, 0);
}

HWTEST_F_L0(JSFinalizationRegistryTest, Unregister_002)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    vm->SetEnableForceGC(false);
    JSHandle<JSTaggedValue> target1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> target2(thread, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target1 = JSHandle<JSTaggedValue>::Cast(obj);
        target2 = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
        JSHandle<JSFinalizationRegistry> finaRegObj1(thread, constructor.GetTaggedValue());
        JSHandle<JSFinalizationRegistry> finaRegObj2(thread, constructor.GetTaggedValue());

        // only second finalization is be registered
        JSFinalizationRegistry::Register(thread, target1, heldValue, unregisterToken, finaRegObj1);
        JSFinalizationRegistry::Unregister(thread, target1, finaRegObj1);

        JSFinalizationRegistry::Register(thread, target2, heldValue, unregisterToken, finaRegObj2);
        EXPECT_EQ(testValue, 0);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    // only trigger second finalization callback
    EXPECT_EQ(testValue, 1);
}

/**
 * @tc.name: CheckAndCall
 * @tc.desc: Check whether each JSFinalizationRegistry object in finalization registry lists has been cleared. If so,
 *           clear the JSFinalizationRegistry object from the lists.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSFinalizationRegistryTest, CheckAndCall)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    JSHandle<JSTaggedValue> target(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
    JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());
    JSHandle<JSTaggedValue> finRegLists(thread, JSTaggedValue::Undefined());
    vm->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken(thread, JSTaggedValue::Undefined());
        JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
        EXPECT_EQ(testValue, 0);
    }
    finRegLists = env->GetFinRegLists();
    EXPECT_EQ(finRegLists.GetTaggedValue(), JSHandle<JSTaggedValue>::Cast(finaRegObj).GetTaggedValue());
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    EXPECT_EQ(testValue, 1);
    // If all objects registered in the current JSFinalizationRegistry are garbage collected,
    // clear the JSFinalizationRegistry from the list
    JSFinalizationRegistry::CheckAndCall(thread);
    finRegLists = env->GetFinRegLists();
    EXPECT_EQ(finRegLists.GetTaggedValue(), JSTaggedValue::Undefined());
}

/**
 * @tc.name: CleanupFinalizationRegistry
 * @tc.desc: Check current JSFinalizationRegistry, While contains any record cell such that cell and WeakRefTarget is
 *           empty, than remove cell from JSFinalizationRegistry and return whether there are still objects that have
 *           not been cleaned up.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSFinalizationRegistryTest, CleanupFinalizationRegistry)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    JSHandle<JSTaggedValue> target(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
    JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());
    bool isCleared = false;
    vm->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken(thread, JSTaggedValue::Undefined());
        JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
        EXPECT_EQ(testValue, 0);

        isCleared = JSFinalizationRegistry::CleanupFinalizationRegistry(thread, finaRegObj);
        EXPECT_FALSE(isCleared);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    EXPECT_EQ(testValue, 1);
    isCleared = JSFinalizationRegistry::CleanupFinalizationRegistry(thread, finaRegObj);
    EXPECT_TRUE(isCleared);
}

/**
 * @tc.name: CleanFinRegLists
 * @tc.desc: Clear the JSFinalizationRegistry object from the finalization registry lists.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSFinalizationRegistryTest, CleanFinRegLists)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    JSHandle<JSTaggedValue> target(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key3"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(3));
    JSObject::SetProperty(thread, target, key, value);
    JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
    JSHandle<JSTaggedValue> unregisterToken(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
    JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());

    JSFinalizationRegistry::Register(thread, target, heldValue, unregisterToken, finaRegObj);
    JSHandle<JSTaggedValue> finRegLists = env->GetFinRegLists();
    EXPECT_EQ(finRegLists.GetTaggedValue(), JSHandle<JSTaggedValue>::Cast(finaRegObj).GetTaggedValue());
    EXPECT_EQ(testValue, 0);

    JSFinalizationRegistry::CleanFinRegLists(thread, finaRegObj);
    finRegLists = env->GetFinRegLists();
    EXPECT_EQ(finRegLists.GetTaggedValue(), JSTaggedValue::Undefined());
}

/**
 * @tc.name: AddFinRegLists
 * @tc.desc: Add a JSFinalizationRegistry object to the finalization registry lists.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSFinalizationRegistryTest, AddFinRegLists)
{
    testValue = 0;
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> objectFunc = env->GetObjectFunction();
    vm->SetEnableForceGC(false);
    JSHandle<JSTaggedValue> target(thread, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc);
        target = JSHandle<JSTaggedValue>::Cast(obj);
        JSHandle<JSTaggedValue> heldValue(thread, JSTaggedValue(100));
        JSHandle<JSTaggedValue> unregisterToken(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> constructor = CreateFinalizationRegistry(thread);
        JSHandle<JSFinalizationRegistry> finaRegObj(thread, constructor.GetTaggedValue());

        JSHandle<CellRecord> cellRecord = factory->NewCellRecord();
        cellRecord->SetToWeakRefTarget(target.GetTaggedValue());
        cellRecord->SetHeldValue(thread, heldValue);
        JSHandle<JSTaggedValue> cell(cellRecord);
        JSHandle<CellRecordVector> noUnregister(thread, finaRegObj->GetNoUnregister());
        noUnregister = CellRecordVector::Append(thread, noUnregister, cell);
        finaRegObj->SetNoUnregister(thread, noUnregister);
        JSFinalizationRegistry::AddFinRegLists(thread, finaRegObj);
        JSHandle<JSTaggedValue> finRegLists = env->GetFinRegLists();
        EXPECT_EQ(finRegLists.GetTaggedValue(), JSHandle<JSTaggedValue>::Cast(finaRegObj).GetTaggedValue());
        EXPECT_EQ(testValue, 0);
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    if (!thread->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread, vm->GetMicroJobQueue());
    }
    vm->SetEnableForceGC(true);
    EXPECT_EQ(testValue, 1);
}
}  // namespace panda::test