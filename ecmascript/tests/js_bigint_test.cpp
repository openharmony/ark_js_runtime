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

#include "ecmascript/js_bigint.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSBigintTest : public testing::Test {
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
 * @tc.name: ComString
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, ComString)
{
    CString str1 = "9007199254740991012345";
    CString str2 = "9007199254740991012345 ";
    CString str3 = "-9007199254740991012345";
    EXPECT_EQ(BigInt::ComString(str1, str1), Comparestr::EQUAL);
    EXPECT_EQ(BigInt::ComString(str3, str2), Comparestr::LESS);
    EXPECT_EQ(BigInt::ComString(str1, str2), Comparestr::LESS);
    EXPECT_EQ(BigInt::ComString(str2, str1), Comparestr::GREATER);
    EXPECT_EQ(BigInt::ComString(str2, str3), Comparestr::GREATER);
}

/**
 * @tc.name: CreateBigint
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, CreateBigint)
{
    uint32_t size = 100;
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, size);
    EXPECT_EQ(bigint->GetLength(), size);
}

/**
 * @tc.name: Equal & SameValue & SameValueZero
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Equal_SameValue_SameValueZero)
{
    // The largest safe integer in JavaScript is in [-(2 ^ 53 - 1), 2 ^ 53 - 1].
    CString maxSafeIntStr = "9007199254740991";
    CString minSafeIntStr = "-9007199254740991";
    JSHandle<BigInt> maxSafeInt = BigIntHelper::SetBigInt(thread, maxSafeIntStr);
    JSHandle<BigInt> minSafeInt = BigIntHelper::SetBigInt(thread, minSafeIntStr);

    // Compare two integers in the safe range.
    JSHandle<BigInt> minusMinSafeInt = BigInt::UnaryMinus(thread, minSafeInt);
    JSHandle<BigInt> minusMaxSafeInt = BigInt::UnaryMinus(thread, maxSafeInt);
    bool result1 = BigInt::Equal(maxSafeInt.GetTaggedValue(), minSafeInt.GetTaggedValue());
    bool result2 = BigInt::SameValue(maxSafeInt.GetTaggedValue(), minSafeInt.GetTaggedValue());
    bool result3 = BigInt::SameValueZero(maxSafeInt.GetTaggedValue(), minSafeInt.GetTaggedValue());
    EXPECT_TRUE(!result1);
    EXPECT_TRUE(!result2);
    EXPECT_TRUE(!result3);
    result1 = BigInt::Equal(maxSafeInt.GetTaggedValue(), minusMinSafeInt.GetTaggedValue());
    result2 = BigInt::SameValue(maxSafeInt.GetTaggedValue(), minusMinSafeInt.GetTaggedValue());
    result3 = BigInt::SameValueZero(maxSafeInt.GetTaggedValue(), minusMinSafeInt.GetTaggedValue());
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
    result1 = BigInt::Equal(minSafeInt.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue());
    result2 = BigInt::SameValue(minSafeInt.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue());
    result3 = BigInt::SameValueZero(minSafeInt.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue());
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);

    // Compare two integers outside the safe range.
    CString unsafeIntStr1 = maxSafeIntStr + "0123456789";
    CString unsafeIntStr2 = minSafeIntStr + "0123456789";
    JSHandle<BigInt> unsafeInt1 = BigIntHelper::SetBigInt(thread, unsafeIntStr1);
    JSHandle<BigInt> unsafeInt2 = BigIntHelper::SetBigInt(thread, unsafeIntStr2);
    JSHandle<BigInt> minusUnsafeInt1 = BigInt::UnaryMinus(thread, unsafeInt1);
    result1 = BigInt::Equal(unsafeInt2.GetTaggedValue(), minusUnsafeInt1.GetTaggedValue());
    result2 = BigInt::SameValue(unsafeInt2.GetTaggedValue(), minusUnsafeInt1.GetTaggedValue());
    result3 = BigInt::SameValueZero(unsafeInt2.GetTaggedValue(), minusUnsafeInt1.GetTaggedValue());
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
}

/**
 * @tc.name: InitializationZero
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, InitializationZero)
{
    CString maxSafeIntPlusOneStr = "9007199254740992";
    JSHandle<BigInt> maxSafeIntPlusOne = BigIntHelper::SetBigInt(thread, maxSafeIntPlusOneStr);
    uint32_t size = maxSafeIntPlusOne->GetLength();
    uint32_t countZero = 0;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t digit = maxSafeIntPlusOne->GetDigit(i);
        if (digit == 0) {
            countZero++;
        }
    }
    EXPECT_NE(countZero, size);

    BigInt::InitializationZero(thread, maxSafeIntPlusOne);
    for (uint32_t i = 0; i < size; i++) {
        uint32_t digit = maxSafeIntPlusOne->GetDigit(i);
        EXPECT_EQ(digit, 0U);
    }
}

/**
 * @tc.name: BitwiseOp & BitwiseAND & BitwiseXOR & BitwiseOR & BitwiseSubOne & BitwiseAddOne & BitwiseNOT
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Bitwise_AND_XOR_OR_NOT_SubOne_AddOne)
{
    CString maxSafeIntStr = "11111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 53 - 1
    CString maxSafeIntPlusOneStr = "100000000000000000000000000000000000000000000000000000"; // Binary: 2 ^ 53
    CString bigintStr1 = "111111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 54 - 1
    JSHandle<BigInt> maxSafeInt = BigIntHelper::SetBigInt(thread, maxSafeIntStr, BigInt::BINARY);
    JSHandle<BigInt> maxSafeIntPlusOne = BigIntHelper::SetBigInt(thread, maxSafeIntPlusOneStr, BigInt::BINARY);
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, bigintStr1, BigInt::BINARY);

    // Bitwise AND operation
    JSHandle<BigInt> addOpRes = BigInt::BitwiseOp(thread, Operate::AND, maxSafeIntPlusOne, bigint1);
    JSHandle<BigInt> andRes = BigInt::BitwiseAND(thread, maxSafeIntPlusOne, bigint1);
    EXPECT_TRUE(BigInt::Equal(addOpRes.GetTaggedValue(), maxSafeIntPlusOne.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(andRes.GetTaggedValue(), maxSafeIntPlusOne.GetTaggedValue()));

    // Bitwise OR operation
    JSHandle<BigInt> orOpRes = BigInt::BitwiseOp(thread, Operate::OR, maxSafeInt, maxSafeIntPlusOne);
    JSHandle<BigInt> orRes = BigInt::BitwiseOR(thread, maxSafeInt, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(orOpRes.GetTaggedValue(), bigint1.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(orRes.GetTaggedValue(), bigint1.GetTaggedValue()));

    // Bitwise XOR operation
    JSHandle<BigInt> xorOpRes = BigInt::BitwiseOp(thread, Operate::XOR, maxSafeIntPlusOne, bigint1);
    JSHandle<BigInt> xorRes = BigInt::BitwiseXOR(thread, maxSafeIntPlusOne, bigint1);
    EXPECT_TRUE(BigInt::Equal(xorOpRes.GetTaggedValue(), maxSafeInt.GetTaggedValue()));
    EXPECT_TRUE(BigInt::Equal(xorRes.GetTaggedValue(), maxSafeInt.GetTaggedValue()));

    // Bitwise NOT operation, include sign bits.
    JSHandle<BigInt> notRes1 = BigInt::BitwiseNOT(thread, maxSafeInt);
    JSHandle<BigInt> minusMaxSafeInt = BigInt::UnaryMinus(thread, maxSafeIntPlusOne);
    // ~x == -x-1 == -(x+1)
    EXPECT_TRUE(BigInt::Equal(notRes1.GetTaggedValue(), minusMaxSafeInt.GetTaggedValue()));
    JSHandle<BigInt> notRes2 = BigInt::BitwiseNOT(thread, minusMaxSafeInt);
    // ~(-x) == ~(~(x-1)) == x-1
    EXPECT_TRUE(BigInt::Equal(notRes2.GetTaggedValue(), maxSafeInt.GetTaggedValue()));

    // Bitwise sub one operation, include sign bits.
    uint32_t maxSize = maxSafeIntPlusOne->GetLength();
    JSHandle<BigInt> subOneRes = BigInt::BitwiseSubOne(thread, maxSafeIntPlusOne, maxSize);
    EXPECT_TRUE(BigInt::Equal(subOneRes.GetTaggedValue(), maxSafeInt.GetTaggedValue()));

    // Bitwise add one operation, include sign bits.
    JSHandle<BigInt> addOneRes = BigInt::BitwiseAddOne(thread, maxSafeInt);
    JSHandle<BigInt> minusMaxSafePlusOneInt = BigInt::UnaryMinus(thread, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(addOneRes.GetTaggedValue(), minusMaxSafePlusOneInt.GetTaggedValue()));
}

/**
 * @tc.name: ToString & ToStdString
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, ToString_ToStdString)
{
    CString bigintStdStr1 = "111111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 54 - 1
    CString bigintStdStr2 = "1234567890987654321"; // Decimal
    JSHandle<BigInt> bigint1 = BigIntHelper::SetBigInt(thread, bigintStdStr1, BigInt::BINARY);
    JSHandle<BigInt> bigint2 = BigIntHelper::SetBigInt(thread, bigintStdStr2, BigInt::DECIMAL);

    JSHandle<EcmaString> bigintEcmaStrBin1 = BigInt::ToString(thread, bigint1, BigInt::BINARY);
    EXPECT_STREQ(CString(bigintEcmaStrBin1->GetCString().get()).c_str(),
        "111111111111111111111111111111111111111111111111111111");
    JSHandle<EcmaString> bigintEcmaStrOct1 = BigInt::ToString(thread, bigint1, BigInt::OCTAL);
    EXPECT_STREQ(CString(bigintEcmaStrOct1->GetCString().get()).c_str(), "777777777777777777");
    JSHandle<EcmaString> bigintEcmaStrDec1 = BigInt::ToString(thread, bigint1, BigInt::DECIMAL);
    EXPECT_STREQ(CString(bigintEcmaStrDec1->GetCString().get()).c_str(), "18014398509481983");
    JSHandle<EcmaString> bigintEcmaStrHex1 = BigInt::ToString(thread, bigint1, BigInt::HEXADECIMAL);
    EXPECT_STREQ(CString(bigintEcmaStrHex1->GetCString().get()).c_str(), "3fffffffffffff");

    JSHandle<EcmaString> bigintEcmaStrBin2 = BigInt::ToString(thread, bigint2, BigInt::BINARY);
    EXPECT_STREQ(CString(bigintEcmaStrBin2->GetCString().get()).c_str(),
        "1000100100010000100001111010010110001011011000001110010110001");
    EXPECT_STREQ(CString(bigintEcmaStrBin2->GetCString().get()).c_str(),
        (bigint2->ToStdString(BigInt::BINARY)).c_str());

    JSHandle<EcmaString> bigintEcmaStrOct2 = BigInt::ToString(thread, bigint2, BigInt::OCTAL);
    EXPECT_STREQ(CString(bigintEcmaStrOct2->GetCString().get()).c_str(), "104420417226133016261");
    EXPECT_STREQ(CString(bigintEcmaStrOct2->GetCString().get()).c_str(),
        (bigint2->ToStdString(BigInt::OCTAL)).c_str());

    JSHandle<EcmaString> bigintEcmaStrDec2 = BigInt::ToString(thread, bigint2, BigInt::DECIMAL);
    EXPECT_STREQ(CString(bigintEcmaStrDec2->GetCString().get()).c_str(), "1234567890987654321");
    EXPECT_STREQ(CString(bigintEcmaStrDec2->GetCString().get()).c_str(),
        (bigint2->ToStdString(BigInt::DECIMAL)).c_str());

    JSHandle<EcmaString> bigintEcmaStrHex2 = BigInt::ToString(thread, bigint2, BigInt::HEXADECIMAL);
    EXPECT_STREQ(CString(bigintEcmaStrHex2->GetCString().get()).c_str(), "112210f4b16c1cb1");
    EXPECT_STREQ(CString(bigintEcmaStrHex2->GetCString().get()).c_str(),
        (bigint2->ToStdString(BigInt::HEXADECIMAL)).c_str());
}

/**
 * @tc.name: UnaryMinus
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, UnaryMinus)
{
    CString maxSafeIntStr = "9007199254740991";
    CString minSafeIntStr = "-9007199254740991";
    CString maxSafeIntPlusOneStr = "9007199254740992";
    CString minSafeIntSubOneStr = "-9007199254740992";
    JSHandle<BigInt> maxSafeInt = BigIntHelper::SetBigInt(thread, maxSafeIntStr);
    JSHandle<BigInt> minSafeInt = BigIntHelper::SetBigInt(thread, minSafeIntStr);
    JSHandle<BigInt> maxSafeIntPlusOne = BigIntHelper::SetBigInt(thread, maxSafeIntPlusOneStr);
    JSHandle<BigInt> minSafeIntSubOne = BigIntHelper::SetBigInt(thread, minSafeIntSubOneStr);

    JSHandle<BigInt> minusRes1 = BigInt::UnaryMinus(thread, maxSafeInt);
    EXPECT_TRUE(BigInt::Equal(minusRes1.GetTaggedValue(), minSafeInt.GetTaggedValue()));
    JSHandle<BigInt> minusRes2 = BigInt::UnaryMinus(thread, minSafeInt);
    EXPECT_TRUE(BigInt::Equal(minusRes2.GetTaggedValue(), maxSafeInt.GetTaggedValue()));
    JSHandle<BigInt> minusRes3 = BigInt::UnaryMinus(thread, maxSafeIntPlusOne);
    EXPECT_TRUE(BigInt::Equal(minusRes3.GetTaggedValue(), minSafeIntSubOne.GetTaggedValue()));
    JSHandle<BigInt> minusRes4 = BigInt::UnaryMinus(thread, minSafeIntSubOne);
    EXPECT_TRUE(BigInt::Equal(minusRes4.GetTaggedValue(), maxSafeIntPlusOne.GetTaggedValue()));
}

/**
 * @tc.name: Exponentiate & Multiply & Divide & Remainder
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSBigintTest, Exponentiate_Multiply_Divide_Remainder)
{
    CString baseBigintStr = "2";
    CString expBigintStr1 = "53";
    CString expBigintStr2 = "54";
    CString resBigintStr1 = "9007199254740992"; // 2 ^ 53
    CString resBigintStr2 = "18014398509481984"; // 2 ^ 54
    CString resBigintStr3 = "162259276829213363391578010288128"; // 2 ^ 107
    CString resBigintStr4 = "162259276829213363391578010288182"; // 2 ^ 107 + 54
    JSHandle<BigInt> baseBigint = BigIntHelper::SetBigInt(thread, baseBigintStr);
    JSHandle<BigInt> expBigint1 = BigIntHelper::SetBigInt(thread, expBigintStr1);
    JSHandle<BigInt> expBigint2 = BigIntHelper::SetBigInt(thread, expBigintStr2);
    JSHandle<BigInt> resBigint1 = BigIntHelper::SetBigInt(thread, resBigintStr1);
    JSHandle<BigInt> resBigint2 = BigIntHelper::SetBigInt(thread, resBigintStr2);
    JSHandle<BigInt> resBigint3 = BigIntHelper::SetBigInt(thread, resBigintStr3);
    JSHandle<BigInt> resBigint4 = BigIntHelper::SetBigInt(thread, resBigintStr4);

    // Exponentiate
    JSHandle<BigInt> expRes1 = BigInt::Exponentiate(thread, baseBigint, expBigint1);
    EXPECT_TRUE(BigInt::Equal(expRes1.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> expRes2 = BigInt::Exponentiate(thread, baseBigint, expBigint2);
    EXPECT_TRUE(BigInt::Equal(expRes2.GetTaggedValue(), resBigint2.GetTaggedValue()));

    // Multiply
    JSHandle<BigInt> mulRes1 = BigInt::Multiply(thread, baseBigint, baseBigint);
    for (int32_t i = 0; i < atoi(expBigintStr1.c_str()) - 2; i++) {
        mulRes1 = BigInt::Multiply(thread, mulRes1, baseBigint);
    }
    EXPECT_TRUE(BigInt::Equal(mulRes1.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> mulRes2 = BigInt::Multiply(thread, baseBigint, baseBigint);
    for (int32_t i = 0; i < atoi(expBigintStr2.c_str()) - 2; i++) {
        mulRes2 = BigInt::Multiply(thread, mulRes2, baseBigint);
    }
    EXPECT_TRUE(BigInt::Equal(mulRes2.GetTaggedValue(), resBigint2.GetTaggedValue()));
    JSHandle<BigInt> mulRes3 = BigInt::Multiply(thread, resBigint1, resBigint2);
    EXPECT_TRUE(BigInt::Equal(mulRes3.GetTaggedValue(), resBigint3.GetTaggedValue()));

    // Divide
    // The result has no remainder.
    JSHandle<BigInt> divRes1 = BigInt::Divide(thread, resBigint3, resBigint2);
    EXPECT_TRUE(BigInt::Equal(divRes1.GetTaggedValue(), resBigint1.GetTaggedValue()));
    JSHandle<BigInt> divRes2 = BigInt::Divide(thread, resBigint3, resBigint1);
    EXPECT_TRUE(BigInt::Equal(divRes2.GetTaggedValue(), resBigint2.GetTaggedValue()));
    // The result has a remainder.
    JSHandle<BigInt> divRes3 = BigInt::Divide(thread, resBigint4, resBigint1);
    EXPECT_TRUE(BigInt::Equal(divRes3.GetTaggedValue(), resBigint2.GetTaggedValue()));
    JSHandle<BigInt> divRes4 = BigInt::Divide(thread, resBigint4, resBigint2);
    EXPECT_TRUE(BigInt::Equal(divRes4.GetTaggedValue(), resBigint1.GetTaggedValue()));

    // Remainder
    JSHandle<BigInt> remRes1 = BigInt::Remainder(thread, resBigint4, resBigint1);
    EXPECT_TRUE(BigInt::Equal(remRes1.GetTaggedValue(), expBigint2.GetTaggedValue()));
    JSHandle<BigInt> remRes2 = BigInt::Remainder(thread, resBigint4, resBigint2);
    EXPECT_TRUE(BigInt::Equal(remRes2.GetTaggedValue(), expBigint2.GetTaggedValue()));
    JSHandle<BigInt> remRes3 = BigInt::Remainder(thread, resBigint4, resBigint3);
    EXPECT_TRUE(BigInt::Equal(remRes3.GetTaggedValue(), expBigint2.GetTaggedValue()));
}
} // namespace panda::test