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

#include "ecmascript/js_bigint.h"
#include "ecmascript/tagged_array-inl.h"
#include "object_factory.h"
#include "ecmascript/base/number_helper.h"
namespace panda::ecmascript {
class ObjectFactory;
int CharToInt(char c)
{
    int res = 0;
    if (c >= '0' && c <= '9') {
        res = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        res = c - 'A' + 10; // 10:res must Greater than 10.
    } else if (c >= 'a' && c <= 'z') {
        res = c - 'a' + 10; // 10:res must Greater than 10
    }
    return res;
}

std::string Division(std::string num, uint32_t conversionToRadix, uint32_t currentRadix, uint32_t &remain)
{
    ASSERT(conversionToRadix != 0);
    std::string dp = "0123456789abcdefghijklmnopqrstuvwxyz";
    int temp = 0;
    remain = 0;
    for (size_t i = 0; i < num.size(); i++) {
        temp = (currentRadix * remain + CharToInt(num[i]));
        num[i] = dp[temp / conversionToRadix];
        remain = temp % conversionToRadix;
    }
    int count = 0;
    while (num[count] == '0') {
        count++;
    }
    return num.substr(count);
}
std::string BigIntHelper::Conversion(std::string num, uint32_t conversionToRadix, uint32_t currentRadix)
{
    ASSERT(conversionToRadix != 0);
    std::string dp = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string res;
    uint32_t remain = 0;
    while (num.size() != 0) {
        num = Division(num, conversionToRadix, currentRadix, remain);
        res = dp[remain] + res;
    }
    return res;
}

JSHandle<BigInt> BigIntHelper::SetBigInt(JSThread *thread, std::string numStr, uint32_t currentRadix)
{
    int flag = 0;
    if (numStr[0] == '-') {
        flag = 1;
    }

    std::string binaryStr = "";
    if (currentRadix != BigInt::BINARY) {
        binaryStr = Conversion(numStr.substr(flag), BigInt::BINARY, currentRadix);
    } else {
        binaryStr = numStr.substr(flag);
    }

    JSHandle<BigInt> bigint;
    size_t len = binaryStr.size() / BigInt::DATEBITS;
    size_t mod = binaryStr.size() % BigInt::DATEBITS;
    int index = 0;
    if (mod == 0) {
        index = len - 1;
        bigint = BigInt::CreateBigint(thread, len);
    } else {
        len++;
        index = len - 1;
        bigint = BigInt::CreateBigint(thread, len);
        uint32_t val = 0;
        for (size_t i = 0; i < mod; ++i) {
            val <<= 1;
            val |= binaryStr[i] - '0';
        }
        BigInt::SetDigit(thread, bigint, index, val);
        index--;
    }
    if (flag == 1) {
        bigint->SetSign(true);
    }
    size_t i = mod;
    while (i < binaryStr.size()) {
        uint32_t val = 0;
        for (size_t j = 0; j < BigInt::DATEBITS && i < binaryStr.size(); ++j, ++i) {
            val <<= 1;
            val |= binaryStr[i] - '0';
        }
        BigInt::SetDigit(thread, bigint, index, val);
        index--;
    }
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigIntHelper::RightTruncate(JSThread *thread, JSHandle<BigInt> x)
{
    int len  = x->GetLength();
    ASSERT(len != 0);
    if (len == 1 && x->GetDigit(0) == 0) {
        x->SetSign(false);
        return x;
    }
    int index = len - 1;
    if (x->GetDigit(index) != 0) {
        return x;
    }
    while (index >= 0) {
        if (x->GetDigit(index) != 0) {
            break;
        }
        index--;
    }
    JSHandle<JSTaggedValue> value(thread, x->GetData());
    JSHandle<TaggedArray> array = JSHandle<TaggedArray>::Cast(value);
    if (index == -1) {
        array->Trim(thread, 1);
    } else {
        array->Trim(thread, index + 1);
    }
    if (x->IsZero()) {
        x->SetSign(false);
    }
    return x;
}

bool BigIntHelper::IsNaN(const std::string &str)
{
    std::string regular = "0123456789";
    size_t len = str.size();
    for (size_t i = 0; i < len; ++i) {
        if (regular.find(str[i]) == std::string::npos) {
            return true;
        }
    }
    return false;
}

bool BigIntHelper::IsHexString(std::string &str)
{
    std::string regular = "0123456789abcdefABCDEF";
    if (!(str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))) { // 2:minlen
        return false;
    }
    std::string numStr = str.substr(2); // 2:dump 0x
    size_t len = numStr.size();
    for (size_t i = 0; i < len; ++i) {
        if (regular.find(numStr[i]) == std::string::npos) {
            return false;
        }
    }
    return true;
}

bool BigIntHelper::IsBinaryString(std::string &str)
{
    if (!(str.size() > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B'))) { // 2:minlen
        return false;
    }
    std::string numStr = str.substr(2); // 2:dump 0b
    size_t len = numStr.size();
    for (size_t i = 0; i < len; ++i) {
        if (numStr[i] < '0' || numStr[i] > '1') {
            return false;
        }
    }
    return true;
}

bool BigIntHelper::IsOctalString(std::string &str)
{
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'o' || str[1] == 'O')) { // 2:minlen
        std::string numStr = str.substr(2); // 2:dump 0o
        size_t len = numStr.size();
        for (size_t i = 0; i < len; ++i) {
            if (numStr[i] < '0' || numStr[i] > (BigInt::MAXOCTALVALUE + '0')) {
                return false;
            }
        }
        return true;
    }
    return false;
}

