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

#include "ecmascript/js_number_format.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSNumberFormatTest : public testing::Test {
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

/**
 * @tc.name: GetIcuCallTarget
 * @tc.desc: Call "NewJSIntlIcuData" function Set IcuCallTarget,check whether the IcuCallTarget through
 *           "GetIcuCallTarget" function is within expectations then call "formatInt" function format
 *           Int type data and check the returned value is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSNumberFormatTest, GetIcuCallTarget)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetNumberFormatFunction();
    JSHandle<JSNumberFormat> numberFormat =
        JSHandle<JSNumberFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    icu::Locale icuLocale("en", "US");
    icu::number::LocalizedNumberFormatter icuNumberFormatter =
        icu::number::NumberFormatter::withLocale(icuLocale).roundingMode(UNUM_ROUND_HALFUP);
    // Set IcuCallTarget
    factory->NewJSIntlIcuData(numberFormat, icuNumberFormatter, JSNumberFormat::FreeIcuNumberformat);
    icu::number::LocalizedNumberFormatter *resultIcuNumberFormatter = numberFormat->GetIcuCallTarget();
    EXPECT_TRUE(resultIcuNumberFormatter != nullptr);
    // Use IcuCallTarget format Int
    int64_t value = -123456;
    UErrorCode status = U_ZERO_ERROR;
    icu::number::FormattedNumber formattedNumber = resultIcuNumberFormatter->formatInt(value, status);
    icu::UnicodeString result = formattedNumber.toString(status);
    JSHandle<EcmaString> stringValue = JSLocale::IcuToString(thread, result);
    EXPECT_STREQ("-123,456", CString(stringValue->GetCString().get()).c_str());
}

/**
 * @tc.name: InitializeNumberFormat
 * @tc.desc: Call "InitializeNumberFormat" function Initialize NumberFormat,and check whether the properties of
 *           the object is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSNumberFormatTest, InitializeNumberFormat)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetNumberFormatFunction();
    JSHandle<JSNumberFormat> numberFormat =
        JSHandle<JSNumberFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    EXPECT_TRUE(*numberFormat != nullptr);

    JSHandle<JSTaggedValue> locales(factory->NewFromASCII("zh-Hans-CN"));
    JSHandle<JSTaggedValue> undefinedOptions(thread, JSTaggedValue::Undefined());
    JSNumberFormat::InitializeNumberFormat(thread, numberFormat, locales, undefinedOptions);
    // Initialize attribute comparison
    EXPECT_TRUE(numberFormat->GetNumberingSystem().IsUndefined());
    JSHandle<EcmaString> localeStr(thread, numberFormat->GetLocale().GetTaggedObject());
    EXPECT_STREQ("zh-Hans-CN", CString(localeStr->GetCString().get()).c_str());
    EXPECT_EQ(NotationOption::STANDARD, numberFormat->GetNotation());
    EXPECT_EQ(CompactDisplayOption::SHORT, numberFormat->GetCompactDisplay());
    EXPECT_EQ(SignDisplayOption::AUTO, numberFormat->GetSignDisplay());
    EXPECT_EQ(CurrencyDisplayOption::SYMBOL, numberFormat->GetCurrencyDisplay());
    EXPECT_EQ(CurrencySignOption::STANDARD, numberFormat->GetCurrencySign());
    EXPECT_EQ(UnitDisplayOption::SHORT, numberFormat->GetUnitDisplay());
    EXPECT_EQ(numberFormat->GetMinimumIntegerDigits().GetInt(), 1); // 1 : 1 default minimum integer
    EXPECT_EQ(numberFormat->GetMinimumFractionDigits().GetInt(), 0); // 0 : 0 default minimum fraction
    EXPECT_EQ(numberFormat->GetMaximumFractionDigits().GetInt(), 3); // 1 : 1 default maximum fraction
    EXPECT_EQ(numberFormat->GetMinimumSignificantDigits().GetInt(), 0); // 0 : 0 default minimum sigfraction
    EXPECT_EQ(numberFormat->GetMaximumSignificantDigits().GetInt(), 0); // 0 : 0 default maximum sigfraction
    EXPECT_TRUE(numberFormat->GetUseGrouping().IsTrue());
    EXPECT_TRUE(numberFormat->GetBoundFormat().IsUndefined());
    EXPECT_TRUE(numberFormat->GetUnit().IsUndefined());
    EXPECT_TRUE(numberFormat->GetCurrency().IsUndefined());
}

/**
 * @tc.name: CurrencyDigits
 * @tc.desc: If the ISO 4217 currency contains currency as an alphabetic code, return the minor unit value
 *           corresponding to the currency from the list
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSNumberFormatTest, CurrencyDigits)
{
    // Alphabetic code:USD
    icu::UnicodeString usdCurrency("USD");
    // 2 : 2 fraction digits
    EXPECT_EQ(JSNumberFormat::CurrencyDigits(usdCurrency), 2);
    // Alphabetic code:EUR
    icu::UnicodeString eurCurrency("EUR");
    // 2 : 2 fraction digits
    EXPECT_EQ(JSNumberFormat::CurrencyDigits(eurCurrency), 2);
    // Alphabetic code:CHF
    icu::UnicodeString numberCurrency("CHF");
    // 2 : 2 fraction digits
    EXPECT_EQ(JSNumberFormat::CurrencyDigits(numberCurrency), 2);
}

/**
 * @tc.name: FormatNumeric
 * @tc.desc: Call "InitializeNumberFormat" function Initialize NumberFormat,Set the sytle attribute of the object to
 *           decimal,construct a bigint type data,and the object calls the FormatNumeric method to interpret the bigint
 *           type data into the corresponding decimal, and check whether the decimal meets the expectation.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSNumberFormatTest, FormatNumeric)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> ctor = env->GetNumberFormatFunction();
    JSHandle<JSNumberFormat> numberFormat =
        JSHandle<JSNumberFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));

    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("decimal"));
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en-US"));
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSNumberFormat::InitializeNumberFormat(thread, numberFormat, localeString, JSHandle<JSTaggedValue>(optionsObj));
    // format BigInt
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(123456789));
    JSHandle<BigInt> jsBigInt(thread, BigInt::NumberToBigInt(thread, number));
    JSHandle<JSTaggedValue> formatResult =
        JSNumberFormat::FormatNumeric(thread, numberFormat, jsBigInt.GetTaggedValue());

    JSHandle<EcmaString> resultEcmaStr(thread, formatResult.GetTaggedValue());
    EXPECT_STREQ("123,456,789", CString(resultEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: UnwrapNumberFormat
 * @tc.desc: Construct an object,If it is a numberformat object,it will be returned.If it is an object of other types
 *           and inherits the numberformat object, it will get value from the fallbacksymbol key and return.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSNumberFormatTest, UnwrapNumberFormat)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    EcmaVM *vm = thread->GetEcmaVM();

    JSHandle<JSTaggedValue> numberFormatFunc = env->GetNumberFormatFunction();
    JSHandle<JSTaggedValue> numberFormat(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(numberFormatFunc), numberFormatFunc));

    Local<FunctionRef> numberFormatLocal = JSNApiHelper::ToLocal<FunctionRef>(numberFormatFunc);
    JSHandle<JSTaggedValue> disPlayNamesFunc = env->GetDisplayNamesFunction();
    Local<FunctionRef> disPlayNamesLocal = JSNApiHelper::ToLocal<FunctionRef>(disPlayNamesFunc);
    // displaynames Inherit numberformat
    disPlayNamesLocal->Inherit(vm, numberFormatLocal);
    JSHandle<JSTaggedValue> disPlayNamesHandle = JSNApiHelper::ToJSHandle(disPlayNamesLocal);
    JSHandle<JSTaggedValue> disPlayNamesObj(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(disPlayNamesHandle), disPlayNamesHandle));
    // object has no Instance
    JSHandle<JSTaggedValue> unwrapNumberFormat1 = JSNumberFormat::UnwrapNumberFormat(thread, numberFormat);
    EXPECT_TRUE(JSTaggedValue::SameValue(numberFormat, unwrapNumberFormat1));
    // object has Instance
    JSHandle<JSTaggedValue> unwrapNumberFormat2 = JSNumberFormat::UnwrapNumberFormat(thread, disPlayNamesObj);
    EXPECT_TRUE(unwrapNumberFormat2->IsUndefined());
}
}  // namespace panda::test