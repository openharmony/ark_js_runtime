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

#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BigInt = ecmascript::BigInt;
class BuiltinsBigIntTest : public testing::Test {
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
        JSRuntimeOptions options;
#if PANDA_TARGET_LINUX
        // for consistency requirement, use ohos_icu4j/data as icu-data-path
        options.SetIcuDataPath(ICU_PATH);
#endif
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootClassSpaces(
            {"ecmascript"}
        );
        options.SetRuntimeType("ecmascript");
        options.SetPreGcHeapVerifyEnabled(true);
        options.SetEnableForceGC(true);
        JSNApi::SetOptions(options);
        static EcmaLanguageContext lcEcma;
        [[maybe_unused]] bool success = Runtime::Create(options, {&lcEcma});
        ASSERT_TRUE(success) << "Cannot create Runtime";
        instance = Runtime::GetCurrent()->GetPandaVM();
        EcmaVM::Cast(instance)->SetEnableForceGC(true);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = EcmaVM::Cast(instance)->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

// new BigInt(123)
HWTEST_F_L0(BuiltinsBigIntTest, BigIntConstructor_001)
{
    JSHandle<JSTaggedValue> numericValue(thread, JSTaggedValue(123));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsBigInt());
}

// new BigInt("456")
HWTEST_F_L0(BuiltinsBigIntTest, BigIntConstructor_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("456"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsBigInt());
}

// AsIntN(64, (2 ^ 63 - 1))
HWTEST_F_L0(BuiltinsBigIntTest, AsIntN_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("9223372036854775807"));
    int bit = 64; // 64-bit

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(bit)));
    ecmaRuntimeCallInfo->SetCallArg(1, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsBigInt::AsIntN(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("9223372036854775807");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// AsIntN(64, (2 ^ 63))
HWTEST_F_L0(BuiltinsBigIntTest, AsIntN_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("9223372036854775808"));
    int bit = 64; // 64-bit

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(bit)));
    ecmaRuntimeCallInfo->SetCallArg(1, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsBigInt::AsIntN(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("-9223372036854775808");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// AsUintN(64, (2 ^ 64 - 1))
HWTEST_F_L0(BuiltinsBigIntTest, AsUintN_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("18446744073709551615"));
    int bit = 64; // 64-bit

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(bit)));
    ecmaRuntimeCallInfo->SetCallArg(1, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsBigInt::AsUintN(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("18446744073709551615");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// AsUintN(64, (2 ^ 64))
HWTEST_F_L0(BuiltinsBigIntTest, AsUintN_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("18446744073709551616"));
    int bit = 64; // 64-bit

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(bit)));
    ecmaRuntimeCallInfo->SetCallArg(1, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsBigInt::AsUintN(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsBigInt());
    JSHandle<BigInt> bigIntHandle(thread, result);
    JSHandle<EcmaString> resultStr = BigInt::ToString(thread, bigIntHandle);
    JSHandle<EcmaString> str = factory->NewFromASCII("0");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// using locale
HWTEST_F_L0(BuiltinsBigIntTest, ToLocaleString_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("123456789123456789"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue::Undefined());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ToLocaleString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    JSHandle<EcmaString> resultStr(factory->NewFromASCII("123.456.789.123.456.789"));
    EXPECT_EQ(ecmaStrHandle->Compare(*resultStr), 0);
}

// using locale and options
HWTEST_F_L0(BuiltinsBigIntTest, ToLocaleString_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("123456789123456789"));
    JSHandle<JSTaggedValue> formatStyle = thread->GlobalConstants()->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleKey(factory->NewFromASCII("currency"));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("EUR"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSObject::SetProperty(thread, optionsObj, formatStyle, styleKey);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, optionsObj.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ToLocaleString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("123.456.789.123.456.789,00 €", CString(ecmaStrHandle->GetCString().get()).c_str());
}

// 17.ToStirng()
HWTEST_F_L0(BuiltinsBigIntTest, ToString_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("17"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue::Undefined());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ToString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("17", CString(ecmaStrHandle->GetCString().get()).c_str());
}

// -0.ToStirng()
HWTEST_F_L0(BuiltinsBigIntTest, ToString_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("-0"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue::Undefined());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ToString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("0", CString(ecmaStrHandle->GetCString().get()).c_str());
}

// -10.ToStirng(2)
HWTEST_F_L0(BuiltinsBigIntTest, ToString_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("-10"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> radix(thread, JSTaggedValue(2));
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, radix.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ToString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("-1010", CString(ecmaStrHandle->GetCString().get()).c_str());
}

// 254.ToStirng(16)
HWTEST_F_L0(BuiltinsBigIntTest, ToString_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("254"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> radix(thread, JSTaggedValue(16));
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, radix.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ToString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result2);
    EXPECT_STREQ("fe", CString(ecmaStrHandle->GetCString().get()).c_str());
}

// BigInt.ValueOf
HWTEST_F_L0(BuiltinsBigIntTest, ValueOf_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("-65536"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(bigIntHandle.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ValueOf(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(BigInt::SameValue(result1, result2), true);
}

// Object.ValueOf
HWTEST_F_L0(BuiltinsBigIntTest, ValueOf_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("65535"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> bigIntObj(bigIntHandle);

    JSHandle<JSPrimitiveRef> jsPrimitiveRef = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, bigIntObj);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsPrimitiveRef.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsBigInt::ValueOf(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(BigInt::SameValue(bigIntHandle.GetTaggedValue(), result2), true);
}

// testcases of NumberToBigint()
HWTEST_F_L0(BuiltinsBigIntTest, NumberToBigint)
{
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> bigint(thread, JSTaggedValue::Undefined());

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    bool compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->GetSign());
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-0xffffffff));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(compareRes);

    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(0));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, number));
    ASSERT_TRUE(bigint->IsBigInt());
    compareRes = JSTaggedValue::Equal(thread, number, bigint);
    ASSERT_TRUE(compareRes);
}

