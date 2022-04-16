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
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/debugger_service.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/interface/js_debugger.h"
#include "ecmascript/tooling/base/pt_script.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;

namespace panda::test {
class DebuggerScriptTest : public testing::Test {
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
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(DebuggerScriptTest, ScriptIdTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetScriptId(100);
    ASSERT_EQ(script->GetScriptId(), 100U);
}

HWTEST_F_L0(DebuggerScriptTest, FileNameTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetFileName("xx");
    ASSERT_EQ(script->GetFileName(), "xx");
}

HWTEST_F_L0(DebuggerScriptTest, UrlTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetUrl("121");
    ASSERT_EQ(script->GetUrl(), "121");
}

HWTEST_F_L0(DebuggerScriptTest, HashTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetHash("111");
    ASSERT_EQ(script->GetHash(), "111");
}

HWTEST_F_L0(DebuggerScriptTest, ScriptSourceTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetScriptSource("a=1");
    ASSERT_EQ(script->GetScriptSource(), "a=1");
}

HWTEST_F_L0(DebuggerScriptTest, SourceMapUrlTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetSourceMapUrl("192");
    ASSERT_EQ(script->GetSourceMapUrl(), "192");
}

HWTEST_F_L0(DebuggerScriptTest, EndLineTest)
{
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(1, "name_1", "url_1", "source_1");
    script->SetEndLine(200);
    ASSERT_EQ(script->GetEndLine(), 200);
}
}  // namespace panda::test