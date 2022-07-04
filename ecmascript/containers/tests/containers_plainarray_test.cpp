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
 
#include "ecmascript/js_api_plain_array.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_plainarray.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_plain_array_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersPlainArrayTest : public testing::Test {
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
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);  // 0 means the value
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 1); // 1 means the value
            JSHandle<JSAPIPlainArray> plainArray(GetCallArg(argv, 2)); // 2 means the value
            if (key->IsNumber()) {
                JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means the value
                JSAPIPlainArray::Add(thread, plainArray, key, newValue);
            }
 
            return JSTaggedValue::True();
        }
    };
protected:
    JSTaggedValue InitializePlainArrayConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);   // 6 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::PlainArray)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIPlainArray> CreateJSAPIPlainArray()
    {
        JSHandle<JSFunction> newTarget(thread, InitializePlainArrayConstructor());
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);   // 4 means the value
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPlainArray::PlainArrayConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIPlainArray> plain(thread, result);
        return plain;
    }
};

HWTEST_F_L0(ContainersPlainArrayTest, PlainArrayConstructor)
{
    InitializePlainArrayConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializePlainArrayConstructor());
    auto objCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);    // 4 means the value
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersPlainArray::PlainArrayConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIPlainArray());
    JSHandle<JSAPIPlainArray> arrayHandle(thread, result);
    JSTaggedValue resultProto = JSObject::GetPrototype(JSHandle<JSObject>::Cast(arrayHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = arrayHandle->GetSize();
    ASSERT_EQ(size, 0);
}

HWTEST_F_L0(ContainersPlainArrayTest, AddAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;    // 8 means the value
    
    JSHandle<JSAPIPlainArray> tArray1 = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray1.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tArray1->GetSize(), static_cast<int>(i + 1));
    }
    EXPECT_EQ(tArray1->GetSize(), static_cast<int>(NODE_NUMBERS));
    // test has
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray1.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(tArray1->GetSize(), static_cast<int>(NODE_NUMBERS));
    // test add string
    JSHandle<JSAPIPlainArray> tArray = CreateJSAPIPlainArray();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(i));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(tArray->GetSize(), static_cast<int>(NODE_NUMBERS));
    // test get
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(i));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result), value));
    }
}

HWTEST_F_L0(ContainersPlainArrayTest, Iterator)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIPlainArray> array = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(array.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(array->GetSize(), static_cast<int>(i + 1));
    }
    // test iterator
    {
        auto callInf = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInf->SetFunction(JSTaggedValue::Undefined());
        callInf->SetThis(array.GetTaggedValue());
        [[maybe_unused]] auto pre = TestHelper::SetupFrame(thread, callInf);
        JSHandle<JSTaggedValue> iter(thread, ContainersPlainArray::GetIteratorObj(callInf));
        TestHelper::TearDownFrame(thread, pre);
        EXPECT_TRUE(iter->IsJSAPIPlainArrayIterator());

        JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            result.Update(JSAPIPlainArrayIterator::Next(callInfo));
            TestHelper::TearDownFrame(thread, prev);
            entries.Update(JSIterator::IteratorValue(thread, result).GetTaggedValue());
            EXPECT_EQ(static_cast<int>(i), JSObject::GetProperty(thread, entries, first).GetValue()->GetInt());
            EXPECT_EQ(static_cast<int>(i + 1), JSObject::GetProperty(thread, entries, second).GetValue()->GetInt());
        }
    }
    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(array.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(100 + i));
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS + i + 1));
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS * 2));
}

HWTEST_F_L0(ContainersPlainArrayTest, GetIndexOfKeyAndGetIndexOfValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIPlainArray> pArray = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 4 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(pArray->GetSize(), static_cast<int>(i + 1));
    }
    // test GetIndexOfKey
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(2));
    }
    // test GetIndexOfValue
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(4));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(3));
    }
    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(100 + i));
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(pArray->GetSize(), static_cast<int>(NODE_NUMBERS + i + 1));
    }
    EXPECT_EQ(pArray->GetSize(), static_cast<int>(NODE_NUMBERS * 2));
    // test GetIndexOfKey
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(102));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(10));
    }
    // test GetIndexOfValue
    {
        std::string tValue("myvalue3");
        value.Update(factory->NewFromStdString(tValue).GetTaggedValue());
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(11));
    }
}
}
