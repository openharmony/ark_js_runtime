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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class LinkedHashTableTest : public testing::Test {
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

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    JSHandle<GlobalEnv> GetGlobalEnv()
    {
        EcmaVM *ecma = thread->GetEcmaVM();
        return ecma->GetGlobalEnv();
    }
};

HWTEST_F_L0(LinkedHashTableTest, MapCreate)
{
    int numOfElement = 64;
    LinkedHashMap *dict = LinkedHashMap::Cast(LinkedHashMap::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(dict != nullptr);
}

HWTEST_F_L0(LinkedHashTableTest, SetCreate)
{
    int numOfElement = 64;
    LinkedHashSet *set = LinkedHashSet::Cast(LinkedHashSet::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(set != nullptr);
}

HWTEST_F_L0(LinkedHashTableTest, addKeyAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // mock object needed in test
    int numOfElement = 64;
    LinkedHashMap *dict = LinkedHashMap::Cast(LinkedHashMap::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(dict != nullptr);
    JSHandle<LinkedHashMap> dictHandle(thread, dict);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv()->GetObjectFunction();

    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = factory->NewFromCanBeCompressString(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromCanBeCompressString(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    // test set()
    dict = LinkedHashMap::Cast(LinkedHashMap::Set(thread, dictHandle, key1, value1).GetTaggedObject());
    EXPECT_EQ(dict->NumberOfElements(), 1);

    // test find()
    int entry1 = dict->FindElement(key1.GetTaggedValue());
    EXPECT_EQ(key1.GetTaggedValue(), dict->GetKey(entry1));
    EXPECT_EQ(value1.GetTaggedValue(), dict->GetValue(entry1));

    dict = LinkedHashMap::Cast(LinkedHashMap::Set(thread, dictHandle, key2, value2).GetTaggedObject());
    EXPECT_EQ(dict->NumberOfElements(), 2);
    // test remove()
    dict = LinkedHashMap::Cast(LinkedHashMap::Delete(thread, dictHandle, key1).GetTaggedObject());
    EXPECT_EQ(-1, dict->FindElement(key1.GetTaggedValue()));
    EXPECT_EQ(dict->NumberOfElements(), 1);

    JSHandle<JSTaggedValue> undefinedKey(thread, JSTaggedValue::Undefined());
    dict = LinkedHashMap::Cast(LinkedHashMap::Set(thread, dictHandle, undefinedKey, value1).GetTaggedObject());
    int entry2 = dict->FindElement(undefinedKey.GetTaggedValue());
    EXPECT_EQ(value1.GetTaggedValue(), dict->GetValue(entry2));
}

HWTEST_F_L0(LinkedHashTableTest, SetaddKeyAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // mock object needed in test
    int numOfElement = 64;
    LinkedHashSet *set = LinkedHashSet::Cast(LinkedHashSet::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(set != nullptr);
    JSHandle<LinkedHashSet> setHandle(thread, set);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv()->GetObjectFunction();

    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = factory->NewFromCanBeCompressString(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromCanBeCompressString(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    // test set()
    set = LinkedHashSet::Cast(LinkedHashSet::Add(thread, setHandle, key1).GetTaggedObject());
    EXPECT_EQ(set->NumberOfElements(), 1);

    // test has()
    EXPECT_TRUE(set->Has(key1.GetTaggedValue()));

    set = LinkedHashSet::Cast(LinkedHashSet::Add(thread, setHandle, key2).GetTaggedObject());
    EXPECT_EQ(set->NumberOfElements(), 2);
    // test remove()
    set = LinkedHashSet::Cast(LinkedHashSet::Delete(thread, setHandle, key1).GetTaggedObject());
    EXPECT_EQ(-1, set->FindElement(key1.GetTaggedValue()));
    EXPECT_EQ(set->NumberOfElements(), 1);

    JSHandle<JSTaggedValue> undefinedKey(thread, JSTaggedValue::Undefined());
    set = LinkedHashSet::Cast(LinkedHashSet::Add(thread, setHandle, undefinedKey).GetTaggedObject());
    EXPECT_TRUE(set->Has(undefinedKey.GetTaggedValue()));
}

HWTEST_F_L0(LinkedHashTableTest, GrowCapacity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numOfElement = 8;
    LinkedHashMap *dict = LinkedHashMap::Cast(LinkedHashMap::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(dict != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    char keyArray[7] = "hello";
    for (int i = 0; i < 33; i++) {
        JSHandle<LinkedHashMap> dictHandle(thread, dict);
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(keyArray));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));

        // test insert()
        dict = LinkedHashMap::Cast(LinkedHashMap::Set(thread, dictHandle, key, value).GetTaggedObject());
        EXPECT_EQ(i, dict->FindElement(key.GetTaggedValue()));
    }

    JSHandle<LinkedHashMap> dictHandle(thread, dict);
    // test order
    for (int i = 0; i < 33; i++) {
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromCanBeCompressString(keyArray);
        // test insert()
        EXPECT_EQ(i, dictHandle->FindElement(stringKey.GetTaggedValue()));
    }
    EXPECT_EQ(dictHandle->NumberOfElements(), 33);
    EXPECT_EQ(dictHandle->Capacity(), 64);
}

HWTEST_F_L0(LinkedHashTableTest, SetGrowCapacity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numOfElement = 8;
    LinkedHashSet *set = LinkedHashSet::Cast(LinkedHashSet::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(set != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    // create key and values
    char keyArray[7] = "hello";
    for (int i = 0; i < 33; i++) {
        JSHandle<LinkedHashSet> setHandle(thread, set);

        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromCanBeCompressString(keyArray);
        JSHandle<JSTaggedValue> key(stringKey);

        // test insert()
        set = LinkedHashSet::Cast(LinkedHashSet::Add(thread, setHandle, key).GetTaggedObject());
        EXPECT_EQ(i, set->FindElement(key.GetTaggedValue()));
    }

    JSHandle<LinkedHashSet> setHandle(thread, set);
    // test order
    for (int i = 0; i < 33; i++) {
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromCanBeCompressString(keyArray);
        // test insert()
        EXPECT_EQ(i, setHandle->FindElement(stringKey.GetTaggedValue()));
    }
    EXPECT_EQ(setHandle->NumberOfElements(), 33);
    EXPECT_EQ(setHandle->Capacity(), 64);
}

HWTEST_F_L0(LinkedHashTableTest, ShrinkCapacity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numOfElement = 64;
    LinkedHashMap *dict = LinkedHashMap::Cast(LinkedHashMap::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(dict != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    char keyArray[7] = "hello";
    for (int i = 0; i < 10; i++) {
        JSHandle<LinkedHashMap> dictHandle(thread, dict);
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(keyArray));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));

        // test insert()
        dict = LinkedHashMap::Cast(LinkedHashMap::Set(thread, dictHandle, key, value).GetTaggedObject());
    }
    JSHandle<LinkedHashMap> dictHandle(thread, dict);
    keyArray[5] = '1' + 9;
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(keyArray));
    dict = LinkedHashMap::Cast(LinkedHashMap::Delete(thread, dictHandle, key).GetTaggedObject());
    JSHandle<LinkedHashMap> handle(thread, dict);
    // test order
    for (int i = 0; i < 9; i++) {
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromCanBeCompressString(keyArray);
        // test insert()
        EXPECT_EQ(i, handle->FindElement(stringKey.GetTaggedValue()));
    }
    EXPECT_EQ(handle->NumberOfElements(), 9);
    EXPECT_EQ(handle->Capacity(), 16);
}

HWTEST_F_L0(LinkedHashTableTest, SetShrinkCapacity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numOfElement = 64;
    LinkedHashSet *set = LinkedHashSet::Cast(LinkedHashSet::Create(thread, numOfElement).GetTaggedObject());
    EXPECT_TRUE(set != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    // create key and values
    char keyArray[7] = "hello";
    for (int i = 0; i < 10; i++) {
        JSHandle<LinkedHashSet> setHandle(thread, set);
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(keyArray));

        // test insert()
        set = LinkedHashSet::Cast(LinkedHashSet::Add(thread, setHandle, key).GetTaggedObject());
    }
    JSHandle<LinkedHashSet> setHandle(thread, set);
    keyArray[5] = '1' + 9;
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromCanBeCompressString(keyArray));
    set = LinkedHashSet::Cast(LinkedHashSet::Delete(thread, setHandle, keyHandle).GetTaggedObject());
    JSHandle<LinkedHashSet> handle(thread, set);
    // test order
    for (int i = 0; i < 9; i++) {
        keyArray[5] = '1' + i;
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromCanBeCompressString(keyArray);
        // test insert()
        EXPECT_EQ(i, handle->FindElement(stringKey.GetTaggedValue()));
    }
    EXPECT_EQ(handle->NumberOfElements(), 9);
    EXPECT_EQ(handle->Capacity(), 16);
}
}  // namespace panda::test
