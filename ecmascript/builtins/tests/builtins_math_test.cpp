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
#include "ecmascript/builtins/builtins_math.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using namespace panda::ecmascript::base;

namespace panda::test {
class BuiltinsMathTest : public testing::Test {
public:
    // Workaround: Avoid thread local leak [F/runtime: cannot create thread specific key for __cxa_get_globals()]
    static void SetUpTestCase()
    {
        TestHelper::CreateEcmaVMWithScope(instance_, thread_, scope_);
    }

    static void TearDownTestCase()
    {
        TestHelper::DestroyEcmaVMWithScope(instance_, scope_);
    }

    static PandaVM *instance_;
    static EcmaHandleScope *scope_;
    static JSThread *thread_;
};
PandaVM *BuiltinsMathTest::instance_ = nullptr;
EcmaHandleScope *BuiltinsMathTest::scope_ = nullptr;
JSThread *BuiltinsMathTest::thread_ = nullptr;

// Math.abs(-10)
HWTEST_F_L0(BuiltinsMathTest, Abs)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(10);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(10)
HWTEST_F_L0(BuiltinsMathTest, Abs_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(10);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(0)
HWTEST_F_L0(BuiltinsMathTest, Abs_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(null)
HWTEST_F_L0(BuiltinsMathTest, Abs_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs("hello")
HWTEST_F_L0(BuiltinsMathTest, Abs_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("helloworld");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(Number.MAX_VALUE + 1)
HWTEST_F_L0(BuiltinsMathTest, Abs_5)
{
    const double testValue = base::MAX_VALUE + 1;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(testValue));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::MAX_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(Number.MIN_VALUE)
HWTEST_F_L0(BuiltinsMathTest, Abs_6)
{
    const double testValue = base::MIN_VALUE + 1;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(testValue));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(Number.POSITIVE_INFINITY + 1)
HWTEST_F_L0(BuiltinsMathTest, Abs_7)
{
    const double testValue = base::POSITIVE_INFINITY + 1;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(testValue));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(Number.NEGATIVE_INFINITY - 1)
HWTEST_F_L0(BuiltinsMathTest, Abs_8)
{
    const double testValue = -base::POSITIVE_INFINITY - 1;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(testValue));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(Number.NAN_VALUE)
HWTEST_F_L0(BuiltinsMathTest, Abs_9)
{
    const double testValue = base::NAN_VALUE;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(testValue));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(VALUE_UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Abs_10)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(true)
HWTEST_F_L0(BuiltinsMathTest, Abs_11)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(1);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(false)
HWTEST_F_L0(BuiltinsMathTest, Abs_12)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs(hole)
HWTEST_F_L0(BuiltinsMathTest, Abs_13)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Hole());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.abs("100.12")
HWTEST_F_L0(BuiltinsMathTest, Abs_14)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("100.12");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Abs(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(100.12);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(-1)
HWTEST_F_L0(BuiltinsMathTest, Acos)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(BuiltinsMath::PI);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(1)
HWTEST_F_L0(BuiltinsMathTest, Acos_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(-1.5)
HWTEST_F_L0(BuiltinsMathTest, Acos_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(null)
HWTEST_F_L0(BuiltinsMathTest, Acos_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Acos_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(true)
HWTEST_F_L0(BuiltinsMathTest, Acos_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(false)
HWTEST_F_L0(BuiltinsMathTest, Acos_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos("0.1")
HWTEST_F_L0(BuiltinsMathTest, Acos_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.4706289056333368);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos("")
HWTEST_F_L0(BuiltinsMathTest, Acos_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acos(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Acos_9)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(1.1)
HWTEST_F_L0(BuiltinsMathTest, Acosh)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(1.1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.4435682543851154);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(0.5)
HWTEST_F_L0(BuiltinsMathTest, Acosh_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(base::POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Acosh_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(null)
HWTEST_F_L0(BuiltinsMathTest, Acosh_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(VALUE_UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Acosh_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(true)
HWTEST_F_L0(BuiltinsMathTest, Acosh_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(false)
HWTEST_F_L0(BuiltinsMathTest, Acosh_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(hole)
HWTEST_F_L0(BuiltinsMathTest, Acosh_7)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Hole());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh("1")
HWTEST_F_L0(BuiltinsMathTest, Acosh_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh("")
HWTEST_F_L0(BuiltinsMathTest, Acosh_9)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.acosh(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Acosh_10)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Acosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(-1)
HWTEST_F_L0(BuiltinsMathTest, Asin)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(1)
HWTEST_F_L0(BuiltinsMathTest, Asin_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Asin_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(null)
HWTEST_F_L0(BuiltinsMathTest, Asin_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Asin_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(true)
HWTEST_F_L0(BuiltinsMathTest, Asin_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(false)
HWTEST_F_L0(BuiltinsMathTest, Asin_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin(""")
HWTEST_F_L0(BuiltinsMathTest, Asin_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asin("1")
HWTEST_F_L0(BuiltinsMathTest, Asin_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(-1)
HWTEST_F_L0(BuiltinsMathTest, Asinh)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.881373587019543);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(1)
HWTEST_F_L0(BuiltinsMathTest, Asinh_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.881373587019543);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(null)
HWTEST_F_L0(BuiltinsMathTest, Asinh_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Asinh_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(NEGATIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Asinh_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(true)
HWTEST_F_L0(BuiltinsMathTest, Asinh_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.881373587019543);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh(false)
HWTEST_F_L0(BuiltinsMathTest, Asinh_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh("")
HWTEST_F_L0(BuiltinsMathTest, Asinh_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.asinh("-5.7")
HWTEST_F_L0(BuiltinsMathTest, Asinh_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-5.7");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Asinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-2.44122070725561);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(-1)
HWTEST_F_L0(BuiltinsMathTest, Atan)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.7853981633974483);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(1)
HWTEST_F_L0(BuiltinsMathTest, Atan_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.7853981633974483);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(null)
HWTEST_F_L0(BuiltinsMathTest, Atan_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Atan_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Atan_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(BuiltinsMath::PI / 2);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(true)
HWTEST_F_L0(BuiltinsMathTest, Atan_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.7853981633974483);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan(false)
HWTEST_F_L0(BuiltinsMathTest, Atan_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan("")
HWTEST_F_L0(BuiltinsMathTest, Atan_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString(" ");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan("-1")
HWTEST_F_L0(BuiltinsMathTest, Atan_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.7853981633974483);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(-1)
HWTEST_F_L0(BuiltinsMathTest, Atanh)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(1)
HWTEST_F_L0(BuiltinsMathTest, Atanh_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(null)
HWTEST_F_L0(BuiltinsMathTest, Atanh_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Atanh_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(1.5)
HWTEST_F_L0(BuiltinsMathTest, Atanh_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(true)
HWTEST_F_L0(BuiltinsMathTest, Atanh_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh(false)
HWTEST_F_L0(BuiltinsMathTest, Atanh_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh("")
HWTEST_F_L0(BuiltinsMathTest, Atanh_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString(" ");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atanh("-1")
HWTEST_F_L0(BuiltinsMathTest, Atanh_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(NaN, 1.5)
HWTEST_F_L0(BuiltinsMathTest, Atan2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::NAN_VALUE));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(-1, 1.5)
HWTEST_F_L0(BuiltinsMathTest, Atan2_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.5880026035475675);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(1, -0)
HWTEST_F_L0(BuiltinsMathTest, Atan2_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(BuiltinsMath::PI / 2);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(0, 1)
HWTEST_F_L0(BuiltinsMathTest, Atan2_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(0, -0)
HWTEST_F_L0(BuiltinsMathTest, Atan2_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(BuiltinsMath::PI);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(-0, 0)
HWTEST_F_L0(BuiltinsMathTest, Atan2_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(-0, -0)
HWTEST_F_L0(BuiltinsMathTest, Atan2_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-BuiltinsMath::PI);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(true, false)
HWTEST_F_L0(BuiltinsMathTest, Atan2_7)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(false, true)
HWTEST_F_L0(BuiltinsMathTest, Atan2_8)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2("-1","")
HWTEST_F_L0(BuiltinsMathTest, Atan2_9)
{
    JSHandle<EcmaString> test_1 = thread_->GetEcmaVM()->GetFactory()->NewFromString("-1");
    JSHandle<EcmaString> test_2 = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test_1.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, test_2.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.5707963267948966);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2("0.23","0.72")
HWTEST_F_L0(BuiltinsMathTest, Atan2_10)
{
    JSHandle<EcmaString> test_1 = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.23");
    JSHandle<EcmaString> test_2 = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.72");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test_1.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, test_2.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.3091989123270746);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.atan2(-NaN, 1.5)
HWTEST_F_L0(BuiltinsMathTest, Atan2_11)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(-1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Atan2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(0)
HWTEST_F_L0(BuiltinsMathTest, Cbrt)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(-0)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(NEGATIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(VALUE_UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(true)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(false)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt("")
HWTEST_F_L0(BuiltinsMathTest, Cbrt_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString(" ");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt("1.23")
HWTEST_F_L0(BuiltinsMathTest, Cbrt_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("1.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0714412696907731);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cbrt(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Cbrt_9)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cbrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(3.25)
HWTEST_F_L0(BuiltinsMathTest, Ceil)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(3.25));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(4.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Ceil_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Ceil_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(null)
HWTEST_F_L0(BuiltinsMathTest, Ceil_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(0)
HWTEST_F_L0(BuiltinsMathTest, Ceil_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(true)
HWTEST_F_L0(BuiltinsMathTest, Ceil_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(false)
HWTEST_F_L0(BuiltinsMathTest, Ceil_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil("")
HWTEST_F_L0(BuiltinsMathTest, Ceil_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil("3.23")
HWTEST_F_L0(BuiltinsMathTest, Ceil_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(4.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.ceil(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Ceil_9)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Ceil(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos(0)
HWTEST_F_L0(BuiltinsMathTest, Cos)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Cos_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Cos_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos(-POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Cos_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos(true)
HWTEST_F_L0(BuiltinsMathTest, Cos_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.5403023058681398);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos(false)
HWTEST_F_L0(BuiltinsMathTest, Cos_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos("")
HWTEST_F_L0(BuiltinsMathTest, Cos_6)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cos("3.23")
HWTEST_F_L0(BuiltinsMathTest, Cos_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cos(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.9960946152060809);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh(0)
HWTEST_F_L0(BuiltinsMathTest, Cosh)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Cosh_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Cosh_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh(-POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Cosh_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh(true)
HWTEST_F_L0(BuiltinsMathTest, Cosh_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5430806348152437);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh(false)
HWTEST_F_L0(BuiltinsMathTest, Cosh_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh("")
HWTEST_F_L0(BuiltinsMathTest, Cosh_6)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString(" ");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.cosh("3.23")
HWTEST_F_L0(BuiltinsMathTest, Cosh_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Cosh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(12.659607234875645);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp(0)
HWTEST_F_L0(BuiltinsMathTest, Exp)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Exp_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Exp_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp(-POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Exp_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp(true)
HWTEST_F_L0(BuiltinsMathTest, Exp_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(2.718281828459045);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp(false)
HWTEST_F_L0(BuiltinsMathTest, Exp_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp("")
HWTEST_F_L0(BuiltinsMathTest, Exp_6)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.exp("-3.23")
HWTEST_F_L0(BuiltinsMathTest, Exp_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Exp(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.039557498788398725);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Expm1(0)
HWTEST_F_L0(BuiltinsMathTest, Expm1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Expm1(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Expm1_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Expm1(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Expm1_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Expm1(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Expm1_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Expm1(-POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Expm1_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.expm1(true)
HWTEST_F_L0(BuiltinsMathTest, Expm1_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    double expect = 1.718281828459045;
    ASSERT_TRUE(result.IsDouble());
    ASSERT_TRUE(std::abs(result.GetDouble() - expect) < 0.00000001);
}

// Math.expm1(false)
HWTEST_F_L0(BuiltinsMathTest, Expm1_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.expm1("")
HWTEST_F_L0(BuiltinsMathTest, Expm1_7)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString(" ");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.expm1("-3.23")
HWTEST_F_L0(BuiltinsMathTest, Expm1_8)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.9604425012116012);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.expm1("0x12")
HWTEST_F_L0(BuiltinsMathTest, Expm1_9)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0x12");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Expm1(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(65659968.13733051);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.floor(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Floor)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Floor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.floor(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Floor_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Floor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.floor(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Floor_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Floor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.floor(true)
HWTEST_F_L0(BuiltinsMathTest, Floor_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Floor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.floor("-3.23")
HWTEST_F_L0(BuiltinsMathTest, Floor_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Floor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-4.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Log)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Log_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Log_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log(true)
HWTEST_F_L0(BuiltinsMathTest, Log_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log("-3.23")
HWTEST_F_L0(BuiltinsMathTest, Log_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log(0.12)
HWTEST_F_L0(BuiltinsMathTest, Log_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.12));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-2.120263536200091);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log1p(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Log1p)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log1p(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log1p(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Log1p_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log1p(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log1p(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Log1p_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log1p(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log1p(true)
HWTEST_F_L0(BuiltinsMathTest, Log1p_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log1p(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.6931471805599453);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log1p("-3.23")
HWTEST_F_L0(BuiltinsMathTest, Log1p_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-3.23");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log1p(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log1p(0.12)
HWTEST_F_L0(BuiltinsMathTest, Log1p_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.12));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log1p(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.11332868530700317);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log10(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Log10)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log10(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Log10(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Log10_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log10(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Log10(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Log10_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log10(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Log10(true)
HWTEST_F_L0(BuiltinsMathTest, Log10_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log10(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Log10("2")
HWTEST_F_L0(BuiltinsMathTest, Log10_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("2");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log10(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.3010299956639812);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Log10(0.12)
HWTEST_F_L0(BuiltinsMathTest, Log10_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.12));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log10(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.9208187539523752);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log2(-0.0)
HWTEST_F_L0(BuiltinsMathTest, Log2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log2(-NAN)
HWTEST_F_L0(BuiltinsMathTest, Log2_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log2(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Log2_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log2(true)
HWTEST_F_L0(BuiltinsMathTest, Log2_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log2("2")
HWTEST_F_L0(BuiltinsMathTest, Log2_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("2");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.log2(1)
HWTEST_F_L0(BuiltinsMathTest, Log2_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Log2(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Max(NaN,1,POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Max)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::NAN_VALUE));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Max(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Max()
HWTEST_F_L0(BuiltinsMathTest, Max_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Max(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Max("3",100,2.5)
HWTEST_F_L0(BuiltinsMathTest, Max_2)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("3");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(100)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(2.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Max(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(100);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Max(3,"100",-101.5)
HWTEST_F_L0(BuiltinsMathTest, Max_3)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("100");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo->SetCallArg(1, test.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(-101.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Max(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(100.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Max(-3,"-100",true)
HWTEST_F_L0(BuiltinsMathTest, Max_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-100");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-3)));
    ecmaRuntimeCallInfo->SetCallArg(1, test.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Max(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(1);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.min(NaN,1,POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Min)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::NAN_VALUE));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Min(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.min()
HWTEST_F_L0(BuiltinsMathTest, Min_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Min(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.min("3",100,2.5)
HWTEST_F_L0(BuiltinsMathTest, Min_2)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("3");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(100)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(2.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Min(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(2.5);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.min(3,"100",-101.5)
HWTEST_F_L0(BuiltinsMathTest, Min_3)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("100");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo->SetCallArg(1, test.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(-101.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Min(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-101.5);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.min(3,100,false)
HWTEST_F_L0(BuiltinsMathTest, Min_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(100)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Min(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.pow(2,"-2")
HWTEST_F_L0(BuiltinsMathTest, Pow)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-2");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo->SetCallArg(1, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Pow(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.25);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.pow(-NaN,-2)
HWTEST_F_L0(BuiltinsMathTest, Pow_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(-2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Pow(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.pow()
HWTEST_F_L0(BuiltinsMathTest, Pow_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Pow(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.pow(false,-2)
HWTEST_F_L0(BuiltinsMathTest, Pow_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::False());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(-2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Pow(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.random()
HWTEST_F_L0(BuiltinsMathTest, Random)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsMath::Random(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue result2 = BuiltinsMath::Random(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    ASSERT_NE(result1.GetRawData(), result2.GetRawData());
}

// Math.random()
HWTEST_F_L0(BuiltinsMathTest, Random_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsMath::Random(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue result2 = BuiltinsMath::Random(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    double value1 = JSTaggedValue(static_cast<JSTaggedType>(result1.GetRawData())).GetDouble();
    double value2 = JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())).GetDouble();
    ASSERT_TRUE(value1 >= 0);
    ASSERT_TRUE(value1 < 1.0);
    ASSERT_TRUE(value2 >= 0);
    ASSERT_TRUE(value2 < 1.0);
}

// Math.round(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Round)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Round(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.round(1.25)
HWTEST_F_L0(BuiltinsMathTest, Round_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(1.25));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Round(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.round(-0.14)
HWTEST_F_L0(BuiltinsMathTest, Round_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.14));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Round(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.round(-0.7)
HWTEST_F_L0(BuiltinsMathTest, Round_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.7));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Round(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.round(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Round_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Round(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.fround(POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Fround)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Fround(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.fround(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Fround_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Fround(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.fround(-0)
HWTEST_F_L0(BuiltinsMathTest, Fround_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Fround(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.fround(1.337)
HWTEST_F_L0(BuiltinsMathTest, Fround_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(1.337));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Fround(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.3370000123977661);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.fround(-668523145.253485)
HWTEST_F_L0(BuiltinsMathTest, Fround_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-668523145.253485));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Fround(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-668523136.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32(NaN)
HWTEST_F_L0(BuiltinsMathTest, Clz32)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(32);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32(-0)
HWTEST_F_L0(BuiltinsMathTest, Clz32_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(32);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32(1)
HWTEST_F_L0(BuiltinsMathTest, Clz32_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(31);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32(568243)
HWTEST_F_L0(BuiltinsMathTest, Clz32_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(568243)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(12);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32(4294967295)
HWTEST_F_L0(BuiltinsMathTest, Clz32_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4294967295)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32(10000000000.123)
HWTEST_F_L0(BuiltinsMathTest, Clz32_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(10000000000.123));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(1);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.clz32()
HWTEST_F_L0(BuiltinsMathTest, Clz32_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Clz32(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(32);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.hypot()
HWTEST_F_L0(BuiltinsMathTest, Hypot)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Hypot(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.hypot(-2.1)
HWTEST_F_L0(BuiltinsMathTest, Hypot_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-2.1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Hypot(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(2.1);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.hypot(-NaN, 1)
HWTEST_F_L0(BuiltinsMathTest, Hypot_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Hypot(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    ASSERT_TRUE(result.IsDouble());
    ASSERT_TRUE(std::isnan(result.GetDouble()));
}

// Math.hypot(true, 5, 8, -0.2, 90000)
HWTEST_F_L0(BuiltinsMathTest, Hypot_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 14);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(8)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(-0.2));
    ecmaRuntimeCallInfo->SetCallArg(4, JSTaggedValue(static_cast<int32_t>(90000)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Hypot(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(90000.00050022222);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Imul()
HWTEST_F_L0(BuiltinsMathTest, Imul)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Imul(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Imul("-2",9.256)
HWTEST_F_L0(BuiltinsMathTest, Imul_1)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-2");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(9.256));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Imul(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(-18);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Imul(5,0xffffffff)
HWTEST_F_L0(BuiltinsMathTest, Imul_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0xffffffff)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Imul(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(-5);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.Imul(5,0xfffffffe)
HWTEST_F_L0(BuiltinsMathTest, Imul_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0xfffffffe)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Imul(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedInt(-10);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(-1)
HWTEST_F_L0(BuiltinsMathTest, Sin)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.8414709848078965);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(-1.5)
HWTEST_F_L0(BuiltinsMathTest, Sin_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.9974949866040544);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(null)
HWTEST_F_L0(BuiltinsMathTest, Sin_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Sin_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(true)
HWTEST_F_L0(BuiltinsMathTest, Sin_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.8414709848078965);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin("0.1")
HWTEST_F_L0(BuiltinsMathTest, Sin_6)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.09983341664682815);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(Number.POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Sin_7)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sin(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Sin_8)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sin(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(-1)
HWTEST_F_L0(BuiltinsMathTest, Sinh)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.1752011936438014);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(-1.5)
HWTEST_F_L0(BuiltinsMathTest, Sinh_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-1.5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-2.1292794550948173);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(null)
HWTEST_F_L0(BuiltinsMathTest, Sinh_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(UNDEFINED)
HWTEST_F_L0(BuiltinsMathTest, Sinh_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(true)
HWTEST_F_L0(BuiltinsMathTest, Sinh_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.1752011936438014);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh("0.1")
HWTEST_F_L0(BuiltinsMathTest, Sinh_5)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.10016675001984403);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(-Number.POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Sinh_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sinh(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Sinh_7)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sinh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt(-1)
HWTEST_F_L0(BuiltinsMathTest, Sqrt)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt(-0)
HWTEST_F_L0(BuiltinsMathTest, Sqrt_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt(null)
HWTEST_F_L0(BuiltinsMathTest, Sqrt_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt(true)
HWTEST_F_L0(BuiltinsMathTest, Sqrt_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt("0.1")
HWTEST_F_L0(BuiltinsMathTest, Sqrt_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.31622776601683794);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt(Number.POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Sqrt_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.sqrt(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Sqrt_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Sqrt(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan(-1)
HWTEST_F_L0(BuiltinsMathTest, Tan)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.5574077246549023);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan(-0)
HWTEST_F_L0(BuiltinsMathTest, Tan_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan(null)
HWTEST_F_L0(BuiltinsMathTest, Tan_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan(true)
HWTEST_F_L0(BuiltinsMathTest, Tan_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.5574077246549023);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan("0.1")
HWTEST_F_L0(BuiltinsMathTest, Tan_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.10033467208545055);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan(Number.POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Tan_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tan(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Tan_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tan(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh(-1)
HWTEST_F_L0(BuiltinsMathTest, Tanh)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.7615941559557649);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh(-0)
HWTEST_F_L0(BuiltinsMathTest, Tanh_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh(null)
HWTEST_F_L0(BuiltinsMathTest, Tanh_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh(true)
HWTEST_F_L0(BuiltinsMathTest, Tanh_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.7615941559557649);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh("0.1")
HWTEST_F_L0(BuiltinsMathTest, Tanh_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0.09966799462495582);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh(Number.POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Tanh_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.tanh(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Tanh_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Tanh(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc(-1)
HWTEST_F_L0(BuiltinsMathTest, Trunc)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc(-0)
HWTEST_F_L0(BuiltinsMathTest, Trunc_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-0.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc(null)
HWTEST_F_L0(BuiltinsMathTest, Trunc_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc(true)
HWTEST_F_L0(BuiltinsMathTest, Trunc_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(1.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc("-0.1")
HWTEST_F_L0(BuiltinsMathTest, Trunc_4)
{
    JSHandle<EcmaString> test = thread_->GetEcmaVM()->GetFactory()->NewFromString("-0.1");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(-0.0);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc(Number.POSITIVE_INFINITY)
HWTEST_F_L0(BuiltinsMathTest, Trunc_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::POSITIVE_INFINITY);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}

// Math.trunc(-NaN)
HWTEST_F_L0(BuiltinsMathTest, Trunc_6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(-base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMath::Trunc(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread_, prev);
    JSTaggedValue expect = BuiltinsBase::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), expect.GetRawData());
}
}  // namespace panda::test
