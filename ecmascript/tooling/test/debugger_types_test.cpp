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

#include "ecmascript/js_array.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tooling/base/pt_types.h"
#include "ecmascript/tooling/dispatcher.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {

// Duplicate name of panda::ecmascript::PropertyDescriptor in js_object-inl.h
using panda::ecmascript::tooling::PropertyDescriptor;

using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

class DebuggerTypesTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
        Logger::InitializeStdLogging(Logger::Level::FATAL, LoggerComponentMaskAll);
    }

    static void TearDownTestCase()
    {
        Logger::InitializeStdLogging(Logger::Level::ERROR, LoggerComponentMaskAll);
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        ecmaVm = EcmaVM::Cast(instance);
        // Main logic is JSON parser, so not need trigger GC to decrease execute time
        ecmaVm->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(DebuggerTypesTest, RemoteObjectCreateTest)
{
    std::string msg;
    std::unique_ptr<RemoteObject> remoteObject;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = 100, ]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = [ "sub": "test" ] }, ]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", ]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object + R"("}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());

    // abnormal params of params.sub-key = [ type = "object", subtype = "unknown"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","subtype":"unknown"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", subtype = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","subtype":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", subtype = "array"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","subtype":")" + ObjectSubType::Array + R"("}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasSubType());
    EXPECT_EQ(ObjectSubType::Array, remoteObject->GetSubType());

    // abnormal params of params.sub-key = [ type = "object", className = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","className":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", className = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","className":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", className = "TestClass"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","className":"TestClass"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasClassName());
    EXPECT_EQ("TestClass", remoteObject->GetClassName());

    // normal params of params.sub-key = [ type = "object", value = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","value":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());

    // normal params of params.sub-key = [ type = "object", value = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","value":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());

    // normal params of params.sub-key = [ type = "object", value = "Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","value":"Test"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());

    // abnormal params of params.sub-key = [ type = "object", unserializableValue = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","unserializableValue":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", unserializableValue = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","unserializableValue":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", unserializableValue = "TestClass"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","unserializableValue":"Test"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasUnserializableValue());
    EXPECT_EQ("Test", remoteObject->GetUnserializableValue());

    // abnormal params of params.sub-key = [ type = "object", description = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","description":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", description = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","description":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", description = "Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","description":"Test"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasDescription());
    EXPECT_EQ("Test", remoteObject->GetDescription());

    // abnormal params of params.sub-key = [ type = "object", objectId = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","objectId":100}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", objectId = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","objectId":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", objectId = "id_1"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","objectId":"1"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasObjectId());
    EXPECT_EQ(remoteObject->GetObjectId(), 1);
}

HWTEST_F_L0(DebuggerTypesTest, RemoteObjectToJsonTest)
{
    std::string msg;
    std::unique_ptr<RemoteObject> remoteObject;
    std::string tmpStr;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
        R"(", "subtype":")" + ObjectSubType::Array +
        R"(","className":"TestClass","value":100, "unserializableValue":"Test","description":"Test","objectId":"1"}})";
    remoteObject = RemoteObject::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(remoteObject, nullptr);
    auto objJson = remoteObject->ToJson();

    ret = objJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);

    ret = objJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Array.c_str()), tmpStr);

    ret = objJson->GetString("className", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("TestClass", tmpStr);

    ret = objJson->GetString("unserializableValue", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("Test", tmpStr);

    ret = objJson->GetString("description", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("Test", tmpStr);

    ret = objJson->GetString("objectId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("1", tmpStr);
}

HWTEST_F_L0(DebuggerTypesTest, ExceptionDetailsCreateTest)
{
    std::string msg;
    std::unique_ptr<ExceptionDetails> exceptionMetaData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId="Test","text"="text0","lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":"Test","text":"text0","lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId={"xx":"yy"},"text"="text0","lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":{"xx":"yy"},"text":"text0","lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"=10,"lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":10,"lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"=["text0"],"lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":["text0"],"lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"="10","columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":"10","columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=["10"],"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":["10"],"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"="20"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":"20"}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=["20"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":["20"]}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"scriptId"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":10}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"scriptId"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"scriptId"="id0"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":"0"}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);
    EXPECT_EQ(exceptionMetaData->GetScriptId(), 0);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"url"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"url":10}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"url"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"url":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"url"="url0"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"url":"url0"}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);
    EXPECT_EQ("url0", exceptionMetaData->GetUrl());

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"exception"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"exception":10}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"exception"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"exception":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"exception"={}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"exception":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Error + R"("}}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);
    RemoteObject *exception = exceptionMetaData->GetException();
    ASSERT_NE(exception, nullptr);
    EXPECT_EQ(exception->GetType(), ObjectType::Object);
    EXPECT_EQ(exception->GetSubType(), ObjectSubType::Error);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"executionContextId"="10"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"executionContextId":"10"}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"executionContextId"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"executionContextId":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"executionContextId"=2]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"executionContextId":2}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);
    EXPECT_EQ(exceptionMetaData->GetExecutionContextId(), 2);
}

