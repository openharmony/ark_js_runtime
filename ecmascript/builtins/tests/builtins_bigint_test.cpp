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
        // for consistency requirement, use ohos_icu4j/data/icudt67l.dat as icu-data-path
        options.SetIcuDataPath("./../../third_party/icu/ohos_icu4j/data/icudt67l.dat");
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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("456"));

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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("9223372036854775807"));
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
    JSHandle<EcmaString> str = factory->NewFromCanBeCompressString("9223372036854775807");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// AsIntN(64, (2 ^ 63))
HWTEST_F_L0(BuiltinsBigIntTest, AsIntN_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("9223372036854775808"));
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
    JSHandle<EcmaString> str = factory->NewFromCanBeCompressString("-9223372036854775808");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// AsUintN(64, (2 ^ 64 - 1))
HWTEST_F_L0(BuiltinsBigIntTest, AsUintN_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("18446744073709551615"));
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
    JSHandle<EcmaString> str = factory->NewFromCanBeCompressString("18446744073709551615");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// AsUintN(64, (2 ^ 64))
HWTEST_F_L0(BuiltinsBigIntTest, AsUintN_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("18446744073709551616"));
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
    JSHandle<EcmaString> str = factory->NewFromCanBeCompressString("0");
    EXPECT_EQ(resultStr->Compare(*str), 0);
}

// using locale
HWTEST_F_L0(BuiltinsBigIntTest, ToLocaleString_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("123456789123456789"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("de-DE"));

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
    JSHandle<EcmaString> resultStr(factory->NewFromCanBeCompressString("123.456.789.123.456.789"));
    EXPECT_EQ(ecmaStrHandle->Compare(*resultStr), 0);
}

// using locale and options
HWTEST_F_L0(BuiltinsBigIntTest, ToLocaleString_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("123456789123456789"));
    JSHandle<JSTaggedValue> formatStyle = thread->GlobalConstants()->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleKey(factory->NewFromCanBeCompressString("currency"));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromCanBeCompressString("EUR"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<BigInt> bigIntHandle(thread, result1);
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("de-DE"));
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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("17"));

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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("-0"));

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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("-10"));

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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("254"));

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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("-65536"));

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
    JSHandle<JSTaggedValue> numericValue(factory->NewFromCanBeCompressString("65535"));

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

    EXPECT_EQ(BigInt::SameValue(result1, result2), true);
}
}  // namespace panda::test