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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
        // Main logic is JSON parser, so not need trigger GC to decrease execute time
        ecmaVm->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(DebuggerTypesTest, RemoteObjectCreateTest)
{
    std::string msg;
    std::unique_ptr<RemoteObject> remoteObject;
    Local<StringRef> tmpStr;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = 100, ]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = [ "sub": "test" ] }, ]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", ]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object + R"("}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());

    // abnormal params of params.sub-key = [ type = "object", subtype = "unknown"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","subtype":"unknown"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", subtype = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","subtype":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", subtype = "array"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","subtype":")" + ObjectSubType::Array + R"("}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasSubType());
    EXPECT_EQ(ObjectSubType::Array, remoteObject->GetSubType());

    // abnormal params of params.sub-key = [ type = "object", className = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","className":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", className = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","className":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", className = "TestClass"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","className":"TestClass"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasClassName());
    EXPECT_EQ("TestClass", remoteObject->GetClassName());

    // normal params of params.sub-key = [ type = "object", value = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","value":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasValue());
    EXPECT_EQ(Local<NumberRef>(remoteObject->GetValue())->Value(), 100.0);

    // normal params of params.sub-key = [ type = "object", value = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","value":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasValue());
    ASSERT_TRUE(remoteObject->GetValue()->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "xx");
    ASSERT_TRUE(Local<ObjectRef>(remoteObject->GetValue())->Has(ecmaVm, Local<JSValueRef>(tmpStr)));

    // normal params of params.sub-key = [ type = "object", value = "Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","value":"Test"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasValue());
    EXPECT_EQ("Test", DebuggerApi::ToStdString(remoteObject->GetValue()));

    // abnormal params of params.sub-key = [ type = "object", unserializableValue = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","unserializableValue":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", unserializableValue = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","unserializableValue":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", unserializableValue = "TestClass"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","unserializableValue":"Test"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasUnserializableValue());
    EXPECT_EQ("Test", remoteObject->GetUnserializableValue());

    // abnormal params of params.sub-key = [ type = "object", description = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","description":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", description = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","description":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", description = "Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","description":"Test"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasDescription());
    EXPECT_EQ("Test", remoteObject->GetDescription());

    // abnormal params of params.sub-key = [ type = "object", objectId = 100]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","objectId":100}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // abnormal params of params.sub-key = [ type = "object", objectId = {"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","objectId":{"xx":"yy"}}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(remoteObject, nullptr);

    // normal params of params.sub-key = [ type = "object", objectId = "id_1"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
          R"(","objectId":"1"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    EXPECT_EQ(ObjectType::Object, remoteObject->GetType());
    ASSERT_TRUE(remoteObject->HasObjectId());
    EXPECT_EQ(remoteObject->GetObjectId(), 1);
}

HWTEST_F_L0(DebuggerTypesTest, RemoteObjectToObjectTest)
{
    std::string msg;
    std::unique_ptr<RemoteObject> remoteObject;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"type":")" + ObjectType::Object +
        R"(", "subtype":")" + ObjectSubType::Array +
        R"(","className":"TestClass","value":100, "unserializableValue":"Test","description":"Test","objectId":"1"}})";
    remoteObject = RemoteObject::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(remoteObject, nullptr);
    Local<ObjectRef> object = remoteObject->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Array.c_str()), DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "className");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("TestClass", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "value");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<NumberRef>(result)->Value(), 100.0);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "unserializableValue");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("Test", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "description");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("Test", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "objectId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("1", DebuggerApi::ToStdString(result));
}

