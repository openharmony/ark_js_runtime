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
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_types.h"
#include "ecmascript/tooling/dispatcher.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;
using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

namespace panda::test {
class DebuggerEventsTest : public testing::Test {
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

HWTEST_F_L0(DebuggerEventsTest, BreakpointResolvedToJsonTest)
{
    BreakpointResolved breakpointResolved;

    auto location = std::make_unique<Location>();
    location->SetScriptId(2).SetLine(99);
    breakpointResolved.SetBreakpointId("00").SetLocation(std::move(location));

    std::unique_ptr<PtJson> json;
    ASSERT_EQ(breakpointResolved.ToJson()->GetObject("params", &json), Result::SUCCESS);
    std::string breakpointId;
    ASSERT_EQ(json->GetString("breakpointId", &breakpointId), Result::SUCCESS);
    EXPECT_EQ(breakpointId, "00");

    std::unique_ptr<PtJson> locationJson;
    ASSERT_EQ(json->GetObject("location", &locationJson), Result::SUCCESS);
    std::string scriptId;
    ASSERT_EQ(locationJson->GetString("scriptId", &scriptId), Result::SUCCESS);
    EXPECT_EQ(scriptId, "2");
    int32_t lineNumber;
    ASSERT_EQ(locationJson->GetInt("lineNumber", &lineNumber), Result::SUCCESS);
    EXPECT_EQ(lineNumber, 99);
}

HWTEST_F_L0(DebuggerEventsTest, PausedToJsonTest)
{
    Paused paused;
    std::vector<std::unique_ptr<CallFrame>> v;
    paused.SetCallFrames(std::move(v))
        .SetReason(PauseReason::EXCEPTION);
    
    std::unique_ptr<PtJson> json = paused.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    std::string reason;
    ASSERT_EQ(params->GetString("reason", &reason), Result::SUCCESS);
    EXPECT_EQ("exception", reason);
    std::unique_ptr<PtJson> callFrames;
    ASSERT_EQ(params->GetArray("callFrames", &callFrames), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerEventsTest, ResumedToJsonTest)
{
    Resumed resumed;
    std::unique_ptr<PtJson> json = resumed.ToJson();

    std::string method;
    ASSERT_EQ(json->GetString("method", &method), Result::SUCCESS);
    EXPECT_EQ(resumed.GetName(), method);
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerEventsTest, ScriptFailedToParseToJsonTest)
{
    ScriptFailedToParse parsed;
    parsed.SetScriptId(100)
        .SetUrl("use/test.js")
        .SetStartLine(0)
        .SetStartColumn(4)
        .SetEndLine(10)
        .SetEndColumn(10)
        .SetExecutionContextId(2)
        .SetHash("hash0001")
        .SetSourceMapURL("usr/")
        .SetHasSourceURL(true)
        .SetIsModule(true)
        .SetLength(34)
        .SetCodeOffset(432)
        .SetScriptLanguage("JavaScript")
        .SetEmbedderName("hh");
    
    std::unique_ptr<PtJson> json = parsed.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    std::string tmpStr;
    ASSERT_EQ(params->GetString("scriptId", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("100", tmpStr);

    ASSERT_EQ(params->GetString("url", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("use/test.js", tmpStr);

    int tmpInt;
    ASSERT_EQ(params->GetInt("startLine", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 0);

    ASSERT_EQ(params->GetInt("startColumn", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 4);

    ASSERT_EQ(params->GetInt("endLine", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ASSERT_EQ(params->GetInt("endColumn", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ASSERT_EQ(params->GetInt("executionContextId", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 2);

    ASSERT_EQ(params->GetString("hash", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("hash0001", tmpStr);

    ASSERT_EQ(params->GetString("sourceMapURL", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("usr/", tmpStr);

    bool tmpBool;
    ASSERT_EQ(params->GetBool("hasSourceURL", &tmpBool), Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ASSERT_EQ(params->GetBool("isModule", &tmpBool), Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ASSERT_EQ(params->GetInt("length", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 34);

    ASSERT_EQ(params->GetInt("codeOffset", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 432);

    ASSERT_EQ(params->GetString("scriptLanguage", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("JavaScript", tmpStr);

    ASSERT_EQ(params->GetString("embedderName", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("hh", tmpStr);
}

HWTEST_F_L0(DebuggerEventsTest, ScriptParsedToJsonTest)
{
    ScriptParsed parsed;
    parsed.SetScriptId(10)
        .SetUrl("use/test.js")
        .SetStartLine(0)
        .SetStartColumn(4)
        .SetEndLine(10)
        .SetEndColumn(10)
        .SetExecutionContextId(2)
        .SetHash("hash0001")
        .SetIsLiveEdit(true)
        .SetSourceMapURL("usr/")
        .SetHasSourceURL(true)
        .SetIsModule(true)
        .SetLength(34)
        .SetCodeOffset(432)
        .SetScriptLanguage("JavaScript")
        .SetEmbedderName("hh");
    
    std::unique_ptr<PtJson> json = parsed.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    std::string tmpStr;
    ASSERT_EQ(params->GetString("scriptId", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("10", tmpStr);

    ASSERT_EQ(params->GetString("url", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("use/test.js", tmpStr);

    int tmpInt;
    ASSERT_EQ(params->GetInt("startLine", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 0);

    ASSERT_EQ(params->GetInt("startColumn", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 4);

    ASSERT_EQ(params->GetInt("endLine", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ASSERT_EQ(params->GetInt("endColumn", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ASSERT_EQ(params->GetInt("executionContextId", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 2);

    ASSERT_EQ(params->GetString("hash", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("hash0001", tmpStr);

    bool tmpBool;
    ASSERT_EQ(params->GetBool("isLiveEdit", &tmpBool), Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ASSERT_EQ(params->GetString("sourceMapURL", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("usr/", tmpStr);

    ASSERT_EQ(params->GetBool("hasSourceURL", &tmpBool), Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ASSERT_EQ(params->GetBool("isModule", &tmpBool), Result::SUCCESS);
    ASSERT_TRUE(tmpBool);

    ASSERT_EQ(params->GetInt("length", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 34);

    ASSERT_EQ(params->GetInt("codeOffset", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 432);

    ASSERT_EQ(params->GetString("scriptLanguage", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("JavaScript", tmpStr);

    ASSERT_EQ(params->GetString("embedderName", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("hh", tmpStr);
}

HWTEST_F_L0(DebuggerEventsTest, ConsoleProfileFinishedToJsonTest)
{
    ConsoleProfileFinished consoleProfileFinished;

    auto location = std::make_unique<Location>();
    location->SetScriptId(13).SetLine(20);
    std::vector<std::unique_ptr<ProfileNode>> v;
    auto profile = std::make_unique<Profile>();
    profile->SetNodes(std::move(v))
        .SetStartTime(0)
        .SetEndTime(15)
        .SetSamples(std::vector<int32_t>{})
        .SetTimeDeltas(std::vector<int32_t>{});
    consoleProfileFinished.SetId("11").SetLocation(std::move(location)).SetProfile(std::move(profile)).SetTitle("001");

    std::unique_ptr<PtJson> json = consoleProfileFinished.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    std::string tmpStr;
    ASSERT_EQ(params->GetString("id", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("11", tmpStr);

    std::unique_ptr<PtJson> tmpJson;
    ASSERT_EQ(params->GetObject("location", &tmpJson), Result::SUCCESS);
    ASSERT_EQ(params->GetObject("profile", &tmpJson), Result::SUCCESS);

    ASSERT_EQ(params->GetString("title", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("001", tmpStr);
}

HWTEST_F_L0(DebuggerEventsTest, ConsoleProfileStartedToJsonTest)
{
    ConsoleProfileStarted consoleProfileStarted;

    auto location = std::make_unique<Location>();
    location->SetScriptId(17).SetLine(30);
    consoleProfileStarted.SetId("12").SetLocation(std::move(location)).SetTitle("002");

    std::unique_ptr<PtJson> json = consoleProfileStarted.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    std::string tmpStr;
    ASSERT_EQ(params->GetString("id", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("12", tmpStr);

    std::unique_ptr<PtJson> tmpJson = consoleProfileStarted.GetLocation()->ToJson();
    ASSERT_EQ(tmpJson->GetString("scriptId", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("17", tmpStr);
    int tmpInt;
    ASSERT_EQ(tmpJson->GetInt("lineNumber", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 30);

    ASSERT_EQ(params->GetString("title", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("002", tmpStr);
}

HWTEST_F_L0(DebuggerEventsTest, PreciseCoverageDeltaUpdateToJsonTest)
{
    PreciseCoverageDeltaUpdate preciseCoverageDeltaUpdate;

    std::vector<std::unique_ptr<ScriptCoverage>> v;
    preciseCoverageDeltaUpdate.SetOccasion("percise")
        .SetResult(std::move(v))
        .SetTimestamp(77);

    std::unique_ptr<PtJson> json = preciseCoverageDeltaUpdate.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    int64_t tmpInt;
    ASSERT_EQ(params->GetInt64("timestamp", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 77);

    std::string tmpStr;
    ASSERT_EQ(params->GetString("occasion", &tmpStr), Result::SUCCESS);
    EXPECT_EQ("percise", tmpStr);

    std::unique_ptr<PtJson> tmpArray;
    ASSERT_EQ(params->GetArray("result", &tmpArray), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerEventsTest, HeapStatsUpdateToJsonTest)
{
    HeapStatsUpdate heapStatsUpdate;
    heapStatsUpdate.SetStatsUpdate(std::vector<int32_t> {});

    std::unique_ptr<PtJson> json = heapStatsUpdate.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    std::unique_ptr<PtJson> tmpArray;
    ASSERT_EQ(params->GetArray("statsUpdate", &tmpArray), Result::SUCCESS);
}

HWTEST_F_L0(DebuggerEventsTest, LastSeenObjectIdToJsonTest)
{
    LastSeenObjectId lastSeenObjectId;
    lastSeenObjectId.SetLastSeenObjectId(10).SetTimestamp(77);

    std::unique_ptr<PtJson> json = lastSeenObjectId.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    int64_t tmpInt64;
    ASSERT_EQ(params->GetInt64("timestamp", &tmpInt64), Result::SUCCESS);
    EXPECT_EQ(tmpInt64, 77);

    int tmpInt;
    ASSERT_EQ(params->GetInt("lastSeenObjectId", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);
}

HWTEST_F_L0(DebuggerEventsTest, ReportHeapSnapshotProgressToJsonTest)
{
    ReportHeapSnapshotProgress reportHeapSnapshotProgress;
    reportHeapSnapshotProgress.SetDone(10).SetTotal(100);

    std::unique_ptr<PtJson> json = reportHeapSnapshotProgress.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    int tmpInt;
    ASSERT_EQ(params->GetInt("done", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 10);

    ASSERT_EQ(params->GetInt("total", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 100);
}

HWTEST_F_L0(DebuggerEventsTest, BufferUsageToJsonTest)
{
    BufferUsage bufferUsage;
    bufferUsage.SetPercentFull(17).SetEventCount(15).SetValue(12);

    std::unique_ptr<PtJson> json = bufferUsage.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    int tmpInt;
    ASSERT_EQ(params->GetInt("percentFull", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 17);

    ASSERT_EQ(params->GetInt("eventCount", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 15);

    ASSERT_EQ(params->GetInt("value", &tmpInt), Result::SUCCESS);
    EXPECT_EQ(tmpInt, 12);
}

HWTEST_F_L0(DebuggerEventsTest, TracingCompleteToJsonTest)
{
    TracingComplete tracingComplete;
    auto traceFormat = std::make_unique<StreamFormat>();
    auto streamCompression = std::make_unique<StreamCompression>();
    tracingComplete.SetDataLossOccurred(true)
                    .SetTraceFormat(std::move(traceFormat))
                    .SetStreamCompression(std::move(streamCompression));

    std::unique_ptr<PtJson> json = tracingComplete.ToJson();
    std::unique_ptr<PtJson> params;
    ASSERT_EQ(json->GetObject("params", &params), Result::SUCCESS);

    bool tmpBool;
    ASSERT_EQ(params->GetBool("dataLossOccurred", &tmpBool), Result::SUCCESS);

    std::string tmpStr;
    ASSERT_EQ(params->GetString("traceFormat", &tmpStr), Result::SUCCESS);
    ASSERT_EQ(params->GetString("streamCompression", &tmpStr), Result::SUCCESS);
}
}  // namespace panda::test