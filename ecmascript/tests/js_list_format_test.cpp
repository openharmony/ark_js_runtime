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
#include "ecmascript/js_array.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSListFormatTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSListFormatTest, Set_Get_IcuListFormatter_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetListFormatFunction();
    JSHandle<JSListFormat> jsFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::Locale icuLocale("en", "Latn", "US");
    icu::ListFormatter* icuFormatter = icu::ListFormatter::createInstance(icuLocale, status);
    JSListFormat::SetIcuListFormatter(thread, jsFormatter, icuFormatter, JSListFormat::FreeIcuListFormatter);
    icu::ListFormatter *resFormatter = jsFormatter->GetIcuListFormatter();
    EXPECT_TRUE(resFormatter != nullptr);

    const int32_t itemNum = 3;
    const icu::UnicodeString items[itemNum] = { "One", "Two", "Three" };
    icu::UnicodeString resStr = "";
    resStr = resFormatter->format(items, itemNum, resStr, status);
    const icu::UnicodeString expectResStr("One, Two, and Three");
    EXPECT_TRUE(resStr.compare(expectResStr) == 0);
}

HWTEST_F_L0(JSListFormatTest, Set_Get_IcuListFormatter_002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetListFormatFunction();
    JSHandle<JSListFormat> jsFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::Locale icuLocale("zh", "Hans", "Cn");
    icu::ListFormatter* icuFormatter = icu::ListFormatter::createInstance(icuLocale, status);
    JSListFormat::SetIcuListFormatter(thread, jsFormatter, icuFormatter, JSListFormat::FreeIcuListFormatter);
    icu::ListFormatter *resFormatter = jsFormatter->GetIcuListFormatter();
    EXPECT_TRUE(resFormatter != nullptr);

    const int32_t itemNum = 3;
    const icu::UnicodeString items[itemNum] = { "一", "二", "三" };
    icu::UnicodeString resStr = "";
    resStr = resFormatter->format(items, itemNum, resStr, status);
    const icu::UnicodeString expectResStr("一、二和三");
    EXPECT_TRUE(resStr.compare(expectResStr) == 0);
}

JSHandle<JSListFormat> CreateJSListFormatterTest(JSThread *thread, icu::Locale icuLocale, JSHandle<JSObject> options)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSTaggedValue> listCtor = env->GetListFormatFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    JSHandle<JSListFormat> listFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(listCtor), listCtor));

    JSHandle<JSTaggedValue> optionsVal = JSHandle<JSTaggedValue>::Cast(options);
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    listFormatter = JSListFormat::InitializeListFormat(thread, listFormatter,
        JSHandle<JSTaggedValue>::Cast(locales), optionsVal);
    return listFormatter;
}

void SetFormatterOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
    std::map<std::string, std::string> &options)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> localeMatcherKey = globalConst->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII(options["localeMatcher"].c_str()));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII(options["type"].c_str()));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII(options["style"].c_str()));
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
}

HWTEST_F_L0(JSListFormatTest, InitializeListFormat)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    icu::Locale icuLocale("en", "Latn", "US");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "conjunction" },
        { "style", "long" }
    };
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSTaggedValue> listCtor = env->GetListFormatFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    JSHandle<JSListFormat> listFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(listCtor), listCtor));

    JSHandle<JSTaggedValue> optionsVal = JSHandle<JSTaggedValue>::Cast(object);
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    listFormatter = JSListFormat::InitializeListFormat(thread, listFormatter,
        JSHandle<JSTaggedValue>::Cast(locales), optionsVal);
    icu::ListFormatter *resFormatter = listFormatter->GetIcuListFormatter();
    EXPECT_TRUE(resFormatter != nullptr);

    const int32_t itemNum = 3;
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    const icu::UnicodeString items[itemNum] = { "Monday", "Tuesday", "Wednesday" };
    icu::UnicodeString resStr = "";
    resStr = resFormatter->format(items, itemNum, resStr, status);
    const icu::UnicodeString expectResStr("Monday, Tuesday, and Wednesday");
    EXPECT_TRUE(resStr.compare(expectResStr) == 0);
}

HWTEST_F_L0(JSListFormatTest, FormatList_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    icu::Locale icuLocale("en", "Latn", "US");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "conjunction" },
        { "style", "long" }
    };
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSListFormat> jsFormatter = CreateJSListFormatterTest(thread, icuLocale, object);
    JSHandle<JSObject> valueObj = JSHandle<JSObject>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value0(factory->NewFromStdString("Zero"));
    JSHandle<JSTaggedValue> value1(factory->NewFromStdString("One"));
    JSHandle<JSTaggedValue> value2(factory->NewFromStdString("Two"));
    JSObject::SetProperty(thread, valueObj, key0, value0);
    JSObject::SetProperty(thread, valueObj, key1, value1);
    JSObject::SetProperty(thread, valueObj, key2, value2);
    JSHandle<JSArray> valueArr = JSHandle<JSArray>::Cast(valueObj);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(CString(valueStr->GetCString().get()).c_str(), "Zero, One, and Two");
}

