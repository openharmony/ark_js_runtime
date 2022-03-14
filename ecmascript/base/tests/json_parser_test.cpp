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

#include "ecmascript/base/json_parser.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JsonParserTest : public testing::Test {
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
};

/**
 * @tc.name: Parser_001
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test without for no Nesting.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonParser<uint8_t> parser(thread);
    // JSON Number
    JSHandle<JSTaggedValue> handleMsg2(factory->NewFromCanBeCompressString("1234"));
    JSHandle<EcmaString> handleStr2(JSTaggedValue::ToString(thread, handleMsg2));
    JSHandle<JSTaggedValue> result2 = parser.ParseUtf8(*handleStr2);
    EXPECT_EQ(result2->GetNumber(), 1234);
    // JSON Literal
    JSHandle<JSTaggedValue> handleMsg3(factory->NewFromCanBeCompressString("true"));
    JSHandle<EcmaString> handleStr3(JSTaggedValue::ToString(thread, handleMsg3));
    JSHandle<JSTaggedValue> result3 = parser.ParseUtf8(*handleStr3);
    EXPECT_EQ(result3.GetTaggedValue(), JSTaggedValue::True());
    // JSON Unexpected
    JSHandle<JSTaggedValue> handleMsg4(factory->NewFromCanBeCompressString("trus"));
    JSHandle<EcmaString> handleStr4(JSTaggedValue::ToString(thread, handleMsg4));
    JSHandle<JSTaggedValue> result4 = parser.ParseUtf8(*handleStr4);
    EXPECT_EQ(result4.GetTaggedValue(), JSTaggedValue::Exception());
}

/**
 * @tc.name: Parser_002
 * @tc.desc: Passing in a character of type "uint16_t" check whether the result returned through "ParseUtf16" function
 *           Test without for no Nesting.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonParser<uint16_t> parser(thread);

    // JSON Number
    uint16_t array1Utf16[] = {0x31, 0x32, 0x33, 0x34}; // "1234"
    uint32_t array1Utf16Len = sizeof(array1Utf16) / sizeof(array1Utf16[0]);
    JSHandle<JSTaggedValue> handleMsg2(factory->NewFromUtf16(&array1Utf16[0], array1Utf16Len));
    JSHandle<EcmaString> handleStr2(JSTaggedValue::ToString(thread, handleMsg2));
    JSHandle<JSTaggedValue> result2 = parser.ParseUtf16(*handleStr2);
    EXPECT_EQ(result2->GetNumber(), 1234);
    // JSON Literal
    uint16_t array2Utf16[] = {0x74, 0x72, 0x75, 0x65}; // "true"
    uint32_t array2Utf16Len = sizeof(array2Utf16) / sizeof(array2Utf16[0]);
    JSHandle<JSTaggedValue> handleMsg3(factory->NewFromUtf16(&array2Utf16[0], array2Utf16Len));
    JSHandle<EcmaString> handleStr3(JSTaggedValue::ToString(thread, handleMsg3));
    JSHandle<JSTaggedValue> result3 = parser.ParseUtf16(*handleStr3);
    EXPECT_EQ(result3.GetTaggedValue(), JSTaggedValue::True());
    // JSON String
    uint16_t array3Utf16[] = {0x22, 0x73, 0x74, 0x72, 0x69, 0x6E, 0X67, 0x22}; // "string"
    uint32_t array3Utf16Len = sizeof(array3Utf16) / sizeof(array3Utf16[0]);
    JSHandle<JSTaggedValue> handleMsg4(factory->NewFromUtf16(&array3Utf16[0], array3Utf16Len));
    JSHandle<EcmaString> handleStr4(JSTaggedValue::ToString(thread, handleMsg4));
    JSHandle<JSTaggedValue> result4 = parser.ParseUtf16(*handleStr4);
    JSHandle<EcmaString> handleEcmaStr(result4);
    EXPECT_STREQ("string", CString(handleEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: Parser_003
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test with for Nesting of numbers, strings, objects, arrays, Booleans.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonParser<uint8_t> parser(thread);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromCanBeCompressString(
        "\t\r \n{\t\r \n \"json\"\t\r\n:\t\r \n{\t\r \n}\t\r \n,\t\r \n \"prop2\"\t\r \n:\t\r \n [\t\r \nfalse\t\r"
        "\n,\t\r \nnull\t\r \ntrue\t\r,123.456\t\r \n]\t\r \n}\t\r \n"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Object
    JSHandle<JSTaggedValue> result = parser.ParseUtf8(*handleStr);
    EXPECT_TRUE(result->IsECMAObject());
}

/**
 * @tc.name: Parser_004
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test with for Nesting of numbers, strings, arrays.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JsonParser<uint8_t> parser(thread);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromCanBeCompressString("[100,2.5,\"abc\"]"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Array
    JSHandle<JSTaggedValue> result = parser.ParseUtf8(*handleStr);

    JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
    EXPECT_TRUE(resultValue.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, resultValue);

    JSHandle<JSTaggedValue> lenResult =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    EXPECT_EQ(length, 3);
}

/**
 * @tc.name: Parser_005
 * @tc.desc: Passing in a character of type "uint8_t" check whether the result returned through "ParserUtf8" function
 *           Test without for Nesting of numbers, strings, objects.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonParserTest, Parser_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonParser<uint8_t> parser(thread);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromCanBeCompressString("{\"epf\":100,\"key1\":400}"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg)); // JSON Object

    JSHandle<JSTaggedValue> result = parser.ParseUtf8(*handleStr);
    JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
    EXPECT_TRUE(resultValue.IsECMAObject());

    JSHandle<JSObject> valueHandle(thread, resultValue);
    JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(thread, valueHandle));
    JSHandle<JSArray> nameResult = JSArray::CreateArrayFromList(thread, nameList);

    JSHandle<JSTaggedValue> handleKey(nameResult);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromCanBeCompressString("length"));
    JSHandle<JSTaggedValue> lenResult = JSObject::GetProperty(thread, handleKey, lengthKey).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    EXPECT_EQ(length, 2);
}
} // namespace panda::test