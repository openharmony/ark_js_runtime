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

#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class GlobalDictionaryTest : public testing::Test {
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

/**
 * @tc.name: IsMatch
 * @tc.desc: Check whether two JSTaggedValue is equal through calling IsMatch function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, IsMatch)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> globalKey = factory->NewFromASCII("key");
    JSTaggedValue globalOtherKey = globalKey.GetTaggedValue();

    EXPECT_EQ(GlobalDictionary::IsMatch(globalKey.GetTaggedValue(), globalOtherKey), true);
    EXPECT_EQ(GlobalDictionary::IsMatch(globalKey.GetTaggedValue(), JSTaggedValue::Undefined()), false);
}

/**
 * @tc.name: Hash
 * @tc.desc: Check whether the hash size through calling Hash function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, Hash)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // test obj is jsSymbol
    JSHandle<JSSymbol> jsSymbol = factory->NewJSSymbol();
    uint32_t hashField = static_cast<uint32_t>(GlobalDictionary::Hash(jsSymbol.GetTaggedValue()));
    EXPECT_EQ(hashField, SymbolTable::Hash(jsSymbol.GetTaggedValue()));

    // the CompressedStringsEnabled must be true
    bool flag = EcmaString::GetCompressedStringsEnabled();
    EXPECT_EQ(flag, true);
    // test obj is string(uint8_t)
    uint8_t utf8ArrayName[4] = {0, 2, 5}; // The last element is "\0"
    uint32_t utf8ArrayNameLen = sizeof(utf8ArrayName) - 1;
    JSHandle<EcmaString> nameStringUtf8Obj = factory->NewFromUtf8(utf8ArrayName, utf8ArrayNameLen);
    EXPECT_EQ(GlobalDictionary::Hash(nameStringUtf8Obj.GetTaggedValue()), 67); // 67 = (0 << 5 - 0 + 2) << 5 - 2 + 5
    // test obj is string(uint16_t)
    uint16_t utf16ArrayName[] = {0x1, 0x2, 0x1};
    uint32_t utf16ArrayNameLen = sizeof(utf16ArrayName) / sizeof(utf16ArrayName[0]);
    JSHandle<EcmaString> nameStringUtf16Obj = factory->NewFromUtf16(utf16ArrayName, utf16ArrayNameLen);
    // 1024 = (1 << 5 - 0 + 1) << 5 - 1 + 1
    EXPECT_EQ(GlobalDictionary::Hash(nameStringUtf16Obj.GetTaggedValue()), 1024);
}

/**
 * @tc.name: GetBoxAndValue
 * @tc.desc: Check whether the Box and Value through calling SetEntry function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, GetBoxAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> globalKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> globalKey1(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(123));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(100));
    JSHandle<JSTaggedValue> propertyBox(factory->NewPropertyBox(handleValue));
    JSHandle<JSTaggedValue> propertyBox1(factory->NewPropertyBox(handleValue1));
    PropertyAttributes attribute(1);
    // create GlobalDictionary
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4);
    EXPECT_TRUE(*handleDict != nullptr);
    handleDict->SetEntry(thread, 0, globalKey.GetTaggedValue(), propertyBox.GetTaggedValue(), attribute);
    // put value and cerate new dictionary
    JSHandle<GlobalDictionary> newDict =
        handleDict->PutIfAbsent(thread, handleDict, globalKey1, propertyBox1, attribute);

    EXPECT_TRUE(handleDict->GetBox(0) != nullptr);
    EXPECT_EQ(handleDict->GetValue(0).GetInt(), 123);

    EXPECT_TRUE(newDict->GetBox(0) != nullptr);
    EXPECT_EQ(newDict->GetValue(0).GetInt(), 123);
    EXPECT_TRUE(newDict->GetBox(1) != nullptr);
    EXPECT_EQ(newDict->GetValue(1).GetInt(), 100);
}

/**
 * @tc.name: GetAttributes
 * @tc.desc: Check whether the Attributes Get through calling SetAttributes function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, GetAttributes)
{
    // create GlobalDictionary
    int numberofElements = 4;
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, numberofElements);
    EXPECT_TRUE(*handleDict != nullptr);
    // set attributes call SetAttributes function
    for (int i = 0; i < numberofElements; i++) {
        handleDict->SetAttributes(thread, i, PropertyAttributes(i));
        EXPECT_EQ(handleDict->GetAttributes(i).GetPropertyMetaData(), i);
    }
}

/**
 * @tc.name: ClearEntry
 * @tc.desc: Create dictionary and set entry calling SetEntry function,Check whether Attributes is
 *           the same as Attributes after calling the ClearEntry function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, ClearEntry)
{
    // create GlobalDictionary
    int numberofElements = 16;
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, numberofElements);
    EXPECT_TRUE(*handleDict != nullptr);
    // set entry
    for (int i = 0; i < numberofElements; i++) {
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> handleValueKey(JSTaggedValue::ToString(thread, handleValue));
        handleDict->SetEntry(thread, i, handleValueKey.GetTaggedValue(),
                                        handleValue.GetTaggedValue(), PropertyAttributes(i));
    }
    // check attributes in three
    EXPECT_EQ(handleDict->GetAttributes(3).GetPropertyMetaData(), 3);
    handleDict->ClearEntry(thread, 3);
    EXPECT_EQ(handleDict->GetAttributes(3).GetPropertyMetaData(), 0);
}

/**
 * @tc.name: UpdateValueAndAttributes
 * @tc.desc: Update value and Attributes through calling UpdateValueAndAttributes function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, UpdateValueAndAttributes)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create GlobalDictionary
    int numberofElements = 16;
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, numberofElements);
    EXPECT_TRUE(*handleDict != nullptr);
    // set entry
    for (int i = 0; i < numberofElements; i++) {
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> propertyBox(factory->NewPropertyBox(handleValue));
        JSHandle<JSTaggedValue> handleValueKey(JSTaggedValue::ToString(thread, handleValue));
        handleDict->SetEntry(thread, i, handleValueKey.GetTaggedValue(),
                                        propertyBox.GetTaggedValue(), PropertyAttributes(i));
    }
    // check attributes in five
    EXPECT_EQ(handleDict->GetAttributes(5).GetPropertyMetaData(), 5);
    EXPECT_EQ(handleDict->GetValue(5).GetInt(), 5);
    // Update value and attributes
    for (int i = 0; i < numberofElements; i++) {
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(static_cast<int>(i + 1)));
        JSHandle<JSTaggedValue> propertyBox(factory->NewPropertyBox(handleValue));
        handleDict->UpdateValueAndAttributes(thread, i, propertyBox.GetTaggedValue(),
                                             PropertyAttributes(static_cast<int>(i + 1)));
    }
    // check attributes in five
    EXPECT_EQ(handleDict->GetAttributes(5).GetPropertyMetaData(), 6);
    EXPECT_EQ(handleDict->GetValue(5).GetInt(), 6);
}

/**
 * @tc.name: GetAllKeys
 * @tc.desc: Get all Attributes from dictionary and store it in the TaggedArray.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, GetAllKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create GlobalDictionary
    int numberofElements = 16;
    std::vector<CString> nameKey = {"a", "b", "c", "d", "e", "f",
                                    "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"};
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, numberofElements);
    EXPECT_TRUE(*handleDict != nullptr);
    JSMutableHandle<GlobalDictionary> dictHandle(handleDict);
    for (int i = 0; i < numberofElements; i++) {
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> handleNameKey(factory->NewFromASCII(nameKey[i]));
        PropertyAttributes metaData;
        // insert value
        JSHandle<GlobalDictionary> dict(GlobalDictionary::PutIfAbsent(thread, dictHandle,
                                                                      handleNameKey, handleValue, metaData));
        dictHandle.Update(dict.GetTaggedValue());
    }
    uint32_t offset = 7;
    // keyArray capacity must be enough for dictionary
    int arraySize = static_cast<uint32_t>(numberofElements) + offset;
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(arraySize);
    dictHandle->GetAllKeys(thread, offset, *keyArray);
    // Skip the first seven positions
    for (uint32_t i = 0; i < offset; i++) {
        EXPECT_TRUE(keyArray->Get(i).IsHole());
    }
    // check key name
    JSHandle<EcmaString> resultFirstKey(thread, keyArray->Get(offset));
    JSHandle<EcmaString> resultLastKey(thread, keyArray->Get(arraySize - 1));
    EXPECT_EQ(nameKey[0], CString(resultFirstKey->GetCString().get()).c_str());
    EXPECT_EQ(nameKey[15], CString(resultLastKey->GetCString().get()).c_str());
}

/**
 * @tc.name: GetEnumAllKeys
 * @tc.desc: Get all Enumerable Attributes from dictionary and store it in the TaggedArray.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, GetEnumAllKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create GlobalDictionary
    int numberofElements = 16;
    std::vector<CString> nameKey = {"a", "b", "c", "d", "e", "f",
                                    "g", "h", "i", "j", "k", "l", "m", "n", "o", "q"};
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, numberofElements);
    EXPECT_TRUE(*handleDict != nullptr);
    JSMutableHandle<GlobalDictionary> dictHandle(handleDict);
    bool enumerable;
    for (int i = 0; i < numberofElements; i++) {
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> handleNameKey(factory->NewFromASCII(nameKey[i]));
        PropertyAttributes metaData;
        enumerable = true;
        if (!(i % 2)) {
            enumerable = false;
        }
        metaData.SetEnumerable(enumerable);
        // insert value
        JSHandle<GlobalDictionary> dict(GlobalDictionary::PutIfAbsent(thread, dictHandle,
                                                                      handleNameKey, handleValue, metaData));
        dictHandle.Update(dict.GetTaggedValue());
    }
    uint32_t offset = 7;
    uint32_t keys = 0;
    // keyArray capacity must be enough for dictionary
    uint32_t arraySize = static_cast<uint32_t>(numberofElements) + offset;
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(arraySize);
    handleDict->GetEnumAllKeys(thread, offset, *keyArray, &keys);
    EXPECT_EQ(keys, 8U);
    JSHandle<EcmaString> resultFirstKey(thread, keyArray->Get(offset));
    JSHandle<EcmaString> resultLastKey(thread, keyArray->Get(offset + keys - 1U));
    EXPECT_EQ(nameKey[1], CString(resultFirstKey->GetCString().get()).c_str());
    EXPECT_EQ(nameKey[15], CString(resultLastKey->GetCString().get()).c_str());
}

/**
 * @tc.name: CompKey
 * @tc.desc: The second element in the two structures is compared.If it is less than,return true.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, CompKey)
{
    std::pair<JSTaggedValue, uint32_t> a(JSTaggedValue(1), 1);
    std::pair<JSTaggedValue, uint32_t> b(JSTaggedValue(2), 2);
    std::pair<JSTaggedValue, uint32_t> c(JSTaggedValue(0), 0);
    EXPECT_TRUE(GlobalDictionary::CompKey(a, b));
    EXPECT_TRUE(!GlobalDictionary::CompKey(a, c));
}

/**
 * @tc.name: InvalidatePropertyBox
 * @tc.desc: Invalidate value which is Configurable in a dictionary.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalDictionaryTest, InvalidatePropertyBox)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create GlobalDictionary
    int numberofElements = 16;
    std::vector<CString> nameKey = {"a", "b", "s", "t", "e", "f",
                                    "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"};
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, numberofElements);
    EXPECT_TRUE(*handleDict != nullptr);
    int invalidatedSet = 3;
    int invalidatedPosition = 12;
    for (int i = 0; i < numberofElements; i++) {
        JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII(nameKey[i]));
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> propertyBox(factory->NewPropertyBox(handleValue));
        PropertyAttributes metaData;
        if (i == invalidatedSet) {
            metaData.SetDictionaryOrder(invalidatedPosition);
        }
        else if (i == invalidatedPosition) {
            metaData.SetConfigurable(true);
        }
        handleDict->SetEntry(thread, i, handleKey.GetTaggedValue(),
                                        propertyBox.GetTaggedValue(), metaData);
    }
    // calling InvalidatePropertyBox function to Invalidate the PropertyBox
    PropertyAttributes newAttr(10);
    GlobalDictionary::InvalidatePropertyBox(thread, handleDict, invalidatedSet, newAttr);
    EXPECT_EQ(handleDict->GetAttributes(invalidatedPosition).GetBoxType(), PropertyBoxType::MUTABLE);
    EXPECT_EQ(handleDict->GetAttributes(invalidatedSet).GetPropertyMetaData(), 10);
    EXPECT_EQ(handleDict->GetValue(invalidatedPosition).GetInt(), invalidatedSet);
}
}  // namespace panda::test