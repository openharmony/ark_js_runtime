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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSDateTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

JSDate *JSDateCreate(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    auto globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    return *dateObject;
}

HWTEST_F_L0(JSDateTest, Create)
{
    double tm = 0.0;
    JSHandle<JSDate> jsDate(thread, JSDateCreate(thread));
    EXPECT_EQ(jsDate->GetTimeValue(), JSTaggedValue(tm));
    EXPECT_EQ(jsDate->GetLocalOffset(), JSTaggedValue(JSDate::MAX_DOUBLE));
    tm = 28 * 60 * 60 * 1000;
    jsDate->SetTimeValue(thread, JSTaggedValue(tm));

    [[maybe_unused]] double temp = jsDate->GetTimeValue().GetDouble();
    EXPECT_EQ(jsDate->GetTimeValue(), JSTaggedValue(tm));
}

HWTEST_F_L0(JSDateTest, MakeTime)
{
    double const day1 = ecmascript::JSDate::MakeDay(0, 11, 31);
    double const time1 = ecmascript::JSDate::MakeTime(0, 0, 0, 0);
    double ms1 = ecmascript::JSDate::TimeClip(ecmascript::JSDate::MakeDate(day1, time1));
    EXPECT_EQ(ms1, -62135683200000.0);

    double const day = ecmascript::JSDate::MakeDay(-1, 11, 31);
    double const time = ecmascript::JSDate::MakeTime(0, 0, 0, 0);
    double ms = ecmascript::JSDate::TimeClip(ecmascript::JSDate::MakeDate(day, time));
    EXPECT_EQ(ms, -62167305600000.0);
}

HWTEST_F_L0(JSDateTest, IsoParseStringToMs)
{
    CString str = "2020-11-19T12:18:18.132Z";
    JSTaggedValue ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605788298132.0);

    str = "2020-11-19Z";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605744000000.0);

    str = "2020-11";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1604188800000.0);

    str = "+275760-09-13T00:00:00.000Z";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 8640000000000000.0);

    str = "-271821-04-20T00:00:00.000Z";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), -8640000000000000.0);

    str = "2020T12:18Z";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1577881080000.0);

    str = "2020T12:18:17.231Z";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1577881097231.0);

    str = "2020-11T12:18:17.231Z";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1604233097231.0);

    str = "1645-11T12:18:17.231+08:00";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), -10229658102769.0);

    str = "2020-11-19T12:18-08:12";
    ms = ecmascript::JSDate::IsoParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605817800000.0);
}

HWTEST_F_L0(JSDateTest, LocalParseStringToMs)
{
    CString str = "Thu Nov 19 2020 20:18:18 GMT+0800";
    JSTaggedValue ms = ecmascript::JSDate::LocalParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605788298000.0);

    str = "Thu Nov 19 2020 20:18 GMT-0800";
    ms = ecmascript::JSDate::LocalParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605845880000.0);

    str = "Thu Nov 03 2093 04:18 GMT";
    ms = ecmascript::JSDate::LocalParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 3908060280000.0);

    str = "Thu Nov 19 1820 GMT+1232";
    ms = ecmascript::JSDate::LocalParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), -4705734720000.0);
}

HWTEST_F_L0(JSDateTest, UtcParseStringToMs)
{
    CString str = "Thu, 19 Nov 2020 20:18:18 GMT+0800";
    JSTaggedValue ms = ecmascript::JSDate::UtcParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605788298000.0);

    str = "Thu, 19 Nov 2020 20:18 GMT-0800";
    ms = ecmascript::JSDate::UtcParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 1605845880000.0);

    str = "Thu 03 Jun 2093 04:18 GMT";
    ms = ecmascript::JSDate::UtcParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), 3894841080000.0);

    str = "Thu 19 Nov 1820 GMT+1232";
    ms = ecmascript::JSDate::UtcParseStringToMs(str);
    EXPECT_EQ(ms.GetDouble(), -4705734720000.0);
}
}  // namespace panda::test