HWTEST_F_L0(DebuggerTypesTest, ExceptionDetailsCreateTest)
{
    std::string msg;
    std::unique_ptr<ExceptionDetails> exceptionMetaData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId="Test","text"="text0","lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":"Test","text":"text0","lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId={"xx":"yy"},"text"="text0","lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":{"xx":"yy"},"text":"text0","lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"=10,"lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":10,"lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"=["text0"],"lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":["text0"],"lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"="10","columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":"10","columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=["10"],"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":["10"],"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"="20"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":"20"}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=["20"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":["20"]}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key = [ exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"scriptId"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":10}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"scriptId"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"scriptId"="id0"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":"0"}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"url"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"url":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"url"="url0"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"url":"url0"}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"exception"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"exception":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"exception"={}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"exception":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Error + R"("}}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // abnormal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"executionContextId"=["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"executionContextId":["10"]}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(exceptionMetaData, nullptr);

    // normal params of params.sub-key =
    // [exceptionId=3,"text"="text0","lineNumber"=10,"columnNumber"=20,"executionContextId"=2]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":3,"text":"text0","lineNumber":10,"columnNumber":20,"executionContextId":2}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(exceptionMetaData, nullptr);
    EXPECT_EQ(exceptionMetaData->GetExceptionId(), 3);
    EXPECT_EQ("text0", exceptionMetaData->GetText());
    EXPECT_EQ(exceptionMetaData->GetLine(), 10);
    EXPECT_EQ(exceptionMetaData->GetColumn(), 20);
    EXPECT_EQ(exceptionMetaData->GetExecutionContextId(), 2);
}

HWTEST_F_L0(DebuggerTypesTest, ExceptionDetailsToObjectTest)
{
    std::string msg;
    std::unique_ptr<ExceptionDetails> exceptionMetaData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "exceptionId":5,"text":"text0","lineNumber":10,"columnNumber":20,"scriptId":"100","url":"url0",
          "exception":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Error + R"("},"executionContextId":30}})";
    exceptionMetaData = ExceptionDetails::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(exceptionMetaData, nullptr);
    Local<ObjectRef> object = exceptionMetaData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "exceptionId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 5);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "text");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("text0", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "columnNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 20);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("100", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("url0", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "exception");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    Local<JSValueRef> subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Error.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "executionContextId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 30);
}

HWTEST_F_L0(DebuggerTypesTest, InternalPropertyDescriptorCreateTest)
{
    std::string msg;
    std::unique_ptr<InternalPropertyDescriptor> internalPropertyDescriptor;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8","value":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":99}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":[99]}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name7"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name7"}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(internalPropertyDescriptor, nullptr);
    EXPECT_EQ("name7", internalPropertyDescriptor->GetName());

    // abnormal params of unknown params.sub-key=["name":"name8","value":{"type":"object","subtype":"map"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":"99"}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key=["name":"name8","value":{"type":"object","subtype":"wrong"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":"wrong"}}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(internalPropertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","value":{"type":"object","subtype":"map"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Map + R"("}}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(internalPropertyDescriptor, nullptr);
    EXPECT_EQ("name8", internalPropertyDescriptor->GetName());
    ASSERT_TRUE(internalPropertyDescriptor->HasValue());
    RemoteObject *value = internalPropertyDescriptor->GetValue();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->GetType(), ObjectType::Object);
    EXPECT_EQ(value->GetSubType(), ObjectSubType::Map);
}

HWTEST_F_L0(DebuggerTypesTest, InternalPropertyDescriptorToObjectTest)
{
    std::string msg;
    std::unique_ptr<InternalPropertyDescriptor> internalPropertyDescriptor;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Map + R"("}}})";
    internalPropertyDescriptor =
        InternalPropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(internalPropertyDescriptor, nullptr);
    Local<ObjectRef> object = internalPropertyDescriptor->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "name");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("name8", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "value");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    Local<JSValueRef> subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Map.c_str()), DebuggerApi::ToStdString(subResult));
}

