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
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {
// Duplicate name of panda::ecmascript::PropertyDescriptor in js_object-inl.h
using panda::ecmascript::tooling::PropertyDescriptor;
using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

class DebuggerReturnsTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
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

HWTEST_F_L0(DebuggerReturnsTest, EnableReturnsToJsonTest)
{
    std::unique_ptr<EnableReturns> enableReturns = std::make_unique<EnableReturns>(100U);
    ASSERT_NE(enableReturns, nullptr);

    std::string debuggerId;
    ASSERT_EQ(enableReturns->ToJson()->GetString("debuggerId", &debuggerId), Result::SUCCESS);
    EXPECT_EQ(debuggerId, "100");
}

HWTEST_F_L0(DebuggerReturnsTest, SetBreakpointByUrlReturnsToJsonTest)
{
    auto locations = std::vector<std::unique_ptr<Location>>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(1).SetLine(99);
    locations.emplace_back(std::move(location));
    std::unique_ptr<SetBreakpointByUrlReturns> setBreakpointByUrlReturns
                     = std::make_unique<SetBreakpointByUrlReturns>("11", std::move(locations));
    ASSERT_NE(setBreakpointByUrlReturns, nullptr);
    std::string id;
    ASSERT_EQ(setBreakpointByUrlReturns->ToJson()->GetString("breakpointId", &id), Result::SUCCESS);
    EXPECT_EQ(id, "11");

    std::unique_ptr<PtJson> locationsJson;
    ASSERT_EQ(setBreakpointByUrlReturns->ToJson()->GetArray("locations", &locationsJson), Result::SUCCESS);
    ASSERT_NE(locationsJson, nullptr);
    EXPECT_EQ(locationsJson->GetSize(), 1);
}

HWTEST_F_L0(DebuggerReturnsTest, EvaluateOnCallFrameReturnsToJsonTest)
{
    std::unique_ptr<RemoteObject> result = std::make_unique<RemoteObject>();
    result->SetType("idle");
    std::unique_ptr<ExceptionDetails> exceptionDetails = std::make_unique<ExceptionDetails>();
    exceptionDetails->SetExceptionId(12);
    std::unique_ptr<EvaluateOnCallFrameReturns> evaluateOnCallFrameReturns
                     = std::make_unique<EvaluateOnCallFrameReturns>(std::move(result), std::move(exceptionDetails));
    ASSERT_NE(evaluateOnCallFrameReturns, nullptr);
    std::unique_ptr<PtJson> json;
    ASSERT_EQ(evaluateOnCallFrameReturns->ToJson()->GetObject("result", &json), Result::SUCCESS);
    std::string type;
    ASSERT_EQ(json->GetString("type", &type), Result::SUCCESS);
    EXPECT_EQ(type, "idle");

    std::unique_ptr<PtJson> tmpJson;
    ASSERT_EQ(evaluateOnCallFrameReturns->ToJson()->GetObject("exceptionDetails", &tmpJson), Result::SUCCESS);
    int32_t exceptionId;
    ASSERT_EQ(tmpJson->GetInt("exceptionId", &exceptionId), Result::SUCCESS);
    EXPECT_EQ(exceptionId, 12);
}

HWTEST_F_L0(DebuggerReturnsTest, GetPossibleBreakpointsReturnsToJsonTest)
{
    auto locations = std::vector<std::unique_ptr<BreakLocation>>();
    std::unique_ptr<BreakLocation> breakLocation = std::make_unique<BreakLocation>();
    breakLocation->SetScriptId(11).SetLine(1).SetColumn(44).SetType("idel5");
    locations.emplace_back(std::move(breakLocation));
    std::unique_ptr<GetPossibleBreakpointsReturns> getPossibleBreakpointsReturns = std::make_unique
                                                    <GetPossibleBreakpointsReturns>(std::move(locations));

    std::unique_ptr<PtJson> locationsJson;
    ASSERT_EQ(getPossibleBreakpointsReturns->ToJson()->GetArray("locations", &locationsJson), Result::SUCCESS);
    ASSERT_NE(locationsJson, nullptr);
    EXPECT_EQ(locationsJson->GetSize(), 1);
}

HWTEST_F_L0(DebuggerReturnsTest, GetScriptSourceReturnsToJsonTest)
{
    std::unique_ptr<GetScriptSourceReturns> getScriptSourceReturns = std::make_unique
                                                                    <GetScriptSourceReturns>("source_1", "bytecode_1");
    ASSERT_NE(getScriptSourceReturns, nullptr);
    
    std::string scriptSource;
    ASSERT_EQ(getScriptSourceReturns->ToJson()->GetString("scriptSource", &scriptSource), Result::SUCCESS);
    EXPECT_EQ(scriptSource, "source_1");

    std::string bytecode;
    ASSERT_EQ(getScriptSourceReturns->ToJson()->GetString("bytecode", &bytecode), Result::SUCCESS);
    EXPECT_EQ(bytecode, "bytecode_1");
}

HWTEST_F_L0(DebuggerReturnsTest, RestartFrameReturnsToJsonTest)
{
    auto callFrames = std::vector<std::unique_ptr<CallFrame>>();
    std::unique_ptr<CallFrame> callFrame = std::make_unique<CallFrame>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(13).SetLine(16);

    std::unique_ptr<RemoteObject> res = std::make_unique<RemoteObject>();
    res->SetType("idle2");

    callFrame->SetCallFrameId(55);
    callFrame->SetLocation(std::move(location));
    callFrame->SetThis(std::move(res));
    callFrames.emplace_back(std::move(callFrame));
    std::unique_ptr<RestartFrameReturns> restartFrameReturns = std::make_unique
                                                                 <RestartFrameReturns>(std::move(callFrames));
    
    std::unique_ptr<PtJson> json;
    ASSERT_EQ(restartFrameReturns->ToJson()->GetArray("callFrames", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 1);
}

HWTEST_F_L0(DebuggerReturnsTest, SetBreakpointReturnsToJsonTest)
{
    std::unique_ptr<Location> location = std::make_unique<Location>();
    std::unique_ptr<SetBreakpointReturns> setBreakpointReturns
                     = std::make_unique<SetBreakpointReturns>("breakpointId_1", std::move(location));
    ASSERT_NE(setBreakpointReturns, nullptr);
    
    std::string breakpointId;
    ASSERT_EQ(setBreakpointReturns->ToJson()->GetString("breakpointId", &breakpointId), Result::SUCCESS);
    EXPECT_EQ(breakpointId, "breakpointId_1");

    std::unique_ptr<PtJson> tmpJson;
    ASSERT_EQ(setBreakpointReturns->ToJson()->GetObject("actualLocation", &tmpJson), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerReturnsTest, SetInstrumentationBreakpointReturnsToJsonTest)
{
    std::unique_ptr<SetInstrumentationBreakpointReturns> setInstrumentationBreakpointReturns
                     = std::make_unique<SetInstrumentationBreakpointReturns>("111");
    ASSERT_NE(setInstrumentationBreakpointReturns, nullptr);
    
    std::string breakpointId;
    ASSERT_EQ(setInstrumentationBreakpointReturns->ToJson()->GetString("breakpointId", &breakpointId),
              Result::SUCCESS);
    EXPECT_EQ(breakpointId, "111");
}

HWTEST_F_L0(DebuggerReturnsTest, SetScriptSourceReturnsToJsonTest)
{
    auto callFrames = std::vector<std::unique_ptr<CallFrame>>();
    std::unique_ptr<CallFrame> callFrame = std::make_unique<CallFrame>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(3).SetLine(36);

    std::unique_ptr<RemoteObject> res = std::make_unique<RemoteObject>();
    res->SetType("idle5");

    callFrame->SetCallFrameId(33);
    callFrame->SetLocation(std::move(location));
    callFrame->SetThis(std::move(res));
    callFrames.emplace_back(std::move(callFrame));
    std::unique_ptr<SetScriptSourceReturns> setScriptSourceReturns = std::make_unique
                                                                <SetScriptSourceReturns>(std::move(callFrames));
    
    std::unique_ptr<PtJson> json;
    ASSERT_EQ(setScriptSourceReturns->ToJson()->GetArray("callFrames", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 1);
}

HWTEST_F_L0(DebuggerReturnsTest, GetPropertiesReturnsToJsonTest)
{
    auto descriptor = std::vector<std::unique_ptr<PropertyDescriptor>>();
    std::unique_ptr<PropertyDescriptor> propertyDescriptor = std::make_unique<PropertyDescriptor>();
    propertyDescriptor->SetName("filename1").SetConfigurable(true).SetEnumerable(true);
    descriptor.emplace_back(std::move(propertyDescriptor));
    std::unique_ptr<GetPropertiesReturns> getPropertiesReturns = std::make_unique
                                                        <GetPropertiesReturns>(std::move(descriptor));
    ASSERT_NE(getPropertiesReturns, nullptr);
    
    std::unique_ptr<PtJson> json;
    ASSERT_EQ(getPropertiesReturns->ToJson()->GetArray("result", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 1);
}

HWTEST_F_L0(DebuggerReturnsTest, CallFunctionOnReturnsToJsonTest)
{
    std::unique_ptr<RemoteObject> result = std::make_unique<RemoteObject>();
    result->SetType("idle2");
    std::unique_ptr<ExceptionDetails> exceptionDetails = std::make_unique<ExceptionDetails>();
    std::unique_ptr<CallFunctionOnReturns> callFunctionOnReturns
                     = std::make_unique<CallFunctionOnReturns>(std::move(result), std::move(exceptionDetails));
    ASSERT_NE(callFunctionOnReturns, nullptr);
    std::unique_ptr<PtJson> json;
    ASSERT_EQ(callFunctionOnReturns->ToJson()->GetObject("result", &json), Result::SUCCESS);
    std::string type;
    ASSERT_EQ(json->GetString("type", &type), Result::SUCCESS);
    EXPECT_EQ(type, "idle2");

    std::unique_ptr<PtJson> tmpJson;
    ASSERT_EQ(callFunctionOnReturns->ToJson()->GetObject("exceptionDetails", &tmpJson), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerReturnsTest, StopSamplingReturnsToJsonTest)
{
    auto res = std::vector<std::unique_ptr<SamplingHeapProfileSample>>();
    std::unique_ptr<RuntimeCallFrame> runtime = std::make_unique<RuntimeCallFrame>();
    std::unique_ptr<SamplingHeapProfileNode> node = std::make_unique<SamplingHeapProfileNode>();
    node->SetCallFrame(std::move(runtime));
    std::unique_ptr<SamplingHeapProfile> profile = std::make_unique<SamplingHeapProfile>();
    profile->SetHead(std::move(node));
    profile->SetSamples(std::move(res));
    std::unique_ptr<StopSamplingReturns> stopSamplingReturns =
                                         std::make_unique<StopSamplingReturns>(std::move(profile));
    ASSERT_NE(stopSamplingReturns, nullptr);

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(stopSamplingReturns->ToJson()->GetObject("profile", &json), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerReturnsTest, GetHeapObjectIdReturnsToJsonTest)
{
    std::unique_ptr<GetHeapObjectIdReturns> getHeapObjectIdReturns = std::make_unique<GetHeapObjectIdReturns>(10);
    ASSERT_NE(getHeapObjectIdReturns, nullptr);

    std::string heapSnapshotObjectId;
    ASSERT_EQ(getHeapObjectIdReturns->ToJson()->GetString("heapSnapshotObjectId", &heapSnapshotObjectId),
              Result::SUCCESS);
    EXPECT_EQ(heapSnapshotObjectId, "10");
}

HWTEST_F_L0(DebuggerReturnsTest, GetObjectByHeapObjectIdReturnsToJsonTest)
{
    std::unique_ptr<RemoteObject> remoteObjectResult = std::make_unique<RemoteObject>();
    remoteObjectResult->SetType("idle5");
    std::unique_ptr<GetObjectByHeapObjectIdReturns> getObjectByHeapObjectIdReturns =
                                    std::make_unique<GetObjectByHeapObjectIdReturns>(std::move(remoteObjectResult));
    ASSERT_NE(getObjectByHeapObjectIdReturns, nullptr);

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(getObjectByHeapObjectIdReturns->ToJson()->GetObject("result", &json), Result::SUCCESS);
    std::string type;
    ASSERT_EQ(json->GetString("type", &type), Result::SUCCESS);
    EXPECT_EQ(type, "idle5");
}

HWTEST_F_L0(DebuggerReturnsTest, StopReturnsToJsonTest)
{
    std::unique_ptr<Profile> profile = std::make_unique<Profile>();
    std::unique_ptr<StopReturns> stopReturns= std::make_unique<StopReturns>(std::move(profile));
    ASSERT_NE(stopReturns, nullptr);

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(stopReturns->ToJson()->GetObject("profile", &json), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerReturnsTest, GetHeapUsageReturnsToJsonTest)
{
    double usedSize = 1;
    double totalSize = 1;
    std::unique_ptr<GetHeapUsageReturns> getHeapUsageReturns =
        std::make_unique<GetHeapUsageReturns>(usedSize, totalSize);
    ASSERT_NE(getHeapUsageReturns, nullptr);

    double pUsedSize;
    ASSERT_EQ(getHeapUsageReturns->ToJson()->GetDouble("usedSize", &pUsedSize), Result::SUCCESS);
    EXPECT_EQ(pUsedSize, 1);

    double pTotalSize;
    ASSERT_EQ(getHeapUsageReturns->ToJson()->GetDouble("totalSize", &pTotalSize), Result::SUCCESS);
    EXPECT_EQ(pTotalSize, 1);
}

HWTEST_F_L0(DebuggerReturnsTest, GetBestEffortCoverageReturnsToJsonTest)
{
    auto result = std::vector<std::unique_ptr<ScriptCoverage>>();
    std::unique_ptr<ScriptCoverage> scriptCoverage = std::make_unique<ScriptCoverage>();
    std::unique_ptr<GetBestEffortCoverageReturns> getBestEffortCoverageReturns =
                                                std::make_unique<GetBestEffortCoverageReturns>(std::move(result));

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(getBestEffortCoverageReturns->ToJson()->GetArray("result", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 0);
}

HWTEST_F_L0(DebuggerReturnsTest, StartPreciseCoverageReturnsToJsonTest)
{
    std::unique_ptr<StartPreciseCoverageReturns> startPreciseCoverageReturns
                     = std::make_unique<StartPreciseCoverageReturns>(1001);
    ASSERT_NE(startPreciseCoverageReturns, nullptr);

    int32_t timestamp;
    ASSERT_EQ(startPreciseCoverageReturns->ToJson()->GetInt("timestamp", &timestamp), Result::SUCCESS);
    EXPECT_EQ(timestamp, 1001);
}

HWTEST_F_L0(DebuggerReturnsTest, TakePreciseCoverageReturnsToJsonTest)
{
    auto coverage = std::vector<std::unique_ptr<ScriptCoverage>>();
    std::unique_ptr<TakePreciseCoverageReturns> takePreciseCoverageReturns =
                                                std::make_unique<TakePreciseCoverageReturns>(std::move(coverage), 1001);
    ASSERT_NE(takePreciseCoverageReturns, nullptr);

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(takePreciseCoverageReturns->ToJson()->GetArray("result", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 0);

    int32_t timestamp;
    ASSERT_EQ(takePreciseCoverageReturns->ToJson()->GetInt("timestamp", &timestamp), Result::SUCCESS);
    EXPECT_EQ(timestamp, 1001);
}

HWTEST_F_L0(DebuggerReturnsTest, TakeTypeProfileturnsToJsonTest)
{
    auto result = std::vector<std::unique_ptr<ScriptTypeProfile>>();
    std::unique_ptr<ScriptTypeProfile> scriptTypeProfile = std::make_unique<ScriptTypeProfile>();
    std::unique_ptr<TakeTypeProfileReturns> takeTypeProfileturns = std::make_unique
                                                    <TakeTypeProfileReturns>(std::move(result));

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(takeTypeProfileturns->ToJson()->GetArray("result", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 0);
}

HWTEST_F_L0(DebuggerReturnsTest, GetCategoriesReturnsToJsonTest)
{
    auto result = std::vector<std::string>();
    std::unique_ptr<GetCategoriesReturns> getCategoriesReturns = std::make_unique
                                                    <GetCategoriesReturns>(std::move(result));

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(getCategoriesReturns->ToJson()->GetArray("categories", &json), Result::SUCCESS);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->GetSize(), 0);
}

HWTEST_F_L0(DebuggerReturnsTest, RequestMemoryDumpReturnsToJsonTest)
{
    std::unique_ptr<RequestMemoryDumpReturns> requestMemoryDumpReturns
                     = std::make_unique<RequestMemoryDumpReturns>("123", true);
    ASSERT_NE(requestMemoryDumpReturns, nullptr);

    std::string dumpGuid;
    ASSERT_EQ(requestMemoryDumpReturns->ToJson()->GetString("dumpGuid", &dumpGuid),
              Result::SUCCESS);
    EXPECT_EQ(dumpGuid, "123");

    bool success;
    ASSERT_EQ(requestMemoryDumpReturns->ToJson()->GetBool("success", &success), Result::SUCCESS);
    ASSERT_TRUE(success);
}
}  // namespace panda::test