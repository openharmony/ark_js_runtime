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
#include "ecmascript/containers/containers_treemap.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_tree_map.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersTreeMapTest : public testing::Test {
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
            JSHandle<JSTaggedValue> map = GetCallArg(argv, 2); // 2 means the secode arg
            if (!map->IsUndefined()) {
                if (value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSAPITreeMap::Set(thread, JSHandle<JSAPITreeMap>::Cast(map), key, newValue);
                }
            }
            JSHandle<JSAPITreeMap> jsTreeMap(GetThis(argv));
            JSAPITreeMap::Set(thread, jsTreeMap, key, value);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestCompareFunction(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> valueX = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> valueY = GetCallArg(argv, 1);

            if (valueX->IsString() && valueY->IsString()) {
                auto xString = static_cast<EcmaString *>(valueX->GetTaggedObject());
                auto yString = static_cast<EcmaString *>(valueY->GetTaggedObject());
                int result = xString->Compare(yString);
                if (result < 0) {
                    return JSTaggedValue(1);
                }
                if (result == 0) {
                    return JSTaggedValue(0);
                }
                return JSTaggedValue(-1);
            }

            if (valueX->IsNumber() && valueY->IsString()) {
                return JSTaggedValue(1);
            }
            if (valueX->IsString() && valueY->IsNumber()) {
                return JSTaggedValue(-1);
            }

            ComparisonResult res = ComparisonResult::UNDEFINED;
            if (valueX->IsNumber() && valueY->IsNumber()) {
                res = JSTaggedValue::StrictNumberCompare(valueY->GetNumber(), valueX->GetNumber());
            } else {
                res = JSTaggedValue::Compare(thread, valueY, valueX);
            }
            return res == ComparisonResult::GREAT ?
                JSTaggedValue(1) : (res == ComparisonResult::LESS ? JSTaggedValue(-1) : JSTaggedValue(0));
        }
    };

protected:
    JSTaggedValue InitializeTreeMapConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::TreeMap)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPITreeMap> CreateJSAPITreeMap(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeTreeMapConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersTreeMap::TreeMapConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPITreeMap> map(thread, result);
        return map;
    }
};

// new TreeMap()
HWTEST_F_L0(ContainersTreeMapTest, TreeMapConstructor)
{
    // Initialize twice and return directly the second time
    InitializeTreeMapConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeTreeMapConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersTreeMap::TreeMapConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPITreeMap());
    JSHandle<JSAPITreeMap> mapHandle(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(mapHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = mapHandle->GetSize();
    ASSERT_EQ(size, 0);
}

// treemap.set(key, value), treemap.get(key)
HWTEST_F_L0(ContainersTreeMapTest, SetAndGet)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, value.GetTaggedValue());
    }
}

// treemap.remove(key)
HWTEST_F_L0(ContainersTreeMapTest, Remove)
{
    constexpr int NODE_NUMBERS = 64;
    constexpr int REMOVE_SIZE = 48;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    for (int i = 0; i < REMOVE_SIZE; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersTreeMap::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue(i));
    }
    EXPECT_EQ(tmap->GetSize(), NODE_NUMBERS - REMOVE_SIZE);

    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i < REMOVE_SIZE) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS - REMOVE_SIZE + i + 1);
    }

    for (int i = 0; i < REMOVE_SIZE; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersTreeMap::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(rvalue, value.GetTaggedValue()));
    }
    EXPECT_EQ(tmap->GetSize(), (NODE_NUMBERS - REMOVE_SIZE) * 2);
}