HWTEST_F_L0(DebuggerTypesTest, ExceptionDetailsToJsonTest)
{
    std::string msg;
    std::unique_ptr<ExceptionDetails> exceptionMetaData;
    std::string tmpStr;
    int32_t tmpInt;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":5,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":"100","url":"url0",
          "exception":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Error + R"("},"executionContextId":30}})";
    exceptionMetaData = ExceptionDetails::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(exceptionMetaData, nullptr);
    auto objJson = exceptionMetaData->ToJson();

    ret = objJson->GetInt("exceptionId", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 5);

    ret = objJson->GetString("text", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("text0", tmpStr);

    ret = objJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ret = objJson->GetInt("columnNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 20);

    ret = objJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("100", tmpStr);

    ret = objJson->GetString("url", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("url0", tmpStr);

    ret = objJson->GetObject("exception", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Error.c_str()), tmpStr);

    ret = objJson->GetInt("executionContextId", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 30);
}

HWTEST_F_L0(DebuggerTypesTest, InternalPropertyDescriptorCreateTest)
{
    std::string msg;
    std::unique_ptr<InternalPropertyDescriptor> internalPropertyDescriptor;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8","value":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":[99]}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name7"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name7"}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(internalPropertyDescriptor, nullptr);
    EXPECT_EQ("name7", internalPropertyDescriptor->GetName());

    // abnormal params of unknown params.sub-key=["name":"name8","value":{"type":"object","subtype":"map"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":"99"}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8","value":{"type":"object","subtype":"wrong"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":"wrong"}}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","value":{"type":"object","subtype":"map"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Map + R"("}}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(internalPropertyDescriptor, nullptr);
    EXPECT_EQ("name8", internalPropertyDescriptor->GetName());
    ASSERT_TRUE(internalPropertyDescriptor->HasValue());
    RemoteObject *value = internalPropertyDescriptor->GetValue();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->GetType(), ObjectType::Object);
    EXPECT_EQ(value->GetSubType(), ObjectSubType::Map);
}

HWTEST_F_L0(DebuggerTypesTest, InternalPropertyDescriptorToJsonTest)
{
    std::string msg;
    std::unique_ptr<InternalPropertyDescriptor> internalPropertyDescriptor;
    std::string tmpStr;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Map + R"("}}})";
    internalPropertyDescriptor = InternalPropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(internalPropertyDescriptor, nullptr);
    auto objJson = internalPropertyDescriptor->ToJson();

    ret = objJson->GetString("name", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("name8", tmpStr);

    ret = objJson->GetObject("value", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Map.c_str()), tmpStr);
}

HWTEST_F_L0(DebuggerTypesTest, PropertyDescriptorCreateTest)
{
    std::string msg;
    std::unique_ptr<PropertyDescriptor> propertyDescriptor;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":10,"configurable":true,"enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":10,"configurable":true,"enumerable":true,"value":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":["name85"],"configurable":true,"enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":["name85"],"configurable":true,"enumerable":true,"value":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":10,"enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":10,"enumerable":true}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":"true","enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":"true","enumerable":true}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":"true"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":"true"}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"value":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"value":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"value":{"ee":"11"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"value":{"ee":"11"}}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"value":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"value":{"type":")" +
          ObjectType::Symbol + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    RemoteObject *value = propertyDescriptor->GetValue();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->GetType(), ObjectType::Symbol);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"writable":98]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"writable":98}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"writable":[true]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"writable":[true]}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"writable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"writable":true}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    ASSERT_TRUE(propertyDescriptor->GetWritable());

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"get":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"get":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"get":[10]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"get":[10]}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"get":{}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"get":{"type":")" +
          ObjectType::Function + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    RemoteObject *get = propertyDescriptor->GetGet();
    ASSERT_NE(get, nullptr);
    EXPECT_EQ(get->GetType(), ObjectType::Function);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"set":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"set":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"set":[10]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"set":[10]}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"set":{}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"set":{"type":")" +
          ObjectType::String + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    RemoteObject *set = propertyDescriptor->GetSet();
    ASSERT_NE(set, nullptr);
    EXPECT_EQ(set->GetType(), ObjectType::String);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"wasThrown":98]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"wasThrown":98}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"wasThrown":[true]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"wasThrown":[true]}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"wasThrown":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"wasThrown":true}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    ASSERT_TRUE(propertyDescriptor->GetWasThrown());

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"isOwn":98]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"isOwn":98}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"isOwn":[true]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"isOwn":[true]}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"isOwn":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"isOwn":true}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    ASSERT_TRUE(propertyDescriptor->GetIsOwn());

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true, "symbol":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"symbol":10}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true, "symbol":[10]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"symbol":[10]}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true, "symbol":{}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"symbol":{"type":")" +
          ObjectType::Wasm + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    RemoteObject *symbol = propertyDescriptor->GetSymbol();
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->GetType(), ObjectType::Wasm);
}

