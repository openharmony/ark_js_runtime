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

HWTEST_F_L0(DebuggerReturnsTest, EnableReturnsToObjectTest)
{
    std::unique_ptr<EnableReturns> enableReturns = std::make_unique<EnableReturns>(100U);
    ASSERT_NE(enableReturns, nullptr);
    Local<ObjectRef> enableObject = enableReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "debuggerId");
    ASSERT_TRUE(enableObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = enableObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("100"), DebuggerApi::ToCString(result));
}

HWTEST_F_L0(DebuggerReturnsTest, SetBreakpointByUrlReturnsToObjectTest)
{
    auto locations = CVector<std::unique_ptr<Location>>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(1);
    locations.emplace_back(std::move(location));
    ASSERT_EQ(locations.back()->GetScriptId(), 1U);

    std::unique_ptr<SetBreakpointByUrlReturns> setBreakpointByUrlReturns
                     = std::make_unique<SetBreakpointByUrlReturns>("11", std::move(locations));
    ASSERT_NE(setBreakpointByUrlReturns, nullptr);
    Local<ObjectRef> setObject = setBreakpointByUrlReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "breakpointId");
    ASSERT_TRUE(setObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = setObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("11"), DebuggerApi::ToCString(result));
}

HWTEST_F_L0(DebuggerReturnsTest, EvaluateOnCallFrameReturnsToObjectTest)
{
    std::unique_ptr<RemoteObject> result1 = std::make_unique<RemoteObject>();
    std::unique_ptr<ExceptionDetails> exceptionDetails = std::make_unique<ExceptionDetails>();
    std::unique_ptr<EvaluateOnCallFrameReturns> evaluateOnCallFrameReturns
                     = std::make_unique<EvaluateOnCallFrameReturns>(std::move(result1), std::move(exceptionDetails));
    ASSERT_NE(evaluateOnCallFrameReturns, nullptr);
    Local<ObjectRef> callObject = evaluateOnCallFrameReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "result");
    ASSERT_TRUE(callObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = callObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_EQ(std::move(result1), nullptr);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "exceptionDetails");
    ASSERT_TRUE(callObject->Has(ecmaVm, tmpStr));
    result = callObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_EQ(std::move(exceptionDetails), nullptr);
}

HWTEST_F_L0(DebuggerReturnsTest, GetPossibleBreakpointsReturnsToObjectTest)
{
    auto locations = CVector<std::unique_ptr<BreakLocation>>();
    std::unique_ptr<BreakLocation> breakLocation = std::make_unique<BreakLocation>();
    std::unique_ptr<GetPossibleBreakpointsReturns> getPossibleBreakpointsReturns = std::make_unique
                                                    <GetPossibleBreakpointsReturns>(std::move(locations));
    Local<ArrayRef> getObject = getPossibleBreakpointsReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "locations");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
}

HWTEST_F_L0(DebuggerReturnsTest, GetScriptSourceReturnsToObjectTest)
{
    std::unique_ptr<GetScriptSourceReturns> getScriptSourceReturns = std::make_unique
                                                                     <GetScriptSourceReturns>("source_1", "bytecode_1");
    ASSERT_NE(getScriptSourceReturns, nullptr);
    Local<ObjectRef> scriptObject = getScriptSourceReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptSource");
    ASSERT_TRUE(scriptObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = scriptObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("source_1"), DebuggerApi::ToCString(result));

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "bytecode");
    ASSERT_TRUE(scriptObject->Has(ecmaVm, tmpStr));
    result = scriptObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("bytecode_1"), DebuggerApi::ToCString(result));
}

HWTEST_F_L0(DebuggerReturnsTest, RestartFrameReturnsToObjectTest)
{
    auto callFrames = CVector<std::unique_ptr<CallFrame>>();
    std::unique_ptr<CallFrame> callFrame = std::make_unique<CallFrame>();
    std::unique_ptr<RestartFrameReturns> restartFrameReturns = std::make_unique
                                                                     <RestartFrameReturns>(std::move(callFrames));
    Local<ArrayRef> restartObject = restartFrameReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrames");
    ASSERT_TRUE(restartObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = restartObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
}

HWTEST_F_L0(DebuggerReturnsTest, SetBreakpointReturnsToObjectTest)
{
    std::unique_ptr<Location> location = std::make_unique<Location>();
    std::unique_ptr<SetBreakpointReturns> setBreakpointReturns
                     = std::make_unique<SetBreakpointReturns>("breakpointId_1", std::move(location));
    ASSERT_NE(setBreakpointReturns, nullptr);
    Local<ObjectRef> breakObject = setBreakpointReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "breakpointId");
    ASSERT_TRUE(breakObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = breakObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("breakpointId_1"), DebuggerApi::ToCString(result));
    
    tmpStr = StringRef::NewFromUtf8(ecmaVm, "actualLocation");
    ASSERT_TRUE(breakObject->Has(ecmaVm, tmpStr));
    result = breakObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_EQ(std::move(location), nullptr);
}

