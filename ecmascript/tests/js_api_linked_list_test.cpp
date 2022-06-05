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
#include "ecmascript/js_api_linked_list.h"
#include "ecmascript/js_api_linked_list_iterator.h"
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
class JSAPILinkedListTest : public testing::Test {
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
    JSAPILinkedList *CreateLinkedList()
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        // 6 : 6 value is 6.
        objCallInfo->SetCallArg(0, JSTaggedValue(6));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
        JSHandle<JSTaggedValue> contianer =
            JSHandle<JSTaggedValue>(thread, ContainersPrivate::Load(objCallInfo.get()));
        JSHandle<JSAPILinkedList> linkedList =
            JSHandle<JSAPILinkedList>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(contianer),
                                                                              contianer));
        JSTaggedValue doubleList = TaggedDoubleList::Create(thread);
        linkedList->SetDoubleList(thread, doubleList);
        return *linkedList;
    }
};

HWTEST_F_L0(JSAPILinkedListTest, LinkedListCreate)
{
    JSAPILinkedList *linkedlist = CreateLinkedList();
    EXPECT_TRUE(linkedlist != nullptr);
}

HWTEST_F_L0(JSAPILinkedListTest, AddAndHas)
{
    constexpr int NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());

    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPILinkedList::Add(thread, toor, value);
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
}

HWTEST_F_L0(JSAPILinkedListTest, AddFirstAndGetFirst)
{
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSAPILinkedList> list(thread, CreateLinkedList());
    list->Add(thread, list, value);
    EXPECT_EQ(list->Length(), 1);
    EXPECT_EQ(list->Get(0).GetInt(), 1);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(2));
    list->AddFirst(thread, list, value1);
    EXPECT_EQ(list->Length(), 2);
    EXPECT_EQ(list->GetFirst().GetInt(), 2);
}

HWTEST_F_L0(JSAPILinkedListTest, InsertAndGetLast)
{    // create jsMap
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPILinkedList::Add(thread, toor, value);
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

HWTEST_F_L0(JSAPILinkedListTest, GetIndexOfAndGetLastIndexOf)
{    // create jsMap
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPILinkedList::Add(thread, toor, value);
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

HWTEST_F_L0(JSAPILinkedListTest, Remove)
{    // create jsMap
    constexpr uint32_t NODE_NUMBERS = 20;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILinkedList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(), 20);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }

    EXPECT_EQ(JSAPILinkedList::RemoveFirst(thread, toor), JSTaggedValue(0));
    value.Update(JSTaggedValue(0));
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 19);

    EXPECT_EQ(JSAPILinkedList::RemoveLast(thread, toor), JSTaggedValue(19));
    value.Update(JSTaggedValue(19));
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 18);

    value.Update(JSTaggedValue(5));
    EXPECT_EQ(JSAPILinkedList::RemoveByIndex(thread, toor, 4), value.GetTaggedValue());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 17);

    value.Update(JSTaggedValue(8));
    EXPECT_EQ(toor->Remove(thread, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 16);

    value.Update(JSTaggedValue(11));
    EXPECT_EQ(JSAPILinkedList::RemoveFirstFound(thread, toor, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 15);

    value.Update(JSTaggedValue(14));
    EXPECT_EQ(JSAPILinkedList::RemoveLastFound(thread, toor, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 14);

    toor->Dump();
}

HWTEST_F_L0(JSAPILinkedListTest, Clear)
{
    JSAPILinkedList *linkedlist = CreateLinkedList();

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSAPILinkedList> list(thread, linkedlist);
    linkedlist->Add(thread, list, value);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(2));
    linkedlist->Insert(thread, list, value1, 0);

    linkedlist->Clear(thread);

    EXPECT_EQ(linkedlist->Length(), 0);
    EXPECT_TRUE(linkedlist->GetFirst() == JSTaggedValue::Undefined());
}

HWTEST_F_L0(JSAPILinkedListTest, Set)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    EXPECT_EQ(toor->GetLast(), JSTaggedValue::Undefined());
    EXPECT_EQ(toor->GetFirst(), JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILinkedList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(), 20);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPILinkedList::Set(thread, toor, i, value);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
}

HWTEST_F_L0(JSAPILinkedListTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());

    std::string linkedListvalue("linkedListvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = linkedListvalue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPILinkedList::Add(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> linkedListKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPILinkedList::GetOwnProperty(thread, toor, linkedListKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> linkedListKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPILinkedList::GetOwnProperty(thread, toor, linkedListKey2));
}
}  // namespace panda::test