HWTEST_F_L0(DebuggerTypesTest, PropertyDescriptorToJsonTest)
{
    std::string msg;
    std::unique_ptr<PropertyDescriptor> propertyDescriptor;
    std::string tmpStr;
    std::unique_ptr<PtJson> tmpJson;
    bool tmpBool;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Map + R"("},
          "writable":true,"get":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Regexp + R"("},"set":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Generator +
          R"("},"configurable":true,"enumerable":true,"wasThrown":true,"isOwn":true,"symbol":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Proxy + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(propertyDescriptor, nullptr);
    auto objJson = propertyDescriptor->ToJson();

    ret = objJson->GetString("name", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("name8", tmpStr);

    ret = objJson->GetObject("value", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Map.c_str()), tmpStr);

    ret = objJson->GetBool("writable", &tmpBool);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ret = objJson->GetObject("get", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Regexp.c_str()), tmpStr);

    ret = objJson->GetObject("set", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Generator.c_str()), tmpStr);

    ret = objJson->GetBool("configurable", &tmpBool);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ret = objJson->GetBool("enumerable", &tmpBool);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ret = objJson->GetBool("wasThrown", &tmpBool);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ret = objJson->GetBool("isOwn", &tmpBool);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ret = objJson->GetObject("symbol", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Proxy.c_str()), tmpStr);
}

HWTEST_F_L0(DebuggerTypesTest, LocationCreateTest)
{
    std::string msg;
    std::unique_ptr<Location> location;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":10,"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":10,"lineNumber":99
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":["id3"],"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":["id3"],"lineNumber":99
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":"99"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":"99"
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":[99]
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // normal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":138]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":138
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(location, nullptr);
    EXPECT_EQ(location->GetScriptId(), 222);
    EXPECT_EQ(location->GetLine(), 899);
    EXPECT_EQ(location->GetColumn(), 138);

    // normal params of params.sub-key=["scriptId":"2122","lineNumber":8299]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2122","lineNumber":8299
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(location, nullptr);
    EXPECT_EQ(location->GetScriptId(), 2122);
    EXPECT_EQ(location->GetLine(), 8299);
}

HWTEST_F_L0(DebuggerTypesTest, LocationToJsonTest)
{
    std::string msg;
    std::unique_ptr<Location> location;
    std::string tmpStr;
    int32_t tmpInt;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2","lineNumber":99,"columnNumber":18
    }})";
    location = Location::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(location, nullptr);
    auto objJson = location->ToJson();

    ret = objJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("2", tmpStr);

    ret = objJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 99);

    ret = objJson->GetInt("columnNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 18);
}

HWTEST_F_L0(DebuggerTypesTest, BreakLocationCreateTest)
{
    std::string msg;
    std::unique_ptr<BreakLocation> breakLocation;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":10,"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":10,"lineNumber":99
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":["id3"],"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":["id3"],"lineNumber":99
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":"99"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":"99"
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":[99]
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":"18"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":"18"
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":"18","type":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":"18","type":10
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":"18","type":"ee"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":"18","type":"ee"
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(breakLocation, nullptr);

    // normal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":138,"type":"return"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":138,"type":"return"
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(breakLocation, nullptr);
    EXPECT_EQ(breakLocation->GetScriptId(), 222);
    EXPECT_EQ(breakLocation->GetLine(), 899);
    EXPECT_EQ(breakLocation->GetColumn(), 138);
    EXPECT_EQ("return", breakLocation->GetType());

    // normal params of params.sub-key=["scriptId":"2122","lineNumber":8299]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2122","lineNumber":8299
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(breakLocation, nullptr);
    EXPECT_EQ(breakLocation->GetScriptId(), 2122);
    EXPECT_EQ(breakLocation->GetLine(), 8299);
}

HWTEST_F_L0(DebuggerTypesTest, BreakLocationToJsonTest)
{
    std::string msg;
    std::unique_ptr<BreakLocation> breakLocation;
    std::string tmpStr;
    int32_t tmpInt;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"12","lineNumber":919,"columnNumber":148,"type":"call"
    }})";
    breakLocation = BreakLocation::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(breakLocation, nullptr);
    auto objJson = breakLocation->ToJson();

    ret = objJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("12", tmpStr);

    ret = objJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 919);

    ret = objJson->GetInt("columnNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 148); 

    ret = objJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("call", tmpStr);
}

