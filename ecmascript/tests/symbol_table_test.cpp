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

#include "ecmascript/symbol_table.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class SymbolTableTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    PandaVM *instance {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(SymbolTableTest, GetKeyIndex)
{
    int entry = 0;
    EXPECT_EQ(SymbolTable::GetKeyIndex(entry), 3);
}

HWTEST_F_L0(SymbolTableTest, GetValueIndex)
{
    int entry = 0;
    EXPECT_EQ(SymbolTable::GetValueIndex(entry), 4);
}

HWTEST_F_L0(SymbolTableTest, GetEntryIndex)
{
    int entry = 0;
    EXPECT_EQ(SymbolTable::GetEntryIndex(entry), 3);
}

HWTEST_F_L0(SymbolTableTest, GetEntrySize)
{
    EXPECT_EQ(SymbolTable::GetEntrySize(), 2);
}

/*
 * Feature: SymbolTableTest
 * Function: IsMatch
 * SubFunction: StringsAreEqual
 * FunctionPoints: Is Match
 * CaseDescription: Judge whether two string variables are equal. If they are equal,
 *                  it returns true, otherwise it returns false.
 */
HWTEST_F_L0(SymbolTableTest, IsMatch)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> symbolTableString = factory->NewFromASCII("name");
    JSTaggedValue symbolTableOther = symbolTableString.GetTaggedValue();

    JSTaggedValue symbolTableName1 = JSTaggedValue::Hole();
    EXPECT_EQ(SymbolTable::IsMatch(symbolTableName1, symbolTableOther), false);

    JSTaggedValue symbolTableName2 = JSTaggedValue::Undefined();
    EXPECT_EQ(SymbolTable::IsMatch(symbolTableName2, symbolTableOther), false);

    JSTaggedValue symbolTableName3 = symbolTableString.GetTaggedValue();
    EXPECT_EQ(SymbolTable::IsMatch(symbolTableName3, symbolTableOther), true);
}

/*
 * Feature: SymbolTableTest
 * Function: Hash_Utf8
 * SubFunction: GetHashCode
 * FunctionPoints: Hash
 * CaseDescription: The hash code is obtained by passing in a character string array or an uint8_t
 *                  type array through a specific calculation formula.
 */
HWTEST_F_L0(SymbolTableTest, Hash_Utf8)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // test obj is not string
    JSHandle<JSTaggedValue> jsObJect(factory->NewEmptyJSObject());
    EXPECT_EQ(SymbolTable::Hash(jsObJect.GetTaggedValue()), JSSymbol::ComputeHash());

    // the CompressedStringsEnabled must  be true
    bool flag = EcmaString::GetCompressedStringsEnabled();
    EXPECT_EQ(flag, true);

    uint8_t utf8ArrayName1[4] = {1, 2, 3}; // The last element is "\0"
    uint32_t utf8ArrayNameLen1 = sizeof(utf8ArrayName1) - 1;
    JSHandle<EcmaString> nameStringUtf8Obj1 = factory->NewFromUtf8(utf8ArrayName1, utf8ArrayNameLen1);
    EXPECT_EQ(SymbolTable::Hash(nameStringUtf8Obj1.GetTaggedValue()), 1026U); // 1026 = (1 << 5 - 1 + 2) << 5 - 2 + 3

    uint8_t utf8ArrayName2[] = "key";
    uint32_t utf8ArrayNameLen2 = sizeof(utf8ArrayName2) - 1;
    JSHandle<EcmaString> nameStringUtf8Obj2 = factory->NewFromUtf8(utf8ArrayName2, utf8ArrayNameLen2);
    EXPECT_EQ(SymbolTable::Hash(nameStringUtf8Obj2.GetTaggedValue()), 106079U);
}

/*
 * Feature: SymbolTableTest
 * Function: Hash_Utf16
 * SubFunction: GetHashCode
 * FunctionPoints: Hash
 * CaseDescription: The hash code is obtained by passing in a character string array or an uint16_t
 *                  type array through a specific calculation formula.
 */
HWTEST_F_L0(SymbolTableTest, Hash_Utf16)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint16_t utf16ArrayName1[] = {1, 2, 3};
    uint32_t utf16ArrayNameLen1 = sizeof(utf16ArrayName1) / sizeof(utf16ArrayName1[0]);
    JSHandle<EcmaString> nameStringUtf16Obj1 = factory->NewFromUtf16(utf16ArrayName1, utf16ArrayNameLen1);
    EXPECT_EQ(SymbolTable::Hash(nameStringUtf16Obj1.GetTaggedValue()), 1026U); // 1026 = (1 << 5 - 1 + 2) << 5 - 2 + 3

    uint16_t utf16ArrayName2[] = {0, 1, 2};
    uint32_t utf16ArrayNameLen2 = sizeof(utf16ArrayName2) / sizeof(utf16ArrayName2[0]);
    JSHandle<EcmaString> nameStringUtf16Obj2 = factory->NewFromUtf16(utf16ArrayName2, utf16ArrayNameLen2);
    EXPECT_EQ(SymbolTable::Hash(nameStringUtf16Obj2.GetTaggedValue()), 33U); // 33 = (0 << 5 - 0 + 1) << 5 - 1 + 2
}

/*
 * Feature: SymbolTableTest
 * Function: Create
 * SubFunction: *Value
 * FunctionPoints: Create
 * CaseDescription: A pointer variable of symboltable type is obtained by changing the function,
 *                  If it is created successfully, the pointer variable is not equal to null,
 *                  The prerequisite for creation is that compressedstringsenabled must be true.
 */
