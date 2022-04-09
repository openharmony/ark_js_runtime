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

#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JsonStringifierTest : public testing::Test {
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

static JSTaggedValue CreateBaseJSObject(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(globalEnv->GetObjectFunction());

    JSHandle<JSObject> jsObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    EXPECT_TRUE(*jsObject != nullptr);

    JSHandle<JSTaggedValue> handleKey1(factory->NewFromASCII(&keyCStr[0]));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1)); // 1 : test case
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), handleKey1, handleValue1);

    CString str2 = "x";
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII(str2));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(3.6)); // 3.6 : test case
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), handleKey2, handleValue2);

    CString str3 = "y";
    JSHandle<JSTaggedValue> handleKey3(factory->NewFromASCII(str3));
    JSHandle<JSTaggedValue> handleValue3(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), handleKey3, handleValue3);

    return jsObject.GetTaggedValue();
}

static JSTaggedValue TestForStringfy1([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // false: test case
    return JSTaggedValue(JSTaggedValue::False());
}

static JSTaggedValue TestForStringfy2([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 10.12: test case
    return JSTaggedValue(10.12);
}

/**
 * @tc.name: Stringify_001
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is JSFunction,the third parameter
 *           is Undefined.if the second parameter is JSFunction,the return value is the parameter stringification
 *           after through "call" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    
    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));
    JSHandle<JSFunction> handleFunc1 = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForStringfy1));
    JSHandle<JSFunction> handleFunc2 = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForStringfy2));
    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer1(thread, handleFunc1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer2(thread, handleFunc2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JsonStringifier stringifier1(thread);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleValue, handleReplacer1, handleGap);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("false", CString(handleEcmaStr1->GetCString().get()).c_str());

    JsonStringifier stringifier2(thread);
    JSHandle<JSTaggedValue> resultString2 = stringifier2.Stringify(handleValue, handleReplacer2, handleGap);
    EXPECT_TRUE(resultString2->IsString());
    JSHandle<EcmaString> handleEcmaStr2(resultString2);
    EXPECT_STREQ("10.12", CString(handleEcmaStr2->GetCString().get()).c_str());
}

/**
 * @tc.name: Stringify_002
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is Undefined,the third parameter
 *           is Number.This situation will stringize parameters through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_002)
{
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));
    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue(static_cast<int32_t>(10)));

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\n          \"z\": 1,\n          \"x\": 3.6,\n          \"y\": \"abc\"\n}",
                                                     CString(handleEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: Stringify_003
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is Undefined,the third parameter
 *           is String,This situation will stringize parameters through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, handleStr.GetTaggedValue());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> resultStr =
        factory->NewFromASCII("{\ntttt\"z\": 1,\ntttt\"x\": 3.6,\ntttt\"y\": \"abc\"\n}");
    EXPECT_EQ(resultStr->Compare(reinterpret_cast<EcmaString *>(resultString->GetRawData())), 0);
}

/**
 * @tc.name: Stringify_004
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is JSArray,the third parameter
 *           is String.This situation will stringize parameters through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleObj1 = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));

    JSArray *handleArr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> handleObj2(thread, handleArr);
    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleValue0(factory->NewFromASCII("z"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj2), handleKey0, handleValue0);

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue1(factory->NewFromASCII("x"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj2), handleKey1, handleValue1);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, handleObj2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleGap(thread, handleStr.GetTaggedValue());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\ntttt\"z\": 1,\ntttt\"x\": 3.6\n}", CString(handleEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: Stringify_005
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is Undefined,the third parameter
 *           is Undefined.This situation will stringize the first parameter through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_005)
{
    JsonStringifier stringifier(thread);
    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\"z\":1,\"x\":3.6,\"y\":\"abc\"}", CString(handleEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: Stringify_006
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the JSArray,the second parameter is Undefined,the third parameter
 *           is String,This situation will stringize parameters through "SerializeJSArray" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_006)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSArray *handleArr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> handleObj(thread, handleArr);

    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleValue0(factory->NewFromASCII("json"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj), handleKey0, handleValue0);

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    PropertyDescriptor handleDesc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(100)), true, true, true);
    JSArray::DefineOwnProperty(thread, handleObj, handleKey1, handleDesc);

    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj), handleKey2, handleValue2);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, handleStr.GetTaggedValue());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[\ntttt\"json\",\ntttt100,\ntttt\"abc\"\n]", CString(handleEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: Stringify_007
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the JSObject,the second parameter is Undefined,the third parameter
 *           is Undefined.This situation will stringize the first parameter through "SerializeJSArray" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_007)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSArray *handleArr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> handleObj(thread, handleArr);

    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    PropertyDescriptor handleDesc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, handleObj, handleKey0, handleDesc0);

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    PropertyDescriptor handleDesc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3.6)), true, true, true);
    JSArray::DefineOwnProperty(thread, handleObj, handleKey1, handleDesc1);

    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj), handleKey2, handleValue2);

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[1,3.6,\"abc\"]", CString(handleEcmaStr->GetCString().get()).c_str());
}

/**
 * @tc.name: Stringify_008
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the JSObject,the second parameter is Undefined,the third parameter
 *           is Undefined.This situation will stringize the first parameter through "SerializePrimitiveRef"
 *           function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_008)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleStr(factory->NewFromASCII("\"\\\b\f\n\r\t"));
    JSHandle<JSPrimitiveRef> handlePrimitiveRef = factory->NewJSString(handleStr);
    JSHandle<JSObject> handleObj(thread, handlePrimitiveRef.GetTaggedValue());

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("\"\\\"\\\\\\b\\f\\n\\r\\t\"", CString(handleEcmaStr->GetCString().get()).c_str());
}
}  // namespace panda::test
