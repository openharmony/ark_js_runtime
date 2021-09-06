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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_array.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsStringTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    JSTaggedValue CreateRegExpObjByPatternAndFlags(JSThread *thread, const JSHandle<EcmaString> &pattern,
                                                   const JSHandle<EcmaString> &flags)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> regexp(env->GetRegExpFunction());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

        // 8 : test case
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, regexp.GetTaggedValue(), 8);
        ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, pattern.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, flags.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo.get());
        return result;
    }
};

HWTEST_F_L0(BuiltinsStringTest, StringConstructor1)
{
    ASSERT_NE(thread, nullptr);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> string(env->GetStringFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSHandle<EcmaString> string2 = factory->NewFromString("ABC");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, string.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo->SetFunction(string.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, string2.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::StringConstructor(ecmaRuntimeCallInfo.get());
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSHandle<JSPrimitiveRef> ref(thread, JSPrimitiveRef::Cast(value.GetTaggedObject()));
    JSTaggedValue test = factory->NewFromString("ABC").GetTaggedValue();
    ASSERT_EQ(
        EcmaString::Cast(ref->GetValue().GetTaggedObject())->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())),
        0);
}

// String.fromCharCode(65, 66, 67)
HWTEST_F_L0(BuiltinsStringTest, fromCharCode1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const double arg1 = 65;
    const double arg2 = 66;
    const double arg3 = 67;

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(arg1));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(arg2));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(arg3));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::FromCharCode(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(value.GetTaggedObject()));
    JSTaggedValue test = factory->NewFromString("ABC").GetTaggedValue();
    ASSERT_EQ(
        EcmaString::Cast(valueHandle->GetTaggedObject())->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())),
        0);
}