std::string BigIntHelper::GetBinary(JSThread *thread, JSHandle<BigInt> bigint)
{
    int index = 0;
    int len = bigint->GetLength();
    int strLen = BigInt::DATEBITS * len;
    std::string res(strLen, '0');
    int strIndex = strLen - 1;
    while (index < len) {
        int bityLen = BigInt::DATEBITS;
        uint32_t val = bigint->GetDigit(index);
        while (bityLen--) {
            res[strIndex--] = (val & 1) + '0';
            val = val >> 1;
        }
        index++;
    }
    size_t count = 0;
    for (size_t i = 0; i < res.size(); ++i) {
        if (res[i] != '0') {
            break;
        }
        count++;
    }
    if (count == res.size()) {
        return "0";
    }
    return res.substr(count);
}

uint32_t BigInt::GetDigit(uint32_t index) const
{
    TaggedArray *TaggedArray = TaggedArray::Cast(GetData().GetTaggedObject());
    JSTaggedValue digit = TaggedArray->Get(index);
    return static_cast<uint32_t>(digit.GetInt());
}

void BigInt::SetDigit(JSThread* thread, JSHandle<BigInt> bigint, uint32_t index, uint32_t digit)
{
    TaggedArray *TaggedArray = TaggedArray::Cast(bigint->GetData().GetTaggedObject());
    TaggedArray->Set(thread, index, JSTaggedValue(static_cast<int32_t>(digit)));
}

uint32_t BigInt::GetLength() const
{
    TaggedArray *TaggedArray = TaggedArray::Cast(GetData().GetTaggedObject());
    return TaggedArray->GetLength();
}

JSHandle<BigInt> BigInt::CreateBigint(JSThread *thread, uint32_t size)
{
    ASSERT(size < MAXSIZE);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<BigInt> bigint = factory->NewBigInt();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(size);
    bigint->SetData(thread, JSHandle<JSTaggedValue>::Cast(taggedArray));
    return bigint;
}

bool BigInt::Equal(const JSTaggedValue &x, const JSTaggedValue &y)
{
    // 6.1.6.2.13
    BigInt* xVal = BigInt::Cast(x.GetTaggedObject());
    BigInt* yVal = BigInt::Cast(y.GetTaggedObject());
    return Equal(xVal, yVal);
}

bool BigInt::Equal(const BigInt *x, const BigInt *y)
{
    if (x->GetSign() != y->GetSign() || x->GetLength() != y->GetLength()) {
        return false;
    }
    for (uint32_t i = 0; i < x->GetLength(); ++i) {
        if (x->GetDigit(i) != y->GetDigit(i)) {
            return false;
        }
    }
    return true;
}

bool BigInt::SameValue(const JSTaggedValue &x, const JSTaggedValue &y)
{
    // 6.1.6.2.14
    return Equal(x, y);
}

bool BigInt::SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y)
{
    // 6.1.6.2.15
    return Equal(x, y);
}

JSHandle<BigInt> BigInt::BitwiseOp(JSThread *thread, Operate op, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    uint32_t maxLen = 0;
    uint32_t minLen = 0;
    uint32_t xlen = x->GetLength();
    uint32_t ylen = y->GetLength();
    if (xlen > ylen) {
        maxLen = xlen;
        minLen = ylen;
    } else {
        maxLen = ylen;
        minLen = xlen;
    }
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, maxLen);
    for (size_t i = 0; i < minLen; ++i) {
        if (op == Operate::OR) {
            SetDigit(thread, bigint, i, x->GetDigit(i) | y->GetDigit(i));
        } else if (op == Operate::AND) {
            SetDigit(thread, bigint, i, x->GetDigit(i) & y->GetDigit(i));
        } else {
            ASSERT(op == Operate::XOR);
            SetDigit(thread, bigint, i, x->GetDigit(i) ^ y->GetDigit(i));
        }
    }
    if (op == Operate::OR || op == Operate::XOR) {
        if (xlen > ylen) {
            for (size_t i = ylen; i < xlen; ++i) {
                SetDigit(thread, bigint, i, x->GetDigit(i));
            }
        } else if (ylen > xlen) {
            for (size_t i = xlen; i < ylen; ++i) {
                SetDigit(thread, bigint, i, y->GetDigit(i));
            }
        }
    }
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> OneIsNegativeAND(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    y->SetSign(false);
    JSHandle<BigInt> yVal = BigInt::BigintSubOne(thread, y);
    y->SetSign(true);
    uint32_t xLength = x->GetLength();
    uint32_t yLength = yVal->GetLength();
    uint32_t minLen = xLength;
    if (xLength > yLength) {
        minLen = yLength;
    }
    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, xLength);
    uint32_t i = 0;
    while (i < minLen) {
        uint32_t res = x->GetDigit(i) & ~(yVal->GetDigit(i));
        BigInt::SetDigit(thread, newBigint, i, res);
        ++i;
    }
    while (i < xLength) {
        BigInt::SetDigit(thread, newBigint, i, x->GetDigit(i));
        ++i;
    }
    return BigIntHelper::RightTruncate(thread, newBigint);
}