HWTEST_F_L0(DebuggerTypesTest, PropertyDescriptorCreateTest)
{
    std::string msg;
    std::unique_ptr<PropertyDescriptor> propertyDescriptor;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":10,"configurable":true,"enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":10,"configurable":true,"enumerable":true,"value":10}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":["name85"],"configurable":true,"enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":["name85"],"configurable":true,"enumerable":true,"value":10}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":10,"enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":10,"enumerable":true}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":"true","enumerable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":"true","enumerable":true}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":10}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":"true"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":"true"}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"value":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"value":10}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"value":{"ee":"11"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"value":{"ee":"11"}}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"value":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"value":{"type":")" +
          ObjectType::Symbol + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"writable":[true]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"writable":[true]}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"writable":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"writable":true}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    ASSERT_TRUE(propertyDescriptor->GetWritable());

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"get":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"get":10}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"get":[10]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"get":[10]}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"get":{}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"get":{"type":")" +
          ObjectType::Function + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"set":[10]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"set":[10]}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"set":{}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"set":{"type":")" +
          ObjectType::String + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"wasThrown":[true]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"wasThrown":[true]}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"wasThrown":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"wasThrown":true}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    ASSERT_TRUE(propertyDescriptor->GetWasThrown());

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"isOwn":98]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"isOwn":98}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"isOwn":[true]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"isOwn":[true]}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true,"isOwn":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"isOwn":true}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    ASSERT_TRUE(propertyDescriptor->GetIsOwn());

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true, "symbol":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"symbol":10}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // abnormal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true, "symbol":[10]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"symbol":[10]}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(propertyDescriptor, nullptr);

    // normal params of params.sub-key=["name":"name8","configurable":true,"enumerable":true, "symbol":{}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name85","configurable":true,"enumerable":true,"symbol":{"type":")" +
          ObjectType::Wasm + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(propertyDescriptor, nullptr);
    EXPECT_EQ("name85", propertyDescriptor->GetName());
    ASSERT_TRUE(propertyDescriptor->GetConfigurable());
    ASSERT_TRUE(propertyDescriptor->GetEnumerable());
    RemoteObject *symbol = propertyDescriptor->GetSymbol();
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->GetType(), ObjectType::Wasm);
}

HWTEST_F_L0(DebuggerTypesTest, PropertyDescriptorToObjectTest)
{
    std::string msg;
    std::unique_ptr<PropertyDescriptor> propertyDescriptor;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "name":"name8","value":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Map + R"("},
          "writable":true,"get":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Regexp + R"("},"set":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Generator +
          R"("},"configurable":true,"enumerable":true,"wasThrown":true,"isOwn":true,"symbol":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Proxy + R"("}}})";
    propertyDescriptor = PropertyDescriptor::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(propertyDescriptor, nullptr);
    Local<ObjectRef> object = propertyDescriptor->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "name");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("name8", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "value");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    Local<JSValueRef> subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Map.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "writable");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "get");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Regexp.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "set");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Generator.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "configurable");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "enumerable");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "wasThrown");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "isOwn");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "symbol");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Proxy.c_str()), DebuggerApi::ToStdString(subResult));
}

HWTEST_F_L0(DebuggerTypesTest, LocationCreateTest)
{
    std::string msg;
    std::unique_ptr<Location> location;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":10,"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":10,"lineNumber":99
    }})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":["id3"],"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":["id3"],"lineNumber":99
    }})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":"99"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":"99"
    }})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":[99]
    }})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(location, nullptr);

    // normal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":138]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":138
    }})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(location, nullptr);
    EXPECT_EQ(location->GetScriptId(), 222);
    EXPECT_EQ(location->GetLine(), 899);
    EXPECT_EQ(location->GetColumn(), 138);

    // normal params of params.sub-key=["scriptId":"2122","lineNumber":8299]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2122","lineNumber":8299
    }})";
    location = Location::Create(DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(location, nullptr);
    EXPECT_EQ(location->GetScriptId(), 2122);
    EXPECT_EQ(location->GetLine(), 8299);
}

HWTEST_F_L0(DebuggerTypesTest, LocationToObjectTest)
{
    std::string msg;
    std::unique_ptr<Location> location;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2","lineNumber":99,"columnNumber":18
    }})";
    location = Location::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(location, nullptr);
    Local<ObjectRef> object = location->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("2", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 99);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "columnNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 18);
}