// treemap.hasKey(key), treemap.hasValue(value)
HWTEST_F_L0(ContainersTreeMapTest, HasKeyAndHasValue)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        // test hasKey
        {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tmap.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeMap::HasKey(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
        // test hasValue
        {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tmap.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeMap::HasValue(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        // test hasKey
        {
            std::string ikey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tmap.GetTaggedValue());
            callInfo->SetCallArg(0, key.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeMap::HasKey(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
        // test hasValue
        {
            std::string ivalue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tmap.GetTaggedValue());
            callInfo->SetCallArg(0, value.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersTreeMap::HasValue(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
    }
}

// treemap.getFirstKey(), treemap.getLastKey()
HWTEST_F_L0(ContainersTreeMapTest, GetFirstKeyAndGetLastKey)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }
    // test getFirstKey
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetFirstKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
    // test getLastKey
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetLastKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS - 1));
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    // test getFirstKey
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetFirstKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
    // test getLastKey
    {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetLastKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, key.GetTaggedValue());
    }
}

// treemap.clear()
HWTEST_F_L0(ContainersTreeMapTest, Clear)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tmap->GetSize(), 0);
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Undefined());
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tmap->GetSize(), 0);
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Undefined());
    }
}

// treemap.setAll(map)
HWTEST_F_L0(ContainersTreeMapTest, SetAll)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> smap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(smap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    JSHandle<JSAPITreeMap> dmap = CreateJSAPITreeMap();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, smap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::SetAll(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(dmap->GetSize(), NODE_NUMBERS);
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(smap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, smap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::SetAll(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(dmap->GetSize(), NODE_NUMBERS * 2);
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, value.GetTaggedValue()));
    }
    EXPECT_EQ(dmap->GetSize(), 2 * NODE_NUMBERS);
}

// treemap.getLowerKey(key), treemap.getHigherKey(key)
HWTEST_F_L0(ContainersTreeMapTest, GetLowerKeyAndGetHigherKey)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test getLowerKey
    for (int i = 0; i <= NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetLowerKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i == 0) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(result, JSTaggedValue(i - 1));
        }
    }
    // test getHigherKey
    for (int i = 0; i <= NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetHigherKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i >= NODE_NUMBERS - 1) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(result, JSTaggedValue(i + 1));
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    // test getLowerKey
    for (int i = 0; i <= NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string ivalue = myKey+ std::to_string(i - 1);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);

        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetLowerKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i == 0) {
            EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS - 1));
        } else {
            EXPECT_TRUE(JSTaggedValue::SameValue(result, value.GetTaggedValue()));
        }
    }
    // test getHigherKey
    for (int i = 0; i <= NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string ivalue = myKey+ std::to_string(i + 1);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::GetHigherKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i >= NODE_NUMBERS - 1) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_TRUE(JSTaggedValue::SameValue(result, value.GetTaggedValue()));
        }
    }
}

// treemap.keys(), treemap.values(), treemap.entries()
HWTEST_F_L0(ContainersTreeMapTest, KeysAndValuesAndEntries)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test keys
    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(tmap.GetTaggedValue());
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterKeys(thread, ContainersTreeMap::Keys(callInfo1));
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(iterKeys->IsJSAPITreeMapIterator());

    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterKeys.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(i, JSIterator::IteratorValue(thread, result)->GetInt());
    }

    // test values
    callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(tmap.GetTaggedValue());
    auto prev2 = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersTreeMap::Values(callInfo1));
    TestHelper::TearDownFrame(thread, prev2);
    EXPECT_TRUE(iterValues->IsJSAPITreeMapIterator());

    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(i, JSIterator::IteratorValue(thread, result)->GetInt());
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result1 = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result1.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result1.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }
    // test keys after add
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterKeys.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSTaggedValue> itRes = JSIterator::IteratorValue(thread, result);
        EXPECT_TRUE(JSTaggedValue::SameValue(key, itRes));
    }
    // test values after add
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(value, JSIterator::IteratorValue(thread, result)));
    }

    // test entries
    {
        auto callInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo3->SetFunction(JSTaggedValue::Undefined());
        callInfo3->SetThis(tmap.GetTaggedValue());
        [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, callInfo3);
        JSHandle<JSTaggedValue> iter(thread, ContainersTreeMap::Entries(callInfo3));
        TestHelper::TearDownFrame(thread, prev3);
        EXPECT_TRUE(iter->IsJSAPITreeMapIterator());

        JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
        JSMutableHandle<JSTaggedValue> result1(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
        for (int i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            result1.Update(JSAPITreeMapIterator::Next(callInfo));
            TestHelper::TearDownFrame(thread, prev);
            entries.Update(JSIterator::IteratorValue(thread, result1).GetTaggedValue());
            EXPECT_EQ(i, JSObject::GetProperty(thread, entries, first).GetValue()->GetInt());
            EXPECT_EQ(i, JSObject::GetProperty(thread, entries, second).GetValue()->GetInt());
        }

        for (int i = 0; i < NODE_NUMBERS; i++) {
            std::string ikey = myKey + std::to_string(i);
            std::string ivalue = myValue + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            result.Update(JSAPITreeMapIterator::Next(callInfo));
            TestHelper::TearDownFrame(thread, prev);
            entries.Update(JSIterator::IteratorValue(thread, result).GetTaggedValue());
            EXPECT_TRUE(JSTaggedValue::SameValue(key, JSObject::GetProperty(thread, entries, first).GetValue()));
            EXPECT_TRUE(JSTaggedValue::SameValue(value, JSObject::GetProperty(thread, entries, second).GetValue()));
        }
    }
}

