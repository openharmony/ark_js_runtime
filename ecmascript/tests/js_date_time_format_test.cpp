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

#include "ecmascript/global_env.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSDateTimeFormatTest : public testing::Test {
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
        options.SetEnableForceGC(true);
        instance = JSNApi::CreateEcmaVM(options);
        instance->SetEnableForceGC(true);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

void SetDateOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
    std::map<std::string, std::string> dateOptions)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto globalConst = thread->GlobalConstants();
    // Date options keys.
    JSHandle<JSTaggedValue> weekdayKey = globalConst->GetHandledWeekdayString();
    JSHandle<JSTaggedValue> yearKey = globalConst->GetHandledYearString();
    JSHandle<JSTaggedValue> monthKey = globalConst->GetHandledMonthString();
    JSHandle<JSTaggedValue> dayKey = globalConst->GetHandledDayString();
    // Date options values.
    JSHandle<JSTaggedValue> weekdayValue(factory->NewFromASCII(dateOptions["weekday"].c_str()));
    JSHandle<JSTaggedValue> yearValue(factory->NewFromASCII(dateOptions["year"].c_str()));
    JSHandle<JSTaggedValue> monthValue(factory->NewFromASCII(dateOptions["month"].c_str()));
    JSHandle<JSTaggedValue> dayValue(factory->NewFromASCII(dateOptions["day"].c_str()));
    // Set date options.
    JSObject::SetProperty(thread, optionsObj, weekdayKey, weekdayValue);
    JSObject::SetProperty(thread, optionsObj, yearKey, yearValue);
    JSObject::SetProperty(thread, optionsObj, monthKey, monthValue);
    JSObject::SetProperty(thread, optionsObj, dayKey, dayValue);
}

void SetTimeOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
    std::map<std::string, std::string> timeOptionsMap)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto globalConst = thread->GlobalConstants();
    // Time options keys.
    JSHandle<JSTaggedValue> dayPeriodKey = globalConst->GetHandledDayPeriodString();
    JSHandle<JSTaggedValue> hourKey = globalConst->GetHandledHourString();
    JSHandle<JSTaggedValue> minuteKey = globalConst->GetHandledMinuteString();
    JSHandle<JSTaggedValue> secondKey = globalConst->GetHandledSecondString();
    JSHandle<JSTaggedValue> fractionalSecondDigitsKey = globalConst->GetHandledFractionalSecondDigitsString();
    // Time options values.
    JSHandle<JSTaggedValue> dayPeriodValue(factory->NewFromASCII(timeOptionsMap["dayPeriod"].c_str()));
    JSHandle<JSTaggedValue> hourValue(factory->NewFromASCII(timeOptionsMap["hour"].c_str()));
    JSHandle<JSTaggedValue> minuteValue(factory->NewFromASCII(timeOptionsMap["minute"].c_str()));
    JSHandle<JSTaggedValue> secondValue(factory->NewFromASCII(timeOptionsMap["second"].c_str()));
    JSHandle<JSTaggedValue> fractionalSecondDigitsValue(
        factory->NewFromASCII(timeOptionsMap["fractionalSecond"].c_str()));
    // Set time options.
    JSObject::SetProperty(thread, optionsObj, dayPeriodKey, dayPeriodValue);
    JSObject::SetProperty(thread, optionsObj, hourKey, hourValue);
    JSObject::SetProperty(thread, optionsObj, minuteKey, minuteValue);
    JSObject::SetProperty(thread, optionsObj, secondKey, secondValue);
    JSObject::SetProperty(thread, optionsObj, fractionalSecondDigitsKey, fractionalSecondDigitsValue);
}