HWTEST_F_L0(DebuggerTypesTest, LocationToJsonTest)
{
    std::string msg;
    std::unique_ptr<Location> location;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2","lineNumber":99,"columnNumber":18
    }})";
    location = Location::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(location, nullptr);
    Local<ObjectRef> object = location->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("2", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 99);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "columnNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 18);
}

HWTEST_F_L0(DebuggerTypesTest, BreakLocationCreateTest)
{
    std::string msg;
    std::unique_ptr<BreakLocation> breakLocation;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":10,"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":10,"lineNumber":99
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":["id3"],"lineNumber":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":["id3"],"lineNumber":99
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":"99"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":"99"
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"222","lineNumber":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":[99]
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":"18"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":"18"
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":"18","type":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":"18","type":10
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // abnormal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":"18","type":"ee"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":"18","type":"ee"
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(breakLocation, nullptr);

    // normal params of params.sub-key=["scriptId":"2","lineNumber":99,"columnNumber":138,"type":"return"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"222","lineNumber":899,"columnNumber":138,"type":"return"
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(breakLocation, nullptr);
    EXPECT_EQ(breakLocation->GetScriptId(), 222);
    EXPECT_EQ(breakLocation->GetLine(), 899);
    EXPECT_EQ(breakLocation->GetColumn(), 138);
    EXPECT_EQ("return", breakLocation->GetType());

    // normal params of params.sub-key=["scriptId":"2122","lineNumber":8299]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"2122","lineNumber":8299
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(breakLocation, nullptr);
    EXPECT_EQ(breakLocation->GetScriptId(), 2122);
    EXPECT_EQ(breakLocation->GetLine(), 8299);
}

HWTEST_F_L0(DebuggerTypesTest, BreakLocationToObjectTest)
{
    std::string msg;
    std::unique_ptr<BreakLocation> breakLocation;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"12","lineNumber":919,"columnNumber":148,"type":"call"
    }})";
    breakLocation = BreakLocation::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(breakLocation, nullptr);
    Local<ObjectRef> object = breakLocation->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("12", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 919);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "columnNumber");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 148);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("call", DebuggerApi::ToStdString(result));
}

HWTEST_F_L0(DebuggerTypesTest, ScopeCreateTest)
{
    std::string msg;
    std::unique_ptr<Scope> scope;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"ss","object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"ss","object":{"type":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":12,"object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":12,"object":{"type":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":10}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"ww":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"name":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"name":10}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"name":["10"]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"name":["10"]}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // normal params of params.sub-key=["type":"global","object":{..},"name":"name128"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"name":"name117"}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(scope, nullptr);
    EXPECT_EQ("name117", scope->GetName());

    // abnormal params of params.sub-key=["type":"global","object":{..},"startLocation":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"startLocation":10}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"startLocation":{"12":"34"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"startLocation":{"12":"34"}}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"endLocation":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"endLocation":10}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // abnormal params of params.sub-key=["type":"global","object":{..},"endLocation":{"12":"34"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("},"endLocation":{"12":"34"}}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scope, nullptr);

    // normal params of params.sub-key=["type":"global","object":{..}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Bigint + R"("}}})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(scope, nullptr);
    EXPECT_EQ("global", scope->GetType());
    RemoteObject *object = scope->GetObject();
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->GetType(), ObjectType::Bigint);
}

HWTEST_F_L0(DebuggerTypesTest, ScopeToObjectTest)
{
    std::string msg;
    std::unique_ptr<Scope> scope;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "type":"global","object":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::Dataview + R"("},"name":"name9",
          "startLocation":{"scriptId":"2","lineNumber":99},
          "endLocation":{"scriptId":"13","lineNumber":146}
    }})";
    scope = Scope::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(scope, nullptr);
    Local<ObjectRef> object = scope->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("global", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "object");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    Local<JSValueRef> subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Dataview.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "name");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("name9", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startLocation");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ("2", DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(subResult)->Value(), 99);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endLocation");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ("13", DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(subResult)->Value(), 146);
}

