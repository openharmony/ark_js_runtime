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

#include "ecmascript/builtins/builtins_relative_time_format.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsRelativeTimeFormatTest : public testing::Test {
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

// new RelativeTimeFormat(newTarget is undefined)
HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, RelativeTimeFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetRelativeTimeFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::RelativeTimeFormatConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSRelativeTimeFormat());
}

static JSTaggedValue JSRelativeTimeFormatCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                              JSHandle<JSTaggedValue> &numericValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetRelativeTimeFormatFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledNumericString();
    JSObject::SetProperty(thread, optionsObj, typeKey, numericValue);

    JSHandle<JSTaggedValue> localesString = locale;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::RelativeTimeFormatConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsJSRelativeTimeFormat());
    return result;
}

// format(1, auto)
HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, Format_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("auto")); // the default value
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("day"));
    JSHandle<JSTaggedValue> numberValue(thread, JSTaggedValue(1));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, unitValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("tomorrow", CString(handleEcmaStr->GetCString().get()).c_str());
}

// format(0, auto)
HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, Format_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("auto"));
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("day"));
    JSHandle<JSTaggedValue> numberValue(thread, JSTaggedValue(0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, unitValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("today", CString(handleEcmaStr->GetCString().get()).c_str());
}

// format(-1, auto)
HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, Format_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("auto")); // the default value
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("day"));
    JSHandle<JSTaggedValue> numberValue(thread, JSTaggedValue(-1));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, unitValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("yesterday", CString(handleEcmaStr->GetCString().get()).c_str());
}

// format(-1, always)
HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, Format_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("always")); // the default value
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("day"));
    JSHandle<JSTaggedValue> numberValue(thread, JSTaggedValue(-1));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, unitValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("1 day ago", CString(handleEcmaStr->GetCString().get()).c_str());
}

// format(1, always)
HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, Format_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("always")); // the default value
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("day"));
    JSHandle<JSTaggedValue> numberValue(thread, JSTaggedValue(1));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, unitValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("in 1 day", CString(handleEcmaStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, FormatToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("always"));
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("seconds"));
    JSHandle<JSTaggedValue> numberValue(thread, JSTaggedValue(10));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberValue.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, unitValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultArr(thread, result);
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSObject>(resultArr), lengthKey).GetValue()->GetInt(), 3);
}

HWTEST_F_L0(BuiltinsRelativeTimeFormatTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("auto"));
    JSHandle<JSRelativeTimeFormat> jsPluralRules =
        JSHandle<JSRelativeTimeFormat>(thread, JSRelativeTimeFormatCreateWithLocaleTest(
                                        thread, locale, numericValue));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsRelativeTimeFormat::ResolvedOptions(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jsrelativetimeformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), locale), true);
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("long"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, styleKey).GetValue(), styleValue), true);
    JSHandle<JSTaggedValue> numberingSystemKey = globalConst->GetHandledNumberingSystemString();
    JSHandle<JSTaggedValue> numberingSystemValue(factory->NewFromASCII("latn"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, numberingSystemKey).GetValue(), numberingSystemValue), true);
}
} // namespace panda::test
