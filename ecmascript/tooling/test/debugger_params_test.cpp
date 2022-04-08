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
using namespace panda::tooling::ecmascript;

namespace panda::test {

// Duplicate name of panda::ecmascript::PropertyDescriptor in js_object-inl.h
using panda::tooling::ecmascript::PropertyDescriptor;

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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        ecmaVm = EcmaVM::Cast(instance);
        // Main logic is JSON parser, so not need trigger GC to decrease execute time
        ecmaVm->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(DebuggerParamsTest, StartSamplingParamsCreateTest)
{
    CString msg;
    std::unique_ptr<StartSamplingParams> startSamplingData;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    EXPECT_EQ(startSamplingData->GetSamplingInterval(), 32768);

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    EXPECT_EQ(startSamplingData->GetSamplingInterval(), 32768);

    // abnormal params of params.sub-key=["samplingInterval":true]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":true}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of params.sub-key=["samplingInterval":true]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":"Test"}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(startSamplingData, nullptr);

    // abnormal params of params.sub-key = [ "size"=100,"nodeId"=1,"ordinal"=10]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":1000}})";
    startSamplingData = StartSamplingParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(startSamplingData, nullptr);
    EXPECT_EQ(startSamplingData->GetSamplingInterval(), 1000);
}

HWTEST_F_L0(DebuggerParamsTest, StartSamplingParamsToObjectTest)
{
    CString msg;
    std::unique_ptr<StartSamplingParams> startSamplingData;
    Local<StringRef> tmpStr;

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"samplingInterval":1000}})";
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
    CString msg;
    std::unique_ptr<StartTrackingHeapObjectsParams> objectData;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetTrackAllocations());

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetTrackAllocations());

    // abnormal params of params.sub-key=["trackAllocations":10]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":10}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["trackAllocations":"Test"]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":"Test"}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["trackAllocations":true]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":true}})";
    objectData = StartTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_TRUE(objectData->GetTrackAllocations());
}

HWTEST_F_L0(DebuggerParamsTest,StartTrackingHeapObjectsParamsToObjectTest)
{
    CString msg;
    std::unique_ptr<StartTrackingHeapObjectsParams> startTrackingData;
    Local<StringRef> tmpStr;

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"trackAllocations":true}})";
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
    CString msg;
    std::unique_ptr<StopTrackingHeapObjectsParams> objectData;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetReportProgress());
    ASSERT_FALSE(objectData->GetTreatGlobalObjectsAsRoots());
    ASSERT_FALSE(objectData->GetCaptureNumericValue());

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_FALSE(objectData->GetReportProgress());
    ASSERT_FALSE(objectData->GetTreatGlobalObjectsAsRoots());
    ASSERT_FALSE(objectData->GetCaptureNumericValue());

    // abnormal params of params.sub-key=["reportProgress":10,"treatGlobalObjectsAsRoots":10,"captureNumericValue":10]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"reportProgress":10,"treatGlobalObjectsAsRoots":10,"captureNumericValue":10}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["reportProgress":"Test","treatGlobalObjectsAsRoots":"Test","captureNumericValue":"Test"]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"reportProgress":"Test","treatGlobalObjectsAsRoots":"Test","captureNumericValue":"Test"}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(objectData, nullptr);

    // abnormal params of params.sub-key=["reportProgress":true,"treatGlobalObjectsAsRoots":true,"captureNumericValue":true]
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"reportProgress":true,"treatGlobalObjectsAsRoots":true,"captureNumericValue":true}})";
    objectData = StopTrackingHeapObjectsParams::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(objectData, nullptr);
    ASSERT_TRUE(objectData->GetReportProgress());
    ASSERT_TRUE(objectData->GetTreatGlobalObjectsAsRoots());
    ASSERT_TRUE(objectData->GetCaptureNumericValue());
}

HWTEST_F_L0(DebuggerParamsTest,StopTrackingHeapObjectsParamsToObjectTest)
{
    CString msg;
    std::unique_ptr<StopTrackingHeapObjectsParams> stopTrackingData;
    Local<StringRef> tmpStr;

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"reportProgress":true,"treatGlobalObjectsAsRoots":true,"captureNumericValue":true}})";
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

}  // namespace panda::test
