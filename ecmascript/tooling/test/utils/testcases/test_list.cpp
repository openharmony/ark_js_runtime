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

#include "ecmascript/tooling/test/utils/testcases/test_list.h"

#include "ecmascript/tooling/test/utils/test_util.h"

// testcase list
#include "js_breakpoint_test.h"
#include "js_breakpoint_arrow_test.h"
#include "js_breakpoint_async_test.h"
#include "js_exception_test.h"
#include "js_single_step_test.h"
#include "js_syntaxException_test.h"
#include "js_throwException_test.h"

namespace panda::ecmascript::tooling::test {
static std::string g_currentTestName = "";

static void RegisterTests()
{
    // Register testcases
    TestUtil::RegisterTest("JsExceptionTest", GetJsExceptionTest());
    TestUtil::RegisterTest("JsSingleStepTest", GetJsSingleStepTest());
    TestUtil::RegisterTest("JsBreakpointTest", GetJsBreakpointTest());
    TestUtil::RegisterTest("JsBreakpointAsyncTest", GetJsBreakpointAsyncTest());
    TestUtil::RegisterTest("JsBreakpointArrowTest", GetJsBreakpointArrowTest());
    TestUtil::RegisterTest("JsSyntaxExceptionTest", GetJsSyntaxExceptionTest());
    TestUtil::RegisterTest("JsThrowExceptionTest", GetJsThrowExceptionTest());
}

std::vector<const char *> GetTestList()
{
    RegisterTests();
    std::vector<const char *> res;

    auto &tests = TestUtil::GetTests();
    for (const auto &entry : tests) {
        res.push_back(entry.first.c_str());
    }
    return res;
}

void SetCurrentTestName(const std::string &testName)
{
    g_currentTestName = testName;
}

std::string GetCurrentTestName()
{
    return g_currentTestName;
}

std::pair<std::string, std::string> GetTestEntryPoint(const std::string &testName)
{
    return TestUtil::GetTest(testName)->GetEntryPoint();
}
}  // namespace panda::ecmascript::tooling::test