// treemap.replace(key, value)
HWTEST_F_L0(ContainersTreeMapTest, Replace)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));
        callInfo->SetCallArg(1, JSTaggedValue(NODE_NUMBERS));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Replace(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(tmap->GetSize(), NODE_NUMBERS);
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i == (NODE_NUMBERS / 2)) {
            EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS));
        } else {
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS / 2);
        std::string ivalue = myValue + std::to_string(NODE_NUMBERS);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Replace(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(tmap->GetSize(), NODE_NUMBERS * 2);
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string ivalue;
        if (i == (NODE_NUMBERS / 2)) {
            ivalue = myValue + std::to_string(NODE_NUMBERS);
        } else {
            ivalue = myValue + std::to_string(i);
        }
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, value.GetTaggedValue()));
    }
}

// treemap.ForEach(callbackfn, this)
HWTEST_F_L0(ContainersTreeMapTest, ForEach)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }
    // test foreach function with TestForEachFunc;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMap> dmap = CreateJSAPITreeMap();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    EXPECT_EQ(dmap->GetSize(), NODE_NUMBERS);
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 2));
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }

    // test add string
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    // test foreach function with TestForEachFunc;
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    EXPECT_EQ(dmap->GetSize(), NODE_NUMBERS * 2);
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 4)); // 4 means 4 times
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 2));
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, value.GetTaggedValue());
    }
}

HWTEST_F_L0(ContainersTreeMapTest, CustomCompareFunctionTest)
{
    constexpr int NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestCompareFunction));
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(func.GetTaggedValue());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test add string
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    // test sort
    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(tmap.GetTaggedValue());
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterKeys(thread, ContainersTreeMap::Keys(callInfo1));
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(iterKeys->IsJSAPITreeMapIterator());

    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1 - i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterKeys.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSTaggedValue> itRes = JSIterator::IteratorValue(thread, result);
        EXPECT_TRUE(JSTaggedValue::SameValue(key, itRes));
    }
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterKeys.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPITreeMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ((NODE_NUMBERS - 1 - i), JSIterator::IteratorValue(thread, result)->GetInt());
    }
}

// treemap.isEmpty()
HWTEST_F_L0(ContainersTreeMapTest, IsEmpty)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap();
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersTreeMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPITreeMap());
        EXPECT_EQ(JSAPITreeMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
        JSTaggedValue isEmpty = ContainersTreeMap::IsEmpty(callInfo);
        EXPECT_EQ(isEmpty, JSTaggedValue::False());
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersTreeMap::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSTaggedValue isEmpty = ContainersTreeMap::IsEmpty(callInfo);
        EXPECT_EQ(isEmpty, JSTaggedValue::True());
    }
}
}  // namespace panda::test