/**
 * @tc.name: GetIcuLocale & SetIcuLocale
 * @tc.desc: Set and obtain localization labels compatible with ICU Libraries.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, Set_Get_IcuLocale)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetDateTimeFormatFunction();
    JSHandle<JSDateTimeFormat> dtf =
        JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    icu::Locale locale1("ko", "Kore", "KR");
    JSDateTimeFormat::SetIcuLocale(thread, dtf, locale1, JSDateTimeFormat::FreeIcuLocale);
    icu::Locale *resLocale1 = dtf->GetIcuLocale();
    EXPECT_STREQ(resLocale1->getBaseName(), "ko_Kore_KR");

    icu::Locale locale2("zh", "Hans", "Cn");
    JSDateTimeFormat::SetIcuLocale(thread, dtf, locale2, JSDateTimeFormat::FreeIcuLocale);
    icu::Locale *resLocale2 = dtf->GetIcuLocale();
    EXPECT_STREQ(resLocale2->getBaseName(), "zh_Hans_CN");
}

/**
 * @tc.name: SetIcuSimpleDateFormat & GetIcuSimpleDateFormat
 * @tc.desc: Set and obtain a simple time and date format compatible with ICU Libraries.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, Set_Get_IcuSimpleDateFormat)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetDateTimeFormatFunction();
    JSHandle<JSDateTimeFormat> dtf =
        JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::UnicodeString dateTime1("2022.05.25 11:09:34");
    icu::UnicodeString dateTime2("2022.May.25 11:09:34");
    icu::UnicodeString dateTime3("2022.May.25 AD 11:09:34 AM");

    icu::UnicodeString pattern("yyyy.MM.dd HH:mm:ss");
    icu::SimpleDateFormat sdf(pattern, status);
    JSDateTimeFormat::SetIcuSimpleDateFormat(thread, dtf, sdf, JSDateTimeFormat::FreeSimpleDateFormat);
    icu::SimpleDateFormat *resSdf = dtf->GetIcuSimpleDateFormat();
    UDate timeStamp = resSdf->parse(dateTime1, status);
    EXPECT_EQ(timeStamp, 1653448174000);
    status = UErrorCode::U_ZERO_ERROR;
    timeStamp = resSdf->parse(dateTime2, status);
    EXPECT_EQ(timeStamp, 1653448174000);
    status = UErrorCode::U_ZERO_ERROR;
    timeStamp = resSdf->parse(dateTime3, status);
    EXPECT_EQ(timeStamp, 0);

    status = UErrorCode::U_ZERO_ERROR;
    icu::UnicodeString pattern2("yyyyy.MMMMM.dd GGG hh:mm::ss aaa");
    icu::SimpleDateFormat sdf2(pattern2, status);
    JSDateTimeFormat::SetIcuSimpleDateFormat(thread, dtf, sdf2, JSDateTimeFormat::FreeSimpleDateFormat);
    icu::SimpleDateFormat *resSdf2 = dtf->GetIcuSimpleDateFormat();
    timeStamp = resSdf2->parse(dateTime1, status);
    EXPECT_EQ(timeStamp, 0);
    status = UErrorCode::U_ZERO_ERROR;
    timeStamp = resSdf2->parse(dateTime2, status);
    EXPECT_EQ(timeStamp, 0);
    status = UErrorCode::U_ZERO_ERROR;
    timeStamp = resSdf2->parse(dateTime3, status);
    EXPECT_EQ(timeStamp, 1653448174000);
}

/**
 * @tc.name: InitializeDateTimeFormat
 * @tc.desc: Initialize the time and date format through localization label locales and options.
 *           Options can include year, month, day, hour, minute, second, time zone and weekday, etc.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, InitializeDateTimeFormat)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    // Create locales.
    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    icu::Locale icuLocale("zh", "Hans", "Cn", "calendar=chinese");
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    // Create options.
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL);
    // Initialize DateTimeFormat.
    JSHandle<JSTaggedValue> dtfCtor = env->GetDateTimeFormatFunction();
    JSHandle<JSDateTimeFormat> dtf =
        JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dtfCtor), dtfCtor));
    dtf = JSDateTimeFormat::InitializeDateTimeFormat(
        thread, dtf, JSHandle<JSTaggedValue>::Cast(locales), JSHandle<JSTaggedValue>::Cast(options));

    JSHandle<JSTaggedValue> localeTagVal(thread, dtf->GetLocale());
    JSHandle<EcmaString> localeEcmaStr = JSHandle<EcmaString>::Cast(localeTagVal);
    std::string localeStr = JSLocale::ConvertToStdString(localeEcmaStr);
    EXPECT_STREQ(localeStr.c_str(), "zh-Hans-CN-u-ca-chinese");
}

/**
 * @tc.name: ToDateTimeOptions
 * @tc.desc: Empty or incomplete option objects are supplemented according to the required option and default option.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, ToDateTimeOptions_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> yearKey = globalConst->GetHandledYearString();
    JSHandle<JSTaggedValue> monthKey = globalConst->GetHandledMonthString();
    JSHandle<JSTaggedValue> dayKey = globalConst->GetHandledDayString();
    JSHandle<JSTaggedValue> hourKey = globalConst->GetHandledHourString();
    JSHandle<JSTaggedValue> minuteKey = globalConst->GetHandledMinuteString();
    JSHandle<JSTaggedValue> secondKey = globalConst->GetHandledSecondString();

    // When the option value is blank, it will be set according to the default option,
    // including the year, month, day, hour, minute and second, and the values are all numeric.
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL); // test "ALL"
    auto yearEcmaStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, yearKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(yearEcmaStr).c_str(), "numeric");
    auto monthEcmaStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, monthKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(monthEcmaStr).c_str(), "numeric");
    auto dayEcmaStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, dayKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(dayEcmaStr).c_str(), "numeric");
    auto hourEcmaStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, hourKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(hourEcmaStr).c_str(), "numeric");
    auto minuteEcmaStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, minuteKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(minuteEcmaStr).c_str(), "numeric");
    auto secondEcmaStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, secondKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(secondEcmaStr).c_str(), "numeric");
}

HWTEST_F_L0(JSDateTimeFormatTest, ToDateTimeOptions_002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> weekdayKey = globalConst->GetHandledWeekdayString();
    JSHandle<JSTaggedValue> yearKey = globalConst->GetHandledYearString();
    JSHandle<JSTaggedValue> monthKey = globalConst->GetHandledMonthString();
    JSHandle<JSTaggedValue> dayKey = globalConst->GetHandledDayString();
    JSHandle<JSTaggedValue> dayPeriodKey = globalConst->GetHandledDayPeriodString();
    JSHandle<JSTaggedValue> hourKey = globalConst->GetHandledHourString();
    JSHandle<JSTaggedValue> minuteKey = globalConst->GetHandledMinuteString();
    JSHandle<JSTaggedValue> secondKey = globalConst->GetHandledSecondString();
    JSHandle<JSTaggedValue> fracSecKey = globalConst->GetHandledFractionalSecondDigitsString();

    // When the option value is not empty, it will be set according to the required options.
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    std::map<std::string, std::string> dateOptionsMap {
        { "weekday", "narrow" },
        { "year", "2-digit" },
        { "month", "2-digit" },
        { "day", "2-digit" }
    };
    std::map<std::string, std::string> timeOptionsMap {
        { "dayPeriod", "narrow" },
        { "hour", "2-digit" },
        { "minute", "2-digit" },
        { "second", "2-digit" },
        { "fractionalSecond", "1" }
    };
    SetDateOptionsTest(thread, options, dateOptionsMap);
    SetTimeOptionsTest(thread, options, timeOptionsMap);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL); // test "ANY"
    auto weekdayStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, weekdayKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(weekdayStr).c_str(), "narrow");
    auto yearStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, yearKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(yearStr).c_str(), "2-digit");
    auto monthStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, monthKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(monthStr).c_str(), "2-digit");
    auto dayStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, dayKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(dayStr).c_str(), "2-digit");

    auto dayPeriodStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, dayPeriodKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(dayPeriodStr).c_str(), "narrow");
    auto hourStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, hourKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(hourStr).c_str(), "2-digit");
    auto minuteStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, minuteKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(minuteStr).c_str(), "2-digit");
    auto secondStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, secondKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(secondStr).c_str(), "2-digit");
    auto fracSecStr = JSHandle<EcmaString>::Cast(JSObject::GetProperty(thread, options, fracSecKey).GetValue());
    EXPECT_STREQ(JSLocale::ConvertToStdString(fracSecStr).c_str(), "1");
}

JSHandle<JSDateTimeFormat> CreateDateTimeFormatTest(JSThread *thread, icu::Locale icuLocale, JSHandle<JSObject> options)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSTaggedValue> dtfCtor = env->GetDateTimeFormatFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    JSHandle<JSDateTimeFormat> dtf =
        JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dtfCtor), dtfCtor));

    JSHandle<JSTaggedValue> optionsVal = JSHandle<JSTaggedValue>::Cast(options);
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    dtf = JSDateTimeFormat::InitializeDateTimeFormat(thread, dtf, JSHandle<JSTaggedValue>::Cast(locales), optionsVal);
    return dtf;
}
/**
 * @tc.name: FormatDateTime
 * @tc.desc: Convert floating-point timestamp to fixed format time date through time date format.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, FormatDateTime_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsEmtpy = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h12"));
    JSObject::SetProperty(thread, optionsEmtpy, hourCycleKey, hourCycleValue);
    JSHandle<JSDateTimeFormat> dtf = CreateDateTimeFormatTest(thread, icuLocale, optionsEmtpy);

    double timeStamp1 = 1653448174000; // test "2022-05-25 11:09:34.000"
    double timeStamp2 = 1653921012999; // test "2022-05-30 22:30:12.999"

    // When the option is blank, the default format is "yyyy/MM/dd", the year, month and day are all numeric,
    // because the default option in initialization is "DefaultsOption::DATE".
    JSHandle<EcmaString> dateTimeEcamStr1 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp1);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr1).c_str(), "2022/5/25");
    JSHandle<EcmaString> dateTimeEcamStr2 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp2);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr2).c_str(), "2022/5/30");
}

HWTEST_F_L0(JSDateTimeFormatTest, FormatDateTime_002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsEmtpy = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h12"));
    JSObject::SetProperty(thread, optionsEmtpy, hourCycleKey, hourCycleValue);
    JSHandle<JSObject> options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(optionsEmtpy), RequiredOption::ANY, DefaultsOption::ALL);
    JSHandle<JSDateTimeFormat> dtf = CreateDateTimeFormatTest(thread, icuLocale, options);

    double timeStamp1 = 1653448174000; // test "2022-05-25 11:09:34.000"
    double timeStamp2 = 1653921012999; // test "2022-05-30 22:30:12.999"

    // Format to include all options by "DefaultsOption::ALL".
    JSHandle<EcmaString> dateTimeEcamStr1 = JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp1);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr1).c_str(), "2022/5/25 上午11:09:34");
    JSHandle<EcmaString> dateTimeEcamStr2 = JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp2);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr2).c_str(), "2022/5/30 下午10:30:12");
}

HWTEST_F_L0(JSDateTimeFormatTest, FormatDateTime_003)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsEmtpy = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h12"));
    JSObject::SetProperty(thread, optionsEmtpy, hourCycleKey, hourCycleValue);

    // Set custom date time format.
    JSHandle<JSObject> options = optionsEmtpy;
    std::map<std::string, std::string> dateOptionsMap {
        { "weekday", "long" },
        { "year", "2-digit" },
        { "month", "2-digit" },
        { "day", "2-digit" }
    };
    std::map<std::string, std::string> timeOptionsMap {
        { "dayPeriod", "long" },
        { "hour", "2-digit" },
        { "minute", "2-digit" },
        { "second", "2-digit" },
        { "fractionalSecond", "3" }
    };
    SetDateOptionsTest(thread, options, dateOptionsMap);
    SetTimeOptionsTest(thread, options, timeOptionsMap);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL);
    JSHandle<JSDateTimeFormat> dtf = CreateDateTimeFormatTest(thread, icuLocale, options);

    double timeStamp1 = 1653448174000; // test "2022-05-25 11:09:34.000"
    double timeStamp2 = 1653921012999; // test "2022-05-30 22:30:12.999"

    JSHandle<EcmaString> dateTimeEcamStr1 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp1);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr1).c_str(), "22年05月25日星期三 上午11:09:34.000");
    JSHandle<EcmaString> dateTimeEcamStr2 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp2);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr2).c_str(), "22年05月30日星期一 晚上10:30:12.999");
}

HWTEST_F_L0(JSDateTimeFormatTest, FormatDateTime_004)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    icu::Locale icuLocale("en", "Hans", "US");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsEmtpy = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h12"));
    JSObject::SetProperty(thread, optionsEmtpy, hourCycleKey, hourCycleValue);

    // Set custom date time format.
    JSHandle<JSObject> options = optionsEmtpy;
    std::map<std::string, std::string> dateOptionsMap {
        { "weekday", "long" },
        { "year", "2-digit" },
        { "month", "2-digit" },
        { "day", "2-digit" }
    };
    std::map<std::string, std::string> timeOptionsMap {
        { "dayPeriod", "long" },
        { "hour", "2-digit" },
        { "minute", "2-digit" },
        { "second", "2-digit" },
        { "fractionalSecond", "3" }
    };
    SetDateOptionsTest(thread, options, dateOptionsMap);
    SetTimeOptionsTest(thread, options, timeOptionsMap);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL);
    JSHandle<JSDateTimeFormat> dtf = CreateDateTimeFormatTest(thread, icuLocale, options);

    double timeStamp1 = 1653448174000; // test "2022-05-25 11:09:34.000"
    double timeStamp2 = 1653921012999; // test "2022-05-30 22:30:12.999"

    JSHandle<EcmaString> dateTimeEcamStr1 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp1);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr1).c_str(),
        "Wednesday, 05/25/22, 11:09:34.000 in the morning");
    JSHandle<EcmaString> dateTimeEcamStr2 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp2);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr2).c_str(),
        "Monday, 05/30/22, 10:30:12.999 at night");
}

std::string GetDateTimePartStringTest(JSThread *thread, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> part)
{
    JSHandle<JSObject> partObj = JSHandle<JSObject>::Cast(part);
    JSHandle<JSTaggedValue> partValue = JSObject::GetProperty(thread, partObj, key).GetValue();
    JSHandle<EcmaString> partEcmaStr = JSHandle<EcmaString>::Cast(partValue);
    std::string partStr = JSLocale::ConvertToStdString(partEcmaStr);
    return partStr;
}

/**
 * @tc.name: FormatDateTimeToParts
 * @tc.desc: Convert floating-point timestamp to fixed format time date through time date format.
 *           The "FormatDateTimeToParts" method converts the output result into an array containing various time and
 *           date attributes.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, FormatDateTimeToParts_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();

    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsEmtpy = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h12"));
    JSObject::SetProperty(thread, optionsEmtpy, hourCycleKey, hourCycleValue);
    JSHandle<JSDateTimeFormat> dtf = CreateDateTimeFormatTest(thread, icuLocale, optionsEmtpy);

    double timeStamp = 1653448174123; // test "2022-05-25 11:09:34.123"
    // Use default date time format and format date and time to parts.
    JSHandle<EcmaString> dateTimeEcamStr1 =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr1).c_str(), "2022/5/25");
    JSHandle<JSArray> dateTimeArray1 = JSDateTimeFormat::FormatDateTimeToParts(thread, dtf, timeStamp);
    auto year = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray1), 0).GetValue();
    auto literal1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray1), 1).GetValue();
    auto month = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray1), 2).GetValue();
    auto literal2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray1), 3).GetValue();
    auto day = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray1), 4).GetValue();
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, year).c_str(), "year");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, year).c_str(), "2022");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal1).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal1).c_str(), "/");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, month).c_str(), "month");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, month).c_str(), "5");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal2).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal2).c_str(), "/");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, day).c_str(), "day");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, day).c_str(), "25");
}

HWTEST_F_L0(JSDateTimeFormatTest, FormatDateTimeToParts_002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> hourCycleKey = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h12"));
    JSObject::SetProperty(thread, options, hourCycleKey, hourCycleValue);

    double timeStamp = 1653448174123; // test "2022-05-25 11:09:34.123"
    // Set custom date time format and format date and time to parts.
    std::map<std::string, std::string> dateOptionsMap {
        { "weekday", "long" },
        { "year", "2-digit" },
        { "month", "2-digit" },
        { "day", "2-digit" }
    };
    std::map<std::string, std::string> timeOptionsMap {
        { "dayPeriod", "long" },
        { "hour", "2-digit" },
        { "minute", "2-digit" },
        { "second", "2-digit" },
        { "fractionalSecond", "3" }
    };
    SetDateOptionsTest(thread, options, dateOptionsMap);
    SetTimeOptionsTest(thread, options, timeOptionsMap);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL);
    JSHandle<JSDateTimeFormat> dtf = CreateDateTimeFormatTest(thread, icuLocale, options);
    JSHandle<EcmaString> dateTimeEcamStr =  JSDateTimeFormat::FormatDateTime(thread, dtf, timeStamp);
    EXPECT_STREQ(JSLocale::ConvertToStdString(dateTimeEcamStr).c_str(), "22年05月25日星期三 上午11:09:34.123");

    JSHandle<JSArray> dateTimeArray = JSDateTimeFormat::FormatDateTimeToParts(thread, dtf, timeStamp);
    auto year = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 0).GetValue();
    auto literal1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 1).GetValue();
    auto month = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 2).GetValue();
    auto literal2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 3).GetValue();
    auto day = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 4).GetValue();
    auto literal3 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 5).GetValue();
    auto weekday = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 6).GetValue();
    auto literal4 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 7).GetValue();
    auto dayPeriod = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 8).GetValue();
    auto hour = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 9).GetValue();
    auto literal5 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 10).GetValue();
    auto minute = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 11).GetValue();
    auto literal6 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 12).GetValue();
    auto second = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 13).GetValue();
    auto literal7 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 14).GetValue();
    auto fracSec = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(dateTimeArray), 15).GetValue();
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, year).c_str(), "year");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, year).c_str(), "22");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal1).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal1).c_str(), "年");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, month).c_str(), "month");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, month).c_str(), "05");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal2).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal2).c_str(), "月");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, day).c_str(), "day");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, day).c_str(), "25");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal3).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal3).c_str(), "日");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, weekday).c_str(), "weekday");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, weekday).c_str(), "星期三");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal4).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal4).c_str(), " ");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, dayPeriod).c_str(), "dayPeriod");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, dayPeriod).c_str(), "上午");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, hour).c_str(), "hour");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, hour).c_str(), "11");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal5).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal5).c_str(), ":");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, minute).c_str(), "minute");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, minute).c_str(), "09");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal6).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal6).c_str(), ":");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, second).c_str(), "second");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, second).c_str(), "34");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, literal7).c_str(), "literal");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, literal7).c_str(), ".");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, typeKey, fracSec).c_str(), "fractionalSecond");
    EXPECT_STREQ(GetDateTimePartStringTest(thread, valueKey, fracSec).c_str(), "123");
}
/**
 * @tc.name: GainAvailableLocales
 * @tc.desc: Get the available localized label array. If the global time date localized label is not set, return an
 *           array containing all available labels. Otherwise, return an array containing self-defined labels.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDateTimeFormatTest, GainAvailableLocales)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    // The date and time format locales is not initialized,
    // then get all available locales and save them in a 'TaggedArray'.
    JSHandle<JSTaggedValue> dateTimeFormatLocales = env->GetDateTimeFormatLocales();
    EXPECT_EQ(dateTimeFormatLocales.GetTaggedValue(), JSTaggedValue::Undefined());

    const char *key = "calendar";
    const char *path = nullptr;
    JSHandle<TaggedArray> availableLocales = JSLocale::GetAvailableLocales(thread, key, path);
    env->SetDateTimeFormatLocales(thread, availableLocales);
    JSHandle<TaggedArray> gainLocales1 = JSDateTimeFormat::GainAvailableLocales(thread);
    EXPECT_EQ(JSHandle<JSTaggedValue>::Cast(gainLocales1).GetTaggedValue().GetRawData(),
        JSHandle<JSTaggedValue>::Cast(availableLocales).GetTaggedValue().GetRawData());

    // The date and time format locales has already been initialized,
    // then get custom locale and save it in a 'TaggedArray'.
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSTaggedValue> dtfCtor = env->GetDateTimeFormatFunction();

    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    icu::Locale icuLocale("zh", "Hans", "Cn", "calendar=chinese");
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    options = JSDateTimeFormat::ToDateTimeOptions(
        thread, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY, DefaultsOption::ALL);
    JSHandle<JSDateTimeFormat> dtf =
        JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dtfCtor), dtfCtor));
    dtf = JSDateTimeFormat::InitializeDateTimeFormat(
        thread, dtf, JSHandle<JSTaggedValue>::Cast(locales), JSHandle<JSTaggedValue>::Cast(options));

    JSHandle<JSTaggedValue> localeTagVal(thread, dtf->GetLocale());
    JSHandle<TaggedArray> localesTagArr = factory->NewTaggedArray(1);
    localesTagArr->Set(thread, 0, localeTagVal);
    env->SetDateTimeFormatLocales(thread, localesTagArr);
    JSHandle<TaggedArray> gainLocales2 = JSDateTimeFormat::GainAvailableLocales(thread);
    EXPECT_EQ(gainLocales2->GetLength(), 1U);
    EXPECT_STREQ(CString(JSHandle<EcmaString>(thread, gainLocales2->Get(0))->GetCString().get()).c_str(),
        "zh-Hans-CN-u-ca-chinese");
}
} // namespace panda::test
