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

HWTEST_F_L0(DebuggerEventsTest, BreakpointResolvedCreateTest)
{
    CString msg;
    std::unique_ptr<BreakpointResolved> breakpointResolved;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(breakpointResolved, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(breakpointResolved, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(breakpointResolved, nullptr);

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(breakpointResolved, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"breakpointId":"00"}})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(breakpointResolved, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
        {"location":{"scriptId":"id2","lineNumber":99}}})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(breakpointResolved, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"breakpointId":"00",
        "location":{"scriptId":"id2","lineNumber":99}}})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(breakpointResolved, nullptr);
}

HWTEST_F_L0(DebuggerEventsTest, BreakpointResolvedToObjectTest)
{
    CString msg;
    std::unique_ptr<BreakpointResolved> breakpointResolved;
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "params");

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"breakpointId":"00",
        "location":{"scriptId":"id2","lineNumber":99}}})";
    breakpointResolved = BreakpointResolved::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());

    ASSERT_NE(breakpointResolved, nullptr);
    Local<ObjectRef> object1 = breakpointResolved->ToObject(ecmaVm);
    Local<JSValueRef> result = object1->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    Local<ObjectRef> object = Local<ObjectRef>(result);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "breakpointId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("00", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "location");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
}

HWTEST_F_L0(DebuggerEventsTest, PausedCreateTest)
{
    CString msg;
    std::unique_ptr<Paused> paused;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(paused, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(paused, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(paused, nullptr);

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(paused, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"reason":"exception"}})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(paused, nullptr);

    msg = CString() +
          R"({"id":0,"method":"Debugger.Test","params":
          {"callFrames":[)" +
          R"({"id":0,"method":"Debugger.Test","params":{
          "callFrameId":10,"functionName":"name0",
          "location":{"scriptId":"id5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}}})" +
          R"(]}})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(paused, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"callFrames":[)" +
          R"({"callFrameId":"10","functionName":"name0",
          "location":{"scriptId":"id5","lineNumber":19},"url":"url7","scopeChain":
          [{"type":"global","object":{"type":")" +
          ObjectType::Object + R"("}}, {"type":"local","object":{"type":")" + ObjectType::Object +
          R"("}}],"this":{"type":")" + ObjectType::Object + R"(","subtype":")" + ObjectSubType::V128 + R"("}})" +
          R"(],"reason":"exception"}})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(paused, nullptr);
}

HWTEST_F_L0(DebuggerEventsTest, PausedToObjectTest)
{
    CString msg;
    std::unique_ptr<Paused> paused;
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "params");

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}],"reason":"exception"}})";
    paused = Paused::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(paused, nullptr);

    Local<ObjectRef> object1 = paused->ToObject(ecmaVm);
    Local<JSValueRef> result = object1->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    Local<ObjectRef> object = Local<ObjectRef>(result);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "reason");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("exception", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "callFrames");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsArray(ecmaVm));
}

HWTEST_F_L0(DebuggerEventsTest, ResumedToObjectTest)
{
    CString msg;
    std::unique_ptr<Resumed> resumed = std::make_unique<Resumed>();
    Local<StringRef> tmpStr;

    Local<ObjectRef> object = resumed->ToObject(ecmaVm);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "method");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    Local<JSValueRef> result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(std::string(resumed->GetName().c_str()), Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "params");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
}

HWTEST_F_L0(DebuggerEventsTest, ScriptFailedToParseCreateTest)
{
    CString msg;
    std::unique_ptr<ScriptFailedToParse> parse;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn:"10}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js"}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00"}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001"}})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]},
    "codeOffset":432,
    "scriptLanguage":"JavaScript"
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]},
    "codeOffset":432
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]}
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/"
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1}
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001"
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]},
    "codeOffset":432,
    "scriptLanguage":"JavaScript",
    "embedderName":"hh"
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);
}

