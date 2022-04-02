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

#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsArray = ecmascript::builtins::BuiltinsArray;
class BuiltinsDateTimeFormatTest : public testing::Test {
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
        options.SetIcuDataPath("./../../third_party/icu/ohos_icu4j/data");
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

// new DateTimeFormat(locale)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, DateTimeFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromCanBeCompressString("en-US"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSDateTimeFormat());
}

static JSTaggedValue BuiltinsDateTimeOptionsSet(JSThread *thread)
{
    auto globalConst = thread->GlobalConstants();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> weekDay = globalConst->GetHandledWeekdayString();
    JSHandle<JSTaggedValue> dayPeriod = globalConst->GetHandledDayPeriodString();
    JSHandle<JSTaggedValue> hourCycle = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> timeZone = globalConst->GetHandledTimeZoneString();
    JSHandle<JSTaggedValue> numicValue(factory->NewFromCanBeCompressString("numeric")); // test numeric
    JSHandle<JSTaggedValue> weekDayValue(factory->NewFromCanBeCompressString("short")); // test short
    JSHandle<JSTaggedValue> dayPeriodValue(factory->NewFromCanBeCompressString("long")); // test long
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromCanBeCompressString("h24")); // test h24
    JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromCanBeCompressString("UTC")); // test UTC

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(6); // 6 : 6 length
    keyArray->Set(thread, 0, globalConst->GetHandledYearString()); // 0 : 0 first position
    keyArray->Set(thread, 1, globalConst->GetHandledMonthString()); // 1 : 1 second position
    keyArray->Set(thread, 2, globalConst->GetHandledDayString()); // 2 : 2 third position
    keyArray->Set(thread, 3, globalConst->GetHandledHourString()); // 3 : 3 fourth position
    keyArray->Set(thread, 4, globalConst->GetHandledMinuteString()); // 4 : 4 fifth position
    keyArray->Set(thread, 5, globalConst->GetHandledSecondString()); // 5 : 5 sixth position

    uint32_t arrayLen = keyArray->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < arrayLen; i++) {
        key.Update(keyArray->Get(thread, i));
        JSObject::SetProperty(thread, optionsObj, key, numicValue);
    }
    JSObject::SetProperty(thread, optionsObj, weekDay, weekDayValue);
    JSObject::SetProperty(thread, optionsObj, dayPeriod, dayPeriodValue);
    JSObject::SetProperty(thread, optionsObj, hourCycle, hourCycleValue);
    JSObject::SetProperty(thread, optionsObj, timeZone, timeZoneValue);
    return optionsObj.GetTaggedValue();
}

static JSTaggedValue JSDateTimeFormatCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());
    JSHandle<JSObject> optionsObj = JSHandle<JSObject>(thread, BuiltinsDateTimeOptionsSet(thread));

    JSHandle<JSTaggedValue> localesString = locale;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsJSDateTimeFormat());
    return result;
}

static double BuiltinsDateCreate(const double year, const double month, const double date)
{
    const double day = JSDate::MakeDay(year, month, date);
    const double time = JSDate::MakeTime(0, 0, 0, 0); // 24:00:00
    double days = JSDate::MakeDate(day, time);
    return days;
}

// Format.Tostring(en-US)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, Format_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsDateTimeFormat::Format(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    // jsDate supports zero to eleven, the month should be added with one
    JSHandle<JSFunction> jsFunction(thread, result1);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    double days = BuiltinsDateCreate(2020, 10, 1);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<double>(days)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromCanBeCompressString("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, value.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsArray::ToString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result2);
    EXPECT_STREQ("Sun, 11/1/2020, 24:00:00", CString(resultStr->GetCString().get()).c_str());
}

// Format.Tostring(pt-BR)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, Format_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("pt-BR"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsDateTimeFormat::Format(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSFunction> jsFunction(thread, result1);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    double days = BuiltinsDateCreate(2020, 5, 11);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<double>(days)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromCanBeCompressString("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, value.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsArray::ToString(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result2);
    EXPECT_STREQ("qui, 11/06/2020 24:00:00", CString(resultStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 16U); // sixteen formatters
}

// FormatRange(zh)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRange_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("zh"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 10, 1);
    double days2 = BuiltinsDateCreate(2021, 6, 1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(days1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(days2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatRange(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result);
    EXPECT_EQ("2020/11/1周日 24:00:00 – 2021/7/1周四 24:00:00", CString(resultStr->GetCString().get()).c_str());
}

// FormatRange(en)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRange_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 12, 1);
    double days2 = BuiltinsDateCreate(2021, 2, 1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(days1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(days2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatRange(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result);
    EXPECT_EQ("Fri, 1/1/2021, 24:00:00 – Mon, 3/1/2021, 24:00:00", CString(resultStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRangeToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 12, 1);
    double days2 = BuiltinsDateCreate(2021, 2, 1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(days1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(days2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatRangeToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 39U); // The number of characters of "Fri1/1/202124:00:00–Mon3/1/202124:00:00"
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("de-ID"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsDateTimeFormat::ResolvedOptions(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jsdatetimeformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromCanBeCompressString("de"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> timeZone = globalConst->GetHandledTimeZoneString();
    JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromCanBeCompressString("UTC"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, timeZone).GetValue(), timeZoneValue), true);
}

// SupportedLocalesOf("best fit")
HWTEST_F_L0(BuiltinsDateTimeFormatTest, SupportedLocalesOf_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("id-u-co-pinyin-de-ID"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    // set the tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue resultArr = BuiltinsDateTimeFormat::SupportedLocalesOf(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);

    JSHandle<EcmaString> resultStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", CString(resultStr->GetCString().get()).c_str());
}

// SupportedLocalesOf("look up")
HWTEST_F_L0(BuiltinsDateTimeFormatTest, SupportedLocalesOf_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
   
    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromCanBeCompressString("lookup"));
    JSHandle<JSTaggedValue> locale(factory->NewFromCanBeCompressString("id-u-co-pinyin-de-DE"));

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
   
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue resultArr = BuiltinsDateTimeFormat::SupportedLocalesOf(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);

    JSHandle<EcmaString> resultStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de", CString(resultStr->GetCString().get()).c_str());
}
}  // namespace panda::test