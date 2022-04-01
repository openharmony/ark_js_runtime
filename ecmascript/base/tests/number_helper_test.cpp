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

#include "ecmascript/base/number_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class NumberHelperTest : public testing::Test {
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
 * @tc.name: IsNaN
 * @tc.desc: Check whether number is Nan type data through "IsNaN" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, IsNaN)
{
    JSTaggedValue number1(1.23);
    EXPECT_FALSE(NumberHelper::IsNaN(number1));

    JSTaggedValue number2(-1.23);
    EXPECT_FALSE(NumberHelper::IsNaN(number2));

    JSTaggedValue number3(0.0f);
    EXPECT_FALSE(NumberHelper::IsNaN(number3));

    JSTaggedValue number4(0.0f / 0.0f);
    EXPECT_TRUE(NumberHelper::IsNaN(number4));

    JSTaggedValue number5(NAN_VALUE);
    EXPECT_TRUE(NumberHelper::IsNaN(number5));
}

/**
 * @tc.name: DoubleToString
 * @tc.desc: This function Convert the double type data into a string.first convert it into the corresponding
 *           hexadecimal number according to the transmitted radix, and convert the hexadecimal number into a
 *           string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, DoubleToString_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int radix;
    
    radix = 2;
    JSHandle<EcmaString> resultStr = factory->NewFromCanBeCompressString("100101");
    JSHandle<EcmaString> handleEcmaStr1(thread, NumberHelper::DoubleToString(thread, 37, radix));
    EXPECT_EQ(handleEcmaStr1->Compare(*resultStr), 0);

    radix = 3;
    resultStr = factory->NewFromCanBeCompressString("-1101");
    JSHandle<EcmaString> handleEcmaStr2(thread, NumberHelper::DoubleToString(thread, -37, radix));
    EXPECT_EQ(handleEcmaStr2->Compare(*resultStr), 0);

    radix = 4;
    resultStr = factory->NewFromCanBeCompressString("211");
    JSHandle<EcmaString> handleEcmaStr3(thread, NumberHelper::DoubleToString(thread, 37, radix));
    EXPECT_EQ(handleEcmaStr3->Compare(*resultStr), 0);

    radix = 5;
    resultStr = factory->NewFromCanBeCompressString("122");
    JSHandle<EcmaString> handleEcmaStr4(thread, NumberHelper::DoubleToString(thread, 37, radix));
    EXPECT_EQ(handleEcmaStr4->Compare(*resultStr), 0);

    radix = 6;
    resultStr = factory->NewFromCanBeCompressString("101");
    JSHandle<EcmaString> handleEcmaStr6(thread, NumberHelper::DoubleToString(thread, 37, radix));
    EXPECT_EQ(handleEcmaStr6->Compare(*resultStr), 0);

    radix = 7;
    resultStr = factory->NewFromCanBeCompressString("52");
    JSHandle<EcmaString> handleEcmaStr7(thread, NumberHelper::DoubleToString(thread, 37, radix));
    EXPECT_EQ(handleEcmaStr7->Compare(*resultStr), 0);

    radix = 36;
    resultStr = factory->NewFromCanBeCompressString("11");
    JSHandle<EcmaString> handleEcmaStr5(thread, NumberHelper::DoubleToString(thread, 37, radix));
    EXPECT_EQ(handleEcmaStr5->Compare(*resultStr), 0);
}

HWTEST_F_L0(NumberHelperTest, DoubleToString_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int radix = 2;

    JSHandle<EcmaString> resultStr =
        factory->NewFromCanBeCompressString("10.111111011011000110000101010010001010100110111101");
    JSHandle<EcmaString> handleEcmaStr1(thread, NumberHelper::DoubleToString(thread, 2.99099, radix));
    EXPECT_EQ(handleEcmaStr1->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("10.000000101001000000000011111011101010001000001001101");
    JSHandle<EcmaString> handleEcmaStr2(thread, NumberHelper::DoubleToString(thread, 2.01001, radix));
    EXPECT_EQ(handleEcmaStr2->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("10.100000000000011010001101101110001011101011000111001");
    JSHandle<EcmaString> handleEcmaStr3(thread, NumberHelper::DoubleToString(thread, 2.5001, radix));
    EXPECT_EQ(handleEcmaStr3->Compare(*resultStr), 0);

    radix = 36;
    resultStr = factory->NewFromCanBeCompressString("0.i04nym8equ");
    JSHandle<EcmaString> handleEcmaStr4(thread, NumberHelper::DoubleToString(thread, 0.5001, radix));
    EXPECT_EQ(handleEcmaStr4->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("0.wej2d0mt58f");
    JSHandle<EcmaString> handleEcmaStr5(thread, NumberHelper::DoubleToString(thread, 0.9001, radix));
    EXPECT_EQ(handleEcmaStr5->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("0.0d384dldb02");
    JSHandle<EcmaString> handleEcmaStr6(thread, NumberHelper::DoubleToString(thread, 0.0101, radix));
    EXPECT_EQ(handleEcmaStr6->Compare(*resultStr), 0);
}

/**
 * @tc.name: IsEmptyString
 * @tc.desc: Check whether the character is empty string through "IsEmptyString" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, IsEmptyString_001)
{
    // 9 ~ 13 and 32 belong to empty string
    uint8_t a[] = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x20};
    for (int i = 0; i < static_cast<int>(sizeof(a)); i++) {
        EXPECT_TRUE(NumberHelper::IsEmptyString(&a[i], &a[i] + 1));
    }
}

HWTEST_F_L0(NumberHelperTest, IsEmptyString_002)
{
    // 14 ~ 31 not belong to empty string
    uint8_t a[] = {0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    for (int i = 0; i < static_cast<int>(sizeof(a)); i++) {
        EXPECT_FALSE(NumberHelper::IsEmptyString(&a[i], &a[i] + 1));
    }
}

HWTEST_F_L0(NumberHelperTest, IsEmptyString_003)
{
    // 0 ~ 8 not belong to empty string
    uint8_t a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    for (int i = 0; i < static_cast<int>(sizeof(a)); i++) {
        EXPECT_FALSE(NumberHelper::IsEmptyString(&a[i], &a[i] + 1));
    }
}

HWTEST_F_L0(NumberHelperTest, IsEmptyString_004)
{
    // 160 belong to empty string
    uint16_t c = 160;
    utf_helper::Utf8Char d = utf_helper::ConvertUtf16ToUtf8(c, 0, true);
    EXPECT_EQ(d.ch.at(1), 160);
    uint8_t b = d.ch.at(1);
    EXPECT_TRUE(NumberHelper::IsEmptyString(&b, &b + 1));
}

/**
 * @tc.name: TruncateDouble
 * @tc.desc:This function takes the integer part of double type.When it is positive,it is rounded down
 *          When it is negative,it is rounded up.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, TruncateDouble)
{
    EXPECT_EQ(NumberHelper::TruncateDouble(NAN_VALUE), 0);
    EXPECT_EQ(NumberHelper::TruncateDouble(POSITIVE_INFINITY), POSITIVE_INFINITY);
    // round down
    EXPECT_EQ(NumberHelper::TruncateDouble(4.1), 4);
    EXPECT_EQ(NumberHelper::TruncateDouble(4.9), 4);
    EXPECT_EQ(NumberHelper::TruncateDouble(101.111), 101);
    EXPECT_EQ(NumberHelper::TruncateDouble(101.999), 101);
    // round up
    EXPECT_EQ(NumberHelper::TruncateDouble(-4.1), -4);
    EXPECT_EQ(NumberHelper::TruncateDouble(-4.9), -4);
    EXPECT_EQ(NumberHelper::TruncateDouble(-101.111), -101);
    EXPECT_EQ(NumberHelper::TruncateDouble(-101.999), -101);
}

/**
 * @tc.name: DoubleToInt
 * @tc.desc: This function takes the double of integer type.When the decimal part is eight and the number of digits
 *           is 15 ~ 16, add one to convert to integer.According to the binary digits of the integer part,it is divided
 *           into 8, 16 and 64 hexadecimal conversion,for example, 8-hexadecimal conversion.The maximum value of the
 *           integer part is 255. If it exceeds 255,the conversion fails.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, DoubleToInt_001)
{
    EXPECT_EQ(NumberHelper::DoubleToInt(9.0, INT8_BITS), 9);
    EXPECT_EQ(NumberHelper::DoubleToInt(9.5555555555555559, INT8_BITS), 9);
    EXPECT_EQ(NumberHelper::DoubleToInt(9.9999999999999999, INT8_BITS), 10);
    EXPECT_EQ(NumberHelper::DoubleToInt(128.123456, INT8_BITS), 128);
    EXPECT_EQ(NumberHelper::DoubleToInt(-128.987654321, INT8_BITS), -128);

    // the exponential bit digits exceeds 7
    EXPECT_EQ(NumberHelper::DoubleToInt(256.0, INT8_BITS), 0);
    EXPECT_EQ(NumberHelper::DoubleToInt(-256.0, INT8_BITS), 0);

    double nanDouble = 0.0f / 0.0f;
    EXPECT_EQ(NumberHelper::DoubleToInt(nanDouble, INT8_BITS), 0);
    double infDouble = POSITIVE_INFINITY;
    EXPECT_EQ(NumberHelper::DoubleToInt(infDouble, INT8_BITS), 0);
}

HWTEST_F_L0(NumberHelperTest, DoubleToInt_002)
{
    EXPECT_EQ(NumberHelper::DoubleToInt(256.0, INT16_BITS), 256);
    EXPECT_EQ(NumberHelper::DoubleToInt(256.555555555555556, INT16_BITS), 256);
    EXPECT_EQ(NumberHelper::DoubleToInt(256.999999999999999, INT16_BITS), 257);
    EXPECT_EQ(NumberHelper::DoubleToInt(1024.56789, INT16_BITS), 1024);
    EXPECT_EQ(NumberHelper::DoubleToInt(-1024.987654, INT16_BITS), -1024);

    // the exponential bit digits exceeds 15
    EXPECT_EQ(NumberHelper::DoubleToInt(65536.0, INT16_BITS), 0);
    EXPECT_EQ(NumberHelper::DoubleToInt(-65536.0, INT16_BITS), 0);

    double nanDouble = 0.0f / 0.0f;
    EXPECT_EQ(NumberHelper::DoubleToInt(nanDouble, INT16_BITS), 0);
    double infDouble = POSITIVE_INFINITY;
    EXPECT_EQ(NumberHelper::DoubleToInt(infDouble, INT16_BITS), 0);
}

HWTEST_F_L0(NumberHelperTest, DoubleToInt_003)
{
    EXPECT_EQ(NumberHelper::DoubleToInt(256.0, INT64_BITS), 256);
    EXPECT_EQ(NumberHelper::DoubleToInt(256.555555555555556, INT64_BITS), 256);
    EXPECT_EQ(NumberHelper::DoubleToInt(256.999999999999999, INT64_BITS), 257);
    EXPECT_EQ(NumberHelper::DoubleToInt(65536.55555, INT64_BITS), 65536);
    EXPECT_EQ(NumberHelper::DoubleToInt(-65536.99999, INT64_BITS), -65536);

    EXPECT_EQ(NumberHelper::DoubleToInt(2147483647.0, INT64_BITS), 2147483647);
    EXPECT_EQ(NumberHelper::DoubleToInt(-2147483647.0, INT64_BITS), -2147483647);

    double nanDouble = 0.0f / 0.0f;
    EXPECT_EQ(NumberHelper::DoubleToInt(nanDouble, INT64_BITS), 0);
    double infDouble = POSITIVE_INFINITY;
    EXPECT_EQ(NumberHelper::DoubleToInt(infDouble, INT64_BITS), 0);
}

/**
 * @tc.name: DoubleInRangeInt32
 * @tc.desc: The function is to convert double type to int type,The maximum value of integer part of double type
 *           cannot exceed the maximum value of int, and the minimum value of integer part cannot exceed the
 *           minimum value of int.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, DoubleInRangeInt32)
{
    // more than INT_MAX
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(2147483649.0), 2147483647);
    // less than INT_MIN
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(-2147483649.0), -2147483648);

    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(128.0), 128);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(-128.999999999999999), -129);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(256.0), 256);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(-256.0), -256);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(12345.6789), 12345);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(-12345.6789), -12345);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(65535.999999999999999), 65536);
    EXPECT_EQ(NumberHelper::DoubleInRangeInt32(-65535), -65535);
}

HWTEST_F_L0(NumberHelperTest, DoubleToExponential)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int radix;

    radix = -4;
    JSHandle<EcmaString> resultStr = factory->NewFromCanBeCompressString("1.239876e+2");
    JSHandle<EcmaString> handleEcmaStr1(thread, NumberHelper::DoubleToExponential(thread, 123.9876, radix));
    EXPECT_EQ(handleEcmaStr1->Compare(*resultStr), 0);

    radix = -6;
    resultStr = factory->NewFromCanBeCompressString("1.239876e+2");
    JSHandle<EcmaString> handleEcmaStr2(thread, NumberHelper::DoubleToExponential(thread, 123.9876, radix));
    EXPECT_EQ(handleEcmaStr2->Compare(*resultStr), 0);

    radix = 2;
    resultStr = factory->NewFromCanBeCompressString("1.24e+2");
    JSHandle<EcmaString> handleEcmaStr3(thread, NumberHelper::DoubleToExponential(thread, 123.567, radix));
    EXPECT_EQ(handleEcmaStr3->Compare(*resultStr), 0);

    radix = 6;
    resultStr = factory->NewFromCanBeCompressString("1.234567e+2");
    JSHandle<EcmaString> handleEcmaStr4(thread, NumberHelper::DoubleToExponential(thread, 123.4567, radix));
    EXPECT_EQ(handleEcmaStr4->Compare(*resultStr), 0);

    radix = 7;
    resultStr = factory->NewFromCanBeCompressString("1.2345670e+2");
    JSHandle<EcmaString> handleEcmaStr5(thread, NumberHelper::DoubleToExponential(thread, 123.45670, radix));
    EXPECT_EQ(handleEcmaStr5->Compare(*resultStr), 0);

    radix = 3;
    resultStr = factory->NewFromCanBeCompressString("1.230e+2");
    JSHandle<EcmaString> handleEcmaStr6(thread, NumberHelper::DoubleToExponential(thread, 123.0123, radix));
    EXPECT_EQ(handleEcmaStr6->Compare(*resultStr), 0);
}

HWTEST_F_L0(NumberHelperTest, DoubleToFixed)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int radix;

    radix = 1;
    JSHandle<EcmaString> resultStr = factory->NewFromCanBeCompressString("123.5");
    JSHandle<EcmaString> handleEcmaStr1(thread, NumberHelper::DoubleToFixed(thread, 123.456, radix));
    EXPECT_EQ(handleEcmaStr1->Compare(*resultStr), 0);

    radix = 2;
    resultStr = factory->NewFromCanBeCompressString("123.46");
    JSHandle<EcmaString> handleEcmaStr2(thread, NumberHelper::DoubleToFixed(thread, 123.456, radix));
    EXPECT_EQ(handleEcmaStr2->Compare(*resultStr), 0);

    radix = 3;
    resultStr = factory->NewFromCanBeCompressString("123.456");
    JSHandle<EcmaString> handleEcmaStr3(thread, NumberHelper::DoubleToFixed(thread, 123.456, radix));
    EXPECT_EQ(handleEcmaStr3->Compare(*resultStr), 0);

    radix = 4;
    resultStr = factory->NewFromCanBeCompressString("123.4560");
    JSHandle<EcmaString> handleEcmaStr4(thread, NumberHelper::DoubleToFixed(thread, 123.456, radix));
    EXPECT_EQ(handleEcmaStr4->Compare(*resultStr), 0);

    radix = 0;
    resultStr = factory->NewFromCanBeCompressString("123");
    JSHandle<EcmaString> handleEcmaStr5(thread, NumberHelper::DoubleToFixed(thread, 123.456, radix));
    EXPECT_EQ(handleEcmaStr5->Compare(*resultStr), 0);
}

/**
 * @tc.name: DoubleToPrecision
 * @tc.desc: Convert double decimal type to Precision type through "DoubleToPrecision" function.If the logarithm
 *           of number based on ten is less than zero and radix Digit is more than zero or it is greater than zero
 *           and radix Digit is less than MAX_EXPONENT_DIGIT call the DoubleToFixed function.other call
 *           DoubleToExponential function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, DoubleToPrecision)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int radix;

    radix = 1;
    JSHandle<EcmaString> resultStr = factory->NewFromCanBeCompressString("0");
    JSHandle<EcmaString> handleEcmaStr1(thread, NumberHelper::DoubleToPrecision(thread, 0.0, radix));
    EXPECT_EQ(handleEcmaStr1->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("0.0001");
    JSHandle<EcmaString> handleEcmaStr2(thread, NumberHelper::DoubleToPrecision(thread, 0.0001, radix));
    EXPECT_EQ(handleEcmaStr2->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("1e-7");
    JSHandle<EcmaString> handleEcmaStr3(thread, NumberHelper::DoubleToPrecision(thread, 0.0000001, radix));
    EXPECT_EQ(handleEcmaStr3->Compare(*resultStr), 0);

    resultStr = factory->NewFromCanBeCompressString("1e+3");
    JSHandle<EcmaString> handleEcmaStr5(thread, NumberHelper::DoubleToPrecision(thread, 1000.1234, radix));
    EXPECT_EQ(handleEcmaStr5->Compare(*resultStr), 0);

    radix = 6;
    resultStr = factory->NewFromCanBeCompressString("1000.12");
    JSHandle<EcmaString> handleEcmaStr6(thread, NumberHelper::DoubleToPrecision(thread, 1000.1234, radix));
    EXPECT_EQ(handleEcmaStr6->Compare(*resultStr), 0);
}

/**
 * @tc.name: DoubleToPrecision
 * @tc.desc: Convert double decimal type to Precision type through "DoubleToPrecision" function.If the logarithm
 *           of number based on ten is less than zero and radix Digit is more than zero or it is greater than zero
 *           and radix Digit is less than MAX_EXPONENT_DIGIT call the DoubleToFixed function.other call
 *           DoubleToExponential function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, StringToDoubleWithRadix)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int radix;
    Span<const uint8_t> sp;
    JSHandle<EcmaString> resultStr;

    radix = 3;
    resultStr = factory->NewFromCanBeCompressString("-12");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    // 5 = 1 * 3 + 2
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), -5);

    radix = 4;
    resultStr = factory->NewFromCanBeCompressString("1234567");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    // 27 = (1 * 4 + 2) * 4 + 3
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 27);
    // string has space
    resultStr = factory->NewFromCanBeCompressString(" 12345 ");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 27);

    radix = 16;
    resultStr = factory->NewFromCanBeCompressString("0x00ff");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 255);

    resultStr = factory->NewFromCanBeCompressString("0x0010");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 16);

    resultStr = factory->NewFromCanBeCompressString("0x1234");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 4660);
    // string has space
    resultStr = factory->NewFromCanBeCompressString(" 0x12  ");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 18);

    resultStr = factory->NewFromCanBeCompressString("0x1234XX");
    sp = Span<const uint8_t>(resultStr->GetDataUtf8(), resultStr->GetUtf8Length() - 1);
    EXPECT_EQ(NumberHelper::StringToDoubleWithRadix(sp.begin(), sp.end(), radix).GetDouble(), 4660);
}

/**
 * @tc.name: IntToString
 * @tc.desc: Convert integer type to string type through "IntToString" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, IntToString)
{
    EXPECT_STREQ(NumberHelper::IntToString(0).c_str(), "0");
    EXPECT_STREQ(NumberHelper::IntToString(-100).c_str(), "-100");
    EXPECT_STREQ(NumberHelper::IntToString(123).c_str(), "123");
    EXPECT_STREQ(NumberHelper::IntToString(1234).c_str(), "1234");
}

/**
 * @tc.name: IntegerToString
 * @tc.desc: Convert the decimal number into the hexadecimal number corresponding to Radix and convert it into a string
 *           Check whether the returned string result is the same as the actual conversion result.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(NumberHelperTest, IntegerToString)
{
    int radix = 2;
    // number = (radix + 1) * MAX_MANTISSA
    CString integerStr = NumberHelper::IntegerToString(static_cast<double>((radix + 1) * (0x1ULL << 52U)), radix);
    EXPECT_STREQ(integerStr.c_str(), "110000000000000000000000000000000000000000000000000000");

    integerStr = NumberHelper::IntegerToString(static_cast<double>(10), radix);
    EXPECT_STREQ(integerStr.c_str(), "1010");

    radix = 3;
    integerStr = NumberHelper::IntegerToString(static_cast<double>(33), radix);
    EXPECT_STREQ(integerStr.c_str(), "1020");

    radix = 8;
    integerStr = NumberHelper::IntegerToString(static_cast<double>(128), radix);
    EXPECT_STREQ(integerStr.c_str(), "200");

    integerStr = NumberHelper::IntegerToString(static_cast<double>(128.985), radix);
    EXPECT_STREQ(integerStr.c_str(), "200");

    radix = 16;
    integerStr = NumberHelper::IntegerToString(static_cast<double>(256), radix);
    EXPECT_STREQ(integerStr.c_str(), "100");

    radix = 24;
    integerStr = NumberHelper::IntegerToString(static_cast<double>(987654), radix);
    EXPECT_STREQ(integerStr.c_str(), "2nag6");

    integerStr = NumberHelper::IntegerToString(static_cast<double>(23), radix);
    EXPECT_STREQ(integerStr.c_str(), "n");
}
} // namespace panda::ecmascript
