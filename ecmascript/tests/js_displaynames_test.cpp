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

#include "ecmascript/js_displaynames.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSDisplayNamesTest : public testing::Test {
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

/**
 * @tc.name: GetIcuLocaleDisplayNames
 * @tc.desc: Call "SetIcuLocaleDisplayNames" function Set IcuLocale DisplayNames,check whether the IcuLocale
 *           DisplayNames through "GetIcuLocaleDisplayNames" function is within expectations then call "getLocale"
 *           function display locale and check the locale is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDisplayNamesTest, GetIcuLocaleDisplayNames)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetDisplayNamesFunction();
    JSHandle<JSDisplayNames> displayNames =
        JSHandle<JSDisplayNames>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    
    icu::Locale icuLocale("en");
    UDisplayContext display_context[] = {UDISPCTX_LENGTH_SHORT};
    icu::LocaleDisplayNames* iculocaledisplaynames =
        icu::LocaleDisplayNames::createInstance(icuLocale, display_context, 1);
    EXPECT_TRUE(iculocaledisplaynames != nullptr);
    JSDisplayNames::SetIcuLocaleDisplayNames(
        thread, displayNames, iculocaledisplaynames, JSDisplayNames::FreeIcuLocaleDisplayNames);
    icu::LocaleDisplayNames *resultIculocaledisplaynames = displayNames->GetIcuLocaleDisplayNames();
    EXPECT_TRUE(iculocaledisplaynames == resultIculocaledisplaynames);
    JSHandle<EcmaString> localeStr = JSLocale::ToLanguageTag(thread, resultIculocaledisplaynames->getLocale());
    EXPECT_STREQ(CString(localeStr->GetCString().get()).c_str(), "en");
}

/**
 * @tc.name: GetAvailableLocales
 * @tc.desc: Call function "GetAvailableLocales" to obtain the available locale from the ICU library and
 *           check whether the obtained locale is empty.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDisplayNamesTest, GetAvailableLocales)
{
    JSHandle<TaggedArray> displayLocale = JSDisplayNames::GetAvailableLocales(thread);
    uint32_t localeLen = displayLocale->GetLength();
    EXPECT_NE(localeLen, 0U);

    for (uint32_t i = 0; i < localeLen; i++) {
        EXPECT_FALSE(displayLocale->Get(i).IsHole());
    }
}

void SetOptionProperties(JSThread *thread, JSHandle<JSObject> &optionsObj,
                         std::map<std::string, std::string> &displayOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    // display options keys
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> fallBackKey = globalConst->GetHandledFallbackString();
    // display options value
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII(displayOptions["style"].c_str()));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII(displayOptions["type"].c_str()));
    JSHandle<JSTaggedValue> fallBackValue(factory->NewFromASCII(displayOptions["fallback"].c_str()));
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);
    JSObject::SetProperty(thread, optionsObj, fallBackKey, fallBackValue);
}

/**
 * @tc.name: InitializeDisplayNames
 * @tc.desc: Call function "InitializeDisplayNames" to initialize the jsdisplaynames class object and check whether the
 *           properties of the object is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDisplayNamesTest, InitializeDisplayNames)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetDisplayNamesFunction();
    JSHandle<JSDisplayNames> displayNames =
        JSHandle<JSDisplayNames>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> displayOptions = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> localeStr(factory->NewFromASCII("en-US"));
    // options is empty
    JSDisplayNames::InitializeDisplayNames(thread, displayNames, localeStr, JSHandle<JSTaggedValue>(displayOptions));
    // Default attribute value and type is undefiend throw a expection
    EXPECT_EQ(displayNames->GetStyle(), StyOption::LONG);
    EXPECT_EQ(displayNames->GetType(), TypednsOption::EXCEPTION);
    EXPECT_EQ(displayNames->GetFallback(), FallbackOption::EXCEPTION);
    EXPECT_TRUE(thread->HasPendingException());
    thread->ClearException();
    // options of the key and value
    std::map<std::string, std::string> displayOptionsProperty {
        { "style", "short" },
        { "type", "script" },
        { "fallback", "none" },
    };
    SetOptionProperties(thread, displayOptions, displayOptionsProperty);
    // options is not empty
    JSDisplayNames::InitializeDisplayNames(thread, displayNames, localeStr, JSHandle<JSTaggedValue>(displayOptions));
    JSHandle<EcmaString> setlocale(thread, displayNames->GetLocale());
    EXPECT_EQ(displayNames->GetStyle(), StyOption::SHORT);
    EXPECT_EQ(displayNames->GetType(), TypednsOption::SCRIPT);
    EXPECT_EQ(displayNames->GetFallback(), FallbackOption::NONE);
    EXPECT_STREQ(CString(setlocale->GetCString().get()).c_str(), "en-US");
    EXPECT_TRUE(displayNames->GetIcuLocaleDisplayNames() != nullptr);
}

/**
 * @tc.name: CanonicalCodeForDisplayNames
 * @tc.desc: Display the language region and script of the locale according to the display configuration of
 *           different regions.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDisplayNamesTest, CanonicalCodeForDisplayNames)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetDisplayNamesFunction();
    JSHandle<JSDisplayNames> displayNames =
        JSHandle<JSDisplayNames>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> displayOptions = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-Hant"));
    // test UDISPCTX_LENGTH_SHORT
    std::map<std::string, std::string> displayOptionsProperty {
        { "style", "narrow" },
        { "type", "script" },
        { "fallback", "none" },
    };
    SetOptionProperties(thread, displayOptions, displayOptionsProperty);
    JSHandle<JSDisplayNames> initDisplayNames =
        JSDisplayNames::InitializeDisplayNames(thread, displayNames, locale, JSHandle<JSTaggedValue>(displayOptions));
    // CanonicalCode for script
    JSHandle<EcmaString> code = factory->NewFromASCII("Kana");
    JSHandle<EcmaString> resultDisplay =
        JSDisplayNames::CanonicalCodeForDisplayNames(thread, initDisplayNames, initDisplayNames->GetType(), code);
    EXPECT_STREQ(CString(resultDisplay->GetCString().get()).c_str(), "片假名");
    // CanonicalCode for languege
    code = factory->NewFromASCII("fr");
    initDisplayNames->SetType(TypednsOption::LANGUAGE);
    resultDisplay =
        JSDisplayNames::CanonicalCodeForDisplayNames(thread, initDisplayNames, initDisplayNames->GetType(), code);
    EXPECT_STREQ(CString(resultDisplay->GetCString().get()).c_str(), "法文");
    // CanonicalCode for region
    code = factory->NewFromASCII("US");
    initDisplayNames->SetType(TypednsOption::REGION);
    resultDisplay =
        JSDisplayNames::CanonicalCodeForDisplayNames(thread, initDisplayNames, initDisplayNames->GetType(), code);
    EXPECT_STREQ(CString(resultDisplay->GetCString().get()).c_str(), "美國");
}

/**
 * @tc.name: ResolvedOptions
 * @tc.desc: Call function "InitializeDisplayNames" to initialize the jsdisplaynames class object and Copy the
 *           properties of jsdisplaynames class to a new object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSDisplayNamesTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> fallBackKey = globalConst->GetHandledFallbackString();

    JSHandle<JSTaggedValue> ctor = env->GetDisplayNamesFunction();
    JSHandle<JSDisplayNames> displayNames =
        JSHandle<JSDisplayNames>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> displayOptions = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-Hant"));
    
    std::map<std::string, std::string> displayOptionsProperty {
        { "style", "short" },
        { "type", "region" },
        { "fallback", "code" },
    };
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII(displayOptionsProperty["style"].c_str()));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII(displayOptionsProperty["type"].c_str()));
    JSHandle<JSTaggedValue> fallBackValue(factory->NewFromASCII(displayOptionsProperty["fallback"].c_str()));
    SetOptionProperties(thread, displayOptions, displayOptionsProperty);
    JSHandle<JSDisplayNames> initDisplayNames =
        JSDisplayNames::InitializeDisplayNames(thread, displayNames, locale, JSHandle<JSTaggedValue>(displayOptions));
    
    JSDisplayNames::ResolvedOptions(thread, initDisplayNames, displayOptions);
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, displayOptions, styleKey).GetValue(), styleValue), true);
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, displayOptions, localeKey).GetValue(), locale), true);
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, displayOptions, fallBackKey).GetValue(), fallBackValue), true);
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, displayOptions, typeKey).GetValue(), typeValue), true);
}
}  // namespace panda::test