JSHandle<BigInt> BigInt::BitwiseAND(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    // 6.1.6.2.20 BigInt::bitwiseAND ( x, y )
    if (x->GetSign() && y->GetSign()) {
        // (-x) & (-y) == -(((x-1) | (y-1)) + 1)
        x->SetSign(false);
        y->SetSign(false);
        JSHandle<BigInt> xVal = BigintSubOne(thread, x);
        JSHandle<BigInt> yVal = BigintSubOne(thread, y);
        x->SetSign(true);
        y->SetSign(true);
        JSHandle<BigInt> temp = BitwiseOp(thread, Operate::OR, xVal, yVal);
        JSHandle<BigInt> res = BigintAddOne(thread, temp);
        res->SetSign(true);
        return res;
    }
    if (x->GetSign() != y->GetSign()) {
        // x & (-y) == x & ~(y-1)
        if (!x->GetSign()) {
            return OneIsNegativeAND(thread, x, y);
        } else {
            return OneIsNegativeAND(thread, y, x);
        }
    }
    return BitwiseOp(thread, Operate::AND, x, y);
}

JSHandle<BigInt> OneIsNegativeXOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    y->SetSign(false);
    JSHandle<BigInt> yVal = BigInt::BigintSubOne(thread, y);
    y->SetSign(true);
    JSHandle<BigInt> temp = BigInt::BitwiseOp(thread, Operate::XOR, x, yVal);
    JSHandle<BigInt> res = BigInt::BigintAddOne(thread, temp);
    res->SetSign(true);
    return res;
}

JSHandle<BigInt> BigInt::BitwiseXOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->GetSign() && y->GetSign()) {
        // (-x) ^ (-y) == (x-1) ^ (y-1)
        x->SetSign(false);
        y->SetSign(false);
        JSHandle<BigInt> xVal = BigintSubOne(thread, x);
        JSHandle<BigInt> yVal = BigintSubOne(thread, y);
        x->SetSign(true);
        y->SetSign(true);
        return BitwiseOp(thread, Operate::XOR, xVal, yVal);
    }
    if (x->GetSign() != y->GetSign()) {
        // x ^ (-y) == -((x ^ (y-1)) + 1)
        if (!x->GetSign()) {
            return OneIsNegativeXOR(thread, x, y);
        } else {
            return OneIsNegativeXOR(thread, y, x);
        }
    }
    return BitwiseOp(thread, Operate::XOR, x, y);
}

JSHandle<BigInt> OneIsNegativeOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    y->SetSign(false);
    JSHandle<BigInt> yVal = BigInt::BigintSubOne(thread, y);
    y->SetSign(true);
    uint32_t xLength = x->GetLength();
    uint32_t yLength = yVal->GetLength();
    uint32_t minLen = xLength;
    if (xLength > yLength) {
        minLen = yLength;
    }
    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, xLength);
    uint32_t i = 0;
    while (i < minLen) {
        uint32_t res = ~(x->GetDigit(i)) & yVal->GetDigit(i);
        BigInt::SetDigit(thread, newBigint, i, res);
        ++i;
    }
    while (i < xLength) {
        BigInt::SetDigit(thread, newBigint, i, x->GetDigit(i));
        ++i;
    }
    JSHandle<BigInt> temp = BigIntHelper::RightTruncate(thread, newBigint);
    JSHandle<BigInt> res = BigInt::BigintAddOne(thread, temp);
    res->SetSign(true);
    return res;
}

JSHandle<BigInt> BigInt::BitwiseOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    // 6.1.6.2.22 BigInt::bitwiseOR ( x, y )
    if (x->GetSign() && y->GetSign()) {
        // (-x) | (-y) == -(((x-1) & (y-1)) + 1)
        x->SetSign(false);
        y->SetSign(false);
        JSHandle<BigInt> xVal = BigintSubOne(thread, x);
        JSHandle<BigInt> yVal = BigintSubOne(thread, y);
        x->SetSign(true);
        y->SetSign(true);
        JSHandle<BigInt> temp = BitwiseOp(thread, Operate::AND, xVal, yVal);
        JSHandle<BigInt> res = BigintAddOne(thread, temp);
        res->SetSign(true);
        return res;
    }
    if (x->GetSign() != y->GetSign()) {
        // x | (-y) == -(((y-1) & ~x) + 1)
        if (!x->GetSign()) {
            return OneIsNegativeOR(thread, x, y);
        } else {
            return OneIsNegativeOR(thread, y, x);
        }
    }
    return BitwiseOp(thread, Operate::OR, x, y);
}

JSHandle<EcmaString> BigInt::ToString(JSThread *thread, JSHandle<BigInt> bigint, uint32_t conversionToRadix)
{
    // 6.1.6.2.23 BigInt::toString ( x )
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::string result =
        BigIntHelper::Conversion(BigIntHelper::GetBinary(thread, bigint), conversionToRadix, BigInt::BINARY);
    if (bigint->GetSign() && !(result.size() == 1 && result[0] == '0')) {
        result = "-" + result;
    }
    return factory->NewFromStdStringUnCheck(result, true);
}

