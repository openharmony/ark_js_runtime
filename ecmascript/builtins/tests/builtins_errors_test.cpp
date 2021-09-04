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

#include "ecmascript/builtins/builtins_errors.h"

#include "ecmascript/base/error_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"

#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using Error = ecmascript::builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using URIError = builtins::BuiltinsURIError;
using EvalError = builtins::BuiltinsEvalError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using JSType = ecmascript::JSType;

class BuiltinsErrorsTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "BuiltinsErrorsTest SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "BuiltinsErrorsTest TearDownCase";
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

/*
 * @tc.name: GetJSErrorObject
 * @tc.desc: get JSError Object
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, GetJSErrorObject)
{
    /**
     * @tc.steps: step1. Create JSError object
     */
    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();

    JSHandle<JSObject> handleObj = factory->GetJSError(ErrorType::TYPE_ERROR);
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    /**
     * @tc.steps: step2. obtain JSError object prototype chain name property and message property
     */
    JSHandle<JSTaggedValue> msgValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), msgKey).GetValue());
    EXPECT_EQ(reinterpret_cast<EcmaString *>(msgValue->GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(
                      ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())),
              0);
    JSHandle<JSTaggedValue> nameValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("TypeError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: GetJSErrorWithMessage
 * @tc.desc: Obtains the TypeError object.
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, GetJSErrorWithMessage)
{
    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();

    JSHandle<JSObject> handleObj = factory->GetJSError(ErrorType::TYPE_ERROR, "I am type error");
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> msgValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("I am type error")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);
    JSHandle<JSTaggedValue> nameValue(
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(handleObj), nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("TypeError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: ErrorNoParameterConstructor
 * @tc.desc: new Error()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = Error::ErrorConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("Error")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
              0);
}