HWTEST_F_L0(DebuggerTypesTest, ScopeCreateTest)
{
    std::string msg;
    std::unique_ptr<Scope> scope;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"ss","object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"ss","object":{"type":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":12,"object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":12,"object":{"type":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":10}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"ww":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"name":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"name":10}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"name":["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"name":["10"]}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // normal params of params.sub-key=["type":"global","object":{..},"name":"name128"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"name":"name117"}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(scope, nullptr);
    EXPECT_EQ("name117", scope->GetName());

    // abnormal params of params.sub-key=["type":"global","object":{..},"startLocation":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"startLocation":10}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"startLocation":{"12":"34"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"startLocation":{"12":"34"}}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"endLocation":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"endLocation":10}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"endLocation":{"12":"34"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"endLocation":{"12":"34"}}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scope, nullptr);

    // normal params of params.sub-key=["type":"global","object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(scope, nullptr);
    EXPECT_EQ("global", scope->GetType());
    RemoteObject *object = scope->GetObject();
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->GetType(), ObjectType::Bigint);
}

HWTEST_F_L0(DebuggerTypesTest, ScopeToJsonTest)
{
    std::string msg;
    std::unique_ptr<Scope> scope;
    std::string tmpStr;
    int32_t tmpInt;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Dataview + R"("},"name":"name9",
          "startLocation":{"scriptId":"2","lineNumber":99},
          "endLocation":{"scriptId":"13","lineNumber":146}
    }})";
    scope = Scope::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(scope, nullptr);
    auto objJson = scope->ToJson();

    ret = objJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("global", tmpStr);   

    ret = objJson->GetObject("object", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Dataview.c_str()), tmpStr);

    ret = objJson->GetString("name", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("name9", tmpStr);

    ret = objJson->GetObject("startLocation", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("2", tmpStr);
    ret = tmpJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 99);

    ret = objJson->GetObject("endLocation", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("13", tmpStr);
    ret = tmpJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 146);
}

HWTEST_F_L0(DebuggerTypesTest, CallFrameCreateTest)
{
    std::string msg;
    std::unique_ptr<CallFrame> callFrame;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":10,"functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":["0"],"functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":10,
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":["name0"],
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0","functionLocation":10,
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0","functionLocation":{"scriptId11":"id5","lineNumber":19},
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId2":"id5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":10,"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":10,"scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":{"url":"url7"},"scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0", "location":{"scriptId":"5","lineNumber":19},
          "url":"url7","scopeChain":10,"this":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          {"type":"22","object":{"type":")" +
          ObjectType::Object + R"("}},"this":{"type":")" + ObjectType::Object + R"(","subtype":")" +
          ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":10}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"11":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("},
          "returnValue":10}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("},
          "returnValue":{"type":"object","subtype":"11"}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(callFrame, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(callFrame, nullptr);
    EXPECT_EQ(callFrame->GetCallFrameId(), 0);
    EXPECT_EQ("name0", callFrame->GetFunctionName());
    ASSERT_FALSE(callFrame->HasFunctionLocation());
    Location *location = callFrame->GetLocation();
    EXPECT_EQ(location->GetScriptId(), 5);
    EXPECT_EQ(location->GetLine(), 19);
    EXPECT_EQ("url7", callFrame->GetUrl());
    const std::vector<std::unique_ptr<Scope>> *scopeChain = callFrame->GetScopeChain();
    EXPECT_EQ(scopeChain->size(), 2U);
    RemoteObject *thisObj = callFrame->GetThis();
    ASSERT_NE(thisObj, nullptr);
    EXPECT_EQ(thisObj->GetType(), ObjectType::Object);
    EXPECT_EQ(thisObj->GetSubType(), ObjectSubType::V128);
    ASSERT_FALSE(callFrame->HasReturnValue());

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"10","functionName":"name0","functionLocation":{"scriptId":"3","lineNumber":16},
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 +
          R"("},"returnValue":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::I32 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(callFrame, nullptr);
    EXPECT_EQ(callFrame->GetCallFrameId(), 10);
    EXPECT_EQ("name0", callFrame->GetFunctionName());
    Location *functionLocation = callFrame->GetFunctionLocation();
    EXPECT_EQ(functionLocation->GetScriptId(), 3);
    EXPECT_EQ(functionLocation->GetLine(), 16);
    location = callFrame->GetLocation();
    EXPECT_EQ(location->GetScriptId(), 5);
    EXPECT_EQ(location->GetLine(), 19);
    EXPECT_EQ("url7", callFrame->GetUrl());
    scopeChain = callFrame->GetScopeChain();
    EXPECT_EQ(scopeChain->size(), 2U);
    thisObj = callFrame->GetThis();
    ASSERT_NE(thisObj, nullptr);
    EXPECT_EQ(thisObj->GetType(), ObjectType::Object);
    EXPECT_EQ(thisObj->GetSubType(), ObjectSubType::V128);
    RemoteObject *returnObj = callFrame->GetReturnValue();
    ASSERT_NE(returnObj, nullptr);
    EXPECT_EQ(returnObj->GetType(), ObjectType::Object);
    EXPECT_EQ(returnObj->GetSubType(), ObjectSubType::I32);
}

