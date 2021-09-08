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

#include <memory>
#include <thread>

#include "ecmascript/tooling/test/test_runner.h"
#include "ecmascript/tooling/test/test_util.h"

namespace panda::tooling::ecmascript::test {
extern const char *GetCurrentTestName();

static std::thread g_debuggerThread;

static std::unique_ptr<TestRunner> g_runner{nullptr};

extern "C" int32_t StartDebugger(const EcmaVM *vm)
{
    const char *testName = GetCurrentTestName();
    g_runner = std::make_unique<TestRunner>(testName, vm);
    g_debuggerThread = std::thread([] {
        TestUtil::WaitForInit();
        g_runner->Run();
    });
    return 0;
}

extern "C" int32_t StopDebugger()
{
    g_debuggerThread.join();
    g_runner->TerminateTest();
    g_runner.reset();
    return 0;
}
}  // namespace panda::tooling::ecmascript::test