JSTaggedValue BigInt::NumberToBigInt(JSThread *thread, JSHandle<JSTaggedValue> number)
{
    JSHandle<EcmaString> strNum = JSTaggedValue::ToString(thread, number);
    std::string str = strNum->GetCString().get();
    if (!number->IsInteger()) {
        std::string err = "The number " + str + " cannot be converted to a BigInt because it is not an integer";
        THROW_RANGE_ERROR_AND_RETURN(thread, err.c_str(), JSTaggedValue::Exception());
    }
    return BigIntHelper::SetBigInt(thread, str).GetTaggedValue();
}

JSHandle<BigInt> BigInt::Int32ToBigInt(JSThread *thread, const int &number)
{
    return BigIntHelper::SetBigInt(thread, std::to_string(number));
}

JSTaggedValue BigInt::StringToBigInt(JSThread *thread, std::string str)
{
    size_t proSpace = 0;
    size_t endSpace = str.size() - 1;
    while (proSpace <= endSpace) {
        if (str[proSpace] != ' ') {
            break;
        }
        proSpace++;
    }
    while (proSpace <= endSpace) {
        if (str[endSpace] != ' ') {
            break;
        }
        endSpace--;
    }
    std::string bigintStr = str.substr(proSpace, endSpace - proSpace + 1);
    if (bigintStr.empty()) {
        return Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    bool hasSign = false;
    std::string sign = "";
    if (bigintStr[0] == '-') {
        hasSign = true;
        sign = "-";
        bigintStr = bigintStr.substr(1);
    } else if (bigintStr[0] == '+') {
        hasSign = true;
        bigintStr = bigintStr.substr(1);
    }
    if (!hasSign && BigIntHelper::IsHexString(bigintStr)) {
        return BigIntHelper::SetBigInt(thread, sign + bigintStr.substr(2), HEXADECIMAL).GetTaggedValue(); // 2:dump 0x
    } else if (!hasSign && BigIntHelper::IsBinaryString(bigintStr)) {
        return BigIntHelper::SetBigInt(thread, sign + bigintStr.substr(2), BINARY).GetTaggedValue(); // 2:dump 0b
    } else if (!hasSign && BigIntHelper::IsOctalString(bigintStr)) {
        return BigIntHelper::SetBigInt(thread, sign + bigintStr.substr(2), OCTAL).GetTaggedValue(); // 2:dump 0o
    } else if (BigIntHelper::IsNaN(bigintStr)) {
        return JSTaggedValue(base::NAN_VALUE);
    } else {
        return BigIntHelper::SetBigInt(thread, sign + bigintStr).GetTaggedValue();
    }
}

JSHandle<BigInt> BigInt::Add(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool xSignFlag = x->GetSign();
    bool ySignFlag = y->GetSign();
    // x + y == x + y
    // -x + -y == -(x + y)
    if (xSignFlag == ySignFlag) {
        return BigintAdd(thread, x, y, xSignFlag);
    }
    // x + -y == x - y == -(y - x)
    // -x + y == y - x == -(x - y)
    uint32_t xLength = x->GetLength();
    uint32_t yLength = y->GetLength();
    uint32_t i = xLength - 1;
    int subSize = xLength - yLength;
    if (subSize > 0) {
        return BigintSub(thread, x, y, xSignFlag);
    } else if (subSize == 0) {
        while (i > 0 && x->GetDigit(i) == y->GetDigit(i)) {
            i--;
        }
        if (i < 0) {
            return BigintSub(thread, x, y, xSignFlag);
        }
        if ((x->GetDigit(i) > y->GetDigit(i))) {
            return BigintSub(thread, x, y, xSignFlag);
        } else {
            return BigintSub(thread, y, x, ySignFlag);
        }
    } else {
        return BigintSub(thread, y, x, ySignFlag);
    }
}
JSHandle<BigInt> BigInt::Subtract(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool xSignFlag = x->GetSign();
    bool ySignFlag = y->GetSign();
    if (xSignFlag != ySignFlag) {
            // x - (-y) == x + y
            // (-x) - y == -(x + y)
        return BigintAdd(thread, x, y, xSignFlag);
    }
    // x - y == -(y - x)
    // (-x) - (-y) == y - x == -(x - y)
    uint32_t xLength = x->GetLength();
    uint32_t yLength = y->GetLength();
    uint32_t i = xLength - 1;
    int subSize = xLength - yLength;
    if (subSize > 0) {
        return BigintSub(thread, x, y, xSignFlag);
    } else if (subSize == 0) {
        while (i > 0 && x->GetDigit(i) == y->GetDigit(i)) {
            i--;
        }
        if (i < 0) {
            return BigintSub(thread, x, y, xSignFlag);
        }
        if ((x->GetDigit(i) > y->GetDigit(i))) {
            return BigintSub(thread, x, y, xSignFlag);
        } else {
            return BigintSub(thread, y, x, !ySignFlag);
        }
    } else {
        return BigintSub(thread, y, x, !ySignFlag);
    }
}

JSHandle<BigInt> BigInt::BigintAdd(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign)
{
    if (x->GetLength() < y->GetLength()) {
        return BigintAdd(thread, y, x, resultSign);
    }
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, x->GetLength() + 1);
    uint32_t bigintCarry = 0;
    uint32_t i = 0;
    while (i < y->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t addPlus = BigIntHelper::AddHelper(x->GetDigit(i), y->GetDigit(i), newBigintCarry);
        addPlus = BigIntHelper::AddHelper(addPlus, bigintCarry, newBigintCarry);
        SetDigit(thread, bigint, i, addPlus);
        bigintCarry = newBigintCarry;
        i++;
    }
    while (i < x->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t addPlus = BigIntHelper::AddHelper(x->GetDigit(i), bigintCarry, newBigintCarry);
        SetDigit(thread, bigint, i, addPlus);
        bigintCarry = newBigintCarry;
        i++;
    }
    SetDigit(thread, bigint, i, bigintCarry);
    bigint->SetSign(resultSign);
    return BigIntHelper::RightTruncate(thread, bigint);
}

