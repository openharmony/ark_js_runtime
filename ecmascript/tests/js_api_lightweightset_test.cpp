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
 
#include "ecmascript/js_api_lightweightset.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_lightweightset_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPILightWeightSetTest : public testing::Test {
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
    JSAPILightWeightSet *CreateLightWeightSet()
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 3 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightSet)));

        auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSHandle<JSTaggedValue> constructor =
            JSHandle<JSTaggedValue>(thread, containers::ContainersPrivate::Load(objCallInfo));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPILightWeightSet> lightweightSet =
            JSHandle<JSAPILightWeightSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor),
                                                                                  constructor));
        JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(8)); // 8 means the value
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(8)); // 8 means the value
        lightweightSet->SetHashes(thread, hashArray);
        lightweightSet->SetValues(thread, valueArray);
        lightweightSet->SetLength(0); // 0 means the value
        return *lightweightSet;
    }
};

HWTEST_F_L0(JSAPILightWeightSetTest, LightWeightSetCreate)
{
    JSAPILightWeightSet *lightweightSet = CreateLightWeightSet();
    EXPECT_TRUE(lightweightSet != nullptr);
}

HWTEST_F_L0(JSAPILightWeightSetTest, AddIncreaseCapacityAddAll)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> srcLws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> destLws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test IncreaseCapacityTo
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        bool result = JSAPILightWeightSet::Add(thread, lws, value);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);
    
    uint32_t tmp = NODE_NUMBERS * 3; // 3 means the value
    JSAPILightWeightSet::IncreaseCapacityTo(thread, lws, static_cast<int32_t>(tmp));
    uint32_t capacity = TaggedArray::Cast(lws->GetValues().GetTaggedObject())->GetLength();
    EXPECT_EQ(JSTaggedValue(capacity), JSTaggedValue(tmp));

    // test AddAll
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        bool result = JSAPILightWeightSet::Add(thread, srcLws, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        EXPECT_TRUE(result);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS + 2; i++) {
        JSAPILightWeightSet::Add(thread, destLws, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
    }
    bool result = JSAPILightWeightSet::AddAll(thread, destLws, JSHandle<JSTaggedValue>::Cast(srcLws));
    EXPECT_TRUE(result);
    tmp = NODE_NUMBERS * 2 + 2; // 2 means the value
    EXPECT_EQ(destLws->GetSize(), tmp);
}

HWTEST_F_L0(JSAPILightWeightSetTest, EqualClearNotEqual)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> equalLws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    bool result = false;
    // test equal
    std::string myValue1("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue1 + std::to_string(i);
        value1.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, lws, value1);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);

    std::string myValue2("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue2 + std::to_string(i);
        value2.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, equalLws, value2);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(equalLws->GetSize(), NODE_NUMBERS);
    result = JSAPILightWeightSet::Equal(thread, lws, JSHandle<JSTaggedValue>::Cast(equalLws));
    EXPECT_TRUE(result);

    equalLws->Clear(thread);
    EXPECT_EQ(equalLws->GetSize(), static_cast<uint32_t>(0)); // 0 means the value

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue2 + std::to_string(i);
        if (i == 2) {
            LOG(ERROR, RUNTIME) << " {} " << iValue;
        } else {
            value2.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            result = JSAPILightWeightSet::Add(thread, equalLws, value2);
            EXPECT_TRUE(result);
        }
    }
    EXPECT_EQ(equalLws->GetSize(), NODE_NUMBERS - 1);
    result = JSAPILightWeightSet::Equal(thread, lws, JSHandle<JSTaggedValue>::Cast(equalLws));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(JSAPILightWeightSetTest, IsEmptyHasHasAll)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> hasAllLws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value3(thread, JSTaggedValue::Undefined());
    bool result = false;
    std::string tValue;
    // test IsEmpty
    result = lws->IsEmpty();
    EXPECT_TRUE(result);
    // test Has
    std::string myValue1("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue1 + std::to_string(i);
        value1.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, lws, value1);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);

    tValue = myValue1 + std::to_string(5);
    value1.Update(factory->NewFromStdString(tValue).GetTaggedValue());
    result = lws->Has(value1);
    EXPECT_TRUE(result);

    std::string myValue2("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS - 5; i++) {
        if (i == 1) {
            std::string myValue3("destValue");
            std::string iValue3 = myValue3 + std::to_string(i);
            value3.Update(factory->NewFromStdString(iValue3).GetTaggedValue());
            result = JSAPILightWeightSet::Add(thread, hasAllLws, value3);
        } else {
            std::string iValue = myValue2 + std::to_string(i);
            value2.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            result = JSAPILightWeightSet::Add(thread, hasAllLws, value2);
            EXPECT_TRUE(result);
        }
    }
    EXPECT_EQ(hasAllLws->GetSize(), NODE_NUMBERS - 5); // 5 means the value
    result = lws->HasAll(JSHandle<JSTaggedValue>::Cast(hasAllLws));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(JSAPILightWeightSetTest, GetIndexOfRemoveRemoveAtGetValueAt)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    bool result = false;

    // test GetSize
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, lws, value);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);
    
    // test GetIndexOf
    std::string tValue("myvalue5");
    value.Update(factory->NewFromStdString(tValue).GetTaggedValue());
    int32_t index = lws->GetIndexOf(value);
    EXPECT_EQ(index, 5); // 5 means the value

    // test GetValueAt
    JSTaggedValue jsValue = lws->GetValueAt(5); // 5 means the value
    EXPECT_EQ(value.GetTaggedValue(), jsValue);

    // test Remove
    jsValue = lws->Remove(thread, value);
    EXPECT_EQ(value.GetTaggedValue(), jsValue);
    
    // test RemoveAt
    result = lws->RemoveAt(thread, 4); // 4 means the value
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS - 2); // 2 means the value
    EXPECT_TRUE(result);
}

HWTEST_F_L0(JSAPILightWeightSetTest, Iterator)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lwm(thread, CreateLightWeightSet());
    
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILightWeightSet::Add(thread, lwm, value);
    }

    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPILightWeightSetIterator(lwm, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        int v = JSIterator::IteratorValue(thread, valueIterResult)->GetInt();
        JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(v));
    }
}
}  // namespace panda::test