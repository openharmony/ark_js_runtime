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

#include "ecmascript/js_regexp.h"

#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/regexp/regexp_parser_cache.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsRegExpTest : public testing::Test {
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
};

JSTaggedValue CreateRegExpObjByPatternAndFlags(JSThread *thread, const JSHandle<EcmaString> &pattern,
                                               const JSHandle<EcmaString> &flags)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> regexp(env->GetRegExpFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    // make dyn_runtime_call_info
    // 8 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*regexp), 8);
    ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, pattern.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, flags.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke RegExpConstructor method
    JSTaggedValue result = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo.get());
    return result;
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpConstructor1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\w+");
    JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
    JSTaggedValue result = CreateRegExpObjByPatternAndFlags(thread, pattern, flags);

    // ASSERT IsRegExp()
    JSHandle<JSTaggedValue> regexpObject(thread, result);
    ASSERT_TRUE(JSObject::IsRegExp(thread, regexpObject));

    JSHandle<JSRegExp> jsRegexp(thread, JSRegExp::Cast(regexpObject->GetTaggedObject()));
    JSHandle<JSTaggedValue> originalSource(thread, jsRegexp->GetOriginalSource());
    uint8_t flagsBits = static_cast<uint8_t>(jsRegexp->GetOriginalFlags().GetInt());
    JSHandle<JSTaggedValue> originalFlags(thread, BuiltinsRegExp::FlagsBitsToString(thread, flagsBits));
    ASSERT_EQ(static_cast<EcmaString *>(originalSource->GetTaggedObject())->Compare(*pattern), 0);
    ASSERT_EQ(static_cast<EcmaString *>(originalFlags->GetTaggedObject())->Compare(*flags), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpConstructor2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\w+");
    JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern, flags);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> regexp(env->GetRegExpFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*regexp), 8);
    ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke RegExpConstructor method
    JSTaggedValue result2 = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo.get());

    // ASSERT IsRegExp()
    JSHandle<JSTaggedValue> regexpObject(thread, result2);
    ASSERT_TRUE(JSObject::IsRegExp(thread, regexpObject));

    JSHandle<JSRegExp> jsRegexp(thread, JSRegExp::Cast(regexpObject->GetTaggedObject()));
    JSHandle<JSTaggedValue> originalSource(thread, jsRegexp->GetOriginalSource());
    uint8_t flagsBits = static_cast<uint8_t>(jsRegexp->GetOriginalFlags().GetInt());
    JSHandle<JSTaggedValue> originalFlags(thread, BuiltinsRegExp::FlagsBitsToString(thread, flagsBits));
    ASSERT_EQ(static_cast<EcmaString *>(originalSource->GetTaggedObject())->Compare(*pattern), 0);
    ASSERT_EQ(static_cast<EcmaString *>(originalFlags->GetTaggedObject())->Compare(*flags), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpConstructor3)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> regexp(env->GetRegExpFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSHandle<EcmaString> flags2 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("gi");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*regexp), 8);
    ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, flags2.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke RegExpConstructor method
    JSTaggedValue result2 = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo.get());

    // ASSERT IsRegExp()
    JSHandle<JSTaggedValue> regexpObject(thread, result2);
    ASSERT_TRUE(JSObject::IsRegExp(thread, regexpObject));

    JSHandle<JSRegExp> jsRegexp(thread, JSRegExp::Cast(regexpObject->GetTaggedObject()));
    JSHandle<JSTaggedValue> originalSource(thread, jsRegexp->GetOriginalSource());
    uint8_t flagsBits = static_cast<uint8_t>(jsRegexp->GetOriginalFlags().GetInt());
    JSHandle<JSTaggedValue> originalFlags(thread, BuiltinsRegExp::FlagsBitsToString(thread, flagsBits));
    ASSERT_EQ(static_cast<EcmaString *>(originalSource->GetTaggedObject())->Compare(*pattern1), 0);
    ASSERT_EQ(static_cast<EcmaString *>(originalFlags->GetTaggedObject())->Compare(*flags2), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, GetSource1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetSource method
    JSHandle<JSTaggedValue> source(
        thread, thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("source").GetTaggedValue());
    JSHandle<JSTaggedValue> sourceResult(JSObject::GetProperty(thread, result1Handle, source).GetValue());

    JSHandle<EcmaString> expect = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("(?:)");
    ASSERT_EQ(static_cast<EcmaString *>(sourceResult->GetTaggedObject())->Compare(*expect), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, GetSource2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("/w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetSource method
    JSHandle<JSTaggedValue> source(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("source"));
    JSHandle<JSTaggedValue> sourceResult(JSObject::GetProperty(thread, result1Handle, source).GetValue());

    JSHandle<EcmaString> expect = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\/w+");
    ASSERT_EQ(static_cast<EcmaString *>(sourceResult->GetTaggedObject())->Compare(*expect), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Get)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("gimuy");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    JSHandle<JSTaggedValue> global(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("global"));
    JSTaggedValue taggedGlobalResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, global).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedGlobalResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> ignoreCase(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("ignoreCase"));
    JSTaggedValue taggedIgnoreCaseResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, ignoreCase).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedIgnoreCaseResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> multiline(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("multiline"));
    JSTaggedValue taggedMultilineResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, multiline).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedMultilineResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> sticky(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("sticky"));
    JSTaggedValue taggedStickyResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, sticky).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedStickyResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> unicode(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("unicode"));
    JSTaggedValue taggedUnicodeResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, unicode).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedUnicodeResult.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsRegExpTest, GetFlags)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("imuyg");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetFlags method
    JSHandle<JSTaggedValue> flags(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("flags"));
    JSHandle<JSTaggedValue> flagsResult(JSObject::GetProperty(thread, result1Handle, flags).GetValue());

    JSHandle<EcmaString> expectResult = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("gimuy");
    ASSERT_EQ(static_cast<EcmaString *>(flagsResult->GetTaggedObject())->Compare(*expectResult), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, toString)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("imuyg");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke ToString method
    JSTaggedValue toStringResult = BuiltinsRegExp::ToString(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(toStringResult.IsString());
    JSHandle<JSTaggedValue> toStringResultHandle(thread, toStringResult);
    JSHandle<EcmaString> expectResult = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("/\\w+/gimuy");
    ASSERT_EQ(static_cast<EcmaString *>(toStringResultHandle->GetTaggedObject())->Compare(*expectResult), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Exec1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("ig");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("The Quick Brown Fox Jumps Over The Lazy Dog");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Exec method
    JSTaggedValue results = BuiltinsRegExp::Exec(ecmaRuntimeCallInfo.get());

    JSHandle<JSTaggedValue> execResult(thread, results);
    JSHandle<EcmaString> resultZero =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("Quick Brown Fox Jumps");
    JSHandle<EcmaString> resultOne = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("Brown");
    JSHandle<EcmaString> resultTwo = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("Jumps");

    JSHandle<JSTaggedValue> index(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("index"));
    JSHandle<JSTaggedValue> indexHandle(JSObject::GetProperty(thread, execResult, index).GetValue());
    uint32_t resultIndex = JSTaggedValue::ToUint32(thread, indexHandle);
    ASSERT_TRUE(resultIndex == 4U);

    JSHandle<JSTaggedValue> input(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("input"));

    JSHandle<JSTaggedValue> inputHandle(JSObject::GetProperty(thread, execResult, input).GetValue());
    JSHandle<EcmaString> outputInput = JSTaggedValue::ToString(thread, inputHandle);
    ASSERT_EQ(outputInput->Compare(*inputString), 0);

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, execResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(outputZero->Compare(*resultZero), 0);

    JSHandle<JSTaggedValue> first(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("1"));
    JSHandle<JSTaggedValue> oneHandle(JSObject::GetProperty(thread, execResult, first).GetValue());
    JSHandle<EcmaString> outputOne = JSTaggedValue::ToString(thread, oneHandle);
    ASSERT_EQ(outputOne->Compare(*resultOne), 0);

    JSHandle<JSTaggedValue> second(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("2"));
    JSHandle<JSTaggedValue> twoHandle(JSObject::GetProperty(thread, execResult, second).GetValue());
    JSHandle<EcmaString> outputTwo = JSTaggedValue::ToString(thread, twoHandle);
    ASSERT_EQ(outputTwo->Compare(*resultTwo), 0);

    JSHandle<JSTaggedValue> regexp = JSHandle<JSTaggedValue>::Cast(value);
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("lastIndex"));
    JSHandle<JSTaggedValue> lastIndexObj(JSObject::GetProperty(thread, regexp, lastIndexHandle).GetValue());
    int lastIndex = lastIndexObj->GetInt();
    ASSERT_TRUE(lastIndex == 25);
}