inline uint32_t BigIntHelper::AddHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry)
{
    uint32_t addPlus = x + y;
    if (addPlus < x) {
        bigintCarry += 1;
    }
    return addPlus;
}

JSHandle<BigInt> BigInt::BigintSub(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign)
{
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, x->GetLength());
    uint32_t bigintCarry = 0;
    uint32_t i = 0;
    while (i < y->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t minuSub = BigIntHelper::SubHelper(x->GetDigit(i), y->GetDigit(i), newBigintCarry);
        minuSub = BigIntHelper::SubHelper(minuSub, bigintCarry, newBigintCarry);
        SetDigit(thread, bigint, i, minuSub);
        bigintCarry = newBigintCarry;
        i++;
    }
    while (i < x->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t minuSub = BigIntHelper::SubHelper(x->GetDigit(i), bigintCarry, newBigintCarry);
        SetDigit(thread, bigint, i, minuSub);
        bigintCarry = newBigintCarry;
        i++;
    }
    bigint->SetSign(resultSign);
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigInt::BigintAddOne(JSThread *thread, JSHandle<BigInt> x)
{
    JSHandle<BigInt> temp = Int32ToBigInt(thread, 1);
    return Add(thread, x, temp);
}

JSHandle<BigInt> BigInt::BigintSubOne(JSThread *thread, JSHandle<BigInt> x)
{
    JSHandle<BigInt> temp = Int32ToBigInt(thread, 1);
    return Subtract(thread, x, temp);
}

inline uint32_t BigIntHelper::SubHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry)
{
    uint32_t minuSub = x - y;
    if (minuSub > x) {
        bigintCarry += 1;
    }
    return minuSub;
}

ComparisonResult BigInt::Compare(JSThread *thread, JSTaggedValue x, JSTaggedValue y)
{
    if (!LessThan(thread, x, y)) {
        if (!LessThan(thread, y, x)) {
            return ComparisonResult::EQUAL;
        }
        return ComparisonResult::GREAT;
    }
    return ComparisonResult::LESS;
}

bool BigInt::LessThan(JSThread *thread, const JSTaggedValue &x, const JSTaggedValue &y)
{
    JSHandle<BigInt> bigLeft(thread, x);
    JSHandle<BigInt> bigRight(thread, y);
    return LessThan(thread, bigLeft, bigRight);
}

bool BigInt::LessThan(JSThread *thread, const JSHandle<BigInt> &x, const JSHandle<BigInt> &y)
{
    bool xSignFlag = x->GetSign();
    bool ySignFlag = y->GetSign();
    int minSize = x->GetLength() - y->GetLength();
    uint32_t i = x->GetLength() - 1;
    if (xSignFlag != ySignFlag) {
        return xSignFlag ? true : false;
    } else {
        if (minSize != 0 && xSignFlag) {
            return minSize > 0 ? true : false;
        }
        if (minSize != 0 && !xSignFlag) {
            return minSize > 0 ? false : true;
        }
        while (i > 0 && x->GetDigit(i) == y->GetDigit(i)) {
            i--;
        }
        if (i < 0) {
            return false;
        }
        if ((x->GetDigit(i) > y->GetDigit(i))) {
            return xSignFlag ? true : false;
        } else if ((x->GetDigit(i) < y->GetDigit(i))) {
            return !xSignFlag ? true : false;
        } else {
            return false;
        }
    }
}

JSHandle<BigInt> BigInt::SignedRightShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool xIsNull = x->GetDigit(0);
    bool yIsNull = y->GetDigit(0);
    if (!xIsNull || !yIsNull) {
        return x;
    }
    if (y->GetSign()) {
        return LeftShiftHelper(thread, x, y);
    } else {
        return RightShiftHelper(thread, x, y);
    }
}

