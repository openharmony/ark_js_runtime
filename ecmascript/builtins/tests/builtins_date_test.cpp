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

#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
namespace panda::test {
const char NEG = '-';
const char PLUS = '+';
const int STR_LENGTH_OTHERS = 2;
const int MINUTE_PER_HOUR = 60;
const int CHINA_BEFORE_1901_MIN = 485;
const int CHINA_AFTER_1901_MIN = 480;
const int64_t CHINA_BEFORE_1900_MS = -2177481943000;
class BuiltinsDateTest : public testing::Test {
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

JSHandle<JSDate> JSDateCreateTest(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    return dateObject;
}

HWTEST_F_L0(BuiltinsDateTest, SetGetDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    [[maybe_unused]] JSTaggedValue result1 = BuiltinsDate::SetDate(ecmaRuntimeCallInfo.get());
    JSTaggedValue result2 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    [[maybe_unused]] JSTaggedValue result3 = BuiltinsDate::SetUTCDate(ecmaRuntimeCallInfo.get());
    JSTaggedValue result4 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    [[maybe_unused]] JSTaggedValue result3 = BuiltinsDate::SetDate(ecmaRuntimeCallInfo.get());
    JSTaggedValue result4 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(29)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusUTCDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    [[maybe_unused]] JSTaggedValue result3 = BuiltinsDate::SetUTCDate(ecmaRuntimeCallInfo.get());
    JSTaggedValue result4 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(29)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    // 2018 : test case
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    // 10 : test case
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    // 2, 6 : test case
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo.get());
    // 2018 : test case
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo.get());
    // 10 : test case
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo.get());
    // 6 : test case
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    // 2018 : test case
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    // 10 : test case
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    // 2, 6 : test case
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCFullYear(ecmaRuntimeCallInfo.get());
    JSTaggedValue result4 = BuiltinsDate::GetUTCFullYear(ecmaRuntimeCallInfo.get());
    // 2018 : test case
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    JSTaggedValue result5 = BuiltinsDate::GetUTCMonth(ecmaRuntimeCallInfo.get());
    // 10 : test case
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo.get());
    // 6 : test case
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-2018)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(-10)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(-6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(-2019)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(1)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(22)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusUTCFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-2018)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(-10)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(-6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCFullYear(ecmaRuntimeCallInfo.get());
    JSTaggedValue result4 = BuiltinsDate::GetUTCFullYear(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(-2019)).GetRawData());

    JSTaggedValue result5 = BuiltinsDate::GetUTCMonth(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(1)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(22)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(18)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(6)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<double>(111)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetHours(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(18)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMinutes(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(18)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(6)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<double>(111)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCHours(ecmaRuntimeCallInfo.get());
    JSTaggedValue result5 = BuiltinsDate::GetUTCHours(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(18)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCMinutes(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result7 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result7.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result8 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result8.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-18)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(-10)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(-6)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<double>(-111)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetHours(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMinutes(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(49)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(53)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(889)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusUTCHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-18)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(-10)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(-6)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<double>(-111)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCHours(ecmaRuntimeCallInfo.get());
    JSTaggedValue result5 = BuiltinsDate::GetUTCHours(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCMinutes(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(49)).GetRawData());

    JSTaggedValue result7 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result7.GetRawData(), JSTaggedValue(static_cast<double>(53)).GetRawData());

    JSTaggedValue result8 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result8.GetRawData(), JSTaggedValue(static_cast<double>(889)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMilliseconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(100)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::SetMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCMilliseconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(100)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result3 = BuiltinsDate::SetUTCMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinutes)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(6)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(111)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetMinutes(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetMinutes(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCMinutes)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(6)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(111)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCMinutes(ecmaRuntimeCallInfo.get());
    JSTaggedValue result4 = BuiltinsDate::GetUTCMinutes(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result5 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMonth)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(8)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetMonth(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(8)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(3)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCMonth)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(8)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCMonth(ecmaRuntimeCallInfo.get());
    JSTaggedValue result3 = BuiltinsDate::GetUTCMonth(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(8)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(3)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetSeconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(59)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(123)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetSeconds(ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(59)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(123)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCSeconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(59)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(123)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetUTCSeconds(ecmaRuntimeCallInfo.get());
    JSTaggedValue result3 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(59)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(123)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetTime)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::SetTime(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetTime(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, UTC)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(2020.982));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(10.23));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(4.32));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(11.32));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1604487600000)).GetRawData());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 18);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(2020.982));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(10.23));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(4.32));
    ecmaRuntimeCallInfo1->SetCallArg(3, JSTaggedValue(11.32));
    ecmaRuntimeCallInfo1->SetCallArg(4, JSTaggedValue(45.1));
    ecmaRuntimeCallInfo1->SetCallArg(5, JSTaggedValue(34.321));
    ecmaRuntimeCallInfo1->SetCallArg(6, JSTaggedValue(static_cast<int32_t>(231)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1604490334231)).GetRawData());

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(10.23));
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue(4.32));
    ecmaRuntimeCallInfo2->SetCallArg(2, JSTaggedValue(11.32));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(-1882224000000)).GetRawData());

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(1994.982));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(757382400000)).GetRawData());

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(19999944.982));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(base::NAN_VALUE)).GetRawData());
}

