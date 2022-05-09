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

#include "ecmascript/builtins/builtins_list_format.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsListFormatTest : public testing::Test {
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

// new Intl.ListFormat(locales)
HWTEST_F_L0(BuiltinsListFormatTest, ListFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetListFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-GB"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::ListFormatConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSListFormat());
}

static JSTaggedValue JSListFormatCreateWithOptionTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                      JSHandle<JSTaggedValue> &typeValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetListFormatFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledTypeString();
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::ListFormatConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSListFormat());
    return result;
}

// Format("Motorcycle" ,type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> listValue(factory->NewFromASCII("Motorcycle"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, listValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("M, o, t, o, r, c, y, c, l and e", CString(handleEcmaStr->GetCString().get()).c_str());
}

// Format(["Motorcycle", "Bus", "Car" ], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("Motorcycle"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("Bus"));
    JSHandle<JSTaggedValue> listValue3(factory->NewFromStdString("Car"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, listValue3, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key2, desc2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("Motorcycle, Bus and Car", CString(handleEcmaStr->GetCString().get()).c_str());
}

// Format(["Motorcycle", "Bus", "Car" ], type(disjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("Motorcycle"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("Bus"));
    JSHandle<JSTaggedValue> listValue3(factory->NewFromStdString("Car"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, listValue3, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key2, desc2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("Motorcycle, Bus or Car", CString(handleEcmaStr->GetCString().get()).c_str());
}

// Format(["中文英文" ], type(disjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("中文英文"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("中文英文", CString(handleEcmaStr->GetCString().get()).c_str());
}

// Format(["中文", "英文", "韩文" ], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("中文"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("英文"));
    JSHandle<JSTaggedValue> listValue3(factory->NewFromStdString("韩文"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, listValue3, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key2, desc2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::Format(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("中文、英文和韩文", CString(handleEcmaStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction"));
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 0U); // zero formatters
}

// FormatToParts(["Apple", "Orange", "Pineapple" ], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("Apple"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("Orange"));
    JSHandle<JSTaggedValue> listValue3(factory->NewFromStdString("Pineapple"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, listValue3, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key2, desc2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 6U);
}

HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("Apple"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("Orange"));
    JSHandle<JSTaggedValue> listValue3(factory->NewFromStdString("Pineapple"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, listValue3, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key2, desc2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue valueList(static_cast<JSTaggedType>(result.GetRawData()));

    ASSERT_TRUE(valueList.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, valueList);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKey).GetValue()->GetInt(), 5);
}

// FormatToParts(["中文", "英文"], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction"));
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("中文"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("英文"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 3U);
}

// FormatToParts(["中文", "英文","韩文","葡萄牙语"], type(disjunction))
HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction"));
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSHandle<JSTaggedValue> listValue1(factory->NewFromStdString("中文"));
    JSHandle<JSTaggedValue> listValue2(factory->NewFromStdString("英文"));
    JSHandle<JSTaggedValue> listValue3(factory->NewFromStdString("韩文"));
    JSHandle<JSTaggedValue> listValue4(factory->NewFromStdString("葡萄牙语"));

    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, listValue1, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, listValue2, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, listValue3, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, listValue4, true, true, true);
    JSArray::DefineOwnProperty(thread, value, key3, desc3);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue valueList(static_cast<JSTaggedType>(result.GetRawData()));

    ASSERT_TRUE(valueList.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, valueList);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKey).GetValue()->GetInt(), 7);
}

// SupportedLocalesOf("best fit")
HWTEST_F_L0(BuiltinsListFormatTest, SupportedLocalesOf_001)
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
    JSTaggedValue resultArr = BuiltinsListFormat::SupportedLocalesOf(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", CString(handleEcmaStr->GetCString().get()).c_str());
}

// SupportedLocalesOf("look up")
HWTEST_F_L0(BuiltinsListFormatTest, SupportedLocalesOf_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII("lookup"));
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("id-u-co-pinyin-de-DE"));

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue resultArr = BuiltinsListFormat::SupportedLocalesOf(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);

    JSHandle<EcmaString> resultStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de", CString(resultStr->GetCString().get()).c_str());
}

HWTEST_F_L0(BuiltinsListFormatTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction"));
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, JSListFormatCreateWithOptionTest(thread, locale, typeValue));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsListFormat::ResolvedOptions(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jslistformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromASCII("de-DE"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, typeKey).GetValue(), typeValue), true);
}
}  // namespace panda::test