JSHandle<BigInt> BigInt::RightShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool flag = (x->GetDigit(0) & 1);
    std::string shiftBinay = BigIntHelper::GetBinary(thread, x);
    std::string revTemp = std::string(shiftBinay.rbegin(), shiftBinay.rend());
    for (uint32_t i = 0; i < y->GetLength(); i++) {
        revTemp = revTemp.erase(0, y->GetDigit(i));
    }
    std::string finalBinay = std::string(revTemp.rbegin(), revTemp.rend());
    JSHandle<BigInt> bigint = BigIntHelper::SetBigInt(thread, finalBinay, BINARY);
    if (flag && x->GetSign()) {
        SetDigit(thread, bigint, 0, bigint->GetDigit(0) + 1);
    }
    bigint->SetSign(x->GetSign());
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigInt::LeftShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (y->GetSign()) {
        return RightShiftHelper(thread, x, y);
    } else {
        return LeftShiftHelper(thread, x, y);
    }
}

JSHandle<BigInt> BigInt::LeftShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    std::string shiftBinay = BigIntHelper::GetBinary(thread, x);
    for (size_t i = 0; i < y->GetLength(); i++) {
        shiftBinay = shiftBinay.append(y->GetDigit(i), '0');
    }
    JSHandle<BigInt> bigint = BigIntHelper::SetBigInt(thread, shiftBinay, BINARY);
    bigint->SetSign(x->GetSign());
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSTaggedValue BigInt::UnsignedRightShift(JSThread *thread)
{
    THROW_TYPE_ERROR_AND_RETURN(thread, "BigInts have no unsigned right shift, use >> instead",
                                JSTaggedValue::Exception());
}

JSHandle<BigInt> BigInt::copy(JSThread *thread, JSHandle<BigInt> x)
{
    uint32_t len = x->GetLength();
    JSHandle<BigInt> temp = CreateBigint(thread, len);
    for (uint32_t i = 0; i < len; i++) {
        SetDigit(thread, temp, i, x->GetDigit(i));
    }
    temp->SetSign(x->GetSign());
    return temp;
}

JSHandle<BigInt> BigInt::UnaryMinus(JSThread *thread, JSHandle<BigInt> x)
{
    if (x->IsZero()) {
        return x;
    }
    JSHandle<BigInt> y = copy(thread, x);
    y->SetSign(!y->GetSign());
    return y;
}

JSHandle<BigInt> BigInt::BitwiseNOT(JSThread *thread, JSHandle<BigInt> x)
{
    // 6.1.6.2.2   BigInt::bitwiseNOT ( x )
    // ~(-x) == ~(~(x-1)) == x-1
    // ~x == -x-1 == -(x+1)
    JSHandle<BigInt> result = BigintAddOne(thread, x);
    if (x->GetSign()) {
        result->SetSign(false);
    } else {
        result->SetSign(true);
    }
    return result;
}

JSHandle<BigInt> BigInt::Exponentiate(JSThread *thread, JSHandle<BigInt> base, JSHandle<BigInt> exponent)
{
    ASSERT(exponent->GetLength() == 1);
    if (exponent->GetSign()) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Exponent must be positive", bigint);
    }
    if (base->GetDigit(0) == 0 && base->GetLength() == 0) {
        return BigIntHelper::SetBigInt(thread, "1");
    }
    if (exponent->IsZero() && base->GetDigit(0) != 0) {
        return BigIntHelper::SetBigInt(thread, "1");
    }
    if (base->IsZero() && exponent->GetDigit(0) != 0) {
        return base;
    }
    uint32_t EValue = exponent->GetDigit(0);
    if (EValue == 1) {
        return base;
    }
    uint32_t j = exponent->GetDigit(0) - 1;
    std::string a = BigIntHelper::GetBinary(thread, base);
    a = BigIntHelper::Conversion(a, DECIMAL, BINARY);
    std::string b = a;
    for (uint32_t i = 0; i < j; ++i) {
        b = BigIntHelper::MultiplyImpl(b, a);
    }
    if (exponent->GetDigit(0) & 1) {
        if (base->GetSign()) {
            b = "-" + b;
        }
    }
    return BigIntHelper::SetBigInt(thread, b, DECIMAL);
}
std::string BigIntHelper::MultiplyImpl(std::string a, std::string b)
{
    int size1 = a.size();
    int size2 = b.size();
    std::string str(size1 + size2, '0');
    for (int i = size2 - 1; i >= 0; --i) {
        int mulflag = 0;
        int addflag = 0;
        for (int j = size1 - 1; j >= 0; --j) {
            int temp1 = (b[i] - '0') * (a[j] - '0') + mulflag;
            mulflag = temp1 / 10; // 10:help to Remove single digits
            temp1 = temp1 % 10; // 10:help to Take single digit
            int temp2 = str[i + j + 1] - '0' + temp1 + addflag;
            str[i + j + 1] = temp2 % 10 + 48; // 2 and 10 and 48 is number
            addflag = temp2 / 10;
        }
        str[i] += mulflag + addflag;
    }
    if (str[0] == '0') {
        str = str.substr(1, str.size());
    }
    return str;
}

JSHandle<BigInt> BigInt::Multiply(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->IsZero()) {
        return x;
    }
    if (y->IsZero()) {
        return y;
    }
    std::string left = BigIntHelper::GetBinary(thread, x);
    std::string right = BigIntHelper::GetBinary(thread, y);
    left = BigIntHelper::Conversion(left, DECIMAL, BINARY);
    right = BigIntHelper::Conversion(right, DECIMAL, BINARY);
    std::string ab = BigIntHelper::MultiplyImpl(left, right);
    if (x->GetSign() != y->GetSign()) {
        ab = "-" + ab;
    }
    return BigIntHelper::SetBigInt(thread, ab, DECIMAL);
}