void SetAllYearAndHours(JSThread *thread, const JSHandle<JSDate> &jsDate)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    // 2018 : test case
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    // 10 : test case
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    // 2, 6 : test case
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDate.GetTaggedValue());
    // 18 : test case
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(18)));
    // 10 : test case
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    // 2, 6 : test case
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<double>(6)));
    // 3, 111 : test case
    ecmaRuntimeCallInfo1->SetCallArg(3, JSTaggedValue(static_cast<double>(111)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
}

void SetAll1(JSThread *thread, const JSHandle<JSDate> &jsDate)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    // 1900 : test case
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(1900)));
    // 11 : test case
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(11)));
    // 2, 31 : test case
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(31)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDate.GetTaggedValue());
    // 23 : test case
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(23)));
    // 54 : test case
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<double>(54)));
    // 2, 16 : test case
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<double>(16)));
    // 3, 888 : test case
    ecmaRuntimeCallInfo1->SetCallArg(3, JSTaggedValue(static_cast<double>(888)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
}

void SetAll2(JSThread *thread, const JSHandle<JSDate> &jsDate)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(1901))); // 1901 : test case
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<double>(1))); // 2 : test case

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    // 12 : test case
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<double>(3))); // 3 : test case
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<double>(21))); // 2, 21 : test case
    ecmaRuntimeCallInfo1->SetCallArg(3, JSTaggedValue(static_cast<double>(129))); // 3, 129 : test case

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsDateTest, parse)
{
    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromString("2020-11-19T12:18:18.132Z");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605788298132)).GetRawData());

    JSHandle<EcmaString> str1 = thread->GetEcmaVM()->GetFactory()->NewFromString("2020-11-19Z");
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, str1.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605744000000)).GetRawData());

    JSHandle<EcmaString> str2 = thread->GetEcmaVM()->GetFactory()->NewFromString("2020-11T12:18:17.231+08:00");
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, str2.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1604204297231)).GetRawData());

    JSHandle<EcmaString> str3 = thread->GetEcmaVM()->GetFactory()->NewFromString("Thu Nov 19 2020 20:18:18 GMT+0800");
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetCallArg(0, str3.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605788298000)).GetRawData());

    JSHandle<EcmaString> str4 = thread->GetEcmaVM()->GetFactory()->NewFromString("Thu 03 Jun 2093 04:18 GMT");
    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetCallArg(0, str4.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(3894841080000)).GetRawData());

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo5->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue::Null());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5.get());
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo5.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(base::NAN_VALUE)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, ToDateString)
{
    JSHandle<EcmaString> expect_value = thread->GetEcmaVM()->GetFactory()->NewFromString("Tue Nov 06 2018");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    SetAllYearAndHours(thread, jsDate);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDate::ToDateString(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToISOString)
{
    JSHandle<EcmaString> expect_value = thread->GetEcmaVM()->GetFactory()->NewFromString("2020-11-19T12:18:18.132Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ToISOString(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToISOStringMinus)
{
    JSHandle<EcmaString> expect_value = thread->GetEcmaVM()->GetFactory()->NewFromString("1831-12-02T21:47:18.382Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ToISOString(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

// test toJSON and toPrimitive
HWTEST_F_L0(BuiltinsDateTest, ToJSON)
{
    JSHandle<EcmaString> expect_value = thread->GetEcmaVM()->GetFactory()->NewFromString("2020-11-19T12:18:18.132Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    jsDate->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ToJSON(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToJSONMinus)
{
    JSHandle<EcmaString> expect_value = thread->GetEcmaVM()->GetFactory()->NewFromString("1831-12-02T21:47:18.382Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    jsDate->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ToJSON(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToString)
{
    int localMin = 0;
    CString localTime;
    localMin = JSDate::GetLocalOffsetFromOS(localMin, true);

    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());

    SetAllYearAndHours(thread, jsDate);
    if (static_cast<int64_t>(JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->GetTimeValue().GetDouble()) <
            CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    JSTaggedValue result1 = BuiltinsDate::ToString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result1.IsString());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> result1_val(thread, reinterpret_cast<EcmaString *>(result1.GetRawData()));
    CString str = "Tue Nov 06 2018 18:10:06 GMT" + localTime;
    JSHandle<EcmaString> str_handle = thread->GetEcmaVM()->GetFactory()->NewFromString(str);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*result1_val, *str_handle));

    JSHandle<JSDate> js_date1 = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(js_date1.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());

    SetAll1(thread, js_date1);
    localTime = "";
    localMin = JSDate::GetLocalOffsetFromOS(localMin, true);
    if (static_cast<int64_t>(JSDate::Cast(js_date1.GetTaggedValue().GetTaggedObject())->GetTimeValue().GetDouble()) <
            CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    JSTaggedValue result2 = BuiltinsDate::ToString(ecmaRuntimeCallInfo1.get());
    ASSERT_TRUE(result2.IsString());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> result2_val(thread, reinterpret_cast<EcmaString *>(result2.GetRawData()));
    str = "Mon Dec 31 1900 23:54:16 GMT" + localTime;
    str_handle = thread->GetEcmaVM()->GetFactory()->NewFromString(str);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*result2_val, *str_handle));

    JSHandle<JSDate> js_date2 = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(js_date2.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());

    SetAll2(thread, js_date2);
    localTime = "";
    localMin = JSDate::GetLocalOffsetFromOS(localMin, true);
    if (static_cast<int64_t>(JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->GetTimeValue().GetDouble()) <
            CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    JSTaggedValue result3 = BuiltinsDate::ToString(ecmaRuntimeCallInfo2.get());
    ASSERT_TRUE(result3.IsString());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> result3_val(thread, reinterpret_cast<EcmaString *>(result3.GetRawData()));
    str = "Tue Jan 01 1901 00:03:21 GMT" + localTime;
    str_handle = thread->GetEcmaVM()->GetFactory()->NewFromString(str);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*result3_val, *str_handle));
}

HWTEST_F_L0(BuiltinsDateTest, ToTimeString)
{
    int localMin = 0;
    CString localTime;
    localMin = JSDate::GetLocalOffsetFromOS(localMin, true);

    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());

    SetAllYearAndHours(thread, jsDate);
    if (static_cast<int64_t>(JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->GetTimeValue().GetDouble()) <
            CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    JSTaggedValue result1 = BuiltinsDate::ToTimeString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result1.IsString());
    JSHandle<EcmaString> result1_val(thread, reinterpret_cast<EcmaString *>(result1.GetRawData()));
    CString str = "18:10:06 GMT" + localTime;
    JSHandle<EcmaString> str_handle = thread->GetEcmaVM()->GetFactory()->NewFromString(str);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*result1_val, *str_handle));

    JSHandle<JSDate> js_date1 = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(js_date1.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    SetAll1(thread, js_date1);
    localTime = "";
    localMin = JSDate::GetLocalOffsetFromOS(localMin, true);
    if (static_cast<int64_t>(JSDate::Cast(js_date1.GetTaggedValue().GetTaggedObject())->GetTimeValue().GetDouble()) <
            CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    JSTaggedValue result2 = BuiltinsDate::ToTimeString(ecmaRuntimeCallInfo1.get());
    ASSERT_TRUE(result2.IsString());
    JSHandle<EcmaString> result2_val(thread, reinterpret_cast<EcmaString *>(result2.GetRawData()));
    str = "23:54:16 GMT" + localTime;
    str_handle = thread->GetEcmaVM()->GetFactory()->NewFromString(str);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*result2_val, *str_handle));
    JSHandle<JSDate> js_date2 = JSDateCreateTest(thread);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(js_date2.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    SetAll2(thread, js_date2);
    localTime = "";
    localMin = JSDate::GetLocalOffsetFromOS(localMin, true);
    if (static_cast<int64_t>(JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->GetTimeValue().GetDouble()) <
            CHINA_BEFORE_1900_MS &&
        localMin == CHINA_AFTER_1901_MIN) {
        localMin = CHINA_BEFORE_1901_MIN;
    }
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    JSTaggedValue result3 = BuiltinsDate::ToTimeString(ecmaRuntimeCallInfo2.get());
    ASSERT_TRUE(result3.IsString());
    JSHandle<EcmaString> result3_val(thread, reinterpret_cast<EcmaString *>(result3.GetRawData()));
    str = "00:03:21 GMT" + localTime;
    str_handle = thread->GetEcmaVM()->GetFactory()->NewFromString(str);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*result3_val, *str_handle));
}

HWTEST_F_L0(BuiltinsDateTest, ToUTCString)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromString("Thu, 19 Nov 2020 12:18:18 GMT");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ToUTCString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToUTCStringMinus)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromString("Fri, 02 Dec 1831 21:47:18 GMT");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ToUTCString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ValueOf)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ValueOf(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605788298132)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, ValueOfMinus)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsDate::ValueOf(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(-4357419161618)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, DateConstructor)
{
    // case1: test new target is undefined.
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> date_func(globalEnv->GetDateFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result1.IsString());

    // case2: length == 0
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 4);
    ecmaRuntimeCallInfo2->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetThis(jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result2.IsObject());

    // case3: length == 1
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo3->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    JSTaggedValue result3 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result3.IsObject());

    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo3.get());
    JSTaggedValue result4 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo3.get());
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    // case3: length > 1
    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo4->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo4->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    JSTaggedValue result5 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo4.get());
    ASSERT_TRUE(result5.IsObject());

    SetAllYearAndHours(thread, jsDate);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo4.get());
    JSTaggedValue result6 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo4.get());
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());
    JSTaggedValue result7 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo4.get());
    ASSERT_EQ(result7.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());
}
}  // namespace panda::test