HWTEST_F_L0(DebuggerTypesTest, CallFrameCreateTest)
{
    std::string msg;
    std::unique_ptr<CallFrame> callFrame;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":10,"functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":["0"],"functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":10,
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":["name0"],
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0","functionLocation":10,
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0","functionLocation":{"scriptId11":"id5","lineNumber":19},
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId2":"id5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":10,"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":10,"scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":{"url":"url7"},"scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0", "location":{"scriptId":"5","lineNumber":19},
          "url":"url7","scopeChain":10,"this":{"type":")" +
          ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          {"type":"22","object":{"type":")" +
          ObjectType::Object + R"("}},"this":{"type":")" + ObjectType::Object + R"(","subtype":")" +
          ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":10}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"11":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("},
          "returnValue":10}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("},
          "returnValue":{"type":"object","subtype":"11"}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(callFrame, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0",
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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

HWTEST_F_L0(DebuggerTypesTest, CallFrameToObjectTest)
{
    std::string msg;
    std::unique_ptr<CallFrame> callFrame;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":"0","functionName":"name0","functionLocation":{"scriptId":"3","lineNumber":16},
          "location":{"scriptId":"5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::Iterator +
          R"("},"returnValue":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::I64 + R"("}}})";
    callFrame = CallFrame::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(callFrame, nullptr);
    Local<ObjectRef> object = callFrame->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrameId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("0", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functionName");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("name0", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functionLocation");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    Local<JSValueRef> subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ("3", DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(subResult)->Value(), 16);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "location");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ("5", DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(subResult)->Value(), 19);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("url7", DebuggerApi::ToStdString(result));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scopeChain");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));

    EXPECT_EQ(Local<ArrayRef>(result)->Length(ecmaVm), 2);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "this");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::Iterator.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "returnValue");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "type");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectType::Object.c_str()), DebuggerApi::ToStdString(subResult));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "subtype");
    ASSERT_TRUE(Local<ObjectRef>(result)->Has(ecmaVm, Local<JSValueRef>(tmpStr)));
    subResult = Local<ObjectRef>(result)->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(std::string(ObjectSubType::I64.c_str()), DebuggerApi::ToStdString(subResult));
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileSampleCreateTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileSample> object;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

   // abnormal params of params.sub-key = [ "size"="Test","nodeId"="Test","ordinal"="Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "size":"Test","nodeId":"Test","ordinal":"Test"}})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of params.sub-key = [ "size"={"xx":"yy"},"nodeId"={"xx":"yy"},"ordinal"={"xx":"yy"}]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "size":{"xx":"yy"},"nodeId":{"xx":"yy"},"ordinal":{"xx":"yy"}}})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of params.sub-key = [ "size"=100,"nodeId"=1,"ordinal"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"size":100,"nodeId":1,"ordinal":10}})";
    object = SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->GetSize(), 100);
    EXPECT_EQ(object->GetNodeId(), 1);
    EXPECT_EQ(object->GetOrdinal(), 10);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileSampleToObjectTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileSample> samplingHeapProfileSampleData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"size":100,"nodeId":1,"ordinal":10}})";
    samplingHeapProfileSampleData =
        SamplingHeapProfileSample::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(samplingHeapProfileSampleData, nullptr);
    Local<ObjectRef> object = samplingHeapProfileSampleData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "size");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 100);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "nodeId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 1);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "ordinal");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileNodeCreateTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileNode> object;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    object = SamplingHeapProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    object = SamplingHeapProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    object = SamplingHeapProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    object = SamplingHeapProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
        "selfSize":10,
        "id":5,
        "children":[]
    }})";
    object = SamplingHeapProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileNodeToObjectTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfileNode> samplingHeapProfileNode;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
        "selfSize":10,
        "id":5,
        "children":[]
    }})";
    samplingHeapProfileNode = SamplingHeapProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(samplingHeapProfileNode, nullptr);
    Local<ObjectRef> object = samplingHeapProfileNode->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrame");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());

    Local<ObjectRef> subObject = samplingHeapProfileNode->GetCallFrame()->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functionName");
    ASSERT_TRUE(subObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> subResult = subObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(subResult), "Create");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(subObject->Has(ecmaVm, tmpStr));
    subResult = subObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(subResult), "10");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(subObject->Has(ecmaVm, tmpStr));
    subResult = subObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(subResult), "url3");

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "selfSize");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "id");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 5);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "children");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
}

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileCreateTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfile> object;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    object = SamplingHeapProfile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    object = SamplingHeapProfile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    object = SamplingHeapProfile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(object, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    object = SamplingHeapProfile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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
    object = SamplingHeapProfile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
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

HWTEST_F_L0(DebuggerTypesTest, SamplingHeapProfileToObjectTest)
{
    std::string msg;
    std::unique_ptr<SamplingHeapProfile> samplingHeapProfile;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "head": {
            "callFrame": {"functionName":"Create", "scriptId":"10", "url":"url3", "lineNumber":100, "columnNumber":20},
            "selfSize":10,
            "id":5,
            "children":[]
        },
        "samples":[{"size":100, "nodeId":1, "ordinal":10}]
    }})";

    samplingHeapProfile = SamplingHeapProfile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(samplingHeapProfile, nullptr);
    Local<ObjectRef> object = samplingHeapProfile->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "head");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());

    Local<ObjectRef> headObject = samplingHeapProfile->GetHead()->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrame");
    ASSERT_TRUE(headObject->Has(ecmaVm, tmpStr));
    result = headObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());

    Local<ObjectRef> callFrameObject = samplingHeapProfile->GetHead()->GetCallFrame()->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functionName");
    ASSERT_TRUE(callFrameObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> subResult = callFrameObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(subResult), "Create");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(callFrameObject->Has(ecmaVm, tmpStr));
    subResult = callFrameObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(subResult), "10");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(callFrameObject->Has(ecmaVm, tmpStr));
    subResult = callFrameObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!subResult.IsEmpty() && !subResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(subResult), "url3");

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "selfSize");
    ASSERT_TRUE(headObject->Has(ecmaVm, tmpStr));
    result = headObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "id");
    ASSERT_TRUE(headObject->Has(ecmaVm, tmpStr));
    result = headObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 5);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "children");
    ASSERT_TRUE(headObject->Has(ecmaVm, tmpStr));
    result = headObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "samples");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));

    Local<ObjectRef> samplesObject = samplingHeapProfile->GetSamples()->data()->get()->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "size");
    ASSERT_TRUE(samplesObject->Has(ecmaVm, tmpStr));
    result = samplesObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 100);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "nodeId");
    ASSERT_TRUE(samplesObject->Has(ecmaVm, tmpStr));
    result = samplesObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 1);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "ordinal");
    ASSERT_TRUE(samplesObject->Has(ecmaVm, tmpStr));
    result = samplesObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);
}

