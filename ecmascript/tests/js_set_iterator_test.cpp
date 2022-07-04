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

#include "ecmascript/js_set_iterator.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_set.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSSetIteratorTest : public testing::Test {
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
};

static JSSet *CreateJSSet(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    return JSSet::Cast(set.GetTaggedValue().GetTaggedObject());
}

/**
 * @tc.name: CreateSetIterator
 * @tc.desc: Call "CreateSetIterator" function create SetIterator,Check whether the the attribute setting of SetIterator
 *           through "GetNextIndex" and "GetIterationKind" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSSetIteratorTest, CreateSetIterator)
{
    JSHandle<JSSet> jsSet(thread, CreateJSSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);

    JSHandle<JSTaggedValue> setIteratorValue1 =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY);

    EXPECT_EQ(setIteratorValue1->IsJSSetIterator(), true);
    JSHandle<JSSetIterator> setIterator1(setIteratorValue1);
    EXPECT_EQ(JSTaggedValue::SameValue(setIterator1->GetIteratedSet(), jsSet->GetLinkedSet()), true);
    EXPECT_EQ(setIterator1->GetNextIndex(), 0U);
    EXPECT_EQ(setIterator1->GetIterationKind(), IterationKind::KEY);

    JSHandle<JSTaggedValue> setIteratorValue2 =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::VALUE);
    
    EXPECT_EQ(setIteratorValue2->IsJSSetIterator(), true);
    JSHandle<JSSetIterator> setIterator2(setIteratorValue2);
    EXPECT_EQ(JSTaggedValue::SameValue(setIterator2->GetIteratedSet(), jsSet->GetLinkedSet()), true);
    EXPECT_EQ(setIterator2->GetNextIndex(), 0U);
    EXPECT_EQ(setIterator2->GetIterationKind(), IterationKind::VALUE);
}

/**
 * @tc.name: Update
 * @tc.desc: Call "NewJSSetIterator" function create SetIterator with emty IteratedSet,create other JSSet and add key
 *           to it,the old JSSet call "Rehash" function set new JSSet to the next table, then SetIterator call "Update"
 *           function upadate IteratedSet,check whether the IteratedSet is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSSetIteratorTest, Update)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSet> jsSet1(thread, CreateJSSet(thread));
    JSHandle<JSSet> jsSet2(thread, CreateJSSet(thread));

    JSHandle<JSTaggedValue> keyHandle1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> keyHandle3(factory->NewFromASCII("key3"));
    // add key to jsSet2
    JSSet::Add(thread, jsSet2, keyHandle1);
    JSSet::Add(thread, jsSet2, keyHandle2);
    JSSet::Add(thread, jsSet2, keyHandle3);

    JSHandle<LinkedHashSet> setHandle1(thread, LinkedHashSet::Cast(jsSet1->GetLinkedSet().GetTaggedObject()));
    JSHandle<LinkedHashSet> setHandle2(thread, LinkedHashSet::Cast(jsSet2->GetLinkedSet().GetTaggedObject()));
    setHandle1->Rehash(thread, *setHandle2);
    // create SetIterator with jsSet1
    JSHandle<JSSetIterator> setIterator = factory->NewJSSetIterator(jsSet1, IterationKind::KEY);
    // update SetIterator
    setIterator->Update(thread);
    LinkedHashSet *resultSet = LinkedHashSet::Cast(setIterator->GetIteratedSet().GetTaggedObject());
    EXPECT_TRUE(resultSet->Has(keyHandle1.GetTaggedValue()));
    EXPECT_TRUE(resultSet->Has(keyHandle2.GetTaggedValue()));
    EXPECT_TRUE(resultSet->Has(keyHandle3.GetTaggedValue()));
}

/**
 * @tc.name: Next
 * @tc.desc: get the next value in setiterator,Check whether the return value obtained by the function is
 *           the next value in the array element.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSSetIteratorTest, KEY_Next)
{
    JSHandle<JSSet> jsSet(thread, CreateJSSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);

    for (int i = 0; i < 3; i++) { // 3 : 3 default numberOfElements
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSSet::Add(thread, jsSet, key);
    }
    // set IterationKind(key or value)
    JSHandle<JSTaggedValue> setIteratorValue =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY);
    JSHandle<JSSetIterator> setIterator(setIteratorValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(setIteratorValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    
    for (int i = 0; i <= 3; i++) { // 3 : 3 default numberOfElements
        JSTaggedValue result = JSSetIterator::Next(ecmaRuntimeCallInfo);
        JSHandle<JSTaggedValue> resultObj(thread, result);
        if (i < 3) {
            EXPECT_EQ(setIterator->GetNextIndex(), static_cast<uint32_t>(i+1));
            EXPECT_EQ(i, JSIterator::IteratorValue(thread, resultObj)->GetInt());
        }
        else {
            EXPECT_EQ(JSIterator::IteratorValue(thread, resultObj).GetTaggedValue(), JSTaggedValue::Undefined());
        }
    }
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(JSSetIteratorTest, KEY_AND_VALUE_Next)
{
    JSHandle<JSSet> jsSet(thread, CreateJSSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);
    JSHandle<JSTaggedValue> index0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> index1(thread, JSTaggedValue(1));

    for (int i = 0; i < 3; i++) { // default numberOfElements
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSSet::Add(thread, jsSet, key);
    }
    // set IterationKind(key and value)
    JSHandle<JSTaggedValue> setIteratorValue =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY_AND_VALUE);
    JSHandle<JSSetIterator> setIterator(setIteratorValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(setIteratorValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    
    for (int i = 0; i <= 3; i++) { // 3 : 3 default numberOfElements
        JSTaggedValue result = JSSetIterator::Next(ecmaRuntimeCallInfo);
        JSHandle<JSTaggedValue> resultObj(thread, result);
        if (i < 3) {
            JSHandle<JSArray> arrayList(thread, JSIterator::IteratorValue(thread, resultObj).GetTaggedValue());
            EXPECT_EQ(setIterator->GetNextIndex(), static_cast<uint32_t>(i+1));
            EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(arrayList), index0).GetValue()->GetInt(), i);
            EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(arrayList), index1).GetValue()->GetInt(), i);
        }
        else {
            EXPECT_EQ(JSIterator::IteratorValue(thread, resultObj).GetTaggedValue(), JSTaggedValue::Undefined());
        }
    }
    TestHelper::TearDownFrame(thread, prev);
}
}  // namespace panda::test