HWTEST_F_L0(DebuggerTypesTest, CallFrameToJsonTest)
{
    std::string msg;
    std::unique_ptr<CallFrame> callFrame;
    std::string tmpStr;
    int32_t tmpInt;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0","functionLocation":{"scriptId":"3","lineNumber":16},
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::Iterator +
          R"("},"returnValue":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::I64 + R"("}}})";
    callFrame = CallFrame::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(callFrame, nullptr);
    auto objJson = callFrame->ToJson();

    ret = objJson->GetString("callFrameId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("0", tmpStr);

    ret = objJson->GetString("functionName", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("name0", tmpStr);

    ret = objJson->GetObject("functionLocation", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("3", tmpStr);
    ret = tmpJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 16);

    ret = objJson->GetObject("location", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("5", tmpStr);
    ret = tmpJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 19);

    ret = objJson->GetString("url", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ("url7", tmpStr);

    ret = objJson->GetArray("scopeChain", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    EXPECT_EQ(tmpJson->GetSize(), 2);

    ret = objJson->GetObject("this", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::Iterator.c_str()), tmpStr);
    
    ret = objJson->GetObject("returnValue", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("type", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), tmpStr);
    ret = tmpJson->GetString("subtype", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(std::string(ObjectSubType::I64.c_str()), tmpStr);
}

#ifdef SUPPORT_PROFILER_CDP
HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileSampleCreateTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileSample> object;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

   // abnormal params of params.sub-key = [ "size"="Test","nodeId"="Test","ordinal"="Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "size":"Test","nodeId":"Test","ordinal":"Test"}})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of params.sub-key = [ "size"={"xx":"yy"},"nodeId"={"xx":"yy"},"ordinal"={"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "size":{"xx":"yy"},"nodeId":{"xx":"yy"},"ordinal":{"xx":"yy"}}})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of params.sub-key = [ "size"=100,"nodeId"=1,"ordinal"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"size":100,"nodeId":1,"ordinal":10}})";
    object = SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->GetSize(), 100);
    EXPECT_EQ(object->GetNodeId(), 1);
    EXPECT_EQ(object->GetOrdinal(), 10);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileSampleToJsonTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileSample> samplingHeapProfileSampleData;
    std::string tmpStr;
    int32_t tmpInt;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"size":100,"nodeId":1,"ordinal":10}})";
    samplingHeapProfileSampleData =
        SamplingHeapProfileSample::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(samplingHeapProfileSampleData, nullptr);
    auto json = samplingHeapProfileSampleData->ToJson();

    ret = json->GetInt("size", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 100);
    ret = json->GetInt("nodeId", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 1);
    ret = json->GetInt("ordinal", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileNodeCreateTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileNode> object;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    object = SamplingHeapProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    object = SamplingHeapProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    object = SamplingHeapProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    object = SamplingHeapProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
        "selfSize":10,
        "id":5,
        "children":[]
    }})";
    object = SamplingHeapProfileNode::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(object, nullptr);
    RuntimeCallFrame *runTimeCallFrame = object->GetCallFrame();
    ASSERT_NE(runTimeCallFrame, nullptr);
    EXPECT_EQ(runTimeCallFrame->GetFunctionName(), "Create");
    EXPECT_EQ(runTimeCallFrame->GetScriptId(), "10");
    EXPECT_EQ(runTimeCallFrame->GetUrl(), "url3");
    EXPECT_EQ(runTimeCallFrame->GetLineNumber(), 100);
    EXPECT_EQ(runTimeCallFrame->GetColumnNumber(), 20);

    EXPECT_EQ(object->GetSelfSize(), 10);
    EXPECT_EQ(object->GetId(), 5);
    const std::vector<std::unique_ptr<SamplingHeapProfileNode>> *children = object->GetChildren();
    ASSERT_NE(children, nullptr);
    EXPECT_EQ((int)children->size(), 0);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileNodeToJsonTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileNode> samplingHeapProfileNode;
    std::string tmpStr;
    std::unique_ptr<PtJson> tmpJson;
    int32_t tmpInt;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
        "selfSize":10,
        "id":5,
        "children":[]
    }})";
    samplingHeapProfileNode = SamplingHeapProfileNode::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(samplingHeapProfileNode, nullptr);
    auto json = samplingHeapProfileNode->ToJson();

    ret = json->GetObject("callFrame", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("functionName", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "Create");
    ret = tmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "10");
    ret = tmpJson->GetString("url", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "url3");

    ret = json->GetInt("selfSize", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);
    ret = json->GetInt("id", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 5);
    ret = json->GetArray("children", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    EXPECT_EQ(tmpJson->GetSize(), 0);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileCreateTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfile> object;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    object = SamplingHeapProfile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    object = SamplingHeapProfile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    object = SamplingHeapProfile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    object = SamplingHeapProfile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(object, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "head": {
            "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
            "selfSize":10,
            "id":5,
            "children":[]
        },
        "samples":[{"size":100, "nodeId":1, "ordinal":10}]
    }})";
    object = SamplingHeapProfile::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(object, nullptr);
    SamplingHeapProfileNode *head = object->GetHead();
    ASSERT_NE(head, nullptr);
    
    RuntimeCallFrame *runTimeCallFrame = head->GetCallFrame();
    ASSERT_NE(runTimeCallFrame, nullptr);
    EXPECT_EQ(runTimeCallFrame->GetFunctionName(), "Create");
    EXPECT_EQ(runTimeCallFrame->GetScriptId(), "10");
    EXPECT_EQ(runTimeCallFrame->GetUrl(), "url3");
    EXPECT_EQ(runTimeCallFrame->GetLineNumber(), 100);
    EXPECT_EQ(runTimeCallFrame->GetColumnNumber(), 20);

    EXPECT_EQ(head->GetSelfSize(), 10);
    EXPECT_EQ(head->GetId(), 5);
    const std::vector<std::unique_ptr<SamplingHeapProfileNode>> *children = head->GetChildren();
    ASSERT_NE(children, nullptr);
    EXPECT_EQ((int)children->size(), 0);

    const std::vector<std::unique_ptr<SamplingHeapProfileSample>> *samples = object->GetSamples();
    ASSERT_NE(samples, nullptr);
    EXPECT_EQ((int)samples->size(), 1);
    EXPECT_EQ(samples->data()->get()->GetSize(), 100);
    EXPECT_EQ(samples->data()->get()->GetNodeId(), 1);
    EXPECT_EQ(samples->data()->get()->GetOrdinal(), 10);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileToJsonTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfile> samplingHeapProfile;
    std::string tmpStr;
    int32_t tmpInt;
    std::unique_ptr<PtJson> tmpJson;
    std::unique_ptr<PtJson> varTmpJson;
    std::unique_ptr<PtJson> exTmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "head": {
            "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
            "selfSize":10,
            "id":5,
            "children":[]
        },
        "samples":[{"size":100, "nodeId":1, "ordinal":10}]
    }})";

    samplingHeapProfile = SamplingHeapProfile::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(samplingHeapProfile, nullptr);
    auto json = samplingHeapProfile->ToJson();
    
    ret = json->GetObject("head", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetObject("callFrame", &varTmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(varTmpJson, nullptr);
    ret = varTmpJson->GetString("functionName", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "Create");
    ret = varTmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "10");
    ret = varTmpJson->GetString("url", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "url3");

    ret = tmpJson->GetInt("selfSize", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);
    ret = tmpJson->GetInt("id", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 5);
    ret = tmpJson->GetArray("children", &exTmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(exTmpJson, nullptr);
    EXPECT_EQ(exTmpJson->GetSize(), 0);

    ret = json->GetArray("samples", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    EXPECT_EQ(tmpJson->GetSize(), 1);
}

HWTEST_F_L0(DebuggerTypesTest, PositionTickInfoCreateTest)
{
    std::string msg;
    std::unique_ptr<PositionTickInfo> positionTickInfo;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of params.sub-key=["line":11,"ticks":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "line":"11","ticks":99}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of params.sub-key=["line":"11","ticks":"99"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "line":"11","ticks":"99"}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of params.sub-key=["line":[11],"ticks":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "line":[11],"ticks":[99]}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(positionTickInfo, nullptr);

    // normal params of params.sub-key=["line":11,"ticks":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"line":1,"ticks":0}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(positionTickInfo, nullptr);
    EXPECT_EQ(positionTickInfo->GetLine(), 1);
    EXPECT_EQ(positionTickInfo->GetTicks(), 0);
}

HWTEST_F_L0(DebuggerTypesTest, PositionTickInfoToJsonTest)
{
    std::string msg;
    std::unique_ptr<PositionTickInfo> positionTickInfo;
    int32_t tmpInt;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"line":1,"ticks":0}})";
    positionTickInfo = PositionTickInfo::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(positionTickInfo, nullptr);
    auto json = positionTickInfo->ToJson();

    ret = json->GetInt("line", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 1);

    ret = json->GetInt("ticks", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 0);
}