std::string BigIntHelper::DeZero(std::string a)
{
    size_t i;
    for (i = 0; i < a.length(); i++) {
        if (a.at(i) > 48) { // 48 is ascill of '0'
            break;
        }
    }
    if (i == a.length()) {
        return "0";
    }
    a.erase(0, i);
    return a;
}
Comparestr BigInt::ComString(std::string a, std::string b)
{
    if (a.length() > b.length()) {
        return Comparestr::GREATER;
    }
    if (a.length() < b.length()) {
        return Comparestr::LESS;
    }
    size_t i;
    for (i = 0; i < a.length(); i++) {
        if (a.at(i) > b.at(i)) {
            return Comparestr::GREATER;
        }
        if (a.at(i) < b.at(i)) {
            return Comparestr::LESS;
        }
    }
    return Comparestr::EQUAL;
}

std::string BigIntHelper::Minus(std::string a, std::string b)
{
    a = DeZero(a);
    b = DeZero(b);
    size_t i = 0;
    int j = 0;
    std::string res = "0";
    std::string result1;
    std::string result2;
    std::string dsymbol = "-";
    if (BigInt::ComString(a, b) == Comparestr::EQUAL) {
        return res;
    }
    if (BigInt::ComString(a, b) == Comparestr::GREATER) {
        result1 = a;
        result2 = b;
    }
    if (BigInt::ComString(a, b) == Comparestr::LESS) {
        result1 = b;
        result2 = a;
        j = -1;
    }
    reverse(result1.begin(), result1.end());
    reverse(result2.begin(), result2.end());
    for (i = 0; i < result2.length(); i++) {
        if (result2.at(i) >= 48 && result2.at(i) <= 57) { // 48 and 57 is '0' and '9'
            result2.at(i) -= 48; // 48:'0'
        }
        if (result2.at(i) >= 97 && result2.at(i) <= 122) { // 97 and 122 is 'a' and 'z'
            result2.at(i) -= 87; // 87 control result is greater than 10
        }
    }
    for (i = 0;i < result1.length(); i++) {
        if (result1.at(i) >= 48 && result1.at(i) <= 57) { // 48 and 57 is '0' and '9'
            result1.at(i) -= 48; // 48:'0'
        }
        if (result1.at(i) >= 97 && result1.at(i) <= 122) { // 97 and 122 is 'a' and 'z'
            result1.at(i) -= 87; // 87:control result is greater than 10
        }
    }
    for (i = 0; i < result2.length(); i++) {
        result1.at(i) = result1.at(i) - result2.at(i);
    }
    for (i = 0; i < result1.length() - 1; i++) {
        if (result1.at(i) < 0) {
            result1.at(i) += BigInt::DECIMAL;
            result1.at(i + 1)--;
        }
    }
    for (i = result1.length() - 1; i >= 0; i--) {
        if (result1.at(i) > 0) {
            break;
        }
    }
    result1.erase(i + 1, result1.length());
    for (i = 0; i < result1.length(); i++) {
        if (result1.at(i) >= 10) { // 10:Hexadecimal a
            result1.at(i) += 87; // 87:control result is greater than 97
        }
        if (result1.at(i) < 10) { // 10: 10:Hexadecimal a
            result1.at(i) += 48; // 48:'0'
        }
    }
    reverse(result1.begin(), result1.end());
    if (j == -1) {
        result1.insert(0, dsymbol);
    }
    return result1;
}

std::string BigIntHelper::Divide(std::string a, std::string b)
{
    if (b.length() == 1 && b.at(0) == 48) { // 48 is number
        return "error";
    }
    size_t i = 0;
    size_t j = 0;
    std::string result1;
    std::string result2;
    std::string dsy;
    std::string quotient;
    if (BigInt::ComString(a, b) == Comparestr::EQUAL) {
        return "1";
    }
    if (BigInt::ComString(a, b) == Comparestr::LESS) {
        return "0";
    }
    result1 = DeZero(a);
    result2 = DeZero(b);
    dsy = "";
    quotient = "";
    for (i = 0; i < result1.length(); i++) {
        j = 0;
        dsy = dsy + result1.at(i);
        dsy = DeZero(dsy);
        while (BigInt::ComString(dsy, b) == Comparestr::EQUAL ||
               BigInt::ComString(dsy, b) == Comparestr::GREATER) {
            dsy = Minus(dsy, b);
            dsy = DeZero(dsy);
            j++;
        }
        quotient = quotient + "0";
        quotient.at(i) = j;
    }
    for (i = 0; i < quotient.length(); i++) {
        if (quotient.at(i) >= 10) { // 10 is number
            quotient.at(i) += 87; // 87 is number
        }
        if (quotient.at(i) < 10) { // 10 is number
            quotient.at(i) += 48; // 48 is number
        }
    }
    quotient = DeZero(quotient);
    return quotient;
}

