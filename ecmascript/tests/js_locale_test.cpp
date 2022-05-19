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

#include "ecmascript/js_locale.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSLocaleTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

void CreateLanguageIterator(std::vector<std::string>& languageTemp)
{
    languageTemp.push_back("zh-Hans-Cn");
    languageTemp.push_back("ko-kore-kr");
    languageTemp.push_back("en-US");
    languageTemp.push_back("en-Latn-US");
    languageTemp.push_back("ja-JP-u-ca-japanese");
    languageTemp.push_back("ar-EG");
}

/**
 * @tc.name: JSIntlIteratorTest
 * @tc.desc: Construct an iterator of JSIntl and then traverse the iterator to compare whether the variable
 *           at each position is equal to the setting.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, JSIntlIteratorTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::vector<std::string> languageVector;
    CreateLanguageIterator(languageVector);
    uint32_t arrayDataLength = languageVector.size();
    JSHandle<TaggedArray> arrayData = factory->NewTaggedArray(arrayDataLength);
    
    for (uint32_t i = 0 ; i < arrayDataLength; i++) {
        JSHandle<JSTaggedValue> languageStr(factory->NewFromASCII(languageVector[i].c_str()));
        arrayData->Set(thread, i, languageStr);
    }
    // construct a JSIntlIterator object
    JSIntlIterator jsIntlIterator(arrayData, arrayDataLength);
    EXPECT_TRUE(jsIntlIterator.hasNext());
    // call "next" function to traverse the container
    for (uint32_t i = 0 ; i < arrayDataLength; i++) {
        EXPECT_TRUE(jsIntlIterator.next() != nullptr);
        EXPECT_STREQ(jsIntlIterator[i].c_str(), languageVector[i].c_str());
    }
    EXPECT_FALSE(jsIntlIterator.hasNext());
}

/**
 * @tc.name: IsPrivateSubTag
 * @tc.desc: Check whether the string is private subtag through "IsPrivateSubTag" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsPrivateSubTag)
{
    std::string result = "en-GB-oed";
    EXPECT_FALSE(JSLocale::IsPrivateSubTag(result, result.length()));

    result = "i-ami";
    EXPECT_TRUE(JSLocale::IsPrivateSubTag(result, result.length()));

    result = "x-default";
    EXPECT_TRUE(JSLocale::IsPrivateSubTag(result, result.length()));
}

/**
 * @tc.name: GetIcuField
 * @tc.desc: Call "NewJSIntlIcuData" function Set locale IcuField,check whether the locale IcuField through
 *           "getBaseName" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetIcuField)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetLocaleFunction();
    JSHandle<JSLocale> locale =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    // call "NewJSIntlIcuData" function Set IcuField
    icu::Locale icuLocale("zh", "Hans", "Cn");
    factory->NewJSIntlIcuData(locale, icuLocale, JSLocale::FreeIcuLocale);

    // call "GetIcuLocale" function Get IcuField
    icu::Locale *result = locale->GetIcuLocale();
    EXPECT_STREQ(result->getBaseName(), "zh_Hans_CN");
}

/**
 * @tc.name: ConvertToStdString
 * @tc.desc: Convert char* type to std string,check whether the returned string through "ConvertToStdString"
 *           function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ConvertToStdString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> handleEcmaStr = factory-> NewFromStdString("一二三四");
    std::string stdString = JSLocale::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "一二三四");

    handleEcmaStr = factory-> NewFromStdString("#%!\0@$");
    stdString = JSLocale::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "#%!\0@$");

    handleEcmaStr = factory-> NewFromStdString("123456");
    stdString = JSLocale::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "123456");

    handleEcmaStr = factory-> NewFromStdString("zhde");
    stdString = JSLocale::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "zhde");
}

/**
 * @tc.name: IsStructurallyValidLanguageTag
 * @tc.desc: Call "IsStructurallyValidLanguageTag" function check Language-Tag is valid structurally.If the tag contains
 *           the correct language, region, script and extension, return true otherwise, return false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsStructurallyValidLanguageTag)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // number-language
    JSHandle<EcmaString> handleEcmaStr = factory->NewFromStdString("123-de");
    EXPECT_FALSE(JSLocale::IsStructurallyValidLanguageTag(handleEcmaStr));
    // only language(zh)
    handleEcmaStr = factory-> NewFromStdString("zh");
    EXPECT_TRUE(JSLocale::IsStructurallyValidLanguageTag(handleEcmaStr));
    // only language and script, region
    handleEcmaStr = factory-> NewFromStdString("zh-Hans-Cn");
    EXPECT_TRUE(JSLocale::IsStructurallyValidLanguageTag(handleEcmaStr));

    handleEcmaStr = factory-> NewFromStdString("ja-JP-u-ca-japanese");
    EXPECT_TRUE(JSLocale::IsStructurallyValidLanguageTag(handleEcmaStr));

    handleEcmaStr = factory-> NewFromStdString("语言脚本地区");
    EXPECT_FALSE(JSLocale::IsStructurallyValidLanguageTag(handleEcmaStr));

    handleEcmaStr = factory-> NewFromStdString("e-US");
    EXPECT_FALSE(JSLocale::IsStructurallyValidLanguageTag(handleEcmaStr));
}

/**
 * @tc.name: CanonicalizeUnicodeLocaleId
 * @tc.desc: Call "CanonicalizeUnicodeLocaleId" function canonicalize locale(Language-Tag),The English case of language,
 *           region and script is fixed.the language is lowercase.the beginning of region is uppercase, and the script
 *           is lowercase.if locale string is IsUtf16,return empty string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, CanonicalizeUnicodeLocaleId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> locale = factory-> NewFromStdString("en-Us");
    JSHandle<EcmaString> canonicalizeLocaleId = JSLocale::CanonicalizeUnicodeLocaleId(thread, locale);
    EXPECT_STREQ("en-US", CString(canonicalizeLocaleId->GetCString().get()).c_str());

    locale = factory-> NewFromStdString("kO-kore-kr");
    canonicalizeLocaleId = JSLocale::CanonicalizeUnicodeLocaleId(thread, locale);
    EXPECT_STREQ("ko-Kore-KR", CString(canonicalizeLocaleId->GetCString().get()).c_str());

    locale = factory-> NewFromStdString("id-u-co-pinyin-de-ID");
    canonicalizeLocaleId = JSLocale::CanonicalizeUnicodeLocaleId(thread, locale);
    EXPECT_STREQ("id-u-co-pinyin-de-id", CString(canonicalizeLocaleId->GetCString().get()).c_str());
    // invalid locale
    uint16_t localeArr[] = {0x122, 0x104, 0x45, 0x72, 0x97, 0x110, 0x115, 0x45, 0x67, 0x78}; // zh-Hans-CN
    uint32_t localeArrLength = sizeof(localeArr) / sizeof(localeArr[0]);
    locale = factory->NewFromUtf16(localeArr, localeArrLength);

    canonicalizeLocaleId = JSLocale::CanonicalizeUnicodeLocaleId(thread, locale);
    JSHandle<EcmaString> emptyString = factory->GetEmptyString();
    EXPECT_EQ(canonicalizeLocaleId->Compare(*emptyString), 0);
}

/**
 * @tc.name: IsValidTimeZoneName
 * @tc.desc: Call "IsValidTimeZoneName" function check whether the TimeZone is valid.if TimeZone include "GMT-Time"
 *           return true otherwise, return false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsValidTimeZoneName)
{
    icu::UnicodeString stringID1("GMT-8:00");
    icu::TimeZone *timeZone = icu::TimeZone::createTimeZone(stringID1);
    EXPECT_TRUE(JSLocale::IsValidTimeZoneName(*timeZone));

    icu::UnicodeString stringID2("Etc/Unknown");
    timeZone = icu::TimeZone::createTimeZone(stringID2);
    EXPECT_FALSE(JSLocale::IsValidTimeZoneName(*timeZone));
}

/**
 * @tc.name: CanonicalizeLocaleList
 * @tc.desc: Create a list of locales and canonicalize the locales in the list through "CanonicalizeUnicodeLocaleId"
 *           function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, CanonicalizeLocaleList)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetLocaleFunction();
    JSHandle<JSLocale> jsLocale =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    // Set IcuLocale
    icu::Locale icuLocale("fr", "Latn", "Fr");
    factory->NewJSIntlIcuData(jsLocale, icuLocale, JSLocale::FreeIcuLocale);
    // test locale is jslocale
    JSHandle<TaggedArray> localeArr = JSLocale::CanonicalizeLocaleList(thread, JSHandle<JSTaggedValue>(jsLocale));
    EXPECT_EQ(localeArr->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, localeArr->Get(0));
    EXPECT_STREQ("fr-Latn-FR", CString(handleEcmaStr->GetCString().get()).c_str());
    // test locale is object
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> localeObj(thread, arr);
    
    JSHandle<JSTaggedValue> localeStr1(factory->NewFromASCII("EN-us"));
    PropertyDescriptor desc1(thread, localeStr1, true, true, true);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("1"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(localeObj), key1, desc1);

    JSHandle<JSTaggedValue> localeStr2(factory->NewFromASCII("en-GB"));
    PropertyDescriptor desc2(thread, localeStr2, true, true, true);
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("2"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(localeObj), key2, desc2);
    // check canonicalized string
    localeArr = JSLocale::CanonicalizeLocaleList(thread, localeObj);
    EXPECT_EQ(localeArr->GetLength(), 2U);
    JSHandle<EcmaString> resultEcmaStr1(thread, localeArr->Get(0));
    EXPECT_STREQ("en-US", CString(resultEcmaStr1->GetCString().get()).c_str());
    JSHandle<EcmaString> resultEcmaStr2(thread, localeArr->Get(1));
    EXPECT_STREQ("en-GB", CString(resultEcmaStr2->GetCString().get()).c_str());
}

/**
 * @tc.name: IcuToString
 * @tc.desc: Call "IcuToString" function Convert UnicodeString to string(Utf16).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IcuToString)
{
    icu::UnicodeString unicodeString1("GMT-12:00"); // times
    JSHandle<EcmaString> ecmaString = JSLocale::IcuToString(thread, unicodeString1);
    EXPECT_STREQ("GMT-12:00", CString(ecmaString->GetCString().get()).c_str());

    icu::UnicodeString unicodeString2("周日16:00:00–周五23:00:00"); // date
    ecmaString = JSLocale::IcuToString(thread, unicodeString2);
    EXPECT_STREQ("周日16:00:00–周五23:00:00", CString(ecmaString->GetCString().get()).c_str());

    icu::UnicodeString unicodeString3("$654K"); // money
    ecmaString = JSLocale::IcuToString(thread, unicodeString3);
    EXPECT_STREQ("$654K", CString(ecmaString->GetCString().get()).c_str());

    icu::UnicodeString unicodeString4("1 minute ago"); // moment
    ecmaString = JSLocale::IcuToString(thread, unicodeString4, 0, 2);
    EXPECT_STREQ("1 ", CString(ecmaString->GetCString().get()).c_str());
}

/**
 * @tc.name: PutElement
 * @tc.desc: Put elements in empty JSArray and return the JSArray.call "GetProperty" function to get the value and
 *           check whether the value is consistent with the value of the put.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, PutElement)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSArray> jsArray = factory->NewJSArray();
    JSHandle<JSTaggedValue> typeString = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> valueString = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> fieldTypeString = globalConst->GetHandledUnitString();
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<int>(11)));

    int index = 1;
    JSHandle<JSObject> recordObj = JSLocale::PutElement(thread, index, jsArray, fieldTypeString, value);
    EXPECT_EQ(JSTaggedValue::SameValue(
              JSObject::GetProperty(thread, recordObj, typeString).GetValue(), fieldTypeString), true);
    EXPECT_EQ(JSObject::GetProperty(thread, recordObj, valueString).GetValue()->GetInt(), 11);

    JSHandle<JSTaggedValue> indexKey(factory->NewFromASCII("1"));
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSObject>(jsArray), indexKey).GetValue()->IsECMAObject());
}

/**
 * @tc.name: ToLanguageTag
 * @tc.desc: call "ToLanguageTag" function Convert ICU Locale into language tag.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ToLanguageTag)
{
    icu::Locale icuLocale1("en", "Latn", "US", "collation=phonebk;currency=euro");
    JSHandle<EcmaString> languageTag = JSLocale::ToLanguageTag(thread, icuLocale1);
    EXPECT_STREQ("en-Latn-US-u-co-phonebk-cu-euro", CString(languageTag->GetCString().get()).c_str());

    icu::Locale icuLocale2("zh", "Hans", "CN", "collation=phonebk;kn=true");
    languageTag = JSLocale::ToLanguageTag(thread, icuLocale2);
    EXPECT_STREQ("zh-Hans-CN-u-co-phonebk-kn", CString(languageTag->GetCString().get()).c_str());

    icu::Locale icuLocale3("ja", "Jpan", "JP", "collation=phonebk;co=yes");
    languageTag = JSLocale::ToLanguageTag(thread, icuLocale3);
    EXPECT_STREQ("ja-Jpan-JP-u-co", CString(languageTag->GetCString().get()).c_str());

    icu::Locale icuLocale4("z", "CN"); // language is fault
    languageTag = JSLocale::ToLanguageTag(thread, icuLocale4);
    EXPECT_STREQ("und-CN", CString(languageTag->GetCString().get()).c_str());

    icu::Locale icuLocale5("zh", "c"); // script is fault
    languageTag = JSLocale::ToLanguageTag(thread, icuLocale5);
    EXPECT_STREQ("zh-x-lvariant-c", CString(languageTag->GetCString().get()).c_str());

    icu::Locale icuLocale6("en", "Latn", "E"); // region is fault
    languageTag = JSLocale::ToLanguageTag(thread, icuLocale6);
    EXPECT_STREQ("en-Latn-x-lvariant-e", CString(languageTag->GetCString().get()).c_str());

    icu::Locale icuLocale7("en", "Latn", "EG", "kf=yes"); // key value is fault
    languageTag = JSLocale::ToLanguageTag(thread, icuLocale7);
    EXPECT_STREQ("en-Latn-EG-u-kf", CString(languageTag->GetCString().get()).c_str());
}

/**
 * @tc.name: GetNumberingSystem
 * @tc.desc: Call "GetNumberingSystem" function get the script from the ICU Locale.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetNumberingSystem)
{
    icu::Locale icuLocale1("en", "US");
    std::string numberingSystem = JSLocale::GetNumberingSystem(icuLocale1);
    EXPECT_STREQ("latn", numberingSystem.c_str());

    icu::Locale icuLocale2("zh", "Hans", "CN", "collation=phonebk;numbers=hans");
    numberingSystem = JSLocale::GetNumberingSystem(icuLocale2);
    EXPECT_STREQ("hans", numberingSystem.c_str());
}

/**
 * @tc.name: GetNumberFieldType
 * @tc.desc: Call "GetNumberFieldType" function get Number Field type.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetNumberFieldType)
{
    auto globalConst = thread->GlobalConstants();
    int32_t fieldId = 0; // UNUM_INTEGER_FIELD

    JSTaggedValue x(0.0f / 0.0f); // Nan
    JSHandle<JSTaggedValue> nanString = globalConst->GetHandledNanString();
    JSHandle<JSTaggedValue> fieldTypeString = JSLocale::GetNumberFieldType(thread, x, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(fieldTypeString, nanString), true);

    JSTaggedValue y(-10); // integer(sign bit)
    JSHandle<JSTaggedValue> integerString = globalConst->GetHandledIntegerString();
    fieldTypeString = JSLocale::GetNumberFieldType(thread, y, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(fieldTypeString, integerString), true);

    fieldId = 10; // UNUM_SIGN_FIELD
    JSHandle<JSTaggedValue> minusSignString = globalConst->GetHandledMinusSignString();
    fieldTypeString = JSLocale::GetNumberFieldType(thread, y, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(fieldTypeString, minusSignString), true);

    JSTaggedValue z(10); // no sign bit
    JSHandle<JSTaggedValue> plusSignString = globalConst->GetHandledPlusSignString();
    fieldTypeString = JSLocale::GetNumberFieldType(thread, z, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(fieldTypeString, plusSignString), true);
}

/**
 * @tc.name: ApplyOptionsToTag
 * @tc.desc: Call "ApplyOptionsToTag" function parse information in option into tag string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ApplyOptionsToTag)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    TagElements tagElements;
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<EcmaString> languageTag = JSLocale::DefaultLocale(thread);

    JSHandle<JSTaggedValue> languageKey = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionKey = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> scriptKey =  thread->GlobalConstants()->GetHandledScriptString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("US"));
    JSHandle<JSTaggedValue> scriptValue(factory->NewFromASCII("Latn"));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageKey, languageValue);
    JSObject::SetProperty(thread, optionsObj, regionKey, regionValue);
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue);
    bool result = JSLocale::ApplyOptionsToTag(thread, languageTag, optionsObj, tagElements);
    EXPECT_TRUE(result);
    EXPECT_EQ(tagElements.language, languageValue);
    EXPECT_EQ(tagElements.script, scriptValue);
    EXPECT_EQ(tagElements.region, regionValue);
    // fault script
    JSHandle<JSTaggedValue> scriptValue1(factory->NewFromASCII(""));
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue1);
    result = JSLocale::ApplyOptionsToTag(thread, languageTag, optionsObj, tagElements);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: HandleLocaleExtension
 * @tc.desc: Find position of subtag "x" or "u" in Locale through "HandleLocaleExtension" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, HandleLocaleExtension)
{
    std::string result = "en-Latn-US-u-ca-gregory-co-compat";
    size_t start = 0;
    size_t extensionEnd = 0;
    JSLocale::HandleLocaleExtension(start, extensionEnd, result, result.size());
    EXPECT_EQ(extensionEnd, 10U); // the position of "u"
    // private extension("x")
    result = "de-zh-x-co-phonebk-nu-kali";
    start = 0;
    extensionEnd = 0;
    JSLocale::HandleLocaleExtension(start, extensionEnd, result, result.size());
    EXPECT_EQ(extensionEnd, 5U); // the position of "x"
}

/**
 * @tc.name: HandleLocale
 * @tc.desc: Call "HandleLocale" function handle locale,if Locale has subtag "u" ignore it.If Locale has
 *           both subtag "x" and "u","x" is in front of "u","u" does not ignore,"x" is after "u","u" ignores.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, HandleLocale)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // no "u" or "x"
    JSHandle<EcmaString> localeString = factory->NewFromASCII("en-Latn-US");
    JSLocale::ParsedLocale parsedResult = JSLocale::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "en-Latn-US");
    // only "x"
    localeString = factory->NewFromASCII("zh-CN-x-ca-pinyin");
    parsedResult = JSLocale::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "zh-CN-x-ca-pinyin");
    // only "u"
    localeString = factory->NewFromASCII("ko-Kore-KR-u-co-phonebk");
    parsedResult = JSLocale::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "ko-Kore-KR");
    // both "x" and "u"
    localeString = factory->NewFromASCII("en-Latn-US-u-x-co-phonebk-kn-true");
    parsedResult = JSLocale::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "en-Latn-US-x-co-phonebk-kn-true");

    localeString = factory->NewFromASCII("en-Latn-US-x-u-ca-pinyin-co-compat");
    parsedResult = JSLocale::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "en-Latn-US-x-u-ca-pinyin-co-compat");
}

/**
 * @tc.name: ConstructLocaleList
 * @tc.desc: Get LocaleList numbers through "ConstructLocaleList" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ConstructLocaleList)
{
    std::vector<std::string> availableLocales = {"zh-Hans-CN", "de-ID", "en-US", "en-GB"};
    JSHandle<TaggedArray> localeArr = JSLocale::ConstructLocaleList(thread, availableLocales);
    EXPECT_EQ(localeArr->GetLength(), 4U); // 4 : 4 Number of locales
}

/**
 * @tc.name: SetNumberFormatDigitOptions
 * @tc.desc: Call "SetNumberFormatDigitOptions" function parse information in option into attributes
 *           of the JSNumberFormat.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, SetNumberFormatDigitOptions_Significant)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> numberFormatObj = env->GetNumberFormatFunction();

    JSHandle<JSTaggedValue> mnidKey = thread->GlobalConstants()->GetHandledMinimumIntegerDigitsString();
    JSHandle<JSTaggedValue> mnsdKey = thread->GlobalConstants()->GetHandledMinimumSignificantDigitsString();
    JSHandle<JSTaggedValue> mxsdKey = thread->GlobalConstants()->GetHandledMaximumSignificantDigitsString();
    JSHandle<JSTaggedValue> mnidValue(thread, JSTaggedValue(10));
    JSHandle<JSTaggedValue> maxFraValue(thread, JSTaggedValue(11));
    JSHandle<JSTaggedValue> minSignValue(thread, JSTaggedValue(12));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, mnidKey, mnidValue);
    JSObject::SetProperty(thread, optionsObj, mnsdKey, maxFraValue);
    JSObject::SetProperty(thread, optionsObj, mxsdKey, minSignValue);
    JSHandle<JSNumberFormat> jsNumberFormat = JSHandle<JSNumberFormat>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(numberFormatObj), numberFormatObj));

    JSLocale::SetNumberFormatDigitOptions(thread, jsNumberFormat, JSHandle<JSTaggedValue>(optionsObj),
                                                   1, 1, NotationOption::COMPACT);
    EXPECT_EQ(jsNumberFormat->GetMinimumSignificantDigits().GetInt(), 11);
    EXPECT_EQ(jsNumberFormat->GetMaximumSignificantDigits().GetInt(), 12);
    EXPECT_EQ(jsNumberFormat->GetMinimumIntegerDigits().GetInt(), 10);
    EXPECT_EQ(jsNumberFormat->GetRoundingType(), RoundingType::SIGNIFICANTDIGITS);
}

HWTEST_F_L0(JSLocaleTest, SetNumberFormatDigitOptions_Fraction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> pluralRulesObj = env->GetPluralRulesFunction();

    JSHandle<JSTaggedValue> mnidKey = thread->GlobalConstants()->GetHandledMinimumIntegerDigitsString();
    JSHandle<JSTaggedValue> mnidValue(thread, JSTaggedValue(10));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, mnidKey, mnidValue);
    JSHandle<JSPluralRules> jsPluralRules = JSHandle<JSPluralRules>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(pluralRulesObj), pluralRulesObj));

    JSLocale::SetNumberFormatDigitOptions(thread, jsPluralRules, JSHandle<JSTaggedValue>(optionsObj),
                                                  10, 13, NotationOption::EXCEPTION);
    EXPECT_EQ(jsPluralRules->GetMinimumFractionDigits().GetInt(), 10);
    EXPECT_EQ(jsPluralRules->GetMaximumFractionDigits().GetInt(), 13);
    EXPECT_EQ(jsPluralRules->GetMinimumIntegerDigits().GetInt(), 10);
    EXPECT_EQ(jsPluralRules->GetRoundingType(), RoundingType::FRACTIONDIGITS);
}

/**
 * @tc.name: CheckLocales
 * @tc.desc: Call "CheckLocales" function check wether language is correct from locale libraries obtained
 *           from different ways.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, CheckLocales)
{
    bool res = false;
    const char *path = JSCollator::uIcuDataColl.c_str();
    // default language
    bool result = JSLocale::CheckLocales("en", nullptr, path, res);
    EXPECT_TRUE(result);
    // fault language
    result = JSLocale::CheckLocales("e", nullptr, path, res);
    EXPECT_FALSE(result);
    // find language in calendar
    result = JSLocale::CheckLocales("en-US", "calendar", nullptr, res);
    EXPECT_TRUE(result);
    // find language in NumberElements
    result = JSLocale::CheckLocales("en-US", "NumberElements", nullptr, res);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: UnicodeExtensionValue
 * @tc.desc: Call "UnicodeExtensionValue" function get subtag after key in Extension.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, UnicodeExtensionValue)
{
    // extension has one "-"
    std::string result = JSLocale::UnicodeExtensionValue("-ca=chinese", "ca");
    EXPECT_STREQ(result.c_str(), "undefined");
    // extension has one "-" and key value is full
    result = JSLocale::UnicodeExtensionValue("-ca", "ca");
    EXPECT_STREQ(result.c_str(), "");
    // extension has two "-"
    result = JSLocale::UnicodeExtensionValue("-ca-chinese", "ca");
    EXPECT_STREQ(result.c_str(), "chinese");

    result = JSLocale::UnicodeExtensionValue("-ca-chinese-co-compat", "co");
    EXPECT_STREQ(result.c_str(), "compat");

    result = JSLocale::UnicodeExtensionValue("-ca-kn-true", "kn");
    EXPECT_STREQ(result.c_str(), "true");
}

/**
 * @tc.name: IsWellCalendar
 * @tc.desc: Call "IsWellCalendar" function judge whether the calendar is well from locale.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsWellCalendar)
{
    EXPECT_TRUE(JSLocale::IsWellCalendar("ar-EG", "islamic"));
    EXPECT_TRUE(JSLocale::IsWellCalendar("ar-EG", "coptic"));
    EXPECT_TRUE(JSLocale::IsWellCalendar("zh-CN", "chinese"));
    EXPECT_TRUE(JSLocale::IsWellCalendar("en-US", "gregory"));

    EXPECT_FALSE(JSLocale::IsWellCalendar("zh-CN", "English"));
}

/**
 * @tc.name: IsWellCollation
 * @tc.desc: Call "IsWellCollation" function judge whether the collation is well from locale.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsWellCollation)
{
    EXPECT_TRUE(JSLocale::IsWellCollation("ar-EG", "compat"));

    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "stroke"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "pinyin"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "phonebk"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "search"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "standard"));
}

/**
 * @tc.name: IsWellNumberingSystem
 * @tc.desc: Call "IsWellNumberingSystem" function judge whether the script is well.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsWellNumberingSystem)
{
    EXPECT_FALSE(JSLocale::IsWellNumberingSystem("finance"));
    EXPECT_FALSE(JSLocale::IsWellNumberingSystem("native"));
    EXPECT_FALSE(JSLocale::IsWellNumberingSystem("traditio"));

    EXPECT_TRUE(JSLocale::IsWellNumberingSystem("hans"));
    EXPECT_TRUE(JSLocale::IsWellNumberingSystem("deva"));
    EXPECT_TRUE(JSLocale::IsWellNumberingSystem("greklow"));
}

/**
 * @tc.name: DefaultNumberOption
 * @tc.desc: Call "DefaultNumberOption" function get default number from value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, DefaultNumberOption)
{
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(static_cast<double>(4.99)));
    int result = JSLocale::DefaultNumberOption(thread, value1, 1, 5, 1);
    EXPECT_EQ(result, 4);
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    result = JSLocale::DefaultNumberOption(thread, value2, 1, 5, 1);
    EXPECT_EQ(result, 1);
}

/**
 * @tc.name: GetOptionOfString
 * @tc.desc: Call "GetOptionOfString" function get the string from Option value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetOptionOfString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> languageProperty = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionProperty = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("zh"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("CN"));
    // Set key value
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageProperty, languageValue);
    JSObject::SetProperty(thread, optionsObj, regionProperty, regionValue);
    std::vector<std::string> stringValues = {"zh", "Hans", "CN"};
    std::string optionValue;
    // Get language
    bool result = JSLocale::GetOptionOfString(thread, optionsObj, languageProperty, stringValues, &optionValue);
    EXPECT_TRUE(result);
    EXPECT_STREQ(optionValue.c_str(), "zh");
    // Get region
    result = JSLocale::GetOptionOfString(thread, optionsObj, regionProperty, stringValues, &optionValue);
    EXPECT_TRUE(result);
    EXPECT_STREQ(optionValue.c_str(), "CN");
}

/**
 * @tc.name: GetOption
 * @tc.desc: Call "GetOption" function get value of the key from Option.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetOption)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> languageProperty = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionProperty = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("zh"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("CN"));
    // Set key value
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageProperty, languageValue);

    JSHandle<TaggedArray> stringValues = factory->NewTaggedArray(3);
    stringValues->Set(thread, 0, languageValue);
    stringValues->Set(thread, 1, regionValue);
    JSHandle<JSTaggedValue> arrayValue(stringValues);
    JSHandle<JSTaggedValue> fallback(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> optionValue =
        JSLocale::GetOption(thread, optionsObj, languageProperty, OptionType::STRING, arrayValue, fallback);
    EXPECT_EQ(JSTaggedValue::SameValue(optionValue, languageValue), true);

    optionValue = JSLocale::GetOption(thread, optionsObj, regionProperty, OptionType::STRING, arrayValue, fallback);
    EXPECT_EQ(JSTaggedValue::SameValue(optionValue, fallback), true);
}

/**
 * @tc.name: GetOptionOfBool
 * @tc.desc: Call "GetOptionOfBool" function get the bool value from Option.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetOptionOfBool)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> numericProperty = thread->GlobalConstants()->GetHandledNumericString();
    JSHandle<JSTaggedValue> numericValue(thread, JSTaggedValue::True());
    // Set key value
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, numericProperty, numericValue);
    bool res;
    // Test correct keyValue
    EXPECT_TRUE(JSLocale::GetOptionOfBool(thread, optionsObj, numericProperty, false, &res));
    EXPECT_TRUE(res);

    JSHandle<JSTaggedValue> numericValue1(thread, JSTaggedValue(0));
    JSObject::SetProperty(thread, optionsObj, numericProperty, numericValue1);
    // Test fault keyValue
    EXPECT_TRUE(JSLocale::GetOptionOfBool(thread, optionsObj, numericProperty, false, &res));
    EXPECT_FALSE(res);
}

/**
 * @tc.name: BestAvailableLocale
 * @tc.desc: Match the best Locale and return from available locale through "BestAvailableLocale" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, BestAvailableLocale)
{
    const char *path = JSCollator::uIcuDataColl.c_str();
    // available locales in uIcuDataColl
    JSHandle<TaggedArray> icuDataAvailableLocales = JSLocale::GetAvailableLocales(thread, nullptr, path);
    // available locales(calendar)
    JSHandle<TaggedArray> calendarAvailableLocales = JSLocale::GetAvailableLocales(thread, "calendar", nullptr);
    // available locales(NumberElements)
    JSHandle<TaggedArray> numberAvailableLocales = JSLocale::GetAvailableLocales(thread, "NumberElements", nullptr);
    // "ar-001" is found
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, icuDataAvailableLocales, "ar-001").c_str(), "ar-001");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, calendarAvailableLocales, "ar-001").c_str(), "ar-001");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, numberAvailableLocales, "ar-001").c_str(), "ar-001");
    // "agq-CM" is not found in uIcuDataColl
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, icuDataAvailableLocales, "agq-CM").c_str(), "");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, calendarAvailableLocales, "agq-CM").c_str(), "agq-CM");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, numberAvailableLocales, "agq-CM").c_str(), "agq-CM");
    // language(und)-region(CN)
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, icuDataAvailableLocales, "und-CN").c_str(), "");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, calendarAvailableLocales, "und-CN").c_str(), "");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, numberAvailableLocales, "und-CN").c_str(), "");
    // language(en)-region(001)
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, icuDataAvailableLocales, "en-001").c_str(), "en-001");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, calendarAvailableLocales, "en-001").c_str(), "en-001");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, numberAvailableLocales, "en-001").c_str(), "en-001");
    // language(en)-script(Hans)-region(US)
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, icuDataAvailableLocales, "en-Hans-US").c_str(), "en");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, calendarAvailableLocales, "en-Hans-US").c_str(), "en");
    EXPECT_STREQ(JSLocale::BestAvailableLocale(thread, numberAvailableLocales, "en-Hans-US").c_str(), "en");
}

/**
 * @tc.name: ResolveLocale
 * @tc.desc: Resolve Locale and return from available locale through "ResolveLocale" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ResolveLocale_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> availableLocales = factory->EmptyArray();
    JSHandle<TaggedArray> requestedLocales = factory->EmptyArray();
    std::set<std::string> relevantExtensionKeys = {"co", "ca"};
    JSHandle<JSTaggedValue> testLocale1(factory->NewFromASCII("id-u-co-pinyin-ca-gregory-de-ID"));
    JSHandle<JSTaggedValue> testLocale2(factory->NewFromASCII("en-Latn-US-u-co-phonebk-ca-ethioaa"));
    // availableLocales and requestLocales is empty
    ResolvedLocale result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-US", result.locale.c_str()); // default locale
    // availableLocales and requestLocales is not empty
    availableLocales = JSLocale::GetAvailableLocales(thread, "calendar", nullptr);
    requestedLocales = factory->NewTaggedArray(1);
    // test locale1
    requestedLocales->Set(thread, 0, testLocale1);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("id-u-ca-gregory-co-pinyin-de-id", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("id-u-ca-gregory-co-pinyin-de-id", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("id-u-ca-gregory-co-pinyin-de-id", result.locale.c_str());
    // test locale2
    requestedLocales->Set(thread, 0, testLocale2);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-u-ca-ethioaa-co-phonebk", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("en-u-ca-ethioaa-co-phonebk", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("en-u-ca-ethioaa-co-phonebk", result.locale.c_str());
}

HWTEST_F_L0(JSLocaleTest, ResolveLocale_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> availableLocales = factory->EmptyArray();
    JSHandle<TaggedArray> requestedLocales = factory->EmptyArray();
    std::set<std::string> relevantExtensionKeys = {"hc", "lb", "kn", "kf"};
    JSHandle<JSTaggedValue> testLocale1(factory->NewFromASCII("id-u-kn-false-kf-yes-de-ID"));
    JSHandle<JSTaggedValue> testLocale2(factory->NewFromASCII("en-US-u-hc-h24-lb-strict"));
    // availableLocales and requestLocales is empty
    ResolvedLocale result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-US", result.locale.c_str()); // default locale
    // availableLocales and requestLocales is not empty
    availableLocales = JSLocale::GetAvailableLocales(thread, "calendar", nullptr);
    requestedLocales = factory->NewTaggedArray(1);
    // test locale1
    requestedLocales->Set(thread, 0, testLocale1);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("id-u-de-id-kf-kn-false", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("id-u-de-id-kf-kn-false", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("id-u-de-id-kf-kn-false", result.locale.c_str());
    // test locale2
    requestedLocales->Set(thread, 0, testLocale2);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-US-u-hc-h24-lb-strict", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("en-US-u-hc-h24-lb-strict", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("en-US-u-hc-h24-lb-strict", result.locale.c_str());
}
}  // namespace panda::test