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
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_list.h"
#include "ecmascript/js_api_list_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPIListTest : public testing::Test {
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
    JSAPIList *CreateList()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::List)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIList> list(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue singleList = TaggedSingleList::Create(thread);
        list->SetSingleList(thread, singleList);
        return *list;
    }
};

HWTEST_F_L0(JSAPIListTest, listCreate)
{
    JSAPIList *list = CreateList();
    EXPECT_TRUE(list != nullptr);
}

HWTEST_F_L0(JSAPIListTest, AddAndHas)
{
    constexpr int NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIList> toor(thread, CreateList());

    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(), NODE_NUMBERS);

    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
    JSTaggedValue gValue = toor->Get(10);
    EXPECT_EQ(gValue, JSTaggedValue::Undefined());

    std::string ivalue = myValue + std::to_string(1);
    value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
    EXPECT_TRUE(toor->Has(value.GetTaggedValue()));

    toor->Dump();
}

HWTEST_F_L0(JSAPIListTest, InsertAndGetLastAndGetFirst)
{    // create jsMap
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIList> toor(thread, CreateList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPIList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->GetLast().GetInt(), 9);
    EXPECT_EQ(toor->GetFirst().GetInt(), 1);

    value.Update(JSTaggedValue(99));
    int len = toor->Length();
    toor->Insert(thread, toor, value, len);
    EXPECT_EQ(toor->GetLast().GetInt(), 99);
    EXPECT_EQ(toor->Length(), 10);

    value.Update(JSTaggedValue(100));
    toor->Insert(thread, toor, value, 0);
    EXPECT_EQ(toor->GetFirst().GetInt(), 100);
    EXPECT_EQ(toor->Length(), 11);

    toor->Dump();

    value.Update(JSTaggedValue(101));
    toor->Insert(thread, toor, value, 5);
    EXPECT_EQ(toor->Length(), 12);
    toor->Dump();
    EXPECT_EQ(toor->Get(5).GetInt(), 101);
}

HWTEST_F_L0(JSAPIListTest, GetIndexOfAndGetLastIndexOf)
{    // create jsMap
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIList> toor(thread, CreateList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPIList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->GetLast().GetInt(), 9);
    EXPECT_EQ(toor->GetFirst().GetInt(), 1);

    value.Update(JSTaggedValue(99));
    int len = toor->Length();
    toor->Insert(thread, toor, value, len);
    EXPECT_EQ(toor->GetIndexOf(value.GetTaggedValue()).GetInt(), 9);
    EXPECT_EQ(toor->GetLastIndexOf(value.GetTaggedValue()).GetInt(), 9);
    EXPECT_EQ(toor->Length(), 10);

    value.Update(JSTaggedValue(100));
    toor->Insert(thread, toor, value, 0);
    EXPECT_EQ(toor->GetIndexOf(value.GetTaggedValue()).GetInt(), 0);
    EXPECT_EQ(toor->GetLastIndexOf(value.GetTaggedValue()).GetInt(), 0);
    EXPECT_EQ(toor->Length(), 11);

    value.Update(JSTaggedValue(101));
    toor->Insert(thread, toor, value, 5);
    EXPECT_EQ(toor->GetIndexOf(value.GetTaggedValue()).GetInt(), 5);
    EXPECT_EQ(toor->GetLastIndexOf(value.GetTaggedValue()).GetInt(), 5);
    EXPECT_EQ(toor->Length(), 12);

    toor->Dump();
}

HWTEST_F_L0(JSAPIListTest, Remove)
{    // create jsMap
    constexpr uint32_t NODE_NUMBERS = 20;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIList> toor(thread, CreateList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPIList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(), 20);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }

    value.Update(JSTaggedValue(4));
    EXPECT_EQ(JSAPIList::RemoveByIndex(thread, toor, 4), value.GetTaggedValue());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 19);

    value.Update(JSTaggedValue(8));
    EXPECT_EQ(toor->Remove(thread, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 18);

    toor->Dump();
}

HWTEST_F_L0(JSAPIListTest, Clear)
{
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSAPIList> list(thread, CreateList());
    list->Add(thread, list, value);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(2));
    list->Insert(thread, list, value1, 0);

    list->Clear(thread);

    EXPECT_EQ(list->Length(), 0);
    EXPECT_TRUE(list->GetFirst() == JSTaggedValue::Undefined());
}

HWTEST_F_L0(JSAPIListTest, Set)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIList> toor(thread, CreateList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPIList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(), 20);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPIList::Set(thread, toor, i, value);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
}

HWTEST_F_L0(JSAPIListTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIList> toor(thread, CreateList());

    std::string listvalue("listvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = listvalue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIList::Add(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> listKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIList::GetOwnProperty(thread, toor, listKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> listKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIList::GetOwnProperty(thread, toor, listKey2));
}
}  // namespace panda::test
