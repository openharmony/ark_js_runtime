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
#include "js_exception_test.h"
#include "js_single_step_test.h"

namespace panda::ecmascript::tooling::test {
static const char *g_currentTestName = nullptr;

static void RegisterTests()
{
    // Register testcases
    TestUtil::RegisterTest(panda_file::SourceLang::ECMASCRIPT, "JsExceptionTest", GetJsExceptionTest());
    TestUtil::RegisterTest(panda_file::SourceLang::ECMASCRIPT, "JsSingleStepTest", GetJsSingleStepTest());
    TestUtil::RegisterTest(panda_file::SourceLang::ECMASCRIPT, "JsBreakpointTest", GetJsBreakpointTest());
}

CVector<const char *> GetTestList(panda_file::SourceLang language)
{
    RegisterTests();
    CVector<const char *> res;
    auto &tests = TestUtil::GetTests();
    auto languageIt = tests.find(language);
    if (languageIt == tests.end()) {
        return {};
    }

    for (const auto &entry : languageIt->second) {
        res.push_back(entry.first);
    }
    return res;
}

void SetCurrentTestName(const char *testName)
{
    g_currentTestName = testName;
}

const char *GetCurrentTestName()
{
    return g_currentTestName;
}

std::pair<CString, CString> GetTestEntryPoint(const char *testName)
{
    return TestUtil::GetTest(testName)->GetEntryPoint();
}
}  // namespace panda::ecmascript::tooling::test
