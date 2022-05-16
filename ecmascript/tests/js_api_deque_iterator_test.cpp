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

#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/js_api_deque.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIDequeIteratorTest : public testing::Test {
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
    static JSHandle<JSAPIDeque> CreateJSApiDeque(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> proto = env->GetFunctionPrototype();
        JSHandle<JSHClass> dequeClass = factory->NewEcmaDynClass(JSAPIDeque::SIZE, JSType::JS_API_DEQUE, proto);
        JSHandle<JSAPIDeque> jsDeque = JSHandle<JSAPIDeque>::Cast(factory->NewJSObject(dequeClass));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
        jsDeque->SetFirst(0);
        jsDeque->SetLast(0);
        jsDeque->SetElements(thread, newElements);
        return jsDeque;
    }
};

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPIDeque,and then loop through the elements of the iterator to check whether
 *           the elements are consistent through Next function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIDequeIteratorTest, Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIDeque> jsDeque = CreateJSApiDeque(thread);
    EXPECT_TRUE(*jsDeque != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    // insert value
    std::string dequeValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = dequeValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, jsDeque, value);
    }
    JSHandle<JSAPIDequeIterator> dequeIterator = factory->NewJSAPIDequeIterator(jsDeque);
    for (uint32_t i = 0; i <= DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(dequeIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = JSAPIDequeIterator::Next(ecmaRuntimeCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        if (i <= DEFAULT_LENGTH - 1U) {
            EXPECT_EQ(dequeIterator->GetNextIndex(), i);
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsString());
        }
        else {
            EXPECT_TRUE(dequeIterator->GetIteratedDeque().IsUndefined());
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsUndefined());
        }
    }
}

/**
 * @tc.name: SetIteratedQueue
 * @tc.desc: Call the "SetIteratedQueue" function, check whether the result returned through "GetIteratedQueue"
 *           function from the JSAPIDequeIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIDequeIteratorTest, SetIteratedDeque)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIDeque> jsDeque1 = CreateJSApiDeque(thread);
    JSHandle<JSAPIDeque> jsDeque2 = CreateJSApiDeque(thread);
    EXPECT_TRUE(*jsDeque1 != nullptr);
    EXPECT_TRUE(*jsDeque2 != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // insert value
    std::string dequeValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = dequeValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, jsDeque1, value);
    }

    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = dequeValue + std::to_string(i + 1U);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, jsDeque2, value);
    }
    JSHandle<JSAPIDequeIterator> dequeIterator = factory->NewJSAPIDequeIterator(jsDeque1);
    EXPECT_EQ(dequeIterator->GetIteratedDeque(), jsDeque1.GetTaggedValue());

    dequeIterator->SetIteratedDeque(thread, jsDeque2.GetTaggedValue());
    EXPECT_EQ(dequeIterator->GetIteratedDeque(), jsDeque2.GetTaggedValue());
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPIDequeIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIDequeIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIDeque> jsDeque = CreateJSApiDeque(thread);
    EXPECT_TRUE(*jsDeque != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // insert value
    std::string dequeValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = dequeValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, jsDeque, value);
    }
    JSHandle<JSAPIDequeIterator> dequeIterator = factory->NewJSAPIDequeIterator(jsDeque);
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        dequeIterator->SetNextIndex(i);
        EXPECT_EQ(dequeIterator->GetNextIndex(), i);
    }
}
}  // namespace panda::test