// String.fromCodePoint(65, 66, 67)
HWTEST_F_L0(BuiltinsStringTest, fromCodePoint1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const double arg1 = 65;
    const double arg2 = 66;
    const double arg3 = 67;

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(arg1));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(arg2));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(arg3));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::FromCodePoint(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("ABC").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "abcabcabc".charAt(5)
HWTEST_F_L0(BuiltinsStringTest, charAt1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisVal = factory->NewFromString("abcabcabc");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::CharAt(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("c").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "一二三四".charAt(2)
HWTEST_F_L0(BuiltinsStringTest, charAt2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisVal = factory->NewFromString("一二三四");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::CharAt(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("三").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "abcabcabc".charAt(-1)
HWTEST_F_L0(BuiltinsStringTest, charAt3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisVal = factory->NewFromString("abcabcabc");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::CharAt(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->GetEmptyString().GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "ABC".charCodeAt(0)
HWTEST_F_L0(BuiltinsStringTest, charCodeAt1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisVal = factory->NewFromString("ABC");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::CharCodeAt(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(65).GetRawData());
}

// "ABC".charCodeAt(-1)
HWTEST_F_L0(BuiltinsStringTest, charCodeAt2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisVal = factory->NewFromString("ABC");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(-1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::CharCodeAt(ecmaRuntimeCallInfo.get());

    JSTaggedValue test = BuiltinsString::GetTaggedDouble(base::NAN_VALUE);
    ASSERT_EQ(result.GetRawData(), test.GetRawData());
}

// "ABC".codePointAt(1)
HWTEST_F_L0(BuiltinsStringTest, codePointAt1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisVal = factory->NewFromString("ABC");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisVal.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::CodePointAt(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(66).GetRawData());
}

// 'a'.concat('b', 'c', 'd')
HWTEST_F_L0(BuiltinsStringTest, concat1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("a");
    JSHandle<EcmaString> val1 = factory->NewFromString("b");
    JSHandle<EcmaString> val2 = factory->NewFromString("c");
    JSHandle<EcmaString> val3 = factory->NewFromString("d");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val1.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, val2.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, val3.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Concat(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("abcd").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "abcabcabc".indexof('b')
HWTEST_F_L0(BuiltinsStringTest, indexof1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::IndexOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

// "abcabcabc".indexof('b', 2)
HWTEST_F_L0(BuiltinsStringTest, indexof2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::IndexOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(4).GetRawData());
}

// "abcabcabc".indexof('d')
HWTEST_F_L0(BuiltinsStringTest, indexof3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("d");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::IndexOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());
}

// "abcabcabc".lastIndexOf('b')
HWTEST_F_L0(BuiltinsStringTest, lastIndexOf1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::LastIndexOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(7).GetRawData());
}
// "abcabcabc".lastIndexOf('b', 2)
HWTEST_F_L0(BuiltinsStringTest, lastIndexOf2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::LastIndexOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

// "abcabcabc".lastIndexOf('d')
HWTEST_F_L0(BuiltinsStringTest, lastIndexOf3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("d");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::LastIndexOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());
}

// "abcabcabc".includes('b')
HWTEST_F_L0(BuiltinsStringTest, Includes2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Includes(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// "abccccccc".includes('b'，2)
HWTEST_F_L0(BuiltinsStringTest, Includes3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abccccccc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Includes(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// "一二三四".includes('二')
HWTEST_F_L0(BuiltinsStringTest, Includes4)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("一二三四");
    JSHandle<EcmaString> val = factory->NewFromString("二");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Includes(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// "To be, or not to be, that is the question.".startsWith('To be')
HWTEST_F_L0(BuiltinsStringTest, startsWith1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("To be, or not to be, that is the question.");
    JSHandle<EcmaString> val = factory->NewFromString("To be");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::StartsWith(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// "To be, or not to be, that is the question.".startsWith('not to be')
HWTEST_F_L0(BuiltinsStringTest, startsWith2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("To be, or not to be, that is the question.");
    JSHandle<EcmaString> val = factory->NewFromString("not to be");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::StartsWith(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// "To be, or not to be, that is the question.".startsWith('not to be', 10)
HWTEST_F_L0(BuiltinsStringTest, startsWith3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("To be, or not to be, that is the question.");
    JSHandle<EcmaString> val = factory->NewFromString("not to be");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::StartsWith(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// "To be, or not to be, that is the question.".endsWith('question.')
HWTEST_F_L0(BuiltinsStringTest, endsWith1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("To be, or not to be, that is the question.");
    JSHandle<EcmaString> val = factory->NewFromString("question.");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::EndsWith(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// "To be, or not to be, that is the question.".endsWith('to be')
HWTEST_F_L0(BuiltinsStringTest, endsWith2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("To be, or not to be, that is the question.");
    JSHandle<EcmaString> val = factory->NewFromString("to be");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::EndsWith(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// "To be, or not to be, that is the question.".endsWith('to be', 19)
HWTEST_F_L0(BuiltinsStringTest, endsWith3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("To be, or not to be, that is the question.");
    JSHandle<EcmaString> val = factory->NewFromString("to be");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(19)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::EndsWith(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// "ABC".toLowerCase()
HWTEST_F_L0(BuiltinsStringTest, toLowerCase1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("ABC");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::ToLowerCase(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> test = factory->NewFromString("abc");
    ASSERT_TRUE(JSTaggedValue::SameValue(resultHandle.GetTaggedValue(), test.GetTaggedValue()));
}

// "abc".toUpperCase()
HWTEST_F_L0(BuiltinsStringTest, toUpperCase1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abc");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::ToUpperCase(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("ABC").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "abc".localecompare('b')
HWTEST_F_L0(BuiltinsStringTest, localecompare1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abc");
    JSHandle<EcmaString> val = factory->NewFromString("b");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::LocaleCompare(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());
}

// "abc".localecompare('abc')
HWTEST_F_L0(BuiltinsStringTest, localecompare2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abc");
    JSHandle<EcmaString> val = factory->NewFromString("abc");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::LocaleCompare(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(0).GetRawData());
}

// "abc".localecompare('aa')
HWTEST_F_L0(BuiltinsStringTest, localecompare3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abc");
    JSHandle<EcmaString> val = factory->NewFromString("aa");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, val.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::LocaleCompare(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

// "abc".repeat(5)
HWTEST_F_L0(BuiltinsStringTest, repeat1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abc");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Repeat(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("abcabcabcabcabc").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// 'The morning is upon us.'.slice(4, -2)
HWTEST_F_L0(BuiltinsStringTest, slice1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("The morning is upon us.");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(4)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(-2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Slice(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("morning is upon u").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// 'The morning is upon us.'.slice(12)
HWTEST_F_L0(BuiltinsStringTest, slice2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("The morning is upon us.");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(12)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Slice(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("is upon us.").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// 'Mozilla'.substring(3, -3)
HWTEST_F_L0(BuiltinsStringTest, substring1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("Mozilla");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(3)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(-3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Substring(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("Moz").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// 'Mozilla'.substring(7, 4)
HWTEST_F_L0(BuiltinsStringTest, substring2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("Mozilla");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(7)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(4)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Substring(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("lla").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

// "   Hello world!   ".trim()
HWTEST_F_L0(BuiltinsStringTest, trim1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("   Hello world!   ");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Trim(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("Hello world!").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

HWTEST_F_L0(BuiltinsStringTest, trim2)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("   Hello world!   ");
    JSHandle<JSFunction> stringObject(env->GetStringFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(thisStr.GetTaggedValue().GetTaggedObject()));
    JSHandle<JSPrimitiveRef> str = factory->NewJSPrimitiveRef(stringObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Trim(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = factory->NewFromString("Hello world!").GetTaggedValue();
    ASSERT_EQ(resultHandle->Compare(reinterpret_cast<EcmaString *>(test.GetRawData())), 0);
}

HWTEST_F_L0(BuiltinsStringTest, ToString)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<JSFunction> stringObject(env->GetStringFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(thisStr.GetTaggedValue().GetTaggedObject()));
    JSHandle<JSPrimitiveRef> str = factory->NewJSPrimitiveRef(stringObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::ToString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = JSTaggedValue(*thisStr);
    ASSERT_EQ(result.GetRawData(), test.GetRawData());
}

HWTEST_F_L0(BuiltinsStringTest, ValueOf)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("abcabcabc");
    JSHandle<JSFunction> stringObject(env->GetStringFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(thisStr.GetTaggedValue().GetTaggedObject()));
    JSHandle<JSPrimitiveRef> str = factory->NewJSPrimitiveRef(stringObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::ValueOf(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSTaggedValue test = JSTaggedValue(*thisStr);
    ASSERT_EQ(result.GetRawData(), test.GetRawData());
}

static inline JSFunction *BuiltinsStringTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(BuiltinsStringTest, Raw)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> foo(factory->NewFromString("foo"));
    JSHandle<JSTaggedValue> bar(factory->NewFromString("bar"));
    JSHandle<JSTaggedValue> baz(factory->NewFromString("baz"));
    JSHandle<JSTaggedValue> rawArray = JSHandle<JSTaggedValue>::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSObject> obj(rawArray);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, foo);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, bar);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, baz);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSTaggedValue> constructor(thread, BuiltinsStringTestCreate(thread));
    JSHandle<JSTaggedValue> templateString(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<JSTaggedValue> rawKey(factory->NewFromString("raw"));
    JSObject::SetProperty(thread, templateString, rawKey, rawArray);
    JSHandle<EcmaString> test = factory->NewFromString("foo5barJavaScriptbaz");

    JSHandle<EcmaString> javascript = factory->NewFromString("JavaScript");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(templateString.GetObject<EcmaString>()));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo->SetCallArg(2, javascript.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Raw(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result.GetRawData()), *test));
}

HWTEST_F_L0(BuiltinsStringTest, Replace)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("Twas the night before Xmas...");
    JSHandle<EcmaString> searchStr = factory->NewFromString("Xmas");
    JSHandle<EcmaString> replaceStr = factory->NewFromString("Christmas");
    JSHandle<EcmaString> expected = factory->NewFromString("Twas the night before Christmas...");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Replace(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result.GetRawData()), *expected));

    JSHandle<EcmaString> replaceStr1 = factory->NewFromString("abc$$");
    JSHandle<EcmaString> expected1 = factory->NewFromString("Twas the night before abc$...");

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, replaceStr1.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result1 = BuiltinsString::Replace(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> resultString1(thread, result1);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString1, *expected1));

    JSHandle<EcmaString> replaceStr2 = factory->NewFromString("abc$$dd");
    JSHandle<EcmaString> expected2 = factory->NewFromString("Twas the night before abc$dd...");

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, replaceStr2.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsString::Replace(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> resultString2(thread, result2);
    ASSERT_TRUE(result2.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString2, *expected2));

    JSHandle<EcmaString> replaceStr3 = factory->NewFromString("abc$&dd");
    JSHandle<EcmaString> expected3 = factory->NewFromString("Twas the night before abcXmasdd...");

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(1, replaceStr3.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    JSTaggedValue result3 = BuiltinsString::Replace(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> resultString3(thread, result3);
    ASSERT_TRUE(result3.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString3, *expected3));

    JSHandle<EcmaString> replaceStr4 = factory->NewFromString("abc$`dd");
    JSHandle<EcmaString> expected4 = factory->NewFromString("Twas the night before abcTwas the night before dd...");

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(1, replaceStr4.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    JSTaggedValue result4 = BuiltinsString::Replace(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> resultString4(thread, result4);
    ASSERT_TRUE(result4.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString4, *expected4));
}

HWTEST_F_L0(BuiltinsStringTest, Replace2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("Twas the night before Xmas...");
    JSHandle<EcmaString> searchStr = factory->NewFromString("Xmas");
    JSHandle<EcmaString> replaceStr = factory->NewFromString("abc$\'dd");
    JSHandle<EcmaString> expected = factory->NewFromString("Twas the night before abc...dd...");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Replace(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result.GetRawData()), *expected));

    JSHandle<EcmaString> replaceStr2 = factory->NewFromString("abc$`dd$\'$ff");
    JSHandle<EcmaString> expected2 =
        factory->NewFromString("Twas the night before abcTwas the night before dd...$ff...");

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(1, replaceStr2.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    JSTaggedValue result2 = BuiltinsString::Replace(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> resultString2(thread, result2);
    ASSERT_TRUE(result2.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString2, *expected2));

    JSHandle<EcmaString> replaceStr3 = factory->NewFromString("abc$`dd$\'$");
    JSHandle<EcmaString> expected3 = factory->NewFromString("Twas the night before abcTwas the night before dd...$...");

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(1, replaceStr3.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    JSTaggedValue result3 = BuiltinsString::Replace(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> resultString3(thread, result3);
    ASSERT_TRUE(result3.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString3, *expected3));

    JSHandle<EcmaString> replaceStr4 = factory->NewFromString("abc$`dd$$");
    JSHandle<EcmaString> expected4 = factory->NewFromString("Twas the night before abcTwas the night before dd$...");

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(1, replaceStr4.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    JSTaggedValue result4 = BuiltinsString::Replace(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result4.IsString());
    JSHandle<EcmaString> resultString4(thread, result4);
    ASSERT_TRUE(EcmaString::StringsAreEqual(*resultString4, *expected4));
}

HWTEST_F_L0(BuiltinsStringTest, Replace3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("Twas the night before Xmas...");
    JSHandle<EcmaString> searchStr = factory->NewFromString("Xmas");
    JSHandle<EcmaString> replaceStr = factory->NewFromString("$&a $` $\' $2 $01 $$1 $21 $32 a");
    JSHandle<EcmaString> expected =
        factory->NewFromString("Twas the night before Xmasa Twas the night before  ... $2 $01 $1 $21 $32 a...");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Replace(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result.GetRawData()), *expected));
}

HWTEST_F_L0(BuiltinsStringTest, Replace4)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromString("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> searchStr(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));
    JSHandle<EcmaString> expected = thread->GetEcmaVM()->GetFactory()->NewFromString(
        "The Quick Brown Fox Jumpsa The   Over The Lazy Dog Jumps Brown $1 Jumps1 $32 a Over The Lazy Dog");

    // make dyn_runtime_call_info2
    JSHandle<EcmaString> thisStr =
        thread->GetEcmaVM()->GetFactory()->NewFromString("The Quick Brown Fox Jumps Over The Lazy Dog");
    JSHandle<EcmaString> replaceStr =
        thread->GetEcmaVM()->GetFactory()->NewFromString("$&a $` $\' $2 $01 $$1 $21 $32 a");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, searchStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceStr.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Replace(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaString::StringsAreEqual(reinterpret_cast<EcmaString *>(result.GetRawData()), *expected));
}

HWTEST_F_L0(BuiltinsStringTest, Split)
{
    // invoke RegExpConstructor method
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("Hello World. How are you doing?");
    JSHandle<EcmaString> separatorStr = factory->NewFromString(" ");
    JSHandle<JSTaggedValue> limit(thread, JSTaggedValue(3));
    JSHandle<EcmaString> expected1 = factory->NewFromString("Hello");
    JSHandle<EcmaString> expected2 = factory->NewFromString("World.");
    JSHandle<EcmaString> expected3 = factory->NewFromString("How");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, separatorStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Split(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());
    JSHandle<JSArray> resultArray(thread, reinterpret_cast<JSArray *>(result.GetRawData()));
    ASSERT_TRUE(resultArray->IsJSArray());
    JSHandle<JSTaggedValue> resultObj(resultArray);
    JSHandle<EcmaString> string1(
        JSObject::GetProperty(thread, resultObj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))).GetValue());
    JSHandle<EcmaString> string2(
        JSObject::GetProperty(thread, resultObj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1))).GetValue());
    JSHandle<EcmaString> string3(
        JSObject::GetProperty(thread, resultObj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2))).GetValue());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*string1, *expected1));
    ASSERT_TRUE(EcmaString::StringsAreEqual(*string2, *expected2));
    ASSERT_TRUE(EcmaString::StringsAreEqual(*string3, *expected3));
}

HWTEST_F_L0(BuiltinsStringTest, Split2)
{
    // invoke RegExpConstructor method
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> thisStr = factory->NewFromString("a-b-c");
    JSHandle<EcmaString> pattern1 = factory->NewFromString("-");
    JSHandle<EcmaString> flags1 = factory->NewFromString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> separatorObj(thread, result1);

    JSHandle<JSTaggedValue> limit(thread, JSTaggedValue(3));
    JSHandle<EcmaString> expected1 = factory->NewFromString("a");
    JSHandle<EcmaString> expected2 = factory->NewFromString("b");
    JSHandle<EcmaString> expected3 = factory->NewFromString("c");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisStr.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, separatorObj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsString::Split(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());
    JSHandle<JSArray> resultArray(thread, result);
    ASSERT_TRUE(resultArray->IsJSArray());
    JSHandle<JSTaggedValue> resultObj(resultArray);
    JSHandle<EcmaString> string1(
        JSObject::GetProperty(thread, resultObj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))).GetValue());
    JSHandle<EcmaString> string2(
        JSObject::GetProperty(thread, resultObj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1))).GetValue());
    JSHandle<EcmaString> string3(
        JSObject::GetProperty(thread, resultObj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2))).GetValue());
    ASSERT_TRUE(EcmaString::StringsAreEqual(*string1, *expected1));
    ASSERT_TRUE(EcmaString::StringsAreEqual(*string2, *expected2));
    ASSERT_TRUE(EcmaString::StringsAreEqual(*string3, *expected3));
}
}  // namespace panda::test
