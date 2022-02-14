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
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
using ArgsPosition = BuiltinsBase::ArgsPosition;
class BuiltinsBaseTest : public testing::Test {
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

/**
 * @tc.name: GetArgsArray
 * @tc.desc: Create msgs through "CreateEcmaRuntimeCallInfo" function,Set ArgsNumber and CallArg ,then call
 *           the "GetArgsArray" function to get an array value is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(BuiltinsBaseTest, GetArgsArray)
{
    array_size_t argvLength = 10;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argvLength);
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(1));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(2));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(3));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSHandle<TaggedArray> resultArray = BuiltinsBase::GetArgsArray(ecmaRuntimeCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(resultArray->GetLength(), 3);
    EXPECT_EQ(resultArray->Get(0).GetInt(), 1);
    EXPECT_EQ(resultArray->Get(1).GetInt(), 2);
    EXPECT_EQ(resultArray->Get(2).GetInt(), 3);
}

/**
 * @tc.name: BuiltinsBase_info_Get
 * @tc.desc: Create msgs through "CreateEcmaRuntimeCallInfo" function,then through "SetFunction","SetThis","SetCallArg"
 *           function set msgs,check result returned through "GetConstructor","GetFunction","GetThis","GetCallArg"
 *           function from BuiltinsBase is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(BuiltinsBaseTest, BuiltinsBase_info_Get)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> handleFunction(env->GetProxyFunction());
    JSHandle<JSObject> handleNewTarget(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(ArgsPosition::FIRST));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(ArgsPosition::SECOND));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(ArgsPosition::FOURTH));
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());

    EXPECT_TRUE(BuiltinsBase::GetConstructor(ecmaRuntimeCallInfo1.get())->IsUndefined());
    EXPECT_TRUE(BuiltinsBase::GetNewTarget(ecmaRuntimeCallInfo1.get())->IsUndefined());
    EXPECT_EQ(BuiltinsBase::GetCallArg(ecmaRuntimeCallInfo1.get(), 0)->GetInt(), ArgsPosition::FIRST);
    EXPECT_EQ(BuiltinsBase::GetCallArg(ecmaRuntimeCallInfo1.get(), 1)->GetInt(), ArgsPosition::SECOND);
    EXPECT_EQ(BuiltinsBase::GetCallArg(ecmaRuntimeCallInfo1.get(), 2)->GetInt(), ArgsPosition::FOURTH);
    TestHelper::TearDownFrame(thread, prev1);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*handleNewTarget), 6);
    ecmaRuntimeCallInfo2->SetFunction(handleFunction.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetThis(handleNewTarget.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());

    EXPECT_TRUE(BuiltinsBase::GetConstructor(ecmaRuntimeCallInfo2.get())->IsJSFunction());
    EXPECT_TRUE(BuiltinsBase::GetNewTarget(ecmaRuntimeCallInfo2.get())->IsJSGlobalObject());
    EXPECT_TRUE(BuiltinsBase::GetCallArg(ecmaRuntimeCallInfo2.get(), 0)->IsUndefined());
    TestHelper::TearDownFrame(thread, prev2);
}

/**
 * @tc.name: GetTaggedString
 * @tc.desc: Check whether the result returned through "GetTaggedString" function is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(BuiltinsBaseTest, GetTaggedString)
{
    char BuiltinsBaseStr1[] = "BuiltinsBase";
    JSTaggedValue resultStr1 = BuiltinsBase::GetTaggedString(thread, BuiltinsBaseStr1);
    EXPECT_TRUE(resultStr1.IsString());
    JSHandle<EcmaString> handleEcmaStr1(thread, resultStr1);
    EXPECT_STREQ(CString(handleEcmaStr1->GetCString().get()).c_str(), "BuiltinsBase");

    char BuiltinsBaseStr2[] = ""; // Empty String
    JSTaggedValue resultStr2 = BuiltinsBase::GetTaggedString(thread, BuiltinsBaseStr2);
    EXPECT_TRUE(resultStr2.IsString());
    JSHandle<EcmaString> handleEcmaStr2(thread, resultStr2);
    EXPECT_STREQ(CString(handleEcmaStr2->GetCString().get()).c_str(), "");
}

/**
 * @tc.name: GetTaggedInt
 * @tc.desc: Check whether the result returned through "GetTaggedInt" function is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(BuiltinsBaseTest, GetTaggedInt)
{
    EXPECT_EQ(BuiltinsBase::GetTaggedInt(1).GetInt(), 1);
    EXPECT_EQ(BuiltinsBase::GetTaggedInt(9).GetInt(), 9);
}

/**
 * @tc.name: GetTaggedDouble
 * @tc.desc: Check whether the result returned through "GetTaggedDouble" function is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(BuiltinsBaseTest, GetTaggedDouble)
{
    EXPECT_EQ(BuiltinsBase::GetTaggedDouble(1.1100).GetNumber(), 1.1100);
    EXPECT_EQ(BuiltinsBase::GetTaggedDouble(9.1200).GetNumber(), 9.1200);
}

/**
 * @tc.name: GetTaggedBoolean
 * @tc.desc: Check whether the result returned through "GetTaggedDouble" function is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(BuiltinsBaseTest, GetTaggedBoolean)
{
    EXPECT_EQ(BuiltinsBase::GetTaggedBoolean(false).GetRawData(), JSTaggedValue::False().GetRawData());
    EXPECT_EQ(BuiltinsBase::GetTaggedBoolean(true).GetRawData(),  JSTaggedValue::True().GetRawData());
}
}  // namespace panda::test