HWTEST_F_L0(DebuggerTypesTest, PositionTickInfoCreateTest)
{
    std::string msg;
    std::unique_ptr<PositionTickInfo> positionTickInfo;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of params.sub-key=["line":11,"ticks":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "line":"11","ticks":99}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of params.sub-key=["line":"11","ticks":"99"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "line":"11","ticks":"99"}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // abnormal params of params.sub-key=["line":[11],"ticks":[99]]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "line":[11],"ticks":[99]}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(positionTickInfo, nullptr);

    // normal params of params.sub-key=["line":11,"ticks":99]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"line":1,"ticks":0}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(positionTickInfo, nullptr);
    EXPECT_EQ(positionTickInfo->GetLine(), 1);
    EXPECT_EQ(positionTickInfo->GetTicks(), 0);
}


HWTEST_F_L0(DebuggerTypesTest, PositionTickInfoToObjectTest)
{
    std::string msg;
    std::unique_ptr<PositionTickInfo> positionTickInfo;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"line":1,"ticks":0}})";
    positionTickInfo = PositionTickInfo::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(positionTickInfo, nullptr);
    Local<ObjectRef> object = positionTickInfo->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "line");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 1);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "ticks");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 0);
}

HWTEST_F_L0(DebuggerTypesTest, ProfileNodeCreateTest)
{
    std::string msg;
    std::unique_ptr<ProfileNode> profileNode;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    profileNode = ProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profileNode, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    profileNode = ProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profileNode, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    profileNode = ProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profileNode, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    profileNode = ProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profileNode, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "id":10,
          "callFrame": {"functionName":"name0", "scriptId":"12", "url":"url15", "lineNumber":11, "columnNumber":20},
          "hitCount":15,"children":[],"positionTicks":[],"deoptReason":"yyy"}})";
    profileNode = ProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());

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