HWTEST_F_L0(SymbolTableTest, Create)
{
    int numberOfElements = SymbolTable::DEFAULT_ELEMENTS_NUMBER;
    // the CompressedStringsEnabled must  be true
    bool flag = EcmaString::GetCompressedStringsEnabled();
    EXPECT_EQ(flag, true);

    JSHandle<SymbolTable> symbolTable = SymbolTable::Create(thread, numberOfElements);
    EXPECT_TRUE(*symbolTable != nullptr);
}

/*
 * Feature: SymbolTableTest
 * Function: ContainsKey
 * SubFunction: Getkey
 * FunctionPoints: Contains Key
 * CaseDescription: Judge whether the key value can be found in the key value in your own created symbol
 *                  table.If you do not have a key value, you will return false.
 */
HWTEST_F_L0(SymbolTableTest, ContainsKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> symbolTableStringKey1 = factory->NewFromASCII("key");
    JSHandle<EcmaString> symbolTableStringKey2 = factory->NewFromASCII("key1");
    JSHandle<EcmaString> symbolTableStringKey3 = factory->NewFromASCII("value");

    int numberOfElements = 2;
    JSHandle<SymbolTable> symbolTable = SymbolTable::Create(thread, numberOfElements);
    EXPECT_EQ(symbolTable->ContainsKey(symbolTableStringKey1.GetTaggedValue()), false);

    symbolTable->SetKey(thread, 1, JSTaggedValue::Hole());
    EXPECT_EQ(symbolTable->ContainsKey(symbolTableStringKey1.GetTaggedValue()), false);

    symbolTable->SetKey(thread, 1, JSTaggedValue::Undefined());
    EXPECT_EQ(symbolTable->ContainsKey(symbolTableStringKey1.GetTaggedValue()), false);

    symbolTable->SetKey(thread, 1, symbolTableStringKey1.GetTaggedValue());
    EXPECT_EQ(symbolTable->ContainsKey(symbolTableStringKey1.GetTaggedValue()), true);

    // the key value has numbers
    symbolTable->SetKey(thread, 1, symbolTableStringKey2.GetTaggedValue());
    EXPECT_EQ(symbolTable->ContainsKey(symbolTableStringKey2.GetTaggedValue()), false);

    symbolTable->SetKey(thread, 1, symbolTableStringKey3.GetTaggedValue());
    EXPECT_EQ(symbolTable->ContainsKey(symbolTableStringKey3.GetTaggedValue()), true);
}

/*
 * Feature: SymbolTableTest
 * Function: GetSymbol
 * SubFunction: GetValue
 * FunctionPoints: Get Symbol
 * CaseDescription: This function obtains the value in the key of symbol table pointer variable created
 *                  by the create function. If the pointer variable has no value set, it returns false.
 */
HWTEST_F_L0(SymbolTableTest, GetSymbol)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int numberOfElements = 2;

    JSHandle<EcmaString> symbolTableStringKey = factory->NewFromASCII("key");
    JSHandle<SymbolTable> symbolTable = SymbolTable::Create(thread, numberOfElements);

    symbolTable->SetKey(thread, 1, symbolTableStringKey.GetTaggedValue());
    EXPECT_EQ(symbolTable->GetSymbol(symbolTableStringKey.GetTaggedValue()), JSTaggedValue::Undefined());

    symbolTable->SetValue(thread, 0, JSTaggedValue(1));
    EXPECT_EQ(symbolTable->GetSymbol(symbolTableStringKey.GetTaggedValue()), JSTaggedValue::Undefined());

    symbolTable->SetValue(thread, 1, JSTaggedValue(1));
    EXPECT_EQ(symbolTable->GetSymbol(symbolTableStringKey.GetTaggedValue()).GetInt(), 1);
}

/*
 * Feature: SymbolTableTest
 * Function: FindSymbol
 * SubFunction: GetKey
 * FunctionPoints: Find Symbol
 * CaseDescription: This function compares the key value in the symboltable pointer variable created by the create
 *                  function with the description value in the jssymbol type variable. If they are equal, it indicates
 *                  that the find symbol is successful, and the return value is the key value. Before creating the
 *                  symboltable pointer, perform the newfromcanbecompressstring operation to obtain the array length.
 */
HWTEST_F_L0(SymbolTableTest, FindSymbol)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> symbolTableStringKey1(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> symbolTableStringKey2(factory->NewFromASCII("key1"));

    int numberOfElements = 2;
    JSHandle<JSSymbol> handleSymbol = factory->NewJSSymbol();
    JSHandle<SymbolTable> symbolTable = SymbolTable::Create(thread, numberOfElements);

    JSTaggedValue resultValue1 = symbolTable->FindSymbol(handleSymbol.GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::SameValue(resultValue1, JSTaggedValue::Undefined()), true);

    handleSymbol->SetDescription(thread, symbolTableStringKey1.GetTaggedValue());
    JSTaggedValue resultValue2 = symbolTable->FindSymbol(handleSymbol.GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::SameValue(resultValue2, JSTaggedValue::Undefined()), true);

    symbolTable->SetKey(thread, 1, symbolTableStringKey1.GetTaggedValue());
    JSTaggedValue resultValue3 = symbolTable->FindSymbol(handleSymbol.GetTaggedValue());
    EXPECT_EQ(resultValue3.GetRawData() == symbolTableStringKey1.GetTaggedValue().GetRawData(), true);

    symbolTable->SetKey(thread, 1, symbolTableStringKey2.GetTaggedValue());
    JSTaggedValue resultValue4 = symbolTable->FindSymbol(handleSymbol.GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::SameValue(resultValue4, JSTaggedValue::Undefined()), true);
}
}  // namespace panda::test
