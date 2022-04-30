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

#include <memory>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class NameDictionaryTest : public testing::Test {
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
};

static JSHandle<GlobalEnv> GetGlobalEnv(JSThread *thread)
{
    EcmaVM *ecma = thread->GetEcmaVM();
    return ecma->GetGlobalEnv();
}

HWTEST_F_L0(NameDictionaryTest, createDictionary)
{
    int numOfElement = 64;
    JSHandle<NameDictionary> dict = NameDictionary::Create(thread, numOfElement);
    EXPECT_TRUE(*dict != nullptr);
}

HWTEST_F_L0(NameDictionaryTest, addKeyAndValue)
{
    // mock object needed in test
    int numOfElement = 64;
    JSHandle<NameDictionary> dictJShandle(NameDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictJShandle != nullptr);
    JSMutableHandle<NameDictionary> dictHandle(dictJShandle);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv(thread)->GetObjectFunction();

    // create key and values
    JSHandle<JSObject> jsObject =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    EXPECT_TRUE(*jsObject != nullptr);

    char keyArray[] = "hello";
    JSHandle<EcmaString> stringKey1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> taggedkey1(stringKey1);
    JSHandle<JSTaggedValue> value1(
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    PropertyAttributes metaData1;

    char key2Array[] = "hello2";
    JSHandle<EcmaString> stringKey2 = thread->GetEcmaVM()->GetFactory()->NewFromASCII(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> taggedkey2(stringKey2);
    JSHandle<JSTaggedValue> value2(
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    PropertyAttributes metaData2;

    // test insert()
    JSHandle<NameDictionary> dict(NameDictionary::PutIfAbsent(thread, dictHandle, key1, value1, metaData1));
    dictHandle.Update(dict.GetTaggedValue());
    EXPECT_EQ(dict->EntriesCount(), 1);

    // test find() and lookup()
    int entry1 = dict->FindEntry(key1.GetTaggedValue());
    EXPECT_EQ(key1.GetTaggedValue(), JSTaggedValue(dict->GetKey(entry1).GetRawData()));
    EXPECT_EQ(value1.GetTaggedValue(), JSTaggedValue(dict->GetValue(entry1).GetRawData()));

    JSHandle<NameDictionary> dict2(NameDictionary::PutIfAbsent(thread, dictHandle, key2, value2, metaData2));
    EXPECT_EQ(dict2->EntriesCount(), 2);
    // test remove()
    dict = NameDictionary::Remove(thread, dictHandle, entry1);
    EXPECT_EQ(-1, dict->FindEntry(key1.GetTaggedValue()));
    EXPECT_EQ(dict->EntriesCount(), 1);
}

HWTEST_F_L0(NameDictionaryTest, GrowCapacity)
{
    int numOfElement = 8;
    JSHandle<NameDictionary> dictHandle(NameDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv(thread)->GetObjectFunction();
    // create key and values
    JSHandle<JSObject> jsObject =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    EXPECT_TRUE(*jsObject != nullptr);
    char keyArray[7] = "hello";
    for (int i = 0; i < 9; i++) {
        JSHandle<NameDictionary> tempHandle = dictHandle;
        keyArray[5] = '1' + static_cast<uint32_t>(i);
        keyArray[6] = 0;

        JSHandle<EcmaString> stringKey = thread->GetEcmaVM()->GetFactory()->NewFromASCII(keyArray);
        ecmascript::JSHandle<JSTaggedValue> key(stringKey);
        JSHandle<JSTaggedValue> keyHandle(key);
        ecmascript::JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> valueHandle(value);
        PropertyAttributes metaData;

        // test insert()
        dictHandle = NameDictionary::PutIfAbsent(thread, tempHandle, keyHandle, valueHandle, metaData);
    }
    EXPECT_EQ(dictHandle->EntriesCount(), 9);
    EXPECT_EQ(dictHandle->Size(), 16);
}

HWTEST_F_L0(NameDictionaryTest, ShrinkCapacity)
{
    int numOfElement = 64;
    JSMutableHandle<NameDictionary> dictHandle(NameDictionary::Create(thread, numOfElement));
    EXPECT_TRUE(*dictHandle != nullptr);
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv(thread)->GetObjectFunction();
    // create key and values
    JSHandle<JSObject> jsObject =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    EXPECT_TRUE(*jsObject != nullptr);
    uint8_t keyArray[7] = "hello";

    auto stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
    for (int i = 0; i < 10; i++) {
        keyArray[5] = '0' + static_cast<uint32_t>(i);
        keyArray[6] = 0;

        JSHandle<JSTaggedValue> key(thread, stringTable->GetOrInternString(keyArray, utf::Mutf8Size(keyArray), true));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        PropertyAttributes metaData;

        // test insert()
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dictHandle, key, value, metaData);
        dictHandle.Update(newDict.GetTaggedValue());
    }

    keyArray[5] = '2';
    keyArray[6] = 0;
    JSHandle<JSTaggedValue> arrayHandle(thread,
                                        stringTable->GetOrInternString(keyArray, utf::Mutf8Size(keyArray), true));

    int entry = dictHandle->FindEntry(arrayHandle.GetTaggedValue());
    EXPECT_NE(entry, -1);

    JSHandle<NameDictionary> newDict1 = NameDictionary::Remove(thread, dictHandle, entry);
    dictHandle.Update(newDict1.GetTaggedValue());
    EXPECT_EQ(dictHandle->EntriesCount(), 9);
    EXPECT_EQ(dictHandle->Size(), 16);
}
}  // namespace panda::test