HWTEST_F_L0(DebuggerTypesTest, ProfileNodeToObjectTest)
{
    std::string msg;
    std::unique_ptr<ProfileNode> profilenode;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "id":10,
          "callFrame": {"functionName":"name0", "scriptId":"12", "url":"url15", "lineNumber":11, "columnNumber":20},
          "hitCount":15,"children":[],"positionTicks":[],"deoptReason":"yyy"}})";
    profilenode = ProfileNode::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(profilenode, nullptr);
    Local<ObjectRef> object = profilenode->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "id");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrame");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());

    Local<ObjectRef> tmpObject = profilenode->GetCallFrame()->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functionName");
    ASSERT_TRUE(tmpObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> tmpResult = tmpObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(tmpResult), "name0");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(tmpObject->Has(ecmaVm, tmpStr));
    tmpResult = tmpObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(tmpResult), "12");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(tmpObject->Has(ecmaVm, tmpStr));
    tmpResult = tmpObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(tmpResult), "url15");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "lineNumber");
    ASSERT_TRUE(tmpObject->Has(ecmaVm, tmpStr));
    tmpResult = tmpObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(tmpResult)->Value(), 11);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "columnNumber");
    ASSERT_TRUE(tmpObject->Has(ecmaVm, tmpStr));
    tmpResult = tmpObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(tmpResult)->Value(), 20);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "hitCount");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 15);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "deoptReason");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "yyy");
}

HWTEST_F_L0(DebuggerTypesTest, ProfileCreateTest)
{
    std::string msg;
    std::unique_ptr<Profile> profile;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    profile = Profile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    profile = Profile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    profile = Profile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    profile = Profile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(profile, nullptr);

    // abnormal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startTime":10,"endTime":25,"nodes":[],"samples":[],"timeDeltas":[]}})";
    profile = Profile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(profile, nullptr);

    EXPECT_EQ(profile->GetStartTime(), 10LL);
    EXPECT_EQ(profile->GetEndTime(), 25LL);
    const std::vector<std::unique_ptr<ProfileNode>> *profileNode = profile->GetNodes();
    ASSERT_NE(profileNode, nullptr);
    EXPECT_EQ((int)profileNode->size(), 0);
}

HWTEST_F_L0(DebuggerTypesTest, ProfileToObjectTest)
{
    std::string msg;
    std::unique_ptr<Profile> profile;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startTime":10,"endTime":25,"nodes":[],"samples":[],"timeDeltas":[]}})";
    profile = Profile::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(profile, nullptr);
    Local<ObjectRef> object = profile->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startTime");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endTime");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 25);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "nodes");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
}