HWTEST_F_L0(DebuggerEventsTest, ScriptFailedToParseToObjectTest)
{
    CString msg;
    std::unique_ptr<ScriptFailedToParse> parse;
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "params");

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]},
    "codeOffset":432,
    "scriptLanguage":"JavaScript",
    "embedderName":"hh"
    }})";
    parse = ScriptFailedToParse::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    Local<ObjectRef> object1 = parse->ToObject(ecmaVm);
    Local<JSValueRef> result = object1->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    Local<ObjectRef> object = Local<ObjectRef>(result);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("00", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("use/test.js", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startLine");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 0);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startColumn");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 4);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endLine");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endColumn");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "executionContextId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 2);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "hash");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("hash0001", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "sourceMapURL");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("usr/", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "hasSourceURL");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "isModule");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "length");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 34);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "codeOffset");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 432);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptLanguage");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("JavaScript", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "embedderName");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("hh", Local<StringRef>(result)->ToString());
}

HWTEST_F_L0(DebuggerEventsTest, ScriptParsedCreateTest)
{
    CString msg;
    std::unique_ptr<ScriptParsed> parse;

    //  abnormal params of null msg
    msg = CString() + R"({})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    // abnormal params of unexist key params
    msg = CString() + R"({"id":0,"method":"Debugger.Test"})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    // abnormal params of null params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    // abnormal params of unknown params.sub-key
    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":{"unknownKey":100}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn:"10}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js"}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00"}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001"}})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    EXPECT_EQ(parse, nullptr);

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "isLiveEdit":true,
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]},
    "codeOffset":432,
    "scriptLanguage":"JavaScript",
    "embedderName":"hh"
    }})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);
}

HWTEST_F_L0(DebuggerEventsTest, ScriptParsedToObjectTest)
{
    CString msg;
    std::unique_ptr<ScriptParsed> parse;
    Local<StringRef> tmpStr = StringRef::NewFromUtf8(ecmaVm, "params");

    msg = CString() + R"({"id":0,"method":"Debugger.Test","params":
    {"scriptId":"00",
    "url":"use/test.js",
    "startLine":0,
    "startColumn":4,
    "endLine":10,
    "endColumn":10,
    "executionContextId":2,
    "hash":"hash0001",
    "executionContextAuxData":{"a":1},
    "isLiveEdit":true,
    "sourceMapURL":"usr/",
    "hasSourceURL":true,
    "isModule":true,
    "length":34,
    "stackTrace":{"callFrames":[{"callFrameId":"10","functionName":"name0",
    "location":{"scriptId":"id5","lineNumber":19},"url":"url7",
    "scopeChain":[{"type":"global","object":{"type":"object"}}, {"type":"local","object":{"type":"object"}}],
    "this":{"type":"object","subtype":"v128"}}]},
    "codeOffset":432,
    "scriptLanguage":"JavaScript",
    "embedderName":"hh"
    }})";
    parse = ScriptParsed::Create(ecmaVm, DispatchRequest(ecmaVm, msg).GetParams());
    ASSERT_NE(parse, nullptr);

    Local<ObjectRef> object1 = parse->ToObject(ecmaVm);
    Local<JSValueRef> result = object1->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());
    Local<ObjectRef> object = Local<ObjectRef>(result);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("00", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "url");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("use/test.js", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startLine");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 0);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "startColumn");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 4);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endLine");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "endColumn");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 10);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "executionContextId");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 2);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "hash");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("hash0001", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsObject());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "isLiveEdit");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "sourceMapURL");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("usr/", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "hasSourceURL");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "isModule");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    ASSERT_TRUE(result->IsTrue());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "length");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 34);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "codeOffset");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ(Local<IntegerRef>(result)->Value(), 432);

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "scriptLanguage");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("JavaScript", Local<StringRef>(result)->ToString());

    tmpStr = StringRef::NewFromUtf8(ecmaVm, "embedderName");
    ASSERT_TRUE(object->Has(ecmaVm, tmpStr));
    result = object->Get(ecmaVm, tmpStr);
    ASSERT_TRUE(!result.IsEmpty() && !result->IsUndefined());
    EXPECT_EQ("hh", Local<StringRef>(result)->ToString());
}
}  // namespace panda::test