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

#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_tree_set.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPITreeSetIteratorTest : public testing::Test {
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
    JSHandle<JSAPITreeSet> CreateTreeSet()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::TreeSet)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPITreeSet> jsSet(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue internal = TaggedTreeSet::Create(thread);
        jsSet->SetTreeSet(thread, internal);
        return jsSet;
    }
};

/**
 * @tc.name: SetIterationKind
 * @tc.desc: Call the "SetIterationKind" function, check whether the result returned through "GetIterationKind"
 *           function from the JSAPITreeSetIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeSetIteratorTest, SetIterationKind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeSet> jsTreeSet = CreateTreeSet();
    EXPECT_TRUE(*jsTreeSet != nullptr);
    JSHandle<JSAPITreeSetIterator> treeSetIterator =
        factory->NewJSAPITreeSetIterator(jsTreeSet, IterationKind::KEY);
    EXPECT_EQ(treeSetIterator->GetIterationKind(), IterationKind::KEY);
    treeSetIterator->SetIterationKind(IterationKind::VALUE);
    EXPECT_EQ(treeSetIterator->GetIterationKind(), IterationKind::VALUE);
    treeSetIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(treeSetIterator->GetIterationKind(), IterationKind::KEY_AND_VALUE);
}

/**
 * @tc.name: SetIteratedSet
 * @tc.desc: Call the "SetIteratedSet" function, check whether the result returned through "GetIteratedSet"
 *           function from the JSAPITreeSetIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeSetIteratorTest, SetIteratedSet)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPITreeSet> jsTreeSet = CreateTreeSet();
    EXPECT_TRUE(*jsTreeSet != nullptr);
    JSHandle<JSAPITreeSetIterator> treeSetIterator =
        factory->NewJSAPITreeSetIterator(jsTreeSet, IterationKind::VALUE);

    std::string setKey("setkey");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = setKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        JSAPITreeSet::Add(thread, jsTreeSet, key);
    }
    treeSetIterator->SetIteratedSet(thread, jsTreeSet.GetTaggedValue());
    JSHandle<JSAPITreeSet> treeSetTo(thread, JSAPITreeSet::Cast(treeSetIterator->GetIteratedSet().GetTaggedObject()));
    EXPECT_EQ(treeSetTo->GetSize(), static_cast<int>(DEFAULT_LENGTH));
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = setKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        EXPECT_TRUE(JSAPITreeSet::Has(thread, jsTreeSet, key));
    }
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPITreeSetIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeSetIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeSet> jsTreeSet = CreateTreeSet();
    EXPECT_TRUE(*jsTreeSet != nullptr);
    JSHandle<JSAPITreeSetIterator> treeSetIterator =
        factory->NewJSAPITreeSetIterator(jsTreeSet, IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(treeSetIterator->GetNextIndex(), 0U);
    
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        treeSetIterator->SetNextIndex(i);
        EXPECT_EQ(treeSetIterator->GetNextIndex(), i);
    }
}

/**
 * @tc.name: CreateTreeSetIterator
 * @tc.desc: Create TreeSet iterator, check whether the result returned through "IsJSAPITreeSetIterator"
 *           function from the JSAPITreeSetIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeSetIteratorTest, CreateTreeSetIterator)
{
    JSHandle<JSAPITreeSet> jsTreeSet = CreateTreeSet();
    EXPECT_TRUE(*jsTreeSet != nullptr);
    JSHandle<JSTaggedValue> treeSetVal(jsTreeSet);
    // Create Iterator with KEY
    JSHandle<JSTaggedValue> treeSetIterator =
        JSAPITreeSetIterator::CreateTreeSetIterator(thread, treeSetVal, IterationKind::KEY);
    EXPECT_TRUE(*treeSetIterator != nullptr);
    EXPECT_TRUE(treeSetIterator->IsJSAPITreeSetIterator());
    // Create Iterator with VALUE
    treeSetIterator = JSAPITreeSetIterator::CreateTreeSetIterator(thread, treeSetVal, IterationKind::VALUE);
    EXPECT_TRUE(*treeSetIterator != nullptr);
    EXPECT_TRUE(treeSetIterator->IsJSAPITreeSetIterator());
    // Create Iterator with KEY_AND_VALUE
    treeSetIterator = JSAPITreeSetIterator::CreateTreeSetIterator(thread, treeSetVal, IterationKind::KEY_AND_VALUE);
    EXPECT_TRUE(*treeSetIterator != nullptr);
    EXPECT_TRUE(treeSetIterator->IsJSAPITreeSetIterator());
}

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPITreeSet,and then loop through the elements(key and keyAndvalue) of the
 *           iterator to check whether the elements through "Next" function are consistent.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeSetIteratorTest, KEY_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPITreeSet> jsTreeSet = CreateTreeSet();
    std::string setKey("setkey");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = setKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        JSAPITreeSet::Add(thread, jsTreeSet, key);
    }
    // Create Iterator with KEY or VALUE
    JSHandle<JSAPITreeSetIterator> treeSetIterator =
        factory->NewJSAPITreeSetIterator(jsTreeSet, IterationKind::KEY);
    // traversal iterator
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeSetIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = JSAPITreeSetIterator::Next(ecmaRuntimeCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKey = setKey + std::to_string(i);
        key.Update(factory->NewFromStdString(resultKey).GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::SameValue(
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), key), true);
        EXPECT_EQ(treeSetIterator->GetNextIndex(), (i + 1U));
    }
}

HWTEST_F_L0(JSAPITreeSetIteratorTest, KEY_AND_VALUE_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPITreeSet> jsTreeSet = CreateTreeSet();
    std::string setKey("setkey");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = setKey + std::to_string(i + 1U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        JSAPITreeSet::Add(thread, jsTreeSet, key);
    }
    // Create Iterator with KEY_AND_VALUE
    JSHandle<JSAPITreeSetIterator> treeSetIterator =
        factory->NewJSAPITreeSetIterator(jsTreeSet, IterationKind::KEY_AND_VALUE);
    // traversal iterator
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeSetIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = JSAPITreeSetIterator::Next(ecmaRuntimeCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKeyAndValue = setKey + std::to_string(i + 1U);
        key.Update(factory->NewFromStdString(resultKeyAndValue).GetTaggedValue());
        
        JSHandle<JSTaggedValue> keyValueArr(JSObject::GetProperty(thread, resultObj, valueStr).GetValue());
        for (int index = 0; index < 2; index++) {
            JSHandle<JSTaggedValue> indexValue(thread, JSTaggedValue(index));
            EXPECT_EQ(JSTaggedValue::SameValue(
                JSObject::GetProperty(thread, keyValueArr, indexValue).GetValue(), key), true);
        }
        EXPECT_EQ(treeSetIterator->GetNextIndex(), (i + 1U));
    }
}
}  // namespace panda::ecmascript