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
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/dispatcher.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {
// Duplicate name of panda::ecmascript::PropertyDescriptor in js_object-inl.h
using panda::ecmascript::tooling::PropertyDescriptor;

using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

class DebuggerParamsTest : public testing::Test {
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

HWTEST_F_L0(DebuggerParamsTest, StartSamplingParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<StartSamplingParams> startSamplingData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    EXPECT_EQ(startSamplingData->GetSamplingInterval(), 32768);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    EXPECT_EQ(startSamplingData->GetSamplingInterval(), 32768);

    // abnormal params of params.sub-key=["samplingInterval":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":true}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of params.sub-key=["samplingInterval":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":"Test"}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of params.sub-key = [ "size"=100,"nodeId"=1,"ordinal"=10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":1000}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    EXPECT_EQ(startSamplingData->GetSamplingInterval(), 1000);
}

HWTEST_F_L0(DebuggerParamsTest, StartSamplingParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<StartSamplingParams> startSamplingData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":1000}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    Local<ObjectRef> object = startSamplingData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "samplingInterval");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<NumberRef>(result)->Value(), 1000);
}

HWTEST_F_L0(DebuggerParamsTest, StartTrackingHeapObjectsParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<StartTrackingHeapObjectsParams> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetTrackAllocations());

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetTrackAllocations());

    // abnormal params of params.sub-key=["trackAllocations":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":10}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["trackAllocations":"Test"]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":"Test"}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["trackAllocations":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":true}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_TRUE(objectData->GetTrackAllocations());
}

HWTEST_F_L0(DebuggerParamsTest, StartTrackingHeapObjectsParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<StartTrackingHeapObjectsParams> startTrackingData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":true}})";
    startTrackingData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startTrackingData, nullptr);
    Local<ObjectRef> object = startTrackingData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "trackAllocations");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
}

HWTEST_F_L0(DebuggerParamsTest, StopTrackingHeapObjectsParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<StopTrackingHeapObjectsParams> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetReportProgress());
    ASSERT_FALSE(objectData->GetTreatGlobalObjectsAsRoots());
    ASSERT_FALSE(objectData->GetCaptureNumericValue());

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetReportProgress());
    ASSERT_FALSE(objectData->GetTreatGlobalObjectsAsRoots());
    ASSERT_FALSE(objectData->GetCaptureNumericValue());

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "reportProgress":10,
            "treatGlobalObjectsAsRoots":10,
            "captureNumericValue":10}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "reportProgress":"Test",
            "treatGlobalObjectsAsRoots":"Test",
            "captureNumericValue":"Test"}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "reportProgress":true,
            "treatGlobalObjectsAsRoots":true,
            "captureNumericValue":true}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_TRUE(objectData->GetReportProgress());
    ASSERT_TRUE(objectData->GetTreatGlobalObjectsAsRoots());
    ASSERT_TRUE(objectData->GetCaptureNumericValue());
}

HWTEST_F_L0(DebuggerParamsTest, StopTrackingHeapObjectsParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<StopTrackingHeapObjectsParams> stopTrackingData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
        "reportProgress":true,
        "treatGlobalObjectsAsRoots":true,
        "captureNumericValue":true}})";
    stopTrackingData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(stopTrackingData, nullptr);
    Local<ObjectRef> object = stopTrackingData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "reportProgress");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "treatGlobalObjectsAsRoots");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "captureNumericValue");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
}

HWTEST_F_L0(DebuggerParamsTest, AddInspectedHeapObjectParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<AddInspectedHeapObjectParams> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["heapObjectId":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"heapObjectId":10}})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["heapObjectId":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"heapObjectId":true}})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["heapObjectId":“10”]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"heapObjectId":"10"}})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    EXPECT_EQ((int)objectData->GetHeapObjectId(), 10);
}

HWTEST_F_L0(DebuggerParamsTest, AddInspectedHeapObjectParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<AddInspectedHeapObjectParams> objectData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"heapObjectId":"10"}})";
    objectData = AddInspectedHeapObjectParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    Local<ObjectRef> object = objectData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "heapObjectId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "10");
}

