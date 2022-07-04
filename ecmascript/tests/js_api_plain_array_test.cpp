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
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_plain_array_iterator.h"
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
class JSAPIPlainArrayTest : public testing::Test {
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
    JSAPIPlainArray *CreatePlainArray()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::PlainArray)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIPlainArray> plainArray(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(8)); // 8 means the value
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(8)); // 8 means the value
        plainArray->SetKeys(thread, keyArray);
        plainArray->SetValues(thread, valueArray);
        return *plainArray;
    }
};

HWTEST_F_L0(JSAPIPlainArrayTest, PlainArrayCreate)
{
    JSAPIPlainArray *plainArray = CreatePlainArray();
    EXPECT_TRUE(plainArray != nullptr);
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_AddAndGetKeyAtAndClear)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIPlainArray
    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
     
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
       
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS));

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test getKeyAt
        JSTaggedValue gvalue = array->GetKeyAt(i);
        EXPECT_EQ(gvalue, key.GetTaggedValue());
    }
    // test clear
    array->Clear(thread);
    EXPECT_EQ(array->GetSize(), 0); // 0 means the value
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_CloneAndHasAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIPlainArray
    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS));

    // test clone
    JSHandle<JSAPIPlainArray> newArray(thread, CreatePlainArray());
    EXPECT_EQ(newArray->GetSize(), 0); // 0 means the value
    newArray = JSAPIPlainArray::Clone(thread, array);
    EXPECT_EQ(newArray->GetSize(), static_cast<int>(NODE_NUMBERS));

    // test has
    key.Update(JSTaggedValue(103)); // 103 means the value
    int32_t lkey = 103;
    bool result = array->Has(lkey);
    EXPECT_TRUE(result);

    // test get
    myValue = std::string("myvalue3");
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, array->Get(key.GetTaggedValue())), value));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_GetIndexOfKeyAndGeIndexOfValueAndIsEmptyAndRemoveRangeFrom)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    EXPECT_TRUE(array->IsEmpty());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS));
    EXPECT_FALSE(array->IsEmpty());

    value.Update(JSTaggedValue(103)); // 103 means the value
    int32_t lvalue = 103;
    JSTaggedValue value2 = array->GetIndexOfKey(lvalue);
    EXPECT_EQ(value2.GetNumber(), 3); // 3 means the value

    myValue = "myvalue2";
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    JSTaggedValue value3 = array->GetIndexOfValue(value.GetTaggedValue());
    EXPECT_EQ(value3.GetNumber(), 2); // 2 means the value

    value.Update(JSTaggedValue(1));
    int32_t batchSize = 3; // 3 means the value
    lvalue = 1;
    value3 = array->RemoveRangeFrom(thread, lvalue, batchSize);
    EXPECT_EQ(value3.GetNumber(), 3); // 3 means the value
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS - 3));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_RemvoeAnrRemvoeAtAndSetValueAtAndGetValueAt)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    EXPECT_TRUE(array->IsEmpty());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS));
    EXPECT_FALSE(array->IsEmpty());

    // test Remove
    myValue = "myvalue2";
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    JSTaggedValue taggedValue =
        array->Remove(thread, JSTaggedValue(102)); // 102 means the value
    EXPECT_TRUE(JSTaggedValue::Equal(thread, value, JSHandle<JSTaggedValue>(thread, taggedValue)));

    // test RemoveAt
    myValue = "myvalue4";
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    taggedValue =
        array->RemoveAt(thread, JSTaggedValue(3)); // 3 means the value
    EXPECT_TRUE(JSTaggedValue::Equal(thread, value, JSHandle<JSTaggedValue>(thread, taggedValue)));
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS - 2));

    // test SetValueAt
    myValue = "myvalue14";
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    array->SetValueAt(thread, JSTaggedValue(3), value.GetTaggedValue()); // 3 means the value
    int32_t lvalue = 3; // 3 means the value
    taggedValue = array->GetValueAt(lvalue);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, value, JSHandle<JSTaggedValue>(thread, taggedValue)));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIPlainArray> toor(thread, CreatePlainArray());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    std::string plainArrayvalue("plainArrayvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = plainArrayvalue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, toor, key, value);
    }
    // test GetOwnProperty
    int testInt = 100 + 1;
    JSHandle<JSTaggedValue> plainArrayKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIPlainArray::GetOwnProperty(thread, toor, plainArrayKey1));
    testInt = 100 + 20;
    JSHandle<JSTaggedValue> plainArrayKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIPlainArray::GetOwnProperty(thread, toor, plainArrayKey2));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_ToString)
{
    constexpr uint32_t NODE_NUMBERS = 3; // 3 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIPlainArray
    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    JSTaggedValue result1 = JSAPIPlainArray::ToString(thread, array);
    JSHandle<EcmaString> resultHandle1(thread, result1);
    [[maybe_unused]] auto *res1 = EcmaString::Cast(resultHandle1.GetTaggedValue().GetTaggedObject());
    JSHandle<EcmaString> det = thread->GetEcmaVM()->GetFactory()->NewFromASCII("");
    ASSERT_EQ(res1->Compare(*det), 0);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = i;
        std::string ivalue = std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0:0,1:1,2:2");
    JSTaggedValue result = JSAPIPlainArray::ToString(thread, array);
    JSHandle<EcmaString> resultHandle(thread, result);
    [[maybe_unused]] auto *res = EcmaString::Cast(resultHandle.GetTaggedValue().GetTaggedObject());

    ASSERT_EQ(res->Compare(*str), 0);
}
}  // namespace panda::test