HWTEST_F_L0(DebuggerTypesTest, CoverageCreateTest)
{
    std::string msg;
    std::unique_ptr<Coverage> coverage;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    coverage = Coverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(coverage, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    coverage = Coverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(coverage, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    coverage = Coverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(coverage, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    coverage = Coverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(coverage, nullptr);

    // normal params of params.sub-key=["startOffset":0,"endOffset":5,"count":13]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startOffset":0,"endOffset":13,"count":13}})";
    coverage = Coverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(coverage, nullptr);
    EXPECT_EQ(coverage->GetStartOffset(), 0);
    EXPECT_EQ(coverage->GetEndOffset(), 13);
    EXPECT_EQ(coverage->GetCount(), 13);
}

HWTEST_F_L0(DebuggerTypesTest, CoverageToObjectTest)
{
    std::string msg;
    std::unique_ptr<Coverage> coverage;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "startOffset":0,"endOffset":13,"count":13}})";
    coverage = Coverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(coverage, nullptr);
    Local<ObjectRef> object = coverage->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startOffset");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 0);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endOffset");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 13);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "count");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 13);
}

HWTEST_F_L0(DebuggerTypesTest, FunctionCoverageCreateTest)
{
    std::string msg;
    std::unique_ptr<FunctionCoverage> functionCoverage;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    functionCoverage = FunctionCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(functionCoverage, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    functionCoverage = FunctionCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(functionCoverage, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    functionCoverage = FunctionCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(functionCoverage, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    functionCoverage = FunctionCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(functionCoverage, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "functionName":"Create0","ranges":[],"isBlockCoverage":true}})";
    functionCoverage = FunctionCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());

    ASSERT_NE(functionCoverage, nullptr);
    EXPECT_EQ(functionCoverage->GetFunctionName(), "Create0");
    const std::vector<std::unique_ptr<Coverage>> *ranges = functionCoverage->GetRanges();
    ASSERT_NE(ranges, nullptr);
    EXPECT_EQ((int)ranges->size(), 0);
    ASSERT_TRUE(functionCoverage->GetIsBlockCoverage());
}

HWTEST_F_L0(DebuggerTypesTest, FunctionCoverageToObjectTest)
{
    std::string msg;
    std::unique_ptr<FunctionCoverage> functionCoverage;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "functionName":"Create0","ranges":[],"isBlockCoverage":true}})";
    functionCoverage = FunctionCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(functionCoverage, nullptr);
    Local<ObjectRef> object = functionCoverage->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functionName");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "Create0");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "ranges");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "isBlockCoverage");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
}

HWTEST_F_L0(DebuggerTypesTest, ScriptCoverageCreateTest)
{
    std::string msg;
    std::unique_ptr<ScriptCoverage> scriptCoverage;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    scriptCoverage = ScriptCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scriptCoverage, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    scriptCoverage = ScriptCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scriptCoverage, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    scriptCoverage = ScriptCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scriptCoverage, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    scriptCoverage = ScriptCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    EXPECT_EQ(scriptCoverage, nullptr);

    // normal params of params.sub-key=[..]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"1001","url":"url17","functions":[]}})";
    scriptCoverage = ScriptCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(scriptCoverage, nullptr);
    EXPECT_EQ(scriptCoverage->GetScriptId(), "1001");
    EXPECT_EQ(scriptCoverage->GetUrl(), "url17");
    const std::vector<std::unique_ptr<FunctionCoverage>> *functions = scriptCoverage->GetFunctions();
    ASSERT_NE(functions, nullptr);
    EXPECT_EQ((int)functions->size(), 0);
}

HWTEST_F_L0(DebuggerTypesTest, ScriptCoverageToObjectTest)
{
    std::string msg;
    std::unique_ptr<ScriptCoverage> scriptCoverage;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
          "scriptId":"1001","url":"url17","functions":[]}})";
    scriptCoverage = ScriptCoverage::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParamsObj());
    ASSERT_NE(scriptCoverage, nullptr);
    Local<ObjectRef> object = scriptCoverage->ToObject(ecmaVm);
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "1001");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "url17");
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "functions");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
}
}  // namespace panda::test