HWTEST_F_L0(DebuggerTypesTest, ProfileNodeCreateTest)
{
    std::string msg;
    std::unique_ptr<ProfileNode> profileNode;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    profileNode = ProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profileNode, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    profileNode = ProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profileNode, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    profileNode = ProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profileNode, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    profileNode = ProfileNode::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profileNode, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "id":10,
          "callFrame": {"functionName":"name0", "scriptId":"12", "url":"url15", "lineNumber":11, "columnNumber":20},
          "hitCount":15,"children":[],"positionTicks":[],"deoptReason":"yyy"}})";
    profileNode = ProfileNode::Create(DispatchRequest(msg).GetParams());

    ASSERT_NE(profileNode, nullptr);
    EXPECT_EQ(profileNode->GetId(), 10);
    RuntimeCallFrame *runTimeCallFrame = profileNode->GetCallFrame();
    ASSERT_NE(runTimeCallFrame, nullptr);
    EXPECT_EQ(runTimeCallFrame->GetFunctionName(), "name0");
    EXPECT_EQ(runTimeCallFrame->GetScriptId(), "12");
    EXPECT_EQ(runTimeCallFrame->GetUrl(), "url15");
    EXPECT_EQ(runTimeCallFrame->GetLineNumber(), 11);
    EXPECT_EQ(runTimeCallFrame->GetColumnNumber(), 20);

    EXPECT_EQ(profileNode->GetHitCount(), 15);
    EXPECT_EQ(profileNode->GetDeoptReason(), "yyy");
}