std::string BigIntHelper::Mod(std::string a, std::string b)
{
    size_t i = 0;
    size_t j = 0;
    std::string result1;
    std::string result2;
    std::string result3;
    std::string remainder;
    if (a.at(0) == '-') {
        j = 1;
    }
    if (BigInt::ComString(a, b) == Comparestr::EQUAL) {
        return "0";
    }
    if (BigInt::ComString(a, b) == Comparestr::LESS) {
        return a;
    }
    result1 = DeZero(a);
    result2 = DeZero(b);
    remainder = "";
    for (i = 0; i < result1.length(); i++) {
        remainder = remainder + result1.at(i);
        while (BigInt::ComString(remainder, b) == Comparestr::EQUAL ||
               BigInt::ComString(remainder, b) == Comparestr::GREATER) {
            remainder = Minus(remainder, b);
            remainder = DeZero(remainder);
        }
    }
    if (j == 1) {
        remainder = Minus(b, remainder);
    }
    return remainder;
}

JSHandle<BigInt> BigIntHelper::DivideImpl(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    std::string a = Conversion(GetBinary(thread, x), BigInt::DECIMAL, BigInt::BINARY);
    std::string b = Conversion(GetBinary(thread, y), BigInt::DECIMAL, BigInt::BINARY);
    std::string ab = Divide(a, b);
    if (ab == "0") {
        ab = "0";
    } else if (x->GetSign() != y->GetSign()) {
        ab = "-" + ab;
    }
    return SetBigInt(thread, ab, BigInt::DECIMAL);
}

JSHandle<BigInt> BigInt::Divide(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (y->IsZero()) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Division by zero", bigint);
    }
    return BigIntHelper::DivideImpl(thread, x, y);
}

JSHandle<BigInt> BigIntHelper::RemainderImpl(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    std::string a = Conversion(GetBinary(thread, x), BigInt::DECIMAL, BigInt::BINARY);
    std::string b = Conversion(GetBinary(thread, y), BigInt::DECIMAL, BigInt::BINARY);
    std::string ab = Mod(a, b);
    if (ab == "0") {
        ab = "0";
    } else if (x->GetSign()) {
        ab = "-" + ab;
    }
    return SetBigInt(thread, ab, BigInt::DECIMAL);
}

JSHandle<BigInt> BigInt::Remainder(JSThread *thread, JSHandle<BigInt> n, JSHandle<BigInt> d)
{
    if (d->IsZero()) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Division by zero", bigint);
    }
    if (n->IsZero()) {
        return n;
    }
    return BigIntHelper::RemainderImpl(thread, n, d);
}

JSHandle<BigInt> BigInt::FloorMod(JSThread *thread, JSHandle<BigInt> leftVal, JSHandle<BigInt> rightVal)
{
    if (leftVal->GetSign()) {
        JSHandle<BigInt> quotientVal = Divide(thread, leftVal, rightVal);
        if (quotientVal->IsZero()) {
            return Add(thread, leftVal, rightVal);
        } else {
            JSHandle<BigInt> num = Multiply(thread, quotientVal, rightVal);
            if (Equal(num.GetTaggedValue(), leftVal.GetTaggedValue())) {
                return Int32ToBigInt(thread, 0);
            } else {
                return Subtract(thread, leftVal, Subtract(thread, num, rightVal));
            }
        }
    }
    return Remainder(thread, leftVal, rightVal);
}

JSTaggedValue BigInt::AsUintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint)
{
    uint32_t bit = bits.ToUint32();
    if (bit == 0) {
        return Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    if (bigint->IsZero()) {
        return bigint.GetTaggedValue();
    }
    JSHandle<BigInt> exponent = Int32ToBigInt(thread, bit);
    JSHandle<BigInt> base = Int32ToBigInt(thread, 2); // 2 : base value
    JSHandle<BigInt> tValue = Exponentiate(thread, base, exponent);
    return FloorMod(thread, bigint, tValue).GetTaggedValue();
}

JSTaggedValue BigInt::AsintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint)
{
    uint32_t bit = bits.ToUint32();
    if (bit == 0) {
        return Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    if (bigint->IsZero()) {
        return bigint.GetTaggedValue();
    }
    JSHandle<BigInt> exp = Int32ToBigInt(thread, bit);
    JSHandle<BigInt> exponent = Int32ToBigInt(thread, bit - 1);
    JSHandle<BigInt> base = Int32ToBigInt(thread, 2); // 2 : base value
    JSHandle<BigInt> tValue = Exponentiate(thread, base, exp);
    JSHandle<BigInt> modValue =  FloorMod(thread, bigint, tValue);
    JSHandle<BigInt> resValue = Exponentiate(thread, base, exponent);
    // If mod ≥ 2bits - 1, return ℤ(mod - 2bits); otherwise, return (mod).
    if (LessThan(thread, resValue, modValue) || Equal(resValue.GetTaggedValue(), modValue.GetTaggedValue())) {
        return Subtract(thread, modValue, tValue).GetTaggedValue();
    }
    return modValue.GetTaggedValue();
}

JSTaggedNumber BigInt::BigIntToNumber(JSThread *thread, JSHandle<BigInt> bigint)
{
    JSHandle<JSTaggedValue> bigintStr = JSHandle<JSTaggedValue>::Cast(ToString(thread, bigint));
    return JSTaggedValue::ToNumber(thread, bigintStr);
}
} // namespace