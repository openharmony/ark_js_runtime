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

#include <cmath>
#include <iostream>
#include <limits>
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsNumberTest : public testing::Test {
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

// new Number(10)
HWTEST_F_L0(BuiltinsNumberTest, NumberConstructor)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> number(env->GetNumberFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*number), 6);
    ecmaRuntimeCallInfo->SetFunction(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::NumberConstructor(ecmaRuntimeCallInfo.get());
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSPrimitiveRef *ref = JSPrimitiveRef::Cast(value.GetTaggedObject());
    ASSERT_EQ(ref->GetValue().GetDouble(), 5.0);
}

// Number.isFinite(-10)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite)
{
    const double value = -10;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(value)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// Number.isFinite(Number.MAX_VALUE)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::MAX_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// Number.isFinite("helloworld")
HWTEST_F_L0(BuiltinsNumberTest, IsFinite2)
{
    JSHandle<EcmaString> test = thread->GetEcmaVM()->GetFactory()->NewFromASCII("helloworld");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Number.isFinite(NaN)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Number.isFinite(Infinity)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Number.isFinite(undefined)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Number.isFinite(null)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Number.isInteger(0.1)
HWTEST_F_L0(BuiltinsNumberTest, IsInteger)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsInteger(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Number.isNaN(0.1)
HWTEST_F_L0(BuiltinsNumberTest, IsNaN)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::IsNaN(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// new Number(123.456).toString(7)
HWTEST_F_L0(BuiltinsNumberTest, ToString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = thread->GetEcmaVM()->GetFactory()->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(7.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ToString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("234.312256641535441");
    CVector<uint8_t> test(res->GetLength() + 1);
    res->CopyDataUtf8(test.data(), res->GetLength());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*res, *correctResult));
}

// new Number(123.456).toExponential(5)
HWTEST_F_L0(BuiltinsNumberTest, IsExponential)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(5.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ToExponential(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("1.23456e+2");
    CVector<uint8_t> test(res->GetLength() + 1);
    res->CopyDataUtf8(test.data(), res->GetLength());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*res, *correctResult));
}

// new Number(123.456).toFixed(10)
HWTEST_F_L0(BuiltinsNumberTest, ToFixed)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(10.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ToFixed(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("123.4560000000");
    ASSERT_TRUE(EcmaString::StringsAreEqual(*res, *correctResult));
}

// new Number(123.456).toFixed(30)
HWTEST_F_L0(BuiltinsNumberTest, ToFixed1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(30.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ToFixed(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("123.456000000000003069544618483633");
    ASSERT_TRUE(EcmaString::StringsAreEqual(*res, *correctResult));
}

// new Number(1e21).toFixed(20)
HWTEST_F_L0(BuiltinsNumberTest, ToFixed2) {
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1e21));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(20.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ToFixed(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("1e+21");
    CVector<uint8_t> test(res->GetLength() + 1);
    res->CopyDataUtf8(test.data(), res->GetLength());
    std::cout << test.data();
    ASSERT_TRUE(EcmaString::StringsAreEqual(*res, *correctResult));
}

// new Number(123.456).toPrecision(8)
HWTEST_F_L0(BuiltinsNumberTest, ToPrecision)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(8.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ToPrecision(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("123.45600");
    ASSERT_TRUE(EcmaString::StringsAreEqual(*res, *correctResult));
}

// Number.parseFloat(0x123)
HWTEST_F_L0(BuiltinsNumberTest, parseFloat)
{
    JSHandle<EcmaString> param = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x123");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, param.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ParseFloat(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(0)).GetRawData());
}

// Number.parseFloat(0x123xx)
HWTEST_F_L0(BuiltinsNumberTest, parseFloat1)
{
    JSHandle<EcmaString> param = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x123xx");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, param.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ParseFloat(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(0)).GetRawData());
}

// Number.parseInt(0x123)
HWTEST_F_L0(BuiltinsNumberTest, parseInt)
{
    const char *number = "0x123";

    JSHandle<EcmaString> param = thread->GetEcmaVM()->GetFactory()->NewFromASCII(number);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, param.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(16.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumber::ParseInt(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(291)).GetRawData());
}

// testcases of StringToDouble flags
HWTEST_F_L0(BuiltinsNumberTest, StringToDoubleFlags)
{
    JSHandle<EcmaString> str;
    Span<const uint8_t> sp;

    // flags of IGNORE_TRAILING

    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0a");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0o");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 00x");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 000.4_");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0.4);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 0010.s ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 10);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 0010e2");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 1000);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 0010e+3_0");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 10000);

    // flags of ALLOW_HEX
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX)));
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0x10 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX), 16);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x1g");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX + base::IGNORE_TRAILING), 1);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0xh");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(
        base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX + base::IGNORE_TRAILING)));

    // flags of ALLOW_OCTAL
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0O");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL)));
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0o10 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL), 8);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0o1d");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL | base::IGNORE_TRAILING),
              1);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0o8");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(
        base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL | base::IGNORE_TRAILING)));

    // flags of ALLOW_BINARY
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY)));
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0b10 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY), 2);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b1d");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY | base::IGNORE_TRAILING),
              1);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b2");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(
        base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY | base::IGNORE_TRAILING)));
}

// testcases of StringToDouble radix
HWTEST_F_L0(BuiltinsNumberTest, StringToDoubleRadix)
{
    JSHandle<EcmaString> str;
    Span<const uint8_t> sp;
    int radix;

    radix = 0;  // default 10
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 100);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 100.3e2 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 10030);
    radix = 1;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0000 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0001 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS)));
    radix = 2;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 4);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  11 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 3);
    radix = 3;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 9);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  21 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 7);
    radix = 4;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 16);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  31 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 13);
    radix = 8;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 64);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  71 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 57);
    radix = 10;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 100);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0020 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 20);
    radix = 16;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 256);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1e ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 30);
    radix = 18;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 324);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1g ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 34);
    radix = 25;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 625);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1g ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 41);
    radix = 36;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 1296);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1z ");
    sp = Span<const uint8_t>(str->GetDataUtf8(), str->GetUtf8Length() - 1);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 71);
}

HWTEST_F_L0(BuiltinsNumberTest, NumberToString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> res = factory->NewFromASCII("100");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(100))->Compare(*res), 0);
    res = factory->NewFromASCII("11223344");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(11223344))->Compare(*res), 0);
    res = factory->NewFromASCII("1234567890");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(1234567890))->Compare(*res), 0);
    res = factory->NewFromASCII("100");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.0)))->Compare(*res), 0);
    res = factory->NewFromASCII("100.5");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.5)))->Compare(*res), 0);
    res = factory->NewFromASCII("100.25");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.25)))->Compare(*res), 0);
    res = factory->NewFromASCII("100.125");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.125)))->Compare(*res), 0);
    res = factory->NewFromASCII("100.6125");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.6125)))->Compare(*res), 0);
    res = factory->NewFromASCII("0.0006125");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(0.0006125)))->Compare(*res), 0);
    res = factory->NewFromASCII("-0.0006125");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(-0.0006125)))->Compare(*res), 0);
    res = factory->NewFromASCII("-1234567890.0006125");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(-1234567890.0006125)))->Compare(*res), 0);
    res = factory->NewFromASCII("1234567890.0006125");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(1234567890.0006125)))->Compare(*res), 0);
    res = factory->NewFromASCII("11234567890.000612");
    ASSERT_EQ(base::NumberHelper::NumberToString(thread, JSTaggedValue(double(11234567890.0006125)))->Compare(*res), 0);
}
}  // namespace panda::test