HWTEST_F_L0(DebuggerTypesTest, ProfileNodeToJsonTest)
{
    std::string msg;
    std::unique_ptr<ProfileNode> profilenode;
    std::string tmpStr;
    int32_t tmpInt;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "id":10,
          "callFrame": {"functionName":"name0", "scriptId":"12", "url":"url15", "lineNumber":11, "columnNumber":20},
          "hitCount":15,"children":[],"positionTicks":[],"deoptReason":"yyy"}})";
    profilenode = ProfileNode::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(profilenode, nullptr);
    auto json = profilenode->ToJson();

    ret = json->GetInt("id", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ret = json->GetObject("callFrame", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    ret = tmpJson->GetString("functionName", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "name0");
    ret = tmpJson->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "12");
    ret = tmpJson->GetString("url", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "url15");
    ret = tmpJson->GetInt("lineNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 11);
    ret = tmpJson->GetInt("columnNumber", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 20);

    ret = json->GetInt("hitCount", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 15);

    ret = json->GetString("deoptReason", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "yyy");
}

HWTEST_F_L0(DebuggerTypesTest, ProfileCreateTest)
{
    std::string msg;
    std::unique_ptr<Profile> profile;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    profile = Profile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    profile = Profile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    profile = Profile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    profile = Profile::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startTime":10, "endTime":25, "nodes":[{"id":12,
          "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20}}],
          "samples":[],"timeDeltas":[]}})";
    profile = Profile::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(profile, nullptr);

    EXPECT_EQ(profile->GetStartTime(), 10LL);
    EXPECT_EQ(profile->GetEndTime(), 25LL);
    const std::vector<std::unique_ptr<ProfileNode>> *profileNode = profile->GetNodes();
    ASSERT_NE(profileNode, nullptr);
    EXPECT_EQ((int)profileNode->size(), 1);
}

HWTEST_F_L0(DebuggerTypesTest, ProfileToJsonTest)
{
    std::string msg;
    std::unique_ptr<Profile> profile;
    std::string tmpStr;
    int32_t tmpInt;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startTime":10, "endTime":25, "nodes":[{"id":12,
          "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20}}],
          "samples":[],"timeDeltas":[]}})";
    profile = Profile::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(profile, nullptr);
    auto json = profile->ToJson();

    ret = json->GetInt("startTime", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ret = json->GetInt("endTime", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 25);
}