HWTEST_F_L0(DebuggerParamsTest, GetHeapObjectIdParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<GetHeapObjectIdParams> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["objectId":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":10}})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["objectId":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":true}})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["objectId":“10”]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":"10"}})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    EXPECT_EQ((int)objectData->GetObjectId(), 10);
}

HWTEST_F_L0(DebuggerParamsTest, GetHeapObjectIdParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<GetHeapObjectIdParams> objectData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":"10"}})";
    objectData = GetHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    Local<ObjectRef> object = objectData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "objectId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "10");
}

HWTEST_F_L0(DebuggerParamsTest, GetObjectByHeapObjectIdParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<GetObjectByHeapObjectIdParams> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["objectId":10]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":10}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["objectId":true]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":"10", "objectGroup":10}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["objectId":“10”]
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":"10"}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    EXPECT_EQ((int)objectData->GetObjectId(), 10);
    ASSERT_FALSE(objectData->HasObjectGroup());

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":"10", "objectGroup":"groupname"}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    EXPECT_EQ((int)objectData->GetObjectId(), 10);
    EXPECT_EQ(objectData->GetObjectGroup(), "groupname");
}

HWTEST_F_L0(DebuggerParamsTest, GetObjectByHeapObjectIdParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<GetObjectByHeapObjectIdParams> objectData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"objectId":"10", "objectGroup":"groupname"}})";
    objectData = GetObjectByHeapObjectIdParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    Local<ObjectRef> object = objectData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "objectId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "10");

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "objectGroup");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(DebuggerApi::ToStdString(result), "groupname");
}

HWTEST_F_L0(DebuggerParamsTest, StartPreciseCoverageParamCreateTest)
{
    std::string msg;
    std::unique_ptr<StartPreciseCoverageParam> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "callCount":8,
            "detailed":8,
            "allowTriggeredUpdates":8}})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "callCount":"Test",
            "detailed":"Test",
            "allowTriggeredUpdates":"Test"}})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "callCount":true,
            "detailed":true,
            "allowTriggeredUpdates":true}})";
    objectData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_TRUE(objectData->GetCallCount());
    ASSERT_TRUE(objectData->GetDetailed());
    ASSERT_TRUE(objectData->GetAllowTriggeredUpdates());
}

HWTEST_F_L0(DebuggerParamsTest, StartPreciseCoverageParamToObjectTest)
{
    std::string msg;
    std::unique_ptr<StartPreciseCoverageParam> startTrackingData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "callCount":true,
            "detailed":true,
            "allowTriggeredUpdates":true}})";
    startTrackingData = StartPreciseCoverageParam::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startTrackingData, nullptr);
    Local<ObjectRef> object = startTrackingData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "callCount");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "detailed");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "allowTriggeredUpdates");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());
}

HWTEST_F_L0(DebuggerParamsTest, SetSamplingIntervalParamsCreateTest)
{
    std::string msg;
    std::unique_ptr<SetSamplingIntervalParams> objectData;

    //  abnormal params of null msg
    msg = std::string() + R"({})";
    objectData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = std::string() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unknown params.sub-key
    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{
            "interval":"500"}})";
    objectData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"interval":500}})";
    objectData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    EXPECT_EQ(objectData->GetInterval(), 500);
}

HWTEST_F_L0(DebuggerParamsTest, SetSamplingIntervalParamsToObjectTest)
{
    std::string msg;
    std::unique_ptr<SetSamplingIntervalParams> setSamplingIntervalData;
    Local<StringRef> tmpStr;

    msg = std::string() + R"({"id":0,"method":"Debugger.Test","params":{"interval":500}})";
    setSamplingIntervalData = SetSamplingIntervalParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(setSamplingIntervalData, nullptr);
    Local<ObjectRef> object = setSamplingIntervalData->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "interval");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 500);
}
}  // namespace panda::test