HWTEST_F_L0(DebuggerReturnsTest, SetInstrumentationBreakpointReturnsToObjectTest)
{
    std::unique_ptr<SetInstrumentationBreakpointReturns> setInstrumentationBreakpointReturns
                     = std::make_unique<SetInstrumentationBreakpointReturns>("111");
    ASSERT_NE(setInstrumentationBreakpointReturns, nullptr);
    Local<ObjectRef> instrumentationObject = setInstrumentationBreakpointReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "breakpointId");
    ASSERT_TRUE(instrumentationObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = instrumentationObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("111"), DebuggerApi::ToCString(result));
}

HWTEST_F_L0(DebuggerReturnsTest, SetScriptSourceReturnsToObjectTest)
{
    auto callFrames = CVector<std::unique_ptr<CallFrame>>();
    std::unique_ptr<CallFrame> callFrame = std::make_unique<CallFrame>();
    std::unique_ptr<ExceptionDetails> exceptionDetails = std::make_unique<ExceptionDetails>();
    std::unique_ptr<SetScriptSourceReturns> setScriptSourceReturns = std::make_unique
                                                                <SetScriptSourceReturns>(std::move(callFrames));
    Local<ArrayRef> setObject = setScriptSourceReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrames");
    ASSERT_TRUE(setObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = setObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));

    ASSERT_NE(setScriptSourceReturns, nullptr);
    exceptionDetails->SetScriptId(5);
    ASSERT_EQ(exceptionDetails->GetScriptId(), 5U);
}

HWTEST_F_L0(DebuggerReturnsTest, GetPropertiesReturnsToObjectTest)
{
    auto descriptor = CVector<std::unique_ptr<PropertyDescriptor>>();
    std::unique_ptr<PropertyDescriptor> propertyDescriptor = std::make_unique<PropertyDescriptor>();
    std::unique_ptr<ExceptionDetails> exceptionDetails = std::make_unique<ExceptionDetails>();
    std::unique_ptr<GetPropertiesReturns> getPropertiesReturns = std::make_unique
                                                        <GetPropertiesReturns>(std::move(descriptor));
    ASSERT_NE(getPropertiesReturns, nullptr);
    exceptionDetails->SetScriptId(6);
    ASSERT_EQ(exceptionDetails->GetScriptId(), 6U);

    Local<ArrayRef> getObject = getPropertiesReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "result");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));

    auto internalDescripties = CVector<std::unique_ptr<InternalPropertyDescriptor>>();
    std::unique_ptr<InternalPropertyDescriptor> internalPropertyDescriptor =
                                                std::make_unique<InternalPropertyDescriptor>();
    internalPropertyDescriptor->SetName("filename1");
    internalDescripties.emplace_back(std::move(internalPropertyDescriptor));
    ASSERT_EQ(internalDescripties.back()->GetName(), "filename1");

    auto privateProperties = CVector<std::unique_ptr<PrivatePropertyDescriptor>>();
    std::unique_ptr<PrivatePropertyDescriptor> privatePropertyDescriptor =
                                               std::make_unique<PrivatePropertyDescriptor>();
    privatePropertyDescriptor->SetName("filename2");
    privateProperties.emplace_back(std::move(privatePropertyDescriptor));
    ASSERT_EQ(privateProperties.back()->GetName(), "filename2");
}

HWTEST_F_L0(DebuggerReturnsTest, StopSamplingReturnsToObjectTest)
{
    std::unique_ptr<SamplingHeapProfile> profile = std::make_unique<SamplingHeapProfile>();
    std::unique_ptr<StopSamplingReturns> stopSamplingReturns =
                                         std::make_unique<StopSamplingReturns>(std::move(profile));
    ASSERT_NE(stopSamplingReturns, nullptr);
    Local<ObjectRef> object = stopSamplingReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "profile");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
}

HWTEST_F_L0(DebuggerReturnsTest, GetHeapObjectIdReturnsToObjectTest)
{
    std::unique_ptr<GetHeapObjectIdReturns> getHeapObjectIdReturns = std::make_unique<GetHeapObjectIdReturns>(10);
    ASSERT_NE(getHeapObjectIdReturns, nullptr);
    
    Local<ObjectRef> object = getHeapObjectIdReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "heapSnapshotObjectId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(CString("10"), DebuggerApi::ToCString(result));
}

