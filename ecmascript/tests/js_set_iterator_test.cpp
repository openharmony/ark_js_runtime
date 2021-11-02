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

#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/global_env.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

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
    ecmascript::EcmaHandleScope *scope {nullptr};
    PandaVM *instance {nullptr};
    JSThread *thread {nullptr};
};

JSSet *CreateSet(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    LinkedHashSet *hashSet = LinkedHashSet::Cast(LinkedHashSet::Create(thread).GetTaggedObject());
    set->SetLinkedSet(thread, JSTaggedValue(hashSet));
    return JSSet::Cast(set.GetTaggedValue().GetTaggedObject());
}

/*
 * Feature: JSSetIterator
 * Function: CreateSetIterator
 * SubFunction: GetIterationKind,GetNextIndex
 * FunctionPoints: Create SetIterator
 * CaseDescription: Check whether the returned value through "CreateSetIterator" function is within expectations.
 */
HWTEST_F_L0(JSSetIteratorTest, CreateSetIterator)
{
    JSHandle<JSSet> jsSet(thread, CreateSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);

    JSHandle<JSTaggedValue> setIteratorValue1 =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY);

    EXPECT_EQ(setIteratorValue1->IsJSSetIterator(), true);
    JSHandle<JSSetIterator> setIterator1(setIteratorValue1);
    EXPECT_EQ(JSTaggedValue::SameValue(setIterator1->GetIteratedSet(), jsSet->GetLinkedSet()), true);
    EXPECT_EQ(setIterator1->GetNextIndex().GetInt(), 0);

    JSTaggedValue iterationKind1 = setIterator1->GetIterationKind();
    EXPECT_EQ(JSTaggedValue::SameValue(iterationKind1, JSTaggedValue(static_cast<int>(IterationKind::KEY))), true);

    JSHandle<JSTaggedValue> setIteratorValue2 =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::VALUE);
    
    EXPECT_EQ(setIteratorValue2->IsJSSetIterator(), true);
    JSHandle<JSSetIterator> setIterator2(setIteratorValue2);
    EXPECT_EQ(JSTaggedValue::SameValue(setIterator2->GetIteratedSet(), jsSet->GetLinkedSet()), true);
    EXPECT_EQ(setIterator2->GetNextIndex().GetInt(), 0);

    JSTaggedValue iterationKind2 = setIterator2->GetIterationKind();
    EXPECT_EQ(JSTaggedValue::SameValue(iterationKind2, JSTaggedValue(static_cast<int>(IterationKind::VALUE))), true);
}

/*
 * Feature: JSSetIterator
 * Function: next
 * SubFunction: IteratorValue
 * FunctionPoints: get the next value in setiterator
 * CaseDescription: Check whether the return value obtained by the function is the next value in the array element.
 */
HWTEST_F_L0(JSSetIteratorTest, Next)
{
    JSHandle<JSSet> jsSet(thread, CreateSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);

    for (int i = 0; i < 3; i++) {
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
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    
    JSTaggedValue result1 = JSSetIterator::Next(ecmaRuntimeCallInfo.get());
    EXPECT_EQ(setIterator->GetNextIndex().GetInt(), 1);
    JSHandle<JSTaggedValue> resultObj1(thread, result1);
    EXPECT_EQ(0, JSIterator::IteratorValue(thread, resultObj1)->GetInt());

    JSTaggedValue result2 = JSSetIterator::Next(ecmaRuntimeCallInfo.get());
    EXPECT_EQ(setIterator->GetNextIndex().GetInt(), 2);
    JSHandle<JSTaggedValue> resultObj2(thread, result2);
    EXPECT_EQ(1, JSIterator::IteratorValue(thread, resultObj2)->GetInt());

    JSTaggedValue result3 = JSSetIterator::Next(ecmaRuntimeCallInfo.get());
    EXPECT_EQ(setIterator->GetNextIndex().GetInt(), 3);
    JSHandle<JSTaggedValue> resultObj3(thread, result3);
    EXPECT_EQ(2, JSIterator::IteratorValue(thread, resultObj3)->GetInt());
    
    JSTaggedValue result4 = JSSetIterator::Next(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> resultObj4(thread, result4);
    EXPECT_EQ(JSIterator::IteratorValue(thread, resultObj4).GetTaggedValue(), JSTaggedValue::Undefined());

    TestHelper::TearDownFrame(thread, prev);
}
}  // namespace panda::test