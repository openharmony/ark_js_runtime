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

#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsNumberFormatTest : public testing::Test {
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

// new DateTimeFormat(newTarget is undefined)
HWTEST_F_L0(BuiltinsNumberFormatTest, NumberFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetNumberFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumberFormat::NumberFormatConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSNumberFormat());
}

static JSTaggedValue BuiltinsFormatTest(JSThread *thread, JSHandle<JSObject> &options,
                                        JSHandle<JSTaggedValue> &number, JSHandle<JSTaggedValue> &locale)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetNumberFormatFunction());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo1->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, options.GetTaggedValue());
    // construct numberformat
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue numberFormat = BuiltinsNumberFormat::NumberFormatConstructor(ecmaRuntimeCallInfo1.get());
    JSHandle<JSTaggedValue> numberFormatVal(thread, numberFormat);
    TestHelper::TearDownFrame(thread, prev);
    // get function by calling Format function
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(numberFormatVal.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue resultFunc = BuiltinsNumberFormat::Format(ecmaRuntimeCallInfo2.get());
    JSHandle<JSFunction> jsFunction(thread, resultFunc);
    TestHelper::TearDownFrame(thread, prev);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromASCII("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(jsObject.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, number.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    JSTaggedValue result = BuiltinsArray::ToString(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// format decimal
HWTEST_F_L0(BuiltinsNumberFormatTest, Format_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("decimal"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> numberVal(thread, JSTaggedValue(3500));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSTaggedValue formatResult = BuiltinsFormatTest(thread, optionsObj, numberVal, localeString);

    JSHandle<EcmaString> resultEcmaStr(thread, formatResult);
    EXPECT_STREQ("3,500", CString(resultEcmaStr->GetCString().get()).c_str());
}

// format currency
HWTEST_F_L0(BuiltinsNumberFormatTest, Format_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> currencyKey = globalConst->GetHandledCurrencyString();
    JSHandle<JSTaggedValue> currencyDisplayKey = globalConst->GetHandledCurrencyDisplayString();
    JSHandle<JSTaggedValue> currencySignDisplayKey = globalConst->GetHandledCurrencySignString();

    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("currency"));
    JSHandle<JSTaggedValue> currencyValue(factory->NewFromASCII("USD"));
    JSHandle<JSTaggedValue> currencyDisplayValue(factory->NewFromASCII("name"));
    JSHandle<JSTaggedValue> currencySignDisplayValue(factory->NewFromASCII("accounting"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> numberVal(thread, JSTaggedValue(static_cast<int32_t>(-3500)));
    
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, currencyKey, currencyValue);
    JSObject::SetProperty(thread, optionsObj, currencyDisplayKey, currencyDisplayValue);
    JSObject::SetProperty(thread, optionsObj, currencySignDisplayKey, currencySignDisplayValue);
    JSTaggedValue formatResult = BuiltinsFormatTest(thread, optionsObj, numberVal, localeString);

    JSHandle<EcmaString> resultEcmaStr(thread, formatResult);
    EXPECT_STREQ("($3,500.00)", CString(resultEcmaStr->GetCString().get()).c_str());
}

// format percent
HWTEST_F_L0(BuiltinsNumberFormatTest, Format_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> signDisplayKey = globalConst->GetHandledSignDisplayString();

    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("percent"));
    JSHandle<JSTaggedValue> signDisplayValue(factory->NewFromASCII("exceptZero"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> numberVal(thread, JSTaggedValue(static_cast<double>(0.55)));
    
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, signDisplayKey, signDisplayValue);
    JSTaggedValue formatResult = BuiltinsFormatTest(thread, optionsObj, numberVal, localeString);

    JSHandle<EcmaString> resultEcmaStr(thread, formatResult);
    EXPECT_STREQ("+55%", CString(resultEcmaStr->GetCString().get()).c_str());
}

// format unit
HWTEST_F_L0(BuiltinsNumberFormatTest, Format_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> unitKey = globalConst->GetHandledUnitString();

    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("unit"));
    JSHandle<JSTaggedValue> unitValue(factory->NewFromASCII("liter"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> numberVal(thread, JSTaggedValue(3500));
    
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, unitKey, unitValue);
    JSTaggedValue formatResult = BuiltinsFormatTest(thread, optionsObj, numberVal, localeString);

    JSHandle<EcmaString> resultEcmaStr(thread, formatResult);
    EXPECT_STREQ("3,500 L", CString(resultEcmaStr->GetCString().get()).c_str());
}

// format notation
HWTEST_F_L0(BuiltinsNumberFormatTest, Format_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> notationKey = globalConst->GetHandledNotationString();
    JSHandle<JSTaggedValue> notationValue(factory->NewFromASCII("compact"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("zh-CN"));
    JSHandle<JSTaggedValue> numberVal(thread, JSTaggedValue(987654321));
    
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, notationKey, notationValue);
    JSTaggedValue formatResult = BuiltinsFormatTest(thread, optionsObj, numberVal, localeString);

    JSHandle<EcmaString> resultEcmaStr(thread, formatResult);
    EXPECT_STREQ("9.9亿", CString(resultEcmaStr->GetCString().get()).c_str());
}

static JSTaggedValue NumberFormatCreateTest(JSThread *thread, JSHandle<JSObject> &options,
                                            JSHandle<JSTaggedValue> &locale)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetNumberFormatFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, options.GetTaggedValue());
    // construct numberformat
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue numberFormat = BuiltinsNumberFormat::NumberFormatConstructor(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    return numberFormat;
}

HWTEST_F_L0(BuiltinsNumberFormatTest, FormatToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> currencyKey = globalConst->GetHandledCurrencyString();

    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("currency"));
    JSHandle<JSTaggedValue> currencyValue(factory->NewFromASCII("EUR"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> numberVal(thread, JSTaggedValue(3500));
    
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, currencyKey, currencyValue);
    JSTaggedValue numberFormat = NumberFormatCreateTest(thread, optionsObj, localeString);
    JSHandle<JSTaggedValue> numberFormatVal(thread, numberFormat);
    // format currency
    JSTaggedValue formatResult = BuiltinsFormatTest(thread, optionsObj, numberVal, localeString);
    JSHandle<EcmaString> resultEcmaStr(thread, formatResult);
    EXPECT_STREQ("3.500,00 €", CString(resultEcmaStr->GetCString().get()).c_str());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(numberFormatVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, numberVal.GetTaggedValue());
    // format currency to part
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumberFormat::FormatToParts(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    
    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 10U); // "3","." ,"5" ,"0" ,"0" ,"," ,"0" ,0" ," " ,"€"
}

HWTEST_F_L0(BuiltinsNumberFormatTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
   
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> currencyKey = globalConst->GetHandledCurrencyString();

    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("currency"));
    JSHandle<JSTaggedValue> currencyValue(factory->NewFromASCII("USD"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en-US"));
    
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, currencyKey, currencyValue);
    JSTaggedValue numberFormat = NumberFormatCreateTest(thread, optionsObj, localeString);
    JSHandle<JSTaggedValue> numberFormatVal(thread, numberFormat);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(numberFormatVal.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsNumberFormat::ResolvedOptions(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jsnumberformat tag
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, styleKey).GetValue(), styleValue), true);
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, currencyKey).GetValue(), currencyValue), true);
}
}