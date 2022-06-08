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

#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_atomics.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using TypedArray = ecmascript::builtins::BuiltinsTypedArray;
class BuiltinsAtomicsTest : public testing::Test {
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
};

JSTypedArray *CreateTypedArray(JSThread *thread, const JSHandle<TaggedArray> &array)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int8_array(env->GetInt8ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    //  6 : test case
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int8_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int8_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = TypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1.get());

    EXPECT_TRUE(result.IsECMAObject());
    JSTypedArray *int8arr = JSTypedArray::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return int8arr;
}

HWTEST_F_L0(BuiltinsAtomicsTest, Add_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(7));
    array->Set(thread, 1, JSTaggedValue(8));
    array->Set(thread, 2, JSTaggedValue(9));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Add(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Add_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(10));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Add(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 0);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Add_3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Add(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 0);


    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    JSTaggedValue results = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(results.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, And_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(10));
    array->Set(thread, 0, JSTaggedValue(7));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Add(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, And_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(10));
    array->Set(thread, 0, JSTaggedValue(7));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::And(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 7);
    
    
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, CompareExchange_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::CompareExchange(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, CompareExchange_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::CompareExchange(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
    
    
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Exchange_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(3));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(6)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Exchange(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 3);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Exchange_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(3));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(6)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Exchange(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 3);
    
    
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 6);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Or_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Or(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Or_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Or(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
    
    
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Sub_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(0));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Sub(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Sub_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(0));
    array->Set(thread, 1, JSTaggedValue(5));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Sub(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
    
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 3);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Xor_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(7));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Xor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Xor_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(7));
    array->Set(thread, 2, JSTaggedValue(0));
    
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));
    
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Xor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 7);
    
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(8)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}


HWTEST_F_L0(BuiltinsAtomicsTest, Store_1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(6));
    array->Set(thread, 2, JSTaggedValue(7));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Store(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetDouble(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Store_2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(5));
    array->Set(thread, 1, JSTaggedValue(6));
    array->Set(thread, 2, JSTaggedValue(7));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsAtomics::Store(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetDouble(), 2);

    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfos->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos.get());
    result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetInt(), 2);
}
}