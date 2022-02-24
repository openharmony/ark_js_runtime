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

#ifndef ECMASCRIPT_JS_BIGINT_H
#define ECMASCRIPT_JS_BIGINT_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_handle.h"
#include "js_object.h"

namespace panda::ecmascript {
enum class Operate : uint32_t { AND = 0, OR, XOR };
enum class Comparestr : uint32_t { EQUAL = 0, GREATER, LESS };

class BigInt : public TaggedObject {
public:
    Comparestr static ComString(std::string a, std::string b);
    static constexpr uint32_t DATEBITS = sizeof(uint32_t) * 8; // 8 : one-bit number of bytes
    static constexpr uint32_t MAXBITS = 1024 * 1024; // 1024 * 1024 : Maximum space that can be opened up
    static constexpr uint32_t MAXSIZE = MAXBITS / DATEBITS; // the maximum value of size
    static constexpr uint32_t MAXOCTALVALUE = 7; // 7 : max octal value
    static constexpr uint32_t BINARY = 2; // 2 : binary

    static constexpr uint32_t OCTAL = 8; // 8 : octal
    static constexpr uint32_t DECIMAL = 10; // 10 : decimal
    static constexpr uint32_t HEXADECIMAL = 16; // 16 : hexadecimal
    CAST_CHECK(BigInt, IsBigInt);
    static JSHandle<BigInt> CreateBigint(JSThread *thread, uint32_t size);

    static bool Equal(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool Equal(const BigInt *x, const BigInt *y);
    static bool SameValue(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y);

    static JSHandle<BigInt> BitwiseOp(JSThread *thread, Operate op, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseAND(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseXOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<EcmaString> ToString(JSThread *thread, JSHandle<BigInt> bigint,
                                         uint32_t conversionToRadix = BigInt::DECIMAL);

    static JSHandle<BigInt> UnaryMinus(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> BitwiseNOT(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> Exponentiate(JSThread *thread, JSHandle<BigInt> base, JSHandle<BigInt> exponent);
    static JSHandle<BigInt> Multiply(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> Divide(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> Remainder(JSThread *thread, JSHandle<BigInt> n, JSHandle<BigInt> d);
    static JSHandle<BigInt> BigintAddOne(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> BigintSubOne(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> copy(JSThread *thread, JSHandle<BigInt> x);

    static JSHandle<BigInt> Add(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> Subtract(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static bool LessThan(JSThread *thread, const JSTaggedValue &x, const JSTaggedValue &y);
    static bool LessThan(JSThread *thread, const JSHandle<BigInt> &x, const JSHandle<BigInt> &y);
    static ComparisonResult Compare(JSThread *thread, JSTaggedValue x, JSTaggedValue y);
    static JSHandle<BigInt> SignedRightShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> RightShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSTaggedValue UnsignedRightShift(JSThread *thread);
    static JSHandle<BigInt> LeftShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> LeftShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BigintAdd(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign);
    static JSHandle<BigInt> BigintSub(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign);

    static JSTaggedValue NumberToBigInt(JSThread *thread, JSHandle<JSTaggedValue> number);
    static JSHandle<BigInt> Int32ToBigInt(JSThread *thread, const int &number);
    static JSTaggedValue StringToBigInt(JSThread *thread, std::string bigintStr);
    static JSHandle<BigInt> FloorMod(JSThread *thread, JSHandle<BigInt> leftVal, JSHandle<BigInt> rightVal);
    static JSTaggedValue AsUintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint);
    static JSTaggedValue AsintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint);
    static JSTaggedNumber BigIntToNumber(JSThread *thread, JSHandle<BigInt> bigint);
    inline bool IsZero()
    {
        return GetLength() == 1 && !GetDigit(0);
    }

    uint32_t GetDigit(uint32_t index) const;
    static void SetDigit(JSThread* thread, JSHandle<BigInt> bigint, uint32_t index, uint32_t digit);

    uint32_t GetLength() const;

    static constexpr size_t DATA_OFFSET = TaggedObjectSize();
    ACCESSORS(Data, DATA_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t SIGN_BITS = 1;
    FIRST_BIT_FIELD(BitField, Sign, bool, SIGN_BITS)

    DECL_VISIT_OBJECT(DATA_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
};

class BigIntHelper {
public:
    static std::string Conversion(std::string num, uint32_t conversionToRadix, uint32_t currentRadix);
    static JSHandle<BigInt> SetBigInt(JSThread *thread, std::string numStr, uint32_t currentRadix = BigInt::DECIMAL);
    static std::string GetBinary(JSThread *thread, JSHandle<BigInt> bigint);
    static JSHandle<BigInt> RightTruncate(JSThread *thread, JSHandle<BigInt> x);
    static bool IsNaN(const std::string &str);
    static bool IsHexString(std::string &str);
    static bool IsBinaryString(std::string &str);
    static bool IsOctalString(std::string &str);

    static JSHandle<BigInt> DivideImpl(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> RemainderImpl(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static std::string MultiplyImpl(std::string a, std::string b);
    static std::string DeZero(std::string a);
    static std::string Minus(std::string a, std::string b);
    static std::string Divide(std::string a, std::string b);
    static std::string Mod(std::string a, std::string b);

    static uint32_t AddHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry);
    static uint32_t SubHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_BIGINT_H