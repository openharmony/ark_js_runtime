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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_lightweightmap.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_lightweightmap.h"
#include "ecmascript/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersLightWeightMapTest : public testing::Test {
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
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            [[maybe_unused]] JSHandle<JSTaggedValue> map = GetCallArg(argv, 2); // 2 means the secode arg
            
            JSHandle<JSAPILightWeightMap> jsTreeMap(GetThis(argv));
            JSAPILightWeightMap::Set(thread, jsTreeMap, key, value);
            return JSTaggedValue::Undefined();
        }
    };
protected:
    JSTaggedValue InitializeLightWeightMapConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(
            0, JSTaggedValue(static_cast<int>(ContainerTag::LightWeightMap))); // 0 means the argument
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPILightWeightMap> CreateJSAPILightWeightMap()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeLightWeightMapConstructor());
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersLightWeightMap::LightWeightMapConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPILightWeightMap> map(thread, result);
        return map;
    }
};

HWTEST_F_L0(ContainersLightWeightMapTest, LightWeightMapConstructor)
{
    InitializeLightWeightMapConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeLightWeightMapConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersLightWeightMap::LightWeightMapConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPILightWeightMap());
    JSHandle<JSAPILightWeightMap> mapHandle(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(mapHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
}

HWTEST_F_L0(ContainersLightWeightMapTest, SetAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result),
                                         JSHandle<JSTaggedValue>(thread, JSTaggedValue::True())));
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = lightWeightMap->GetLength();
        EXPECT_EQ(length, i + 1);
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = lightWeightMap->GetLength();
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result),
                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::True())));
        EXPECT_EQ(length, NODE_NUMBERS + 1 + i);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i + 1));
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, value.GetTaggedValue());
    }
}

HWTEST_F_L0(ContainersLightWeightMapTest, Remove)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        EXPECT_EQ(result, JSTaggedValue::True());
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = lightWeightMap->GetLength();
        EXPECT_EQ(length, i + 1);
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersLightWeightMap::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue(NODE_NUMBERS / 2  + 1));
        uint32_t len = lightWeightMap->GetLength();
        EXPECT_EQ(len, NODE_NUMBERS - 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue keyResult = ContainersLightWeightMap::HasKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (NODE_NUMBERS / 2 == i) {
            EXPECT_EQ(keyResult, JSTaggedValue::False());
        } else {
            EXPECT_EQ(keyResult, JSTaggedValue::True());
        }

        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i + 1));
        prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue valueResult = ContainersLightWeightMap::HasValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (NODE_NUMBERS / 2 == i) {
            EXPECT_EQ(valueResult, JSTaggedValue::False());
        } else {
            EXPECT_EQ(valueResult, JSTaggedValue::True());
        }
    }
}

HWTEST_F_L0(ContainersLightWeightMapTest, Clear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = lightWeightMap->GetLength();
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result),
                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::True())));
        EXPECT_EQ(length, i + 1);
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLightWeightMap::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t len = lightWeightMap->GetLength();
        uint32_t empty = 0;
        EXPECT_EQ(len, empty);
    }
}

HWTEST_F_L0(ContainersLightWeightMapTest, ToString)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = lightWeightMap->GetLength();
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result),
                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::True())));
        EXPECT_EQ(length, i + 1);
    }

    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersLightWeightMap::ToString(callInfo);
        JSHandle<JSTaggedValue> resHandle(thread, res);
        TestHelper::TearDownFrame(thread, prev);
        std::string myString("0:0,1:1,2:2,3:3,4:4,5:5,6:6,7:7");
        JSHandle<JSTaggedValue> expectResHandle(thread, factory->NewFromStdString(myString).GetTaggedValue());
        EXPECT_TRUE(JSTaggedValue::Equal(thread, resHandle, expectResHandle));
    }
}

// LightWeightMap.setAll(map)
HWTEST_F_L0(ContainersLightWeightMapTest, SetAll)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> oldLightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(oldLightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = oldLightWeightMap->GetLength();
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result),
                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::True())));
        EXPECT_EQ(length, i + 1);
    }

    JSHandle<JSAPILightWeightMap> LightWeightMap = CreateJSAPILightWeightMap();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(LightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, oldLightWeightMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLightWeightMap::SetAll(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = LightWeightMap->GetLength();
        EXPECT_EQ(length, NODE_NUMBERS);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(LightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue keyResult = ContainersLightWeightMap::HasKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(keyResult, JSTaggedValue::True());

        prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue valueResult = ContainersLightWeightMap::HasValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(valueResult, JSTaggedValue::True());
    }
}

HWTEST_F_L0(ContainersLightWeightMapTest, KeysAndValuesAndEntries)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t length = lightWeightMap->GetLength();
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result),
                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::True())));
        EXPECT_EQ(length, i + 1);
    }

    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(lightWeightMap.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSHandle<JSTaggedValue> iterKeys(thread, ContainersLightWeightMap::Keys(callInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterKeys->IsJSAPILightWeightMapIterator());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterKeys.GetTaggedValue());

        prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPILightWeightMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSTaggedValue> keyHandle = JSIterator::IteratorValue(thread, result);

        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, keyHandle.GetTaggedValue());

        prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue keyResult = ContainersLightWeightMap::HasKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(keyResult, JSTaggedValue::True());
    }

    // test values
    callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(lightWeightMap.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, callInfo);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersLightWeightMap::Values(callInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterValues->IsJSAPILightWeightMapIterator());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPILightWeightMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSTaggedValue> ValueHandle = JSIterator::IteratorValue(thread, result);

        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, ValueHandle.GetTaggedValue());

        prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue valueResult = ContainersLightWeightMap::HasValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(valueResult, JSTaggedValue::True());
    }
    // test entries
    callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(lightWeightMap.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, callInfo);
    JSHandle<JSTaggedValue> iter(thread, ContainersLightWeightMap::Entries(callInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iter->IsJSAPILightWeightMapIterator());

    JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
    JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iter.GetTaggedValue());

        prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPILightWeightMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        entries.Update(JSIterator::IteratorValue(thread, result).GetTaggedValue());

        int entriesKey = JSObject::GetProperty(thread, entries, first).GetValue()->GetInt();
        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(entriesKey));
        prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue keyResult = ContainersLightWeightMap::HasKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(keyResult, JSTaggedValue::True());

        int entriesValue = JSObject::GetProperty(thread, entries, second).GetValue()->GetInt();
        callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(entriesValue));
        prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue valueResult = ContainersLightWeightMap::HasValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(valueResult, JSTaggedValue::True());
    }
}

// LightWeightMap.ForEach(callbackfn, this)
HWTEST_F_L0(ContainersLightWeightMapTest, ForEach)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        uint32_t len = lightWeightMap->GetLength();
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(len, i + 1);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightMap> newLightWeightMap = CreateJSAPILightWeightMap();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(lightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, newLightWeightMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLightWeightMap::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(newLightWeightMap->GetLength(), NODE_NUMBERS);
    EXPECT_EQ(lightWeightMap->GetLength(), NODE_NUMBERS);


    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(newLightWeightMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLightWeightMap::HasKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
    }
}
}  // namespace panda::test
