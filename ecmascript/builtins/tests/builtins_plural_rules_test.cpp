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

#include "ecmascript/builtins/builtins_plural_rules.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsPluralRulesTest : public testing::Test {
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

// new DateTimeFormat(newTarget is defined)
HWTEST_F_L0(BuiltinsPluralRulesTest, PluralRulesConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetPluralRulesFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::PluralRulesConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSPluralRules());
}

static JSTaggedValue JSPluralRulesCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                       JSHandle<JSTaggedValue> &typeValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetPluralRulesFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledTypeString();
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);

    JSHandle<JSTaggedValue> localesString = locale;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::PluralRulesConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSPluralRules());
    return result;
}

// select(0, type(cardinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("cardinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));
        
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("other", CString(handleEcmaStr->GetCString().get()).c_str());
}

// select(1, type(cardinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("cardinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));
        
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("one", CString(handleEcmaStr->GetCString().get()).c_str());
}

// select(2, type(cardinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("cardinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));
        
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("other", CString(handleEcmaStr->GetCString().get()).c_str());
}

// select(3, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(3));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("few", CString(handleEcmaStr->GetCString().get()).c_str());
}

// select(2, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));
        
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("two", CString(handleEcmaStr->GetCString().get()).c_str());
}

// select(0, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_006)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("other", CString(handleEcmaStr->GetCString().get()).c_str());
}

// select(1, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_007)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("one", CString(handleEcmaStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsPluralRulesTest, SupportedLocalesOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("id-u-co-pinyin-de-ID"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    // set the tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue resultArr = BuiltinsPluralRules::SupportedLocalesOf(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", CString(handleEcmaStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsPluralRulesTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsPluralRules::ResolvedOptions(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jspluralrulesformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromASCII("en"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, typeKey).GetValue(), typeValue), true);
}
} // namespace panda::test
