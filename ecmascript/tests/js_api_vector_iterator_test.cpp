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

#include "ecmascript/js_api_vector_iterator.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_vector.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIVectorIteratorTest : public testing::Test {
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
    JSHandle<JSAPIVector> CreateVector()
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
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::Vector)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = containers::ContainersPrivate::Load(ecmaRuntimeCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIVector> jsVector(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        jsVector->SetLength(0);
        return jsVector;
    }
};

/**
 * @tc.name: SetIteratedSet
 * @tc.desc: Call the "SetIteratedSet" function, check whether the result returned through "GetIteratedSet"
 *           function from the JSAPIVectorIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorIteratorTest, SetIteratedVector)
{
    constexpr uint32_t DEFAULT_LENGTH = 10;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIVector> jsVector = CreateVector();
    EXPECT_TRUE(*jsVector != nullptr);
    JSHandle<JSAPIVectorIterator> vectorIterator = factory->NewJSAPIVectorIterator(jsVector);

    std::string vectorValue("vectorvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = vectorValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIVector::Add(thread, jsVector, value);
    }
    vectorIterator->SetIteratedVector(thread, jsVector.GetTaggedValue());
    JSHandle<JSAPIVector> VectorTo(thread, JSAPIVector::Cast(vectorIterator->GetIteratedVector().GetTaggedObject()));
    EXPECT_EQ(VectorTo->GetSize(), static_cast<int>(DEFAULT_LENGTH));
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = vectorValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        EXPECT_EQ(JSAPIVector::Get(thread, jsVector, i), value.GetTaggedValue());
    }
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPIVectorIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 10;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIVector> jsVector = CreateVector();
    EXPECT_TRUE(*jsVector != nullptr);
    JSHandle<JSAPIVectorIterator> vectorIterator = factory->NewJSAPIVectorIterator(jsVector);
    EXPECT_EQ(vectorIterator->GetNextIndex(), 0U);
    
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        vectorIterator->SetNextIndex(i);
        EXPECT_EQ(vectorIterator->GetNextIndex(), i);
    }
}

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPIVector,and then loop through the elements of the iterator to check
 *           whether the elements through "Next" function are consistent.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorIteratorTest, Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 10;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPIVector> jsVector = CreateVector();
    std::string vectorValue("vectorvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = vectorValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIVector::Add(thread, jsVector, value);
    }
    JSHandle<JSAPIVectorIterator> vectorIterator = factory->NewJSAPIVectorIterator(jsVector);
    // traversal iterator
    for (uint32_t i = 0; i <= DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(vectorIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = JSAPIVectorIterator::Next(ecmaRuntimeCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultValue = vectorValue + std::to_string(i);
        if (i <= DEFAULT_LENGTH - 1U) {
            value.Update(factory->NewFromStdString(resultValue).GetTaggedValue());
            EXPECT_EQ(JSTaggedValue::SameValue(
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), value), true);
        }
        else {
            EXPECT_TRUE(vectorIterator->GetIteratedVector().IsUndefined());
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsUndefined());
        }
    }
}
}  // namespace panda::ecmascript