HWTEST_F_L0(BuiltinsRegExpTest, Exec2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("((1)|(12))((3)|(23))");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("ig");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("123");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Exec method
    JSTaggedValue results = BuiltinsRegExp::Exec(ecmaRuntimeCallInfo.get());

    JSHandle<JSTaggedValue> execResult(thread, results);
    JSHandle<EcmaString> resultZero = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("123");
    JSHandle<EcmaString> resultOne = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("1");
    JSHandle<EcmaString> resultTwo = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("1");
    JSHandle<EcmaString> resultFour = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("23");
    JSHandle<EcmaString> resultSix = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("23");

    JSHandle<JSTaggedValue> index(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("index"));
    JSHandle<JSTaggedValue> indexHandle(JSObject::GetProperty(thread, execResult, index).GetValue());
    uint32_t resultIndex = JSTaggedValue::ToUint32(thread, indexHandle);
    ASSERT_TRUE(resultIndex == 0U);

    JSHandle<JSTaggedValue> input(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("input"));
    JSHandle<JSTaggedValue> inputHandle(JSObject::GetProperty(thread, execResult, input).GetValue());
    JSHandle<EcmaString> outputInput = JSTaggedValue::ToString(thread, inputHandle);
    ASSERT_EQ(outputInput->Compare(*inputString), 0);

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, execResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(outputZero->Compare(*resultZero), 0);

    JSHandle<JSTaggedValue> first(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("1"));
    JSHandle<JSTaggedValue> oneHandle(JSObject::GetProperty(thread, execResult, first).GetValue());
    JSHandle<EcmaString> outputOne = JSTaggedValue::ToString(thread, oneHandle);
    ASSERT_EQ(outputOne->Compare(*resultOne), 0);

    JSHandle<JSTaggedValue> second(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("2"));
    JSHandle<JSTaggedValue> twoHandle(JSObject::GetProperty(thread, execResult, second).GetValue());
    JSHandle<EcmaString> outputTwo = JSTaggedValue::ToString(thread, twoHandle);
    ASSERT_EQ(outputTwo->Compare(*resultTwo), 0);

    JSHandle<JSTaggedValue> regexp = JSHandle<JSTaggedValue>::Cast(value);
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("lastIndex"));
    JSHandle<JSTaggedValue> lastIndexObj(JSObject::GetProperty(thread, regexp, lastIndexHandle).GetValue());
    int lastIndex = lastIndexObj->GetInt();
    ASSERT_TRUE(lastIndex == 3);

    JSHandle<JSTaggedValue> third(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("3"));
    JSHandle<JSTaggedValue> thirdHandle(JSObject::GetProperty(thread, execResult, third).GetValue());
    ASSERT_TRUE(thirdHandle->IsUndefined());

    JSHandle<JSTaggedValue> four(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("4"));
    JSHandle<JSTaggedValue> fourHandle(JSObject::GetProperty(thread, execResult, four).GetValue());
    JSHandle<EcmaString> outputFour = JSTaggedValue::ToString(thread, fourHandle);
    ASSERT_EQ(outputFour->Compare(*resultFour), 0);

    JSHandle<JSTaggedValue> five(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("5"));
    JSHandle<JSTaggedValue> fiveHandle(JSObject::GetProperty(thread, execResult, five).GetValue());
    ASSERT_TRUE(fiveHandle->IsUndefined());

    JSHandle<JSTaggedValue> six(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("6"));
    JSHandle<JSTaggedValue> sixHandle(JSObject::GetProperty(thread, execResult, six).GetValue());
    JSHandle<EcmaString> outputSix = JSTaggedValue::ToString(thread, sixHandle);
    ASSERT_EQ(outputSix->Compare(*resultSix), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Match1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("The Quick Brown Fox Jumps Over The Lazy Dog");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Match method
    JSTaggedValue matchResults = BuiltinsRegExp::Match(ecmaRuntimeCallInfo.get());

    JSHandle<JSTaggedValue> matchResult(thread, matchResults);
    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("0"));
    JSHandle<EcmaString> resultZero =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("Quick Brown Fox Jumps");
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, matchResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(outputZero->Compare(*resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Test1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("The Quick Brown Fox Jumps Over The Lazy Dog");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Test method
    JSTaggedValue testResult = BuiltinsRegExp::Test(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(testResult.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsRegExpTest, Search1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("The Quick Brown Fox Jumps Over The Lazy Dog");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Search method
    JSTaggedValue searchResult = BuiltinsRegExp::Search(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(searchResult.GetRawData(), JSTaggedValue(4).GetRawData());
}

HWTEST_F_L0(BuiltinsRegExpTest, Split1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("-");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Split method
    JSTaggedValue splitResults = BuiltinsRegExp::Split(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> splitResult(thread, splitResults);

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, splitResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);

    ASSERT_EQ(outputZero->Compare(*inputString), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Split2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("-");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("a-b-c");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke Split method
    JSTaggedValue splitResults = BuiltinsRegExp::Split(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> splitResult(thread, splitResults);
    JSHandle<EcmaString> resultZero = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("a");
    JSHandle<EcmaString> resultOne = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("b");
    JSHandle<EcmaString> resultTwo = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("c");

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, splitResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(outputZero->Compare(*resultZero), 0);

    JSHandle<JSTaggedValue> first(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("1"));
    JSHandle<JSTaggedValue> oneHandle(JSObject::GetProperty(thread, splitResult, first).GetValue());
    JSHandle<EcmaString> outputOne = JSTaggedValue::ToString(thread, oneHandle);
    ASSERT_EQ(outputOne->Compare(*resultOne), 0);

    JSHandle<JSTaggedValue> second(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("2"));
    JSHandle<JSTaggedValue> twoHandle(JSObject::GetProperty(thread, splitResult, second).GetValue());
    JSHandle<EcmaString> outputTwo = JSTaggedValue::ToString(thread, twoHandle);
    ASSERT_EQ(outputTwo->Compare(*resultTwo), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, GetSpecies)
{
    // invoke RegExpConstructor method
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol.GetTaggedValue().IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetRegExpFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    EXPECT_EQ(value, newTarget.GetTaggedValue());
}

HWTEST_F_L0(BuiltinsRegExpTest, Replace1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("iug");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("The Quick Brown Fox Jumps Over The Lazy Dog");
    JSHandle<EcmaString> replaceString =
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("$&a $` $\' $2 $01 $$1 $21 $32 a");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke replace method
    JSTaggedValue results = BuiltinsRegExp::Replace(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> replaceResult(thread, results);
    JSHandle<EcmaString> resultZero = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(
        "The Quick Brown Fox Jumpsa The   Over The Lazy Dog Jumps Brown $1 Jumps1 $32 a Over The Lazy Dog");
    ASSERT_EQ(static_cast<EcmaString *>(replaceResult->GetTaggedObject())->Compare(*resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Replace2)
{
    // invoke RegExpConstructor method
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> pattern1 = factory->NewFromCanBeCompressString("b(c)(z)?(.)");
    JSHandle<EcmaString> flags1 = factory->NewFromCanBeCompressString("");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = factory->NewFromCanBeCompressString("abcde");
    JSHandle<EcmaString> replaceString = factory->NewFromCanBeCompressString("[$01$02$03$04$00]");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke replace method
    JSTaggedValue results = BuiltinsRegExp::Replace(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> replaceResult(thread, results);
    JSHandle<EcmaString> resultZero = factory->NewFromCanBeCompressString("a[cd$04$00]e");
    ASSERT_EQ(static_cast<EcmaString *>(replaceResult->GetTaggedObject())->Compare(*resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Replace3)
{
    // invoke RegExpConstructor method
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> pattern1 = factory->NewFromCanBeCompressString("abc");
    JSHandle<EcmaString> flags1 = factory->NewFromCanBeCompressString("g");
    JSTaggedValue result1 = CreateRegExpObjByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = factory->NewFromCanBeCompressString("abcde");
    JSHandle<EcmaString> replaceString = factory->NewFromCanBeCompressString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    // invoke replace method
    JSTaggedValue results = BuiltinsRegExp::Replace(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> replaceResult(thread, results);
    JSHandle<EcmaString> resultZero = factory->NewFromCanBeCompressString("de");
    ASSERT_EQ(static_cast<EcmaString *>(replaceResult->GetTaggedObject())->Compare(*resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpParseCache)
{
    RegExpParserCache *regExpParserCache = thread->GetEcmaVM()->GetRegExpParserCache();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> string1 = factory->NewFromCanBeCompressString("abc");
    JSHandle<EcmaString> string2 = factory->NewFromCanBeCompressString("abcd");
    regExpParserCache->SetCache(*string1, 0, JSTaggedValue::True(), 2);
    ASSERT_TRUE(regExpParserCache->GetCache(*string1, 0).first == JSTaggedValue::True());
    ASSERT_TRUE(regExpParserCache->GetCache(*string1, 0).second == 2U);
    ASSERT_TRUE(regExpParserCache->GetCache(*string1, RegExpParserCache::CACHE_SIZE).first == JSTaggedValue::Hole());
    ASSERT_TRUE(regExpParserCache->GetCache(*string2, 0).first == JSTaggedValue::Hole());
}
}  // namespace panda::test