HWTEST_F_L0(JSListFormatTest, FormatList_002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    icu::Locale icuLocale("en", "Latn", "US");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    // when style is narrow, type can only be unit
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "unit" },
        { "style", "narrow" }
    };
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSListFormat> jsFormatter = CreateJSListFormatterTest(thread, icuLocale, object);
    JSHandle<JSObject> valueObj = JSHandle<JSObject>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value0(factory->NewFromStdString("Zero"));
    JSHandle<JSTaggedValue> value1(factory->NewFromStdString("One"));
    JSHandle<JSTaggedValue> value2(factory->NewFromStdString("Two"));
    JSObject::SetProperty(thread, valueObj, key0, value0);
    JSObject::SetProperty(thread, valueObj, key1, value1);
    JSObject::SetProperty(thread, valueObj, key2, value2);
    JSHandle<JSArray> valueArr = JSHandle<JSArray>::Cast(valueObj);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(CString(valueStr->GetCString().get()).c_str(), "Zero One Two");
}

HWTEST_F_L0(JSListFormatTest, FormatList_003)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "disjunction" },
        { "style", "long" }
    };
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSListFormat> jsFormatter = CreateJSListFormatterTest(thread, icuLocale, object);
    JSHandle<JSObject> valueObj = JSHandle<JSObject>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value0(factory->NewFromStdString("苹果"));
    JSHandle<JSTaggedValue> value1(factory->NewFromStdString("梨子"));
    JSHandle<JSTaggedValue> value2(factory->NewFromStdString("桃"));
    JSObject::SetProperty(thread, valueObj, key0, value0);
    JSObject::SetProperty(thread, valueObj, key1, value1);
    JSObject::SetProperty(thread, valueObj, key2, value2);
    JSHandle<JSArray> valueArr = JSHandle<JSArray>::Cast(valueObj);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(CString(valueStr->GetCString().get()).c_str(), "苹果、梨子或桃");
}

std::string GetListPartStringTest(JSThread *thread, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> part)
{
    JSHandle<JSObject> partObj = JSHandle<JSObject>::Cast(part);
    JSHandle<JSTaggedValue> partValue = JSObject::GetProperty(thread, partObj, key).GetValue();
    JSHandle<EcmaString> partEcmaStr = JSHandle<EcmaString>::Cast(partValue);
    std::string partStr = JSLocale::ConvertToStdString(partEcmaStr);
    return partStr;
}

HWTEST_F_L0(JSListFormatTest, FormatListToParts)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    icu::Locale icuLocale("zh", "Hans", "Cn");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "conjunction" },
        { "style", "long" }
    };
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSListFormat> jsFormatter = CreateJSListFormatterTest(thread, icuLocale, object);
    JSHandle<JSObject> valueObj = JSHandle<JSObject>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value0(factory->NewFromStdString("苹果"));
    JSHandle<JSTaggedValue> value1(factory->NewFromStdString("梨子"));
    JSHandle<JSTaggedValue> value2(factory->NewFromStdString("桃"));
    JSObject::SetProperty(thread, valueObj, key0, value0);
    JSObject::SetProperty(thread, valueObj, key1, value1);
    JSObject::SetProperty(thread, valueObj, key2, value2);
    JSHandle<JSArray> valueArr = JSHandle<JSArray>::Cast(valueObj);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(CString(valueStr->GetCString().get()).c_str(), "苹果、梨子和桃");
    
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    JSHandle<JSArray> parts = JSListFormat::FormatListToParts(thread, jsFormatter, valueArr);
    auto element1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 0).GetValue();
    auto literal1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 1).GetValue();
    auto element2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 2).GetValue();
    auto literal2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 3).GetValue();
    auto element3 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 4).GetValue();
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, element1).c_str(), "element");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, element1).c_str(), "苹果");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, literal1).c_str(), "literal");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, literal1).c_str(), "、");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, element2).c_str(), "element");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, element2).c_str(), "梨子");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, literal2).c_str(), "literal");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, literal2).c_str(), "和");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, element3).c_str(), "element");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, element3).c_str(), "桃");
}

HWTEST_F_L0(JSListFormatTest, StringListFromIterable)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    JSHandle<TaggedArray> data = factory->NewTaggedArray(3);
    JSHandle<JSTaggedValue> value0(factory->NewFromStdString("one"));
    JSHandle<JSTaggedValue> value1(factory->NewFromStdString("two"));
    JSHandle<JSTaggedValue> value2(factory->NewFromStdString("three"));
    data->Set(thread, 0, value0.GetTaggedValue());
    data->Set(thread, 1, value1.GetTaggedValue());
    data->Set(thread, 2, value2.GetTaggedValue());
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    JSHandle<JSArrayIterator> iter(JSIterator::GetIterator(thread, array));
    JSHandle<JSTaggedValue> arrayString =
        JSListFormat::StringListFromIterable(thread, JSHandle<JSTaggedValue>::Cast(iter));
    JSHandle<JSArray> strValue = JSHandle<JSArray>::Cast(arrayString);
    EXPECT_EQ(strValue->GetArrayLength(), 3U);

    auto resValue0 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(strValue), 0).GetValue();
    auto resValue1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(strValue), 1).GetValue();
    auto resValue2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(strValue), 2).GetValue();
    EXPECT_STREQ(CString(JSHandle<EcmaString>::Cast(resValue0)->GetCString().get()).c_str(), "one");
    EXPECT_STREQ(CString(JSHandle<EcmaString>::Cast(resValue1)->GetCString().get()).c_str(), "two");
    EXPECT_STREQ(CString(JSHandle<EcmaString>::Cast(resValue2)->GetCString().get()).c_str(), "three");
}
}  // namespace panda::test