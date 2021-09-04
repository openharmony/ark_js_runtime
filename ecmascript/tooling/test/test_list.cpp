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

#include "ecmascript/tooling/test/test_list.h"
#include "ecmascript/tooling/test/api_tests/api_tests.h"
#include "ecmascript/tooling/test/test_util.h"

namespace panda::tooling::ecmascript::test {
static const char *g_currentTestName = nullptr;

static void RegisterTests()
{
    TestUtil::RegisterTest(panda_file::SourceLang::ECMASCRIPT, "JsBreakpoint", GetJsBreakpointTest());
    TestUtil::RegisterTest(panda_file::SourceLang::ECMASCRIPT, "JsSingleStepTest", GetJsSingleStepTest());
}

std::vector<const char *> GetTestList(panda_file::SourceLang language)
{
    RegisterTests();
    std::vector<const char *> res;
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
}  // namespace panda::tooling::ecmascript::test