HWTEST_F_L0(DebuggerReturnsTest, GetObjectByHeapObjectIdReturnsToObjectTest)
{
    std::unique_ptr<RemoteObject> remoteObjectResult = std::make_unique<RemoteObject>();
    std::unique_ptr<GetObjectByHeapObjectIdReturns> getObjectByHeapObjectIdReturns =
                                    std::make_unique<GetObjectByHeapObjectIdReturns>(std::move(remoteObjectResult));
    ASSERT_NE(getObjectByHeapObjectIdReturns, nullptr);

    Local<ObjectRef> object = getObjectByHeapObjectIdReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "result");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));

    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_EQ(std::move(remoteObjectResult), nullptr);
}

HWTEST_F_L0(DebuggerReturnsTest, StopReturnsToObjectTest)
{
    std::unique_ptr<Profile> profile = std::make_unique<Profile>();
    std::unique_ptr<StopReturns> stopReturns= std::make_unique<StopReturns>(std::move(profile));
    ASSERT_NE(stopReturns, nullptr);
    Local<ObjectRef> temp = stopReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "profile");
    ASSERT_TRUE(temp->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = temp->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
}

HWTEST_F_L0(DebuggerReturnsTest, GetHeapUsageReturnsToObjectTest)
{
    double usedSize = 1;
    double totalSize = 1;
    std::unique_ptr<GetHeapUsageReturns> getHeapUsageReturns =
        std::make_unique<GetHeapUsageReturns>(usedSize, totalSize);
    ASSERT_NE(getHeapUsageReturns, nullptr);
    Local<ObjectRef> getObject = getHeapUsageReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "usedSize");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<NumberRef>(result)->Value(), 1);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "totalSize");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    result = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<NumberRef>(result)->Value(), 1);
}

HWTEST_F_L0(DebuggerReturnsTest, GetBestEffortCoverageReturnsToObjectTest)
{
    auto result = CVector<std::unique_ptr<ScriptCoverage>>();
    std::unique_ptr<ScriptCoverage> scriptCoverage = std::make_unique<ScriptCoverage>();
    std::unique_ptr<GetBestEffortCoverageReturns> getBestEffortCoverageReturns =
                                                std::make_unique<GetBestEffortCoverageReturns>(std::move(result));
    Local<ArrayRef> getObject = getBestEffortCoverageReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "result");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> tmpResult = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    ASSERT_TRUE(tmpResult->IsArray(ecmaVm));
}

HWTEST_F_L0(DebuggerReturnsTest, StartPreciseCoverageReturnsToObjectTest)
{
    std::unique_ptr<StartPreciseCoverageReturns> startPreciseCoverageReturns
                     = std::make_unique<StartPreciseCoverageReturns>(1001);
    ASSERT_NE(startPreciseCoverageReturns, nullptr);
    Local<ObjectRef> getObject = startPreciseCoverageReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "timestamp");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 1001);
}

HWTEST_F_L0(DebuggerReturnsTest, TakePreciseCoverageReturnsToObjectTest)
{
    auto coverage = CVector<std::unique_ptr<ScriptCoverage>>();
    std::unique_ptr<TakePreciseCoverageReturns> takePreciseCoverageReturns =
                                                std::make_unique<TakePreciseCoverageReturns>(std::move(coverage), 1001);
    ASSERT_NE(takePreciseCoverageReturns, nullptr);
    Local<ArrayRef> getObject = takePreciseCoverageReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "result");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));

    Local<ObjectRef> instrumentationObject = takePreciseCoverageReturns->ToObject(ecmaVm);
    Local<StringRef> tmperStr = StringRef::NewFromUtf8(ecmaVm, "timestamp");
    ASSERT_TRUE(instrumentationObject->Has(ecmaVm, tmperStr));
    Local<JSValueRef> tmpResult = instrumentationObject->Get(ecmaVm, tmperStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(tmpResult)->Value(), 1001);
}

HWTEST_F_L0(DebuggerReturnsTest, TakeTypeProfileturnsToObjectTest)
{
    auto result = CVector<std::unique_ptr<ScriptTypeProfile>>();
    std::unique_ptr<ScriptTypeProfile> scriptTypeProfile = std::make_unique<ScriptTypeProfile>();
    std::unique_ptr<TakeTypeProfileturns> takeTypeProfileturns = std::make_unique
                                                    <TakeTypeProfileturns>(std::move(result));
    Local<ArrayRef> getObject = takeTypeProfileturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "result");
    ASSERT_TRUE(getObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> tmpResult = getObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!tmpResult.IsEmpty() && !tmpResult->IsUndefined());
    ASSERT_TRUE(tmpResult->IsArray(ecmaVm));
}
}  // namespace panda::test