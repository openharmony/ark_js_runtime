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
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table.h"
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

    EcmaVM *instance {nullptr};
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
    JSHandle<LinkedHashMap> dict = LinkedHashMap::Create(thread, numOfElement);
    EXPECT_TRUE(*dict != nullptr);
}

HWTEST_F_L0(LinkedHashTableTest, SetCreate)
{
    int numOfElement = 64;
    JSHandle<LinkedHashSet> set = LinkedHashSet::Create(thread, numOfElement);
    EXPECT_TRUE(*set != nullptr);
}

HWTEST_F_L0(LinkedHashTableTest, addKeyAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // mock object needed in test
    int numOfElement = 64;
    JSHandle<LinkedHashMap> dictHandle = LinkedHashMap::Create(thread, numOfElement);
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv()->GetObjectFunction();

    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = factory->NewFromASCII(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromASCII(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    // test set()
    dictHandle = LinkedHashMap::Set(thread, dictHandle, key1, value1);
    EXPECT_EQ(dictHandle->NumberOfElements(), 1);

    // test find()
    int entry1 = dictHandle->FindElement(key1.GetTaggedValue());
    EXPECT_EQ(key1.GetTaggedValue(), dictHandle->GetKey(entry1));
    EXPECT_EQ(value1.GetTaggedValue(), dictHandle->GetValue(entry1));

    dictHandle = LinkedHashMap::Set(thread, dictHandle, key2, value2);
    EXPECT_EQ(dictHandle->NumberOfElements(), 2);
    // test remove()
    dictHandle = LinkedHashMap::Delete(thread, dictHandle, key1);
    EXPECT_EQ(-1, dictHandle->FindElement(key1.GetTaggedValue()));
    EXPECT_EQ(dictHandle->NumberOfElements(), 1);

    JSHandle<JSTaggedValue> undefinedKey(thread, JSTaggedValue::Undefined());
    dictHandle = LinkedHashMap::Set(thread, dictHandle, undefinedKey, value1);
    int entry2 = dictHandle->FindElement(undefinedKey.GetTaggedValue());
    EXPECT_EQ(value1.GetTaggedValue(), dictHandle->GetValue(entry2));
}

HWTEST_F_L0(LinkedHashTableTest, SetaddKeyAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // mock object needed in test
    int numOfElement = 64;
    JSHandle<LinkedHashSet> setHandle = LinkedHashSet::Create(thread, numOfElement);
    EXPECT_TRUE(*setHandle != nullptr);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv()->GetObjectFunction();

    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = factory->NewFromASCII(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromASCII(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    // test set()
    setHandle = LinkedHashSet::Add(thread, setHandle, key1);
    EXPECT_EQ(setHandle->NumberOfElements(), 1);

    // test has()
    EXPECT_TRUE(setHandle->Has(key1.GetTaggedValue()));

    setHandle = LinkedHashSet::Add(thread, setHandle, key2);
    EXPECT_EQ(setHandle->NumberOfElements(), 2);
    // test remove()
    setHandle = LinkedHashSet::Delete(thread, setHandle, key1);
    EXPECT_EQ(-1, setHandle->FindElement(key1.GetTaggedValue()));
    EXPECT_EQ(setHandle->NumberOfElements(), 1);

    JSHandle<JSTaggedValue> undefinedKey(thread, JSTaggedValue::Undefined());
    setHandle = LinkedHashSet::Add(thread, setHandle, undefinedKey);
    EXPECT_TRUE(setHandle->Has(undefinedKey.GetTaggedValue()));
}

HWTEST_F_L0(LinkedHashTableTest, GrowCapacity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numOfElement = 8;
    JSHandle<LinkedHashMap> dictHandle = LinkedHashMap::Create(thread, numOfElement);
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    char keyArray[7] = "hello";
    for (int i = 0; i < 33; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));

        // test insert()
        dictHandle = LinkedHashMap::Set(thread, dictHandle, key, value);
        EXPECT_EQ(i, dictHandle->FindElement(key.GetTaggedValue()));
    }

    // test order
    for (int i = 0; i < 33; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromASCII(keyArray);
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
    JSHandle<LinkedHashSet> setHandle = LinkedHashSet::Create(thread, numOfElement);
    EXPECT_TRUE(*setHandle != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    // create key and values
    char keyArray[7] = "hello";
    for (int i = 0; i < 33; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromASCII(keyArray);
        JSHandle<JSTaggedValue> key(stringKey);

        // test insert()
        setHandle = LinkedHashSet::Add(thread, setHandle, key);
        EXPECT_EQ(i, setHandle->FindElement(key.GetTaggedValue()));
    }

    // test order
    for (int i = 0; i < 33; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromASCII(keyArray);
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
    JSHandle<LinkedHashMap> dictHandle = LinkedHashMap::Create(thread, numOfElement);
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    char keyArray[7] = "hello";
    for (int i = 0; i < 10; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));

        // test insert()
        dictHandle = LinkedHashMap::Set(thread, dictHandle, key, value);
    }
    keyArray[5] = '1' + 9;
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
    dictHandle = LinkedHashMap::Delete(thread, dictHandle, key);
    // test order
    for (int i = 0; i < 9; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromASCII(keyArray);
        // test insert()
        EXPECT_EQ(i, dictHandle->FindElement(stringKey.GetTaggedValue()));
    }
    EXPECT_EQ(dictHandle->NumberOfElements(), 9);
    EXPECT_EQ(dictHandle->Capacity(), 16);
}

HWTEST_F_L0(LinkedHashTableTest, SetShrinkCapacity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numOfElement = 64;
    JSHandle<LinkedHashSet> setHandle = LinkedHashSet::Create(thread, numOfElement);
    EXPECT_TRUE(*setHandle != nullptr);
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    // create key and values
    char keyArray[7] = "hello";
    for (int i = 0; i < 10; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));

        // test insert()
        setHandle = LinkedHashSet::Add(thread, setHandle, key);
    }
    keyArray[5] = '1' + 9;
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromASCII(keyArray));
    setHandle = LinkedHashSet::Delete(thread, setHandle, keyHandle);
    // test order
    for (int i = 0; i < 9; i++) {
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;
        JSHandle<EcmaString> stringKey = factory->NewFromASCII(keyArray);
        // test insert()
        EXPECT_EQ(i, setHandle->FindElement(stringKey.GetTaggedValue()));
    }
    EXPECT_EQ(setHandle->NumberOfElements(), 9);
    EXPECT_EQ(setHandle->Capacity(), 16);
}
}  // namespace panda::test