// testcases of BigintToNumber()
HWTEST_F_L0(BuiltinsBigIntTest, BigintToNumber)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> bigint(thread, JSTaggedValue::Undefined());
    JSTaggedNumber number(0);

    JSHandle<JSTaggedValue> parma(factory->NewFromASCII("0xffff"));
    bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), static_cast<double>(0xffff));

    parma = JSHandle<JSTaggedValue>(
        factory->NewFromASCII("0xfffffffffffff8000000000000000000000000000000000000000000000000000"
                              "0000000000000000000000000000000000000000000000000000000000000000000"
                              "0000000000000000000000000000000000000000000000000000000000000000000"
                              "000000000000000000000000000000000000000000000000000000000"));
    bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), base::MAX_VALUE);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue::False());
    bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->IsZero());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), 0.0);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue(base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), base::MAX_VALUE);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-base::MAX_VALUE));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), -base::MAX_VALUE);

    parma = JSHandle<JSTaggedValue>(thread, JSTaggedValue(-0xffffffff));
    bigint = JSHandle<JSTaggedValue>(thread, BigInt::NumberToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = BigInt::BigIntToNumber(JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(number.GetNumber(), -0xffffffff);
}

// testcases of StringToBigInt(EcmaString)
HWTEST_F_L0(BuiltinsBigIntTest, StringToBigInt)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> bigint;
    JSHandle<EcmaString> str;
    JSHandle<JSTaggedValue> parma;

    // hex string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0xffff"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::HEXADECIMAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("ffff"));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0XFFFF"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::HEXADECIMAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("ffff"));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    // binary string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0b11111111"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::BINARY);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("11111111"));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0B11111111"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::BINARY);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("11111111"));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    // octal string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0o123456"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::OCTAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("123456"));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("0O123456"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint), BigInt::OCTAL);
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("123456"));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    // decimal string
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("999999999"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    str = BigInt::ToString(thread, JSHandle<BigInt>::Cast(bigint));
    ASSERT_EQ(str->Compare(reinterpret_cast<EcmaString *>(parma->GetRawData())), 0);

    // string has space
    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("  123  "));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(static_cast<double>(123)));
    bool compareRes = JSTaggedValue::Equal(thread, bigint, number);
    ASSERT_TRUE(compareRes);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("123   "));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<double>(123)));
    compareRes = JSTaggedValue::Equal(thread, bigint, number);
    ASSERT_TRUE(compareRes);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("   123"));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<double>(123)));
    compareRes = JSTaggedValue::Equal(thread, bigint, number);
    ASSERT_TRUE(compareRes);

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII(""));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->IsZero());

    parma = JSHandle<JSTaggedValue>(factory->NewFromASCII("    "));
    bigint = JSHandle<JSTaggedValue>(thread, base::NumberHelper::StringToBigInt(thread, parma));
    ASSERT_TRUE(bigint->IsBigInt());
    ASSERT_TRUE(JSHandle<BigInt>::Cast(bigint)->IsZero());
}
}  // namespace panda::test
