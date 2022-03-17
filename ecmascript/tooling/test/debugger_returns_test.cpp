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
using namespace panda::tooling::ecmascript;

namespace panda::test {
// Duplicate name of panda::ecmascript::PropertyDescriptor in js_object-inl.h
using panda::tooling::ecmascript::PropertyDescriptor;
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        ecmaVm = EcmaVM::Cast(instance);
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

HWTEST_F_L0(DebuggerReturnsTest, EnableReturnsToObjectTest)
{
    std::unique_ptr<EnableReturns> enableReturns = std::make_unique<EnableReturns>("100");
    ASSERT_NE(enableReturns, nullptr);
    Local<ObjectRef> enableObject = enableReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "debuggerId");
    ASSERT_TRUE(enableObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = enableObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(std::string("100"), Local<StringRef>(result)->ToString());
}

HWTEST_F_L0(DebuggerReturnsTest, SetBreakpointByUrlReturnsToObjectTest)
{
    auto locations = CVector<std::unique_ptr<Location>>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId("id_1");
    locations.emplace_back(std::move(location));
    ASSERT_EQ(locations.back()->GetScriptId(), "id_1");

    std::unique_ptr<SetBreakpointByUrlReturns> setBreakpointByUrlReturns
                     = std::make_unique<SetBreakpointByUrlReturns>("11", std::move(locations));
    ASSERT_NE(setBreakpointByUrlReturns, nullptr);
    Local<ObjectRef> setObject = setBreakpointByUrlReturns->ToObject(ecmaVm);
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "breakpointId");
    ASSERT_TRUE(setObject->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = setObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(std::string("11"), Local<StringRef>(result)->ToString());
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
    EXPECT_EQ(std::string("source_1"), Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "bytecode");
    ASSERT_TRUE(scriptObject->Has(ecmaVm, tmpStr));
    result = scriptObject->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(std::string("bytecode_1"), Local<StringRef>(result)->ToString());
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
    EXPECT_EQ(std::string("breakpointId_1"), Local<StringRef>(result)->ToString());
    
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
    EXPECT_EQ(std::string("111"), Local<StringRef>(result)->ToString());
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
    exceptionDetails->SetScriptId("id_5");
    ASSERT_EQ(exceptionDetails->GetScriptId(), "id_5");
}

HWTEST_F_L0(DebuggerReturnsTest, GetPropertiesReturnsToObjectTest)
{
    auto descriptor = CVector<std::unique_ptr<PropertyDescriptor>>();
    std::unique_ptr<PropertyDescriptor> propertyDescriptor = std::make_unique<PropertyDescriptor>();
    std::unique_ptr<ExceptionDetails> exceptionDetails = std::make_unique<ExceptionDetails>();
    std::unique_ptr<GetPropertiesReturns> getPropertiesReturns = std::make_unique
                                                        <GetPropertiesReturns>(std::move(descriptor));
    ASSERT_NE(getPropertiesReturns, nullptr);
    exceptionDetails->SetScriptId("id_6");
    ASSERT_EQ(exceptionDetails->GetScriptId(), "id_6");

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
}  // namespace panda::test