HWTEST_F_L0(DebuggerTypesTest, CoverageCreateTest)
{
    std::string msg;
    std::unique_ptr<Coverage> coverage;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    coverage = Coverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(coverage, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    coverage = Coverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(coverage, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    coverage = Coverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(coverage, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    coverage = Coverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(coverage, nullptr);

    // normal params of params.sub-key=["startOffset":0,"endOffset":5,"count":13]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startOffset":0,"endOffset":13,"count":13}})";
    coverage = Coverage::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(coverage, nullptr);
    EXPECT_EQ(coverage->GetStartOffset(), 0);
    EXPECT_EQ(coverage->GetEndOffset(), 13);
    EXPECT_EQ(coverage->GetCount(), 13);
}

HWTEST_F_L0(DebuggerTypesTest, CoverageToJsonTest)
{
    std::string msg;
    std::unique_ptr<Coverage> coverage;
    std::string tmpStr;
    int32_t tmpInt;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startOffset":0,"endOffset":13,"count":13}})";
    coverage = Coverage::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(coverage, nullptr);
    auto json = coverage->ToJson();

    ret = json->GetInt("startOffset", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 0);

    ret = json->GetInt("endOffset", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 13);

    ret = json->GetInt("count", &tmpInt);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpInt, 13);
}

HWTEST_F_L0(DebuggerTypesTest, FunctionCoverageCreateTest)
{
    std::string msg;
    std::unique_ptr<FunctionCoverage> functionCoverage;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    functionCoverage = FunctionCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(functionCoverage, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    functionCoverage = FunctionCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(functionCoverage, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    functionCoverage = FunctionCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(functionCoverage, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    functionCoverage = FunctionCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(functionCoverage, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "functionName":"Create0","ranges":[{"startOffset":0,"endOffset":13,"count":13}],"isBlockCoverage":true}})";
    functionCoverage = FunctionCoverage::Create(DispatchRequest(msg).GetParams());

    ASSERT_NE(functionCoverage, nullptr);
    EXPECT_EQ(functionCoverage->GetFunctionName(), "Create0");
    const std::vector<std::unique_ptr<Coverage>> *ranges = functionCoverage->GetRanges();
    ASSERT_NE(ranges, nullptr);
    EXPECT_EQ((int)ranges->size(), 1);
    ASSERT_TRUE(functionCoverage->GetIsBlockCoverage());
}

HWTEST_F_L0(DebuggerTypesTest, FunctionCoverageToJsonTest)
{
    std::string msg;
    std::unique_ptr<FunctionCoverage> functionCoverage;
    std::string tmpStr;
    bool tmpBool;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "functionName":"Create0","ranges":[{"startOffset":0,"endOffset":13,"count":13}],"isBlockCoverage":true}})";
    functionCoverage = FunctionCoverage::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(functionCoverage, nullptr);
    auto json = functionCoverage->ToJson();

    ret = json->GetString("functionName", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "Create0");

    ret = json->GetArray("ranges", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    EXPECT_EQ(tmpJson->GetSize(), 1);

    ret = json->GetBool("isBlockCoverage", &tmpBool);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_TRUE(tmpBool);
}

HWTEST_F_L0(DebuggerTypesTest, ScriptCoverageCreateTest)
{
    std::string msg;
    std::unique_ptr<ScriptCoverage> scriptCoverage;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    scriptCoverage = ScriptCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scriptCoverage, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    scriptCoverage = ScriptCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scriptCoverage, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    scriptCoverage = ScriptCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scriptCoverage, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    scriptCoverage = ScriptCoverage::Create(DispatchRequest(msg).GetParams());
    EXPECT_EQ(scriptCoverage, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"1001",
          "url":"url17",
          "functions":[{"functionName":"Create0",
          "ranges":[{"startOffset":0, "endOffset":13, "count":13}],
          "isBlockCoverage":true}]}})";
    scriptCoverage = ScriptCoverage::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(scriptCoverage, nullptr);
    EXPECT_EQ(scriptCoverage->GetScriptId(), "1001");
    EXPECT_EQ(scriptCoverage->GetUrl(), "url17");
    const std::vector<std::unique_ptr<FunctionCoverage>> *functions = scriptCoverage->GetFunctions();
    ASSERT_NE(functions, nullptr);
    EXPECT_EQ((int)functions->size(), 1);
}

HWTEST_F_L0(DebuggerTypesTest, ScriptCoverageToJsonTest)
{
    std::string msg;
    std::unique_ptr<ScriptCoverage> scriptCoverage;
    std::string tmpStr;
    std::unique_ptr<PtJson> tmpJson;
    Result ret;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"1001",
          "url":"url17",
          "functions": [{"functionName":"Create0",
          "ranges": [{"startOffset":0, "endOffset":13, "count":13}],
          "isBlockCoverage":true}]}})";
    scriptCoverage = ScriptCoverage::Create(DispatchRequest(msg).GetParams());
    ASSERT_NE(scriptCoverage, nullptr);
    auto json = scriptCoverage->ToJson();

    ret = json->GetString("scriptId", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "1001");

    ret = json->GetString("url", &tmpStr);
    EXPECT_EQ(ret, Result::SUCCESS);
    EXPECT_EQ(tmpStr, "url17");

    ret = json->GetArray("functions", &tmpJson);
    EXPECT_EQ(ret, Result::SUCCESS);
    ASSERT_NE(tmpJson, nullptr);
    EXPECT_EQ(tmpJson->GetSize(), 1);
}
#endif
}  // namespace panda::test