/*
 * @tc.name: ErrorParameterConstructor
 * @tc.desc: new Error("Hello Error!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello Error!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = Error::ErrorConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());

    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("Hello Error!")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
        0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("Error")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
              0);
}

/*
 * @tc.name: ErrorNoParameterToString
 * @tc.desc: new Error().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = Error::ToString(ecmaRuntimeCallInfo.get());

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("Error")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(*resultHandle)),
              0);
}

/*
 * @tc.name: ErrorToString
 * @tc.desc: new Error("This is Error!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(thread, factory->NewFromString("This is Error!").GetTaggedValue()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = Error::ToString(ecmaRuntimeCallInfo.get());

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Error: This is Error!")).GetRawData())
                  ->Compare(*resultHandle),
              0);
}

/*
 * @tc.name: RangeErrorNoParameterConstructor
 * @tc.desc: new RangeError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetRangeErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = RangeError::RangeErrorConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(JSTaggedValue(msgValue.GetTaggedValue()).GetRawData())),
              0);
    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("RangeError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(JSTaggedValue(nameValue.GetTaggedValue()).GetRawData())),
        0);
}

/*
 * @tc.name: RangeErrorParameterConstructor
 * @tc.desc: new RangeError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetRangeErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello RangeError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = RangeError::RangeErrorConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Hello RangeError!")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("RangeError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: RangeErrorNoParameterToString
 * @tc.desc: new RangeError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetRangeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = RangeError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<JSTaggedValue> resultHandle(thread, result);

    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("RangeError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(resultHandle->GetRawData())),
        0);
}

/*
 * @tc.name: RangeErrorToString
 * @tc.desc: new RangeError("This is RangeError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, RangeErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetRangeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(factory->NewFromString("This is RangeError!")));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = RangeError::ToString(ecmaRuntimeCallInfo.get());

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(factory->NewFromString("RangeError: This is RangeError!")->Compare(*resultHandle), 0);
}

/*
 * @tc.name: ReferenceErrorNoParameterConstructor
 * @tc.desc: new ReferenceError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetReferenceErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = ReferenceError::ReferenceErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("ReferenceError")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
              0);
}

/*
 * @tc.name: ReferenceErrorParameterConstructor
 * @tc.desc: new ReferenceError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetReferenceErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello ReferenceError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = ReferenceError::ReferenceErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();
    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Hello ReferenceError!")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("ReferenceError")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
              0);
}

/*
 * @tc.name: ReferenceErrorNoParameterToString
 * @tc.desc: new ReferenceError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetReferenceErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = ReferenceError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("ReferenceError")).GetRawData())
                  ->Compare(*resultHandle),
              0);
}

/*
 * @tc.name: ReferenceErrorToString
 * @tc.desc: new ReferenceError("This is ReferenceError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, ReferenceErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetReferenceErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(factory->NewFromString("This is ReferenceError!")));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = ReferenceError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(factory->NewFromString("ReferenceError: This is ReferenceError!")->Compare(*resultHandle), 0);
}

/*
 * @tc.name: TypeErrorNoParameterConstructor
 * @tc.desc: new TypeError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetTypeErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = TypeError::TypeErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    EXPECT_EQ(reinterpret_cast<EcmaString *>(JSTaggedValue(nameValue.GetTaggedValue()).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(JSTaggedValue(nameValue.GetTaggedValue()).GetRawData())),
              0);
}

/*
 * @tc.name: TypeErrorParameterConstructor
 * @tc.desc: new TypeError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetTypeErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello TypeError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = TypeError::TypeErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Hello TypeError!")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("TypeError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: TypeErrorNoParameterToString
 * @tc.desc: new TypeError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetTypeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = TypeError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("TypeError")).GetRawData())
            ->Compare(*resultHandle),
        0);
}

/*
 * @tc.name: TypeErrorToString
 * @tc.desc: new TypeError("This is TypeError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, TypeErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetTypeErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> value(factory->NewFromString("This is TypeError!"));
    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = TypeError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(factory->NewFromString("TypeError: This is TypeError!")->Compare(*resultHandle), 0);
}

/*
 * @tc.name: URIErrorNoParameterConstructor
 * @tc.desc: new URIError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetURIErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = URIError::URIErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("URIError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: URIErrorParameterConstructor
 * @tc.desc: new URIError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetURIErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello URIError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = URIError::URIErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Hello URIError!")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("URIError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: URIErrorNoParameterToString
 * @tc.desc: new URIError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetURIErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = URIError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("URIError")).GetRawData())
            ->Compare(*resultHandle),
        0);
}

/*
 * @tc.name: URIErrorToString
 * @tc.desc: new URIError("This is URIError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, URIErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetURIErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(
        thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
        JSHandle<JSTaggedValue>(thread, factory->NewFromString("This is URIError!").GetTaggedValue()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = URIError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("URIError: This is URIError!")).GetRawData())
                  ->Compare(*resultHandle),
              0);
}

/*
 * @tc.name: SyntaxErrorNoParameterConstructor
 * @tc.desc: new SyntaxError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetSyntaxErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = SyntaxError::SyntaxErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("SyntaxError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: SyntaxErrorParameterConstructor
 * @tc.desc: new SyntaxError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetSyntaxErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello SyntaxError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = SyntaxError::SyntaxErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Hello SyntaxError!")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("SyntaxError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: SyntaxErrorNoParameterToString
 * @tc.desc: new SyntaxError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetSyntaxErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = SyntaxError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("SyntaxError")).GetRawData())
            ->Compare(*resultHandle),
        0);
}

/*
 * @tc.name: SyntaxErrorToString
 * @tc.desc: new SyntaxError("This is SyntaxError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, SyntaxErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetSyntaxErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
                          JSHandle<JSTaggedValue>(factory->NewFromString("This is SyntaxError!")));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = SyntaxError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());

    EXPECT_EQ(factory->NewFromString("SyntaxError: This is SyntaxError!")->Compare(*resultHandle), 0);
}

/*
 * @tc.name: EvalErrorNoParameterConstructor
 * @tc.desc: new EvalError()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorNoParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetEvalErrorFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 4);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = EvalError::EvalErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("EvalError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: EvalErrorParameterConstructor
 * @tc.desc: new EvalError("Hello RangeError!")
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorParameterConstructor)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSFunction> error(env->GetEvalErrorFunction());
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromString("Hello EvalError!"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*error), 6);
    ecmaRuntimeCallInfo->SetFunction(error.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));
    ecmaRuntimeCallInfo->SetCallArg(0, paramMsg.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = EvalError::EvalErrorConstructor(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> errorObject(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromString("message"));
    JSHandle<JSTaggedValue> nameKey = thread->GlobalConstants()->GetHandledNameString();

    JSHandle<JSTaggedValue> msgValue(JSObject::GetProperty(thread, errorObject, msgKey).GetValue());
    ASSERT_EQ(reinterpret_cast<EcmaString *>(
                  ecmascript::JSTaggedValue(*factory->NewFromString("Hello EvalError!")).GetRawData())
                  ->Compare(reinterpret_cast<EcmaString *>(msgValue->GetRawData())),
              0);

    JSHandle<JSTaggedValue> nameValue(JSObject::GetProperty(thread, errorObject, nameKey).GetValue());
    ASSERT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("EvalError")).GetRawData())
            ->Compare(reinterpret_cast<EcmaString *>(nameValue->GetRawData())),
        0);
}

/*
 * @tc.name: EvalErrorNoParameterToString
 * @tc.desc: new EvalError().toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorNoParameterToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();
    JSHandle<JSTaggedValue> errorObject = env->GetEvalErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = EvalError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(
        reinterpret_cast<EcmaString *>(ecmascript::JSTaggedValue(*factory->NewFromString("EvalError")).GetRawData())
            ->Compare(*resultHandle),
        0);
}

/*
 * @tc.name: EvalErrorToString
 * @tc.desc: new EvalError("This is EvalError!").toString()
 * @tc.type: FUNC
 */
HWTEST_F_L0(BuiltinsErrorsTest, EvalErrorToString)
{
    ASSERT_NE(thread, nullptr);

    ObjectFactory *factory = EcmaVM::Cast(instance)->GetFactory();
    JSHandle<GlobalEnv> env = EcmaVM::Cast(instance)->GetGlobalEnv();

    JSHandle<JSTaggedValue> errorObject = env->GetEvalErrorFunction();
    JSHandle<JSObject> error = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(errorObject), errorObject);

    JSHandle<JSTaggedValue> handleMsgKey(factory->NewFromString("message"));
    JSObject::SetProperty(
        thread, JSHandle<JSTaggedValue>(error), handleMsgKey,
        JSHandle<JSTaggedValue>(thread, factory->NewFromString("This is EvalError!").GetTaggedValue()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*error));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = EvalError::ToString(ecmaRuntimeCallInfo.get());
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(factory->NewFromString("EvalError: This is EvalError!")->Compare(*resultHandle), 0);
}
}  // namespace panda::test
