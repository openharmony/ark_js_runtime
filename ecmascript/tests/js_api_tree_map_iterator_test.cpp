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

#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_tree_map.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPITreeMapIteratorTest : public testing::Test {
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

protected:
    JSHandle<JSAPITreeMap> CreateTreeMap()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::TreeMap)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = containers::ContainersPrivate::Load(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPITreeMap> jsTreeMap(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue internal = TaggedTreeMap::Create(thread);
        jsTreeMap->SetTreeMap(thread, internal);
        return jsTreeMap;
    }
};

/**
 * @tc.name: SetIterationKind
 * @tc.desc: Call the "SetIterationKind" function, check whether the result returned through "GetIterationKind"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SetIterationKind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSAPITreeMapIterator> treeMapIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY);
    EXPECT_EQ(treeMapIterator->GetIterationKind(), IterationKind::KEY);
    treeMapIterator->SetIterationKind(IterationKind::VALUE);
    EXPECT_EQ(treeMapIterator->GetIterationKind(), IterationKind::VALUE);
    treeMapIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(treeMapIterator->GetIterationKind(), IterationKind::KEY_AND_VALUE);
}

/**
 * @tc.name: SetIteratedMap
 * @tc.desc: Call the "SetIteratedMap" function, check whether the result returned through "GetIteratedMap"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SetIteratedMap)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSAPITreeMapIterator> treeMapIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::VALUE);

    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i);
        std::string ivalue = mapValue + std::to_string(i + 2U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, jsTreeMap, key, value);
    }
    treeMapIterator->SetIteratedMap(thread, jsTreeMap.GetTaggedValue());
    JSHandle<JSAPITreeMap> treeMapTo(thread, JSAPITreeMap::Cast(treeMapIterator->GetIteratedMap().GetTaggedObject()));
    EXPECT_EQ(treeMapTo->GetSize(), static_cast<int>(DEFAULT_LENGTH));
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i);
        std::string ivalue = mapValue + std::to_string(i + 2U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue result = JSAPITreeMap::Get(thread, treeMapTo, key);
        EXPECT_EQ(result, value.GetTaggedValue());
    }
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSAPITreeMapIterator> treeMapIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(treeMapIterator->GetNextIndex(), 0U);
    
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        treeMapIterator->SetNextIndex(i);
        EXPECT_EQ(treeMapIterator->GetNextIndex(), i);
    }
}

/**
 * @tc.name: CreateTreeMapIterator
 * @tc.desc: Create TreeMap iterator, check whether the result returned through "IsJSAPITreeMapIterator"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, CreateTreeMapIterator)
{
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSTaggedValue> treeMapVal(jsTreeMap);
    // Create Iterator with KEY
    JSHandle<JSTaggedValue> treeMapIterator =
        JSAPITreeMapIterator::CreateTreeMapIterator(thread, treeMapVal, IterationKind::KEY);
    EXPECT_TRUE(*treeMapIterator != nullptr);
    EXPECT_TRUE(treeMapIterator->IsJSAPITreeMapIterator());
    // Create Iterator with VALUE
    treeMapIterator = JSAPITreeMapIterator::CreateTreeMapIterator(thread, treeMapVal, IterationKind::VALUE);
    EXPECT_TRUE(*treeMapIterator != nullptr);
    EXPECT_TRUE(treeMapIterator->IsJSAPITreeMapIterator());
    // Create Iterator with KEY_AND_VALUE
    treeMapIterator = JSAPITreeMapIterator::CreateTreeMapIterator(thread, treeMapVal, IterationKind::KEY_AND_VALUE);
    EXPECT_TRUE(*treeMapIterator != nullptr);
    EXPECT_TRUE(treeMapIterator->IsJSAPITreeMapIterator());
}

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPITreeMap,and then loop through the elements(key,value and keyAndvalue) of the
 *           iterator to check whether the elements through "Next" function are consistent.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, KEY_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i);
        std::string ivalue = mapValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, jsTreeMap, key, value);
    }
    // Create Iterator with KEY
    JSHandle<JSAPITreeMapIterator> treeMapKeyIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY);
    // traversal iterator
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapKeyIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKey = mapKey + std::to_string(i);
        key.Update(factory->NewFromStdString(resultKey).GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::SameValue(
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), key), true);
        EXPECT_EQ(treeMapKeyIterator->GetNextIndex(), (i + 1U));
    }
}

HWTEST_F_L0(JSAPITreeMapIteratorTest, VALUE_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i);
        std::string ivalue = mapValue + std::to_string(i + 1U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, jsTreeMap, key, value);
    }
    // Create Iterator with VALUE
    JSHandle<JSAPITreeMapIterator> treeMapKeyIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::VALUE);
    // traversal iterator
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapKeyIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKey = mapValue + std::to_string(i + 1U);
        value.Update(factory->NewFromStdString(resultKey).GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::SameValue(
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), value), true);
        EXPECT_EQ(treeMapKeyIterator->GetNextIndex(), (i + 1U));
    }
}

HWTEST_F_L0(JSAPITreeMapIteratorTest, KEY_AND_VALUE_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i + 1U);
        std::string ivalue = mapValue + std::to_string(i + 2U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, jsTreeMap, key, value);
    }
    // Create Iterator with KEY_AND_VALUE
    JSHandle<JSAPITreeMapIterator> treeMapKeyIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY_AND_VALUE);
    // traversal iterator
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapKeyIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKeyAndValue = mapKey + std::to_string(i + 1U);
        value.Update(factory->NewFromStdString(resultKeyAndValue).GetTaggedValue());
        
        JSHandle<JSTaggedValue> keyValueArr(JSObject::GetProperty(thread, resultObj, valueStr).GetValue());
        for (int index = 0; index < 2; index++) {
            JSHandle<JSTaggedValue> indexValue(thread, JSTaggedValue(index));
            EXPECT_EQ(JSTaggedValue::SameValue(
                JSObject::GetProperty(thread, keyValueArr, indexValue).GetValue(), value), true);
            resultKeyAndValue = mapValue + std::to_string(i + 2U);
            value.Update(factory->NewFromStdString(resultKeyAndValue).GetTaggedValue());
        }
        EXPECT_EQ(treeMapKeyIterator->GetNextIndex(), (i + 1U));
    }
}
}  // namespace panda::ecmascript